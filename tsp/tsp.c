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

#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <signal.h>

#define BYTES_PER_CMD   16
#define MIN_PIXEL       100
#define NAME            "tspd"
#define MSB             (1<<(sizeof(int)*8-1))
#define SMSB            (1<<(sizeof(int)*8-2))
#define NIBBLE          ((sizeof(int)/2)*8)

#define TYPE_ID 0
#define TYPE_FD 1

#ifdef NDEBUG
#   define DEBUG
#else
#   define DEBUG(x) x
#endif

struct pollfd *pfds;
int sock_count, fd_active;

struct chain_socket
{
    LIST_ENTRY(chain_socket) chain;
    int id, fd, prev_fd;
};
LIST_HEAD(sockets, chain_socket) socklist;

struct chain_socket* find_socket(int type, int id)
{
    struct chain_socket *s = socklist.lh_first;
    while(s)
    {
        if(type == TYPE_ID && s->id == id)
            return s;
        if(type == TYPE_FD && s->fd == id)
            return s;
        s = s->chain.le_next;
    }
    return 0;
}

void delete_socket(struct chain_socket *s)
{
    char buf[256];
    snprintf(buf, 256, "/var/" NAME "/socket_%i", s->id);
    if(remove(buf) < 0)
        DEBUG(perror("Failed to remove socket"));
    LIST_REMOVE(s, chain);
    free(s);
}

void add_socket(int id)
{
    struct chain_socket *s = find_socket(TYPE_ID, id);
    if(s)
    {
        DEBUG(printf("Socket exists: %i\n", id));
        return;
    }
    else
        DEBUG(printf("Socket added: %i\n", id));
    
    s = malloc(sizeof(struct chain_socket));
    LIST_INSERT_HEAD(&socklist, s, chain);
    s->id = id;
    
    char buf[256];
    snprintf(buf, 256, "/var/" NAME "/socket_%i", id);
    if(mknod(buf, S_IFIFO|0666, 0) < 0 && errno != EEXIST)
    {
        perror("Failed to create socket");
        return;
    }
}

void activate_socket(int id)
{
    struct chain_socket *s = find_socket(TYPE_ID, id);
    if(!s)
    {
        DEBUG(printf("Failed to find socket"));
        return;
    }
    
    char buf[256];
    snprintf(buf, 256, "/var/" NAME "/socket_%i", id);
    if((s->fd = open(buf, O_WRONLY|O_NONBLOCK)) < 0)
    {
        if(errno == ENXIO)
        {
            DEBUG(printf("Client not listening\n"));
            delete_socket(s);
        }
        else
            DEBUG(perror("Failed to open socket"));
        return;
    }
    
    sock_count++;
    pfds = realloc(pfds, sizeof(struct pollfd)*(sock_count+2));
    pfds[sock_count+1].fd = s->fd;
    pfds[sock_count+1].events = 0;
    
    s->prev_fd = fd_active;
    fd_active = s->fd;
}

void del_socket(int fd, int pos)
{
    struct chain_socket *s = find_socket(TYPE_FD, fd);
    DEBUG(printf("Socket removed: %i", s->id));
    
    if(fd == fd_active)
    {
        if(s->prev_fd != -1)
        {
#ifndef NDEBUG
            struct chain_socket *s2 = find_socket(TYPE_FD, s->prev_fd);
            printf(", active: %i\n", s2->id);
#endif
            fd_active = s->prev_fd;
        }
        else
        {
            DEBUG(printf(", active: none\n"));
            fd_active = -1;
        }
    }
    else
        DEBUG(printf("\n"));
    
    close(s->fd);
    delete_socket(s);
    
    memmove(pfds+pos, pfds+pos+1, sizeof(struct pollfd)*(sock_count-pos+1));
    sock_count--;
    pfds = realloc(pfds, sizeof(struct pollfd)*(sock_count+2));
}

void cleanup(int start)
{
    int i;
    for(i=start; i<sock_count+2; i++)
        close(pfds[i].fd);
    free(pfds);
}

void signal_handler(int signal)
{
    switch(signal)
    {
    case SIGINT:
        cleanup(0);
        break;
    case SIGPIPE:
    {
        int i;
        for(i=2; i<sock_count+2; i++)
            if(pfds[i].fd == fd_active)
            {
                del_socket(fd_active, i);
                break;
            }
        break;
    }
    }
}

void usage(const char *name)
{
    printf("Usage: %s [-f] <screen>\n", name);
}

