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

#include "tkbio_layout_default.h"

const unsigned char tkbio_colors[][4] =
    {
        {0x00, 0xFF, 0x00, 0x00},   // PRIMARY
        {0x00, 0x88, 0x00, 0x00},   // SHIFT
        {0x00, 0x00, 0xFF, 0x00},   // NUM
        {0x00, 0x00, 0x88, 0x00},   // SNUM
        {0xFF, 0x88, 0x00, 0x00},   // META
        {0xDE, 0xB8, 0x87, 0x00},   // GER
        {0x88, 0x88, 0x88, 0x00},   // FK
        {0x00, 0xCC, 0xCC, 0x00},   // ADMIN
        
        {0x00, 0x00, 0x00, 0x00},   // BLACK
        {0xFF, 0x00, 0x00, 0x00},   // HOLD
        {0xFF, 0xFF, 0x00, 0x00}    // TOGGLE
    };

const unsigned char admin_colors[][4] =
    {
        {0x00, 0x00, 0x00, 0x00},   // BG
        {0xFF, 0xFF, 0x00, 0x00},   // SWITCH
        {0xFF, 0x00, 0x00, 0x00},   // QUIT
        {0x00, 0xFF, 0x00, 0x00},   // ACTIVATE
        {0x00, 0x00, 0xFF, 0x00}    // MENU
    };

#define VALUE(V)             { .i = V }
#define ONE(C)               { .c = {{C,  0,  0,  0}} }
#define TWO(C1, C2)          { .c = {{C1, C2, 0,  0}} }
#define THREE(C1, C2, C3)    { .c = {{C1, C2, C3, 0}} }
#define FOUR(C1, C2, C3, C4) { .c = {{C1, C2, C3, C4}} }

#define COLOR(Color)    (Color << 4 | TKBIO_COLOR_BLACK)
#define ADCOLOR(Color)  (Color << 4 | ADMIN_COLOR_BG)
#define DEFAULT_OPTIONS TKBIO_LAYOUT_OPTION_BORDER|TKBIO_LAYOUT_OPTION_COPY

#define CHAR(N, C, Color)  {N, TKBIO_LAYOUT_TYPE_CHAR|DEFAULT_OPTIONS, 0, C, COLOR(Color), 0}
#define LCHAR(N, C, Color) {N, TKBIO_LAYOUT_TYPE_CHAR|DEFAULT_OPTIONS, 0, C, COLOR(Color), TKBIO_LAYOUT_CONNECT_LEFT}
#define GOTO(N, T)         {N, TKBIO_LAYOUT_TYPE_GOTO|DEFAULT_OPTIONS, 0, VALUE(TKBIO_TYPE_ ## T), COLOR(TKBIO_TYPE_ ## T), 0}
#define TOGG(N, T)         {N, TKBIO_LAYOUT_TYPE_TOGGLE|DEFAULT_OPTIONS, 0, VALUE(T), COLOR(TKBIO_COLOR_TOGGLE), 0}
#define HOLD               {"Hold", TKBIO_LAYOUT_TYPE_HOLD|DEFAULT_OPTIONS, 0, VALUE(0), COLOR(TKBIO_COLOR_HOLD), 0}
#define NOP                {0, TKBIO_LAYOUT_TYPE_NOP, 0, VALUE(0), 0, 0}
#define SYS(N, V, Color)   {N, TKBIO_LAYOUT_TYPE_SYSTEM|DEFAULT_OPTIONS, 0, VALUE(V), ADCOLOR(Color), 0}
#define USYS(N, V, Color)  {N, TKBIO_LAYOUT_TYPE_SYSTEM|DEFAULT_OPTIONS, 0, VALUE(V), ADCOLOR(Color), TKBIO_LAYOUT_CONNECT_UP}

