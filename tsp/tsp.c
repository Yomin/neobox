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
#include <sys/ioctl.h>
#include <linux/input.h>

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


int screen_fd, aux_fd, power_fd, rpc_sock;
int client_count, pfds_cap;
struct client_sockets client_list;
char *pwd, *screen_dev, *aux_dev, *power_dev;
struct pollfd *pfds;


int open_rpc_socket(int *sock, struct sockaddr_un *addr)
{
    addr->sun_family = AF_UNIX;
    sprintf(addr->sun_path, "%s/%s", pwd, TSP_RPC);
    
    if((*sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to open rpc socket");
        return 11;
    }
    
    if(unlink(addr->sun_path) == -1 && errno != ENOENT)
    {
        perror("Failed to unlink rpc socket file");
        return 12;
    }
    
    if(bind(*sock, (struct sockaddr*)addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("Failed to bind rpc socket");
        return 13;
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
    cs->hide = 0;
    if(client_count+4 == pfds_cap)
    {
        pfds_cap += 10;
        pfds = realloc(pfds, pfds_cap*sizeof(struct pollfd));
    }
    pfds[client_count+4].fd = fd;
    pfds[client_count+4].events = POLLIN;
    client_count++;
    DEBUG(printf("Client added [%i]\n", fd));
}

int send_client(unsigned char event, union tsp_value value, struct chain_socket *cs)
{
    struct tsp_event evnt;
    char *ptr = (char*)&evnt;
    int count, size = sizeof(struct tsp_event);
    
    if(client_list.cqh_first == (void*)&client_list)
        return 0;
    
    evnt.event = event;
    evnt.value = value;
    
    if(!cs)
        cs = client_list.cqh_first;
    
    while((count = send(cs->sock, ptr, size, 0)) != size)
    {
        if(count == -1)
        {
            DEBUG(perror("Failed to send client event"));
            return -1;
        }
        ptr += count;
        size -= count;
    }
    
    return 0;
}

void send_client_cord(unsigned char event, short int y, short int x, struct chain_socket *cs)
{
    union tsp_value value;
    
    value.cord.y = y;
    value.cord.x = x;
    
    send_client(event, value, cs);
}

void send_client_status(unsigned char event, int status, struct chain_socket *cs)
{
    union tsp_value value;
    
    value.status = status;
    
    send_client(event, value, cs);
}

int recv_client(int sock, struct tsp_cmd *cmd)
{
    char *ptr = (char*)cmd;
    int count, size = sizeof(struct tsp_cmd);
    
    while((count = recv(sock, ptr, size, 0)) != size)
    {
        if(count == -1)
        {
            DEBUG(perror("Failed to recv client cmd"));
            return -1;
        }
        ptr += count;
        size -= count;
    }
    
    return 0;
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
    int active;
    DEBUG(int fd);
    
    if(!(cs = find_client(pos, pid)))
        return 0;
    
    DEBUG(fd = cs->sock);
    pid = cs->pid;
    active = cs == client_list.cqh_first;
    
    memcpy(pfds+pos, pfds+pos+1, client_count+4-pos-1);
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

void cleanup()
{
    DEBUG(printf("cleanup\n"));
    if(screen_fd)
        close(screen_fd);
    if(aux_fd)
        close(aux_fd);
    if(power_fd)
        close(power_fd);
    if(rpc_sock)
        close(rpc_sock);
    free(screen_dev);
    free(aux_dev);
    free(power_dev);
    free(pfds);
    free_clients();
}

void signal_handler(int signal)
{
    cleanup();
    exit(0);
}

#ifndef NDEBUG

char tmpbuf[20];

const char* strevent(unsigned char ev)
{
    switch(ev)
    {
    case EV_SYN: return "EV_SYN";
    case EV_KEY: return "EV_KEY";
    case EV_REL: return "EV_REL";
    case EV_ABS: return "EV_ABS";
    case EV_MSC: return "EV_MSC";
    case EV_SW: return "EV_SW";
    case EV_LED: return "EV_LED";
    case EV_SND: return "EV_SND";
    case EV_REP: return "EV_REP";
    case EV_FF: return "EV_FF";
    case EV_FF_STATUS: return "EV_FF_STATUS";
    case EV_PWR: return "EV_PWR";
    default:
        snprintf(tmpbuf, 20, "unknown(%02hhx)", ev);
        return tmpbuf;
    }
}

void print_info(int fd)
{
    char buf[100];
    int version;
    struct input_id id;
    char evtype[(EV_MAX+7)/8];
    int i;
    
    if(ioctl(fd, EVIOCGNAME(100), buf) == -1)
    {
        if(errno == ENOTTY) // sim
            return;
        perror("Failed to get device name");
    }
    else
        printf("Device: %s\n", buf);
    if(ioctl(fd, EVIOCGID, &id) == -1)
        perror("Failed to get device id");
    else
    {
        switch(id.bustype)
        {
        case BUS_PCI: printf("Bustype PCI "); break;
        case BUS_ISAPNP: printf("Bustype ISAPNP "); break;
        case BUS_USB: printf("Bustype USB "); break;
        case BUS_HIL: printf("Bustype HIL "); break;
        case BUS_BLUETOOTH: printf("Bustype BLUETOOTH "); break;
        case BUS_VIRTUAL: printf("Bustype VIRTUAL "); break;
        case BUS_ISA: printf("Bustype ISA "); break;
        case BUS_I8042: printf("Bustype I8042 "); break;
        case BUS_XTKBD: printf("Bustype XTKBD "); break;
        case BUS_RS232: printf("Bustype RS232 "); break;
        case BUS_GAMEPORT: printf("Bustype GAMEPORT "); break;
        case BUS_PARPORT: printf("Bustype PARPORT "); break;
        case BUS_AMIGA: printf("Bustype AMIGA "); break;
        case BUS_ADB: printf("Bustype ADB "); break;
        case BUS_I2C: printf("Bustype I2C "); break;
        case BUS_HOST: printf("Bustype HOST "); break;
        case BUS_GSC: printf("Bustype GSC "); break;
        case BUS_ATARI: printf("Bustype ATARI "); break;
        case BUS_SPI: printf("Bustype SPI "); break;
        default: printf("Bustype unknown(%04hx) ", id.bustype);
        }
        printf("Vendor %04hx ", id.vendor);
        printf("Product %04hx ", id.product);
        printf("Version %04hx\n", id.version);
    }
    if(ioctl(fd, EVIOCGVERSION, &version) == -1)
        perror("Failed to get driver version");
    else
        printf("Driver: %i.%i.%i\n", version>>16,
            (version>>8) & 0xff, version & 0xff);
    if(ioctl(fd, EVIOCGBIT(0, EV_MAX), &evtype) == -1)
        perror("Failed to get event types");
    else
    {
        printf("Events: ");
        for(i=0; i<EV_MAX; i++)
            if(evtype[i/8] & (1<<(i%8)))
                printf("%s ", strevent(i));
        printf("\n");
    }
}
#endif

void usage(const char *name)
{
    printf("Usage: %s [-f] [-d pwd] <config>\n", name);
}

int main(int argc, char* argv[])
{
    int opt, ret;
    int daemon = 1, lock = 0;
    int screen_status, aux_pressed, power_pressed, y, x;
    char *config;
    FILE *file;
    struct sockaddr_un rpc_addr;
    
    struct input_event input;
    struct tsp_cmd cmd;
    struct chain_socket *cs;
    
    size_t size;
    
    y = x = screen_status = aux_pressed = power_pressed = 0;
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
    
    config = argv[optind];
    
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
    
    if(!(file = fopen(config, "r")))
    {
        perror("Failed to open config file");
        return 4;
    }
    
    if((size = getline(&screen_dev, &size, file)) == -1)
    {
        fprintf(stderr, "Failed to read screen config\n");
        return 5;
    }
    screen_dev[size-1] = 0;
    
    if((screen_fd = open(screen_dev, O_RDONLY)) == -1)
    {
        perror("Failed to open screen socket");
        return 6;
    }
    
    if((size = getline(&aux_dev, &size, file)) == -1)
    {
        fprintf(stderr, "Failed to read aux config\n");
        return 7;
    }
    aux_dev[size-1] = 0;
    
    if((aux_fd = open(aux_dev, O_RDONLY)) == -1)
    {
        perror("Failed to open aux socket");
        return 8;
    }
    
    if((size = getline(&power_dev, &size, file)) == -1)
    {
        fprintf(stderr, "Failed to read power config\n");
        return 9;
    }
    power_dev[size-1] = 0;
    fclose(file);
    
    if((power_fd = open(power_dev, O_RDONLY)) == -1)
    {
        perror("Failed to open power socket");
        return 10;
    }
    
    DEBUG(print_info(screen_fd));
    DEBUG(print_info(aux_fd));
    DEBUG(print_info(power_fd));
    
    if((ret = open_rpc_socket(&rpc_sock, &rpc_addr)))
        return ret;
    
    pfds_cap = 10;
    pfds = malloc(pfds_cap*sizeof(struct pollfd));
    pfds[0].fd = screen_fd;
    pfds[0].events = POLLIN;
    pfds[1].fd = aux_fd;
    pfds[1].events = POLLIN;
    pfds[2].fd = power_fd;
    pfds[2].events = POLLIN;
    pfds[3].fd = rpc_sock;
    pfds[3].events = POLLIN;
    
    client_count = 0;
    CIRCLEQ_INIT(&client_list);
    
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    DEBUG(printf("Ready\n"));
    
    while(1)
    {
        poll(pfds, client_count+4, -1);
        
        if(pfds[0].revents & POLLHUP || pfds[0].revents & POLLERR)
        {
            DEBUG(printf("pollhup/err on screen socket\n"));
            close(screen_fd);
            if((screen_fd = open(screen_dev, O_RDONLY)) < 0)
            {
                perror("Failed to open screen socket");
                screen_fd = 0;
                cleanup();
                return 6;
            }
        }
        else if(pfds[0].revents & POLLIN)
        {
            read(screen_fd, &input, sizeof(struct input_event));
            
            if(lock)
                continue;
            
            switch(input.type)
            {
            case EV_KEY:
                switch(input.code)
                {
                case BTN_TOUCH:
                    switch(input.value)
                    {
                        case 0:
                            screen_status = 0;
                            break;
                        case 1:
                            screen_status = 1;
                            break;
                    }
                    break;
                }
                break;
            case EV_ABS:
                switch(input.code)
                {
                case ABS_X:
                    y = input.value-MIN_PIXEL;
                    break;
                case ABS_Y:
                    x = input.value-MIN_PIXEL;
                    break;
                case ABS_PRESSURE:
                    break;
                }
                break;
            case EV_SYN:
                switch(input.code)
                {
                case SYN_REPORT:
                    switch(screen_status)
                    {
                    case 0:
                        DEBUG(printf("Touchscreen released (%i,%i)\n", y, x));
                        send_client_cord(TSP_EVENT_RELEASED, y, x, 0);
                        break;
                    case 1:
                        DEBUG(printf("Touchscreen pressed (%i,%i)\n", y, x));
                        send_client_cord(TSP_EVENT_PRESSED, y, x, 0);
                        screen_status = 2;
                        break;
                    case 2:
                        send_client_cord(TSP_EVENT_MOVED, y, x, 0);
                        break;
                    }
                    break;
                }
                break;
            }
        }
        else if(pfds[1].revents & POLLHUP || pfds[1].revents & POLLERR)
        {
            DEBUG(printf("pollhup/err on aux socket\n"));
            close(aux_fd);
            if((aux_fd = open(aux_dev, O_RDONLY)) == -1)
            {
                perror("Failed to open aux socket");
                aux_fd = 0;
                cleanup();
                return 8;
            }
        }
        else if(pfds[1].revents & POLLIN)
        {
            read(aux_fd, &input, sizeof(struct input_event));
            switch(input.type)
            {
            case EV_KEY:
                switch(input.code)
                {
                case KEY_PHONE:
                    aux_pressed = input.value;
                    break;
                }
                break;
            case EV_SYN:
                switch(input.code)
                {
                case SYN_REPORT:
                    DEBUG(printf("AUX %s\n",
                        aux_pressed ? "pressed" : "released"));
                    send_client_status(TSP_EVENT_AUX, aux_pressed, 0);
                    break;
                }
                break;
            }
        }
        else if(pfds[2].revents & POLLHUP || pfds[2].revents & POLLERR)
        {
            DEBUG(printf("pollhup/err on power socket\n"));
            close(power_fd);
            if((power_fd = open(power_dev, O_RDONLY)) == -1)
            {
                perror("Failed to open power socket");
                power_fd = 0;
                cleanup();
                return 10;
            }
        }
        else if(pfds[2].revents & POLLIN)
        {
            read(power_fd, &input, sizeof(struct input_event));
            switch(input.type)
            {
            case EV_KEY:
                switch(input.code)
                {
                case KEY_POWER:
                    power_pressed = input.value;
                    break;
                }
                break;
            case EV_PWR:
                break;
            case EV_SYN:
                switch(input.code)
                {
                case SYN_REPORT:
                    DEBUG(printf("Power %s\n",
                        power_pressed ? "pressed" : "released"));
                    send_client_status(TSP_EVENT_POWER, power_pressed, 0);
                    break;
                }
                break;
            }
        }
        else if(pfds[3].revents & POLLHUP || pfds[3].revents & POLLERR)
        {
            DEBUG(printf("pollhup/err on rpc socket\n"));
            close(rpc_sock);
            if((ret = open_rpc_socket(&rpc_sock, &rpc_addr)))
            {
                rpc_sock = 0;
                cleanup();
                return ret;
            }
        }
        else if(pfds[3].revents & POLLIN)
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
                send_client_status(TSP_EVENT_DEACTIVATED, 0, cs);
            else
                send_client_status(TSP_EVENT_ACTIVATED, 0, 0);
        }
        else
        {
            int x;
            for(x=4; x<client_count+4; x++)
                if(pfds[x].revents & POLLHUP || pfds[x].revents & POLLERR)
                {
                    DEBUG(printf("pollhup/pollerr on client socket [%i]\n", pfds[x].fd));
                    if(rem_client(x, 0))
                        send_client_status(TSP_EVENT_ACTIVATED, 0, 0);
                }
                else if(pfds[x].revents & POLLIN)
                {
                    if(recv_client(pfds[x].fd, &cmd))
                        break;
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
                                    send_client_status(
                                        TSP_EVENT_ACTIVATED, 0, 0);
                            }
                            else
                            {
                                DEBUG(printf("Client remove [%i] %i\n",
                                    cs->sock, cs->pid));
                                send_client_status(
                                    TSP_EVENT_REMOVED, 0, cs);
                            }
                        }
                        break;
                    case TSP_CMD_SWITCH:
                        cs = client_list.cqh_first;
                        if(switch_client(cmd.value, x, cmd.pid))
                            send_client_status(
                                TSP_EVENT_DEACTIVATED, 0, cs);
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
                            send_client_status(
                                TSP_EVENT_ACTIVATED, 0, 0);
                            break;
                        case TSP_EVENT_REMOVED:
                            if(rem_client(x, 0))
                                send_client_status(
                                    TSP_EVENT_ACTIVATED, 0, 0);
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
    
    cleanup();
    
    return 0;
}
