/*
 * Copyright (c) 2012 Martin Rödel aka Yomin
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

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "tkbio_layout_default.h"

struct tkbio_global tkbio;

struct tkbio_point
{
    int y, x;
};

int tkbio_hash(const char *name)
{
    int h = 0;
    while(*name)
        h += *(name++);
    return h;
}

void tkbio_set_signal_handler(void handler(int signal))
{
    tkbio.custom_signal_handler = handler;
}

void tkbio_signal_handler(int signal)
{
    if(signal == SIGALRM)
        tkbio.pause = 0;
    else if(tkbio.custom_signal_handler)
        tkbio.custom_signal_handler(signal);
}

void tkbio_connect(int posA, int posB, int map)
{
    int width = tkbio.layout.maps[map].width;
    struct vector *va = &tkbio.connect[map][posA];
    struct vector *vb = &tkbio.connect[map][posB];
    struct vector *vpa, *vpb;
    struct tkbio_point pa = { .y = posA/width, .x = posA%width };
    struct tkbio_point pb = { .y = posB/width, .x = posB%width };
    struct tkbio_point *ppa, *ppb;
    int count_a = vector_size(va);
    int count_b = vector_size(vb);
    int i, j;
        
    vector_push(&pb, va);
    vector_push(&pa, vb);
    
    for(i=0; i<count_b; i++)
    {
        // pos of one partner of b
        ppb = vector_at(i, vb);
        // partners of one partner of b
        vpb = &tkbio.connect[map][ppb->y*width+ppb->x];
        
        // set partner at ppb as partner of a
        vector_push(ppb, va);
        // set a as partner of partner at ppb
        vector_push(&pa, vpb);
    }
    for(i=0; i<count_a; i++)
    {
        // pos of one partner of a
        ppa = vector_at(i, va);
        // partners of one partner of a
        vpa = &tkbio.connect[map][ppa->y*width+ppa->x];
        
        // set partner at ppa as partner of b
        vector_push(ppa, vb);
        // set b as partner of partner at ppa
        vector_push(&pb, vpa);
    }
    // set all partners of p as partners of all partners of me
    for(i=0; i<count_a; i++)
    {
        // pos of one partner of a
        ppa = vector_at(i, va);
        // partners of one partner of a
        vpa = &tkbio.connect[map][ppa->y*width+ppa->x];
        
        for(j=0; j<count_b; j++)
        {
            // pos of one partner of b
            ppb = vector_at(j, vb);
            // partners of one partner of b
            vpb = &tkbio.connect[map][ppb->y*width+ppb->x];
            
            // set partner of b at ppb as partner of the partner of a at ppa
            vector_push(ppb, vpa);
            // set partner of a at ppa as partner of the partner of b at ppb
            vector_push(ppa, vpb);
        }
    }
}

int tkbio_register_socket(int tries)
{
    while((tkbio.fd_rpc = open("/var/" RPCNAME "/rpc", O_WRONLY|O_NONBLOCK)) < 0)
    {
        DEBUG(perror("Failed to open rpc socket"));
        if(tries > 0 && !--tries)
        {
            DEBUG(perror("Unable to open rpc socket"));
            return TKBIO_ERROR_RPC_OPEN;
        }
        sleep(1);
    }
    
    write(tkbio.fd_rpc, &tkbio.id, sizeof(int));
    return 0;
}

int tkbio_activate_socket(int tries)
{
    char buf[256];
    snprintf(buf, 256, "/var/" RPCNAME "/socket_%i", tkbio.id);
    while((tkbio.fd_sc = open(buf, O_RDONLY|O_NONBLOCK)) < 0)
    {
        DEBUG(perror("Failed to open screen socket"));
        if(tries > 0 && !--tries)
        {
            DEBUG(perror("Unable to open screen socket"));
            return TKBIO_ERROR_SCREEN_OPEN;
        }
        sleep(1);
    }
    
    tkbio.id |= MSB;
    write(tkbio.fd_rpc, &tkbio.id, sizeof(int));
    tkbio.id &= ~MSB;
    return 0;
}

int tkbio_init_custom(const char *name, struct tkbio_config config)
{
    int ret;
    
    DEBUG(printf("tkbio init\n"));
    
    // register socket
    tkbio.id = tkbio_hash(name);
    if((ret = tkbio_register_socket(-1)) < 0)
        return ret;
    
    // open framebuffer
    if((tkbio.fb.fd = open(config.fb, O_RDWR)) < 0)
    {
        DEBUG(perror("Failed to open framebuffer"));
        close(tkbio.fd_rpc);
        return TKBIO_ERROR_FB_OPEN;
    }
    if(ioctl(tkbio.fb.fd, FBIOGET_VSCREENINFO, &(tkbio.fb.vinfo))) {
        DEBUG(perror("Failed to get variable screeninfo"));
        close(tkbio.fd_rpc);
        close(tkbio.fb.fd);
        return TKBIO_ERROR_FB_VINFO;
    }
    if(ioctl(tkbio.fb.fd, FBIOGET_FSCREENINFO, &(tkbio.fb.finfo))) {
        DEBUG(perror("Failed to get fixed screeninfo"));
        close(tkbio.fd_rpc);
        close(tkbio.fb.fd);
        return TKBIO_ERROR_FB_FINFO;
    }
    
    tkbio.fb.bpp = tkbio.fb.vinfo.bits_per_pixel/8;
    tkbio.fb.size = tkbio.fb.vinfo.xres*tkbio.fb.vinfo.yres*tkbio.fb.bpp;
    DEBUG(printf("framebuffer size %i x %i, ", tkbio.fb.vinfo.xres,tkbio.fb.vinfo.yres));
    DEBUG(printf("bytes per pixel %i, ", tkbio.fb.bpp));
    DEBUG(printf("line length %i\n", tkbio.fb.finfo.line_length));
    
    if((tkbio.fb.ptr = (char*) mmap(0, tkbio.fb.size, PROT_READ|PROT_WRITE, MAP_SHARED, tkbio.fb.fd, 0)) == MAP_FAILED)
    {
        DEBUG(perror("Failed to mmap framebuffer"));
        close(tkbio.fd_rpc);
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
    tkbio.connect = malloc(config.layout.size*sizeof(struct tkbio_positions*));
    int i, j, width, size;
    for(i=0; i<config.layout.size; i++)
    {
        width = config.layout.maps[i].width;
        size = config.layout.maps[i].height*width;
        tkbio.connect[i] = malloc(size*sizeof(struct vector));
        for(j=0; j<size; j++)
        {
            struct vector *v = &tkbio.connect[i][j];
            vector_init(sizeof(struct tkbio_point), &v);
            if(j%width>0 && config.layout.maps[i].map[j].connect & TKBIO_LAYOUT_CONNECT_LEFT)
                tkbio_connect(j-1, j, i);
            if(j>=width && config.layout.maps[i].map[j].connect & TKBIO_LAYOUT_CONNECT_UP)
                tkbio_connect(j-width, j, i);
        }
    }
    
    // init parser
    tkbio.parser.map = config.layout.start;
    tkbio.parser.toggle = 0;
    tkbio.parser.hold = 0;
    tkbio.parser.pressed = 0;
    
    // activate socket
    if((ret = tkbio_activate_socket(-1)) < 0)
    {
        close(tkbio.fd_rpc);
        close(tkbio.fb.fd);
        return ret;
    }
    
    // else
    tkbio.pause = 0;
    tkbio.custom_signal_handler = 0;
    signal(SIGALRM, tkbio_signal_handler);
    
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
                    char borders = tkbio_connect_to_borders(fy, fx, elem->connect);
                    tkbio_fb_draw_rect_border(fy*fheight, fx*fwidth, fheight, fwidth, elem->color >> 4, borders, DENSITY, 0);
                }
            }
    }
    
    // return screen descriptor for manual polling
    return tkbio.fd_sc;
}

struct tkbio_config tkbio_default_config()
{
    struct tkbio_config config;
    config.fb = "/dev/fb0";
    config.layout = tkbLayoutDefault;
    config.format = TKBIO_FORMAT_LANDSCAPE;
    config.options = 0;
    return config;
}

int tkbio_init_default(const char *name)
{
    return tkbio_init_custom(name, tkbio_default_config());
}

void tkbio_finish()
{
    DEBUG(printf("tkbio finish\n"));
    close(tkbio.fd_rpc);
    close(tkbio.fb.fd);
    close(tkbio.fd_sc);
    if(tkbio.fb.copy)
        free(tkbio.fb.copy);
    
    int i, j, size;
    for(i=0; i<tkbio.layout.size; i++)
    {
        size = tkbio.layout.maps[i].height * tkbio.layout.maps[i].width;
        for(j=0; j<size; j++)
            vector_finish(&tkbio.connect[i][j]);
        free(tkbio.connect[i]);
    }
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
        DEBUG(perror("Failed to set timer"));
        sleep(1);
    }
}

struct tkbio_charelem tkbio_handle_event()
{
    int msg;
    read(tkbio.fd_sc, &msg, sizeof(int));
    
    struct tkbio_map *map = (struct tkbio_map*) &tkbio.layout.maps[tkbio.parser.map];
    struct tkbio_charelem ret = (struct tkbio_charelem) {{0, 0, 0, 0}};
    
    int mx = msg & ((1<<NIBBLE)-1);
    int my = ((msg & ~MSB) & ~SMSB) >> NIBBLE;
    
    if(mx >= SCREENMAX || my >= SCREENMAX)
        return ret;
    
    int width = map->width, height = map->height, scrWidth, scrHeight;
    tkbio_layout_to_fb_sizes(&height, &width, &scrHeight, &scrWidth);
    
    int y, x;
    
    if(tkbio.parser.pressed
        && mx >= scrWidth*tkbio.parser.x - scrWidth*(INCREASE/100.0)
        && mx <= scrWidth*(tkbio.parser.x+1) + scrWidth*(INCREASE/100.0)
        && (SCREENMAX - my) >= scrHeight*tkbio.parser.y - scrHeight*(INCREASE/100.0)
        && (SCREENMAX - my) <= scrHeight*(tkbio.parser.y+1) + scrHeight*(INCREASE/100.0))
    {
        x = tkbio.parser.x;
        y = tkbio.parser.y;
    }
    else
    {
        x = mx / scrWidth;
        y = (SCREENMAX - my) / scrHeight;
    }
    
    int mapY = y, mapX = x;
    tkbio_fb_to_layout_cords(&mapY, &mapX);
    struct tkbio_mapelem *elem = (struct tkbio_mapelem*) &map->map[mapY*map->width+mapX];
    
    if(msg & SMSB) // moved
    {
        if(tkbio.parser.pressed && !tkbio.pause &&
          (tkbio.parser.y != y || tkbio.parser.x != x))
        {
            struct vector *vec = &tkbio.connect
                [tkbio.parser.map][tkbio.parser.py*map->width+tkbio.parser.px];
            
            int i;
            struct tkbio_point *p;
            for(i=0; i<vector_size(vec); i++)
            {
                p = vector_at(i, vec);
                if(p->y == y && p->x == x)
                {
                    DEBUG(printf("Move to partner (%i,%i)\n", mapY, mapX));
                    tkbio.parser.y = y;
                    tkbio.parser.x = x;
                    return ret;
                }
            }
            
            DEBUG(printf("Move (%i,%i)\n", mapY, mapX));
            switch(tkbio.fb.status)
            {
                case FB_STATUS_NOP:
                    break;
                case FB_STATUS_COPY:
                    tkbio_fb_fill_rect(tkbio.parser.y*height, tkbio.parser.x*width,
                        height, width, DENSITY, tkbio.fb.copy);
                    break;
                case FB_STATUS_FILL:
                    tkbio_fb_draw_rect(tkbio.parser.y*height, tkbio.parser.x*width,
                        height, width, tkbio.fb.copyColor, DENSITY, 0);
                    break;
            }
            goto pressed;
        }
    }
    else if(msg & MSB) // released
    {
        if(tkbio.parser.pressed)
        {
            DEBUG(printf("Release (%i,%i)\n", mapY, mapX));
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
            
            struct vector *vec = &tkbio.connect
                [tkbio.parser.map][tkbio.parser.py*map->width+tkbio.parser.px];
            
            switch(tkbio.fb.status)
            {
                case FB_STATUS_NOP:
                    break;
                case FB_STATUS_COPY:
                    if(tkbio.fb.copy)
                    {
                        tkbio_fb_fill_rect(tkbio.parser.y*height, tkbio.parser.x*width,
                            height, width, DENSITY, tkbio.fb.copy);
                        
                        int i;
                        struct tkbio_point *p;
                        int size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp*sizeof(char);
                        char *ptr = tkbio.fb.copy+size;
                        for(i=0; i<vector_size(vec); i++, ptr += size)
                        {
                            p = vector_at(i, vec);
                            tkbio_fb_fill_rect(p->y*height, p->x*width, height,
                                width, DENSITY, ptr);
                        }
                    }
                    break;
                case FB_STATUS_FILL:
                {
                    tkbio_fb_draw_rect(tkbio.parser.y*height, tkbio.parser.x*width,
                        height, width, elem->color & 15, DENSITY, 0);
                    
                    int i;
                    struct tkbio_point *p;
                    for(i=0; i<vector_size(vec); i++)
                    {
                        p = vector_at(i, vec);
                        tkbio_fb_draw_rect(p->y*height, p->x*width, height,
                            width, elem->color & 15, DENSITY, 0);
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
            DEBUG(printf("Press (%i,%i)\n", mapY, mapX));
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
                    struct vector *vec = &tkbio.connect[tkbio.parser.map][y*map->width+x];
                    
                    if(elem->type & TKBIO_LAYOUT_OPTION_COPY)
                    {
                        int size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp*sizeof(char);
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
                        
                        tkbio_fb_draw_rect(y*height, x*width, height, width,
                            elem->color >> 4, DENSITY, tkbio.fb.copy);
                        
                        int i;
                        struct tkbio_point *p;
                        char *ptr = tkbio.fb.copy+size;
                        for(i=0; i<vector_size(vec); i++, ptr += size)
                        {
                            p = vector_at(i, vec);
                            tkbio_fb_draw_rect(p->y*height, p->x*width, height, width,
                                elem->color >> 4, DENSITY, ptr);
                        }
                        
                        tkbio.fb.copyColor = elem->color;
                        tkbio.fb.status = FB_STATUS_COPY;
                    }
                    else
                    {
                        tkbio_fb_draw_rect(y*height, x*width, height, width,
                            elem->color >> 4, DENSITY, 0);
                        
                        int i;
                        struct tkbio_point *p;
                        for(i=0; i<vector_size(vec); i++)
                        {
                            p = vector_at(i, vec);
                            tkbio_fb_draw_rect(p->y*height, p->x*width, height, width,
                                elem->color >> 4, DENSITY, 0);
                        }
                        
                        tkbio.fb.status = FB_STATUS_FILL;
                    }
                }
            }
            
            tkbio.parser.y = y;
            tkbio.parser.x = x;
            tkbio.parser.py = y;
            tkbio.parser.px = x;
            tkbio.parser.pressed = 1;
            
            tkbio_set_pause();
        }
    }
    return ret;
}

int tkbio_run(tkbio_handler *handler, void *state)
{
    struct pollfd pfds[1];
    pfds[0].fd = tkbio.fd_sc;
    pfds[0].events = POLLIN;
    
    int ret;
    
    DEBUG(printf("tkbio run\n"));
    
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
            DEBUG(printf("Failed to poll screen socket\n"));
            tkbio_register_socket(-1);
            tkbio_activate_socket(-1);
            pfds[0].fd = tkbio.fd_sc;
        }
    }
}

// - init -
// register socket
// open/mmap fb
// parse keyboard/screen layout
// enable/disable cursor
// print inital screen

// - register_handler -
// call function with char

// - draw_text/draw_point/draw_rect
