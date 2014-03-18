/*
 * Copyright (c) 2012-2014 Martin Rödel aka Yomin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define _BSD_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>

#include "tkbio.h"
#include "tkbio_def.h"
#include "tkbio_fb.h"

#include <tsp.h>

#include "tkbio_layout_default.h"

#include "tkbio_type_button.h"
#include "tkbio_type_slider.h"

#define TYPEFUNC(e, m, r, ...) \
    do { \
        switch((e)->type & TKBIO_LAYOUT_MASK_TYPE) \
        { \
        case TKBIO_LAYOUT_TYPE_NOP: \
            break; \
        case TKBIO_LAYOUT_TYPE_CHAR: \
        case TKBIO_LAYOUT_TYPE_GOTO: \
        case TKBIO_LAYOUT_TYPE_HOLD: \
        case TKBIO_LAYOUT_TYPE_TOGGLE: \
        case TKBIO_LAYOUT_TYPE_SYSTEM: \
            r tkbio_type_button_ ## m (__VA_ARGS__); \
            break; \
        case TKBIO_LAYOUT_TYPE_HSLIDER: \
        case TKBIO_LAYOUT_TYPE_VSLIDER: \
            r tkbio_type_slider_ ## m (__VA_ARGS__); \
            break; \
        } \
    } \
    while(0)

struct tkbio_global tkbio;

void tkbio_set_signal_handler(void handler(int signal))
{
    tkbio.custom_signal_handler = handler;
}

void tkbio_signal_handler(int signal)
{
    switch(signal)
    {
    case SIGALRM:
        tkbio.pause = 0;
        break;
    case SIGINT:
    {
        if(tkbio.custom_signal_handler)
            tkbio.custom_signal_handler(signal);
        else
        {
            tkbio_finish();
            exit(128+SIGINT);
        }
        break;
    }
    default:
        if(tkbio.custom_signal_handler)
            tkbio.custom_signal_handler(signal);
    }
}

void tkbio_init_partner()
{
    int i, j, k, width, size;
    
    const struct tkbio_mapelem *map;
    const struct tkbio_mapelem *elem;
    struct tkbio_point p, p2, *pp;
    struct tkbio_partner **partner, **partner_prev;
    
    // create for every group of buttons a vector
    // filled with tkbio_points of all buttons
    // every tkbio.save[i][x].partner->connect of a button points
    // to the same vector
    // the vectors and points are saved in layout format
    
    tkbio.save = malloc(tkbio.layout.size*sizeof(struct tkbio_save*));
    
    for(i=0; i<tkbio.layout.size; i++)
    {
        width = tkbio.layout.maps[i].width;
        size = tkbio.layout.maps[i].height*width;
        tkbio.save[i] = calloc(size, sizeof(struct tkbio_save));
        
        for(j=0; j<size; j++)
        {
            map = tkbio.layout.maps[i].map;
            elem = &map[j];
            p = (struct tkbio_point){j/width, j%width, elem};
            
            if(j%width>0 && elem->connect & TKBIO_LAYOUT_CONNECT_LEFT)
            {
                partner_prev = &tkbio.save[i][j-1].partner;
                if(!*partner_prev) // left unconnected
                {
                    *partner_prev = calloc(1, sizeof(struct tkbio_partner));
                    vector_init(sizeof(struct tkbio_point),
                        &(*partner_prev)->connect);
                    p2 = (struct tkbio_point){(j-1)/width,
                        (j-1)%width, &map[j-1]};
                    vector_push(&p2, (*partner_prev)->connect);
                }
                tkbio.save[i][j].partner = *partner_prev;
                vector_push(&p, (*partner_prev)->connect);
            }
            if(j>=width && elem->connect & TKBIO_LAYOUT_CONNECT_UP)
            {
                partner_prev = &tkbio.save[i][j-width].partner;
                partner = &tkbio.save[i][j].partner;
                if(*partner) // already left connected
                {
                    if(!*partner_prev) // up unconnected
                    {
                        p2 = (struct tkbio_point){j/width-1,
                            j%width, &map[j]};
                        *partner_prev = *partner;
                        vector_push(&p2, (*partner)->connect);
                        continue;
                    }
                    if(*partner_prev == *partner)
                        continue;
                    // merge vectors
                    if(vector_size((*partner_prev)->connect)
                        > vector_size((*partner)->connect))
                    {
                        *partner = *partner_prev;
                        partner_prev = &tkbio.save[i][j].partner;
                    }
                    for(k=0; k<vector_size((*partner)->connect); k++)
                    {
                        pp = vector_at(k, (*partner)->connect);
                        vector_push(pp, (*partner_prev)->connect);
                        tkbio.save[i][pp->y*width+pp->x].partner
                            = *partner_prev;
                    }
                    vector_finish((*partner)->connect);
                    free(*partner);
                }
                else
                {
                    if(!*partner_prev) // up unconnected
                    {
                        *partner_prev = calloc(1, sizeof(struct tkbio_partner));
                        vector_init(sizeof(struct tkbio_point),
                            &(*partner_prev)->connect);
                        p2 = (struct tkbio_point){j/width-1,
                            j%width, &map[j-width]};
                        vector_push(&p2, (*partner_prev)->connect);
                    }
                    *partner = *partner_prev;
                    vector_push(&p, (*partner)->connect);
                }
            }
        }
    }
}

void tkbio_init_save_data()
{
    int i, y, x, pos;
    const struct tkbio_map *map;
    struct tkbio_save *save;
    const struct tkbio_mapelem *elem;
    
    for(i=0; i<tkbio.layout.size; i++)
    {
        map = &tkbio.layout.maps[i];
        for(y=0; y<map->height; y++)
            for(x=0; x<map->width; x++)
            {
                pos = y*map->width+x;
                save = &tkbio.save[i][pos];
                elem = &map->map[pos];
                TYPEFUNC(elem, init, , y, x, map, elem, save);
            }
    }
}

int tkbio_open_socket(int tries)
{
    struct sockaddr_un addr;
    
    VERBOSE(printf("[TKBIO] Connecting to rpc socket\n"));
    
    if((tkbio.sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        VERBOSE(perror("[TKBIO] Failed to open rpc socket"));
        return TKBIO_ERROR_RPC_OPEN;
    }
    
    addr.sun_family = AF_UNIX;
    sprintf(addr.sun_path, "%s/%s", tkbio.tsp, TSP_RPC);
    
    while(connect(tkbio.sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        VERBOSE(perror("[TKBIO] Failed to connect rpc socket"));
        if(errno != EADDRINUSE)
            return TKBIO_ERROR_RPC_OPEN;
        
        if(tries-- == 1)
        {
            VERBOSE(perror("[TKBIO] Unable to connect rpc socket"));
            return TKBIO_ERROR_RPC_OPEN;
        }
        sleep(1);
        VERBOSE(printf("[TKBIO] Retry connecting to rpc socket\n"));
    }
    return 0;
}

struct tkbio_config tkbio_args(int *argc, char *argv[], struct tkbio_config config)
{
    struct option fboption[] =
    {
        { "tkbio-fb", 1, 0, 'f' },      // path to framebuffer
        { "tkbio-tsp", 1, 0, 't' },     // path to tsp directory
        { "tkbio-verbose", 0, 0, 'v' }, // verbose messages
        { "tkbio-format", 1, 0, 'r' },  // p: portrait, l: landscape
        {0, 0, 0, 0}
    };
    int opt, x, y;
    opterr = 0;

    while((opt = getopt_long(*argc, argv, "", fboption, 0)) != -1)
    {
        switch(opt)
        {
        case 'f':
            config.fb = optarg;
            break;
        case 't':
            config.tsp = optarg;
            break;
        case 'v':
            config.verbose = 1;
            break;
        case 'r':
            config.format = TKBIO_FORMAT_LANDSCAPE;
            if(optarg[0] == 'p')
                config.format = TKBIO_FORMAT_PORTRAIT;
            break;
        default:
            continue;
        }
        y = optarg ? 2 : 1;
        optind -= y;
        for(x=optind; x<*argc-y; x++)
            argv[x] = argv[x+y];
        *argc -= y;
    }
    opterr = 1;
    optind = 1;
    return config;
}

void tkbio_init_screen()
{
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    const struct tkbio_mapelem *elem;
    int y, x, height, width;
    char sim_tmp = 'x'; // content irrelevant
    
    tkbio_get_sizes_current(&height, &width, 0, 0, 0, 0);
    
    for(y=0; y<map->height; y++)
        for(x=0; x<map->width; x++)
        {
            elem = &map->map[y*map->width+x];
            
            if(elem->type & TKBIO_LAYOUT_OPTION_COPY)
            {
                if(elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                    tkbio_layout_draw_connect(y*height, x*width, y, x,
                        height, width, elem->color, elem->connect,
                        DENSITY, 0);
            }
            else
            {
                if(elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                    tkbio_layout_draw_rect_connect(y*height, x*width,
                        y, x, height, width, elem->color,
                        elem->connect, DENSITY, 0);
                else
                    tkbio_layout_draw_rect(y*height, x*width,
                        height, width, elem->color >> 4, DENSITY, 0);
            }
        }
    
    // notify framebuffer for redraw
    if(tkbio.sim)
        send(tkbio.fb.sock, &sim_tmp, 1, 0);
}

int tkbio_init_custom(const char *name, struct tkbio_config config)
{
    int ret;
    struct sockaddr_un addr;
    
    tkbio.verbose = config.verbose;
    
    if(config.verbose)
    {
        printf("[TKBIO] init\n");
        printf("[TKBIO] fb: %s\n", config.fb);
        printf("[TKBIO] tsp: %s\n", config.tsp);
        printf("[TKBIO] format: %s\n", config.format
            == TKBIO_FORMAT_LANDSCAPE ? "landscape" : "portrait");
        printf("[TKBIO] options:\n");
        printf("[TKBIO]   initial print: %s\n", config.options
            & TKBIO_OPTION_NO_INITIAL_PRINT ? "no" : "yes");
    }
    
    tkbio.tsp = config.tsp;
    
    // open rpc socket
    tkbio.name = name;
    if((ret = tkbio_open_socket(-1)) < 0)
        return ret;
    
    // open framebuffer
    if(strncmp(config.fb+strlen(config.fb)-4, ".ipc", 4))
    {
        tkbio.sim = 0;
        VERBOSE(printf("[TKBIO] Opening framebuffer\n"));
        if((tkbio.fb.fd = open(config.fb, O_RDWR)) == -1)
        {
            VERBOSE(perror("[TKBIO] Failed to open framebuffer"));
            close(tkbio.sock);
            return TKBIO_ERROR_FB_OPEN;
        }
        if(ioctl(tkbio.fb.fd, FBIOGET_VSCREENINFO, &(tkbio.fb.vinfo)) == -1)
        {
            VERBOSE(perror("[TKBIO] Failed to get variable screeninfo"));
            close(tkbio.sock);
            close(tkbio.fb.fd);
            return TKBIO_ERROR_FB_VINFO;
        }
        if(ioctl(tkbio.fb.fd, FBIOGET_FSCREENINFO, &(tkbio.fb.finfo)) == -1)
        {
            VERBOSE(perror("[TKBIO] Failed to get fixed screeninfo"));
            close(tkbio.sock);
            close(tkbio.fb.fd);
            return TKBIO_ERROR_FB_FINFO;
        }
    }
    else
    {
        tkbio.sim = 1;
        VERBOSE(printf("[TKBIO] Simulating framebuffer\n"));
        tkbio.fb.sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if(tkbio.fb.sock == -1)
        {
            VERBOSE(perror("[TKBIO] Failed to open framebuffer socket"));
            return TKBIO_ERROR_FB_OPEN;
        }
        
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, config.fb);
        
        if(connect(tkbio.fb.sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
        {
            VERBOSE(perror("[TKBIO] Failed to connect framebuffer socket"));
            return TKBIO_ERROR_FB_OPEN;
        }
        
        recv(tkbio.fb.sock, tkbio.fb.shm, sizeof(tkbio.fb.shm), 0);
        recv(tkbio.fb.sock, &tkbio.fb.vinfo, sizeof(struct fb_var_screeninfo), 0);
        recv(tkbio.fb.sock, &tkbio.fb.finfo, sizeof(struct fb_fix_screeninfo), 0);
        
        if((tkbio.fb.fd = shm_open(tkbio.fb.shm, O_CREAT|O_RDWR, 0644)) == -1)
        {
            VERBOSE(perror("[TKBIO] Failed to open shared memory"));
            return TKBIO_ERROR_FB_OPEN;
        }
    }
    
    tkbio.fb.bpp = tkbio.fb.vinfo.bits_per_pixel/8;
    tkbio.fb.size = tkbio.fb.vinfo.xres*tkbio.fb.vinfo.yres*tkbio.fb.bpp;
    
    if(config.verbose)
    {
        printf("[TKBIO] framebuffer info:\n");
        printf("[TKBIO]   size %i x %i\n", tkbio.fb.vinfo.xres,tkbio.fb.vinfo.yres);
        printf("[TKBIO]   bytes per pixel %i\n", tkbio.fb.bpp);
        printf("[TKBIO]   line length %i\n", tkbio.fb.finfo.line_length);
    }
    
    if(tkbio.sim && ftruncate(tkbio.fb.fd, tkbio.fb.size) == -1)
    {
        VERBOSE(perror("[TKBIO] Failed to truncate shared memory"));
        return TKBIO_ERROR_FB_OPEN;
    }
    
    if((tkbio.fb.ptr = (unsigned char*) mmap(0, tkbio.fb.size,
        PROT_READ|PROT_WRITE, MAP_SHARED, tkbio.fb.fd, 0)) == MAP_FAILED)
    {
        VERBOSE(perror("[TKBIO] Failed to mmap framebuffer"));
        close(tkbio.sock);
        close(tkbio.fb.fd);
        return TKBIO_ERROR_FB_MMAP;
    }
    
    tkbio.fb.ptr += tkbio.fb.vinfo.xoffset*tkbio.fb.bpp
                  + tkbio.fb.vinfo.yoffset*tkbio.fb.finfo.line_length;
    
    // save layout and format
    tkbio.layout = config.layout;
    tkbio.format = config.format;
    
    // calculate partner connects, init save data
    tkbio_init_partner();
    tkbio_init_save_data();
    
    // init parser
    tkbio.parser.map = config.layout.start;
    tkbio.parser.toggle = 0;
    tkbio.parser.hold = 0;
    tkbio.parser.pressed = 0;
    
    // else
    tkbio.pause = 0;
    tkbio.custom_signal_handler = 0;
    signal(SIGALRM, tkbio_signal_handler);
    signal(SIGINT, tkbio_signal_handler);
    
    // print initial screen
    if(!(config.options & TKBIO_OPTION_NO_INITIAL_PRINT))
        tkbio_init_screen();
    
    VERBOSE(printf("[TKBIO] init done\n"));
    
    // return socket for manual polling
    return tkbio.sock;
}

int tkbio_init_custom_args(const char *name, struct tkbio_config config, int *argc, char *argv[])
{
    return tkbio_init_custom(name, tkbio_args(argc, argv, config));
}

struct tkbio_config tkbio_config_default()
{
    struct tkbio_config config;
    config.fb = "/dev/fb0";
    config.tsp = TSP_PWD;
    config.layout = tkbLayoutDefault;
    config.format = TKBIO_FORMAT_LANDSCAPE;
    config.options = 0;
    config.verbose = 0;
    return config;
}

struct tkbio_config tkbio_config_args(int *argc, char *argv[])
{
    return tkbio_args(argc, argv, tkbio_config_default());
}

int tkbio_init_default(const char *name)
{
    return tkbio_init_custom(name, tkbio_config_default());
}

int tkbio_init_args(const char *name, int *argc, char *argv[])
{
    return tkbio_init_custom(name, tkbio_args(argc, argv, tkbio_config_default()));
}

int tkbio_init_layout(const char *name, struct tkbio_layout layout)
{
    struct tkbio_config config = tkbio_config_default();
    config.layout = layout;
    return tkbio_init_custom(name, config);
}

int tkbio_init_layout_args(const char *name, struct tkbio_layout layout, int *argc, char *argv[])
{
    struct tkbio_config config = tkbio_config_default();
    config.layout = layout;
    return tkbio_init_custom(name, tkbio_args(argc, argv, config));
}

void tkbio_finish()
{
    unsigned char cmd = TSP_CMD_REMOVE;
    
    int i, j, k, size;
    struct tkbio_point *p;
    struct tkbio_partner *partner;
    struct tkbio_save *save;
    const struct tkbio_mapelem *elem;
    const struct tkbio_map *map;
    
    VERBOSE(printf("[TKBIO] finish\n"));
    
    send(tkbio.sock, &cmd, sizeof(unsigned char), 0);
    close(tkbio.sock);
    
    if(tkbio.sim)
    {
        close(tkbio.fb.fd);
        close(tkbio.fb.sock);
    }
    else
        close(tkbio.fb.fd);
    
    for(i=0; i<tkbio.layout.size; i++)
    {
        map = &tkbio.layout.maps[i];
        size = map->height * map->width;
        for(j=0; j<size; j++)
        {
            save = &tkbio.save[i][j];
            partner = save->partner;
            elem = &map->map[j];
            
            TYPEFUNC(elem, finish, , save);
            
            if(!save->partner)
                continue;
            
            for(k=0; k<vector_size(partner->connect); k++)
            {
                p = vector_at(k, partner->connect);
                tkbio.save[i][p->y*map->width+p->x].partner = 0;
            }
            vector_finish(partner->connect);
            free(partner);
        }
        free(tkbio.save[i]);
    }
    free(tkbio.save);
}

void tkbio_set_pause()
{
    struct itimerval timerval;
    
    tkbio.pause = 1;
    timerval.it_interval.tv_sec = 0;
    timerval.it_interval.tv_usec = 0;
    timerval.it_value.tv_sec = 0;
    timerval.it_value.tv_usec = DELAY;
    while(setitimer(ITIMER_REAL, &timerval, 0) < 0)
    {
        VERBOSE(perror("[TKBIO] Failed to set timer"));
        sleep(1);
    }
}

struct tkbio_return tkbio_handle_event()
{
    struct tkbio_return ret = { .type = TKBIO_RETURN_NOP, .id = 0, .value.i = 0 };
    struct tsp_event event;
    const struct tkbio_map *map;
    const struct tkbio_mapelem *elem_last, *elem_curr;
    struct tkbio_save *save_last, *save_curr;
    struct tkbio_partner *partner_last;
    struct tkbio_point *p;
    
    int y, x, button_y, button_x, i;
    int width, height, fb_height, fb_width, scr_height, scr_width;
    char sim_tmp = 'x'; // content irrelevant

    if(recv(tkbio.sock, &event, sizeof(struct tsp_event), 0) == -1)
    {
        VERBOSE(perror("[TKBIO] Failed to receive event"));
        return ret;
    }
    
    if(event.x >= SCREENMAX || event.y >= SCREENMAX)
        return ret;
    
    // calculate height and width
    tkbio_get_sizes_current(&height, &width, &fb_height, &fb_width,
        &scr_height, &scr_width);
    
    // screen coordinates to button coordinates
    button_y = fb_height*(((SCREENMAX-event.y)%scr_height)/(scr_height*1.0));
    button_x = fb_width*(((event.x+1)%scr_width)/(scr_width*1.0));
    tkbio_fb_to_layout_pos_width(&button_y, &button_x, fb_width);
    
    // current map
    map = &tkbio.layout.maps[tkbio.parser.map];
    // last mapelem
    elem_last = &map->map[tkbio.parser.y*map->width+tkbio.parser.x];
    
    // broader functions increase type dependent the actual size
    // of the current button
    // if a type broader doesnt hit calculate position of new button
    i = 1;
    if(tkbio.parser.pressed)
        TYPEFUNC(elem_last, broader, i=, &y, &x, event.y, event.x, elem_last);
    if(!tkbio.parser.pressed || i)
    {
        x = event.x / scr_width;
        y = (SCREENMAX - event.y) / scr_height;
        tkbio_fb_to_layout_cords(&y, &x);
    }
    
    // current mapelem
    elem_curr = &map->map[y*map->width+x];
    
    // save of last/current button
    save_last = &tkbio.save[tkbio.parser.map]
        [tkbio.parser.y*map->width+tkbio.parser.x];
    save_curr = &tkbio.save[tkbio.parser.map][y*map->width+x];
    
    // partner of last button
    partner_last = save_last->partner;
    
    if(event.event & TSP_EVENT_MOVED)
    {
        if( tkbio.parser.pressed
            && !tkbio.pause) // avoid toggling between two buttons
        {
            // move on same button
            if(tkbio.parser.y == y && tkbio.parser.x == x)
move:           TYPEFUNC(elem_last, move, , y, x, button_y, button_x, map, elem_last, save_last);
            else
            {
                // check if moved to partner
                if(partner_last)
                    for(i=0; i<vector_size(partner_last->connect); i++)
                    {
                        p = vector_at(i, partner_last->connect);
                        if(p->y == y && p->x == x)
                        {
                            VERBOSE(printf("[TKBIO] Move to partner (%i,%i)\n", y, x));
                            goto move;
                        }
                    }
                
                // move to other button
                VERBOSE(printf("[TKBIO] Move (%i,%i)\n", y, x));
                
                TYPEFUNC(elem_last, focus_out, , map, elem_last, save_last);
                TYPEFUNC(elem_curr, focus_in, , y, x, button_y, button_x, map, elem_curr, save_curr);
            }
        }
    }
    else if(!(event.event & TSP_EVENT_PRESSED))
    {
        if(tkbio.parser.pressed)
        {
            VERBOSE(printf("[TKBIO] Release (%i,%i) ", y, x));
            
            TYPEFUNC(elem_curr, release, ret=, y, x, button_y, button_x, map, elem_curr, save_curr);
            
            VERBOSE(printf("\n"));
            
            tkbio.parser.pressed = 0;
        }
    }
    else // pressed
    {
        if(!tkbio.parser.pressed)
        {
            VERBOSE(printf("[TKBIO] Press (%i,%i)\n", y, x));
            
            TYPEFUNC(elem_curr, press, , y, x, button_y, button_x, map, elem_curr, save_curr);
            
            tkbio.parser.pressed = 1;
            
            tkbio_set_pause();
        }
    }
    
    tkbio.parser.y = y;
    tkbio.parser.x = x;
    
    // notify framebuffer for redraw
    if(tkbio.sim)
        send(tkbio.fb.sock, &sim_tmp, 1, 0);
    
    return ret;
}

int tkbio_run(tkbio_handler *handler, void *state)
{
    struct pollfd pfds[1];
    struct tkbio_return tret;
    int ret;
    
    pfds[0].fd = tkbio.sock;
    pfds[0].events = POLLIN;
    
    VERBOSE(printf("[TKBIO] run\n"));
    
    while(1)
    {
        poll(pfds, 1, -1);
        
        if(pfds[0].revents & POLLIN)
        {
            if((ret = handler((tret = tkbio_handle_event()), state)))
                return ret;
            if(tret.type == TKBIO_RETURN_QUIT)
                return 0;
        }
        else if(pfds[0].revents & POLLHUP)
        {
            VERBOSE(printf("[TKBIO] Failed to poll rpc socket\n"));
            tkbio_open_socket(-1);
        }
    }
    
    return 0;
}

