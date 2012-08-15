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

#ifndef __TKBIO_LAYOUT_DEFAULT_H__
#define __TKBIO_LAYOUT_DEFAULT_H__

#include "tkbio_layout.h"

const char tkbio_colors[10][4] =
    {
        {0x00, 0xFF, 0x00, 0x00},   // PRIMARY
        {0x00, 0x88, 0x00, 0x00},   // SHIFT
        {0xFF, 0x00, 0x00, 0x00},   // NUM
        {0x88, 0x00, 0x00, 0x00},   // SNUM
        {0x00, 0x88, 0xFF, 0x00},   // META
        {0xFF, 0xFF, 0x00, 0x00},   // GER
        {0xFF, 0x88, 0x00, 0x00},   // FK
        
        {0x00, 0x00, 0x00, 0x00},   // BLACK
        {0x00, 0x00, 0xFF, 0x00},   // HOLD
        {0x00, 0xFF, 0xFF, 0x00}    // TOGGLE
    };

#define TKBIO_TYPE_PRIMARY    0
#define TKBIO_TYPE_SHIFT      1
#define TKBIO_TYPE_NUM        2
#define TKBIO_TYPE_SNUM       3
#define TKBIO_TYPE_META       4
#define TKBIO_TYPE_GER        5
#define TKBIO_TYPE_FK         6

#define TKBIO_COLOR_BLACK     7
#define TKBIO_COLOR_HOLD      8
#define TKBIO_COLOR_TOGGLE    9

#define ONE(C)               {{C,  0,  0,  0}}
#define TWO(C1, C2)          {{C1, C2, 0,  0}}
#define THREE(C1, C2, C3)    {{C1, C2, C3, 0}}
#define FOUR(C1, C2, C3, C4) {{C1, C2, C3, C4}}

#define COLOR(Color)    (Color << 4 | TKBIO_COLOR_BLACK)
#define DEFAULT_OPTIONS TKBIO_LAYOUT_OPTION_BORDER|TKBIO_LAYOUT_OPTION_COPY

#define CHAR(C, Color)  {TKBIO_LAYOUT_TYPE_CHAR|DEFAULT_OPTIONS, C, COLOR(Color), 0}
#define GOTO(T)         {TKBIO_LAYOUT_TYPE_GOTO|DEFAULT_OPTIONS, ONE(TKBIO_TYPE_ ## T), COLOR(TKBIO_TYPE_ ## T), 0}
#define TOGG(T)         {TKBIO_LAYOUT_TYPE_TOGG|DEFAULT_OPTIONS, ONE(T), COLOR(TKBIO_COLOR_TOGGLE), 0}
#define HOLD            {TKBIO_LAYOUT_TYPE_HOLD|DEFAULT_OPTIONS, ONE(0), COLOR(TKBIO_COLOR_HOLD), 0}
#define NOP             {TKBIO_LAYOUT_TYPE_CHAR, ONE(0), 0, 0}

#define PRIM(C)          CHAR(ONE(C), TKBIO_TYPE_PRIMARY)
#define SHIFT(C)         CHAR(ONE(C), TKBIO_TYPE_SHIFT)
#define NUM(C)           CHAR(ONE(C), TKBIO_TYPE_NUM)
#define SNUM(C1, C2)     CHAR(TWO(C1, C2), TKBIO_TYPE_SNUM)
#define META1(C1)        CHAR(ONE(C1), TKBIO_TYPE_META)
#define META2(C1, C2)    CHAR(FOUR(27, '[', C1, C2), TKBIO_TYPE_META)
#define GER1(C1)         CHAR(ONE(C1), TKBIO_TYPE_GER)
#define GER2(C1, C2)     CHAR(TWO(C1, C2), TKBIO_TYPE_GER)
#define GER3(C1, C2, C3) CHAR(THREE(C1, C2, C3), TKBIO_TYPE_GER)

#define ALT     1
#define CTRLL   2
#define SUPER   4


const struct tkbio_mapelem tkbio_map_primary[5*6] =
    {
        PRIM('q'),   PRIM('w'),  PRIM('t'), PRIM('z'), PRIM('o'), PRIM('p'),
        PRIM('a'),   PRIM('e'),  PRIM('r'), PRIM('u'), PRIM('i'), PRIM('l'),
        PRIM('d'),   PRIM('f'),  PRIM('g'), PRIM('h'), PRIM('j'), PRIM('k'),
        GOTO(SHIFT), PRIM('y'),  PRIM('s'), PRIM('b'), PRIM('n'), PRIM('m'),
        GOTO(META),  GOTO(NUM),  PRIM('x'), PRIM('c'), PRIM('v'), PRIM('\b')
    };

const struct tkbio_mapelem tkbio_map_shift[5*6] =
    {
        SHIFT('Q'), SHIFT('W'), SHIFT('T'), SHIFT('Z'), SHIFT('O'), SHIFT('P'),
        SHIFT('A'), SHIFT('E'), SHIFT('R'), SHIFT('U'), SHIFT('I'), SHIFT('L'),
        SHIFT('D'), SHIFT('F'), SHIFT('G'), SHIFT('H'), SHIFT('J'), SHIFT('K'),
        HOLD,       SHIFT('Y'), SHIFT('S'), SHIFT('B'), SHIFT('N'), SHIFT('M'),
        NOP,        NOP,        SHIFT('X'), SHIFT('C'), SHIFT('V'), NOP
    };

