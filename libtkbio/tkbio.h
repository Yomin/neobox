/*
 * Copyright (c) 2012-2014 Martin Rödel aka Yomin
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
#define TKBIO_FORMAT_PORTRAIT   1

#define TKBIO_OPTION_NO_INITIAL_PRINT   1

typedef int tkbio_handler(struct tkbio_return ret, void *state);

struct tkbio_config
{
    char *fb, *tsp;
    struct tkbio_layout layout;
    int format;
    int options;
};

int tkbio_init_default(const char *name);
int tkbio_init_default_args(const char *name, int *argc, char *argv[]);
int tkbio_init_layout(const char *name, struct tkbio_layout layout);
int tkbio_init_layout_args(const char *name, struct tkbio_layout layout, int *argc, char *argv[]);
int tkbio_init_custom(const char *name, struct tkbio_config config);
int tkbio_init_custom_args(const char *name, struct tkbio_config config, int *argc, char *argv[]);

struct tkbio_config tkbio_config_default();
struct tkbio_config tkbio_config_args(int *argc, char *argv[]);

void tkbio_finish();

int tkbio_run(tkbio_handler *h, void *state);
struct tkbio_return tkbio_handle_event();

void tkbio_set_signal_handler(void handler(int signal));

// from tkbio_fb.c
void tkbio_draw_rect(int y, int x, int height, int width, int color, int density, unsigned char *copy);
void tkbio_draw_rect_border(int y, int x, int height, int width, int color, unsigned char borders, int density, unsigned char *copy);
void tkbio_fill_rect(int y, int x, int height, int width, int density, unsigned char *copy);
void tkbio_fill_rect_border(int y, int x, int height, int width, unsigned char borders, int density, unsigned char *copy);

#endif

