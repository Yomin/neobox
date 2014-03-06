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

#ifndef __TKBIO_LAYOUT_DEFAULT_H__
#define __TKBIO_LAYOUT_DEFAULT_H__

#include "tkbio_layout.h"

#define TKBIO_TYPE_PRIMARY    0
#define TKBIO_TYPE_SHIFT      1
#define TKBIO_TYPE_NUM        2
#define TKBIO_TYPE_SNUM       3
#define TKBIO_TYPE_META       4
#define TKBIO_TYPE_GER        5
#define TKBIO_TYPE_FK         6
#define TKBIO_TYPE_ADMIN      7

#define TKBIO_COLOR_BLACK     8
#define TKBIO_COLOR_HOLD      9
#define TKBIO_COLOR_TOGGLE    10

#define ADMIN_COLOR_BG        0
#define ADMIN_COLOR_SWITCH    1
#define ADMIN_COLOR_QUIT      2
#define ADMIN_COLOR_ACTIVATE  3
#define ADMIN_COLOR_MENU      4

#define TKBIO_MAP(N, O)      {5, 6, tkbio_map_ ## N, tkbio_colors, O}
#define TKBIO_MAP_PRIMARY(O) TKBIO_MAP(primary, O)
#define TKBIO_MAP_SHIFT(O)   TKBIO_MAP(shift, O)
#define TKBIO_MAP_NUM(O)     TKBIO_MAP(num, O)
#define TKBIO_MAP_SNUM(O)    TKBIO_MAP(snum, O)
#define TKBIO_MAP_META(O)    TKBIO_MAP(meta, O)
#define TKBIO_MAP_GER(O)     TKBIO_MAP(ger, O)
#define TKBIO_MAP_FK(O)      TKBIO_MAP(fk, O)
#define ADMIN_MAP(O)         {3, 3, admin_map, admin_colors, O}

#define TKBIO_MAPS(O) \
    TKBIO_MAP_PRIMARY(O), TKBIO_MAP_SHIFT(O), TKBIO_MAP_NUM(O), \
    TKBIO_MAP_SNUM(O), TKBIO_MAP_META(O), TKBIO_MAP_GER(O), \
    TKBIO_MAP_FK(O), ADMIN_MAP(O)
#define TKBIO_MAPS_SIZE (sizeof(tkbio_maps)/sizeof(struct tkbio_map))

extern const unsigned char tkbio_colors[][4];
extern const unsigned char admin_colors[][4];

extern const struct tkbio_mapelem tkbio_map_primary[];
extern const struct tkbio_mapelem tkbio_map_shift[];
extern const struct tkbio_mapelem tkbio_map_num[];
extern const struct tkbio_mapelem tkbio_map_snum[];
extern const struct tkbio_mapelem tkbio_map_meta[];
extern const struct tkbio_mapelem tkbio_map_ger[];
extern const struct tkbio_mapelem tkbio_map_fk[];
extern const struct tkbio_mapelem admin_map[];

extern const struct tkbio_map tkbio_maps[];
extern const struct tkbio_map admin_maps[];

union tkbio_elem tkbio_parse(int map, union tkbio_elem elem, unsigned char toggle);

const struct tkbio_layout tkbLayoutDefault;
const struct tkbio_layout adminLayout;

#endif

