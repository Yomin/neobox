/*
 * Copyright (c) 2012 Martin Rödel aka Yomin
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

#ifndef __GPS_LAYOUT_H__
#define __GPS_LAYOUT_H__

#include "neobox_layout.h"

const char gps_colors[1][4] =
    {
        {0x00, 0x00, 0xFF, 0x00}
    };

#define GPS_TYPE_PRIMARY    0

#define ONE(C) {{C,  0,  0,  0}}

#define COLOR(Color)    Color << 4
#define DEFAULT_OPTIONS NEOBOX_LAYOUT_TYPE_CHAR|NEOBOX_LAYOUT_OPTION_BORDER

#define CHAR(C)  {DEFAULT_OPTIONS, ONE(C), COLOR(GPS_TYPE_PRIMARY), 0}
#define NOP      {NEOBOX_LAYOUT_TYPE_CHAR, ONE(0), 0, 0}
#define BNOP     {DEFAULT_OPTIONS, ONE(0), 0, 0}
#define CBNOP    {DEFAULT_OPTIONS, ONE(0), 0, NEOBOX_LAYOUT_CONNECT_LEFT}


const struct neobox_mapelem gps_map_primary[5*6] =
    {
        NOP,  NOP,   NOP,   NOP,   NOP,   CHAR('+'),
        NOP,  NOP,   NOP,   NOP,   NOP,   CHAR('-'),
        NOP,  NOP,   NOP,   NOP,   NOP,   BNOP,
        NOP,  NOP,   NOP,   NOP,   NOP,   CHAR('o'),
        BNOP, CBNOP, CBNOP, CBNOP, CBNOP, CHAR('q')
    };

const struct neobox_map gps_maps[1] =
    {
        {5, 6, gps_map_primary}
    };

struct neobox_layout gpsLayout =
    {
        .start  = 0,
        .size   = sizeof(gps_maps)/sizeof(struct neobox_map),
        .maps   = gps_maps,
        .colors = gps_colors,
        .fun    = 0
    };

#undef ONE
#undef COLOR
#undef DEFAULT_OPTIONS
#undef CHAR
#undef NOP
#undef BNOP
#undef CNOP

#endif

