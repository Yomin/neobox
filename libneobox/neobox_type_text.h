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

#ifndef __NEOBOX_TYPE_TEXT_H__
#define __NEOBOX_TYPE_TEXT_H__

#include "neobox_type_macros.h"
#include "neobox_fb.h"

struct neobox_save_text
{
    char *text;     // text buffer
    int fill, size; // text buffer fill/size
    int window;     // display text (rel value from text)
    int cursor;     // cursor pos (rel value from window)
    int max;        // max window size
    
    int copysize;
    unsigned char *copy;
    
    // draw infos
    int click_y, click_x, click_map, click_map_main;
    const struct neobox_mapelem *click_elem;
};

TYPE_FUNCTIONS(text);

TYPE_FUNC_ACTION(text, set);
TYPE_FUNC_ACTION(text, get);
TYPE_FUNC_ACTION(text, reset);

#endif
