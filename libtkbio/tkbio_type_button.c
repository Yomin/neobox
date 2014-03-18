/*
 * Copyright (c) 2014 Martin Rödel aka Yomin
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <tsp.h>

#include "tkbio_fb.h"
#include "tkbio_type_button.h"

#define COPY(e)   ((e)->type & TKBIO_LAYOUT_OPTION_COPY)
#define BORDER(e) ((e)->type & TKBIO_LAYOUT_OPTION_BORDER)

extern struct tkbio_global tkbio;

static int copy_size;
static unsigned char *copy;

static void alloc_copy(int height, int width, struct tkbio_partner *partner)
{
    int size = (width/DENSITY) * (height/DENSITY) * tkbio.fb.bpp;
    
    if(partner)
        size *= vector_size(partner->connect);
    
    if(!copy_size)
    {
        copy = malloc(size);
        copy_size = size;
    }
    else if(copy_size < size)
    {
        copy = realloc(copy, size);
        copy_size = size;
    }
}

void tkbio_type_button_init(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    copy_size = 0;
}

void tkbio_type_button_finish(struct tkbio_save *save)
{
    if(copy_size)
    {
        free(copy);
        copy_size = 0;
    }
}

int tkbio_type_button_broader(int *y, int *x, int scr_y, int scr_x, const struct tkbio_mapelem *elem)
{
    int scr_height, scr_width, fb_y, fb_x;
    
    tkbio_get_sizes_current(0, 0, 0, 0, &scr_height, &scr_width);
    
    fb_y = tkbio.parser.y;
    fb_x = tkbio.parser.x;
    tkbio_layout_to_fb_cords(&fb_y, &fb_x);
    
    if(    scr_x >= scr_width*fb_x - scr_width*(INCREASE/100.0)
        && scr_x <= scr_width*(fb_x+1) + scr_width*(INCREASE/100.0)
        && (SCREENMAX - scr_y) >= scr_height*fb_y - scr_height*(INCREASE/100.0)
        && (SCREENMAX - scr_y) <= scr_height*(fb_y+1) + scr_height*(INCREASE/100.0))
    {
        *x = tkbio.parser.x;
        *y = tkbio.parser.y;
        return 0;
    }
    return 1;
}

void tkbio_type_button_press(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    int i, height, width;
    unsigned char *ptr = 0;
    struct vector *connect;
    struct tkbio_point *p;
    
    tkbio_get_sizes(map, &height, &width, 0, 0, 0, 0);
    
    if(elem->type & TKBIO_LAYOUT_OPTION_COPY)
    {
        alloc_copy(height, width, save->partner);
        ptr = copy;
    }
    
    if(!save->partner)
        tkbio_layout_draw_rect(y*height, x*width, height, width,
            elem->color >> 4, DENSITY, &ptr);
    else
    {
        connect = save->partner->connect;
        for(i=0; i<vector_size(connect); i++)
        {
            p = vector_at(i, connect);
            tkbio_layout_draw_rect(p->y*height, p->x*width,
                height, width, p->elem->color >> 4, DENSITY, &ptr);
        }
    }
}

void tkbio_type_button_move(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    
}

struct tkbio_return tkbio_type_button_release(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_return ret;
    struct tsp_cmd cmd;
    int nmap = tkbio.parser.map;
    
    ret.type = TKBIO_RETURN_NOP;
    ret.id = elem->id;
    
    switch(elem->type & TKBIO_LAYOUT_MASK_TYPE)
    {
    case TKBIO_LAYOUT_TYPE_CHAR:
        ret.type = TKBIO_RETURN_CHAR;
        if(tkbio.layout.fun) // layout specific convert function
            ret.value = tkbio.layout.fun(tkbio.parser.map, elem->elem,
                tkbio.parser.toggle);
        else
            ret.value = elem->elem;
        tkbio.parser.toggle = 0;
        VERBOSE(if(elem->name)
            printf("[%s]", elem->name);
        else
            printf("[%c]", elem->elem.c.c[0]));
        break;
    case TKBIO_LAYOUT_TYPE_GOTO:
        nmap = elem->elem.i + map->offset;
        tkbio.parser.hold = 0;
        VERBOSE(printf("goto %s", elem->name));
        goto ret;
    case TKBIO_LAYOUT_TYPE_HOLD:
        tkbio.parser.hold = !tkbio.parser.hold;
        VERBOSE(printf("hold %s", tkbio.parser.hold ? "on" : "off"));
        break;
    case TKBIO_LAYOUT_TYPE_TOGGLE:
        tkbio.parser.toggle ^= elem->elem.i;
        VERBOSE(printf("%s %s", elem->name,
            tkbio.parser.toggle & elem->elem.i ? "on" : "off"));
        break;
    case TKBIO_LAYOUT_TYPE_SYSTEM:
        switch(elem->elem.i)
        {
        case TKBIO_SYSTEM_NEXT:
            VERBOSE(printf("app switch next"));
            ret.type = TKBIO_RETURN_SWITCH;
            cmd.cmd = TSP_CMD_NEXT;
            break;
        case TKBIO_SYSTEM_PREV:
            VERBOSE(printf("app switch prev"));
            ret.type = TKBIO_RETURN_SWITCH;
            cmd.cmd = TSP_CMD_PREV;
            break;
        case TKBIO_SYSTEM_QUIT:
            VERBOSE(printf("app quit"));
            ret.type = TKBIO_RETURN_QUIT;
            cmd.cmd = TSP_CMD_REMOVE;
            break;
        case TKBIO_SYSTEM_ACTIVATE:
            VERBOSE(printf("app activate"));
            break;
        case TKBIO_SYSTEM_MENU:
            VERBOSE(printf("app menu"));
            // todo
            break;
        }
        
        if(ret.type != TKBIO_RETURN_NOP)
        {
            cmd.value = 0;
            send(tkbio.sock, &cmd, sizeof(struct tsp_cmd), 0);
        }
        
        break;
    }
    
    if(!tkbio.parser.hold) // reset map to default if not on hold
        nmap = tkbio.layout.start;
    
ret:
    tkbio_type_button_focus_out(map, elem, save);
    
    tkbio.parser.map = nmap;
    
    return ret;
}

void tkbio_type_button_focus_in(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    tkbio_type_button_press(y, x, button_y, button_x, map, elem, save);
}

void tkbio_type_button_focus_out(const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    int i, height, width;
    unsigned char *ptr;
    struct vector *connect;
    struct tkbio_point *p;
    
    tkbio_get_sizes(map, &height, &width, 0, 0, 0, 0);
    
    if(COPY(elem))
    {
        ptr = copy;
        if(!save->partner)
            tkbio_layout_fill_rect(tkbio.parser.y*height,
                tkbio.parser.x*width, height, width, DENSITY, &ptr);
        else
        {
            connect = save->partner->connect;
            for(i=0; i<vector_size(connect); i++)
            {
                p = vector_at(i, connect);
                tkbio_layout_fill_rect(p->y*height, p->x*width,
                    height, width, DENSITY, &ptr);
            }
        }
    }
    else
    {
        if(!save->partner)
        {
            if(BORDER(elem))
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
        {
            connect = save->partner->connect;
            for(i=0; i<vector_size(connect); i++)
            {
                p = vector_at(i, connect);
                if(BORDER(p->elem))
                    tkbio_layout_draw_rect_connect(p->y*height,
                        p->x*width, p->y, p->x, height, width,
                        p->elem->color, p->elem->connect, DENSITY, 0);
                else
                    tkbio_layout_draw_rect(p->y*height, p->x*width,
                        height, width, p->elem->color & 15, DENSITY, 0);
            }
        }
    }
}
