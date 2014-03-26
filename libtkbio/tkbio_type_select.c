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

#include "tkbio.h"
#include "tkbio_select.h"
#include "tkbio_type_select.h"
#include "tkbio_type_button.h"
#include "tkbio_type_help.h"

#define NOP (struct tkbio_return) { .type = TKBIO_RETURN_NOP }

extern struct tkbio_global tkbio;
extern int button_copy_size;
extern unsigned char *button_copy;

static int forceprint;

inline static void set_data(int *size, unsigned char **copy, struct tkbio_save_select *select, struct tkbio_save *save)
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

inline static void restore_data(int size, unsigned char *copy, struct tkbio_save_select *select, struct tkbio_save *save)
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

void tkbio_type_select_init(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    
    if(save->partner)
    {
        if(!save->partner->data)
            save->partner->data = calloc(1, sizeof(struct tkbio_save_select));
    }
    else
        save->data = calloc(1, sizeof(struct tkbio_save_select));
    
    forceprint = 0;
}

void tkbio_type_select_finish(struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(select)
    {
        if(select->size)
            free(select->copy);
        free(select);
    }
}

void tkbio_type_select_draw(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    forceprint = 1;
    tkbio_type_select_focus_out(y, x, map, elem, save);
    forceprint = 0;
}

int tkbio_type_select_broader(int *y, int *x, int scr_y, int scr_x, const struct tkbio_mapelem *elem)
{
    return tkbio_type_button_broader(y, x, scr_y, scr_x, elem);
}

struct tkbio_return tkbio_type_select_press(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    int size;
    unsigned char *copy;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(select->status & TKBIO_TYPE_SELECT_STATUS_LOCKED)
        return NOP;
    
    set_data(&size, &copy, select, save);
    if(select->status & TKBIO_TYPE_SELECT_STATUS_ACTIVE)
        tkbio_type_button_focus_out(y, x, map, elem, save);
    else
        tkbio_type_button_press(y, x, button_y, button_x, map, elem, save);
    restore_data(size, copy, select, save);
    
    return NOP;
}

struct tkbio_return tkbio_type_select_move(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    return NOP;
}

struct tkbio_return tkbio_type_select_release(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    struct tkbio_return ret;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(!(select->status & TKBIO_TYPE_SELECT_STATUS_LOCKED))
        select->status ^= TKBIO_TYPE_SELECT_STATUS_ACTIVE;
    
    ret.type = TKBIO_RETURN_INT;
    ret.id = elem->id;
    ret.value.i = select->status & TKBIO_TYPE_SELECT_STATUS_ACTIVE;
    
    VERBOSE(printf("[TKBIO] select %s\n", ret.value.i ? "on" : "off"));
    
    return ret;
}

struct tkbio_return tkbio_type_select_focus_in(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    return tkbio_type_select_press(y, x, button_y, button_x, map, elem, save);
}

struct tkbio_return tkbio_type_select_focus_out(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    int size;
    unsigned char *copy;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(!forceprint && select->status & TKBIO_TYPE_SELECT_STATUS_LOCKED)
        return NOP;
    
    set_data(&size, &copy, select, save);
    if(select->status & TKBIO_TYPE_SELECT_STATUS_ACTIVE)
        tkbio_type_button_press(y, x, 0, 0, map, elem, save);
    else
        tkbio_type_button_focus_out(y, x, map, elem, save);
    restore_data(size, copy, select, save);
    
    return NOP;
}

void tkbio_type_select_set_name(const void *name, int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    
    select->name = name;
    
    if(map)
    {
        forceprint = 1;
        tkbio_type_select_focus_out(y, x, map, elem, save);
        forceprint = 0;
    }
}

void tkbio_type_select_set_active(const void *active, int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    if(*(int*)active)
        select->status |= TKBIO_TYPE_SELECT_STATUS_ACTIVE;
    else
        select->status &= ~TKBIO_TYPE_SELECT_STATUS_ACTIVE;
    
    if(map)
    {
        forceprint = 1;
        tkbio_type_select_focus_out(y, x, map, elem, save);
        forceprint = 0;
    }
}

void tkbio_type_select_set_locked(const void *locked, int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    if(*(int*)locked)
        select->status |= TKBIO_TYPE_SELECT_STATUS_LOCKED;
    else
        select->status &= ~TKBIO_TYPE_SELECT_STATUS_LOCKED;
}

void tkbio_select_set_name(int id, int mappos, const char *name, int redraw)
{
    tkbio_type_help_set_value(TKBIO_LAYOUT_TYPE_SELECT, id, mappos,
        name, redraw, tkbio_type_select_set_name);
}

void tkbio_select_set_active(int id, int mappos, int active, int redraw)
{
    tkbio_type_help_set_value(TKBIO_LAYOUT_TYPE_SELECT, id, mappos,
        &active, redraw, tkbio_type_select_set_active);
}

void tkbio_select_set_locked(int id, int mappos, int locked)
{
    tkbio_type_help_set_value(TKBIO_LAYOUT_TYPE_SELECT, id, mappos,
        &locked, 0, tkbio_type_select_set_locked);
}
