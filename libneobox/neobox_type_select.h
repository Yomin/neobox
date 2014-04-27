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

#ifndef __NEOBOX_TYPE_SELECT_H__
#define __NEOBOX_TYPE_SELECT_H__

#include "neobox_def.h"
#include "neobox_layout.h"

#define NEOBOX_TYPE_SELECT_STATUS_ACTIVE 1
#define NEOBOX_TYPE_SELECT_STATUS_LOCKED 2

struct neobox_save_select
{
    const char *name;
    int status;
    int size;
    unsigned char *copy;
};

void neobox_type_select_init(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
void neobox_type_select_finish(struct neobox_save *save);

void neobox_type_select_draw(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);

int neobox_type_select_broader(int *y, int *x, int scr_y, int scr_x, const struct neobox_mapelem *elem);

struct neobox_event neobox_type_select_press(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
struct neobox_event neobox_type_select_move(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
struct neobox_event neobox_type_select_release(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
struct neobox_event neobox_type_select_focus_in(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
struct neobox_event neobox_type_select_focus_out(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);

void neobox_type_select_set_name(const void *name, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
void neobox_type_select_set_active(const void *active, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);
void neobox_type_select_set_locked(const void *locked, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);

#endif
