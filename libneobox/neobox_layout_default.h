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

#ifndef __NEOBOX_LAYOUT_DEFAULT_H__
#define __NEOBOX_LAYOUT_DEFAULT_H__

#include "neobox_layout.h"

#define NEOBOX_TYPE_PRIMARY    0
#define NEOBOX_TYPE_SHIFT      1
#define NEOBOX_TYPE_NUM        2
#define NEOBOX_TYPE_SNUM       3
#define NEOBOX_TYPE_META       4
#define NEOBOX_TYPE_GER        5
#define NEOBOX_TYPE_FK         6
#define NEOBOX_TYPE_ADMIN      7

#define NEOBOX_COLOR_BLACK     8
#define NEOBOX_COLOR_HOLD      9
#define NEOBOX_COLOR_TOGGLE    10

#define ADMIN_COLOR_BG        0
#define ADMIN_COLOR_SWITCH    1
#define ADMIN_COLOR_QUIT      2
#define ADMIN_COLOR_ACTIVATE  3
#define ADMIN_COLOR_MENU      4

#define NEOBOX_MAP(N, O)      {5, 6, neobox_map_ ## N, neobox_colors, O}
#define NEOBOX_MAP_PRIMARY(O) NEOBOX_MAP(primary, O)
#define NEOBOX_MAP_SHIFT(O)   NEOBOX_MAP(shift, O)
#define NEOBOX_MAP_NUM(O)     NEOBOX_MAP(num, O)
#define NEOBOX_MAP_SNUM(O)    NEOBOX_MAP(snum, O)
#define NEOBOX_MAP_META(O)    NEOBOX_MAP(meta, O)
#define NEOBOX_MAP_GER(O)     NEOBOX_MAP(ger, O)
#define NEOBOX_MAP_FK(O)      NEOBOX_MAP(fk, O)
#define ADMIN_MAP(O)         {3, 3, admin_map, admin_colors, O}

#define NEOBOX_MAPS(O) \
    NEOBOX_MAP_PRIMARY(O), NEOBOX_MAP_SHIFT(O), NEOBOX_MAP_NUM(O), \
    NEOBOX_MAP_SNUM(O), NEOBOX_MAP_META(O), NEOBOX_MAP_GER(O), \
    NEOBOX_MAP_FK(O), ADMIN_MAP(O)
#define NEOBOX_MAPS_SIZE (sizeof(neobox_maps)/sizeof(struct neobox_map))

extern const unsigned char neobox_colors[][4];
extern const unsigned char admin_colors[][4];

extern const struct neobox_mapelem neobox_map_primary[];
extern const struct neobox_mapelem neobox_map_shift[];
extern const struct neobox_mapelem neobox_map_num[];
extern const struct neobox_mapelem neobox_map_snum[];
extern const struct neobox_mapelem neobox_map_meta[];
extern const struct neobox_mapelem neobox_map_ger[];
extern const struct neobox_mapelem neobox_map_fk[];
extern const struct neobox_mapelem admin_map[];

extern const struct neobox_map neobox_maps[];
extern const struct neobox_map admin_maps[];

union neobox_elem neobox_parse(int map, union neobox_elem elem, unsigned char toggle);

const struct neobox_layout tkbLayoutDefault;
const struct neobox_layout adminLayout;

#endif

