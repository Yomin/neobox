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

#include "neobox_fb.h"
#include "neobox_type_button.h"
#include "neobox_type_help.h"

#define COPY(e)    ((e)->options & NEOBOX_LAYOUT_OPTION_COPY)
#define BORDER(e)  ((e)->options & NEOBOX_LAYOUT_OPTION_BORDER)
#define CONNECT(e) ((e)->options & NEOBOX_LAYOUT_OPTION_MASK_CONNECT)
#define ALIGN(e)   ((e)->options & NEOBOX_LAYOUT_OPTION_MASK_ALIGN)
#define NOP        (struct neobox_return) { .type = NEOBOX_RETURN_NOP }

extern struct neobox_global neobox;

static int docopy;
int button_copy_size;
unsigned char *button_copy;

static void alloc_copy(int height, int width, struct neobox_partner *partner)
{
    int size = (width/DENSITY) * (height/DENSITY) * neobox.fb.bpp;
    
    if(partner)
        size *= vector_size(partner->connect);
    
    if(!button_copy_size)
    {
        button_copy = malloc(size);
        button_copy_size = size;
    }
    else if(button_copy_size < size)
    {
        button_copy = realloc(button_copy, size);
        button_copy_size = size;
    }
}

void neobox_type_button_init(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    button_copy_size = 0;
    docopy = 1;
}

void neobox_type_button_finish(struct neobox_save *save)
{
    if(button_copy_size)
    {
        free(button_copy);
        button_copy_size = 0;
    }
}

void neobox_type_button_draw(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    docopy = 0;
    neobox_type_button_focus_out(y, x, map, elem, save);
    docopy = 1;
}

int neobox_type_button_broader(int *y, int *x, int scr_y, int scr_x, const struct neobox_mapelem *elem)
{
    int scr_height, scr_width, fb_y, fb_x;
    
    neobox_get_sizes_current(0, 0, 0, 0, &scr_height, &scr_width);
    
    fb_y = neobox.parser.y;
    fb_x = neobox.parser.x;
    neobox_layout_to_fb_cords(&fb_y, &fb_x);
    
    if(    scr_x >= scr_width*fb_x - scr_width*(INCREASE/100.0)
        && scr_x <= scr_width*(fb_x+1) + scr_width*(INCREASE/100.0)
        && (SCREENMAX - scr_y) >= scr_height*fb_y - scr_height*(INCREASE/100.0)
        && (SCREENMAX - scr_y) <= scr_height*(fb_y+1) + scr_height*(INCREASE/100.0))
    {
        *x = neobox.parser.x;
        *y = neobox.parser.y;
        return 0;
    }
    return 1;
}

struct neobox_return neobox_type_button_press(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    int i, height, width;
    unsigned char *ptr = 0, color;
    const char *text;
    struct vector *connect;
    struct neobox_point *p, *p2;
    
    neobox_get_sizes(map, &height, &width, 0, 0, 0, 0);
    
    if(COPY(elem))
    {
        alloc_copy(height, width, save->partner);
        ptr = button_copy;
    }
    
    text = save->partner && save->partner->data ? save->partner->data :
        (!save->partner && save->data ? save->data :
        (elem->name ? elem->name :
        (elem->elem.i ? elem->elem.c.c : 0)));
    color = elem->color_text == elem->color_fg ?
        elem->color_bg : elem->color_text;
    
    if(!save->partner)
    {
        neobox_layout_draw_rect(y*height, x*width, height, width,
            elem->color_fg, DENSITY, &ptr);
        
        if(text)
            neobox_layout_draw_string(y*height, x*width, height,
                width, color, ALIGN(elem), text);
    }
    else
    {
        connect = save->partner->connect;
        for(i=0; i<vector_size(connect); i++)
        {
            p = vector_at(i, connect);
            neobox_layout_draw_rect(p->y*height, p->x*width,
                height, width, p->elem->color_fg, DENSITY, &ptr);
        }
        
        if(text)
        {
            p2 = vector_at(0, connect);
            neobox_layout_draw_string(p2->y*height, p2->x*width,
                (p->y-p2->y+1)*height, (p->x-p2->x+1)*width,
                color, ALIGN(elem), text);
        }
    }
    
    return NOP;
}

struct neobox_return neobox_type_button_move(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    return NOP;
}

