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

#include "neobox_type_slider.h"
#include "neobox_type_help.h"
#include "neobox_fb.h"
#include "neobox_slider.h"

#define BORDER(e)  ((e)->options & NEOBOX_LAYOUT_OPTION_BORDER)
#define CONNECT(e) ((e)->options & NEOBOX_LAYOUT_OPTION_MASK_CONNECT)
#define LANDSCAPE  (neobox.format == NEOBOX_FORMAT_LANDSCAPE)
#define HSLIDER(e) ((e)->type == NEOBOX_LAYOUT_TYPE_HSLIDER)
#define NOP        (struct neobox_event) { .type = NEOBOX_EVENT_NOP }
#define COPY       (!(neobox.options & NEOBOX_OPTION_PRINT_MASK) || map->invisible)

extern struct neobox_global neobox;

static int forceprint;

static void alloc_copy(int height, int width, struct neobox_save *save)
{
    struct neobox_save_slider *slider = save->data;
    int size = (width/DENSITY) * (height/DENSITY) * neobox.fb.bpp;
    
    if(save->partner)
    {
        size *= vector_size(save->partner->connect);
        slider = save->partner->data;
    }
    
    if(!slider->copy)
        slider->copy = malloc(size);
}

TYPE_FUNC_INIT(slider)
{
    struct neobox_save_slider *slider;
    struct neobox_point *p;
    struct vector *connect;
    int i, height, width, h_y, v_y, h_x, v_x, h_max, v_max;
    
    neobox_get_sizes(&height, &width, 0, 0, 0, 0, map);
    
    if(!save->partner)
    {
        slider = save->data = malloc(sizeof(struct neobox_save_slider));
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
            malloc(sizeof(struct neobox_save_slider));
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
    
    forceprint = 0;
}

TYPE_FUNC_FINISH(slider)
{
    struct neobox_save_slider *slider;
    
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

TYPE_FUNC_DRAW(slider)
{
    forceprint = 1;
    TYPE_FUNC_FOCUS_OUT_CALL(slider);
    forceprint = 0;
}

TYPE_FUNC_BROADER(slider)
{
    int scr_height, scr_width, fb_y, fb_x;
    
    neobox_get_sizes(0, 0, 0, 0, &scr_height, &scr_width, map);
    
    fb_y = neobox.parser.y;
    fb_x = neobox.parser.x;
    neobox_layout_to_fb_cords(&fb_y, &fb_x, map);
    
    if((LANDSCAPE && !HSLIDER(elem)) || (!LANDSCAPE && HSLIDER(elem)))
    {
        if(    (SCREENMAX - scr_y) >= scr_height*fb_y - scr_height*(INCREASE/100.0)
            && (SCREENMAX - scr_y) <= scr_height*(fb_y+1) + scr_height*(INCREASE/100.0))
        {
            *x = scr_x / scr_width;
            *y = fb_y;
            neobox_fb_to_layout_cords(y, x, map);
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
            neobox_fb_to_layout_cords(y, x, map);
            return 0;
        }
    }
    return 1;
}

TYPE_FUNC_PRESS(slider)
{
    return TYPE_FUNC_MOVE_CALL(slider);
}

TYPE_FUNC_MOVE(slider)
{
    int i, height, width, pos, partner_x, partner_y, hticks;
    float tick;
    unsigned char *ptr;
    struct vector *connect;
    struct neobox_point *p;
    struct neobox_save_slider *slider;
    struct neobox_event ret;
    
    neobox_get_sizes(&height, &width, 0, 0, 0, 0, map);
    
    if(COPY)
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
                
                if(!forceprint && slider->pos_tmp == (hticks+1)/2)
                    return NOP;
                
                slider->pos_tmp = (hticks+1)/2;
                button_x = slider->pos_tmp*tick;
            }
            
            neobox_layout_draw_rect(y*height, x*width, height, button_x,
                elem->color_fg, DENSITY, map, &ptr);
            if(BORDER(elem))
                neobox_layout_draw_rect_connect(y*height,
                    x*width+button_x, y, x, height, width-button_x,
                    elem->color_fg, elem->color_bg,
                    CONNECT(elem), DENSITY, map, &ptr);
            else
                neobox_layout_draw_rect(y*height, x*width+button_x,
                    height, width-button_x, elem->color_bg,
                    DENSITY, map, &ptr);
        }
        else
        {
            if(slider->ticks)
            {
                tick = height/(slider->ticks*1.0);
                hticks = (height-button_y)/(tick/2);
                
                if(!forceprint && slider->pos_tmp == (hticks+1)/2)
                    return NOP;
                
                slider->pos_tmp = (hticks+1)/2;
                button_y = height-slider->pos_tmp*tick;
            }
            
            neobox_layout_draw_rect(y*height+button_y, x*width,
                height-button_y, width, elem->color_fg, DENSITY, map, &ptr);
            if(BORDER(elem))
                neobox_layout_draw_rect_connect(y*height, x*width,
                    y, x, button_y, width, elem->color_fg,
                    elem->color_bg, CONNECT(elem), DENSITY, map, &ptr);
            else
                neobox_layout_draw_rect(y*height, x*width, button_y,
                    width, elem->color_bg, DENSITY, map, &ptr);
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
                
                if(!forceprint && slider->pos_tmp == (hticks+1)/2)
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
                
                if(!forceprint && slider->pos_tmp == (hticks+1)/2)
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
                neobox_layout_draw_rect(partner_y, partner_x,
                    height, width, p->elem->color_fg,
                    DENSITY, map, &ptr);
            }
            else if(HSLIDER(elem) && partner_x < pos)
            {
                neobox_layout_draw_rect(partner_y, partner_x,
                    height, pos-partner_x, p->elem->color_fg,
                    DENSITY, map, &ptr);
                if(BORDER(elem))
                    neobox_layout_draw_rect_connect(partner_y,
                        pos, p->y, p->x, height,
                        width-(pos-partner_x), p->elem->color_fg,
                        p->elem->color_bg, CONNECT(p->elem),
                        DENSITY, map, &ptr);
                else
                    neobox_layout_draw_rect(partner_y,
                        pos, height, width-(pos-partner_x),
                        p->elem->color_bg, DENSITY, map, &ptr);
            }
            else if(!HSLIDER(p->elem) && partner_y+height > pos)
            {
                neobox_layout_draw_rect(pos, partner_x,
                    height-(pos-partner_y), width,
                    p->elem->color_fg, DENSITY, map, &ptr);
                if(BORDER(elem))
                    neobox_layout_draw_rect_connect(partner_y,
                        partner_x, p->y, p->x, pos-partner_y,
                        width, p->elem->color_fg, p->elem->color_bg,
                        CONNECT(p->elem), DENSITY, map, &ptr);
                else
                    neobox_layout_draw_rect(partner_y, partner_x,
                        pos-partner_y, width, p->elem->color_bg,
                        DENSITY, map, &ptr);
            }
            else
            {
                if(BORDER(elem))
                    neobox_layout_draw_rect_connect(partner_y, partner_x,
                        p->y, p->x, height, width, p->elem->color_fg,
                        p->elem->color_bg, CONNECT(p->elem),
                        DENSITY, map, &ptr);
                else
                    neobox_layout_draw_rect(partner_y, partner_x,
                        height, width, p->elem->color_bg,
                        DENSITY, map, &ptr);
            }
        }
    }
    
    ret.type = NEOBOX_EVENT_INT;
    ret.id = elem->id;
    
    if(slider->ticks)
    {
        ret.value.i = slider->pos_tmp;
        neobox_printf(1, "slider %i: %i/%i\n",
            ret.id, ret.value.i, slider->ticks);
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
        neobox_printf(1, "slider %i: %02i%%\n",
            ret.id, ret.value.i);
    }
    
    return ret;
}

