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
#include <string.h>

#include "neobox_fb.h"
#include "neobox_text.h"
#include "neobox_type_text.h"
#include "neobox_type_button.h"
#include "neobox_type_help.h"

#define INITSIZE    20
#define NOP         (struct neobox_event) { .type = NEOBOX_EVENT_NOP }
#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))

extern struct neobox_global neobox;

static void text_insert(const char *str, int len, struct neobox_save_text *text)
{
    int size = text->fill+len, diff;
    int end = text->window+text->cursor == text->fill;
    char *cursor;
    
    if(size >= text->size)
    {
        text->size += MAX(size, INITSIZE);
        text->text = realloc(text->text, text->size+1);
    }
    
    cursor = text->text+text->window+text->cursor;
    
    // if cursor not at text end make space
    if(!end)
        memmove(cursor+len, cursor, text->fill-text->window-text->cursor);
    
    strncpy(cursor, str, len);
    
    text->fill += len;
    text->text[text->fill] = 0;
    
    if(end)
    {
        text->cursor = MIN(text->max-1, text->cursor+len);
        text->window = text->fill-text->cursor;
    }
    else
    {
        // fill up until window full
        diff = text->max-text->fill+len;
        // if full fill up until cursor at 2/3 max
        if(diff <= 0)
            diff = 2*(text->max/3)-text->cursor+1;
        if(diff > 0)
        {
            text->cursor += MIN(diff, len);
            len -= MIN(diff, len);
        }
        // otherwise move window right
        text->window += len;
    }
}

static void text_delete(struct neobox_save_text *text)
{
    int right = text->fill-text->window-text->max;
    char *cursor = text->text+text->window+text->cursor;
    
    if(!text->cursor)
        return;
    
    memmove(cursor-1, cursor, text->fill-text->window-text->cursor);
    
    text->fill--;
    text->text[text->fill] = 0;
    
    // delete to left until cursor at 1/3 max or no text after window left
    if(text->cursor > text->max/3 && right > 0)
        text->cursor--;
    else if(text->window)
        text->window--;
    else
        text->cursor--;
}

static void text_cursor_left(struct neobox_save_text *text)
{
    int cursor;
    
    // move window left if cursor in first 1/3
    if(text->cursor <= text->max/3 && text->window)
    {
        cursor = MIN(text->max/2, text->window+text->cursor-1);
        text->window -= cursor-text->cursor+1;
        text->cursor = cursor;
    }
    // move window 1 left if cursor at text end
    else if(text->window+text->cursor == text->fill && text->window)
        text->window--;
    else if(text->cursor)
        text->cursor--;
}

static void text_cursor_right(struct neobox_save_text *text)
{
    int cursor, right = text->fill-text->window-text->max;
    
    // move window right if cursor in last 1/3
    if(text->cursor > 2*(text->max/3) && right > 0)
    {
        cursor = MAX(text->max/2, text->cursor-right+1);
        text->window += text->cursor-cursor+1;
        text->cursor = cursor;
    }
    // move window 1 right to put cursor at text end
    else if(text->window+text->cursor == text->fill-1 && text->cursor == text->max-1)
        text->window++;
    else if(text->window+text->cursor < text->fill)
        text->cursor++;
}

static struct neobox_event finish(struct neobox_save *save, int evt)
{
    struct neobox_event event = NOP;
    struct neobox_save_text *text;
    
    text = save->partner ? save->partner->data : save->data;
    
    neobox.filter_fun = 0;
    neobox.parser.map_main = text->click_map_main;
    neobox.parser.map = text->click_map;
    
    neobox_type_text_focus_out(text->click_y, text->click_x,
        &neobox.layout.maps[text->click_map], text->click_elem, save);
    
    if(evt)
    {
        event.type = NEOBOX_EVENT_TEXT;
        event.id = text->click_elem->id;
        
        VERBOSE(printf("[NEOBOX] text %i [%s] set \"%s\"\n",
            text->click_elem->id, text->click_elem->name, text->text));
    }
    else
        VERBOSE(printf("[NEOBOX] text %i [%s] abort\n",
            text->click_elem->id, text->click_elem->name));
    
    return event;
}

static struct neobox_event key_grab(struct neobox_event event, void *state)
{
    struct neobox_save *save = state;
    struct neobox_save_text *text;
    
    text = save->partner ? save->partner->data : save->data;
    
    if(event.type != NEOBOX_EVENT_CHAR)
        return event;
    
    switch(event.value.c.c[0])
    {
    case '\n':
        return finish(save, 1);
    case '\b':
        text_delete(text);
        break;
    case 27: // Esc
        switch(event.value.c.c[1])
        {
        case 0:
            return finish(save, 0);
        case '[':
            break;
        default:
            return NOP;
        }
        switch(event.value.c.c[2])
        {
        case 'D': // left
            text_cursor_left(text);
            break;
        case 'C': // right
            text_cursor_right(text);
            break;
        default:
            return NOP;
        }
        break;
    default:
        if(event.value.c.c[0] < ' ' || event.value.c.c[0] > '~')
            return NOP;
        text_insert(event.value.c.c, 1, text);
    }
    
