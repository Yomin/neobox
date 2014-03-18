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

#include "tkbio.h"
#include "tkbio_fb.h"
#include "tkbio_type_slider.h"

#define COPY(e)    ((e)->type & TKBIO_LAYOUT_OPTION_COPY)
#define BORDER(e)  ((e)->type & TKBIO_LAYOUT_OPTION_BORDER)
#define LANDSCAPE  (tkbio.format == TKBIO_FORMAT_LANDSCAPE)
#define HSLIDER(e) (((e)->type & TKBIO_LAYOUT_MASK_TYPE) == TKBIO_LAYOUT_TYPE_HSLIDER)

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

void tkbio_type_slider_press(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    tkbio_type_slider_move(y, x, button_y, button_x, map, elem, save);
}

void tkbio_type_slider_move(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    int i, height, width, offset_l, offset_p;
    unsigned char *ptr;
    struct vector *connect;
    struct tkbio_point *p;
    struct tkbio_save_slider *slider;
    
    tkbio_get_sizes(map, &height, &width, 0, 0, 0, 0);
    
    if(elem->type & TKBIO_LAYOUT_OPTION_COPY)
        alloc_copy(height, width, save);
    
    if(!save->partner)
    {
        slider = save->data;
        ptr = slider->copy;
        if(HSLIDER(elem))
        {
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
            offset_l = LANDSCAPE ? height-button_y : 0;
            offset_p = !LANDSCAPE ? button_y : 0;
            
            tkbio_layout_draw_rect(y*height+offset_p, x*width,
                height-button_y, width, elem->color >> 4, DENSITY, &ptr);
            if(BORDER(elem))
                tkbio_layout_draw_rect_connect(y*height-offset_l,
                    x*width, y, x, button_y, width,
                    elem->color, elem->connect, DENSITY, &ptr);
            else
                tkbio_layout_draw_rect(y*height-offset_l, x*width,
                    button_y, width, elem->color & 15, DENSITY, &ptr);
        }
    }
    else
    {
        slider = save->partner->data;
        ptr = slider->copy;
        connect = save->partner->connect;
        offset_l = LANDSCAPE ? height-button_y : 0;
        offset_p = !LANDSCAPE ? button_y : 0;
        
        for(i=0; i<vector_size(connect); i++)
        {
            p = vector_at(i, connect);
            if( ( HSLIDER(p->elem) && p->x < x) ||
                (!HSLIDER(p->elem) && p->y > y))
            {
                tkbio_layout_draw_rect(p->y*height, p->x*width,
                    height, width, p->elem->color >> 4,
                    DENSITY, &ptr);
            }
            else if(HSLIDER(p->elem) && p->x == x)
            {
                tkbio_layout_draw_rect(p->y*height, p->x*width,
                    height, button_x, p->elem->color >> 4,
                    DENSITY, &ptr);
                if(BORDER(elem))
                    tkbio_layout_draw_rect_connect(p->y*height,
                        p->x*width+button_x, p->y, p->x,
                        height, width-button_x, p->elem->color,
                        p->elem->connect, DENSITY, &ptr);
                else
                    tkbio_layout_draw_rect(p->y*height,
                        p->x*width+button_x, height,
                        width-button_x, p->elem->color & 15,
                        DENSITY, &ptr);
            }
            else if(!HSLIDER(p->elem) && p->y == y)
            {
                tkbio_layout_draw_rect(p->y*height+offset_p,
                    p->x*width, height-button_y, width,
                    p->elem->color >> 4, DENSITY, &ptr);
                if(BORDER(elem))
                    tkbio_layout_draw_rect_connect(p->y*height-offset_l,
                        p->x*width, p->y, p->x,
                        button_y, width, p->elem->color,
                        p->elem->connect, DENSITY, &ptr);
                else
                    tkbio_layout_draw_rect(p->y*height-offset_l,
                        p->x*width, button_y, width,
                        p->elem->color & 15, DENSITY, &ptr);
            }
            else
            {
                if(BORDER(elem))
                    tkbio_layout_draw_rect_connect(p->y*height,
                        p->x*width, p->y, p->x, height, width,
                        p->elem->color, p->elem->connect,
                        DENSITY, &ptr);
                else
                    tkbio_layout_draw_rect(p->y*height, p->x*width,
                        height, width, p->elem->color & 15,
                        DENSITY, &ptr);
            }
        }
    }
}

struct tkbio_return tkbio_type_slider_release(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_return ret;
    struct tkbio_save_slider *slider;
    int height, width;
    
    tkbio_get_sizes(map, &height, &width, 0, 0, 0, 0);
    
    ret.type = TKBIO_RETURN_INT;
    ret.id = elem->id;
    
    if(HSLIDER(elem))
    {
        if(save->partner)
        {
            slider = save->partner->data;
            ret.value.i = (x*width-slider->start+button_x)*100;
            ret.value.i /= slider->size;
        }
        else
        {
            slider = save->data;
            ret.value.i = (button_x*100)/width;
        }
        slider->y = y*height+button_y;
        slider->x = x*width+button_x;
        VERBOSE(printf("hslider %i: %02i%%", ret.id, ret.value.i));
    }
    else
    {
        if(save->partner)
        {
            slider = save->partner->data;
            ret.value.i = (slider->start-(y*height)-button_y)*100;
            ret.value.i /= slider->size;
        }
        else
        {
            slider = save->data;
            ret.value.i = ((height-button_y)*100)/height;
        }
        slider->y = y*height+button_y;
        slider->x = x*width+button_x;
        VERBOSE(printf("vslider %i: %02i%%", ret.id, ret.value.i));
    }
    
    return ret;
}

void tkbio_type_slider_focus_in(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    tkbio_type_slider_move(y, x, button_y, button_x, map, elem, save);
}

void tkbio_type_slider_focus_out(const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_slider *slider;
    int height, width;
    
    tkbio_get_sizes(map, &height, &width, 0, 0, 0, 0);
    slider = save->partner ? save->partner->data : save->data;
    
    tkbio_type_slider_move(slider->y/height, slider->x/width,
        slider->y%height, slider->x%width, map, elem, save);
}
