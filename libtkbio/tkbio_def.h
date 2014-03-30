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
#include <sys/queue.h>
#include <sys/time.h>
#include <alg/vector.h>

#include <tsp.h>

#include "tkbio.h"

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

#define TIMER_SYSTEM 0
#define TIMER_USER   1

#define EVENT_NOP   0
#define EVENT_TSP   1
#define EVENT_TKBIO 2

struct tkbio_event
{
    char type;
    union
    {
        struct tsp_event tsp;
        struct tkbio_return tkbio;
    } event;
};

struct tkbio_chain_queue
{
    CIRCLEQ_ENTRY(tkbio_chain_queue) chain;
    struct tkbio_event event;
};
CIRCLEQ_HEAD(tkbio_queue, tkbio_chain_queue);

struct tkbio_chain_timer
{
    CIRCLEQ_ENTRY(tkbio_chain_timer) chain;
    struct timeval tv;
    unsigned char id, type;
};
CIRCLEQ_HEAD(tkbio_timer, tkbio_chain_timer);

struct tkbio_point
{
    int y, x;
    const struct tkbio_mapelem *elem;
};

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
};

struct tkbio_parser
{
    int pressed;          // button pressed
    int y, x;             // last pos (layout format)
    int map;              // current map
    int hold;             // hold mode active
    unsigned char toggle; // active toggle buttons
};

struct tkbio_tsp
{
    const char *dir;
    int sock;
    int lock;       // app has screen locked
    int hide;       // app is hidden
    int priority;   // apps hidden priority
    int grab;       // app grabbs buttons
};

struct tkbio_partner
{
    char flag; // check if already processed
    void *data;
    struct vector *connect;
};

struct tkbio_save
{
    void *data;
    struct tkbio_partner *partner;
};

struct tkbio_global
{
    int format;         // portrait or landscape
    int pause;          // pause for debouncer
    int verbose;        // verbose messages
    int redraw;         // redraw screen on activate
    int sim;            // sim enabled
    char flagstat;      // last partner flag
    
    struct tkbio_fb fb;
    struct tkbio_tsp tsp;
    struct tkbio_layout layout;
    struct tkbio_parser parser;
    struct tkbio_save **save; // button save array
    struct tkbio_queue queue; // event queue
    struct tkbio_timer timer; // timer queue
};

#endif

