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

#include "neobox_def.h"
#include "neobox_layout_default.h"

const unsigned char neobox_colors[][4] =
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
#define ONE(C)               { .c = { .u = {C,  0,  0,  0} } }
#define TWO(C1, C2)          { .c = { .u = {C1, C2, 0,  0} } }
#define THREE(C1, C2, C3)    { .c = { .u = {C1, C2, C3, 0} } }
#define FOUR(C1, C2, C3, C4) { .c = { .u = {C1, C2, C3, C4} } }

#define COLOR(Color)    Color,NEOBOX_COLOR_BLACK,Color
#define ADCOLOR(Color)  Color,ADMIN_COLOR_BG,Color
#define DEFAULT_OPTIONS NEOBOX_LAYOUT_OPTION_BORDER|NEOBOX_LAYOUT_OPTION_COPY

#define CHAR(N, C, Color)  {N, NEOBOX_LAYOUT_TYPE_CHAR, 0, C, COLOR(Color), DEFAULT_OPTIONS}
#define LCHAR(N, C, Color) {N, NEOBOX_LAYOUT_TYPE_CHAR, 0, C, COLOR(Color), DEFAULT_OPTIONS|NEOBOX_LAYOUT_OPTION_CONNECT_LEFT}
#define GOTO(N, T)         {N, NEOBOX_LAYOUT_TYPE_GOTO, 0, VALUE(NEOBOX_TYPE_ ## T), COLOR(NEOBOX_TYPE_ ## T), DEFAULT_OPTIONS}
#define TOGG(N, T)         {N, NEOBOX_LAYOUT_TYPE_TOGGLE, 0, VALUE(T), COLOR(NEOBOX_COLOR_TOGGLE), DEFAULT_OPTIONS}
#define HOLD               {"Hold", NEOBOX_LAYOUT_TYPE_HOLD, 0, VALUE(0), COLOR(NEOBOX_COLOR_HOLD), DEFAULT_OPTIONS}
#define NOP                {0, NEOBOX_LAYOUT_TYPE_NOP, 0, VALUE(0), 0, 0}
#define SYS(N, V, Color)   {N, NEOBOX_LAYOUT_TYPE_SYSTEM, 0, VALUE(V), ADCOLOR(Color), DEFAULT_OPTIONS}
#define USYS(N, V, Color)  {N, NEOBOX_LAYOUT_TYPE_SYSTEM, 0, VALUE(V), ADCOLOR(Color), DEFAULT_OPTIONS|NEOBOX_LAYOUT_OPTION_CONNECT_UP}

#define PRIM(C)             CHAR(0, ONE(C), NEOBOX_TYPE_PRIMARY)
#define NPRIM(N, C)         CHAR(N, ONE(C), NEOBOX_TYPE_PRIMARY)
#define SHIFT(C)            CHAR(0, ONE(C), NEOBOX_TYPE_SHIFT)
#define NSHIFT(N, C)        CHAR(N, ONE(C), NEOBOX_TYPE_SHIFT)
#define NUM(C)              CHAR(0, ONE(C), NEOBOX_TYPE_NUM)
#define SNUM1(C1)           CHAR(0, ONE(C1), NEOBOX_TYPE_SNUM)
#define SNUM2(N, C1, C2)    CHAR(N, TWO(C1, C2), NEOBOX_TYPE_SNUM)
#define META1(N, C1)        CHAR(N, ONE(C1), NEOBOX_TYPE_META)
#define LMETA1(N, C1)       LCHAR(N, ONE(C1), NEOBOX_TYPE_META)
#define META2(N, C1)        CHAR(N, THREE(27, '[', C1), NEOBOX_TYPE_META)
#define META3(N, C1)        CHAR(N, FOUR(27, '[', C1, '~'), NEOBOX_TYPE_META)
#define GER1(C1)            CHAR(0, ONE(C1), NEOBOX_TYPE_GER)
#define GER2(N, C1, C2)     CHAR(N, TWO(C1, C2), NEOBOX_TYPE_GER)
#define GER3(N, C1, C2, C3) CHAR(N, THREE(C1, C2, C3), NEOBOX_TYPE_GER)
#define FK1(N, C1)          CHAR(N, THREE(27, 'O', C1), NEOBOX_TYPE_FK)
#define FK2(N, C1, C2)      CHAR(N, FOUR(0, C1, C2, '~'), NEOBOX_TYPE_FK) // 0 hack
#define ADMIN(N, T)         SYS(N, NEOBOX_SYSTEM_ ## T, ADMIN_COLOR_ ## T)
#define VADMIN(N, V, T)     SYS(N, NEOBOX_SYSTEM_ ## V, ADMIN_COLOR_ ## T)
#define UVADMIN(N, V, T)    USYS(N, NEOBOX_SYSTEM_ ## V, ADMIN_COLOR_ ## T)

