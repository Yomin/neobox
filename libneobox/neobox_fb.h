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

#ifndef __NEOBOX_FB_H__
#define __NEOBOX_FB_H__

#include "neobox_layout.h"

#define NEOBOX_BORDER_LEFT   1
#define NEOBOX_BORDER_RIGHT  2
#define NEOBOX_BORDER_TOP    4
#define NEOBOX_BORDER_BOTTOM 8
#define NEOBOX_BORDER_ALL    15

unsigned char* neobox_color(unsigned char dst[4], int color, const struct neobox_map *map);

void neobox_get_sizes(int *height, int *width, int *fb_height, int *fb_width, int *screen_height, int *screen_width, const struct neobox_map *map);

void neobox_layout_to_fb_sizes(int *height, int *width);
void neobox_fb_to_layout_sizes(int *height, int *width);
void neobox_layout_to_fb_cords(int *cord_y, int *cord_x, const struct neobox_map *map);
void neobox_fb_to_layout_cords(int *cord_y, int *cord_x, const struct neobox_map *map);
void neobox_layout_to_fb_pos(int *pos_y, int *pos_x);
void neobox_fb_to_layout_pos(int *pos_y, int *pos_x);
void neobox_layout_to_fb_pos_width(int *pos_y, int *pos_x, int width);
void neobox_fb_to_layout_pos_width(int *pos_y, int *pos_x, int width);
void neobox_layout_to_fb_pos_rel(int *pos_y, int *pos_x, int height);
void neobox_fb_to_layout_pos_rel(int *pos_y, int *pos_x, int width);

unsigned char neobox_fb_connect_to_borders(int y, int x, unsigned char connect, const struct neobox_map *map);
unsigned char neobox_layout_connect_to_borders(int y, int x, unsigned char connect, const struct neobox_map *map);

void neobox_draw_rect(int pos_y, int pos_x, int height, int width, unsigned char color[4], int density, unsigned char **copy);
void neobox_layout_draw_rect(int pos_y, int pos_x, int height, int width, int color, int density, const struct neobox_map *map, unsigned char **copy);

void neobox_draw_border(int pos_y, int pos_x, int height, int width, unsigned char color[4], unsigned char borders, int density, unsigned char **copy);
void neobox_draw_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char color[4], unsigned char connect, int density, const struct neobox_map *map, unsigned char **copy);
void neobox_layout_draw_border(int pos_y, int pos_x, int height, int width, int color, unsigned char borders, int density, const struct neobox_map *map, unsigned char **copy);
void neobox_layout_draw_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, int color, unsigned char connect, int density, const struct neobox_map *map, unsigned char **copy);

void neobox_draw_rect_border(int pos_y, int pos_x, int height, int width, unsigned char color_fg[4], unsigned char color_bg[4], unsigned char borders, int density, unsigned char **copy);
void neobox_draw_rect_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char color_fg[4], unsigned char color_bg[4], unsigned char borders, int density, const struct neobox_map *map, unsigned char **copy);
void neobox_layout_draw_rect_border(int pos_y, int pos_x, int height, int width, int color_fg, int color_bg, unsigned char borders, int density, const struct neobox_map *map, unsigned char **copy);
void neobox_layout_draw_rect_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, int color_fg, int color_bg, unsigned char connect, int density, const struct neobox_map *map, unsigned char **copy);

void neobox_fill_rect(int pos_y, int pos_x, int height, int width, int density, unsigned char **fill);
void neobox_layout_fill_rect(int pos_y, int pos_x, int height, int width, int density, unsigned char **fill);

void neobox_fill_border(int pos_y, int pos_x, int height, int width, unsigned char borders, int density, unsigned char **fill);
void neobox_fill_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char borders, int density, const struct neobox_map *map, unsigned char **fill);
void neobox_layout_fill_border(int pos_y, int pos_x, int height, int width, unsigned char borders, int density, unsigned char **fill);
void neobox_layout_fill_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char connect, int density, const struct neobox_map *map, unsigned char **fill);

void neobox_draw_string(int pos_y, int pos_x, int height, int width, unsigned char color[4], int align, const char *str);
void neobox_draw_string_rotate(int pos_y, int pos_x, int height, int width, unsigned char color[4], int align, const char *str);
void neobox_layout_draw_string(int pos_y, int pos_x, int height, int width, int color, int align, const char *str, const struct neobox_map *map);

#endif
