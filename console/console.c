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

#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "tkbio.h"

#define NAME "neobox-console"

inline static int insert(int fd, char c)
{
    int err;
    
    if(ioctl(fd, TIOCSTI, &c) < 0)
    {
        err = errno;
        perror("Failed to insert byte into input queue");
        return err;
    }
    return 0;
}

int handler(struct tkbio_return ret, void *state)
{
    int fd = *(int*) state;
    char *ptr = ret.value.c.c;
    int i = 0, err;
    
    switch(ret.type)
    {
    case TKBIO_RETURN_CHAR:
        break;
    default:
        return TKBIO_HANDLER_DEFER;
    }
    
    // fk 0 hack
    if(!ptr[0] && ptr[1])
    {
        if((err = insert(fd, 27)))
            return TKBIO_HANDLER_ERROR|err;
        if((err = insert(fd, '[')))
            return TKBIO_HANDLER_ERROR|err;
        ptr++;
        i++;
    }
    
    while(*ptr && i++ < TKBIO_CHARELEM_MAX)
    {
        if((err = insert(fd, *ptr)))
            return TKBIO_HANDLER_ERROR|err;
        ptr++;
    }
    
    return TKBIO_HANDLER_SUCCESS;
}

void usage(char *name)
{
    printf("Usage: %s [-s] [-d] <tty>\n", name);
}

int main(int argc, char* argv[])
{
    int opt;
    int show = 0;
    int daemon = 0;
    char *tty;
    
    struct tkbio_config config = tkbio_config_default(&argc, argv);
    
    while((opt = getopt(argc, argv, "sd")) != -1)
    {
        switch(opt)
        {
        case 's':
            show = 1;
            break;
        case 'd':
            daemon = 1;
            break;
        default:
            usage(argv[0]);
            return 0;
        }
    }
    
    if(optind == argc)
    {
        usage(argv[0]);
        return 0;
    }
    
    tty = argv[optind];
    
    if(daemon)
    {
        int tmp = fork();
        if(tmp < 0)
        {
            perror("Failed to fork process");
            return 1;
        }
        if(tmp > 0)
        {
            return 0;
        }
        
        setsid();
        
        close(0); close(1); close(2);
        tmp = open("/dev/null", O_RDWR); dup(tmp); dup(tmp);
        
        if(mkdir("/var/" NAME, 0755) == -1 && errno != EEXIST)
        {
            perror("Failed to create working dir");
            return 2;
        }
        chdir("/var/" NAME);
        
        if((tmp = open("pid", O_WRONLY|O_CREAT, 755)) < 0)
        {
            perror("Failed to open pidfile");
            return 3;
        }
        
        pid_t pid = getpid();
        char buf[10];
        int count = sprintf(buf, "%i", pid);
        write(tmp, &buf, count*sizeof(char));
        close(tmp);
    }
    
    int fd;
    if(!show)
        config.options |= TKBIO_OPTION_NO_INITIAL_PRINT;
    
    if((fd = tkbio_init_custom(config)) < 0)
        return fd;
    
    if((fd = open(tty, O_RDONLY)) < 0)
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

