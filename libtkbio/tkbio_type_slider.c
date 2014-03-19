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

#include "tkbio.h"
#include "tkbio_fb.h"
#include "tkbio_slider.h"
#include "tkbio_type_slider.h"

#define COPY(e)       ((e)->type & TKBIO_LAYOUT_OPTION_COPY)
#define BORDER(e)     ((e)->type & TKBIO_LAYOUT_OPTION_BORDER)
#define LANDSCAPE     (tkbio.format == TKBIO_FORMAT_LANDSCAPE)
#define TSLIDER(e, t) (((e)->type & TKBIO_LAYOUT_MASK_TYPE) == t)
#define HSLIDER(e)    TSLIDER(e, TKBIO_LAYOUT_TYPE_HSLIDER)
#define SLIDER(e)     (TSLIDER(e, TKBIO_LAYOUT_TYPE_HSLIDER) || TSLIDER(e, TKBIO_LAYOUT_TYPE_VSLIDER))
#define NOP           (struct tkbio_return) { .type = TKBIO_RETURN_NOP }

extern struct tkbio_global tkbio;

static void alloc_copy(int height, int width, struct tkbio_save *save)
{
    struct tkbio_save_slider *slider = save->data;
    int size = (width/DENSITY) * (height/DENSITY) * tkbio.fb.bpp;
    
    if(save->partner)
    {
        size *= vector_size(save->partner->connect);
        slider = save->partner->data;
    }
    
    if(!slider->copy)
        slider->copy = malloc(size);
}

