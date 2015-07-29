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

#ifndef __NEOBOX_TYPE_BUTTON_H__
#define __NEOBOX_TYPE_BUTTON_H__

#include "neobox_button.h"
#include "neobox_type_macros.h"

struct neobox_save_button
{
    char *name;
    neobox_checkfun *check;
};

TYPE_FUNCTIONS(button);

TYPE_FUNC_ACTION(button, set_name);
TYPE_FUNC_ACTION(button, set_check);

// set button copy save for reuse of button functions
void neobox_type_button_copy_set(int size, unsigned char *copy, char *name, struct neobox_save *save);
void neobox_type_button_copy_restore(int *size, unsigned char **copy, void *data, struct neobox_save *save);

#endif
