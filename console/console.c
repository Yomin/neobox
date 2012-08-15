/*
 * Copyright (c) 2012 Martin Rödel aka Yomin
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
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "tkbio.h"

#define NAME "console"

int handler(struct tkbio_charelem elem, void *state)
{
    int fd = *(int*) state;
    char *ptr = elem.c;
    int i = 0;
    while(*ptr && i++ < TKBIO_CHARELEM_MAX)
    {
        if(ioctl(fd, TIOCSTI, ptr) < 0)
        {
            int e = errno;
            perror("Failed to insert byte into input queue");
            return e;
        }
        ptr++;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if(argc != 2) {
        printf("Usage: %s tty\n", argv[0]);
        return 0;
    }
    
    int fd;
    
    if((fd = tkbio_init_default(NAME)) < 0)
        return fd;
    
    if((fd = open(argv[1], O_RDONLY)) < 0)
    {
        int e = errno;
        perror("Failed to open tty");
        tkbio_finish();
        return e;
    }
    
    int ret = tkbio_run(handler, &fd);
    tkbio_finish();
    close(fd);
    return ret;
}

