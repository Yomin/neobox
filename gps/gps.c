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
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include "tkbio.h"
#include "gps_layout.h"

#define NAME "gps"

int main(int argc, char* argv[])
{
    if(argc != 2) {
        printf("Usage: %s gps\n", argv[0]);
        return 0;
    }
    
    int ret = 0;
    struct pollfd pfds[2];
    struct tkbio_config config = tkbio_default_config();
    config.layout = gpsLayout;
    
    if((pfds[0].fd = tkbio_init_custom(NAME, config)) < 0)
        return pfds[0].fd;
    
    if((pfds[1].fd = open(argv[1], O_RDONLY)) < 0)
    {
        ret = errno;
        perror("Failed to open gps device");
        tkbio_finish();
        return 1;
    }
    
    pfds[0].events = POLLIN;
    pfds[1].events = POLLIN;
    
    while(1)
    {
        poll(pfds, 2, -1);
        
        if(pfds[0].revents & POLLIN)
        {
            switch(tkbio_handle_event().c[0])
            {
                case '+':
                    break;
                case '-':
                    break;
                case 'o':
                    break;
                case 'q':
                    goto end;
            }
        }
        else if(pfds[0].revents & POLLHUP)
        {
            printf("Failed to poll iod socket");
            ret = 2;
            break;
        }
        if(pfds[1].revents & POLLIN)
        {
            
        }
        else if(pfds[1].revents & POLLHUP)
        {
            printf("Failed to poll gps socket");
            ret = 3;
            break;
        }
    }
    
end:
    tkbio_finish();
    close(pfds[1].fd);
    
    return ret;
}

