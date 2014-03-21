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

#include <sys/socket.h>

#include <tsp.h>

#include "tkbio_def.h"
#include "tkbio_type_help.h"

#define TYPE(e, t) (((e)->type & TKBIO_LAYOUT_MASK_TYPE) == (t))

extern struct tkbio_global tkbio;

int tkbio_type_help_find_type(int type, int id, int mappos)
{
    const struct tkbio_map *map = &tkbio.layout.maps[mappos];
    const struct tkbio_mapelem *elem;
    int size = map->height*map->width;
    int i;
    
    for(i=0; i<size; i++)
    {
        elem = &map->map[i];
        if(TYPE(elem, type) && elem->id == id)
            return i;
    }
    
    return -1;
}

void tkbio_type_help_set_pos_value(int pos, int mappos, int value, int redraw, tkbio_type_help_set_func f)
{
    const struct tkbio_map *map = &tkbio.layout.maps[mappos];
    const struct tkbio_mapelem *elem;
    struct tkbio_save *save;
    unsigned char sim_tmp = 'x';
    
    if(pos < 0)
        return;
    
    elem = &map->map[pos];
    save = &tkbio.save[mappos][pos];
    
    // only redraw if map is active
    if(redraw && tkbio.parser.map == mappos)
    {
        f(value, pos/map->width, pos%map->width, map, elem, save);
        // notify framebuffer for redraw
        if(tkbio.sim)
            send(tkbio.fb.sock, &sim_tmp, 1, 0);
    }
    else
        f(value, pos/map->width, pos%map->width, 0, elem, save);
}

void tkbio_type_help_set_value(int type, int id, int mappos, int value, int redraw, tkbio_type_help_set_func f)
{
    int pos = tkbio_type_help_find_type(type, id, mappos);
    tkbio_type_help_set_pos_value(pos, mappos, value, redraw, f);
}
