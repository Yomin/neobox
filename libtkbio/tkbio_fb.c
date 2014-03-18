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
#include "tkbio_layout.h"
#include <string.h>

extern struct tkbio_global tkbio;

void tkbio_color32to16(unsigned char* dst, const unsigned char* src)
{
    dst[0] = (src[0]>>0 & ~7)  | (src[1]>>5 & 7);
    dst[1] = (src[1]<<3 & ~31) | (src[2]>>3 & 31);
}

void tkbio_get_sizes(const struct tkbio_map *map, int *height, int *width, int *fb_height, int *fb_width, int *screen_height, int *screen_width)
{
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        if(height)        *height = tkbio.fb.vinfo.xres/(map->height);
        if(width)         *width  = tkbio.fb.vinfo.yres/(map->width);
        if(fb_height)     *fb_height = tkbio.fb.vinfo.yres/(map->width);
        if(fb_width)      *fb_width  = tkbio.fb.vinfo.xres/(map->height);
        if(screen_height) *screen_height = SCREENMAX/(map->width);
        if(screen_width)  *screen_width  = SCREENMAX/(map->height);
    }
    else
    {
        if(height)        *height = tkbio.fb.vinfo.yres/(map->height);
        if(width)         *width  = tkbio.fb.vinfo.xres/(map->width);
        if(fb_height)     *fb_height = tkbio.fb.vinfo.yres/(map->height);
        if(fb_width)      *fb_width  = tkbio.fb.vinfo.xres/(map->width);
        if(screen_height) *screen_height = SCREENMAX/(map->height);
        if(screen_width)  *screen_width  = SCREENMAX/(map->width);
    }
}

void tkbio_get_sizes_current(int *height, int *width, int *fb_height, int *fb_width, int *screen_height, int *screen_width)
{
    tkbio_get_sizes(&tkbio.layout.maps[tkbio.parser.map],
        height, width, fb_height, fb_width, screen_height, screen_width);
}

void tkbio_layout_to_fb_sizes(int *height, int *width)
{
    int tmp;
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        tmp = *height;
        *height = *width;
        *width = tmp;
    }
}

void tkbio_fb_to_layout_sizes(int *height, int *width)
{
    int tmp;
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        tmp = *height;
        *height = *width;
        *width = tmp;
    }
}

void tkbio_layout_to_fb_cords(int *cord_y, int *cord_x)
{
    int tmp;
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        tmp = *cord_y;
        *cord_y  = *cord_x;
        *cord_x  = tkbio.layout.maps[tkbio.parser.map].height-tmp-1;
    }
}

void tkbio_fb_to_layout_cords(int *cord_y, int *cord_x)
{
    int tmp;
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        tmp = *cord_y;
        *cord_y  = tkbio.layout.maps[tkbio.parser.map].height-*cord_x-1;
        *cord_x  = tmp;
    }
}

void tkbio_layout_to_fb_pos_width(int *pos_y, int *pos_x, int width)
{
    int tmp;
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        tmp = *pos_y;
        *pos_y = *pos_x;
        *pos_x = width - tmp;
    }
}

void tkbio_fb_to_layout_pos_width(int *pos_y, int *pos_x, int width)
{
    int tmp;
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        tmp = *pos_y;
        *pos_y = width - *pos_x;
        *pos_x = tmp;
    }
}

void tkbio_layout_to_fb_pos(int *pos_y, int *pos_x)
{
    tkbio_layout_to_fb_pos_width(pos_y, pos_x, tkbio.fb.vinfo.xres);
}

void tkbio_fb_to_layout_pos(int *pos_y, int *pos_x)
{
    tkbio_fb_to_layout_pos_width(pos_y, pos_x, tkbio.fb.vinfo.xres);
}

void tkbio_layout_to_fb_pos_rel(int *pos_y, int *pos_x)
{
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    int width = tkbio.fb.vinfo.xres/map->height;
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        tkbio_layout_to_fb_pos(pos_y, pos_x);
        *pos_x -= width;
        // fixme: this works only for points at button corners
    }
}

void tkbio_fb_to_layout_pos_rel(int *pos_y, int *pos_x)
{
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    int height = tkbio.fb.vinfo.xres/map->height;
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        tkbio_fb_to_layout_pos(pos_y, pos_x);
        *pos_y -= height;
        // fixme: this works only for points at button corners
    }
}