void tkbio_type_slider_init(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_slider *slider;
    struct tkbio_point *p;
    struct vector *connect;
    int i, height, width, h_y, v_y, h_x, v_x, h_max, v_max;
    
    tkbio_get_sizes(map, &height, &width, 0, 0, 0, 0);
    
    if(!save->partner)
    {
        slider = save->data = malloc(sizeof(struct tkbio_save_slider));
        slider->y = HSLIDER(elem) ? y*height : (y+1)*height-1;
        slider->x = x*width;
        slider->start = HSLIDER(elem) ? slider->x : slider->y;
        slider->size = HSLIDER(elem) ? width : height;
    }
    else
    {
        if(save->partner->data)
            return;
        
        slider = save->partner->data =
            malloc(sizeof(struct tkbio_save_slider));
        connect = save->partner->connect;
        
        h_x = v_x = map->width-1;
        h_y = v_max = map->height-1;
        v_y = h_max = 0;
        
        for(i=0; i<vector_size(connect); i++)
        {
            p = vector_at(i, connect);
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
        if(HSLIDER(elem))
        {
            slider->y = h_y*height;
            slider->x = slider->start = h_x*width;
            slider->size = (h_max-h_x+1)*width;
        }
        else
        {
            slider->y = slider->start = (v_y+1)*height-1;
            slider->x = v_x*width;
            slider->size = (v_y-v_max+1)*height;
        }
    }
    
    slider->copy = 0;
    slider->ticks = elem->elem.i;
    slider->pos = 0;
}

void tkbio_type_slider_finish(struct tkbio_save *save)
{
    struct tkbio_save_slider *slider;
    
    if(save->data)
    {
        slider = save->data;
        if(slider->copy)
            free(slider->copy);
        free(slider);
    }
    
    if(save->partner && save->partner->data)
    {
        slider = save->partner->data;
        if(slider->copy)
            free(slider->copy);
        free(slider);
    }
}

int tkbio_type_slider_broader(int *y, int *x, int scr_y, int scr_x, const struct tkbio_mapelem *elem)
{
    int scr_height, scr_width, fb_y, fb_x;
    
    tkbio_get_sizes_current(0, 0, 0, 0, &scr_height, &scr_width);
    
    fb_y = tkbio.parser.y;
    fb_x = tkbio.parser.x;
    tkbio_layout_to_fb_cords(&fb_y, &fb_x);
    
    if((LANDSCAPE && !HSLIDER(elem)) || (!LANDSCAPE && HSLIDER(elem)))
    {
        if(    (SCREENMAX - scr_y) >= scr_height*fb_y - scr_height*(INCREASE/100.0)
            && (SCREENMAX - scr_y) <= scr_height*(fb_y+1) + scr_height*(INCREASE/100.0))
        {
            *x = scr_x / scr_width;
            *y = fb_y;
            tkbio_fb_to_layout_cords(y, x);
            return 0;
        }
    }
    else if((LANDSCAPE && HSLIDER(elem)) || (!LANDSCAPE && !HSLIDER(elem)))
    {
        if(    scr_x >= scr_width*fb_x - scr_width*(INCREASE/100.0)
            && scr_x <= scr_width*(fb_x+1) + scr_width*(INCREASE/100.0))
        {
            *x = fb_x;
            *y = (SCREENMAX - scr_y) / scr_height;
            tkbio_fb_to_layout_cords(y, x);
            return 0;
        }
    }
    return 1;
}

struct tkbio_return tkbio_type_slider_press(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    return tkbio_type_slider_move(y, x, button_y, button_x, map, elem, save);
}

struct tkbio_return tkbio_type_slider_move(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    int i, height, width, pos, partner_x, partner_y, hticks;
    float tick;
    unsigned char *ptr;
    struct vector *connect;
    struct tkbio_point *p;
    struct tkbio_save_slider *slider;
    struct tkbio_return ret;
    
    tkbio_get_sizes(map, &height, &width, 0, 0, 0, 0);
    
    if(elem->type & TKBIO_LAYOUT_OPTION_COPY)
        alloc_copy(height, width, save);
    
    if(!save->partner)
    {
        slider = save->data;
        ptr = slider->copy;
        
        if(HSLIDER(elem))
        {
            if(slider->ticks)
            {
                tick = width/(slider->ticks*1.0);
                hticks = button_x/(tick/2);
                
                if(slider->pos_tmp == (hticks+1)/2)
                    return NOP;
                
                slider->pos_tmp = (hticks+1)/2;
                button_x = slider->pos_tmp*tick;
            }
            
            tkbio_layout_draw_rect(y*height, x*width, height, button_x,
                elem->color >> 4, DENSITY, &ptr);
            if(BORDER(elem))
                tkbio_layout_draw_rect_connect(y*height,
                    x*width+button_x, y, x, height, width-button_x,
                    elem->color, elem->connect, DENSITY, &ptr);
            else
                tkbio_layout_draw_rect(y*height, x*width+button_x,
                    height, width-button_x, elem->color & 15,
                    DENSITY, &ptr);
        }
        else
        {
            if(slider->ticks)
            {
                tick = height/(slider->ticks*1.0);
                hticks = (height-button_y)/(tick/2);
                
                if(slider->pos_tmp == (hticks+1)/2)
                    return NOP;
                
                slider->pos_tmp = (hticks+1)/2;
                button_y = height-slider->pos_tmp*tick;
            }
            
            tkbio_layout_draw_rect(y*height+button_y, x*width,
                height-button_y, width, elem->color >> 4, DENSITY, &ptr);
            if(BORDER(elem))
                tkbio_layout_draw_rect_connect(y*height, x*width,
                    y, x, button_y, width, elem->color,
                    elem->connect, DENSITY, &ptr);
            else
                tkbio_layout_draw_rect(y*height, x*width, button_y,
                    width, elem->color & 15, DENSITY, &ptr);
        }
        slider->y_tmp = y*height+button_y;
        slider->x_tmp = x*width+button_x;
    }
    else
    {
        slider = save->partner->data;
        ptr = slider->copy;
        connect = save->partner->connect;
        
        if(HSLIDER(elem))
        {
            pos = x*width+button_x;
            if(slider->ticks)
            {
                tick = slider->size/(slider->ticks*1.0);
                hticks = (pos-slider->start)/(tick/2);
                
                if(slider->pos_tmp == (hticks+1)/2)
                    return NOP;
                
                slider->pos_tmp = (hticks+1)/2;
                pos = slider->pos_tmp*tick+slider->start;
            }
            slider->y_tmp = y+height+button_y;
            slider->x_tmp = pos;
        }
        else
        {
            pos = y*height+button_y;
            if(slider->ticks)
            {
                tick = slider->size/(slider->ticks*1.0);
                hticks = (slider->start-pos)/(tick/2);
                
                if(slider->pos_tmp == (hticks+1)/2)
                    return NOP;
                
                slider->pos_tmp = (hticks+1)/2;
                pos = slider->start-slider->pos_tmp*tick;
            }
            slider->y_tmp = pos;
            slider->x_tmp = x*width+button_x;
        }
        
        for(i=0; i<vector_size(connect); i++)
        {
            p = vector_at(i, connect);
            partner_y = p->y*height;
            partner_x = p->x*width;
            
            if( ( HSLIDER(elem) && partner_x+width  < pos) ||
                (!HSLIDER(elem) && partner_y        > pos))
            {
                tkbio_layout_draw_rect(partner_y, partner_x,
                    height, width, p->elem->color >> 4,
                    DENSITY, &ptr);
            }
            else if(HSLIDER(elem) && partner_x < pos)
            {
                tkbio_layout_draw_rect(partner_y, partner_x,
                    height, pos-partner_x, p->elem->color >> 4,
                    DENSITY, &ptr);
                if(BORDER(elem))
                    tkbio_layout_draw_rect_connect(partner_y,
                        pos, p->y, p->x, height,
                        width-(pos-partner_x), p->elem->color,
                        p->elem->connect, DENSITY, &ptr);
                else
                    tkbio_layout_draw_rect(partner_y,
                        pos, height, width-(pos-partner_x),
                        p->elem->color & 15, DENSITY, &ptr);
            }
            else if(!HSLIDER(p->elem) && partner_y+height > pos)
            {
                tkbio_layout_draw_rect(pos, partner_x,
                    height-(pos-partner_y), width,
                    p->elem->color >> 4, DENSITY, &ptr);
                if(BORDER(elem))
                    tkbio_layout_draw_rect_connect(partner_y,
                        partner_x, p->y, p->x, pos-partner_y,
                        width, p->elem->color,
                        p->elem->connect, DENSITY, &ptr);
                else
                    tkbio_layout_draw_rect(partner_y, partner_x,
                        pos-partner_y, width, p->elem->color & 15,
                        DENSITY, &ptr);
            }
            else
            {
                if(BORDER(elem))
                    tkbio_layout_draw_rect_connect(partner_y, partner_x,
                        p->y, p->x, height, width, p->elem->color,
                        p->elem->connect, DENSITY, &ptr);
                else
                    tkbio_layout_draw_rect(partner_y, partner_x,
                        height, width, p->elem->color & 15,
                        DENSITY, &ptr);
            }
        }
    }
    
    ret.type = TKBIO_RETURN_INT;
    ret.id = elem->id;
    
    if(slider->ticks)
    {
        ret.value.i = slider->pos_tmp;
        VERBOSE(printf("[TKBIO] slider %i: %i/%i\n",
            ret.id, ret.value.i, slider->ticks));
    }
    else
    {
        if(HSLIDER(elem))
        {
            ret.value.i = (slider->x_tmp-slider->start)*100;
            ret.value.i /= slider->size;
        }
        else
        {
            ret.value.i = (slider->start-slider->y_tmp)*100;
            ret.value.i /= slider->size;
        }
        VERBOSE(printf("[TKBIO] slider %i: %02i%%\n",
            ret.id, ret.value.i));
    }
    
    return ret;
}

struct tkbio_return tkbio_type_slider_release(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_slider *slider;
    
    slider = save->partner ? save->partner->data : save->data;
    
    slider->y = slider->y_tmp;
    slider->x = slider->x_tmp;
    slider->pos = slider->pos_tmp;
    
    return NOP;
}

struct tkbio_return tkbio_type_slider_focus_in(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    return tkbio_type_slider_move(y, x, button_y, button_x, map, elem, save);
}

struct tkbio_return tkbio_type_slider_focus_out(const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_slider *slider;
    int height, width;
    
    tkbio_get_sizes(map, &height, &width, 0, 0, 0, 0);
    slider = save->partner ? save->partner->data : save->data;
    
    return tkbio_type_slider_move(slider->y/height, slider->x/width,
        slider->y%height, slider->x%width, map, elem, save);
}

void tkbio_type_slider_set_ticks(int ticks, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_slider *slider;
    
    if(ticks < 0)
        return;
    
    slider = save->partner ? save->partner->data : save->data;
    slider->ticks = ticks;
    slider->pos = 0;
    if(HSLIDER(elem))
        slider->x = slider->start;
    else
        slider->y = slider->start;
    
    if(map)
        tkbio_type_slider_focus_out(map, elem, save);
}

void tkbio_type_slider_set_pos(int pos, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_slider *slider;
    
    slider = save->partner ? save->partner->data : save->data;
    
    if( ( slider->ticks && (pos < 0 || pos > slider->ticks)) ||
        (!slider->ticks && (pos < 0 || pos > 100)))
        return;
    
    if(slider->ticks)
    {
        slider->pos = pos;
        if(HSLIDER(elem))
            slider->x = slider->start+pos*(slider->size/slider->ticks);
        else
            slider->y = slider->start-pos*(slider->size/slider->ticks);
    }
    else
    {
        if(HSLIDER(elem))
            slider->x = slider->start+(slider->size*pos)/100;
        else
            slider->y = slider->start-(slider->size*pos)/100;
    }
    
    if(map)
        tkbio_type_slider_focus_out(map, elem, save);
}

static int find_slider(int mappos, int id)
{
    const struct tkbio_map *map = &tkbio.layout.maps[mappos];
    const struct tkbio_mapelem *elem;
    int size = map->height*map->width;
    int i;
    
    for(i=0; i<size; i++)
    {
        elem = &map->map[i];
        if(SLIDER(elem) && elem->id == id)
            return i;
    }
    
    return -1;
}

typedef void setfunc(int value, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);

static void set_value(int mappos, int id, int value, int redraw, setfunc f)
{
    const struct tkbio_map *map = &tkbio.layout.maps[mappos];
    const struct tkbio_mapelem *elem;
    struct tkbio_save *save;
    unsigned char sim_tmp = 'x';
    int i;
    
    if((i = find_slider(mappos, id)) == -1)
        return;
    
    elem = &map->map[i];
    save = &tkbio.save[mappos][i];
    
    // only redraw if map is active
    if(redraw && tkbio.parser.map == mappos)
    {
        f(value, map, elem, save);
        // notify framebuffer for redraw
        if(tkbio.sim)
            send(tkbio.fb.sock, &sim_tmp, 1, 0);
    }
    else
        f(value, 0, elem, save);
}

void tkbio_slider_set_ticks(int mappos, int id, int ticks)
{
    set_value(mappos, id, ticks, 1, tkbio_type_slider_set_ticks);
}

void tkbio_slider_set_pos(int mappos, int id, int pos)
{
    set_value(mappos, id, pos, 1, tkbio_type_slider_set_pos);
}

void tkbio_slider_set_ticks_pos(int mappos, int id, int ticks, int pos)
{
    set_value(mappos, id, ticks, 0, tkbio_type_slider_set_ticks);
    set_value(mappos, id, pos, 1, tkbio_type_slider_set_pos);
}
