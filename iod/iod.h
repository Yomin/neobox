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

#ifndef __IOD_H__
#define __IOD_H__

#include <sys/types.h>

#define IOD_NAME    "iod"
#define IOD_PWD     "/var/" IOD_NAME
#define IOD_RPC     "rpc"

#define IOD_CMD_REGISTER    1   // register pid
#define IOD_CMD_REMOVE      2   // remove app
#define IOD_CMD_SWITCH      3   // switch to app
#define IOD_CMD_LOCK        4   // lock screen
#define IOD_CMD_HIDE        5   // hide app
#define IOD_CMD_ACK         6   // ack remove/deactivate
#define IOD_CMD_GRAB        7   // get exclusive button
#define IOD_CMD_POWERSAVE   8   // broadcast powersave request

#define IOD_SWITCH_PID      0       // switch to app
#define IOD_SWITCH_PREV     1       // switch to prev app
#define IOD_SWITCH_NEXT     2       // switch to next app
#define IOD_SWITCH_HIDDEN   3       // switch to hidden app (lowest prio)
#define IOD_HIDE_MASK       (1<<7)  // hide bit|prio
#define IOD_GRAB_AUX        0       // grab/ungrab aux
#define IOD_GRAB_POWER      1       // grab/ungrab power
#define IOD_GRAB_MASK       (1<<7)  // grab bit|button

#define IOD_EVENT_PRESSED       0   // button pressed
#define IOD_EVENT_RELEASED      1   // button released
#define IOD_EVENT_MOVED         2   // moved while button pressed
#define IOD_EVENT_ACTIVATED     3   // app activated
#define IOD_EVENT_DEACTIVATED   4   // app deactivated
#define IOD_EVENT_REMOVED       5   // app removed
#define IOD_EVENT_AUX           6   // aux pressed/released
#define IOD_EVENT_POWER         7   // power pressed/released
#define IOD_EVENT_LOCK          8   // screen locked/unlocked
#define IOD_EVENT_GRAB          9   // button grabbed/ungrabbed
#define IOD_EVENT_POWERSAVE     10  // powersave request

#define IOD_SUCCESS_MASK    (1<<7)  // lock/grab success

struct iod_cmd
{
    unsigned char cmd;
    pid_t pid;
    int value;
} __attribute__((packed));

union iod_value
{
    struct { short int y, x; } cord;
    int status;
};

struct iod_event
{
    unsigned char event;
    union iod_value value;
} __attribute__((packed));

#endif
