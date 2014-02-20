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

void tkbio_init_connect()
{
    int i, j, k, width, size;
    
    const struct tkbio_mapelem *map;
    const struct tkbio_mapelem *elem;
    struct tkbio_point p, p2, *pp;
    struct vector *v, *v2;
    
    // create for every group of buttons a vector
    // filled with tkbio_points of all buttons
    // every tkbio.connect[i][x] of a button points
    // to the same vector
    // the vectors and points are saved in layout format
    
    tkbio.connect = malloc(tkbio.layout.size*sizeof(struct vector**));
    
    for(i=0; i<tkbio.layout.size; i++)
    {
        width = tkbio.layout.maps[i].width;
        size = tkbio.layout.maps[i].height*width;
        tkbio.connect[i] = malloc(size*sizeof(struct vector*));
        memset(tkbio.connect[i], 0, size*sizeof(struct vector*));
        for(j=0; j<size; j++)
        {
            map = tkbio.layout.maps[i].map;
            elem = &map[j];
            p = (struct tkbio_point){j/width, j%width, elem};
            
            if(j%width>0 && elem->connect & TKBIO_LAYOUT_CONNECT_LEFT)
            {
                v = tkbio.connect[i][j-1];
                if(!v) // left unconnected
                {
                    vector_init(sizeof(struct tkbio_point), &tkbio.connect[i][j-1]);
                    v = tkbio.connect[i][j-1];
                    p2 = (struct tkbio_point){(j-1)/width, (j-1)%width, &map[j-1]};
                    vector_push(&p2, v);
                }
                tkbio.connect[i][j] = v;
                vector_push(&p, v);
            }
            if(j>=width && elem->connect & TKBIO_LAYOUT_CONNECT_UP)
            {
                v = tkbio.connect[i][j-width];
                if(tkbio.connect[i][j]) // already left connected
                {
                    if(!v) // up unconnected
                    {
                        v = tkbio.connect[i][j];
                        p2 = (struct tkbio_point){j/width-1, j%width, &map[j]};
                        tkbio.connect[i][j-width] = v;
                        vector_push(&p2, v);
                        continue;
                    }
                    v2 = tkbio.connect[i][j];
                    if(v == v2)
                        continue;
                    // merge vectors
                    if(vector_size(v2) > vector_size(v))
                    {
                        v2 = v;
                        v = tkbio.connect[i][j];
                    }
                    for(k=0; k<vector_size(v2); k++)
                    {
                        pp = vector_at(k, v2);
                        vector_push(pp, v);
                        tkbio.connect[i][pp->y*width+pp->x] = v;
                    }
                    vector_finish(v2);
                    free(v2);
                }
                else
                {
                    if(!v) // up unconnected
                    {
                        vector_init(sizeof(struct tkbio_point), &tkbio.connect[i][j-width]);
                        v = tkbio.connect[i][j-width];
                        p2 = (struct tkbio_point){j/width-1, j%width, &map[j-width]};
                        vector_push(&p2, v);
                    }
                    tkbio.connect[i][j] = v;
                    vector_push(&p, v);
                }
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
    
    // calculate connects
    tkbio_init_connect();
    
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
    struct vector *v;
    struct tkbio_point *p;
    
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
            v = tkbio.connect[i][j];
            if(!v)
                continue;
            for(k=0; k<vector_size(v); k++)
            {
                p = vector_at(k, v);
                tkbio.connect[i][p->y*width+p->x] = 0;
            }
            vector_finish(v);
        }
        free(tkbio.connect[i]);
    }
    free(tkbio.connect);
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
    struct vector *vec = tkbio.connect
        [tkbio.parser.map][tkbio.parser.y*map->width+tkbio.parser.x];
    
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
        if(!vec)
            tkbio_layout_fill_rect(tkbio.parser.y*height,
                tkbio.parser.x*width, height, width, DENSITY,
                tkbio.fb.copy);
        else
        {
            // bytes of one button
            size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp;
            ptr = tkbio.fb.copy;
            for(i=0; i<vector_size(vec); i++, ptr += size)
            {
                p = vector_at(i, vec);
                tkbio_layout_fill_rect(p->y*height, p->x*width,
                    height, width, DENSITY, ptr);
            }
        }
        break;
    case FB_STATUS_FILL: // overdraw last button
        if(!vec)
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
            for(i=0; i<vector_size(vec); i++)
            {
                p = vector_at(i, vec);
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
}

int tkbio_event_move(int y, int x)
{
    // current map
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    // partner vector of last button
    struct vector *vec = tkbio.connect
        [tkbio.parser.map][tkbio.parser.y*map->width+tkbio.parser.x];
    
    // mapelem of current button
    const struct tkbio_mapelem *elem = &map->map[y*map->width+x];
    
    int i, slider;
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
    if(vec)
        for(i=0; i<vector_size(vec); i++)
        {
            p = vector_at(i, vec);
            if(p->y == y && p->x == x)
                return slider ? slider : MOVE_PARTNER;
        }
    return MOVE_NEXT;
}

struct tkbio_return tkbio_event_released(int y, int x, int button_y, int button_x, int height, int width)
{
    // current map
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    // partner vector of current button
    struct vector *vec = tkbio.connect[tkbio.parser.map][y*map->width+x];
    
    // mapelem of current button
    const struct tkbio_mapelem *elem = &map->map[y*map->width+x];
    
    struct tkbio_return ret = { .type = TKBIO_RETURN_NOP, .id = 0, .value.i = 0 };
    
    int i, min, max;
    struct tkbio_point *p;
    
    switch(elem->type & TKBIO_LAYOUT_MASK_TYPE)
    {
    case TKBIO_LAYOUT_TYPE_CHAR:
        if(!elem->elem.c[0]) // charelem unused
            break;
        if(tkbio.layout.fun) // layout specific convert function
            ret.value.c = tkbio.layout.fun(tkbio.parser.map,
                elem->elem, tkbio.parser.toggle);
        else
            ret.value.c = elem->elem;
        ret.type = TKBIO_RETURN_CHAR;
        ret.id = elem->id;
        if(!tkbio.parser.hold) // reset map to default if not on hold
            tkbio.parser.map = tkbio.layout.start;
        tkbio.parser.toggle = 0;
        break;
    case TKBIO_LAYOUT_TYPE_GOTO:
        tkbio.parser.map = (int) elem->elem.c[0];
        break;
    case TKBIO_LAYOUT_TYPE_HOLD:
        if(tkbio.parser.hold)
            tkbio.parser.map = tkbio.layout.start;
        tkbio.parser.hold = !tkbio.parser.hold;
        break;
    case TKBIO_LAYOUT_TYPE_TOGGLE:
        if(!tkbio.parser.hold)
            tkbio.parser.map = tkbio.layout.start;
        tkbio.parser.toggle ^= elem->elem.c[0];
        break;
    case TKBIO_LAYOUT_TYPE_HSLIDER:
        if(vec)
        {
            for(i=0, min=max=x; i<vector_size(vec); i++)
            {
                p = vector_at(i, vec);
                min = p->x < min ? p->x : min;
                max = p->x > max ? p->x : max;
            }
            ret.value.i = ((x-min)*width+button_x)*100;
            ret.value.i /= (max-min+1)*width;
        }
        else
            ret.value.i = (button_x*100)/width;
        ret.type = TKBIO_RETURN_INT;
        ret.id = elem->id;
        break;
    case TKBIO_LAYOUT_TYPE_VSLIDER:
        if(vec)
        {
            for(i=0, min=max=y; i<vector_size(vec); i++)
            {
                p = vector_at(i, vec);
                min = p->y < min ? p->y : min;
                max = p->y > max ? p->y : max;
            }
            ret.value.i = ((max-y)*height+(height-button_y))*100;
            ret.value.i /= (max-min+1)*height;
        }
        else
            ret.value.i = ((height-button_y)*100)/height;
        ret.type = TKBIO_RETURN_INT;
        ret.id = elem->id;
        break;
    }
    
    return ret;
}

int tkbio_event_pressed(int y, int x, int button_y, int button_x, int height, int width)
{
    // current map
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    // partner vector of current button
    struct vector *vec = tkbio.connect[tkbio.parser.map][y*map->width+x];
    
    // mapelem of current button
    const struct tkbio_mapelem *elem = &map->map[y*map->width+x];
    
    int button_height = height;
    int button_width = width;
    int hwidth = 0, vheight = 0;
    
    int slider = 0, hslider = 0, vslider = 0;
    
    int i, size, allsize;
    struct tkbio_point *p;
    unsigned char *ptr;
    
    switch(elem->type & TKBIO_LAYOUT_MASK_TYPE)
    {
    case TKBIO_LAYOUT_TYPE_CHAR:
        if(!elem->elem.c[0]) // if charelem unused dont draw anything
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
        slider = vslider = PARSER_STATUS_VSLIDER;
        break;
    }
    
    if(elem->type & TKBIO_LAYOUT_OPTION_COPY) // first save then draw button
    {
        // bytes of one button / all buttons
        size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp;
        allsize = size * (vec ? vector_size(vec) : 1);
        
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
        
        if(!vec)
            tkbio_layout_draw_rect(y*button_height, x*button_width,
                button_height, button_width, elem->color >> 4,
                DENSITY, tkbio.fb.copy);
        else
        {
            ptr = tkbio.fb.copy;
            for(i=0; i<vector_size(vec); i++, ptr += size)
            {
                p = vector_at(i, vec);
                tkbio_layout_draw_rect(p->y*button_height,
                    p->x*button_width, button_height, button_width,
                    p->elem->color >> 4, DENSITY, ptr);
            }
        }
        tkbio.fb.status = FB_STATUS_COPY;
    }
    else // draw button
    {
        if(!vec)
        {
            tkbio_layout_draw_rect(y*button_height, x*button_width,
                height, width, elem->color >> 4, DENSITY, 0);
            if(slider && elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                tkbio_layout_draw_rect_connect(y*button_height-vheight,
                    x*button_width+hwidth, y, x, button_height-vheight,
                    button_width-hwidth, elem->color, elem->connect,
                    DENSITY, 0);
            else if(slider)
                tkbio_layout_draw_rect(y*button_height-vheight,
                    x*button_width+hwidth, button_height-vheight,
                    button_width-hwidth, elem->color & 15, DENSITY, 0);
        }
        else
        {
            for(i=0; i<vector_size(vec); i++)
            {
                p = vector_at(i, vec);
                if(!slider || (hslider && p->x < x) || (vslider && p->y > y))
                    tkbio_layout_draw_rect(p->y*button_height,
                        p->x*button_width, button_height, button_width,
                        p->elem->color >> 4, DENSITY, 0);
                else if((hslider && p->x == x) || (vslider && p->y == y))
                {
                    tkbio_layout_draw_rect(p->y*button_height,
                        p->x*button_width, height, width,
                        p->elem->color >> 4, DENSITY, 0);
                    if(p->elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                        tkbio_layout_draw_rect_connect(
                            p->y*button_height-vheight,
                            p->x*button_width+hwidth, p->y, p->x,
                            button_height-vheight, button_width-hwidth,
                            p->elem->color, p->elem->connect,
                            DENSITY, 0);
                    else
                        tkbio_layout_draw_rect(
                            p->y*button_height-vheight,
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
    
    int y, x, fb_y, fb_x, button_y, button_x;
    int width, height, fb_height, fb_width, screen_width, screen_height;
    char sim_tmp = 'x'; // content irrelevant
    
    int tmp;
    int hslider = PARSER_STATUS_HSLIDER, vslider = PARSER_STATUS_VSLIDER;
    
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
            VERBOSE(printf("[TKBIO] Release (%i,%i)\n", y, x));
            
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
    int ret;
    
    pfds[0].fd = tkbio.sock;
    pfds[0].events = POLLIN;
    
    VERBOSE(printf("[TKBIO] run\n"));
    
    while(1)
    {
        poll(pfds, 1, -1);
        
        if(pfds[0].revents & POLLIN)
        {
            if((ret = handler(tkbio_handle_event(), state)) != 0)
                return ret;
        }
        else if(pfds[0].revents & POLLHUP)
        {
            VERBOSE(printf("[TKBIO] Failed to poll rpc socket\n"));
            tkbio_open_socket(-1);
        }
    }
    
    return 0;
}
