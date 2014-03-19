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

#include "tkbio_type_select.h"
#include "tkbio_type_button.h"

#define NOP (struct tkbio_return) { .type = TKBIO_RETURN_NOP }

extern struct tkbio_global tkbio;
extern int button_copy_size;
extern unsigned char *button_copy;

inline static void set_copy(int *size, unsigned char **copy, struct tkbio_save_select *select)
{
    *size = button_copy_size;
    *copy = button_copy;
    button_copy_size = select->size;
    button_copy = select->copy;
}

inline static void restore_copy(int size, unsigned char *copy, struct tkbio_save_select *select)
{
    select->size = button_copy_size;
    select->copy = button_copy;
    button_copy_size = size;
    button_copy = copy;
}

void tkbio_type_select_init(int y, int x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    void *ptr = calloc(1, sizeof(struct tkbio_save_select));
    
    if(save->partner)
        save->partner->data = ptr;
    else
        save->data = ptr;
}

void tkbio_type_select_finish(struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(select->size)
        free(select->copy);
    free(select);
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
    
    set_copy(&size, &copy, select);
    if(select->status)
        tkbio_type_button_focus_out(map, elem, save);
    else
        tkbio_type_button_press(y, x, button_y, button_x, map, elem, save);
    restore_copy(size, copy, select);
    
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
    
    ret.type = TKBIO_RETURN_INT;
    ret.value.i = select->status = !select->status;
    
    VERBOSE(printf("[TKBIO] select %s\n", select->status ? "on" : "off"));
    
    return ret;
}

struct tkbio_return tkbio_type_select_focus_in(int y, int x, int button_y, int button_x, const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    return tkbio_type_select_press(y, x, button_y, button_x, map, elem, save);
}

struct tkbio_return tkbio_type_select_focus_out(const struct tkbio_map *map, const struct tkbio_mapelem *elem, struct tkbio_save *save)
{
    struct tkbio_save_select *select;
    int size;
    unsigned char *copy;
    
    select = save->partner ? save->partner->data : save->data;
    
    set_copy(&size, &copy, select);
    if(!select->status)
        tkbio_type_button_focus_out(map, elem, save);
    else
        tkbio_type_button_press(tkbio.parser.y, tkbio.parser.x,
            0, 0, map, elem, save);
    restore_copy(size, copy, select);
    
    return NOP;
}
