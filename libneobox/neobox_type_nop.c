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

#include "neobox_type_nop.h"
#include "neobox_type_button.h"
#include "neobox_type_help.h"

#include <stdlib.h>

void neobox_type_nop_init(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    if(save->partner)
    {
        if(!save->partner->data)
            save->partner->data = calloc(1, sizeof(struct neobox_save_button));
    }
    else
        save->data = calloc(1, sizeof(struct neobox_save_button));
}

void neobox_type_nop_finish(struct neobox_save *save)
{
    if(save->partner)
        free(save->partner->data);
    else
        free(save->data);
}

void neobox_type_nop_draw(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    neobox_type_button_draw(y, x, map, elem, save);
}

void neobox_type_nop_broader(int *y, int *x, int scr_y, int scr_x, const struct neobox_map *map, const struct neobox_mapelem *elem)
{
    
}

void neobox_type_nop_press(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    
}

void neobox_type_nop_move(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    
}

void neobox_type_nop_release(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    
}

void neobox_type_nop_focus_in(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    
}

void neobox_type_nop_focus_out(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    
}

void neobox_nop_set_name(int id, int mappos, char *name, int redraw)
{
    neobox_type_help_action(NEOBOX_LAYOUT_TYPE_NOP, id, mappos,
        name, redraw, neobox_type_button_set_name);
}