struct neobox_return neobox_type_button_release(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    struct neobox_return ret;
    int nmap = neobox.parser.map;
    
    ret.type = NEOBOX_RETURN_NOP;
    ret.id = elem->id;
    
    switch(elem->type)
    {
    case NEOBOX_LAYOUT_TYPE_CHAR:
        ret.type = NEOBOX_RETURN_CHAR;
        if(neobox.layout.fun) // layout specific convert function
            ret.value = neobox.layout.fun(neobox.parser.map, elem->elem,
                neobox.parser.toggle);
        else
            ret.value = elem->elem;
        neobox.parser.toggle = 0;
        if(neobox.verbose)
        {
            if(elem->name)
                printf("[NEOBOX] button %i [%s]\n", elem->id, elem->name);
            else if(elem->elem.c.c[0])
                printf("[NEOBOX] button %i [%c]\n", elem->id, elem->elem.c.c[0]);
            else
                printf("[NEOBOX] button %i\n", elem->id);
        }
        break;
    case NEOBOX_LAYOUT_TYPE_GOTO:
        nmap = elem->elem.i + map->offset;
        neobox.parser.hold = 0;
        VERBOSE(printf("[NEOBOX] goto %s\n", elem->name));
        goto ret;
    case NEOBOX_LAYOUT_TYPE_HOLD:
        neobox.parser.hold = !neobox.parser.hold;
        VERBOSE(printf("[NEOBOX] hold %s\n",
            neobox.parser.hold ? "on" : "off"));
        break;
    case NEOBOX_LAYOUT_TYPE_TOGGLE:
        neobox.parser.toggle ^= elem->elem.i;
        VERBOSE(printf("[NEOBOX] %s %s\n", elem->name,
            neobox.parser.toggle & elem->elem.i ? "on" : "off"));
        break;
    case NEOBOX_LAYOUT_TYPE_SYSTEM:
        ret.type = NEOBOX_RETURN_SYSTEM;
        ret.value.i = elem->elem.i;
        if(neobox.verbose)
            switch(elem->elem.i)
            {
            case NEOBOX_SYSTEM_NEXT:
                printf("[NEOBOX] app switch next\n");
                break;
            case NEOBOX_SYSTEM_PREV:
                printf("[NEOBOX] app switch prev\n");
                break;
            case NEOBOX_SYSTEM_QUIT:
                printf("[NEOBOX] app quit\n");
                break;
            case NEOBOX_SYSTEM_ACTIVATE:
                printf("[NEOBOX] app activate\n");
                break;
            case NEOBOX_SYSTEM_MENU:
                printf("[NEOBOX] app menu\n");
                break;
            }
        break;
    }
    
    if(!neobox.parser.hold) // reset map to default if not on hold
        nmap = neobox.layout.start;
    
ret:
    neobox_type_button_focus_out(y, x, map, elem, save);
    
    neobox.parser.map = nmap;
    
    return ret;
}

struct neobox_return neobox_type_button_focus_in(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    return neobox_type_button_press(y, x, button_y, button_x, map, elem, save);
}

struct neobox_return neobox_type_button_focus_out(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    int i, height, width;
    unsigned char *ptr;
    const char *text;
    struct vector *connect;
    struct neobox_point *p, *p2;
    
    neobox_get_sizes(map, &height, &width, 0, 0, 0, 0);
    
    if(COPY(elem) && docopy)
    {
        ptr = button_copy;
        if(!save->partner)
            neobox_layout_fill_rect(y*height, x*width,
                height, width, DENSITY, &ptr);
        else
        {
            connect = save->partner->connect;
            for(i=0; i<vector_size(connect); i++)
            {
                p = vector_at(i, connect);
                neobox_layout_fill_rect(p->y*height, p->x*width,
                    height, width, DENSITY, &ptr);
            }
        }
    }
    else
    {
        text = save->partner && save->partner->data ? save->partner->data :
            (!save->partner && save->data ? save->data :
            (elem->name ? elem->name :
            (elem->elem.i ? elem->elem.c.c : 0)));
        
        if(!save->partner)
        {
            if(BORDER(elem))
                neobox_layout_draw_rect_connect(y*height, x*width,
                    y, x, height, width, elem->color_fg, elem->color_bg,
                    CONNECT(elem), DENSITY, 0);
            else
                neobox_layout_draw_rect(y*height, x*width,
                    height, width, elem->color_bg, DENSITY, 0);
            
            if(text)
                neobox_layout_draw_string(y*height, x*width, height,
                    width, elem->color_text, ALIGN(elem), text);
        }
        else
        {
            connect = save->partner->connect;
            for(i=0; i<vector_size(connect); i++)
            {
                p = vector_at(i, connect);
                if(BORDER(p->elem))
                    neobox_layout_draw_rect_connect(p->y*height,
                        p->x*width, p->y, p->x, height, width,
                        p->elem->color_fg, p->elem->color_bg,
                        CONNECT(p->elem), DENSITY, 0);
                else
                    neobox_layout_draw_rect(p->y*height, p->x*width,
                        height, width, p->elem->color_bg, DENSITY, 0);
            }
            
            if(text)
            {
                p2 = vector_at(0, connect);
                neobox_layout_draw_string(p2->y*height, p2->x*width,
                    (p->y-p2->y+1)*height, (p->x-p2->x+1)*width,
                    p->elem->color_text, ALIGN(elem), text);
            }
        }
    }
    
    return NOP;
}

void neobox_type_button_set_name(const void *name, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    if(!save->partner)
        save->data = (void*)name;
    else
        save->partner->data = (void*)name;
    
    if(map)
        neobox_type_button_draw(y, x, map, elem, save);
}

void neobox_button_set_name(int id , int mappos, const char *name, int redraw)
{
    neobox_type_help_set_range_value(NEOBOX_LAYOUT_TYPE_CHAR,
        NEOBOX_LAYOUT_TYPE_SYSTEM, id, mappos, name, redraw,
        neobox_type_button_set_name);
}
