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

#include <neobox.h>
#include <neobox_config.h>
#include <neobox_log.h>

inline static int insert(int fd, char c)
{
    int err;
    
    if(ioctl(fd, TIOCSTI, &c) < 0)
    {
        err = errno;
        neobox_app_perror("Failed to insert byte into input queue");
        return err;
    }
    return 0;
}

int handler(struct neobox_event event, void *state)
{
    int fd = *(int*) state;
    char *ptr = event.value.c.c;
    int i = 0, err;
    
    switch(event.type)
    {
    case NEOBOX_EVENT_CHAR:
        break;
    default:
        return NEOBOX_HANDLER_DEFER;
    }
    
    // fk 0 hack
    if(!ptr[0] && ptr[1])
    {
        if((err = insert(fd, 27)))
            return NEOBOX_HANDLER_ERROR|err;
        if((err = insert(fd, '[')))
            return NEOBOX_HANDLER_ERROR|err;
        ptr++;
        i++;
    }
    
    while(*ptr && i++ < NEOBOX_CHARELEM_MAX)
    {
        if((err = insert(fd, *ptr)))
            return NEOBOX_HANDLER_ERROR|err;
        ptr++;
    }
    
    return NEOBOX_HANDLER_SUCCESS;
}

void usage(char *name)
{
    neobox_app_printf("Usage: %s [-s] [tty]\n", name);
}

int main(int argc, char* argv[])
{
    int opt, fd, err, ret;
    int show = 0;
    char *tty = 0;
    
    struct neobox_options options = neobox_options_default(&argc, argv);
    
    while((opt = getopt(argc, argv, "s")) != -1)
    {
        switch(opt)
        {
        case 's':
            show = 1;
            break;
        default:
            usage(argv[0]);
            return 0;
        }
    }
    
    if(optind < argc)
        tty = argv[optind];
    
    options.options &= ~NEOBOX_OPTION_PRINT_MASK;
    options.options |= show ? NEOBOX_OPTION_FORCE_PRINT :
        NEOBOX_OPTION_NO_PRINT;
    options.options |= NEOBOX_OPTION_ADMINMAP;
    
    if((ret = neobox_init_custom(options)) < 0)
        return ret;
    
    if(!tty && !(tty = neobox_config("tty", 0)))
    {
        neobox_app_fprintf(stderr, "TTY not configured\n");
        neobox_finish();
        return 1;
    }
    
    if((fd = open(tty, O_RDONLY)) < 0)
    {
        err = errno;
        neobox_app_perror("Failed to open tty");
        neobox_finish();
        return err;
    }
    
    ret = neobox_run(handler, &fd);
    neobox_finish();
    close(fd);
    
    return ret;
}

