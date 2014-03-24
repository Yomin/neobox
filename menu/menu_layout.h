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
        {0x00, 0xFF, 0x00, 0x00}, // text
        {0x00, 0xCC, 0xCC, 0x00}  // admin
    };

#define COLOR_BG     0
#define COLOR_MENU   1
#define COLOR_SELECT 2
#define COLOR_TEXT   3
#define COLOR_ADMIN  4

#define VALUE(V) { .i = V }
#define ONE(C)   { .c = {{C,  0,  0,  0}} }

#define COLOR(C)        C,COLOR_BG,C
#define TEXTCOLOR(C)    C,COLOR_BG,COLOR_TEXT
#define DEFAULT_OPTIONS TKBIO_LAYOUT_OPTION_BORDER
#define SELECT_OPTIONS  TKBIO_LAYOUT_OPTION_BORDER|TKBIO_LAYOUT_OPTION_ALIGN_LEFT

#define CHAR(N, C)  {N, TKBIO_LAYOUT_TYPE_CHAR, 0, ONE(C), COLOR(COLOR_MENU), DEFAULT_OPTIONS}
#define UCHAR(N, C) {N, TKBIO_LAYOUT_TYPE_CHAR, 0, ONE(C), COLOR(COLOR_MENU), DEFAULT_OPTIONS|TKBIO_LAYOUT_OPTION_CONNECT_UP}

#define SEL(I)  {0, TKBIO_LAYOUT_TYPE_SELECT, I, VALUE(0), TEXTCOLOR(COLOR_SELECT), SELECT_OPTIONS}
#define LSEL(I) {0, TKBIO_LAYOUT_TYPE_SELECT, I, VALUE(0), TEXTCOLOR(COLOR_SELECT), SELECT_OPTIONS|TKBIO_LAYOUT_OPTION_CONNECT_LEFT}
#define SEL4(I) SEL(I),LSEL(I),LSEL(I),LSEL(I)

#define ADMIN  {"Admn", TKBIO_LAYOUT_TYPE_GOTO, 0, VALUE(1), COLOR(COLOR_ADMIN), DEFAULT_OPTIONS}
#define UADMIN {"Admn", TKBIO_LAYOUT_TYPE_GOTO, 0, VALUE(1), COLOR(COLOR_ADMIN), DEFAULT_OPTIONS|TKBIO_LAYOUT_OPTION_CONNECT_UP}

const struct tkbio_mapelem menu_map[] =
    {
        SEL4(0), CHAR("kbd", 'k'),
        SEL4(1), UCHAR("kbd", 'k'),
        SEL4(2), CHAR("Up", 'u'),
        SEL4(3), UCHAR("Up", 'u'),
        SEL4(4), UCHAR("Up", 'u'),
        SEL4(5), CHAR("Down", 'd'),
        SEL4(6), UCHAR("Down", 'd'),
        SEL4(7), UCHAR("Down", 'd'),
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
#undef COLOR_TEXT
#undef COLOR_ADMIN
#undef VALUE
#undef ONE
#undef COLOR
#undef TEXTCOLOR
#undef DEFAULT_OPTIONS
#undef SELECT_OPTIONS
#undef CHAR
#undef UCHAR
#undef SEL
#undef LSEL
#undef SEL4
#undef ADMIN
#undef UADMIN

#endif

