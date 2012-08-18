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

int tkbio_init_custom(const char *name, struct tkbio_config config)
{
    DEBUG(printf("tkbio init\n"));
    
    // register socket
    if((tkbio.fd_rpc = open("/var/" RPCNAME "/rpc", O_WRONLY|O_NONBLOCK)) < 0)
    {
        DEBUG(perror("Failed to open rpc socket"));
        return TKBIO_ERROR_RPC_OPEN;
    }
    tkbio.id = tkbio_hash(name);
    write(tkbio.fd_rpc, &tkbio.id, sizeof(int));
    
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
    
    // init parser
    tkbio.parser.map = config.layout.start;
    tkbio.parser.toggle = 0;
    tkbio.parser.hold = 0;
    tkbio.parser.pressed = 0;
    
    // open screen socket
    char buf[256];
    snprintf(buf, 256, "/var/" RPCNAME "/socket_%i", tkbio.id);
    int tries = 3;
    while((tkbio.fd_sc = open(buf, O_RDONLY|O_NONBLOCK)) < 0)
    {
        DEBUG(perror("Failed to open screen socket"));
        if(!--tries)
        {
            DEBUG(perror("Unable to open screen socket"));
            close(tkbio.fd_rpc);
            close(tkbio.fb.fd);
            return TKBIO_ERROR_SCREEN_OPEN;
        }
        sleep(1);
    }
    
    // activate socket
    tkbio.id |= MSB;
    write(tkbio.fd_rpc, &tkbio.id, sizeof(int));
    tkbio.id &= ~MSB;
    
    // else
    tkbio.pause = 0;
    tkbio.custom_signal_handler = 0;
    signal(SIGALRM, tkbio_signal_handler);
    
    // print initial screen
    struct tkbio_map *map = (struct tkbio_map*) &tkbio.layout.maps[tkbio.parser.map];
    struct tkbio_mapelem *elem;
    int y, x, fy, fx, fheight = map->height, fwidth = map->width;
    tkbio_layout_to_fb_sizes(&fheight, &fwidth, 0, 0);
    for(y=0; y<map->height; y++)
        for(x=0; x<map->width; x++)
        {
            fy = y; fx = x;
            tkbio_layout_to_fb_cords(&fy, &fx, map->height);
            elem = (struct tkbio_mapelem*) &map->map[y*map->width+x];
            if(!(elem->type & TKBIO_LAYOUT_OPTION_COPY))
                tkbio_fb_draw_rect(fy*fheight, fx*fwidth, fheight, fwidth, elem->color & 15, DENSITY, 0);
            if(elem->type & TKBIO_LAYOUT_OPTION_BORDER)
                tkbio_fb_draw_rect_border(fy*fheight, fx*fwidth, fheight, fwidth, elem->color >> 4, DENSITY, 0);
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
    tkbio_fb_to_layout_cords(&mapY, &mapX, map->height);
    struct tkbio_mapelem *elem = (struct tkbio_mapelem*) &map->map[mapY*map->width+mapX];
    
    if(msg & SMSB) // moved
    {
        if(tkbio.parser.pressed && !tkbio.pause &&
          (tkbio.parser.y != y || tkbio.parser.x != x))
        {
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
            
            switch(tkbio.fb.status)
            {
                case FB_STATUS_NOP:
                    break;
                case FB_STATUS_COPY:
                    if(tkbio.fb.copy)
                        tkbio_fb_fill_rect(tkbio.parser.y*height, tkbio.parser.x*width,
                            height, width, DENSITY, tkbio.fb.copy);
                    break;
                case FB_STATUS_FILL:
                    tkbio_fb_draw_rect(tkbio.parser.y*height, tkbio.parser.x*width,
                        height, width, elem->color & 15, DENSITY, 0);
                    break;
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
                    if(elem->type & TKBIO_LAYOUT_OPTION_COPY)
                    {
                        int size = (width/DENSITY)*(height/DENSITY)*tkbio.fb.bpp*sizeof(char);
                        
                        if(!tkbio.fb.copy)
                        {
                            tkbio.fb.copy = malloc(size);
                            tkbio.fb.copySize = size;
                        }
                        else if(tkbio.fb.copySize < size)
                        {
                            tkbio.fb.copy = realloc(tkbio.fb.copy, size);
                            tkbio.fb.copySize = size;
                        }
                        
                        tkbio_fb_draw_rect(y*height, x*width, height, width,
                            elem->color >> 4, DENSITY, tkbio.fb.copy);
                        tkbio.fb.copyColor = elem->color;
                        tkbio.fb.status = FB_STATUS_COPY;
                    }
                    else
                    {
                        tkbio_fb_draw_rect(y*height, x*width, height, width,
                            elem->color >> 4, DENSITY, 0);
                        tkbio.fb.status = FB_STATUS_FILL;
                    }
                }
            }
            
            tkbio.parser.y = y;
            tkbio.parser.x = x;
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
            DEBUG(printf("Failed to poll screen socket"));
            return TKBIO_ERROR_SCREEN_POLL;
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
