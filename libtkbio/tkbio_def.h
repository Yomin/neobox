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

#ifndef __TKBIO_DEF_H__
#define __TKBIO_DEF_H__

#include <linux/fb.h>
#include "tkbio_layout.h"

#ifdef NDEBUG
#   define DEBUG
#else
#   define DEBUG(x) x
#endif

#define RPCNAME     "tspd"
#define SCREENMAX   830
#define DENSITY     1       // pixel
#define INCREASE    33      // percent
#define DELAY       100000  // us

#define MSB         (1<<(sizeof(int)*8-1))
#define SMSB        (1<<(sizeof(int)*8-2))
#define NIBBLE      ((sizeof(int)/2)*8)

#define FB_STATUS_NOP  0
#define FB_STATUS_COPY 1
#define FB_STATUS_FILL 2

struct tkbio_fb
{
    int fd;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int size, bpp;
    char *ptr;
    int copySize;
    char *copy, copyColor;
    int status; // last button nop/copy/fill
};

struct tkbio_parser
{
    int pressed; // button currently pressed
    int y, x;    // last pressed pos
    int map;
    int hold;
    char toggle;
};

struct tkbio_global
{
    int id, format, pause;
    int fd_rpc, fd_sc;
    struct tkbio_fb fb;
    struct tkbio_layout layout;
    struct tkbio_parser parser;
    void (*custom_signal_handler)(int signal);
};

#endif