unsigned char tkbio_layout_connect_to_borders(int cord_y, int cord_x, unsigned char connect)
{
    tkbio_layout_to_fb_cords(&cord_y, &cord_x);
    return tkbio_fb_connect_to_borders(cord_y, cord_x, connect);
}

unsigned char tkbio_fb_connect_to_borders(int cord_y, int cord_x, unsigned char connect)
{
    unsigned char borders = TKBIO_BORDER_ALL;
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        if(connect & TKBIO_LAYOUT_CONNECT_LEFT)
            borders &= ~TKBIO_BORDER_TOP;
        if(connect & TKBIO_LAYOUT_CONNECT_UP)
            borders &= ~TKBIO_BORDER_RIGHT;
        if(cord_y < map->width-1 &&
            map->map[(map->height-cord_x-1)*map->width+cord_y+1]
            .connect & TKBIO_LAYOUT_CONNECT_LEFT)
        {
            borders &= ~TKBIO_BORDER_BOTTOM;
        }
        if(cord_x > 0 &&
            map->map[(map->height-cord_x)*map->width+cord_y]
            .connect & TKBIO_LAYOUT_CONNECT_UP)
        {
            borders &= ~TKBIO_BORDER_LEFT;
        }
    }
    else
    {
        if(connect & TKBIO_LAYOUT_CONNECT_LEFT)
            borders &= ~TKBIO_BORDER_LEFT;
        if(connect & TKBIO_LAYOUT_CONNECT_UP)
            borders &= ~TKBIO_BORDER_TOP;
        if(cord_x < map->width-1 &&
            map->map[cord_y*map->width+cord_x+1]
            .connect & TKBIO_LAYOUT_CONNECT_LEFT)
        {
            borders &= ~TKBIO_BORDER_RIGHT;
        }
        if(cord_y < map->height-1 &&
            map->map[(cord_y+1)*map->width+cord_x]
            .connect & TKBIO_LAYOUT_CONNECT_UP)
        {
            borders &= ~TKBIO_BORDER_BOTTOM;
        }
    }
    
    return borders;
}

void tkbio_layout_draw_rect(int pos_y, int pos_x, int height, int width, int color, int density, unsigned char **copy)
{
    tkbio_layout_to_fb_pos_rel(&pos_y, &pos_x);
    tkbio_layout_to_fb_sizes(&height, &width);
    tkbio_draw_rect(pos_y, pos_x, height, width, color, density, copy);
}

void tkbio_draw_rect(int pos_y, int pos_x, int height, int width, int color, int density, unsigned char **copy)
{
    unsigned char *base = tkbio.fb.ptr + pos_y*tkbio.fb.finfo.line_length + pos_x*tkbio.fb.bpp;
    unsigned char *ptr = base, *cpy = copy ? *copy : 0;
    unsigned char col[4];
    int i, j, k;
    
    if(tkbio.fb.bpp == 2)
        tkbio_color32to16(col, tkbio.layout.maps[tkbio.parser.map].colors[color]);
    else
        memcpy(col, tkbio.layout.maps[tkbio.parser.map].colors[color], 4);
    
    for(i=0; i<height; i+=density, ptr = base)
    {
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            for(k=tkbio.fb.bpp-1; k>=0; k--)
            {
                if(cpy)
                    *(cpy++) = *ptr;
                *(ptr++) = col[k];
            }
        }
        base += density*tkbio.fb.finfo.line_length;
    }
    
    if(cpy)
        *copy = cpy;
}

void tkbio_layout_draw_border(int pos_y, int pos_x, int height, int width, int color, unsigned char borders, int density, unsigned char **copy)
{
    tkbio_layout_to_fb_pos_rel(&pos_y, &pos_x);
    tkbio_layout_to_fb_sizes(&height, &width);
    tkbio_draw_border(pos_y, pos_x, height, width, color, borders, density, copy);
}

void tkbio_layout_draw_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, int color, unsigned char connect, int density, unsigned char **copy)
{
    unsigned char borders;
    
    tkbio_layout_to_fb_pos_rel(&pos_y, &pos_x);
    tkbio_layout_to_fb_sizes(&height, &width);
    borders = tkbio_layout_connect_to_borders(cord_y, cord_x, connect);
    tkbio_draw_border(pos_y, pos_x, height, width, color, borders, density, copy);
}