const struct tkbio_mapelem tkbio_map_num[5*6] =
    {
        NUM('1'),   NUM('2'),  NUM('3'), NUM('4'), NUM('5'),  NUM('*'),
        NUM('6'),   NUM('7'),  NUM('8'), NUM('9'), NUM('0'),  NUM('/'),
        NUM('@'),   NUM('\''), NUM('|'), NUM('?'), NUM('\\'), NUM('+'),
        GOTO(SNUM), NUM('#'),  NUM('>'), NUM(';'), NUM(':'),  NUM('-'),
        NUM('~'),   HOLD,      NUM('<'), NUM(','), NUM('.'),  NUM('_')
    };

const struct tkbio_mapelem tkbio_map_snum[5*6] =
    {
        SNUM('!', 0), SNUM('"', 0), SNUM(-62, -89), SNUM('$', 0), SNUM('%', 0), NOP,
        SNUM('&', 0), SNUM('/', 0), SNUM('(', 0),   SNUM(')', 0), SNUM('=', 0), NOP,
        NOP,          NOP,          NOP,            NOP,          NOP,          NOP,
        HOLD,         NOP,          NOP,            NOP,          NOP,          NOP,
        NOP,          NOP,          NOP,            NOP,          NOP,          NOP
    };

const struct tkbio_mapelem tkbio_map_meta[5*6] =
    {
        META1(27),   NOP,       NOP,        NOP,             NOP,             META2('6', '~'),
        META1('\t'), NOP,       NOP,        NOP,             META2('A', 0),   META2('5', '~'),
        TOGG(SUPER), GOTO(FK),  NOP,        META2('D', 0),   META2('B', 0),   META2('C', 0),
        TOGG(CTRLL), GOTO(GER), NOP,        META2('1', '~'), META2('2', '~'), META2('4', '~'),
        HOLD,        TOGG(ALT), META1(' '), META1(' '),      META2('3', '~'), META1('\n')
    };

const struct tkbio_mapelem tkbio_map_ger[5*6] =
    {
        GER2(-61, -92),       GER2(-61, -74),  GER2(-61, -68),  GER2(-61, -97), NOP,            NOP,
        GER2(-61, -124),      GER2(-61, -106), GER2(-61, -100), NOP,            NOP,            NOP,
        GER3(-30, -126, -84), GER2(-62, -75),  GER1('^'),       GER2(-62, -80), GER2(-62, -76), GER1('`'),
        NOP,                  HOLD,            NOP,             NOP,            NOP,            NOP,
        NOP,                  NOP,             NOP,             NOP,            NOP,            NOP
    };

const struct tkbio_mapelem tkbio_map_fk[5*6] =
    {
        NOP, NOP,  NOP, NOP, NOP, NOP,
        NOP, NOP,  NOP, NOP, NOP, NOP,
        NOP, HOLD, NOP, NOP, NOP, NOP,
        NOP, NOP,  NOP, NOP, NOP, NOP,
        NOP, NOP,  NOP, NOP, NOP, NOP
    };


const struct tkbio_map tkbio_maps[7] =
    {
        {5, 6, tkbio_map_primary},
        {5, 6, tkbio_map_shift},
        {5, 6, tkbio_map_num},
        {5, 6, tkbio_map_snum},
        {5, 6, tkbio_map_meta},
        {5, 6, tkbio_map_ger},
        {5, 6, tkbio_map_fk}
    };

struct tkbio_charelem tkbio_parse(int map, struct tkbio_charelem elem, char toggle)
{
    if(toggle & CTRLL && !(toggle & ~CTRLL))
    {
        switch(map)
        {
            case TKBIO_TYPE_PRIMARY:
                if(elem.c[0] != '\b')
                    elem.c[0] -= 96;
                break;
            case TKBIO_TYPE_SHIFT:
                elem.c[0] -= 64;
                break;
        }
    }
    if(toggle & ALT && !(toggle & ~ALT))
    {
        switch(map)
        {
            case TKBIO_TYPE_PRIMARY:
            case TKBIO_TYPE_SHIFT:
                if(elem.c[0] != '\b')
                {
                    elem.c[2] = elem.c[0];
                    elem.c[0] = 27;
                    elem.c[1] = '[';
                }
                break;
        }
    }
    return elem;
}

struct tkbio_layout tkbLayoutDefault =
    {
        .start  = TKBIO_TYPE_PRIMARY,
        .maps   = tkbio_maps,
        .colors = tkbio_colors,
        .fun    = tkbio_parse
    };

#undef ONE
#undef TWO
#undef THREE
#undef FOUR

#undef COLOR
#undef DEFAULT_OPTIONS

#undef CHAR
#undef GOTO
#undef TOGG
#undef HOLD
#undef NOP

#undef PRIM
#undef SHIFT
#undef NUM
#undef SNUM
#undef META1
#undef META2
#undef GER1
#undef GER2
#undef GER3

#undef ALT
#undef CTRLL
#undef SUPER

#endif

