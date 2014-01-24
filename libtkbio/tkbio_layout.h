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

#ifndef __TKBIO_LAYOUT_H__
#define __TKBIO_LAYOUT_H__

#define TKBIO_LAYOUT_TYPE_CHAR      0
#define TKBIO_LAYOUT_TYPE_GOTO      1
#define TKBIO_LAYOUT_TYPE_HOLD      2
#define TKBIO_LAYOUT_TYPE_TOGG      3
#define TKBIO_LAYOUT_OPTION_BORDER  4
#define TKBIO_LAYOUT_OPTION_COPY    8
#define TKBIO_LAYOUT_CONNECT_LEFT   1
#define TKBIO_LAYOUT_CONNECT_UP     2

#define TKBIO_LAYOUT_MASK_TYPE      3

#define TKBIO_CHARELEM_MAX          4

struct tkbio_charelem
{
    char c[TKBIO_CHARELEM_MAX];
};

typedef struct tkbio_charelem tkbio_parsefun(int map, struct tkbio_charelem elem, char toggle);

struct tkbio_mapelem
{
    char type;
    struct tkbio_charelem elem;
    char color;
    char connect;
};

struct tkbio_map
{
    int height, width;
    const struct tkbio_mapelem *map;
};

struct tkbio_layout
{
    int start, size;
    const struct tkbio_map *maps;
    const char (*colors)[4];
    tkbio_parsefun *fun;
};

#endif

