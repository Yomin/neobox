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

#ifndef __TSP_H__
#define __TSP_H__

#include <sys/types.h>

#define TSP_NAME    "tspd"
#define TSP_PWD     "/var/" TSP_NAME
#define TSP_RPC     "rpc"

#define TSP_CMD_REGISTER    1   // register pid
#define TSP_CMD_REMOVE      2   // remove app
#define TSP_CMD_SWITCH      3   // switch to app
#define TSP_CMD_LOCK        4   // lock screen
#define TSP_CMD_HIDE        5   // hide app
#define TSP_CMD_ACK         6   // ack remove/deactivate
#define TSP_CMD_GRAB        7   // get exclusive button
#define TSP_CMD_POWERSAVE   8   // broadcast powersave request

#define TSP_SWITCH_PID      0       // switch to app
#define TSP_SWITCH_PREV     1       // switch to prev app
#define TSP_SWITCH_NEXT     2       // switch to next app
#define TSP_SWITCH_HIDDEN   3       // switch to hidden app (lowest prio)
#define TSP_HIDE_MASK       (1<<7)  // hide bit|prio
#define TSP_GRAB_AUX        0       // grab/ungrab aux
#define TSP_GRAB_POWER      1       // grab/ungrab power
#define TSP_GRAB_MASK       (1<<7)  // grab bit|button

#define TSP_EVENT_PRESSED       0   // button pressed
#define TSP_EVENT_RELEASED      1   // button released
#define TSP_EVENT_MOVED         2   // moved while button pressed
#define TSP_EVENT_ACTIVATED     3   // app activated
#define TSP_EVENT_DEACTIVATED   4   // app deactivated
#define TSP_EVENT_REMOVED       5   // app removed
#define TSP_EVENT_AUX           6   // aux pressed/released
#define TSP_EVENT_POWER         7   // power pressed/released
#define TSP_EVENT_LOCK          8   // screen locked/unlocked
#define TSP_EVENT_GRAB          9   // button grabbed/ungrabbed
#define TSP_EVENT_POWERSAVE     10  // powersave request

#define TSP_SUCCESS_MASK    (1<<7)  // lock/grab success

struct tsp_cmd
{
    unsigned char cmd;
    pid_t pid;
    int value;
} __attribute__((packed));

union tsp_value
{
    struct { short int y, x; } cord;
    int status;
};

struct tsp_event
{
    unsigned char event;
    union tsp_value value;
} __attribute__((packed));

#endif
