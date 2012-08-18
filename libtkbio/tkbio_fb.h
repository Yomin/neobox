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

#ifndef __TKBIO_FB_H__
#define __TKBIO_FB_H__

void tkbio_color32to16(char* dst, const char* src);

void tkbio_layout_to_fb_sizes(int *height, int *width, int *scrHeight, int *scrWidth);
void tkbio_layout_to_fb_cords(int *y, int *x, int mapHeight);
void tkbio_fb_to_layout_cords(int *y, int *x, int mapHeight);

void tkbio_fb_draw_rect(int y, int x, int height, int width, int color, int density, char *copy);
void tkbio_fb_draw_rect_border(int y, int x, int height, int width, int color, int density, char *copy);
void tkbio_fb_fill_rect(int y, int x, int height, int width, int density, char *fill);
void tkbio_fb_fill_rect_border(int y, int x, int height, int width, int density, char *fill);

#endif