    return neobox_type_text_press(text->click_y, text->click_x, 0, 0,
        &neobox.layout.maps[text->click_map], text->click_elem, save);
}

TYPE_FUNC_INIT(text)
{
    struct neobox_save_text *text;
    struct vector *connect;
    struct neobox_point *p, *p2;
    int height, width;
    
    neobox_get_sizes(&height, &width, 0, 0, 0, 0, map);
    
    if(save->partner)
    {
        if(!save->partner->data)
        {
            text = save->partner->data = calloc(1, sizeof(struct neobox_save_text));
            
            connect = save->partner->connect;
            p = vector_at(vector_size(connect)-1, connect);
            p2 = vector_at(0, connect);
            
            text->max = neobox_string_size((p->x - p2->x +1)*width);
        }
    }
    else
    {
        text = save->data = calloc(1, sizeof(struct neobox_save_text));
        text->max = neobox_string_size(width);
    }
    
    if(!text->size)
    {
        text->size = INITSIZE;
        text->text = malloc(INITSIZE+1);
    }
}

TYPE_FUNC_FINISH(text)
{
    struct neobox_save_text *text;
    
    text = save->partner ? save->partner->data : save->data;
    
    if(text)
    {
        if(text->size)
            free(text->text);
        free(text);
    }
}

TYPE_FUNC_DRAW(text)
{
    neobox_type_text_focus_out(y, x, map, elem, save);
}

TYPE_FUNC_BROADER(text)
{
    return neobox_type_button_broader(y, x, scr_y, scr_x, map, elem);
}

TYPE_FUNC_PRESS(text)
{
    struct neobox_save_text *text;
    struct neobox_text_control control;
    
    text = save->partner ? save->partner->data : save->data;
    
    control.soh = 1;
    control.text = text->text+text->window;
    control.cursor = text->cursor;
    control.left = text->window;
    
    neobox_type_button_copy_set(text->copysize, text->copy, (char*)&control, save);
    neobox_type_button_press(y, x, button_y, button_x, map, elem, save);
    neobox_type_button_copy_restore(&text->copysize, &text->copy, text, save);
    
    return NOP;
}

TYPE_FUNC_MOVE(text)
{
    return NOP;
}

TYPE_FUNC_RELEASE(text)
{
    struct neobox_save_text *text;
    
    text = save->partner ? save->partner->data : save->data;
    
    VERBOSE(printf("[NEOBOX] text %i [%s] input\n", elem->id, elem->name));
    
    // save click info to draw field
    text->click_y = y;
    text->click_x = x;
    text->click_map = neobox.parser.map;
    text->click_map_main = neobox.parser.map_main;
    text->click_elem = elem;
    
    // goto keyboard
    neobox.parser.map_main = neobox.parser.map = elem->elem.i + map->offset;
    neobox.parser.hold = 0;
    
    // install event filter for key grabbing
    neobox.filter_fun = key_grab;
    neobox.filter_state = save;
    
    return NOP;
}

TYPE_FUNC_FOCUS_IN(text)
{
    return neobox_type_text_press(y, x, button_y, button_x, map, elem, save);
}

TYPE_FUNC_FOCUS_OUT(text)
{
    struct neobox_save_text *text;
    struct neobox_text_control control;
    int left;
    
    text = save->partner ? save->partner->data : save->data;
    left = text->window+text->cursor == text->fill && text->window;
    
    control.soh = 1;
    control.text = text->text+text->window-left;
    control.cursor = -1;
    control.left = text->window-left;
    
    neobox_type_button_copy_set(text->copysize, text->copy, (char*)&control, save);
    neobox_type_button_focus_out(y, x, map, elem, save);
    neobox_type_button_copy_restore(&text->copysize, &text->copy, text, save);
    
    return NOP;
}

TYPE_FUNC_ACTION(text, set)
{
    struct neobox_save_text *text;
    int len = strlen(data);
    
    text = save->partner ? save->partner->data : save->data;
    
    if(len >= text->size)
    {
        text->size += MAX(len-text->size, INITSIZE);
        text->text = realloc(text->text, text->size+1);
    }
    
    strcpy(text->text, data);
    
    text->fill = len;
    text->cursor = 0;
    text->window = 0;
    
    if(map)
        neobox_type_text_focus_out(y, x, map, elem, save);
    
    return 0;
}

void neobox_text_set(int id, int mappos, char *text, int redraw)
{
    neobox_type_help_action(NEOBOX_LAYOUT_TYPE_TEXT, id, mappos,
        (void*)text, redraw, neobox_type_text_set);
}

TYPE_FUNC_ACTION(text, reset)
{
    struct neobox_save_text *text;
    
    text = save->partner ? save->partner->data : save->data;
    
    text->text = realloc(text->text, INITSIZE);
    text->size = INITSIZE;
    text->window = 0;
    text->cursor = 0;
    text->fill = 0;
    
    if(map)
        neobox_type_text_focus_out(y, x, map, elem, save);
    
    return 0;
}

void neobox_text_reset(int id, int mappos, int redraw)
{
    neobox_type_help_action(NEOBOX_LAYOUT_TYPE_TEXT, id, mappos,
        0, redraw, neobox_type_text_reset);
}
