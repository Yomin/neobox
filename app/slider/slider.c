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

#include <neobox.h>
#include <neobox_nop.h>
#include <neobox_slider.h>

#include "slider_layout.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int fd, tick;
char buf[100];

int handler(struct neobox_event event, void *state)
{
    int count, err;
    
    switch(event.type)
    {
    case NEOBOX_EVENT_INT:
        count = snprintf(buf, 100, "%i", event.value.i*tick);
        if(write(fd, buf, count) == -1)
        {
            err = errno;
            perror("Failed to write device");
            return NEOBOX_HANDLER_ERROR|err;
        }
        break;
    default:
        return NEOBOX_HANDLER_DEFER;
    }
    return NEOBOX_HANDLER_SUCCESS;
}

void check_num(char *str, char *name)
{
    if(strspn(str, "1234567890") != strlen(str))
    {
        printf("%s value not a number\n", name);
        neobox_finish();
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    int ret, err;
    
    if((ret = neobox_init_layout(sliderLayout, &argc, argv)) < 0)
        return ret;

    if(argc != 4)
    {
        printf("Usage: %s <max> <ticks> <file>\n", argv[0]);
        return 0;
    }
    
    check_num(argv[1], "max");
    check_num(argv[2], "ticks");
    
    tick = atoi(argv[1])/atoi(argv[2]);
    
    if((fd = open(argv[3], O_RDWR)) == -1)
    {
        err = errno;
        perror("Failed to open device");
        neobox_finish();
        return err;
    }
    
    if(read(fd, buf, 100) == -1)
    {
        err = errno;
        perror("Failed to read device");
        close(fd);
        neobox_finish();
        return err;
    }
    
    neobox_slider_set_ticks_pos(0, 0, atoi(argv[2]), atoi(buf)/tick, 0);
    
    neobox_nop_set_name(1, 0, argv[0], 0);
    
    ret = neobox_run(handler, 0);
    
    neobox_finish();
    
    close(fd);
    
    return ret;
}