#define PRIM(C)             CHAR(0, ONE(C), TKBIO_TYPE_PRIMARY)
#define NPRIM(N, C)         CHAR(N, ONE(C), TKBIO_TYPE_PRIMARY)
#define SHIFT(C)            CHAR(0, ONE(C), TKBIO_TYPE_SHIFT)
#define NSHIFT(N, C)        CHAR(N, ONE(C), TKBIO_TYPE_SHIFT)
#define NUM(C)              CHAR(0, ONE(C), TKBIO_TYPE_NUM)
#define SNUM(C1, C2)        CHAR(0, TWO(C1, C2), TKBIO_TYPE_SNUM)
#define META1(N, C1)        CHAR(N, ONE(C1), TKBIO_TYPE_META)
#define LMETA1(N, C1)       LCHAR(N, ONE(C1), TKBIO_TYPE_META)
#define META2(N, C1, C2)    CHAR(N, FOUR(27, '[', C1, C2), TKBIO_TYPE_META)
#define GER1(C1)            CHAR(0, ONE(C1), TKBIO_TYPE_GER)
#define GER2(N, C1, C2)     CHAR(N, TWO(C1, C2), TKBIO_TYPE_GER)
#define GER3(N, C1, C2, C3) CHAR(N, THREE(C1, C2, C3), TKBIO_TYPE_GER)
#define FK(N, C1, C2)       CHAR(N, FOUR(27, '[', C1, C2), TKBIO_TYPE_FK)
#define ADMIN(N, T)         SYS(N, TKBIO_SYSTEM_ ## T, ADMIN_COLOR_ ## T)
#define VADMIN(N, V, T)     SYS(N, TKBIO_SYSTEM_ ## V, ADMIN_COLOR_ ## T)
#define UVADMIN(N, V, T)    USYS(N, TKBIO_SYSTEM_ ## V, ADMIN_COLOR_ ## T)

#define ALT     1
#define CTRLL   2
#define SUPER   4

const struct tkbio_mapelem tkbio_map_primary[] =
    {
        PRIM('q'),           PRIM('w'),        PRIM('t'), PRIM('z'), PRIM('o'), PRIM('p'),
        PRIM('a'),           PRIM('e'),        PRIM('r'), PRIM('u'), PRIM('i'), PRIM('l'),
        PRIM('d'),           PRIM('f'),        PRIM('g'), PRIM('h'), PRIM('j'), PRIM('k'),
        GOTO("Shft", SHIFT), PRIM('y'),        PRIM('s'), PRIM('b'), PRIM('n'), PRIM('m'),
        GOTO("Meta", META),  GOTO("Num", NUM), PRIM('x'), PRIM('c'), PRIM('v'), NPRIM("BkSp", '\b')
    };

const struct tkbio_mapelem tkbio_map_shift[] =
    {
        SHIFT('Q'), SHIFT('W'), SHIFT('T'), SHIFT('Z'), SHIFT('O'), SHIFT('P'),
        SHIFT('A'), SHIFT('E'), SHIFT('R'), SHIFT('U'), SHIFT('I'), SHIFT('L'),
        SHIFT('D'), SHIFT('F'), SHIFT('G'), SHIFT('H'), SHIFT('J'), SHIFT('K'),
        NOP,        SHIFT('Y'), SHIFT('S'), SHIFT('B'), SHIFT('N'), SHIFT('M'),
        HOLD,       NOP,        SHIFT('X'), SHIFT('C'), SHIFT('V'), NSHIFT("BkSp", '\b')
    };

const struct tkbio_mapelem tkbio_map_num[] =
    {
        NUM('1'),           NUM('2'),  NUM('3'), NUM('4'), NUM('5'),  NUM('*'),
        NUM('6'),           NUM('7'),  NUM('8'), NUM('9'), NUM('0'),  NUM('/'),
        NUM('@'),           NUM('\''), NUM('|'), NUM('?'), NUM('\\'), NUM('+'),
        GOTO("SNum", SNUM), NUM('#'),  NUM('>'), NUM(';'), NUM(':'),  NUM('-'),
        HOLD,               NUM('~'),  NUM('<'), NUM(','), NUM('.'),  NUM('_')
    };

const struct tkbio_mapelem tkbio_map_snum[] =
    {
        SNUM('!', 0), SNUM('"', 0), SNUM(-62, -89), SNUM('$', 0), SNUM('%', 0), NOP,
        SNUM('&', 0), SNUM('/', 0), SNUM('(', 0),   SNUM(')', 0), SNUM('=', 0), NOP,
        NOP,          NOP,          NOP,            NOP,          NOP,          NOP,
        NOP,          NOP,          NOP,            NOP,          NOP,          NOP,
        HOLD,         NOP,          NOP,            NOP,          NOP,          NOP
    };

