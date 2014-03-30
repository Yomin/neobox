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

#include <sys/types.h>
#include <poll.h>
#include "tkbio_layout.h"

#define TKBIO_ERROR_RPC_OPEN    -1
#define TKBIO_ERROR_FB_OPEN     -2
#define TKBIO_ERROR_FB_VINFO    -3
#define TKBIO_ERROR_FB_FINFO    -4
#define TKBIO_ERROR_FB_MMAP     -5
#define TKBIO_ERROR_SCREEN_OPEN -6
#define TKBIO_ERROR_SCREEN_POLL -7
#define TKBIO_ERROR_SIGNAL      -8
#define TKBIO_ERROR_POLL        -9
#define TKBIO_ERROR_REGISTER    -10

#define TKBIO_FORMAT_LANDSCAPE  0
#define TKBIO_FORMAT_PORTRAIT   1

#define TKBIO_OPTION_NO_INITIAL_PRINT   1

#define TKBIO_SYSTEM_PREV           0
#define TKBIO_SYSTEM_NEXT           1
#define TKBIO_SYSTEM_MENU           2
#define TKBIO_SYSTEM_QUIT           3
#define TKBIO_SYSTEM_ACTIVATE       4

#define TKBIO_BUTTON_AUX            1
#define TKBIO_BUTTON_POWER          2

#define TKBIO_SET_FAILURE           0
#define TKBIO_SET_SUCCESS           1

#define TKBIO_RETURN_NOP            0
#define TKBIO_RETURN_CHAR           1
#define TKBIO_RETURN_INT            2
#define TKBIO_RETURN_QUIT           3
#define TKBIO_RETURN_ACTIVATE       4
#define TKBIO_RETURN_DEACTIVATE     5
#define TKBIO_RETURN_SIGNAL         6
#define TKBIO_RETURN_SYSTEM         7
#define TKBIO_RETURN_POLLIN         8
#define TKBIO_RETURN_POLLOUT        9
#define TKBIO_RETURN_POLLHUPERR     10
#define TKBIO_RETURN_TIMER          11
#define TKBIO_RETURN_REMOVE         12
#define TKBIO_RETURN_BUTTON         13
#define TKBIO_RETURN_LOCK           14
#define TKBIO_RETURN_GRAB           15
#define TKBIO_RETURN_POWERSAVE      16

#define TKBIO_HANDLER_SUCCESS       0
#define TKBIO_HANDLER_QUIT          1
#define TKBIO_HANDLER_DEFER         2
#define TKBIO_HANDLER_ERROR         (1<<7)

struct tkbio_config
{
    char *fb, *tsp;
    struct tkbio_layout layout;
    int format;
    int options;
    int verbose;
};

struct tkbio_return
{
    unsigned char type, id;
    union tkbio_elem value;
};

int tkbio_init_default(int *argc, char *argv[]);
int tkbio_init_layout(struct tkbio_layout layout, int *argc, char *argv[]);
int tkbio_init_custom(struct tkbio_config config);

struct tkbio_config tkbio_config_default(int *argc, char *argv[]);

void tkbio_finish();

typedef int tkbio_handler(struct tkbio_return ret, void *state);

int tkbio_run(tkbio_handler *handler, void *state);
int tkbio_run_pfds(tkbio_handler *handler, void *state, struct pollfd *pfds, int count);

int tkbio_handle_event(tkbio_handler *handler, void *state);
int tkbio_handle_queue(tkbio_handler *handler, void *state);

int tkbio_catch_signal(int signal, int flags);

int tkbio_timer(unsigned char id, unsigned int sec, unsigned int usec);

void tkbio_init_screen();

void tkbio_switch(pid_t pid);
void tkbio_hide(pid_t pid, int priority, int hide);
void tkbio_powersave(int powersave);

int tkbio_lock(int lock);
int tkbio_grab(int button, int grab);

#endif
