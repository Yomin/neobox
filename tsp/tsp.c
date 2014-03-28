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

#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/queue.h>
#include <sys/stat.h>

#include "tsp.h"

#define BYTES_PER_CMD   16
#define MIN_PIXEL       100

#ifdef NDEBUG
#   define DEBUG(x)
#else
#   define DEBUG(x) x
#endif


struct chain_socket
{
    CIRCLEQ_ENTRY(chain_socket) chain;
    int sock, priority, hide;
    pid_t pid;
};
CIRCLEQ_HEAD(client_sockets, chain_socket);


int client_count, pfds_cap;
struct client_sockets client_list;
char *pwd;
struct pollfd *pfds;


int open_rpc_socket(int *sock, struct sockaddr_un *addr)
{
    addr->sun_family = AF_UNIX;
    sprintf(addr->sun_path, "%s/%s", pwd, TSP_RPC);
    
    if((*sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to open rpc socket");
        return 6;
    }
    
    if(unlink(addr->sun_path) == -1 && errno != ENOENT)
    {
        perror("Failed to unlink rpc socket file");
        return 7;
    }
    
    if(bind(*sock, (struct sockaddr*)addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("Failed to bind rpc socket");
        return 8;
    }
    
    listen(*sock, 10);
    
    return 0;
}

void add_client(int fd)
{
    struct chain_socket *cs = malloc(sizeof(struct chain_socket));
    CIRCLEQ_INSERT_HEAD(&client_list, cs, chain);
    cs->sock = fd;
    cs->pid = 0;
    if(client_count+2 == pfds_cap)
    {
        pfds_cap += 10;
        pfds = realloc(pfds, pfds_cap*sizeof(struct pollfd));
    }
    pfds[client_count+2].fd = fd;
    pfds[client_count+2].events = POLLIN;
    client_count++;
    DEBUG(printf("Client added [%i]\n", fd));
}

void send_client(unsigned char event, int y, int x, struct chain_socket *cs)
{
    struct tsp_event evnt;
    
    if(client_list.cqh_first == (void*)&client_list)
        return;
    
    evnt.event = event;
    evnt.y = y;
    evnt.x = x;
    
    if(!cs)
        cs = client_list.cqh_first;
    
    send(cs->sock, &evnt, sizeof(struct tsp_event), 0);
}

struct chain_socket* find_client(int pos, pid_t pid)
{
    struct chain_socket *cs = client_list.cqh_first;
    
    if(pid)
    {
        while(cs != (void*)&client_list && cs->pid != pid)
            cs = cs->chain.cqe_next;
        if(cs == (void*)&client_list)
            return 0;
    }
    else
    {
        while(cs != (void*)&client_list && cs->sock != pfds[pos].fd)
            cs = cs->chain.cqe_next;
        if(cs == (void*)&client_list)
            return 0;
    }
    
    return cs;
}

struct chain_socket* client_list_rotate(int dir)
{
    client_list.cqh_last->chain.cqe_next = client_list.cqh_first;
    client_list.cqh_first->chain.cqe_prev = client_list.cqh_last;
    if(dir > 0)
    {
        client_list.cqh_last = client_list.cqh_first;
        client_list.cqh_first = client_list.cqh_first->chain.cqe_next;
    }
    else
    {
        client_list.cqh_first = client_list.cqh_last;
        client_list.cqh_last = client_list.cqh_last->chain.cqe_prev;
    }
    client_list.cqh_last->chain.cqe_next = (void*)&client_list;
    client_list.cqh_first->chain.cqe_prev = (void*)&client_list;
    
    return client_list.cqh_first;
}

struct chain_socket* client_list_rotate_next(int hidden)
{
    struct chain_socket *cs, *cs_nxt;
    
    cs = cs_nxt = client_list.cqh_first;
    
    if(cs == (void*)&client_list)
        return 0;
    
    while(cs != (void*)&client_list)
    {
        if(!cs->hide && !hidden)
        {
            cs_nxt = cs;
            break;
        }
        if(cs->hide)
        {
            if(!cs_nxt->hide || cs->priority > cs_nxt->priority)
                cs_nxt = cs;
        }
        cs = cs->chain.cqe_next;
    }
    
    if(hidden && !cs_nxt->hide)
        return 0;
    
    if(client_list.cqh_first != cs_nxt)
        while(client_list_rotate(+1) != cs_nxt);
    
    return cs_nxt;
}

int rem_client(int pos, pid_t pid)
{
    struct chain_socket *cs = client_list.cqh_first;
    int fd, active;
    
    if(!(cs = find_client(pos, pid)))
        return 0;
    
    fd = cs->sock;
    pid = cs->pid;
    active = cs == client_list.cqh_first;
    
    memcpy(pfds+pos, pfds+pos+1, client_count+2-pos-1);
    CIRCLEQ_REMOVE(&client_list, cs, chain);
    free(cs);
    client_count--;
    
    if(client_list.cqh_first != (void*)&client_list)
    {
        if(active)
        {
            if((cs = client_list_rotate_next(0))->hide)
                DEBUG(printf("Active client removed [%i] %i, switch invisible [%i] %i\n",
                    fd, pid, cs->sock, cs->pid));
            else
                DEBUG(printf("Active client removed [%i] %i, switch [%i] %i\n",
                    fd, pid, cs->sock, cs->pid));
            return 1;
        }
        else
        {
            DEBUG(printf("Client removed [%i] %i\n", fd, pid));
            return 0;
        }
    }
    else
    {
        DEBUG(printf("Active client removed [%i] %i, last\n", fd, pid));
        return 0;
    }
}

int switch_client(int cmd, int pos, pid_t pid)
{
    struct chain_socket *cs = client_list.cqh_first, *cs2;
    int dir = +1;
    
    if(cs == (void*)&client_list || cs->chain.cqe_next == (void*)&client_list)
        return 0;
    
    switch(cmd)
    {
    case TSP_SWITCH_PID:
        if(pid)
        {
            if(cs->pid == pid)
                return 0;
            while((cs2 = client_list_rotate(+1))->pid != pid && cs != cs2);
            if(cs == cs2)
                return 0;
        }
        else
        {
            if(cs->sock == pfds[pos].fd)
                return 0;
            while((cs2 = client_list_rotate(+1))->sock != pfds[pos].fd && cs != cs2);
            if(cs == cs2)
                return 0;
        }
        break;
    case TSP_SWITCH_PREV:
        dir = -1;
    case TSP_SWITCH_NEXT:
        while((cs2 = client_list_rotate(dir))->hide && cs != cs2);
        if(cs == cs2)
            return 0;
        break;
    case TSP_SWITCH_HIDDEN:
        if(!(cs2 = client_list_rotate_next(1)) || cs == cs2)
            return 0;
        break;
    }
    
    DEBUG(printf("Client switch [%i] %i\n",
        client_list.cqh_first->sock, client_list.cqh_first->pid));
    
    return 1;
}

void register_client(int pos, pid_t pid)
{
    struct chain_socket *cs;
    
    if(!(cs = find_client(pos, 0)))
        return;
    
    cs->pid = pid;
    
    DEBUG(printf("Client registered [%i] %i\n", cs->sock, pid));
}

void hide_client(int pos, pid_t pid, int priority, int hide)
{
    struct chain_socket *cs;
    
    if(!(cs = find_client(pos, pid)))
        return;
    
    cs->priority = priority;
    cs->hide = hide;
    
    DEBUG(printf("Client set %s (%i) [%i] %i\n",
        hide ? "invisible" : "visible", priority, cs->sock, cs->pid));
}

void free_clients()
{
    struct chain_socket *cs;
    while((cs = client_list.cqh_first) != (void*)&client_list)
    {
        CIRCLEQ_REMOVE(&client_list, cs, chain);
        free(cs);
    }
}

void signal_handler(int signal)
{
    DEBUG(printf("cleanup\n"));
    free_clients();
    free(pfds);
    exit(0);
}

void usage(const char *name)
{
    printf("Usage: %s [-f] [-d pwd] <screen>\n", name);
}

int main(int argc, char* argv[])
{
    int opt, ret;
    int daemon = 1, lock = 0;
    int pressed, y, x;
    char *screen_dev;
    int screen_fd, rpc_sock;
    struct sockaddr_un rpc_addr;
    
    unsigned char screen_buf[BYTES_PER_CMD];
    struct tsp_cmd cmd;
    struct chain_socket *cs;
    
    y = x = pressed = 0;
    pwd = TSP_PWD;
    
    while((opt = getopt(argc, argv, "fd:")) != -1)
    {
        switch(opt)
        {
        case 'f':
            daemon = 0;
            break;
        case 'd':
            pwd = optarg;
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
    
    screen_dev = argv[optind];
    
    if(mkdir(pwd, 0777) == -1 && errno != EEXIST)
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
        
        chdir(pwd);
        
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
            perror("Daemon already running");
            return 3;
        }
    }
    
    if((screen_fd = open(screen_dev, O_RDONLY)) < 0)
    {
        perror("Failed to open screen socket");
        return 4;
    }
    
    if((ret = open_rpc_socket(&rpc_sock, &rpc_addr)))
        return ret;
    
    pfds_cap = 10;
    pfds = malloc(pfds_cap*sizeof(struct pollfd));
    pfds[0].fd = screen_fd;
    pfds[0].events = POLLIN;
    pfds[1].fd = rpc_sock;
    pfds[1].events = POLLIN;
    
    client_count = 0;
    CIRCLEQ_INIT(&client_list);
    
    signal(SIGINT, signal_handler);
    
    DEBUG(printf("Ready\n"));
    
    while(1)
    {
        poll(pfds, client_count+2, -1);
        
        if(pfds[0].revents & POLLIN)
        {
            read(screen_fd, screen_buf, BYTES_PER_CMD);
            
            if(lock)
                continue;
            
            if( screen_buf[8] == 0x01 && screen_buf[9] == 0x00 &&
                screen_buf[10] == 0x4A && screen_buf[11] == 0x01)
            {
                DEBUG(printf("Button"));
                switch(screen_buf[12])
                {
                    case 0x00:
                        if(pressed)
                        {
                            DEBUG(printf(" released (%i,%i)\n", y, x));
                            send_client(TSP_EVENT_RELEASED, y, x, 0);
                            pressed = 0;
                        }
                        else
                            DEBUG(printf(" already released (%i,%i)\n", y, x));
                        break;
                    case 0x01:
                        if(!pressed)
                        {
                            DEBUG(printf(" pressed (%i,%i)\n", y, x));
                            send_client(TSP_EVENT_PRESSED, y, x, 0);
                            pressed = 1;
                        }
                        else
                            DEBUG(printf(" already pressed (%i,%i)\n", y, x));
                        break;
                    default:
                        DEBUG(printf(" unrecognized (%i,%i)\n", y, x));
                }
            }
            else if(screen_buf[8] == 0x03 && screen_buf[9] == 0x00 &&
                    screen_buf[10] == 0x00 && screen_buf[11] == 0x00)
            {
                y = screen_buf[13]*256+screen_buf[12]-MIN_PIXEL;
            }
            else if(screen_buf[8] == 0x03 && screen_buf[9] == 0x00 &&
                    screen_buf[10] == 0x01 && screen_buf[11] == 0x00)
            {
                x = screen_buf[13]*256+screen_buf[12]-MIN_PIXEL;
                if(pressed)
                {
                    DEBUG(printf("Move (%i,%i)\n", y, x));
                    send_client(TSP_EVENT_MOVED, y, x, 0);
                }
            }
        }
        else if(pfds[0].revents & POLLHUP)
        {
            DEBUG(printf("pollhup on screen socket\n"));
            close(screen_fd);
            if((screen_fd = open(screen_dev, O_RDONLY)) < 0)
            {
                perror("Failed to open screen socket");
                close(rpc_sock);
                free_clients();
                return 4;
            }
        }
        else if(pfds[1].revents & POLLIN)
        {
            int client;
            if((client = accept(rpc_sock, 0, 0)) == -1)
            {
                DEBUG(perror("Failed to accept client"));
                continue;
            }
            cs = client_list.cqh_first;
            add_client(client);
            if(cs != (void*)&client_list)
                send_client(TSP_EVENT_DEACTIVATED, 0, 0, cs);
            else
                send_client(TSP_EVENT_ACTIVATED, 0, 0, 0);
        }
        else if(pfds[1].revents & POLLHUP)
        {
            DEBUG(printf("pollhup on rpc socket\n"));
            close(rpc_sock);
            if((ret = open_rpc_socket(&rpc_sock, &rpc_addr)))
            {
                close(screen_fd);
                free_clients();
                return ret;
            }
        }
        else
        {
            int x;
            for(x=2; x<client_count+2; x++)
                if(pfds[x].revents & POLLHUP || pfds[x].revents & POLLERR)
                {
                    DEBUG(printf("pollhup/pollerr on client socket [%i]\n", pfds[x].fd));
                    if(rem_client(x, 0))
                        send_client(TSP_EVENT_ACTIVATED, 0, 0, 0);
                }
                else if(pfds[x].revents & POLLIN)
                {
                    recv(pfds[x].fd, &cmd, sizeof(struct tsp_cmd), 0);
                    switch(cmd.cmd)
                    {
                    case TSP_CMD_REGISTER:
                        register_client(x, cmd.pid);
                        break;
                    case TSP_CMD_REMOVE:
                        if((cs = find_client(x, cmd.pid)))
                        {
                            if(cs->sock == pfds[x].fd)
                            {
                                if(rem_client(x, cmd.pid))
                                    send_client(TSP_EVENT_ACTIVATED, 0, 0, 0);
                            }
                            else
                            {
                                DEBUG(printf("Client remove [%i] %i\n",
                                    cs->sock, cs->pid));
                                send_client(TSP_EVENT_REMOVED, 0, 0, cs);
                            }
                        }
                        break;
                    case TSP_CMD_SWITCH:
                        cs = client_list.cqh_first;
                        if(switch_client(cmd.value, x, cmd.pid))
                            send_client(TSP_EVENT_DEACTIVATED, 0, 0, cs);
                        break;
                    case TSP_CMD_LOCK:
                        lock = cmd.value;
                        DEBUG(printf("Screen %s\n",
                            lock ? "locked" : "unlocked"));
                        break;
                    case TSP_CMD_HIDE:
                        hide_client(x, cmd.pid,
                            cmd.value & ~TSP_HIDE_MASK,
                            cmd.value & TSP_HIDE_MASK);
                        break;
                    case TSP_CMD_ACK:
                        switch(cmd.value)
                        {
                        case TSP_EVENT_DEACTIVATED:
                            cs = client_list.cqh_first;
                            DEBUG(printf("Client switched [%i] %i -> [%i] %i\n",
                                pfds[x].fd, find_client(x, 0)->pid,
                                cs->sock, cs->pid));
                            send_client(TSP_EVENT_ACTIVATED, 0, 0, 0);
                            break;
                        case TSP_EVENT_REMOVED:
                            if(rem_client(x, 0))
                                send_client(TSP_EVENT_ACTIVATED, 0, 0, 0);
                            break;
                        default:
                            DEBUG(printf("Client done [%i] %i\n",
                                pfds[x].fd, find_client(x, 0)->pid));
                            break;
                        }
                        break;
                    default:
                        DEBUG(printf("Unrecognized command 0x%02hhx [%i] %i\n",
                            cmd.cmd, pfds[x].fd, find_client(x, 0)->pid));
                    }
                }
        }
    }
    
    close(screen_fd);
    close(rpc_sock);
    free_clients();
    
    return 0;
}
