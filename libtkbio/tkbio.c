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

#include "tkbio.h"
#include "tkbio_def.h"
#include "tkbio_fb.h"

#include "tsp.h"

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

#include "tkbio_layout_default.h"

struct tkbio_global tkbio;

struct tkbio_point
{
    int y, x;
    const struct tkbio_mapelem *elem;
};

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
    int i, j, k, height, width, size, height_px, width_px;
    int hslider, h_y, v_y, h_x, v_x, h_max, v_max;
    struct tkbio_save *save;
    struct tkbio_save_slider *slider;
    struct tkbio_point *p;
    
    for(i=0; i<tkbio.layout.size; i++)
    {
        width = tkbio.layout.maps[i].width;
        height = tkbio.layout.maps[i].height;
        size = height*width;
        tkbio_get_sizes(&height_px, &width_px, 0, 0, 0, 0);
        
        for(j=0; j<size; j++)
        {
            save = &tkbio.save[i][j];
            
            if(!save->partner)
            {
                switch(tkbio.layout.maps[i].map[j].type & TKBIO_LAYOUT_MASK_TYPE)
                {
                case TKBIO_LAYOUT_TYPE_HSLIDER:
                case TKBIO_LAYOUT_TYPE_VSLIDER:
                    save->data.slider = slider =
                        malloc(sizeof(struct tkbio_save_slider));
                    slider->y = 0;
                    slider->x = 0;
                    break;
                }
                continue;
            }
            
            if(save->partner->data.data)
                continue;
            
            hslider = 0;
            
            switch(tkbio.layout.maps[i].map[j].type & TKBIO_LAYOUT_MASK_TYPE)
            {
            case TKBIO_LAYOUT_TYPE_HSLIDER:
                hslider = 1;
            case TKBIO_LAYOUT_TYPE_VSLIDER:
                save->partner->data.slider = slider =
                    malloc(sizeof(struct tkbio_save_slider));
                h_x = v_x = width-1;
                h_y = v_max = height-1;
                v_y = h_max = 0;
                for(k=0; k<vector_size(save->partner->connect); k++)
                {
                    p = vector_at(k, save->partner->connect);
                    if(p->x < h_x)
                    {
                        h_x = p->x;
                        h_y = p->y;
                    }
                    if(p->y > v_y)
                    {
                        v_y = p->y;
                        v_x = p->x;
                    }
                    if(p->x > h_max)
                        h_max = p->x;
                    if(p->y < v_max)
                        v_max = p->y;
                }
                if(hslider)
                {
                    slider->y = h_y*height_px;
                    slider->x = slider->start = h_x*width_px;
                    slider->size = (h_max-h_x+1)*width_px;
                }
                else
                {
                    slider->y = slider->start = (v_y+1)*height_px;
                    slider->x = v_x*width_px;
                    slider->size = (v_y-v_max+1)*height_px;
                }
                break;
            }
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
    
    tkbio_get_sizes(&height, &width, 0, 0, 0, 0);
    
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
    tkbio.fb.copy = 0;
    
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
    tkbio.parser.status = PARSER_STATUS_NOP;
    
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
    
    int i, j, k, width, size;
    struct tkbio_point *p;
    struct tkbio_partner *partner;
    struct tkbio_save *save;
    
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
    if(tkbio.fb.copy)
        free(tkbio.fb.copy);
    
    for(i=0; i<tkbio.layout.size; i++)
    {
        width = tkbio.layout.maps[i].width;
        size = tkbio.layout.maps[i].height * width;
        for(j=0; j<size; j++)
        {
            save = &tkbio.save[i][j];
            partner = save->partner;
            if(save->data.data)
                free(save->data.data);
            if(!partner)
                continue;
            for(k=0; k<vector_size(partner->connect); k++)
            {
                p = vector_at(k, partner->connect);
                tkbio.save[i][p->y*width+p->x].partner = 0;
            }
            if(partner->data.data)
                free(partner->data.data);
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

void tkbio_event_cleanup(int height, int width)
{
    // current map
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    // partner vector of last button
    struct tkbio_partner *partner = tkbio.save[tkbio.parser.map]
        [tkbio.parser.y*map->width+tkbio.parser.x].partner;
    
    // mapelem of last button
    const struct tkbio_mapelem *elem = &map->map
        [tkbio.parser.y*map->width+tkbio.parser.x];
    
    int i, size;
    struct tkbio_point *p;
    unsigned char *ptr;
    
    switch(tkbio.fb.status)
    {
    case FB_STATUS_NOP: // no cleanup
        break;
    case FB_STATUS_COPY: // write back saved contents of last button
        if(!partner)
            tkbio_layout_fill_rect(tkbio.parser.y*height,
                tkbio.parser.x*width, height, width, DENSITY,
                tkbio.fb.copy);
        else
        {
            // bytes of one button
            size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp;
            ptr = tkbio.fb.copy;
            for(i=0; i<vector_size(partner->connect); i++, ptr += size)
            {
                p = vector_at(i, partner->connect);
                tkbio_layout_fill_rect(p->y*height, p->x*width,
                    height, width, DENSITY, ptr);
            }
        }
        break;
    case FB_STATUS_FILL: // overdraw last button
        if(!partner)
        {
            if(elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                tkbio_layout_draw_rect_connect(tkbio.parser.y*height,
                    tkbio.parser.x*width, tkbio.parser.y,
                    tkbio.parser.x, height, width, elem->color,
                    elem->connect, DENSITY, 0);
            else
                tkbio_layout_draw_rect(tkbio.parser.y*height,
                    tkbio.parser.x*width, height, width,
                    elem->color & 15, DENSITY, 0);
        }
        else
            for(i=0; i<vector_size(partner->connect); i++)
            {
                p = vector_at(i, partner->connect);
                if(p->elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                    tkbio_layout_draw_rect_connect(p->y*height,
                        p->x*width, p->y, p->x, height, width,
                        p->elem->color, p->elem->connect, DENSITY, 0);
                else
                    tkbio_layout_draw_rect(p->y*height, p->x*width,
                        height, width, p->elem->color & 15, DENSITY, 0);
            }
        break;
    }
    
    tkbio.parser.map = tkbio.parser.nmap;
}

int tkbio_event_move(int y, int x)
{
    // current map
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    // partner vector of last button
    struct tkbio_partner *partner = tkbio.save[tkbio.parser.map]
        [tkbio.parser.y*map->width+tkbio.parser.x].partner;
    
    // mapelem of current button
    const struct tkbio_mapelem *elem = &map->map[y*map->width+x];
    
    int i, slider = MOVE_NOP;
    struct tkbio_point *p;
    
    switch(elem->type & TKBIO_LAYOUT_MASK_TYPE)
    {
    case TKBIO_LAYOUT_TYPE_HSLIDER:
        slider = MOVE_HSLIDER;
        break;
    case TKBIO_LAYOUT_TYPE_VSLIDER:
        slider = MOVE_VSLIDER;
        break;
    }
    
    // skip if same button
    if(tkbio.parser.y == y && tkbio.parser.x == x)
        return slider ? slider : MOVE_NOP;
    
    // check if moved/slid to partner
    if(partner)
        for(i=0; i<vector_size(partner->connect); i++)
        {
            p = vector_at(i, partner->connect);
            if(p->y == y && p->x == x)
                return slider ? slider : MOVE_PARTNER;
        }
    return MOVE_NEXT;
}

struct tkbio_return tkbio_event_released(int y, int x, int button_y, int button_x, int height, int width)
{
    // current map
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    // save of current button
    struct tkbio_save *save = &tkbio.save[tkbio.parser.map][y*map->width+x];
    
    // mapelem of current button
    const struct tkbio_mapelem *elem = &map->map[y*map->width+x];
    
    struct tkbio_return ret = { .type = TKBIO_RETURN_NOP, .id = 0, .value.i = 0 };
    
    struct tsp_cmd cmd;
    struct tkbio_save_slider *slider;
    
    switch(elem->type & TKBIO_LAYOUT_MASK_TYPE)
    {
    case TKBIO_LAYOUT_TYPE_CHAR:
        if(!elem->elem.i) // elem unused
        {
            VERBOSE(printf("NOP\n"));
            break;
        }
        if(tkbio.layout.fun) // layout specific convert function
            ret.value = tkbio.layout.fun(tkbio.parser.map,
                elem->elem, tkbio.parser.toggle);
        else
            ret.value = elem->elem;
        ret.type = TKBIO_RETURN_CHAR;
        ret.id = elem->id;
        if(!tkbio.parser.hold) // reset map to default if not on hold
            tkbio.parser.nmap = tkbio.layout.start;
        tkbio.parser.toggle = 0;
        VERBOSE(if(elem->name)
            printf("[%s]\n", elem->name);
        else
            printf("[%c]\n", elem->elem.c.c[0]));
        break;
    case TKBIO_LAYOUT_TYPE_GOTO:
        tkbio.parser.nmap = elem->elem.i + map->offset;
        tkbio.parser.hold = 0;
        VERBOSE(printf("goto %s\n", elem->name));
        break;
    case TKBIO_LAYOUT_TYPE_HOLD:
        if(tkbio.parser.hold)
            tkbio.parser.nmap = tkbio.layout.start;
        tkbio.parser.hold = !tkbio.parser.hold;
        VERBOSE(printf("hold %s\n", tkbio.parser.hold ? "on" : "off"));
        break;
    case TKBIO_LAYOUT_TYPE_TOGGLE:
        if(!tkbio.parser.hold)
            tkbio.parser.nmap = tkbio.layout.start;
        tkbio.parser.toggle ^= elem->elem.i;
        VERBOSE(printf("%s %s\n", elem->name,
            tkbio.parser.toggle&elem->elem.i ? "on" : "off"));
        break;
    case TKBIO_LAYOUT_TYPE_HSLIDER:
        if(save->partner)
        {
            slider = save->partner->data.slider;
            ret.value.i = (x*width-slider->start+button_x)*100;
            ret.value.i /= slider->size;
        }
        else
        {
            slider = save->data.slider;
            ret.value.i = (button_x*100)/width;
        }
        ret.type = TKBIO_RETURN_INT;
        ret.id = elem->id;
        slider->y = y*height+button_y;
        slider->x = x*width+button_x;
        VERBOSE(printf("hslider %i: %02i%%\n", ret.id, ret.value.i));
        break;
    case TKBIO_LAYOUT_TYPE_VSLIDER:
        if(save->partner)
        {
            slider = save->partner->data.slider;
            ret.value.i = (slider->start-(y*height)-button_y)*100;
            ret.value.i /= slider->size;
        }
        else
        {
            slider = save->data.slider;
            ret.value.i = ((height-button_y)*100)/height;
        }
        ret.type = TKBIO_RETURN_INT;
        ret.id = elem->id;
        slider->y = y*height+button_y;
        slider->x = x*width+button_x;
        VERBOSE(printf("vslider %i: %02i%%\n", ret.id, ret.value.i));
        break;
    case TKBIO_LAYOUT_TYPE_SYSTEM:
        switch(elem->elem.i)
        {
        case TKBIO_SYSTEM_NEXT:
            VERBOSE(printf("app switch next\n"));
            ret.type = TKBIO_RETURN_SWITCH;
            cmd.cmd = TSP_CMD_NEXT;
            break;
        case TKBIO_SYSTEM_PREV:
            VERBOSE(printf("app switch prev\n"));
            ret.type = TKBIO_RETURN_SWITCH;
            cmd.cmd = TSP_CMD_PREV;
            break;
        case TKBIO_SYSTEM_QUIT:
            VERBOSE(printf("app quit\n"));
            ret.type = TKBIO_RETURN_QUIT;
            cmd.cmd = TSP_CMD_REMOVE;
            break;
        case TKBIO_SYSTEM_ACTIVATE:
            VERBOSE(printf("app activate\n"));
            goto done;
        case TKBIO_SYSTEM_MENU:
            VERBOSE(printf("app menu\n"));
            // todo
            goto done;
        }
        
        cmd.value = 0;
        send(tkbio.sock, &cmd, sizeof(struct tsp_cmd), 0);
        
done:   tkbio.parser.nmap = tkbio.layout.start;
        break;
    }
    
    return ret;
}

int tkbio_event_pressed(int y, int x, int button_y, int button_x, int height, int width)
{
    // current map
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    // partner vector of current button
    struct tkbio_partner *partner = tkbio.save[tkbio.parser.map]
        [y*map->width+x].partner;
    
    // mapelem of current button
    const struct tkbio_mapelem *elem = &map->map[y*map->width+x];
    
    int button_height = height;
    int button_width = width;
    int hwidth = 0, vheight = 0, voffset_l = 0, voffset_p = 0;
    
    int slider = 0, hslider = 0, vslider = 0;
    
    int i, size, allsize;
    struct tkbio_point *p;
    unsigned char *ptr;
    
    switch(elem->type & TKBIO_LAYOUT_MASK_TYPE)
    {
    case TKBIO_LAYOUT_TYPE_CHAR:
        if(!elem->elem.i) // if elem unused dont draw anything
        {
            tkbio.fb.status = FB_STATUS_NOP;
            return 0;
        }
        break;
    case TKBIO_LAYOUT_TYPE_HSLIDER:
        width = hwidth = button_x;
        slider = hslider = PARSER_STATUS_HSLIDER;
        break;
    case TKBIO_LAYOUT_TYPE_VSLIDER:
        height = vheight = height-button_y;
        if(tkbio.format == TKBIO_FORMAT_PORTRAIT)
            voffset_p = button_y;
        else
            voffset_l = height;
        slider = vslider = PARSER_STATUS_VSLIDER;
        break;
    }
    
    if(elem->type & TKBIO_LAYOUT_OPTION_COPY) // first save then draw button
    {
        // bytes of one button / all buttons
        size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp;
        allsize = size * (partner ? vector_size(partner->connect) : 1);
        
        if(!tkbio.fb.copy)
        {
            tkbio.fb.copy = malloc(allsize);
            tkbio.fb.copy_size = allsize;
        }
        else if(tkbio.fb.copy_size < allsize)
        {
            tkbio.fb.copy = realloc(tkbio.fb.copy, allsize);
            tkbio.fb.copy_size = allsize;
        }
        
        if(!partner)
            tkbio_layout_draw_rect(y*button_height, x*button_width,
                button_height-vheight, button_width, elem->color >> 4,
                DENSITY, tkbio.fb.copy);
        else
        {
            ptr = tkbio.fb.copy;
            for(i=0; i<vector_size(partner->connect); i++, ptr += size)
            {
                p = vector_at(i, partner->connect);
                tkbio_layout_draw_rect(p->y*button_height,
                    p->x*button_width, button_height, button_width,
                    p->elem->color >> 4, DENSITY, ptr);
            }
        }
        tkbio.fb.status = FB_STATUS_COPY;
    }
    else // draw button
    {
        if(!partner)
        {
            tkbio_layout_draw_rect(y*button_height+voffset_p,
                x*button_width, height, width, elem->color >> 4,
                DENSITY, 0);
            if(slider && elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                tkbio_layout_draw_rect_connect(y*button_height-voffset_l,
                    x*button_width+hwidth, y, x, button_height-vheight,
                    button_width-hwidth, elem->color, elem->connect,
                    DENSITY, 0);
            else if(slider)
                tkbio_layout_draw_rect(y*button_height-voffset_l,
                    x*button_width+hwidth, button_height-vheight,
                    button_width-hwidth, elem->color & 15, DENSITY, 0);
        }
        else
        {
            for(i=0; i<vector_size(partner->connect); i++)
            {
                p = vector_at(i, partner->connect);
                if(!slider || (hslider && p->x < x) || (vslider && p->y > y))
                    tkbio_layout_draw_rect(p->y*button_height,
                        p->x*button_width, button_height, button_width,
                        p->elem->color >> 4, DENSITY, 0);
                else if((hslider && p->x == x) || (vslider && p->y == y))
                {
                    tkbio_layout_draw_rect(p->y*button_height+voffset_p,
                        p->x*button_width, height, width,
                        p->elem->color >> 4, DENSITY, 0);
                    if(p->elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                        tkbio_layout_draw_rect_connect(
                            p->y*button_height-voffset_l,
                            p->x*button_width+hwidth, p->y, p->x,
                            button_height-vheight, button_width-hwidth,
                            p->elem->color, p->elem->connect,
                            DENSITY, 0);
                    else
                        tkbio_layout_draw_rect(
                            p->y*button_height-voffset_l,
                            p->x*button_width+hwidth,
                            button_height-vheight, button_width-hwidth,
                            p->elem->color & 15, DENSITY, 0);
                }
                else
                {
                    if(p->elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                        tkbio_layout_draw_rect_connect(p->y*button_height,
                            p->x*button_width, p->y, p->x, button_height,
                            button_width, p->elem->color,
                            p->elem->connect, DENSITY, 0);
                    else
                        tkbio_layout_draw_rect(p->y*button_height,
                            p->x*button_width, button_height,
                            button_width, p->elem->color & 15,
                            DENSITY, 0);
                }
            }
        }
        tkbio.fb.status = FB_STATUS_FILL;
    }
    return slider;
}

struct tkbio_return tkbio_handle_event()
{
    struct tkbio_return ret = { .type = TKBIO_RETURN_NOP, .id = 0, .value.i = 0 };
    
    struct tsp_event event;
    
    // current map
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    // save of last button
    struct tkbio_save *save = &tkbio.save[tkbio.parser.map]
        [tkbio.parser.y*map->width+tkbio.parser.x];
    
    int y, x, fb_y, fb_x, button_y, button_x;
    int width, height, fb_height, fb_width, screen_width, screen_height;
    char sim_tmp = 'x'; // content irrelevant
    
    int tmp;
    int hslider = PARSER_STATUS_HSLIDER, vslider = PARSER_STATUS_VSLIDER;
    struct tkbio_save_slider *slider;
    
    if(recv(tkbio.sock, &event, sizeof(struct tsp_event), 0) == -1)
    {
        VERBOSE(perror("[TKBIO] Failed to receive event"));
        return ret;
    }
    
    if(event.x >= SCREENMAX || event.y >= SCREENMAX)
        return ret;
    
    // calculate height and width
    tkbio_get_sizes(&height, &width, &fb_height, &fb_width,
        &screen_height, &screen_width);
    
    // last coordinates are saved in layout format
    fb_y = tkbio.parser.y;
    fb_x = tkbio.parser.x;
    tkbio_layout_to_fb_cords(&fb_y, &fb_x);
    
    // screen coordinates to button coordinates
    button_y = fb_height*(((SCREENMAX-event.y)%screen_height)/(screen_height*1.0));
    button_x = fb_width*(((event.x+1)%screen_width)/(screen_width*1.0));
    tkbio_fb_to_layout_pos_width(&button_y, &button_x, fb_width);
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        hslider = PARSER_STATUS_VSLIDER;
        vslider = PARSER_STATUS_HSLIDER;
    }
    
    // if event coordinates within increased last button, same button
    if(tkbio.parser.status == PARSER_STATUS_PRESSED
        && event.x >= screen_width*fb_x - screen_width*(INCREASE/100.0)
        && event.x <= screen_width*(fb_x+1) + screen_width*(INCREASE/100.0)
        && (SCREENMAX - event.y) >= screen_height*fb_y - screen_height*(INCREASE/100.0)
        && (SCREENMAX - event.y) <= screen_height*(fb_y+1) + screen_height*(INCREASE/100.0))
    {
        x = tkbio.parser.x;
        y = tkbio.parser.y;
    }
    // if hslider only update x if within y boundaries
    else if(tkbio.parser.status == hslider
        && (SCREENMAX - event.y) >= screen_height*fb_y - screen_height*(INCREASE/100.0)
        && (SCREENMAX - event.y) <= screen_height*(fb_y+1) + screen_height*(INCREASE/100.0))
    {
        x = event.x / screen_width;
        y = fb_y;
        tkbio_fb_to_layout_cords(&y, &x);
    }
    // if vslider only update y if within x boundaries
    else if(tkbio.parser.status == vslider
        && event.x >= screen_width*fb_x - screen_width*(INCREASE/100.0)
        && event.x <= screen_width*(fb_x+1) + screen_width*(INCREASE/100.0))
    {
        x = fb_x;
        y = (SCREENMAX - event.y) / screen_height;
        tkbio_fb_to_layout_cords(&y, &x);
    }
    else // calculate layout position of new button
    {
        x = event.x / screen_width;
        y = (SCREENMAX - event.y) / screen_height;
        tkbio_fb_to_layout_cords(&y, &x);
    }
    
    if(event.event & TSP_EVENT_MOVED)
    {
        if( tkbio.parser.status // move only possible if pressed/slider
            && !tkbio.pause) // avoid toggling between two buttons
        {
            switch((tmp = tkbio_event_move(y, x)))
            {
            case MOVE_NOP:
                break;
            case MOVE_PARTNER:
                VERBOSE(printf("[TKBIO] Move to partner (%i,%i)\n", y, x));
                break;
            case MOVE_HSLIDER:
            case MOVE_VSLIDER:
                VERBOSE(printf("[TKBIO] Slide (%i,%i)\n", y, x));
                tkbio_event_pressed(y, x, button_y, button_x, height, width);
                tkbio.parser.status = tmp == MOVE_HSLIDER ?
                    PARSER_STATUS_HSLIDER : PARSER_STATUS_VSLIDER;
                break;
            case MOVE_NEXT:
                VERBOSE(printf("[TKBIO] Move (%i,%i)\n", y, x));
                if(tkbio.parser.status & PARSER_STATUS_SLIDER)
                {
                    if(save->partner)
                        slider = save->partner->data.slider;
                    else
                        slider = save->data.slider;
                    tkbio_event_pressed(slider->y/height,
                        slider->x/width, slider->y%height,
                        slider->x%width, height, width);
                }
                else
                    tkbio_event_cleanup(height, width);
                tkbio_event_pressed(y, x, button_y, button_x, height, width);
                tkbio.parser.status = PARSER_STATUS_PRESSED;
                break;
            }
            tkbio.parser.y = y;
            tkbio.parser.x = x;
        }
    }
    else if(!(event.event & TSP_EVENT_PRESSED))
    {
        if(tkbio.parser.status) // release only possible if pressed/slider
        {
            VERBOSE(printf("[TKBIO] Release (%i,%i) ", y, x));
            
            ret = tkbio_event_released(y, x, button_y, button_x, height, width);
            if(!(tkbio.parser.status & PARSER_STATUS_SLIDER))
                tkbio_event_cleanup(height, width);
            
            tkbio.fb.status = FB_STATUS_NOP;
            tkbio.parser.status = PARSER_STATUS_NOP;
        }
    }
    else // pressed
    {
        if(!tkbio.parser.status) // press only possible if not pressed/no slider
        {
            VERBOSE(printf("[TKBIO] Press (%i,%i)\n", y, x));
            
            if((tmp = tkbio_event_pressed(y, x, button_y, button_x,
                height, width)))
            {
                tkbio.parser.status = tmp;
            }
            else
                tkbio.parser.status = PARSER_STATUS_PRESSED;
            
            
            tkbio.parser.y = y;
            tkbio.parser.x = x;
            
            tkbio_set_pause();
        }
    }
    
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
