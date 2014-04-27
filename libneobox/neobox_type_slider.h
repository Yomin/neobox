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

#ifndef __NEOBOX_TYPE_SLIDER_H__
#define __NEOBOX_TYPE_SLIDER_H__

#include "neobox_def.h"
#include "neobox_layout.h"

struct neobox_save_slider
{
    int y, x;   // last pos in pixel
    int start;  // start pos in pixel, only used for partner slider
    int size;   // size in pixel, only used for partner slider
    int ticks, pos; // max ticks, current pos in ticks
    int y_tmp, x_tmp, pos_tmp; // keep old values to restore on focus_out
    unsigned char *copy; // copy save buffer
};

void neobox_type_slider_init(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
void neobox_type_slider_finish(struct neobox_save *save);

void neobox_type_slider_draw(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);

int neobox_type_slider_broader(int *y, int *x, int scr_y, int scr_x, const struct neobox_mapelem *elem);

struct neobox_return neobox_type_slider_press(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
struct neobox_return neobox_type_slider_move(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
struct neobox_return neobox_type_slider_release(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
struct neobox_return neobox_type_slider_focus_in(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
struct neobox_return neobox_type_slider_focus_out(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);

void neobox_type_slider_set_ticks(const void *ticks, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
void neobox_type_slider_set_pos(const void *pos, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);

#endif
