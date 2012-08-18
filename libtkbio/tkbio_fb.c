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
#include <string.h>

extern struct tkbio_global tkbio;

void tkbio_color32to16(char* dst, const char* src)
{
    dst[0] = (src[0] & ~7)>>0 | src[2]>>5;
    dst[1] = (src[2] & ~7)<<3 | (src[1] & ~7)>>2;
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

void tkbio_layout_to_fb_cords(int *y, int *x, int mapHeight)
{
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        int tmp;
        tmp = *y;
        *y  = *x;
        *x  = mapHeight-tmp-1;
    }
}

void tkbio_fb_to_layout_cords(int *y, int *x, int mapHeight)
{
    if(tkbio.format == TKBIO_FORMAT_LANDSCAPE)
    {
        int tmp;
        tmp = *y;
        *y  = mapHeight-*x-1;
        *x  = tmp;
    }
}

void tkbio_draw_rect(int y, int x, int height, int width, int color, int density, char *copy)
{
    tkbio_layout_to_fb_cords(&y, &x, tkbio.layout.maps[tkbio.parser.map].height);
    tkbio_fb_draw_rect(y, x, height, width, color, density, copy);
}

void tkbio_fb_draw_rect(int y, int x, int height, int width, int color, int density, char *copy)
{
    char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    char *ptr = base;
    char col[4];
    int i, j, k;
    
    if(tkbio.fb.bpp == 2)
        tkbio_color32to16(col, tkbio.layout.colors[color]);
    else
        memcpy(col, tkbio.layout.colors[color], 4);
    
    for(i=0; i<height; i+=density, ptr = base)
    {
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            for(k=0; k<tkbio.fb.bpp; k++)
            {
                if(copy)
                    *(copy++) = *ptr;
                *(ptr++) = col[k];
            }
        }
        base += density*tkbio.fb.finfo.line_length;
    }
}

void tkbio_draw_rect_border(int y, int x, int height, int width, int color, int density, char *copy)
{
    tkbio_layout_to_fb_cords(&y, &x, tkbio.layout.maps[tkbio.parser.map].height);
    tkbio_fb_draw_rect_border(y, x, height, width, color, density, copy);
}

void tkbio_fb_draw_rect_border(int y, int x, int height, int width, int color, int density, char *copy)
{
    char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    char *ptr = base, *base2 = base;
    char col[4];
    int i, j, k;
    
    if(tkbio.fb.bpp == 2)
        tkbio_color32to16(col, tkbio.layout.colors[color]);
    else
        memcpy(col, tkbio.layout.colors[color], 4);
    
    for(i=0; i<2; i++, ptr = base2)
    {
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            for(k=0; k<tkbio.fb.bpp; k++)
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
        for(j=0; j<2; j++, ptr+=tkbio.fb.bpp*(width-2))
        {
            for(k=0; k<tkbio.fb.bpp; k++)
            {
                if(copy)
                    *(copy++) = *ptr;
                *(ptr++) = col[k];
            }
        }
        base2 += density*tkbio.fb.finfo.line_length;
    }
}

void tkbio_fill_rect(int y, int x, int height, int width, int density, char *fill)
{
    tkbio_layout_to_fb_cords(&y, &x, tkbio.layout.maps[tkbio.parser.map].height);
    tkbio_fb_fill_rect(y, x, height, width, density, fill);
}

void tkbio_fb_fill_rect(int y, int x, int height, int width, int density, char *fill)
{
    char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    char *ptr = base;
    int i, j, k;
    
    for(i=0; i<height; i+=density, ptr = base)
    {
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            for(k=0; k<tkbio.fb.bpp; k++)
            {
                *(ptr++) = *fill;
                fill++;
            }
        }
        base += density*tkbio.fb.finfo.line_length;
    }
}

void tkbio_fill_rect_border(int y, int x, int height, int width, int density, char *fill)
{
    tkbio_layout_to_fb_cords(&y, &x, tkbio.layout.maps[tkbio.parser.map].height);
    tkbio_fb_fill_rect_border(y, x, height, width, density, fill);
}

void tkbio_fb_fill_rect_border(int y, int x, int height, int width, int density, char *fill)
{
    char *base = tkbio.fb.ptr + y*tkbio.fb.finfo.line_length + x*tkbio.fb.bpp;
    char *ptr = base, *base2 = base;
    int i, j, k;
    
    for(i=0; i<2; i++, ptr = base2)
    {
        for(j=0; j<width; j+=density, ptr+=tkbio.fb.bpp*(density-1))
        {
            for(k=0; k<tkbio.fb.bpp; k++)
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
        for(j=0; j<2; j++, ptr+=tkbio.fb.bpp*(width-2))
        {
            for(k=0; k<tkbio.fb.bpp; k++)
            {
                *(ptr++) = *fill;
                fill++;
            }
        }
        base2 += density*tkbio.fb.finfo.line_length;
    }
}

