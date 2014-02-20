/*
 * Copyright (c) 2013-2014 Martin Rödel aka Yomin
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
#include <alg/vector.h>
#include "tkbio_layout.h"

#ifdef NDEBUG
#   define DEBUG(x)
#else
#   define DEBUG(x) x
#endif

#define VERBOSE(x)  if(tkbio.verbose) { x; }

#define SCREENMAX   830     // screen size in pixel
#define DENSITY     1       // button draw density in pixel
#define INCREASE    33      // button size increase in percent
#define DELAY       100000  // debouncer pause delay in us

#define FB_STATUS_NOP  0    // last button not drawn
#define FB_STATUS_COPY 1    // last button saved/drawn
#define FB_STATUS_FILL 2    // last button drawn

#define PARSER_STATUS_NOP     0 // no action
#define PARSER_STATUS_PRESSED 1 // button pressed
#define PARSER_STATUS_HSLIDER 2 // horz slider active
#define PARSER_STATUS_VSLIDER 3 // vert slider active
#define PARSER_STATUS_SLIDER  2 // some slider active hack

#define MOVE_NOP     0  // no move
#define MOVE_NEXT    1  // move to next button
#define MOVE_PARTNER 2  // move to partner
#define MOVE_HSLIDER 3  // hslide on button/partners
#define MOVE_VSLIDER 4  // vslide on button/partners

struct tkbio_fb
{
    int fd;   // framebuffer file descriptor
    int sock; // unix socket descriptor for sim
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int size; // framebuffer size
    int bpp;  // bytes per pixel
    unsigned char *ptr; // mmap'ed framebuffer pointer
    
    char shm[20]; // shared memory segment name for sim
    
    int copy_size;       // allocated copy size
    unsigned char *copy; // copy buf pointer
    
    int status; // last button nop/copy/fill
};

struct tkbio_parser
{
    int status;           // button pressed or slider
    int y, x;             // last pos (layout format)
    int map;              // current map
    int hold;             // hold mode active
    unsigned char toggle; // active toggle buttons
};

struct tkbio_partner
{
    unsigned char active;   // slider used
    int y, x;               // slider save data
    struct vector *connect; // partner array
};

struct tkbio_global
{
    const char *name;   // application name
    const char *tsp;    // tsp directory
    int format;         // portrait or landscape
    int pause;          // pause for debouncer
    int verbose;        // verbose messages
    int sock;           // unix socket to tsp
    int sim;            // sim enabled
    
    struct tkbio_fb fb;
    struct tkbio_layout layout;
    struct tkbio_parser parser;
    struct tkbio_partner ***partner; // button partner array
    
    void (*custom_signal_handler)(int signal);
};

#endif