TYPE_FUNC_RELEASE(slider)
{
    struct neobox_save_slider *slider;
    
    slider = save->partner ? save->partner->data : save->data;
    
    slider->y = slider->y_tmp;
    slider->x = slider->x_tmp;
    slider->pos = slider->pos_tmp;
    
    return NOP;
}

TYPE_FUNC_FOCUS_IN(slider)
{
    return TYPE_FUNC_MOVE_CALL(slider);
}

TYPE_FUNC_FOCUS_OUT(slider)
{
    struct neobox_save_slider *slider;
    int height, width;
    
    neobox_get_sizes(&height, &width, 0, 0, 0, 0, map);
    slider = save->partner ? save->partner->data : save->data;
    
    return neobox_type_slider_move(slider->y/height, slider->x/width,
        slider->y%height, slider->x%width, map, elem, save);
}

TYPE_FUNC_ACTION(slider, set_ticks)
{
    struct neobox_save_slider *slider;
    int ticks = *(int*)data;
    
    if(ticks < 0)
        return 0;
    
    slider = save->partner ? save->partner->data : save->data;
    slider->ticks = ticks;
    slider->pos = 0;
    if(HSLIDER(elem))
        slider->x = slider->start;
    else
        slider->y = slider->start;
    
    if(map)
        TYPE_FUNC_FOCUS_OUT_CALL(slider);
    
    return 0;
}

TYPE_FUNC_ACTION(slider, set_pos)
{
    struct neobox_save_slider *slider;
    int pos = *(int*)data;
    
    slider = save->partner ? save->partner->data : save->data;
    
    if( ( slider->ticks && (pos < 0 || pos > slider->ticks)) ||
        (!slider->ticks && (pos < 0 || pos > 100)))
        return 0;
    
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
        TYPE_FUNC_FOCUS_OUT_CALL(slider);
    
    return 0;
}

void neobox_slider_set_ticks(int id, int mappos, int ticks, int redraw)
{
    neobox_type_help_action_range(NEOBOX_LAYOUT_TYPE_HSLIDER,
        NEOBOX_LAYOUT_TYPE_VSLIDER, id, mappos, &ticks, redraw,
        neobox_type_slider_set_ticks);
}

void neobox_slider_set_pos(int id, int mappos, int pos, int redraw)
{
    neobox_type_help_action_range(NEOBOX_LAYOUT_TYPE_HSLIDER,
        NEOBOX_LAYOUT_TYPE_VSLIDER, id, mappos, &pos, redraw,
        neobox_type_slider_set_ticks);
}

void neobox_slider_set_ticks_pos(int id, int mappos, int ticks, int pos, int redraw)
{
    int tpos = neobox_type_help_find_type(
        NEOBOX_LAYOUT_TYPE_HSLIDER, NEOBOX_LAYOUT_TYPE_VSLIDER,
        id, mappos);
    
    neobox_type_help_action_pos(tpos, mappos, &ticks, 0,
        neobox_type_slider_set_ticks);
    neobox_type_help_action_pos(tpos, mappos, &pos, redraw,
        neobox_type_slider_set_pos);
}
