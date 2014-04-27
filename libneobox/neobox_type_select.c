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
#include <stdlib.h>

#include "neobox_select.h"
#include "neobox_type_select.h"
#include "neobox_type_button.h"
#include "neobox_type_help.h"

#define NOP (struct neobox_event) { .type = NEOBOX_EVENT_NOP }

extern struct neobox_global neobox;
extern int button_copy_size;
extern unsigned char *button_copy;

static int forceprint;

inline static void set_data(int *size, unsigned char **copy, struct neobox_save_select *select, struct neobox_save *save)
{
    *size = button_copy_size;
    *copy = button_copy;
    button_copy_size = select->size;
    button_copy = select->copy;
    
    if(save->partner)
        save->partner->data = (void*)select->name;
    else
        save->data = (void*)select->name;
}

inline static void restore_data(int size, unsigned char *copy, struct neobox_save_select *select, struct neobox_save *save)
{
    select->size = button_copy_size;
    select->copy = button_copy;
    button_copy_size = size;
    button_copy = copy;
    
    if(save->partner)
        save->partner->data = select;
    else
        save->data = select;
}

void neobox_type_select_init(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    
    if(save->partner)
    {
        if(!save->partner->data)
            save->partner->data = calloc(1, sizeof(struct neobox_save_select));
    }
    else
        save->data = calloc(1, sizeof(struct neobox_save_select));
    
    forceprint = 0;
}

void neobox_type_select_finish(struct neobox_save *save)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(select)
    {
        if(select->size)
            free(select->copy);
        free(select);
    }
}

void neobox_type_select_draw(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    forceprint = 1;
    neobox_type_select_focus_out(y, x, map, elem, save);
    forceprint = 0;
}

int neobox_type_select_broader(int *y, int *x, int scr_y, int scr_x, const struct neobox_mapelem *elem)
{
    return neobox_type_button_broader(y, x, scr_y, scr_x, elem);
}

struct neobox_event neobox_type_select_press(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    struct neobox_save_select *select;
    int size;
    unsigned char *copy;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(select->status & NEOBOX_TYPE_SELECT_STATUS_LOCKED)
        return NOP;
    
    set_data(&size, &copy, select, save);
    if(select->status & NEOBOX_TYPE_SELECT_STATUS_ACTIVE)
        neobox_type_button_focus_out(y, x, map, elem, save);
    else
        neobox_type_button_press(y, x, button_y, button_x, map, elem, save);
    restore_data(size, copy, select, save);
    
    return NOP;
}

struct neobox_event neobox_type_select_move(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    return NOP;
}

struct neobox_event neobox_type_select_release(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    struct neobox_save_select *select;
    struct neobox_event ret;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(!(select->status & NEOBOX_TYPE_SELECT_STATUS_LOCKED))
        select->status ^= NEOBOX_TYPE_SELECT_STATUS_ACTIVE;
    
    ret.type = NEOBOX_EVENT_INT;
    ret.id = elem->id;
    ret.value.i = select->status & NEOBOX_TYPE_SELECT_STATUS_ACTIVE;
    
    VERBOSE(printf("[NEOBOX] select %s\n", ret.value.i ? "on" : "off"));
    
    return ret;
}

struct neobox_event neobox_type_select_focus_in(int y, int x, int button_y, int button_x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    return neobox_type_select_press(y, x, button_y, button_x, map, elem, save);
}

struct neobox_event neobox_type_select_focus_out(int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    struct neobox_save_select *select;
    int size;
    unsigned char *copy;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(!forceprint && select->status & NEOBOX_TYPE_SELECT_STATUS_LOCKED)
        return NOP;
    
    set_data(&size, &copy, select, save);
    if(select->status & NEOBOX_TYPE_SELECT_STATUS_ACTIVE)
        neobox_type_button_press(y, x, 0, 0, map, elem, save);
    else
        neobox_type_button_focus_out(y, x, map, elem, save);
    restore_data(size, copy, select, save);
    
    return NOP;
}

void neobox_type_select_set_name(const void *name, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    
    select->name = name;
    
    if(map)
    {
        forceprint = 1;
        neobox_type_select_focus_out(y, x, map, elem, save);
        forceprint = 0;
    }
}

void neobox_type_select_set_active(const void *active, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    if(*(int*)active)
        select->status |= NEOBOX_TYPE_SELECT_STATUS_ACTIVE;
    else
        select->status &= ~NEOBOX_TYPE_SELECT_STATUS_ACTIVE;
    
    if(map)
    {
        forceprint = 1;
        neobox_type_select_focus_out(y, x, map, elem, save);
        forceprint = 0;
    }
}

void neobox_type_select_set_locked(const void *locked, int y, int x, const struct neobox_map *map, const struct neobox_mapelem *elem, struct neobox_save *save)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    if(*(int*)locked)
        select->status |= NEOBOX_TYPE_SELECT_STATUS_LOCKED;
    else
        select->status &= ~NEOBOX_TYPE_SELECT_STATUS_LOCKED;
}

void neobox_select_set_name(int id, int mappos, const char *name, int redraw)
{
    neobox_type_help_set_value(NEOBOX_LAYOUT_TYPE_SELECT, id, mappos,
        name, redraw, neobox_type_select_set_name);
}

void neobox_select_set_active(int id, int mappos, int active, int redraw)
{
    neobox_type_help_set_value(NEOBOX_LAYOUT_TYPE_SELECT, id, mappos,
        &active, redraw, neobox_type_select_set_active);
}

void neobox_select_set_locked(int id, int mappos, int locked)
{
    neobox_type_help_set_value(NEOBOX_LAYOUT_TYPE_SELECT, id, mappos,
        &locked, 0, neobox_type_select_set_locked);
}
