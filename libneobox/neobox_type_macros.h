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

#ifndef __NEOBOX_TYPE_MACROS_H__
#define __NEOBOX_TYPE_MACROS_H__

#include "neobox_def.h"
#include "neobox_layout.h"

#define TYPE_FUNC_INIT(NAME) \
    void neobox_type_##NAME##_init(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
#define TYPE_FUNC_FINISH(NAME) \
    void neobox_type_##NAME##_finish(struct neobox_save *save)

#define TYPE_FUNC_DRAW(NAME) \
    void neobox_type_##NAME##_draw(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
#define TYPE_FUNC_BROADER(NAME) \
    int neobox_type_##NAME##_broader(int *y, int *x, int scr_y, int scr_x, const struct neobox_mapelem *elem)

#define TYPE_FUNC_PRESS(NAME) \
    struct neobox_event neobox_type_##NAME##_press(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
#define TYPE_FUNC_MOVE(NAME) \
    struct neobox_event neobox_type_##NAME##_move(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
#define TYPE_FUNC_RELEASE(NAME) \
    struct neobox_event neobox_type_##NAME##_release(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
#define TYPE_FUNC_FOCUS_IN(NAME) \
    struct neobox_event neobox_type_##NAME##_focus_in(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
#define TYPE_FUNC_FOCUS_OUT(NAME) \
    struct neobox_event neobox_type_##NAME##_focus_out(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)

#define TYPE_FUNC_ACTION(NAME, ACTION) \
    void neobox_type_##NAME##_##ACTION(const void *data, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)

#define TYPE_FUNCTIONS(NAME) \
    TYPE_FUNC_INIT(NAME); \
    TYPE_FUNC_FINISH(NAME); \
    TYPE_FUNC_DRAW(NAME); \
    TYPE_FUNC_BROADER(NAME); \
    TYPE_FUNC_PRESS(NAME); \
    TYPE_FUNC_MOVE(NAME); \
    TYPE_FUNC_RELEASE(NAME); \
    TYPE_FUNC_FOCUS_IN(NAME); \
    TYPE_FUNC_FOCUS_OUT(NAME)

#endif
