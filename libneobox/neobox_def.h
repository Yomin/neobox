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

#ifndef __NEOBOX_DEF_H__
#define __NEOBOX_DEF_H__

#include <linux/fb.h>
#include <sys/queue.h>

#include <alg/vector.h>
#include <iod.h>

#include "neobox.h"

#ifdef NDEBUG
#   define DEBUG(x)
#else
#   define DEBUG(x) x
#endif

#ifdef SIM
#   define SIMV(x)  x
#   define FONTMULT 1
#else
#   define SIMV(x)
#   define FONTMULT 2   // fontmult pixel for 1 pixel
#endif

#define VERBOSE(x)  if(neobox.verbose) { x; }

#define SCREENMAX   830     // screen size in pixel
#define DENSITY     1       // button draw density in pixel
#define INCREASE    33      // button size increase in percent
#define DELAY       100     // debouncer pause delay in us

#define TIMER_SYSTEM 0
#define TIMER_USER   1

#define STASH_NOP    0
#define STASH_IOD    1
#define STASH_NEOBOX 2

typedef struct neobox_event neobox_filter(struct neobox_event event, void *state);

struct neobox_stash
{
    char type;
    union
    {
        struct iod_event iod;
        struct neobox_event neobox;
    } event;
};

struct neobox_chain_queue
{
    CIRCLEQ_ENTRY(neobox_chain_queue) chain;
    struct neobox_stash stash;
};
CIRCLEQ_HEAD(neobox_queue, neobox_chain_queue);

struct neobox_chain_timer
{
    CIRCLEQ_ENTRY(neobox_chain_timer) chain;
    struct timeval tv;
    unsigned char id, type;
};
CIRCLEQ_HEAD(neobox_timer, neobox_chain_timer);

struct neobox_point
{
    int y, x;
    const struct neobox_mapelem *elem;
};

struct neobox_fb
{
    int fd;   // framebuffer file descriptor
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int size; // framebuffer size
    int bpp;  // bytes per pixel
    unsigned char *ptr; // mmap'ed framebuffer pointer
    
#ifdef SIM
    int sock;     // unix socket descriptor
    char shm[20]; // shared memory segment name
#endif
};

struct neobox_parser
{
    int pressed;          // button pressed
    int y, x;             // last pos (layout format)
    int map;              // current map
    int map_main;         // main map
    int hold;             // hold mode active
    unsigned char toggle; // active toggle buttons
};

struct neobox_iod
{
    const char *usock;
    int sock;
    int lock;       // app has screen locked
    int hide;       // app is hidden
    int priority;   // apps hidden priority
    int grab;       // app grabbs buttons
};

struct neobox_partner
{
    char flag; // check if already processed
    void *data;
    struct vector *connect;
};

struct neobox_save
{
    void *data;
    struct neobox_partner *partner;
};

struct neobox_config
{
    char *file, flags;
    void *rj;
};

struct neobox_global
{
    int format;         // portrait or landscape
    int pause;          // pause for debouncer
    int verbose;        // verbose messages
    int options;        // options from neobox init
    char *flagstat;     // last partner flag per map
    int sleep;          // sleep status
    
    neobox_filter *filter_fun;  // filter events
    void *filter_state;         // filter_fun state
    
    struct neobox_fb fb;
    struct neobox_iod iod;
    struct neobox_layout layout;
    struct neobox_parser parser;
    struct neobox_save **save; // button save array
    struct neobox_queue queue; // event queue
    struct neobox_timer timer; // timer queue
    struct neobox_config config;
};

#endif