#define ALT     1
#define CTRLL   2
#define SUPER   4

const struct neobox_mapelem neobox_map_primary[] =
    {
        PRIM('q'),           PRIM('w'),        PRIM('t'), PRIM('z'), PRIM('o'), PRIM('p'),
        PRIM('a'),           PRIM('e'),        PRIM('r'), PRIM('u'), PRIM('i'), PRIM('l'),
        PRIM('d'),           PRIM('f'),        PRIM('g'), PRIM('h'), PRIM('j'), PRIM('k'),
        GOTO("Shft", SHIFT), PRIM('y'),        PRIM('s'), PRIM('b'), PRIM('n'), PRIM('m'),
        GOTO("Meta", META),  GOTO("Num", NUM), PRIM('x'), PRIM('c'), PRIM('v'), NPRIM("BkSp", '\b')
    };

const struct neobox_mapelem neobox_map_shift[] =
    {
        SHIFT('Q'), SHIFT('W'), SHIFT('T'), SHIFT('Z'), SHIFT('O'), SHIFT('P'),
        SHIFT('A'), SHIFT('E'), SHIFT('R'), SHIFT('U'), SHIFT('I'), SHIFT('L'),
        SHIFT('D'), SHIFT('F'), SHIFT('G'), SHIFT('H'), SHIFT('J'), SHIFT('K'),
        NOP,        SHIFT('Y'), SHIFT('S'), SHIFT('B'), SHIFT('N'), SHIFT('M'),
        HOLD,       NOP,        SHIFT('X'), SHIFT('C'), SHIFT('V'), NSHIFT("BkSp", '\b')
    };

const struct neobox_mapelem neobox_map_num[] =
    {
        NUM('1'),           NUM('2'),  NUM('3'), NUM('4'), NUM('5'),  NUM('*'),
        NUM('6'),           NUM('7'),  NUM('8'), NUM('9'), NUM('0'),  NUM('/'),
        NUM('@'),           NUM('\''), NUM('|'), NUM('?'), NUM('\\'), NUM('+'),
        GOTO("SNum", SNUM), NUM('#'),  NUM('>'), NUM(';'), NUM(':'),  NUM('-'),
        HOLD,               NUM('~'),  NUM('<'), NUM(','), NUM('.'),  NUM('_')
    };

const struct neobox_mapelem neobox_map_snum[] =
    {
        SNUM1('!'), SNUM1('"'), SNUM2("sect", 194, 167), SNUM1('$'), SNUM1('%'), NOP,
        SNUM1('&'), SNUM1('/'), SNUM1('('),              SNUM1(')'), SNUM1('='), NOP,
        NOP,        NOP,        NOP,                     NOP,        NOP,        NOP,
        NOP,        NOP,        NOP,                     NOP,        NOP,        NOP,
        HOLD,       NOP,        NOP,                     NOP,        NOP,        NOP
    };

