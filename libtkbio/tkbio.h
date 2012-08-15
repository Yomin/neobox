/*
 * Copyright (c) 2012 Martin Rödel aka Yomin
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

#ifndef __TKBIO_H__
#define __TKBIO_H__

#include "tkbio_layout.h"

#define TKBIO_ERROR_RPC_OPEN    -1
#define TKBIO_ERROR_FB_OPEN     -2
#define TKBIO_ERROR_FB_VINFO    -3
#define TKBIO_ERROR_FB_FINFO    -4
#define TKBIO_ERROR_FB_MMAP     -5
#define TKBIO_ERROR_SCREEN_OPEN -6
#define TKBIO_ERROR_SCREEN_POLL -7

#define TKBIO_FORMAT_LANDSCAPE  0
#define TKBIO_FORMAT_PORTAIT    1

typedef int tkbio_handler(struct tkbio_charelem elem, void *state);

struct tkbio_config
{
    char *fb;
    struct tkbio_layout layout;
    int format;
};

int tkbio_init_default(const char *name);
int tkbio_init_custom(const char *name, struct tkbio_config config);

struct tkbio_config tkbio_default_config();

void tkbio_finish();

int tkbio_run(tkbio_handler *h, void *state);
struct tkbio_charelem tkbio_handle_event();

#endif

