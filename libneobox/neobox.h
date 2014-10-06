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

#ifndef __NEOBOX_H__
#define __NEOBOX_H__

#include <sys/types.h>
#include <poll.h>
#include <signal.h>

#include "neobox_layout.h"

#define NEOBOX_ERROR_IOD_OPEN    -1
#define NEOBOX_ERROR_FB_OPEN     -2
#define NEOBOX_ERROR_FB_VINFO    -3
#define NEOBOX_ERROR_FB_FINFO    -4
#define NEOBOX_ERROR_FB_MMAP     -5
#define NEOBOX_ERROR_SCREEN_OPEN -6
#define NEOBOX_ERROR_SCREEN_POLL -7
#define NEOBOX_ERROR_SIGNAL      -8
#define NEOBOX_ERROR_POLL        -9
#define NEOBOX_ERROR_REGISTER    -10
#define NEOBOX_ERROR_CONFIG      -11

#define NEOBOX_FORMAT_LANDSCAPE  0
#define NEOBOX_FORMAT_PORTRAIT   1

#define NEOBOX_OPTION_NO_PRINT      0 // nothing visible
#define NEOBOX_OPTION_NORM_PRINT    1 // visibility derived from layout
#define NEOBOX_OPTION_FORCE_PRINT   2 // everything visible
#define NEOBOX_OPTION_PRINT_MASK    3
#define NEOBOX_OPTION_ADMINMAP      4 // enable admin goto in meta map

#define NEOBOX_MAP_DEFAULT       -1
#define NEOBOX_CONFIG_DEFAULT    0

#define NEOBOX_SYSTEM_PREV           0
#define NEOBOX_SYSTEM_NEXT           1
#define NEOBOX_SYSTEM_MENU           2
#define NEOBOX_SYSTEM_QUIT           3
#define NEOBOX_SYSTEM_ACTIVATE       4

#define NEOBOX_BUTTON_AUX            1
#define NEOBOX_BUTTON_POWER          2

#define NEOBOX_SET_SUCCESS           0
#define NEOBOX_SET_FAILURE           1

#define NEOBOX_EVENT_NOP            0
#define NEOBOX_EVENT_CHAR           1
#define NEOBOX_EVENT_INT            2
#define NEOBOX_EVENT_QUIT           3
#define NEOBOX_EVENT_ACTIVATE       4
#define NEOBOX_EVENT_DEACTIVATE     5
#define NEOBOX_EVENT_SIGNAL         6
#define NEOBOX_EVENT_SYSTEM         7
#define NEOBOX_EVENT_POLLIN         8
#define NEOBOX_EVENT_POLLOUT        9
#define NEOBOX_EVENT_POLLHUPERR     10
#define NEOBOX_EVENT_TIMER          11
#define NEOBOX_EVENT_REMOVE         12
#define NEOBOX_EVENT_BUTTON         13
#define NEOBOX_EVENT_LOCK           14
#define NEOBOX_EVENT_GRAB           15
#define NEOBOX_EVENT_POWERSAVE      16

#define NEOBOX_HANDLER_SUCCESS       0
#define NEOBOX_HANDLER_QUIT          1
#define NEOBOX_HANDLER_DEFER         2
#define NEOBOX_HANDLER_ERROR         (1<<7)

struct neobox_options
{
    char *fb, *iod, *config;
    struct neobox_layout layout;
    int format;
    int options;
    int verbose;
    int map;
};

struct neobox_event
{
    unsigned char type, id;
    union neobox_elem value;
};

int neobox_init_default(int *argc, char *argv[]);
int neobox_init_layout(struct neobox_layout layout, int *argc, char *argv[]);
int neobox_init_custom(struct neobox_options options);

struct neobox_options neobox_options_default(int *argc, char *argv[]);

void neobox_finish();

typedef int neobox_handler(struct neobox_event event, void *state);

int neobox_run(neobox_handler *handler, void *state);
int neobox_run_pfds(neobox_handler *handler, void *state, struct pollfd *pfds, int count);

int neobox_handle_event(neobox_handler *handler, void *state);
int neobox_handle_queue(neobox_handler *handler, void *state, sigset_t *oldset);

int neobox_catch_signal(int signal, int flags);

int neobox_sleep(unsigned int sec, unsigned int msec);
int neobox_timer(unsigned char id, unsigned int sec, unsigned int msec);
void neobox_timer_remove(unsigned char id);

void neobox_init_screen();

void neobox_switch(pid_t pid);
void neobox_hide(pid_t pid, int priority, int hide);
void neobox_powersave(int powersave);

int neobox_lock(int lock);
int neobox_grab(int button, int grab);

void neobox_map_set(int map);
void neobox_map_reset();

#endif