const struct neobox_mapelem neobox_map_meta[] =
    {
        META1("Esc", 27),    NOP,                 NOP,                NOP,                 NOP,                META3("PgUp", '5'),
        META1("Tab", '\t'),  GOTO("FK", FK),      NOP,                NOP,                 META2("Up", 'A'),   META3("PgDn", '6'),
        TOGG("Supr", SUPER), GOTO("Ger", GER),    NOP,                META2("Left", 'D'),  META2("Down", 'B'), META2("Rght", 'C'),
        TOGG("Ctrl", CTRLL), GOTO("Admn", ADMIN), NOP,                META3("Home", '1'),  META3("Ins", '2'),  META3("End", '4'),
        HOLD,                TOGG("Alt", ALT),    META1("Spce", ' '), LMETA1("Spce", ' '), META3("Del", '3'),  META1("Entr", '\n')
    };

const struct neobox_mapelem neobox_map_ger[] =
    {
        GER2("ae", 195, 164),        GER2("oe", 195, 182),   GER2("ue", 195, 188), GER2("sz", 195, 159),   NOP,                    NOP,
        GER2("Ae", 195, 132),        GER2("Oe", 105, 150),   GER2("Ue", 195, 156), NOP,                    NOP,                    NOP,
        GER3("euro", 226, 130, 172), GER2("mcro", 194, 181), GER1('^'),            GER2("ring", 194, 176), GER2("akut", 194, 180), GER1('`'),
        NOP,                         NOP,                    NOP,                  NOP,                    NOP,                    NOP,
        HOLD,                        NOP,                    NOP,                  NOP,                    NOP,                    NOP
    };

const struct neobox_mapelem neobox_map_fk[] =
    {
        FK1("F1", 'P'),       FK1("F2", 'Q'),       FK1("F3", 'R'),      FK1("F4", 'S'),      FK2("F5", '1', '5'),  NOP,
        FK2("F6", '1', '7'),  FK2("F7", '1', '8'),  FK2("F8", '1', '9'), FK2("F9", '2', '0'), FK2("F10", '2', '1'), NOP,
        FK2("F11", '2', '3'), FK2("F12", '2', '4'), NOP,                 NOP,                 NOP,                  NOP,
        NOP,                  NOP,                  NOP,                 NOP,                 NOP,                  NOP,
        HOLD,                 NOP,                  NOP,                 NOP,                 NOP,                  NOP
    };

const struct neobox_mapelem admin_map[] =
    {
        VADMIN("prev", PREV, SWITCH),  ADMIN("quit", QUIT),     VADMIN("next", NEXT, SWITCH),
        UVADMIN("prev", PREV, SWITCH), ADMIN("actv", ACTIVATE), UVADMIN("next", NEXT, SWITCH),
        UVADMIN("prev", PREV, SWITCH), ADMIN("menu", MENU),     UVADMIN("next", NEXT, SWITCH)
    };


const struct neobox_map neobox_maps[] =
    {
        NEOBOX_MAPS(0)
    };

const struct neobox_map admin_maps[] =
    {
        ADMIN_MAP(0)
    };

union neobox_elem neobox_parse(int map, union neobox_elem elem, unsigned char toggle)
{
    if(toggle & CTRLL && !(toggle & ~CTRLL))
    {
        switch(map)
        {
            case NEOBOX_TYPE_PRIMARY:
                if(elem.c.c[0] != '\b')
                    elem.c.c[0] -= 96;
                break;
            case NEOBOX_TYPE_SHIFT:
                if(elem.c.c[0] != '\b')
                    elem.c.c[0] -= 64;
                break;
        }
    }
    if(toggle & ALT && !(toggle & ~ALT))
    {
        switch(map)
        {
            case NEOBOX_TYPE_PRIMARY:
            case NEOBOX_TYPE_SHIFT:
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

const struct neobox_layout tkbLayoutDefault =
    {
        .start  = NEOBOX_TYPE_PRIMARY,
        .size   = NEOBOX_MAPS_SIZE,
        .maps   = neobox_maps,
        .fun    = neobox_parse
    };

const struct neobox_layout adminLayout =
    {
        .start  = 0,
        .size   = sizeof(admin_maps)/sizeof(struct neobox_map),
        .maps   = admin_maps,
        .fun    = 0
    };