const struct tkbio_mapelem tkbio_map_meta[] =
    {
        META1("Esc", 27),    NOP,                 NOP,                NOP,                     NOP,                    META2("PgUp", '6', '~'),
        META1("Tab", '\t'),  GOTO("FK", FK),      NOP,                NOP,                     META2("Up", 'A', 0),    META2("PgDn", '5', '~'),
        TOGG("Supr", SUPER), GOTO("Ger", GER),    NOP,                META2("Left", 'D', 0),   META2("Down", 'B', 0),  META2("Rght", 'C', 0),
        TOGG("Ctrl", CTRLL), GOTO("Admn", ADMIN), NOP,                META2("Home", '1', '~'), META2("Ins", '2', '~'), META2("End", '4', '~'),
        HOLD,                TOGG("Alt", ALT),    META1("Spce", ' '), LMETA1("Spce", ' '),     META2("Del", '3', '~'), META1("Entr", '\n')
    };

const struct tkbio_mapelem tkbio_map_ger[] =
    {
        GER2("ä", -61, -92),       GER2("ö", -61, -74),  GER2("ü", -61, -68),  GER2("ß", -61, -97), NOP,                 NOP,
        GER2("Ä", -61, -124),      GER2("Ö", -61, -106), GER2("Ü", -61, -100), NOP,                 NOP,                 NOP,
        GER3("€", -30, -126, -84), GER2("µ", -62, -75),  GER1('^'),            GER2("°", -62, -80), GER2("´", -62, -76), GER1('`'),
        NOP,                       NOP,                  NOP,                  NOP,                 NOP,                 NOP,
        HOLD,                      NOP,                  NOP,                  NOP,                 NOP,                 NOP
    };

const struct tkbio_mapelem tkbio_map_fk[] =
    {
        FK("F1", 'P', 0),   FK("F2", 'Q', 0),   FK("F3", 'R', 0),  FK("F4", 'S', 0),  FK("F5", 15, '~'),  NOP,
        FK("F6", 17, '~'),  FK("F7", 18, '~'),  FK("F8", 19, '~'), FK("F9", 20, '~'), FK("F10", 21, '~'), NOP,
        FK("F11", 23, '~'), FK("F12", 24, '~'), NOP,               NOP,               NOP,                NOP,
        NOP,                NOP,                NOP,               NOP,               NOP,                NOP,
        HOLD,               NOP,                NOP,               NOP,               NOP,                NOP
    };

const struct tkbio_mapelem admin_map[] =
    {
        VADMIN("prev", PREV, SWITCH),  ADMIN("quit", QUIT),     VADMIN("next", NEXT, SWITCH),
        UVADMIN("prev", PREV, SWITCH), ADMIN("actv", ACTIVATE), UVADMIN("next", NEXT, SWITCH),
        UVADMIN("prev", PREV, SWITCH), ADMIN("menu", MENU),     UVADMIN("next", NEXT, SWITCH)
    };


const struct tkbio_map tkbio_maps[] =
    {
        TKBIO_MAPS(0)
    };

const struct tkbio_map admin_maps[] =
    {
        ADMIN_MAP(0)
    };

union tkbio_elem tkbio_parse(int map, union tkbio_elem elem, unsigned char toggle)
{
    if(toggle & CTRLL && !(toggle & ~CTRLL))
    {
        switch(map)
        {
            case TKBIO_TYPE_PRIMARY:
                if(elem.c.c[0] != '\b')
                    elem.c.c[0] -= 96;
                break;
            case TKBIO_TYPE_SHIFT:
                if(elem.c.c[0] != '\b')
                    elem.c.c[0] -= 64;
                break;
        }
    }
    if(toggle & ALT && !(toggle & ~ALT))
    {
        switch(map)
        {
            case TKBIO_TYPE_PRIMARY:
            case TKBIO_TYPE_SHIFT:
                if(elem.c.c[0] != '\b')
                {
                    elem.c.c[2] = elem.c.c[0];
                    elem.c.c[0] = 27;
                    elem.c.c[1] = '[';
                }
                break;
        }
    }
    return elem;
}

const struct tkbio_layout tkbLayoutDefault =
    {
        .start  = TKBIO_TYPE_PRIMARY,
        .size   = TKBIO_MAPS_SIZE,
        .maps   = tkbio_maps,
        .fun    = tkbio_parse
    };

const struct tkbio_layout adminLayout =
    {
        .start  = 0,
        .size   = sizeof(admin_maps)/sizeof(struct tkbio_map),
        .maps   = admin_maps,
        .fun    = 0
    };
