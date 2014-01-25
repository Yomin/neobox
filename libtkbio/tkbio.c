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
        unsigned char cmd = TSP_CMD_REMOVE;
        send(tkbio.sock, &cmd, sizeof(unsigned char), 0);
        if(tkbio.custom_signal_handler)
            tkbio.custom_signal_handler(signal);
        else
            exit(128+SIGINT);
        break;
    }
    default:
        if(tkbio.custom_signal_handler)
            tkbio.custom_signal_handler(signal);
    }
}

void tkbio_init_connect()
{
    tkbio.connect = malloc(tkbio.layout.size*sizeof(struct vector**));
    int i, j, k, width, size;
    for(i=0; i<tkbio.layout.size; i++)
    {
        width = tkbio.layout.maps[i].width;
        size = tkbio.layout.maps[i].height*width;
        tkbio.connect[i] = malloc(size*sizeof(struct vector*));
        memset(tkbio.connect[i], 0, size*sizeof(struct vector*));
        for(j=0; j<size; j++)
        {
            const struct tkbio_mapelem *map = tkbio.layout.maps[i].map;
            const struct tkbio_mapelem *elem = &map[j];
            struct tkbio_point p = {j/width, j%width, elem}, p2, *pp;
            struct vector *v, *v2;
            
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
    DEBUG(printf("[TKBIO] Connecting to rpc socket\n"));
    
    tkbio.sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(tkbio.sock == -1)
    {
        DEBUG(perror("[TKBIO] Failed to open rpc socket"));
        return TKBIO_ERROR_RPC_OPEN;
    }
    
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    sprintf(addr.sun_path, "%s/%s", tkbio.tsp, TSP_RPC);
    
    while(connect(tkbio.sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        DEBUG(perror("[TKBIO] Failed to connect rpc socket"));
        if(errno != EADDRINUSE)
            return TKBIO_ERROR_RPC_OPEN;
        
        if(tries-- == 1)
        {
            DEBUG(perror("[TKBIO] Unable to connect rpc socket"));
            return TKBIO_ERROR_RPC_OPEN;
        }
        sleep(1);
        DEBUG(printf("[TKBIO] Retry connecting to rpc socket\n"));
    }
    return 0;
}

struct tkbio_config tkbio_args(int *argc, char *argv[], struct tkbio_config config)
{
    struct option fboption[] =
    {
        { "tkbio-fb", 1, 0, 'f' },
        { "tkbio-tsp", 1, 0, 't' },
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

int tkbio_init_custom(const char *name, struct tkbio_config config)
{
    int ret;
    
#ifndef NDEBUG
    printf("[TKBIO] init\n");
    printf("[TKBIO] fb: %s\n", config.fb);
    printf("[TKBIO] tsp: %s\n", config.tsp);
    printf("[TKBIO] format: %s\n", config.format == TKBIO_FORMAT_LANDSCAPE ? "landscape" : "portrait");
    printf("[TKBIO] options:\n");
    printf("[TKBIO]   initial print: %s\n", config.options & TKBIO_OPTION_NO_INITIAL_PRINT ? "no" : "yes");
#endif
    
    tkbio.tsp = config.tsp;
    
    // open rpc socket
    tkbio.name = name;
    if((ret = tkbio_open_socket(-1)) < 0)
        return ret;
    
    // open framebuffer
    if(strncmp(config.fb+strlen(config.fb)-4, ".ipc", 4))
    {
        tkbio.sim = 0;
        DEBUG(printf("[TKBIO] Opening framebuffer\n"));
        if((tkbio.fb.fd = open(config.fb, O_RDWR)) == -1)
        {
            DEBUG(perror("[TKBIO] Failed to open framebuffer"));
            close(tkbio.sock);
            return TKBIO_ERROR_FB_OPEN;
        }
        if(ioctl(tkbio.fb.fd, FBIOGET_VSCREENINFO, &(tkbio.fb.vinfo)) == -1)
        {
            DEBUG(perror("[TKBIO] Failed to get variable screeninfo"));
            close(tkbio.sock);
            close(tkbio.fb.fd);
            return TKBIO_ERROR_FB_VINFO;
        }
        if(ioctl(tkbio.fb.fd, FBIOGET_FSCREENINFO, &(tkbio.fb.finfo)) == -1)
        {
            DEBUG(perror("[TKBIO] Failed to get fixed screeninfo"));
            close(tkbio.sock);
            close(tkbio.fb.fd);
            return TKBIO_ERROR_FB_FINFO;
        }
    }
    else
    {
        tkbio.sim = 1;
        DEBUG(printf("[TKBIO] Simulating framebuffer\n"));
        tkbio.fb.sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if(tkbio.fb.sock == -1)
        {
            DEBUG(perror("[TKBIO] Failed to open framebuffer socket"));
            return TKBIO_ERROR_FB_OPEN;
        }
        
        struct sockaddr_un addr;
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, config.fb);
        
        if(connect(tkbio.fb.sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
        {
            DEBUG(perror("[TKBIO] Failed to connect framebuffer socket"));
            return TKBIO_ERROR_FB_OPEN;
        }
        
        recv(tkbio.fb.sock, tkbio.fb.shm, sizeof(tkbio.fb.shm), 0);
        recv(tkbio.fb.sock, &tkbio.fb.vinfo, sizeof(struct fb_var_screeninfo), 0);
        recv(tkbio.fb.sock, &tkbio.fb.finfo, sizeof(struct fb_fix_screeninfo), 0);
        
        if((tkbio.fb.fd = shm_open(tkbio.fb.shm, O_CREAT|O_RDWR, 0644)) == -1)
        {
            DEBUG(perror("[TKBIO] Failed to open shared memory"));
            return TKBIO_ERROR_FB_OPEN;
        }
    }
    
    tkbio.fb.bpp = tkbio.fb.vinfo.bits_per_pixel/8;
    tkbio.fb.size = tkbio.fb.vinfo.xres*tkbio.fb.vinfo.yres*tkbio.fb.bpp;
    DEBUG(printf("[TKBIO] framebuffer info:\n"));
    DEBUG(printf("[TKBIO]   size %i x %i\n", tkbio.fb.vinfo.xres,tkbio.fb.vinfo.yres));
    DEBUG(printf("[TKBIO]   bytes per pixel %i\n", tkbio.fb.bpp));
    DEBUG(printf("[TKBIO]   line length %i\n", tkbio.fb.finfo.line_length));
    
    if(tkbio.sim && ftruncate(tkbio.fb.fd, tkbio.fb.size) == -1)
    {
        DEBUG(perror("[TKBIO] Failed to truncate shared memory"));
        return TKBIO_ERROR_FB_OPEN;
    }
    
    if((tkbio.fb.ptr = (unsigned char*) mmap(0, tkbio.fb.size, PROT_READ|PROT_WRITE, MAP_SHARED, tkbio.fb.fd, 0)) == MAP_FAILED)
    {
        DEBUG(perror("[TKBIO] Failed to mmap framebuffer"));
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
    tkbio.parser.pressed = 0;
    
    // else
    tkbio.pause = 0;
    tkbio.custom_signal_handler = 0;
    signal(SIGALRM, tkbio_signal_handler);
    signal(SIGINT, tkbio_signal_handler);
    
    // print initial screen
    if(!(config.options & TKBIO_OPTION_NO_INITIAL_PRINT))
    {
        struct tkbio_map *map = (struct tkbio_map*) &tkbio.layout.maps[tkbio.parser.map];
        struct tkbio_mapelem *elem;
        int y, x, fy, fx, fheight = map->height, fwidth = map->width;
        tkbio_layout_to_fb_sizes(&fheight, &fwidth, 0, 0);
        for(y=0; y<map->height; y++)
            for(x=0; x<map->width; x++)
            {
                fy = y; fx = x;
                tkbio_layout_to_fb_cords(&fy, &fx);
                elem = (struct tkbio_mapelem*) &map->map[y*map->width+x];
                if(!(elem->type & TKBIO_LAYOUT_OPTION_COPY))
                    tkbio_fb_draw_rect(fy*fheight, fx*fwidth, fheight, fwidth, elem->color & 15, DENSITY, 0);
                if(elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                {
                    unsigned char borders = tkbio_fb_connect_to_borders(fy, fx, elem->connect);
                    tkbio_fb_draw_rect_border(fy*fheight, fx*fwidth, fheight, fwidth, elem->color >> 4, borders, DENSITY, 0);
                }
            }
        // notify framebuffer for redraw
        if(tkbio.sim)
        {
            unsigned char tmp = 'x';
            send(tkbio.fb.sock, &tmp, 1, 0);
        }
    }
    
    DEBUG(printf("[TKBIO] init done\n"));
    
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
    DEBUG(printf("[TKBIO] finish\n"));
    close(tkbio.sock);
    if(tkbio.sim)
    {
        shm_unlink(tkbio.fb.shm);
        close(tkbio.fb.fd);
        close(tkbio.fb.sock);
    }
    else
        close(tkbio.fb.fd);
    if(tkbio.fb.copy)
        free(tkbio.fb.copy);
    
    int i, j, k, width, size;
    struct vector *v, **v2;
    struct tkbio_point *p;
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
                v2 = &tkbio.connect[i][p->y*width+p->x];
                vector_finish(*v2);
                *v2 = 0;
            }
        }
        free(tkbio.connect[i]);
    }
    free(tkbio.connect);
}

void tkbio_set_pause()
{
    tkbio.pause = 1;
    struct itimerval timerval;
    timerval.it_interval.tv_sec = 0;
    timerval.it_interval.tv_usec = 0;
    timerval.it_value.tv_sec = 0;
    timerval.it_value.tv_usec = DELAY;
    while(setitimer(ITIMER_REAL, &timerval, 0) < 0)
    {
        DEBUG(perror("[TKBIO] Failed to set timer"));
        sleep(1);
    }
}

struct tkbio_charelem tkbio_handle_event()
{
    struct tkbio_map *map = (struct tkbio_map*) &tkbio.layout.maps[tkbio.parser.map];
    struct tkbio_charelem ret = (struct tkbio_charelem) {{0, 0, 0, 0}};
    
    struct tsp_event event;
    if(recv(tkbio.sock, &event, sizeof(struct tsp_event), 0) == -1)
    {
        DEBUG(perror("[TKBIO] Failed to receive event"));
        return ret;
    }
    
    if(event.x >= SCREENMAX || event.y >= SCREENMAX)
        return ret;
    
    int width = map->width, height = map->height, scrWidth, scrHeight;
    tkbio_layout_to_fb_sizes(&height, &width, &scrHeight, &scrWidth);
    
    int y, x;
    
    if(tkbio.parser.pressed
        && event.x >= scrWidth*tkbio.parser.x - scrWidth*(INCREASE/100.0)
        && event.x <= scrWidth*(tkbio.parser.x+1) + scrWidth*(INCREASE/100.0)
        && (SCREENMAX - event.y) >= scrHeight*tkbio.parser.y - scrHeight*(INCREASE/100.0)
        && (SCREENMAX - event.y) <= scrHeight*(tkbio.parser.y+1) + scrHeight*(INCREASE/100.0))
    {
        x = tkbio.parser.x;
        y = tkbio.parser.y;
    }
    else
    {
        x = event.x / scrWidth;
        y = (SCREENMAX - event.y) / scrHeight;
    }
    
    int mapY = y, mapX = x;
    tkbio_fb_to_layout_cords(&mapY, &mapX);
    struct tkbio_mapelem *elem = (struct tkbio_mapelem*) &map->map[mapY*map->width+mapX];
    
    if(event.event & TSP_EVENT_MOVED)
    {
        if(tkbio.parser.pressed && !tkbio.pause &&
          (tkbio.parser.y != y || tkbio.parser.x != x))
        {
            struct vector *vec = tkbio.connect
                [tkbio.parser.map][tkbio.parser.py*map->width+tkbio.parser.px];
            
            int i;
            struct tkbio_point *p;
            
            if(vec)
                for(i=0; i<vector_size(vec); i++)
                {
                    p = vector_at(i, vec);
                    if(p->y == mapY && p->x == mapX)
                    {
                        DEBUG(printf("[TKBIO] Move to partner (%i,%i)\n", mapY, mapX));
                        tkbio.parser.y = y;
                        tkbio.parser.x = x;
                        return ret;
                    }
                }
            
            DEBUG(printf("[TKBIO] Move (%i,%i)\n", mapY, mapX));
            switch(tkbio.fb.status)
            {
                case FB_STATUS_NOP:
                    break;
                case FB_STATUS_COPY:
                    if(!vec)
                        tkbio_fill_field(tkbio.parser.py, tkbio.parser.px,
                            height, width, DENSITY, tkbio.fb.copy);
                    else
                    {
                        int size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp*sizeof(unsigned char);
                        unsigned char *ptr = tkbio.fb.copy+size;
                        for(i=0; i<vector_size(vec); i++, ptr += size)
                        {
                            p = vector_at(i, vec);
                            tkbio_fill_field(p->y, p->x, height, width, DENSITY, ptr);
                        }
                    }
                    break;
                case FB_STATUS_FILL:
                    if(!vec)
                        tkbio_draw_field(tkbio.parser.py, tkbio.parser.px,
                            height, width, tkbio.fb.fillColor & 15, DENSITY, 0);
                    else
                        for(i=0; i<vector_size(vec); i++)
                        {
                            p = vector_at(i, vec);
                            tkbio_draw_field(p->y, p->x, height, width,
                                p->elem->color & 15, DENSITY, 0);
                        }
                    break;
            }
            goto pressed;
        }
    }
    else if(!(event.event & TSP_EVENT_PRESSED))
    {
        if(tkbio.parser.pressed)
        {
            DEBUG(printf("[TKBIO] Release (%i,%i)\n", mapY, mapX));
            switch(elem->type & TKBIO_LAYOUT_MASK_TYPE)
            {
                case TKBIO_LAYOUT_TYPE_CHAR:
                    if(!elem->elem.c[0])
                        break;
                    if(tkbio.layout.fun)
                        ret = tkbio.layout.fun(tkbio.parser.map, elem->elem, tkbio.parser.toggle);
                    else
                        ret = elem->elem;
                    if(!tkbio.parser.hold)
                        tkbio.parser.map = tkbio.layout.start;
                    tkbio.parser.toggle = 0;
                    break;
                case TKBIO_LAYOUT_TYPE_GOTO:
                    tkbio.parser.map = (int) elem->elem.c[0];
                    break;
                case TKBIO_LAYOUT_TYPE_HOLD:
                    if(tkbio.parser.hold)
                    {
                        tkbio.parser.map = tkbio.layout.start;
                        tkbio.parser.hold = 0;
                    }
                    else
                        tkbio.parser.hold = 1;
                    break;
                case TKBIO_LAYOUT_TYPE_TOGG:
                    if(!tkbio.parser.hold)
                        tkbio.parser.map = tkbio.layout.start;
                    tkbio.parser.toggle ^= elem->elem.c[0];
                    break;
            }
            
            struct vector *vec = tkbio.connect
                [tkbio.parser.map][tkbio.parser.py*map->width+tkbio.parser.px];
            
            switch(tkbio.fb.status)
            {
                case FB_STATUS_NOP:
                    break;
                case FB_STATUS_COPY:
                    if(tkbio.fb.copy)
                    {
                        if(!vec)
                            tkbio_fb_fill_rect(tkbio.parser.y*height, tkbio.parser.x*width,
                                height, width, DENSITY, tkbio.fb.copy);
                        else
                        {
                            int i;
                            struct tkbio_point *p;
                            int size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp*sizeof(unsigned char);
                            unsigned char *ptr = tkbio.fb.copy+size;
                            for(i=0; i<vector_size(vec); i++, ptr += size)
                            {
                                p = vector_at(i, vec);
                                tkbio_fill_field(p->y, p->x, height, width, DENSITY, ptr);
                            }
                        }
                    }
                    break;
                case FB_STATUS_FILL:
                {
                    if(!vec)
                        tkbio_fb_draw_rect(tkbio.parser.y*height, tkbio.parser.x*width,
                            height, width, elem->color & 15, DENSITY, 0);
                    else
                    {
                        int i;
                        struct tkbio_point *p;
                        for(i=0; i<vector_size(vec); i++)
                        {
                            p = vector_at(i, vec);
                            tkbio_draw_field(p->y, p->x, height, width,
                                p->elem->color & 15, DENSITY, 0);
                        }
                    }
                    break;
                }
            }
            
            tkbio.fb.status = FB_STATUS_NOP;
            tkbio.parser.pressed = 0;
        }
    }
    else // pressed
    {
        if(!tkbio.parser.pressed)
        {
            DEBUG(printf("[TKBIO] Press (%i,%i)\n", mapY, mapX));
pressed:
            switch(elem->type & TKBIO_LAYOUT_MASK_TYPE)
            {
                case TKBIO_LAYOUT_TYPE_CHAR:
                    if(!elem->elem.c[0])
                    {
                        tkbio.fb.status = FB_STATUS_NOP;
                        break;
                    }
                default:
                {
                    struct vector *vec = tkbio.connect[tkbio.parser.map][mapY*map->width+mapX];
                    
                    if(elem->type & TKBIO_LAYOUT_OPTION_COPY)
                    {
                        int size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp*sizeof(unsigned char);
                        int allsize = size * (vector_size(vec)+1);
                        
                        if(!tkbio.fb.copy)
                        {
                            tkbio.fb.copy = malloc(allsize);
                            tkbio.fb.copySize = allsize;
                        }
                        else if(tkbio.fb.copySize < allsize)
                        {
                            tkbio.fb.copy = realloc(tkbio.fb.copy, allsize);
                            tkbio.fb.copySize = allsize;
                        }
                        
                        if(!vec)
                            tkbio_fb_draw_rect(y*height, x*width, height,
                                width, elem->color >> 4, DENSITY, tkbio.fb.copy);
                        else
                        {
                            int i;
                            struct tkbio_point *p;
                            unsigned char *ptr = tkbio.fb.copy+size;
                            for(i=0; i<vector_size(vec); i++, ptr += size)
                            {
                                p = vector_at(i, vec);
                                tkbio_draw_field(p->y, p->x, height, width,
                                    p->elem->color >> 4, DENSITY, ptr);
                            }
                        }
                        tkbio.fb.status = FB_STATUS_COPY;
                    }
                    else
                    {
                        if(!vec)
                            tkbio_fb_draw_rect(y*height, x*width, height,
                                width, elem->color >> 4, DENSITY, 0);
                        else
                        {
                            int i;
                            struct tkbio_point *p;
                            for(i=0; i<vector_size(vec); i++)
                            {
                                p = vector_at(i, vec);
                                tkbio_draw_field(p->y, p->x, height, width,
                                    p->elem->color >> 4, DENSITY, 0);
                            }
                        }
                        tkbio.fb.fillColor = elem->color;
                        tkbio.fb.status = FB_STATUS_FILL;
                    }
                }
            }
            
            tkbio.parser.y = y;
            tkbio.parser.x = x;
            tkbio.parser.py = mapY;
            tkbio.parser.px = mapX;
            tkbio.parser.pressed = 1;
            
            tkbio_set_pause();
        }
    }
    
    // notify framebuffer for redraw
    if(tkbio.sim)
    {
        unsigned char tmp = 'x';
        send(tkbio.fb.sock, &tmp, 1, 0);
    }
    
    return ret;
}

int tkbio_run(tkbio_handler *handler, void *state)
{
    struct pollfd pfds[1];
    pfds[0].fd = tkbio.sock;
    pfds[0].events = POLLIN;
    
    int ret;
    
    DEBUG(printf("[TKBIO] run\n"));
    
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
            DEBUG(printf("[TKBIO] Failed to poll rpc socket\n"));
            tkbio_open_socket(-1);
        }
    }
    
    return 0;
}
