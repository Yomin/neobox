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

#define TKBIO_LAYOUT_TYPE_NOP       0
#define TKBIO_LAYOUT_TYPE_CHAR      1
#define TKBIO_LAYOUT_TYPE_GOTO      2
#define TKBIO_LAYOUT_TYPE_HOLD      3
#define TKBIO_LAYOUT_TYPE_TOGGLE    4
#define TKBIO_LAYOUT_TYPE_HSLIDER   5
#define TKBIO_LAYOUT_TYPE_VSLIDER   6
#define TKBIO_LAYOUT_TYPE_SYSTEM    7
#define TKBIO_LAYOUT_TYPE_SELECT    8

#define TKBIO_LAYOUT_OPTION_ALIGN_CENTER    0
#define TKBIO_LAYOUT_OPTION_ALIGN_TOP       1
#define TKBIO_LAYOUT_OPTION_ALIGN_BOTTOM    2
#define TKBIO_LAYOUT_OPTION_ALIGN_LEFT      3
#define TKBIO_LAYOUT_OPTION_ALIGN_RIGHT     4
#define TKBIO_LAYOUT_OPTION_MASK_ALIGN      (1+2+4)
#define TKBIO_LAYOUT_OPTION_BORDER          8
#define TKBIO_LAYOUT_OPTION_COPY            16
#define TKBIO_LAYOUT_OPTION_CONNECT_LEFT    32
#define TKBIO_LAYOUT_OPTION_CONNECT_UP      64
#define TKBIO_LAYOUT_OPTION_MASK_CONNECT    (32+64)

#define TKBIO_CHARELEM_MAX          4

#define TKBIO_SYSTEM_PREV           0
#define TKBIO_SYSTEM_NEXT           1
#define TKBIO_SYSTEM_QUIT           2
#define TKBIO_SYSTEM_ACTIVATE       3
#define TKBIO_SYSTEM_MENU           4

#define TKBIO_RETURN_NOP            0
#define TKBIO_RETURN_CHAR           1
#define TKBIO_RETURN_INT            2
#define TKBIO_RETURN_QUIT           3
#define TKBIO_RETURN_SWITCH         4
#define TKBIO_RETURN_ACTIVATE       5

struct tkbio_charelem
{
    char c[TKBIO_CHARELEM_MAX];
};

union tkbio_elem
{
    struct tkbio_charelem c;
    int i;
};

struct tkbio_return
{
    unsigned char type, id;
    union tkbio_elem value;
};

typedef union tkbio_elem tkbio_parsefun(int map, union tkbio_elem elem, unsigned char toggle);

struct tkbio_mapelem
{
    char *name;
    unsigned char type, id;
    union tkbio_elem elem;
    unsigned char color;
    unsigned char options;
};

struct tkbio_map
{
    int height, width;
    const struct tkbio_mapelem *map;
    const unsigned char (*colors)[4];
    unsigned char offset; // map offset for goto
};

struct tkbio_layout
{
    int start, size;
    const struct tkbio_map *maps;
    tkbio_parsefun *fun;
};

#endif

