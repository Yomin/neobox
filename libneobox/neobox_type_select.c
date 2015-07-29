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

static int forceprint; // force redraw regardless of locked status

TYPE_FUNC_INIT(select)
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

TYPE_FUNC_FINISH(select)
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

TYPE_FUNC_DRAW(select)
{
    forceprint = 1; // initial draw regardless of locked status
    TYPE_FUNC_FOCUS_OUT_CALL(select);
    forceprint = 0;
}

TYPE_FUNC_BROADER(select)
{
    return TYPE_FUNC_BROADER_CALL(button);
}

TYPE_FUNC_PRESS(select)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    
    if(select->status & NEOBOX_TYPE_SELECT_STATUS_LOCKED)
        return NOP;
    
    neobox_type_button_copy_set(select->size, select->copy, select->name, save);
    if(select->status & NEOBOX_TYPE_SELECT_STATUS_ACTIVE)
        TYPE_FUNC_FOCUS_OUT_CALL(button);
    else
        TYPE_FUNC_PRESS_CALL(button);
    neobox_type_button_copy_restore(&select->size, &select->copy, select, save);
    
    return NOP;
}

TYPE_FUNC_MOVE(select)
{
    return NOP;
}

TYPE_FUNC_RELEASE(select)
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

TYPE_FUNC_FOCUS_IN(select)
{
    return TYPE_FUNC_PRESS_CALL(select);
}

TYPE_FUNC_FOCUS_OUT(select)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    
    // if locked and no eg initial draw skip redraw
    if(!forceprint && select->status & NEOBOX_TYPE_SELECT_STATUS_LOCKED)
        return NOP;
    
    neobox_type_button_copy_set(select->size, select->copy, select->name, save);
    if(select->status & NEOBOX_TYPE_SELECT_STATUS_ACTIVE)
        TYPE_FUNC_PRESS_YX0_CALL(button);
    else
        TYPE_FUNC_FOCUS_OUT_CALL(button);
    neobox_type_button_copy_restore(&select->size, &select->copy, select, save);
    
    return NOP;
}

TYPE_FUNC_ACTION(select, set_name)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    
    select->name = data;
    
    if(map)
    {
        forceprint = 1; // redraw regardless of locked status
        TYPE_FUNC_FOCUS_OUT_CALL(select);
        forceprint = 0;
    }
    
    return 0;
}

TYPE_FUNC_ACTION(select, set_active)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    if(*(int*)data)
        select->status |= NEOBOX_TYPE_SELECT_STATUS_ACTIVE;
    else
        select->status &= ~NEOBOX_TYPE_SELECT_STATUS_ACTIVE;
    
    if(map)
    {
        forceprint = 1; // redraw regardless of locked status
        TYPE_FUNC_FOCUS_OUT_CALL(select);
        forceprint = 0;
    }
    
    return 0;
}

TYPE_FUNC_ACTION(select, set_locked)
{
    struct neobox_save_select *select;
    
    select = save->partner ? save->partner->data : save->data;
    if(*(int*)data)
        select->status |= NEOBOX_TYPE_SELECT_STATUS_LOCKED;
    else
        select->status &= ~NEOBOX_TYPE_SELECT_STATUS_LOCKED;
    
    return 0;
}

void neobox_select_set_name(int id, int mappos, char *name, int redraw)
{
    neobox_type_help_action(NEOBOX_LAYOUT_TYPE_SELECT, id, mappos,
        name, redraw, neobox_type_select_set_name);
}

void neobox_select_set_active(int id, int mappos, int active, int redraw)
{
    neobox_type_help_action(NEOBOX_LAYOUT_TYPE_SELECT, id, mappos,
        &active, redraw, neobox_type_select_set_active);
}

void neobox_select_set_locked(int id, int mappos, int locked)
{
    neobox_type_help_action(NEOBOX_LAYOUT_TYPE_SELECT, id, mappos,
        &locked, 0, neobox_type_select_set_locked);
}
