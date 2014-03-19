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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <tkbio.h>
#include <tkbio_slider.h>

#include "slider_layout.h"

#define NAME "slider"

int fd, tick;
char buf[100];

int handler(struct tkbio_return ret, void *state)
{
    int count;
    
    switch(ret.type)
    {
    case TKBIO_RETURN_INT:
        count = snprintf(buf, 100, "%i", ret.value.i*tick);
        write(fd, buf, count);
        break;
    }
    return 0;
}

void check_num(char *str, char *name)
{
    if(strspn(str, "1234567890") != strlen(str))
    {
        printf("%s value not a number\n", name);
        tkbio_finish();
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    int err;
    
    tkbio_init_layout_args(NAME, sliderLayout, &argc, argv);

    if(argc != 4) {
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
        tkbio_finish();
        return err;
    }
    
    read(fd, buf, 100);
    tkbio_slider_set_ticks_pos(0, 0, atoi(argv[2]), atoi(buf)/tick);

    tkbio_run(handler, 0);
    tkbio_finish();
    
    close(fd);
    
    return 0;
}

