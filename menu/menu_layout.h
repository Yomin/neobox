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

#ifndef __MENU_LAYOUT_H__
#define __MENU_LAYOUT_H__

#include "tkbio_layout_default.h"

const unsigned char menu_colors[][4] =
    {
        {0x00, 0x00, 0x00, 0x00}, // bg
        {0xFF, 0x00, 0x00, 0x00}, // menu
        {0x55, 0x55, 0x55, 0x00}, // select
        {0x00, 0xCC, 0xCC, 0x00}  // admin
    };

#define COLOR_BG     0
#define COLOR_MENU   1
#define COLOR_SELECT 2
#define COLOR_ADMIN  3

#define VALUE(V) { .i = V }
#define ONE(C)   { .c = {{C,  0,  0,  0}} }

#define COLOR(Color)    (Color << 4 | COLOR_BG)
#define DEFAULT_OPTIONS TKBIO_LAYOUT_TYPE_CHAR|TKBIO_LAYOUT_OPTION_BORDER
#define GOTO_OPTIONS    TKBIO_LAYOUT_TYPE_GOTO|TKBIO_LAYOUT_OPTION_BORDER
#define SELECT_OPTIONS  TKBIO_LAYOUT_TYPE_SELECT|TKBIO_LAYOUT_OPTION_BORDER

#define CHAR(C)   {0, DEFAULT_OPTIONS, 0, ONE(C), COLOR(COLOR_MENU), 0}
#define UCHAR(C)  {0, DEFAULT_OPTIONS, 0, ONE(C), COLOR(COLOR_MENU), TKBIO_LAYOUT_CONNECT_UP}

#define SEL(I)  {0, SELECT_OPTIONS, I, VALUE(0), COLOR(COLOR_SELECT), 0}
#define LSEL(I) {0, SELECT_OPTIONS, I, VALUE(0), COLOR(COLOR_SELECT), TKBIO_LAYOUT_CONNECT_LEFT}
#define SEL4(I) SEL(I),LSEL(I),LSEL(I),LSEL(I)

#define ADMIN  {"Admn", GOTO_OPTIONS, 0, VALUE(1), COLOR(COLOR_ADMIN), 0}
#define UADMIN {"Admn", GOTO_OPTIONS, 0, VALUE(1), COLOR(COLOR_ADMIN), TKBIO_LAYOUT_CONNECT_UP}

const struct tkbio_mapelem menu_map[] =
    {
        SEL4(0), CHAR('k'),
        SEL4(1), UCHAR('k'),
        SEL4(2), CHAR('u'),
        SEL4(3), UCHAR('u'),
        SEL4(4), UCHAR('u'),
        SEL4(5), CHAR('d'),
        SEL4(6), UCHAR('d'),
        SEL4(7), UCHAR('d'),
        SEL4(8), ADMIN,
        SEL4(9), UADMIN
    };

const struct tkbio_map menu_maps[] =
    {
        {10, 5, menu_map, menu_colors},
        ADMIN_MAP(1)
    };

const struct tkbio_layout menuLayout =
    {
        .start  = 0,
        .size   = sizeof(menu_maps)/sizeof(struct tkbio_map),
        .maps   = menu_maps,
        .fun    = 0
    };

#undef COLOR_BG
#undef COLOR_MENU
#undef COLOR_SELECT
#undef COLOR_ADMIN
#undef VALUE
#undef ONE
#undef COLOR
#undef DEFAULT_OPTIONS
#undef GOTO_OPTIONS
#undef SELECT_OPTIONS
#undef CHAR
#undef UCHAR
#undef SEL
#undef LSEL
#undef SEL4
#undef ADMIN
#undef UADMIN

#endif

