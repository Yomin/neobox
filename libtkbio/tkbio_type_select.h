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

#ifndef __TKBIO_TYPE_SELECT_H__
#define __TKBIO_TYPE_SELECT_H__

#include "tkbio_def.h"
#include "tkbio_layout.h"

#define TKBIO_TYPE_SELECT_STATUS_ACTIVE 1
#define TKBIO_TYPE_SELECT_STATUS_LOCKED 2

struct tkbio_save_select
{
    int status;
    int size;
    unsigned char *copy;
};

void tkbio_type_select_init(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);
void tkbio_type_select_finish(struct tkbio_save *save);

void tkbio_type_select_draw(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);

int tkbio_type_select_broader(int *y, int *x, int scr_y, int scr_x, const struct tkbio_mapelem *elem);

struct tkbio_return tkbio_type_select_press(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);
struct tkbio_return tkbio_type_select_move(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);
struct tkbio_return tkbio_type_select_release(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);
struct tkbio_return tkbio_type_select_focus_in(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);
struct tkbio_return tkbio_type_select_focus_out(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);

void tkbio_type_select_set_active(const void *active, int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);
void tkbio_type_select_set_locked(const void *locked, int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save);

#endif
