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

void tkbio_layout_to_fb_sizes(int *height, int *width, int *scrHeight, int *scrWidth)
{
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        if(scrHeight && scrWidth)
        {
            *scrHeight = SCREENMAX/(*width);
            *scrWidth  = SCREENMAX/(*height);
        }
        int tmp;
        tmp     = *width;
        *width  = tkbio.fb.vinfo.xres/(*height);
        *height = tkbio.fb.vinfo.yres/tmp;
    }
    else
    {
        if(scrHeight && scrWidth)
        {
            *scrHeight = SCREENMAX/(*height);
            *scrWidth  = SCREENMAX/(*width);
        }
        *height = tkbio.fb.vinfo.yres/(*height);
        *width  = tkbio.fb.vinfo.xres/(*width);
    }
}

void tkbio_layout_to_fb_cords(int *y, int *x)
{
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        int tmp;
        tmp = *y;
        *y  = *x;
        *x  = tkbio.layout.maps[tkbio.parser.map].height-tmp-1;
    }
}

void tkbio_fb_to_layout_cords(int *y, int *x)
{
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        int tmp;
        tmp = *y;
        *y  = tkbio.layout.maps[tkbio.parser.map].height-*x-1;
        *x  = tmp;
    }
}

unsigned char tkbio_connect_to_borders(int y, int x, unsigned char connect)
{
    tkbio_layout_to_fb_cords(&y, &x);
    return tkbio_fb_connect_to_borders(y, x, connect);
}

unsigned char tkbio_fb_connect_to_borders(int y, int x, unsigned char connect)
{
    unsigned char borders = TKBIO_BORDER_ALL;
    const struct tkbio_map *map = &tkbio.layout.maps[tkbio.parser.map];
    
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        if(connect & TKBIO_LAYOUT_CONNECT_LEFT)
            borders &= ~TKBIO_BORDER_TOP;
        if(connect & TKBIO_LAYOUT_CONNECT_UP)
            borders &= ~TKBIO_BORDER_RIGHT;
        if(y < map->width-1 && map->map[(map->height-x-1)*map->width+y+1].connect & TKBIO_LAYOUT_CONNECT_LEFT)
            borders &= ~TKBIO_BORDER_BOTTOM;
        if(x > 0 && map->map[(map->height-x)*map->width+y].connect & TKBIO_LAYOUT_CONNECT_UP)
            borders &= ~TKBIO_BORDER_LEFT;
    }
    else
    {
        if(connect & TKBIO_LAYOUT_CONNECT_LEFT)
            borders &= ~TKBIO_BORDER_LEFT;
        if(connect & TKBIO_LAYOUT_CONNECT_UP)
            borders &= ~TKBIO_BORDER_TOP;
        if(x < map->width-1 && map->map[y*map->width+x+1].connect & TKBIO_LAYOUT_CONNECT_LEFT)
            borders &= ~TKBIO_BORDER_RIGHT;
        if(y < map->height-1 && map->map[(y+1)*map->width+x].connect & TKBIO_LAYOUT_CONNECT_UP)
            borders &= ~TKBIO_BORDER_BOTTOM;
    }
    
    return borders;
}

void tkbio_fb_draw_rect(int y, int x, int height, int width, int color, int density, unsigned char *copy)
{
    tkbio_draw_rect(y*height, x*width, height, width, color, density, copy);
}

void tkbio_layout_draw_rect(int y, int x, int height, int width, int color, int density, unsigned char *copy)
{
    tkbio_layout_to_fb_cords(&y, &x);
    tkbio_draw_rect(y*height, x*width, height, width, color, density, copy);
}

void tkbio_draw_rect(int y, int x, int height, int width, int color, int density, unsigned char *copy)
{
    unsigned char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    unsigned char *ptr = base;
    unsigned char col[4];
    int i, j, k;
    
    if(tkbio.fb.bpp == 2)
        tkbio_color32to16(col, tkbio.layout.colors[color]);
    else
        memcpy(col, tkbio.layout.colors[color], 4);
    
    for(i=0; i<height; i+=density, ptr = base)
    {
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            for(k=tkbio.fb.bpp-1; k>=0; k--)
            {
                if(copy)
                    *(copy++) = *ptr;
                *(ptr++) = col[k];
            }
        }
        base += density*tkbio.fb.finfo.line_length;
    }
}

void tkbio_fb_draw_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy)
{
    tkbio_draw_border(y*height, x*width, height, width, color, borders, density, copy);
}

void tkbio_fb_draw_connect(int y, int x, int height, int width, int color, unsigned char connect, int density, unsigned char *copy)
{
    unsigned char borders;
    
    borders = tkbio_fb_connect_to_borders(y, x, connect);
    tkbio_draw_border(y*height, x*width, height, width, color, borders, density, copy);
}

void tkbio_layout_draw_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy)
{
    tkbio_layout_to_fb_cords(&y, &x);
    tkbio_draw_border(y*height, x*width, height, width, color, borders, density, copy);
}

void tkbio_layout_draw_connect(int y, int x, int height, int width, int color, unsigned char connect, int density, unsigned char *copy)
{
    unsigned char borders;
    
    tkbio_layout_to_fb_cords(&y, &x);
    borders = tkbio_fb_connect_to_borders(y, x, connect);
    tkbio_draw_border(y*height, x*width, height, width, color, borders, density, copy);
}