void tkbio_draw_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, int color, unsigned char connect, int density, unsigned char **copy)
{
    unsigned char borders;
    
    borders = tkbio_fb_connect_to_borders(cord_y, cord_x, connect);
    tkbio_draw_border(pos_y, pos_x, height, width, color, borders, density, copy);
}

void tkbio_draw_border(int pos_y, int pos_x, int height, int width, int color, unsigned char borders, int density, unsigned char **copy)
{
    unsigned char *base = tkbio.fb.ptr + pos_y*tkbio.fb.finfo.line_length + pos_x*tkbio.fb.bpp;
    unsigned char *ptr = base, *base2 = base, *cpy = copy ? *copy : 0;
    unsigned char col[4];
    int i, j, k, skipped;
    
    if(tkbio.fb.bpp == 2)
        tkbio_color32to16(col, tkbio.layout.maps[tkbio.parser.map].colors[color]);
    else
        memcpy(col, tkbio.layout.maps[tkbio.parser.map].colors[color], 4);
    
    for(i=0, skipped=0; i<(borders&TKBIO_BORDER_BOTTOM?2:1); i++, ptr = base2)
    {
        if(!skipped && !(borders & TKBIO_BORDER_TOP))
            skipped = 1;
        else
            for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
            {
                for(k=tkbio.fb.bpp-1; k>=0; k--)
                {
                    if(cpy)
                        *(cpy++) = *ptr;
                    *(ptr++) = col[k];
                }
            }
        base2 += (height-1)*tkbio.fb.finfo.line_length;
    }
    base2 = base;
    ptr   = base;
    for(i=0; i<height; i++, ptr = base2)
    {
        for(j=0, skipped=0; j<(borders&TKBIO_BORDER_RIGHT?2:1); j++, ptr+=tkbio.fb.bpp*(width-2))
        {
            if(!skipped && !(borders & TKBIO_BORDER_LEFT))
            {
                skipped = 1;
                continue;
            }
            for(k=tkbio.fb.bpp-1; k>=0; k--)
            {
                if(cpy)
                    *(cpy++) = *ptr;
                *(ptr++) = col[k];
            }
        }
        base2 += density*tkbio.fb.finfo.line_length;
    }
    
    if(cpy)
        *copy = cpy;
}

void tkbio_layout_draw_rect_border(int pos_y, int pos_x, int height, int width, int color, unsigned char borders, int density, unsigned char **copy)
{
    tkbio_layout_to_fb_pos_rel(&pos_y, &pos_x);
    tkbio_layout_to_fb_sizes(&height, &width);
    tkbio_draw_rect_border(pos_y, pos_x, height, width, color, borders, density, copy);
}

void tkbio_layout_draw_rect_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, int color, unsigned char connect, int density, unsigned char **copy)
{
    unsigned char borders;
    
    tkbio_layout_to_fb_pos_rel(&pos_y, &pos_x);
    tkbio_layout_to_fb_sizes(&height, &width);
    borders = tkbio_layout_connect_to_borders(cord_y, cord_x, connect);
    tkbio_draw_rect_border(pos_y, pos_x, height, width, color, borders, density, copy);
}

void tkbio_draw_rect_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, int color, unsigned char connect, int density, unsigned char **copy)
{
    unsigned char borders;
    
    borders = tkbio_fb_connect_to_borders(cord_y, cord_x, connect);
    tkbio_draw_rect_border(pos_y, pos_x, height, width, color, borders, density, copy);
}

void tkbio_draw_rect_border(int pos_y, int pos_x, int height, int width, int color, unsigned char borders, int density, unsigned char **copy)
{
    unsigned char *base = tkbio.fb.ptr + pos_y*tkbio.fb.finfo.line_length + pos_x*tkbio.fb.bpp;
    unsigned char *ptr = base, *cpy = copy ? *copy : 0;
    unsigned char fg[4], bg[4], *col = bg;
    int i, j, k, cset;
    
    if(tkbio.fb.bpp == 2)
    {
        tkbio_color32to16(fg, tkbio.layout.maps[tkbio.parser.map].colors[color>>4]);
        tkbio_color32to16(bg, tkbio.layout.maps[tkbio.parser.map].colors[color&15]);
    }
    else
    {
        memcpy(fg, tkbio.layout.maps[tkbio.parser.map].colors[color>>4], 4);
        memcpy(bg, tkbio.layout.maps[tkbio.parser.map].colors[color&15], 4);
    }
    
    for(i=0; i<height; i+=density, ptr = base, col = bg)
    {
        if(!i && borders & TKBIO_BORDER_TOP)
            col = fg;
        if(i == height-1 && borders & TKBIO_BORDER_BOTTOM)
            col = fg;
        
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            if(col == fg)
                cset = 1;
            else
            {
                cset = 0;
                if(!j && borders & TKBIO_BORDER_LEFT)
                    col = fg;
                if(j == width-1 && borders & TKBIO_BORDER_RIGHT)
                    col = fg;
            }
            
            for(k=tkbio.fb.bpp-1; k>=0; k--)
            {
                if(cpy)
                    *(cpy++) = *ptr;
                *(ptr++) = col[k];
            }
            
            if(!cset)
                col = bg;
        }
        base += density*tkbio.fb.finfo.line_length;
    }
    
    if(cpy)
        *copy = cpy;
}

