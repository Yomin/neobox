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

#ifndef __LOGIN_LAYOUT_H__
#define __LOGIN_LAYOUT_H__

#include "neobox_layout.h"

const unsigned char login_colors[][4] =
    {
        {0x00, 0x00, 0x00, 0x00}, // bg
        {0x00, 0xFF, 0xFF, 0x00}  // fg
    };

#define COLOR_BG    0
#define COLOR_FG    1

#define ONE(C)          { .c = {{C, 0, 0, 0}} }
#define COLOR(C)        C,COLOR_BG,C
#define DEFAULT_OPTIONS NEOBOX_LAYOUT_OPTION_BORDER

#define NOP       {0, NEOBOX_LAYOUT_TYPE_NOP, 0, ONE(0), 0, 0, 0, 0}
#define CHAR(I)   {0, NEOBOX_LAYOUT_TYPE_CHAR, I, ONE(0), COLOR(COLOR_FG), DEFAULT_OPTIONS}
#define LCHAR(I)  {0, NEOBOX_LAYOUT_TYPE_CHAR, I, ONE(0), COLOR(COLOR_FG), DEFAULT_OPTIONS|NEOBOX_LAYOUT_OPTION_CONNECT_LEFT}
#define UCHAR(I)  {0, NEOBOX_LAYOUT_TYPE_CHAR, I, ONE(0), COLOR(COLOR_FG), DEFAULT_OPTIONS|NEOBOX_LAYOUT_OPTION_CONNECT_UP}
#define LUCHAR(I) {0, NEOBOX_LAYOUT_TYPE_CHAR, I, ONE(0), COLOR(COLOR_FG), DEFAULT_OPTIONS|NEOBOX_LAYOUT_OPTION_CONNECT_LEFT|NEOBOX_LAYOUT_OPTION_CONNECT_UP}

#define NOP2    NOP,NOP
#define BTN(I)  CHAR(I),LCHAR(I)
#define UBTN(I) UCHAR(I),LUCHAR(I)

const struct neobox_mapelem login_map[] =
    {
        NOP, NOP2,    NOP2,    NOP2,    NOP,
        NOP, BTN(1),  BTN(2),  BTN(3),  NOP,
        NOP, UBTN(1), UBTN(2), UBTN(3), NOP,
        NOP, BTN(4),  BTN(0),  BTN(5),  NOP,
        NOP, UBTN(4), UBTN(0), UBTN(5), NOP,
        NOP, BTN(6),  BTN(7),  BTN(8),  NOP,
        NOP, UBTN(6), UBTN(7), UBTN(8), NOP,
        NOP, NOP2,    NOP2,    NOP2,    NOP
    };

const struct neobox_map login_maps[] =
    {
        {8, 8, login_map, login_colors}
    };

const struct neobox_layout loginLayout =
    {
        .start  = 0,
        .size   = sizeof(login_maps)/sizeof(struct neobox_map),
        .maps   = login_maps,
        .fun    = 0
    };

#undef COLOR_BG
#undef COLOR_FG
#undef ONE
#undef COLOR
#undef DEFAULT_OPTIONS
#undef BTN
#undef LBTN
#undef UBTN
#undef LUBTN

#endif
