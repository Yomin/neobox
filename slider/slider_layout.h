/*
 * Copyright (c) 2013-2014 Martin Rödel aka Yomin
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

#ifndef __SLIDER_LAYOUT_H__
#define __SLIDER_LAYOUT_H__

#include "tkbio_layout_default.h"

const unsigned char slider_colors[][4] =
    {
        {0x00, 0x00, 0x00, 0x00}, // BG
        {0xFF, 0x00, 0x00, 0x00}, // SLIDER
        {0x00, 0xCC, 0xCC, 0x00}  // ADMIN
    };

#define COLOR_BG 0
#define COLOR_SLIDER 1
#define COLOR_ADMIN  2

#define VALUE(V) { .i = V }

#define COLOR(Color)    (Color << 4 | COLOR_BG)
#define DEFAULT_OPTIONS TKBIO_LAYOUT_OPTION_BORDER

#define GOTO(N, P, C)  {N, TKBIO_LAYOUT_TYPE_GOTO, 0, VALUE(P), COLOR(COLOR_ ## C), DEFAULT_OPTIONS}
#define LGOTO(N, P, C) {N, TKBIO_LAYOUT_TYPE_GOTO, 0, VALUE(P), COLOR(COLOR_ ## C), DEFAULT_OPTIONS|TKBIO_LAYOUT_OPTION_CONNECT_LEFT}
#define NOP            {0, TKBIO_LAYOUT_TYPE_NOP, 0, VALUE(0), 0, 0}
#define SLID(I, C)     {0, TKBIO_LAYOUT_TYPE_HSLIDER, I, VALUE(10), COLOR(COLOR_SLIDER), DEFAULT_OPTIONS|C}

#define ADMIN   GOTO("Admn", 1, ADMIN)
#define LADMIN  LGOTO("Admn", 1, ADMIN)

#define HSLID   SLID(0, 0)
#define LHSLID  SLID(0, TKBIO_LAYOUT_OPTION_CONNECT_LEFT)
#define UHSLID  SLID(0, TKBIO_LAYOUT_OPTION_CONNECT_UP)
#define LUHSLID SLID(0, TKBIO_LAYOUT_OPTION_CONNECT_LEFT|TKBIO_LAYOUT_OPTION_CONNECT_UP)

#define NOP3     NOP,NOP,NOP
#define LHSLID3  LHSLID,LHSLID,LHSLID
#define LUHSLID3 LUHSLID,LUHSLID,LUHSLID
#define NOP7     NOP3,NOP3,NOP
#define LHSLID7  LHSLID3,LHSLID3,LHSLID
#define LUHSLID7 LUHSLID3,LUHSLID3,LUHSLID

const struct tkbio_mapelem slider_map[] =
    {
        NOP, NOP,    NOP7,     NOP,     NOP,
        NOP, HSLID,  LHSLID7,  LHSLID,  NOP,
        NOP, UHSLID, LUHSLID7, LUHSLID, NOP,
        NOP, NOP,    NOP7,     NOP,     NOP,
        NOP, NOP,    NOP7,     ADMIN,   LADMIN
    };

const struct tkbio_map slider_maps[] =
    {
        {5, 11, slider_map, slider_colors, 0},
        ADMIN_MAP(1)
    };

struct tkbio_layout sliderLayout =
    {
        .start  = 0,
        .size   = sizeof(slider_maps)/sizeof(struct tkbio_map),
        .maps   = slider_maps,
        .fun    = 0
    };

#undef COLOR_BG
#undef COLOR_SLIDER
#undef COLOR_ADMIN
#undef VALUE
#undef ONE
#undef COLOR
#undef DEFAULT_OPTIONS
#undef GOTO
#undef LGOTO
#undef NOP
#undef SLID
#undef ADMIN
#undef LADMIN
#undef HSLID
#undef UHSLID
#undef LHSLID
#undef LUHSLID
#undef NOP3
#undef LHSLID3
#undef LUHSLID3
#undef NOP7
#undef LHSLID7
#undef LUHSLID7

#endif