int main(int argc, char* argv[])
{
    int opt;
    int daemon = 1;
    char *screen;
    
    while((opt = getopt(argc, argv, "f")) != -1)
    {
        switch(opt)
        {
        case 'f':
            daemon = 0;
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
    
    screen = argv[optind];
    
    if(mkdir("/var/" NAME, 0755) == -1 && errno != EEXIST)
    {
        perror("Failed to create working dir");
        return 7;
    }
    
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
        
        chdir("/var/" NAME);
        
        if((tmp = open("pid", O_WRONLY|O_CREAT, 755)) < 0)
        {
            perror("Failed to open pidfile");
            return 2;
        }
        
        pid_t pid = getpid();
        char buf[10];
        int count = sprintf(buf, "%i", pid);
        write(tmp, &buf, count*sizeof(char));
        
        if(lockf(tmp, F_TLOCK, -count*sizeof(char)) < 0)
        {
            perror("Deamon already running");
            return 3;
        }
    }
    
    int fd_sc, fd_rpc;
    fd_active = -1;
    
    if((fd_sc = open(screen, O_RDONLY)) < 0)
    {
        perror("Failed to open screen socket");
        return 4;
    }
    
    if(mknod("/var/" NAME "/rpc", S_IFIFO|0666, 0) < 0 && errno != EEXIST)
    {
        perror("Failed to create rpc socket");
        close(fd_sc);
        return 5;
    }
    
    if((fd_rpc = open("/var/" NAME "/rpc", O_RDONLY|O_NONBLOCK)) < 0)
    {
        perror("Failed to open rpc socket");
        close(fd_sc);
        return 6;
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, signal_handler);
    
    LIST_INIT(&socklist);
    
    pfds = malloc(sizeof(struct pollfd)*2);
    pfds[0].fd = fd_sc;
    pfds[0].events = POLLIN;
    pfds[1].fd = fd_rpc;
    pfds[1].events = POLLIN;
    
    unsigned char buf[BYTES_PER_CMD];
    int pressed = 0;
    int y = 0, x = 0;
    sock_count = 0;
    
    DEBUG(printf("Ready\n"));
    
    while(1)
    {
        poll(pfds, 2, -1);
        
        if(pfds[0].revents & POLLIN)
        {
            read(fd_sc, buf, BYTES_PER_CMD);
            
            if(buf[8] == 0x01 && buf[9] == 0x00 && buf[10] == 0x4A && buf[11] == 0x01)
            {
                DEBUG(printf("Button"));
                switch(buf[12])
                {
                    case 0x00:
                        if(pressed)
                        {
                            DEBUG(printf(" released (%i,%i)\n", y, x));
                            pressed = 0;
                            int msg = x | y << NIBBLE | MSB;
                            if(fd_active >= 0)
                                write(fd_active, &msg, sizeof(int));
                        }
                        break;
                    case 0x01:
                        if(!pressed)
                        {
                            DEBUG(printf(" pressed (%i,%i)\n", y, x));
                            pressed = 1;
                            int msg = x | y << NIBBLE;
                            if(fd_active >= 0)
                                write(fd_active, &msg, sizeof(int));
                        }
                        break;
                    default:
                        DEBUG(printf(" unrecognized (%i,%i)\n", y, x));
                }
            }
            else if(buf[8] == 0x03 && buf[9] == 0x00 && buf[10] == 0x00 && buf[11] == 0x00)
            {
                y = buf[13]*256+buf[12]-MIN_PIXEL;
            }
            else if(buf[8] == 0x03 && buf[9] == 0x00 && buf[10] == 0x01 && buf[11] == 0x00)
            {
                x = buf[13]*256+buf[12]-MIN_PIXEL;
                if(pressed)
                {
                    int msg = x | y << NIBBLE | SMSB;
                    DEBUG(printf("Move (%i,%i)\n", y, x));
                    if(fd_active >= 0)
                        write(fd_active, &msg, sizeof(int));
                }
            }
            
        }
        else if(pfds[0].revents & POLLHUP)
        {
            DEBUG(printf("POLLHUP on screen socket\n"));
            close(fd_sc);
            if((fd_sc = open(argv[1], O_RDONLY)) < 0)
            {
                perror("Failed to open screen socket");
                cleanup(2);
                close(fd_rpc);
                return 4;
            }
        }
        else if(pfds[1].revents & POLLIN)
        {
            int id;
            read(fd_rpc, &id, sizeof(int));
            if(id & MSB)
                activate_socket(id & ~MSB);
            else
                add_socket(id);
        }
        else if(pfds[1].revents & POLLHUP)
        {
            DEBUG(printf("POLLHUP on rpc socket\n"));
            close(fd_rpc);
            if((fd_rpc = open("/var/" NAME "/rpc", O_RDONLY|O_NONBLOCK)) < 0)
            {
                perror("Failed to open rpc socket");
                cleanup(2);
                close(fd_sc);
                return 6;
            }
        }
        else
        {
            int i;
            for(i=2; i<sock_count+2; i++)
                if(pfds[i].revents & POLLHUP)
                {
                    del_socket(pfds[i].fd, i);
                    i--;
                }
        }
    }
    
    cleanup(0);
    
    return 0;
}

