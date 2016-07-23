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

#include <stdio.h>
#include <sys/socket.h>

#include "neobox_type_help.h"
#include "neobox_def.h"
#include "neobox_log.h"

extern struct neobox_global neobox;

int neobox_type_help_find_type(int type_from, int type_to, int id, int mappos)
{
    const struct neobox_map *map = &neobox.layout.maps[mappos];
    const struct neobox_mapelem *elem;
    int size = map->height*map->width;
    int i;
    
    for(i=0; i<size; i++)
    {
        elem = &map->map[i];
        if(elem->type >= type_from && elem->type <= type_to && elem->id == id)
            return i;
    }
    
    neobox_printf(1, "elem %i not found\n", id);
    
    return -1;
}

void* neobox_type_help_action_pos(int pos, int mappos, void *value, int redraw, neobox_type_help_action_func f)
{
    const struct neobox_map *map = &neobox.layout.maps[mappos];
    const struct neobox_mapelem *elem;
    struct neobox_save *save;
    SIMV(unsigned char sim_tmp = 'x');
    void *ret;
    
    if(pos < 0)
        return 0;
    
    elem = &map->map[pos];
    save = &neobox.save[mappos][pos];
    
    // only redraw if map is active
    if(redraw && neobox.parser.map == mappos)
    {
        ret = f(value, pos/map->width, pos%map->width, map, elem, save);
        // notify framebuffer for redraw
        SIMV(send(neobox.fb.sock, &sim_tmp, 1, 0));
        return ret;
    }
    else
        return f(value, pos/map->width, pos%map->width, 0, elem, save);
}

void* neobox_type_help_action_range(int type_from, int type_to, int id, int mappos, void *value, int redraw, neobox_type_help_action_func f)
{
    int pos = neobox_type_help_find_type(type_from, type_to, id, mappos);
    return neobox_type_help_action_pos(pos, mappos, value, redraw, f);
}

void* neobox_type_help_action(int type, int id, int mappos, void *value, int redraw, neobox_type_help_action_func f)
{
    int pos = neobox_type_help_find_type(type, type, id, mappos);
    return neobox_type_help_action_pos(pos, mappos, value, redraw, f);
}
