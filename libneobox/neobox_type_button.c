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
#include <string.h>
#include <sys/socket.h>

#include "neobox_type_button.h"
#include "neobox_fb.h"
#include "neobox_log.h"
#include "neobox_type_help.h"

#define BORDER(e)  ((e)->options & NEOBOX_LAYOUT_OPTION_BORDER)
#define CONNECT(e) ((e)->options & NEOBOX_LAYOUT_OPTION_MASK_CONNECT)
#define ALIGN(e)   ((e)->options & NEOBOX_LAYOUT_OPTION_MASK_ALIGN)
#define PASSWORD(e)((e)->options & NEOBOX_LAYOUT_OPTION_PASSWORD)
#define NOP        (struct neobox_event) { .type = NEOBOX_EVENT_NOP }
#define COPY       (!(neobox.options & NEOBOX_OPTION_PRINT_MASK) || map->invisible)

extern struct neobox_global neobox;

static int docopy; // deactivate copy on eg initial draw

// button copy is limited on one click therefore one save space is sufficient
int button_copy_size, button_copy_size_backup;
unsigned char *button_copy, *button_copy_backup;
struct neobox_save_button button_copy_save;

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

TYPE_FUNC_INIT(button)
{
    button_copy_size = 0;
    docopy = 1;
    
    if(save->partner)
    {
        if(!save->partner->data)
            save->partner->data = calloc(1, sizeof(struct neobox_save_button));
    }
    else
        save->data = calloc(1, sizeof(struct neobox_save_button));
}

TYPE_FUNC_FINISH(button)
{
    if(button_copy_size)
    {
        free(button_copy);
        button_copy_size = 0;
    }
    
    if(save->partner)
        free(save->partner->data);
    else
        free(save->data);
}

TYPE_FUNC_DRAW(button)
{
    docopy = 0; // disable copy on initial draw
    TYPE_FUNC_FOCUS_OUT_CALL(button);
    docopy = 1;
}

