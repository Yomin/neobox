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

#ifndef __TKBIO_FB_H__
#define __TKBIO_FB_H__

#define TKBIO_BORDER_LEFT   1
#define TKBIO_BORDER_RIGHT  2
#define TKBIO_BORDER_TOP    4
#define TKBIO_BORDER_BOTTOM 8
#define TKBIO_BORDER_ALL    15

void tkbio_color32to16(unsigned char* dst, const unsigned char* src);

void tkbio_layout_to_fb_sizes(int *height, int *width, int *scrHeight, int *scrWidth);
void tkbio_layout_to_fb_cords(int *y, int *x);
void tkbio_fb_to_layout_cords(int *y, int *x);

unsigned char tkbio_connect_to_borders(int y, int x, unsigned char connect);
unsigned char tkbio_fb_connect_to_borders(int y, int x, unsigned char connect);

void tkbio_draw_rect(int y, int x, int height, int width, int color, int density, unsigned char *copy);
void tkbio_fb_draw_rect(int y, int x, int height, int width, int color, int density, unsigned char *copy);
void tkbio_layout_draw_rect(int y, int x, int height, int width, int color, int density, unsigned char *copy);

void tkbio_draw_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy);
void tkbio_fb_draw_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy);
void tkbio_fb_draw_connect(int y, int x, int height, int width, int color, unsigned char connect, int density, unsigned char *copy);
void tkbio_layout_draw_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy);
void tkbio_layout_draw_connect(int y, int x, int height, int width, int color, unsigned char connect, int density, unsigned char *copy);

void tkbio_draw_rect_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy);
void tkbio_fb_draw_rect_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy);
void tkbio_fb_draw_rect_connect(int y, int x, int height, int width, int color, unsigned char connect, int density, unsigned char *copy);
void tkbio_layout_draw_rect_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy);
void tkbio_layout_draw_rect_connect(int y, int x, int height, int width, int color, unsigned char connect, int density, unsigned char *copy);

void tkbio_fill_rect(int y, int x, int height, int width, int density, unsigned char *fill);
void tkbio_fb_fill_rect(int y, int x, int height, int width, int density, unsigned char *fill);
void tkbio_layout_fill_rect(int y, int x, int height, int width, int density, unsigned char *fill);

void tkbio_fill_border(int y, int x, int height, int width, unsigned char borders, int density, unsigned char *fill);
void tkbio_fb_fill_border(int y, int x, int height, int width, unsigned char borders, int density, unsigned char *fill);
void tkbio_fb_fill_connect(int y, int x, int height, int width, unsigned char connect, int density, unsigned char *fill);
void tkbio_layout_fill_border(int y, int x, int height, int width, unsigned char borders, int density, unsigned char *fill);
void tkbio_layout_fill_connect(int y, int x, int height, int width, unsigned char connect, int density, unsigned char *fill);


#endif

