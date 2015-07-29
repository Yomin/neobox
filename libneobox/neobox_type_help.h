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

#ifndef __NEOBOX_TYPE_HELP_H__
#define __NEOBOX_TYPE_HELP_H__

#include "neobox_def.h"
#include "neobox_layout.h"

typedef void* neobox_type_help_action_func(void *value, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save);

int   neobox_type_help_find_type(int type_from, int type_to, int id, int map);
void* neobox_type_help_action_pos(int pos, int map, void *value, int redraw, neobox_type_help_action_func f);
void* neobox_type_help_action_range(int type_from, int type_to, int id, int map, void *value, int redraw, neobox_type_help_action_func f);
void* neobox_type_help_action(int type, int id, int map, void *value, int redraw, neobox_type_help_action_func f);

#endif
