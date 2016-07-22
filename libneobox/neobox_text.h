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

#ifndef __NEOBOX_TEXT_H__
#define __NEOBOX_TEXT_H__

#define NEOBOX_TEXT_PASSWORD_LAYOUT      0 // take from layout
#define NEOBOX_TEXT_PASSWORD_OFF         1 // ignore layout, mode off
#define NEOBOX_TEXT_PASSWORD_ON          2 // ignore layout, mode on
#define NEOBOX_TEXT_PASSWORD_ALLBUTONE   3 // ignore layout, all characters but last *

void neobox_text_set(int id, int map, char *text, int redraw);
void neobox_text_reset(int id, int map, int redraw);
void neobox_text_password(int id, int map, int mode, int redraw);

char* neobox_text_get(int id, int map);

#endif