void tkbio_draw_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy)
{
    unsigned char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    unsigned char *ptr = base, *base2 = base;
    unsigned char col[4];
    int i, j, k, skipped;
    
    if(tkbio.fb.bpp == 2)
        tkbio_color32to16(col, tkbio.layout.colors[color]);
    else
        memcpy(col, tkbio.layout.colors[color], 4);
    
    for(i=0, skipped=0; i<(borders&TKBIO_BORDER_BOTTOM?2:1); i++, ptr = base2)
    {
        if(!skipped && !(borders & TKBIO_BORDER_TOP))
            skipped = 1;
        else
            for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
            {
                for(k=tkbio.fb.bpp-1; k>=0; k--)
                {
                    if(copy)
                        *(copy++) = *ptr;
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
                if(copy)
                    *(copy++) = *ptr;
                *(ptr++) = col[k];
            }
        }
        base2 += density*tkbio.fb.finfo.line_length;
    }
}

void tkbio_fb_draw_rect_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy)
{
    tkbio_draw_rect_border(y*height, x*width, height, width, color, borders, density, copy);
}

void tkbio_fb_draw_rect_connect(int y, int x, int height, int width, int color, unsigned char connect, int density, unsigned char *copy)
{
    unsigned char borders;
    
    borders = tkbio_fb_connect_to_borders(y, x, connect);
    tkbio_draw_rect_border(y*height, x*width, height, width, color, borders, density, copy);
}

void tkbio_layout_draw_rect_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy)
{
    tkbio_layout_to_fb_cords(&y, &x);
    tkbio_draw_rect_border(y*height, x*width, height, width, color, borders, density, copy);
}

void tkbio_layout_draw_rect_connect(int y, int x, int height, int width, int color, unsigned char connect, int density, unsigned char *copy)
{
    unsigned char borders;
    
    tkbio_layout_to_fb_cords(&y, &x);
    borders = tkbio_fb_connect_to_borders(y, x, connect);
    tkbio_draw_rect_border(y*height, x*width, height, width, color, borders, density, copy);
}

void tkbio_draw_rect_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy)
{
    unsigned char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    unsigned char *ptr = base;
    unsigned char fg[4], bg[4], *col = bg;
    int i, j, k, cset;
    
    if(tkbio.fb.bpp == 2)
    {
        tkbio_color32to16(fg, tkbio.layout.colors[color>>4]);
        tkbio_color32to16(bg, tkbio.layout.colors[color&15]);
    }
    else
    {
        memcpy(fg, tkbio.layout.colors[color>>4], 4);
        memcpy(bg, tkbio.layout.colors[color&15], 4);
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
                if(copy)
                    *(copy++) = *ptr;
                *(ptr++) = col[k];
            }
            
            if(!cset)
                col = bg;
        }
        base += density*tkbio.fb.finfo.line_length;
    }
}

void tkbio_fb_fill_rect(int y, int x, int height, int width, int density, unsigned char *fill)
{
    tkbio_fill_rect(y*height, x*width, height, width, density, fill);
}

void tkbio_layout_fill_rect(int y, int x, int height, int width, int density, unsigned char *fill)
{
    tkbio_layout_to_fb_cords(&y, &x);
    tkbio_fill_rect(y*height, x*width, height, width, density, fill);
}

void tkbio_fill_rect(int y, int x, int height, int width, int density, unsigned char *fill)
{
    unsigned char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    unsigned char *ptr = base;
    int i, j, k;
    
    for(i=0; i<height; i+=density, ptr = base)
    {
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            for(k=tkbio.fb.bpp-1; k>=0; k--)
            {
                *(ptr++) = *fill;
                fill++;
            }
        }
        base += density*tkbio.fb.finfo.line_length;
    }
}

void tkbio_fb_fill_border(int y, int x, int height, int width, unsigned char borders, int density, unsigned char *fill)
{
    tkbio_fill_border(y*height, x*width, height, width, borders, density, fill);
}

void tkbio_fb_fill_connect(int y, int x, int height, int width, unsigned char connect, int density, unsigned char *fill)
{
    unsigned char borders;
    
    borders = tkbio_fb_connect_to_borders(y, x, connect);
    tkbio_fill_border(y*height, x*width, height, width, borders, density, fill);
}

void tkbio_layout_fill_border(int y, int x, int height, int width, unsigned char borders, int density, unsigned char *fill)
{
    tkbio_layout_to_fb_cords(&y, &x);
    tkbio_fill_border(y*height, x*width, height, width, borders, density, fill);
}

void tkbio_layout_fill_connect(int y, int x, int height, int width, unsigned char connect, int density, unsigned char *fill)
{
    unsigned char borders;
    
    tkbio_layout_to_fb_cords(&y, &x);
    borders = tkbio_fb_connect_to_borders(y, x, connect);
    tkbio_fill_border(y*height, x*width, height, width, borders, density, fill);
}

void tkbio_fill_border(int y, int x, int height, int width, unsigned char borders, int density, unsigned char *fill)
{
    unsigned char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    unsigned char *ptr = base, *base2 = base;
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
                    *(ptr++) = *fill;
                    fill++;
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
                *(ptr++) = *fill;
                fill++;
            }
        }
        base2 += density*tkbio.fb.finfo.line_length;
    }
}