void tkbio_layout_fill_rect(int pos_y, int pos_x, int height, int width, int density, unsigned char **fill)
{
    tkbio_layout_to_fb_pos_rel(&pos_y, &pos_x);
    tkbio_layout_to_fb_sizes(&height, &width);
    tkbio_fill_rect(pos_y, pos_x, height, width, density, fill);
}

void tkbio_fill_rect(int pos_y, int pos_x, int height, int width, int density, unsigned char **fill)
{
    unsigned char *base = tkbio.fb.ptr + pos_y*tkbio.fb.finfo.line_length + pos_x*tkbio.fb.bpp;
    unsigned char *ptr = base, *fll = *fill;
    int i, j, k;
    
    for(i=0; i<height; i+=density, ptr = base)
    {
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            for(k=tkbio.fb.bpp-1; k>=0; k--)
            {
                *(ptr++) = *fll;
                fll++;
            }
        }
        base += density*tkbio.fb.finfo.line_length;
    }
    
    *fill = fll;
}

void tkbio_layout_fill_border(int pos_y, int pos_x, int height, int width, unsigned char borders, int density, unsigned char **fill)
{
    tkbio_layout_to_fb_pos_rel(&pos_y, &pos_x);
    tkbio_layout_to_fb_sizes(&height, &width);
    tkbio_fill_border(pos_y, pos_x, height, width, borders, density, fill);
}

void tkbio_layout_fill_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char connect, int density, unsigned char **fill)
{
    unsigned char borders;
    
    tkbio_layout_to_fb_pos_rel(&pos_y, &pos_x);
    tkbio_layout_to_fb_sizes(&height, &width);
    borders = tkbio_layout_connect_to_borders(cord_y, cord_x, connect);
    tkbio_fill_border(pos_y, pos_x, height, width, borders, density, fill);
}

void tkbio_fill_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char connect, int density, unsigned char **fill)
{
    unsigned char borders;
    
    borders = tkbio_fb_connect_to_borders(cord_y, cord_x, connect);
    tkbio_fill_border(pos_y, pos_x, height, width, borders, density, fill);
}

void tkbio_fill_border(int pos_y, int pos_x, int height, int width, unsigned char borders, int density, unsigned char **fill)
{
    unsigned char *base = tkbio.fb.ptr + pos_y*tkbio.fb.finfo.line_length + pos_x*tkbio.fb.bpp;
    unsigned char *ptr = base, *base2 = base, *fll = *fill;
    int i, j, k, skipped;
    
    for(i=0, skipped=0; i<(borders&TKBIO_BORDER_BOTTOM?2:1); i++, ptr = base2)
    {
        if(!skipped && !(borders & TKBIO_BORDER_TOP))
            skipped = 1;
        else
            for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
            {
                for(k=tkbio.fb.bpp-1; k>=0; k--)
                {
                    *(ptr++) = *fll;
                    fll++;
                }
            }
        base2 += (height-1)*tkbio.fb.finfo.line_length;
    }
    base2 = base;
    ptr   = base;
    for(i=0; i<height; i++, ptr = base2)
    {
        for(j=0, skipped=0; j<(borders&TKBIO_BORDER_RIGHT?0:1); j++, ptr+=tkbio.fb.bpp*(width-2))
        {
            if(!skipped && !(borders & TKBIO_BORDER_LEFT))
            {
                skipped = 1;
                continue;
            }
            for(k=tkbio.fb.bpp-1; k>=0; k--)
            {
                *(ptr++) = *fll;
                fll++;
            }
        }
        base2 += density*tkbio.fb.finfo.line_length;
    }
    
    *fill = fll;
}

