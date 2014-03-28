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

#define TSP_NAME    "tspd"
#define TSP_PWD     "/var/" TSP_NAME
#define TSP_RPC     "rpc"

#define TSP_CMD_REGISTER    1
#define TSP_CMD_REMOVE      2
#define TSP_CMD_SWITCH      3
#define TSP_CMD_LOCK        4

#define TSP_SWITCH_PID      0
#define TSP_SWITCH_PREV     1
#define TSP_SWITCH_NEXT     2

#define TSP_EVENT_PRESSED   1
#define TSP_EVENT_MOVED     2
#define TSP_EVENT_ACTIVATED 4

struct tsp_cmd
{
    unsigned char cmd;
    pid_t pid;
    int value;
};

struct tsp_event
{
    unsigned char event;
    int y, x;
};

#endif
