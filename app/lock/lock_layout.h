/*
 * Copyright (c) 2016 Martin Rödel aka Yomin
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

#ifndef __LOCK_LAYOUT_H__
#define __LOCK_LAYOUT_H__

#include "neobox_layout_default.h"

const unsigned char lock_colors[][4] =
    {
        {0x00, 0x00, 0x00, 0x00}, // bg
        {0x00, 0xFF, 0xFF, 0x00}, // fg
        {0xFF, 0xFF, 0x00, 0x00}  // text
    };

#define COLOR_BG    0
#define COLOR_FG    1
#define COLOR_TEXT  2

#define ID_PASS     1
#define ID_SHOW     1

#define ONE(C)          { .c = {{C, 0, 0, 0}} }
#define COLOR(C)        C,COLOR_BG,C
#define VALUE(V)        { .i = V }
#define DEFAULT_OPTIONS NEOBOX_LAYOUT_OPTION_BORDER|NEOBOX_LAYOUT_OPTION_ALIGN_CENTER
#define LEFT            NEOBOX_LAYOUT_OPTION_CONNECT_LEFT

#define NOP             {0, NEOBOX_LAYOUT_TYPE_NOP, 0, ONE(0), 0, 0, 0, 0}
#define BTN(N, I, C, O) {N, NEOBOX_LAYOUT_TYPE_CHAR, I, ONE(C), COLOR(COLOR_FG), DEFAULT_OPTIONS|O}
#define TEXT(N, I, O)   {N, NEOBOX_LAYOUT_TYPE_TEXT, I, VALUE(1), COLOR(COLOR_TEXT), DEFAULT_OPTIONS|O}

#define PASS(O)     TEXT("Password", ID_PASS, NEOBOX_LAYOUT_OPTION_PASSWORD|O)
#define UNLOCK(O)   BTN("unlock", 0, 'u', O)
#define SHOW(O)     BTN("show", ID_SHOW, 's', O)

const struct neobox_mapelem lock_map[] =
    {
        NOP,     NOP,        NOP,        NOP,        NOP,        NOP,
        NOP,     PASS(0),    PASS(LEFT), PASS(LEFT), PASS(LEFT), NOP,
        NOP,     NOP,        NOP,        NOP,        NOP,        NOP,
        SHOW(0), SHOW(LEFT), NOP,        NOP,        UNLOCK(0),  UNLOCK(LEFT)
    };

const struct neobox_map lock_maps[] =
    {
        {4, 6, lock_map, lock_colors},
        NEOBOX_MAPS(1)
    };

const struct neobox_layout lockLayout =
    {
        .start  = 0,
        .size   = sizeof(lock_maps)/sizeof(struct neobox_map),
        .maps   = lock_maps,
        .fun    = 0
    };

#undef COLOR_BG
#undef COLOR_FG
#undef COLOR_TEXT
#undef ONE
#undef COLOR
#undef VALUE
#undef DEFAULT_OPTIONS
#undef LEFT
#undef NOP
#undef TEXT
#undef BTN
#undef PASS
#undef UNLOCK

#endif