TYPE_FUNC_BROADER(button)
{
    int scr_height, scr_width, fb_y, fb_x;
    
    neobox_get_sizes(0, 0, 0, 0, &scr_height, &scr_width, map);
    
    fb_y = neobox.parser.y;
    fb_x = neobox.parser.x;
    neobox_layout_to_fb_cords(&fb_y, &fb_x, map);
    
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

TYPE_FUNC_PRESS(button)
{
    int i, height, width;
    unsigned char *ptr = 0, color_text;
    const char *text;
    struct vector *connect;
    struct neobox_point *p, *p2;
    struct neobox_save_button *button;
    
    neobox_get_sizes(&height, &width, 0, 0, 0, 0, map);
    
    if(COPY)
    {
        alloc_copy(height, width, save->partner);
        ptr = button_copy;
    }
    
    button = save->partner ? save->partner->data : save->data;
    
    text = button->name ? button->name :
           elem->name ? elem->name :
           elem->elem.i ? elem->elem.c.c : 0;
    
    color_text = elem->color_text == elem->color_fg ?
        elem->color_bg : elem->color_text;
    
    if(!save->partner)
    {
        neobox_layout_draw_rect(y*height, x*width, height, width,
            elem->color_fg, DENSITY, map, &ptr);
        
        if(text)
            neobox_layout_draw_string(y*height, x*width, height,
                width, elem->color_fg, elem->color_bg, color_text,
                ALIGN(elem), PASSWORD(elem), text, map);
    }
    else
    {
        connect = save->partner->connect;
        for(i=0; i<vector_size(connect); i++)
        {
            p = vector_at(i, connect);
            neobox_layout_draw_rect(p->y*height, p->x*width,
                height, width, p->elem->color_fg, DENSITY, map, &ptr);
        }
        
        if(text)
        {
            p2 = vector_at(0, connect);
            neobox_layout_draw_string(p2->y*height, p2->x*width,
                (p->y-p2->y+1)*height, (p->x-p2->x+1)*width,
                elem->color_fg, elem->color_bg, color_text,
                ALIGN(elem), PASSWORD(elem), text, map);
        }
    }
    
    return NOP;
}

TYPE_FUNC_MOVE(button)
{
    return NOP;
}

TYPE_FUNC_RELEASE(button)
{
    struct neobox_event ret;
    struct neobox_save_button *button;
    int nmap = neobox.parser.map;
    
    button = save->partner ? save->partner->data : save->data;
    
    ret.type = NEOBOX_EVENT_NOP;
    ret.id = elem->id;
    
    switch(elem->type)
    {
    case NEOBOX_LAYOUT_TYPE_CHAR:
        if(button->check && button->check(elem->id) != NEOBOX_BUTTON_CHECK_SUCCESS)
            goto ret;
        ret.type = NEOBOX_EVENT_CHAR;
        if(neobox.layout.fun) // layout specific convert function
            ret.value = neobox.layout.fun(neobox.parser.map, elem->elem,
                neobox.parser.toggle);
        else
            ret.value = elem->elem;
        neobox.parser.toggle = 0;
        if(elem->name)
            neobox_printf(1, "button %i [%s]\n", elem->id, elem->name);
        else if(elem->elem.c.c[0])
            neobox_printf(1, "button %i [%c]\n", elem->id, elem->elem.c.c[0]);
        else
            neobox_printf(1, "button %i\n", elem->id);
        break;
    case NEOBOX_LAYOUT_TYPE_GOTO:
        if(button->check && button->check(elem->id) != NEOBOX_BUTTON_CHECK_SUCCESS)
            goto ret;
        if(!strcmp("Admn", elem->name) && !(neobox.options & NEOBOX_OPTION_ADMINMAP))
        {
            neobox_printf(1, "adminmap disabled\n");
            goto ret;
        }
        nmap = elem->elem.i + map->offset;
        neobox.parser.hold = 0;
        neobox_printf(1, "goto %s\n", elem->name);
        goto ret;
    case NEOBOX_LAYOUT_TYPE_HOLD:
        if(button->check && button->check(elem->id) != NEOBOX_BUTTON_CHECK_SUCCESS)
            goto ret;
        neobox.parser.hold = !neobox.parser.hold;
        neobox_printf(1, "hold %s\n",
            neobox.parser.hold ? "on" : "off");
        break;
    case NEOBOX_LAYOUT_TYPE_TOGGLE:
        if(button->check && button->check(elem->id) != NEOBOX_BUTTON_CHECK_SUCCESS)
            goto ret;
        neobox.parser.toggle ^= elem->elem.i;
        neobox_printf(1, "%s %s\n", elem->name,
            neobox.parser.toggle & elem->elem.i ? "on" : "off");
        break;
    case NEOBOX_LAYOUT_TYPE_SYSTEM:
        ret.type = NEOBOX_EVENT_SYSTEM;
        ret.value.i = elem->elem.i;
        switch(elem->elem.i)
        {
        case NEOBOX_SYSTEM_NEXT:
            neobox_printf(1, "app switch next\n");
            break;
        case NEOBOX_SYSTEM_PREV:
            neobox_printf(1, "app switch prev\n");
            break;
        case NEOBOX_SYSTEM_QUIT:
            neobox_printf(1, "app quit\n");
            break;
        case NEOBOX_SYSTEM_ACTIVATE:
            neobox_printf(1, "app activate\n");
            break;
        case NEOBOX_SYSTEM_MENU:
            neobox_printf(1, "app menu\n");
            break;
        }
    break;
    }
    
    if(!neobox.parser.hold) // reset map to default if not on hold
        nmap = neobox.parser.map_main;
    
ret:
    TYPE_FUNC_FOCUS_OUT_CALL(button);
    
    neobox.parser.map = nmap;
    
    return ret;
}

TYPE_FUNC_FOCUS_IN(button)
{
    return TYPE_FUNC_PRESS_CALL(button);
}

TYPE_FUNC_FOCUS_OUT(button)
{
    int i, height, width;
    unsigned char *ptr;
    const char *text;
    struct vector *connect;
    struct neobox_point *p, *p2;
    struct neobox_save_button *button;
    
    neobox_get_sizes(&height, &width, 0, 0, 0, 0, map);
    
    if(COPY && docopy) // only copy if invisible map and not initial draw
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
        button = save->partner ? save->partner->data : save->data;
        
        text = button->name ? button->name :
               elem->name ? elem->name :
               elem->elem.i ? elem->elem.c.c : 0;
        
        if(!save->partner)
        {
            if(BORDER(elem))
                neobox_layout_draw_rect_connect(y*height, x*width,
                    y, x, height, width, elem->color_fg, elem->color_bg,
                    CONNECT(elem), DENSITY, map, 0);
            else
                neobox_layout_draw_rect(y*height, x*width,
                    height, width, elem->color_bg, DENSITY, map, 0);
            
            if(text)
                neobox_layout_draw_string(y*height, x*width, height,
                    width, elem->color_fg, elem->color_bg, elem->color_text,
                    ALIGN(elem), PASSWORD(elem), text, map);
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
                        CONNECT(p->elem), DENSITY, map, 0);
                else
                    neobox_layout_draw_rect(p->y*height, p->x*width,
                        height, width, p->elem->color_bg, DENSITY, map, 0);
            }
            
            if(text)
            {
                p2 = vector_at(0, connect);
                neobox_layout_draw_string(p2->y*height, p2->x*width,
                    (p->y-p2->y+1)*height, (p->x-p2->x+1)*width,
                    p->elem->color_fg, p->elem->color_bg, p->elem->color_text,
                    ALIGN(elem), PASSWORD(elem), text, map);
            }
        }
    }
    
    return NOP;
}

