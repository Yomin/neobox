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

#ifndef __2048_LAYOUT_H__
#define __2048_LAYOUT_H__

#include "tkbio_layout_default.h"

const unsigned char app2048_colors[][4] =
    {
        {0x00, 0x00, 0x00, 0x00},
        {0xFF, 0xFF, 0x00, 0x00},
        {0x00, 0xCC, 0xCC, 0x00}
    };

#define COLOR_BG    0
#define COLOR_FG    1
#define COLOR_ADMIN 2

#define ONE(C)      { .c = {{C,  0,  0,  0}} }
#define VALUE(V)    { .i = V }

#define COLOR(C)        C,COLOR_BG,C
#define DEFAULT_OPTIONS TKBIO_LAYOUT_OPTION_BORDER

#define NOP     {0, TKBIO_LAYOUT_TYPE_NOP, 23, ONE(0), 0, 0}
#define BTN(I)  {0, TKBIO_LAYOUT_TYPE_CHAR, I, ONE(0), COLOR(COLOR_FG), DEFAULT_OPTIONS}
#define BNOP(I) {0, TKBIO_LAYOUT_TYPE_NOP, I, ONE(0), COLOR(COLOR_FG), DEFAULT_OPTIONS}
#define HEAD    {0, TKBIO_LAYOUT_TYPE_CHAR, 42, ONE(0), COLOR(COLOR_FG), 0}
#define LHEAD   {0, TKBIO_LAYOUT_TYPE_CHAR, 42, ONE(0), COLOR(COLOR_FG), TKBIO_LAYOUT_OPTION_CONNECT_LEFT}
#define ADMIN   {0, TKBIO_LAYOUT_TYPE_GOTO, 0, VALUE(1), COLOR(COLOR_ADMIN), DEFAULT_OPTIONS}

const struct tkbio_mapelem app2048_map[] =
    {
        NOP,      HEAD,     LHEAD,    NOP,
        BNOP(0),  BTN(1),   BTN(2),   BNOP(3),
        BTN(4),   BNOP(5),  BNOP(6),  BTN(7),
        BTN(8),   BNOP(9),  BNOP(10), BTN(11),
        BNOP(12), BTN(13),  BTN(14),  BNOP(15),
        NOP,      NOP,      NOP,      ADMIN
    };

const struct tkbio_map app2048_maps[] =
    {
        {6, 4, app2048_map, app2048_colors},
        ADMIN_MAP(1)
    };

const struct tkbio_layout app2048Layout =
    {
        .start  = 0,
        .size   = sizeof(app2048_maps)/sizeof(struct tkbio_map),
        .maps   = app2048_maps,
        .fun    = 0
    };

#undef COLOR_BG
#undef COLOR_FG
#undef ONE
#undef VALUE
#undef COLOR
#undef DEFAULT_OPTIONS
#undef BTN
#undef NOP
#undef BNOP
#undef HEAD
#undef LHEAD
#undef ADMIN

#endif

