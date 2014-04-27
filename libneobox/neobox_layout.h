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

#ifndef __NEOBOX_LAYOUT_H__
#define __NEOBOX_LAYOUT_H__

#define NEOBOX_LAYOUT_TYPE_NOP       0
#define NEOBOX_LAYOUT_TYPE_CHAR      1
#define NEOBOX_LAYOUT_TYPE_GOTO      2
#define NEOBOX_LAYOUT_TYPE_HOLD      3
#define NEOBOX_LAYOUT_TYPE_TOGGLE    4
#define NEOBOX_LAYOUT_TYPE_SYSTEM    5
#define NEOBOX_LAYOUT_TYPE_HSLIDER   6
#define NEOBOX_LAYOUT_TYPE_VSLIDER   7
#define NEOBOX_LAYOUT_TYPE_SELECT    8

#define NEOBOX_LAYOUT_OPTION_ALIGN_CENTER    0
#define NEOBOX_LAYOUT_OPTION_ALIGN_TOP       1
#define NEOBOX_LAYOUT_OPTION_ALIGN_LEFT      2
#define NEOBOX_LAYOUT_OPTION_ALIGN_BOTTOM    3
#define NEOBOX_LAYOUT_OPTION_ALIGN_RIGHT     4
#define NEOBOX_LAYOUT_OPTION_MASK_ALIGN      (1+2+4)
#define NEOBOX_LAYOUT_OPTION_BORDER          8
#define NEOBOX_LAYOUT_OPTION_COPY            16
#define NEOBOX_LAYOUT_OPTION_CONNECT_LEFT    32
#define NEOBOX_LAYOUT_OPTION_CONNECT_UP      64
#define NEOBOX_LAYOUT_OPTION_MASK_CONNECT    (32+64)
#define NEOBOX_LAYOUT_OPTION_SET_MAP         128

#define NEOBOX_CHARELEM_MAX          4

union neobox_charelem
{
    char c[NEOBOX_CHARELEM_MAX];
    unsigned char u[NEOBOX_CHARELEM_MAX];
};

union neobox_elem
{
    union neobox_charelem c;
    int i;
};

typedef union neobox_elem neobox_parsefun(int map, union neobox_elem elem, unsigned char toggle);

struct neobox_mapelem
{
    char *name;
    unsigned char type, id;
    union neobox_elem elem;
    unsigned char color_fg, color_bg, color_text;
    unsigned char options;
};

struct neobox_map
{
    int height, width;
    const struct neobox_mapelem *map;
    const unsigned char (*colors)[4];
    unsigned char offset; // map offset for goto
};

struct neobox_layout
{
    int start, size;
    const struct neobox_map *maps;
    neobox_parsefun *fun;
};

#endif