TYPE_FUNC_ACTION(button, set_name)
{
    struct neobox_save_button *button;
    
    button = save->partner ? save->partner->data : save->data;
    
    button->name = data;
    
    if(button->name)
        neobox_printf(1, "%s %i [%s] renamed to '%s'\n",
            elem->type==NEOBOX_LAYOUT_TYPE_NOP?"label":"button",
            elem->id, elem->name, button->name);
    else
        neobox_printf(1, "%s %i [%s] name removed\n",
            elem->type==NEOBOX_LAYOUT_TYPE_NOP?"label":"button",
            elem->id, elem->name);
    
    if(map)
        TYPE_FUNC_DRAW_CALL(button);
    
    return 0;
}

TYPE_FUNC_ACTION(button, set_check)
{
    struct neobox_save_button *button, *sdata = data;
    
    button = save->partner ? save->partner->data : save->data;
    
    button->check = sdata->check;
    
    neobox_printf(1, "button %i [%s] check func %s\n",
        elem->id, elem->name, button->check?"assigned":"removed");
    
    if(map)
        TYPE_FUNC_DRAW_CALL(button);
    
    return 0;
}

void neobox_button_set_name(int id, int mappos, char *name, int redraw)
{
    neobox_type_help_action_range(NEOBOX_LAYOUT_TYPE_CHAR,
        NEOBOX_LAYOUT_TYPE_SYSTEM, id, mappos, name, redraw,
        neobox_type_button_set_name);
}

void neobox_button_set_check(int id, int mappos, neobox_checkfun *check, int redraw)
{
    struct neobox_save_button save;
    
    save.check = check;
    
    neobox_type_help_action_range(NEOBOX_LAYOUT_TYPE_CHAR,
        NEOBOX_LAYOUT_TYPE_SYSTEM, id, mappos, &save, redraw,
        neobox_type_button_set_check);
}

void neobox_type_button_copy_set(int size, unsigned char *copy, char *name, struct neobox_save *save)
{
    button_copy_size_backup = button_copy_size;
    button_copy_backup = button_copy;
    button_copy_size = size;
    button_copy = copy;
    button_copy_save.name = name;
    
    if(save->partner)
        save->partner->data = &button_copy_save;
    else
        save->data = &button_copy_save;
}

void neobox_type_button_copy_restore(int *size, unsigned char **copy, void *data, struct neobox_save *save)
{
    *size = button_copy_size;
    *copy = button_copy;
    button_copy_size = button_copy_size_backup;
    button_copy = button_copy_backup;
    
    if(save->partner)
        save->partner->data = data;
    else
        save->data = data;
}
