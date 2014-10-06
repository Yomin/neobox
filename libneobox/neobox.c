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


#define _GNU_SOURCE
#include <poll.h>

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>

#include "neobox_def.h"
#include "neobox_fb.h"
#include "neobox_config.h"

#include "neobox_layout_default.h"

#include "neobox_type_nop.h"
#include "neobox_type_button.h"
#include "neobox_type_slider.h"
#include "neobox_type_select.h"

#define TYPEFUNC(e, m, r, ...) \
    do { \
        switch((e)->type) \
        { \
        case NEOBOX_LAYOUT_TYPE_NOP: \
            neobox_type_nop_ ## m (__VA_ARGS__); \
            break; \
        case NEOBOX_LAYOUT_TYPE_CHAR: \
        case NEOBOX_LAYOUT_TYPE_GOTO: \
        case NEOBOX_LAYOUT_TYPE_HOLD: \
        case NEOBOX_LAYOUT_TYPE_TOGGLE: \
        case NEOBOX_LAYOUT_TYPE_SYSTEM: \
            r neobox_type_button_ ## m (__VA_ARGS__); \
            break; \
        case NEOBOX_LAYOUT_TYPE_HSLIDER: \
        case NEOBOX_LAYOUT_TYPE_VSLIDER: \
            r neobox_type_slider_ ## m (__VA_ARGS__); \
            break; \
        case NEOBOX_LAYOUT_TYPE_SELECT: \
            r neobox_type_select_ ## m (__VA_ARGS__); \
            break; \
        } \
    } \
    while(0)

#define WRITE_STR(fd, s) write(fd, s, strlen(s))
#define WRITE_INT_STR(fd, i, x, buf, len, s) \
    if(!(i)) { (buf)[0]='0'; write(fd, (buf), 1); } \
    else \
    { \
        for((x)=(len)-1; (x)>=0 && (i)>0; (i)/=10,(x)--) \
            (buf)[x]=(i)%10+'0'; \
        write(fd, (buf)+(x)+1, (len)-(x)-1); \
    } \
    write(fd, s, strlen(s));

struct neobox_global neobox;

int neobox_iod_cmd(unsigned char cmd, pid_t pid, int value);
int neobox_handle_timer(unsigned char *id, unsigned char *type);

void neobox_signal_handler(int signal)
{
    struct neobox_chain_queue *cq;
    unsigned char id, type;
    int ret, err = errno;
    
    switch(signal)
    {
    case SIGALRM:
        do
        {
            if((ret = neobox_handle_timer(&id, &type)) == -1)
                continue;
            if(type == TIMER_SYSTEM)
            {
                switch(id)
                {
                case 0: neobox.pause = 0; break;
                case 1: neobox.sleep = 0; break;
                }
            }
            else
            {
                cq = malloc(sizeof(struct neobox_chain_queue));
                CIRCLEQ_INSERT_TAIL(&neobox.queue, cq, chain);
                cq->stash.type = STASH_NEOBOX;
                cq->stash.event.neobox.type = NEOBOX_EVENT_TIMER;
                cq->stash.event.neobox.id = id;
            }
        }
        while(ret);
        break;
    default:
        cq = malloc(sizeof(struct neobox_chain_queue));
        CIRCLEQ_INSERT_TAIL(&neobox.queue, cq, chain);
        cq->stash.type = STASH_NEOBOX;
        cq->stash.event.neobox.type = NEOBOX_EVENT_SIGNAL;
        cq->stash.event.neobox.value.i = signal;
    }
    errno = err;
}

int neobox_catch_signal(int sig, int flags)
{
    struct sigaction sa;
    sa.sa_handler = neobox_signal_handler;
    sa.sa_flags = flags;
    sigfillset(&sa.sa_mask);
    
    switch(sig)
    {
    case SIGALRM:
    case SIGINT:
        return NEOBOX_ERROR_SIGNAL;
    default:
        if(sigaction(sig, &sa, 0) == -1)
        {
            VERBOSE(perror("[NEOBOX] Failed to catch signal"));
            return NEOBOX_ERROR_SIGNAL;
        }
    }
    return 0;
}

struct neobox_stash neobox_get_event(sigset_t *oldset)
{
    struct neobox_stash stash;
    struct neobox_chain_queue *cq;
    sigset_t set, soldset;
    
    sigfillset(&set);
    stash.type = STASH_NOP;
    
    if(!oldset)
        oldset = &soldset;
    
    sigprocmask(SIG_BLOCK, &set, oldset);
    
    if((cq = neobox.queue.cqh_first) == (void*)&neobox.queue)
    {
        // if oldset exists keep blocking
        if(oldset == &soldset)
            sigprocmask(SIG_SETMASK, oldset, 0);
        return stash;
    }
    
    CIRCLEQ_REMOVE(&neobox.queue, cq, chain);
    
    // if oldset exists keep blocking
    if(oldset == &soldset)
        sigprocmask(SIG_SETMASK, oldset, 0);
    
    stash = cq->stash;
    free(cq);
    
    return stash;
}

void neobox_queue_event(char type, void *event)
{
    struct neobox_chain_queue *cq;
    sigset_t set, oldset;
    
    switch(type)
    {
    case STASH_IOD:
        cq = malloc(sizeof(struct neobox_chain_queue));
        cq->stash.type = type;
        cq->stash.event.iod = *(struct iod_event*)event;
        break;
    case STASH_NEOBOX:
        if(((struct neobox_event*)event)->type == NEOBOX_EVENT_NOP)
            return;
        cq = malloc(sizeof(struct neobox_chain_queue));
        cq->stash.type = type;
        cq->stash.event.neobox = *(struct neobox_event*)event;
        break;
    }
    
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, &oldset);
    
    CIRCLEQ_INSERT_TAIL(&neobox.queue, cq, chain);
    
    sigprocmask(SIG_SETMASK, &oldset, 0);
}

void neobox_init_partner()
{
    int i, j, k, width, size;
    
    const struct neobox_mapelem *map;
    const struct neobox_mapelem *elem;
    struct neobox_point p, p2, *pp;
    struct neobox_partner **partner, **partner_prev;
    
    // create for every group of buttons a vector
    // filled with neobox_points of all buttons
    // every neobox.save[i][x].partner->connect of a button points
    // to the same vector
    // the vectors and points are saved in layout format
    
    neobox.save = malloc(neobox.layout.size*sizeof(struct neobox_save*));
    
    for(i=0; i<neobox.layout.size; i++)
    {
        width = neobox.layout.maps[i].width;
        size = neobox.layout.maps[i].height*width;
        neobox.save[i] = calloc(size, sizeof(struct neobox_save));
        
        for(j=0; j<size; j++)
        {
            map = neobox.layout.maps[i].map;
            elem = &map[j];
            p = (struct neobox_point){j/width, j%width, elem};
            
            if(j%width>0 && elem->options & NEOBOX_LAYOUT_OPTION_CONNECT_LEFT)
            {
                partner_prev = &neobox.save[i][j-1].partner;
                if(!*partner_prev) // left unconnected
                {
                    *partner_prev = calloc(1, sizeof(struct neobox_partner));
                    vector_init(sizeof(struct neobox_point),
                        &(*partner_prev)->connect);
                    p2 = (struct neobox_point){(j-1)/width,
                        (j-1)%width, &map[j-1]};
                    vector_push(&p2, (*partner_prev)->connect);
                }
                neobox.save[i][j].partner = *partner_prev;
                vector_push(&p, (*partner_prev)->connect);
            }
            if(j>=width && elem->options & NEOBOX_LAYOUT_OPTION_CONNECT_UP)
            {
                partner_prev = &neobox.save[i][j-width].partner;
                partner = &neobox.save[i][j].partner;
                if(*partner) // already left connected
                {
                    if(!*partner_prev) // up unconnected
                    {
                        p2 = (struct neobox_point){j/width-1,
                            j%width, &map[j]};
                        *partner_prev = *partner;
                        vector_push(&p2, (*partner)->connect);
                        continue;
                    }
                    if(*partner_prev == *partner)
                        continue;
                    // merge vectors
                    if(vector_size((*partner_prev)->connect)
                        > vector_size((*partner)->connect))
                    {
                        *partner = *partner_prev;
                        partner_prev = &neobox.save[i][j].partner;
                    }
                    for(k=0; k<vector_size((*partner)->connect); k++)
                    {
                        pp = vector_at(k, (*partner)->connect);
                        vector_push(pp, (*partner_prev)->connect);
                        neobox.save[i][pp->y*width+pp->x].partner
                            = *partner_prev;
                    }
                    vector_finish((*partner)->connect);
                    free(*partner);
                }
                else
                {
                    if(!*partner_prev) // up unconnected
                    {
                        *partner_prev = calloc(1, sizeof(struct neobox_partner));
                        vector_init(sizeof(struct neobox_point),
                            &(*partner_prev)->connect);
                        p2 = (struct neobox_point){j/width-1,
                            j%width, &map[j-width]};
                        vector_push(&p2, (*partner_prev)->connect);
                    }
                    *partner = *partner_prev;
                    vector_push(&p, (*partner)->connect);
                }
            }
        }
    }
}

void neobox_init_save_data()
{
    int i, y, x, pos;
    const struct neobox_map *map;
    struct neobox_save *save;
    const struct neobox_mapelem *elem;
    
    for(i=0; i<neobox.layout.size; i++)
    {
        map = &neobox.layout.maps[i];
        for(y=0; y<map->height; y++)
            for(x=0; x<map->width; x++)
            {
                pos = y*map->width+x;
                save = &neobox.save[i][pos];
                elem = &map->map[pos];
                TYPEFUNC(elem, init, , y, x, map, elem, save);
            }
    }
}

int neobox_iod_connect(int tries)
{
    struct sockaddr_un addr;
    int reuse = 1;
    
    VERBOSE(printf("[NEOBOX] Connecting to iod socket\n"));
    
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, neobox.iod.usock, UNIX_PATH_MAX);
    
    while(1)
    {
        if(neobox.iod.sock)
            while(close(neobox.iod.sock) == -1 && errno == EINTR);
        
        if((neobox.iod.sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        {
            VERBOSE(perror("[NEOBOX] Failed to open iod socket"));
            goto retry;
        }
        
        if(setsockopt(neobox.iod.sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
        {
            VERBOSE(perror("Failed to set SO_REUSEADDR"));
            goto retry;
        }
        
        while(connect(neobox.iod.sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
        {
            if(errno == EINTR)
                continue;
            VERBOSE(perror("[NEOBOX] Failed to connect iod socket"));
            goto retry;
        }
        
        break;
        
retry:  if(tries-- == 1)
        {
            VERBOSE(perror("[NEOBOX] Unable to connect iod socket"));
            return NEOBOX_ERROR_IOD_OPEN;
        }
        
        VERBOSE(printf("[NEOBOX] Retry connecting to iod socket in 1 second\n"));
        
        while(sleep(1));
    }
    
    return 0;
}

int neobox_iod_cmd(unsigned char cmd, pid_t pid, int value)
{
    struct iod_cmd iod;
    char *ptr = (char*)&iod;
    int count, err, size = sizeof(struct iod_cmd);
    
    iod.cmd = cmd;
    iod.pid = pid;
    iod.value = value;
    
    while((count = send(neobox.iod.sock, ptr, size, 0)) != size)
    {
        if(count == -1)
        {
            if((err = errno) == EINTR)
                continue;
            VERBOSE(perror("[NEOBOX] Failed to send iod cmd"));
            return err;
        }
        ptr += count;
        size -= count;
    }
    
    return 0;
}

int neobox_iod_recv(struct iod_event *event)
{
    char *ptr = (char*)event;
    int count, err, size = sizeof(struct iod_event);
    
    while((count = recv(neobox.iod.sock, ptr, size, 0)) != size)
        switch(count)
        {
        case 0:
            VERBOSE(perror("[NEOBOX] Failed to recv iod event"));
            return 1;
        case -1:
            if((err = errno) == EINTR)
                continue;
            VERBOSE(perror("[NEOBOX] Failed to recv iod event"));
            return err;
        default:
            ptr += count;
            size -= count;
        }
    
    return 0;
}

void neobox_iod_reconnect()
{
    while(1)
    {
        neobox_iod_connect(-1);
        
        if(neobox_iod_cmd(IOD_CMD_REGISTER, getpid(), 0))
            continue;
        if(neobox.iod.lock)
            if(neobox_iod_cmd(IOD_CMD_LOCK, 0, 1))
                continue;
        if(neobox.iod.hide)
            if(neobox_iod_cmd(IOD_CMD_HIDE, 0, IOD_HIDE_MASK|neobox.iod.priority))
                continue;
        if(neobox.iod.grab & NEOBOX_BUTTON_AUX)
        {
            if(neobox_iod_cmd(IOD_CMD_GRAB, 0, IOD_GRAB_MASK|IOD_GRAB_AUX))
                continue;
        }
        else if(neobox.iod.grab & NEOBOX_BUTTON_POWER)
            if(neobox_iod_cmd(IOD_CMD_GRAB, 0, IOD_GRAB_MASK|IOD_GRAB_POWER))
                continue;
        break;
    }
}

struct neobox_options neobox_args(int *argc, char *argv[], struct neobox_options options)
{
    struct option fboption[] =
    {
        { "neobox-fb", 1, 0, 'f' },      // path to framebuffer
        { "neobox-iod", 1, 0, 't' },     // path to iod socket
        { "neobox-verbose", 0, 0, 'v' }, // verbose messages
        { "neobox-format", 1, 0, 'r' },  // p: portrait, l: landscape
        { "neobox-config", 1, 0, 'c' },  // config path
        { "neobox-print", 1, 0, 'p'},    // n: no print, f: force print
        {0, 0, 0, 0}
    };
    int opt, x, y;
    opterr = 0;

    while((opt = getopt_long(*argc, argv, "", fboption, 0)) != -1)
    {
        switch(opt)
        {
        case 'f':
            options.fb = optarg;
            break;
        case 't':
            options.iod = optarg;
            break;
        case 'v':
            options.verbose = 1;
            break;
        case 'r':
            options.format = NEOBOX_FORMAT_LANDSCAPE;
            if(optarg[0] == 'p')
                options.format = NEOBOX_FORMAT_PORTRAIT;
            break;
        case 'c':
            options.config = optarg;
            break;
        case 'p':
            options.options &= ~NEOBOX_OPTION_PRINT_MASK;
            switch(optarg[0])
            {
            case 'n': // [n]o
                options.options |= NEOBOX_OPTION_NO_PRINT;
                break;
            case 'f': // [f]orce
                options.options |= NEOBOX_OPTION_FORCE_PRINT;
                break;
            }
            break;
        default:
            continue;
        }
        y = optarg ? 2 : 1;
        optind -= y;
        for(x=optind; x<*argc-y; x++)
            argv[x] = argv[x+y];
        *argc -= y;
    }
    opterr = 1;
    optind = 1;
    return options;
}

void neobox_init_screen()
{
    const struct neobox_map *map = &neobox.layout.maps[neobox.parser.map];
    struct neobox_save *save, *saves = neobox.save[neobox.parser.map];
    const struct neobox_mapelem *elem;
    int y, x, height, width, pos;
    SIMV(char sim_tmp = 'x');
    unsigned char color[4] = {0};
    char flagstat = neobox.flagstat[neobox.parser.map];
    
    neobox_get_sizes(&height, &width, 0, 0, 0, 0, map);
    
    for(y=0; y<map->height; y++)
        for(x=0; x<map->width; x++)
        {
            pos = y*map->width+x;
            elem = &map->map[pos];
            save = &saves[pos];
            
            if(save->partner)
            {
                if(save->partner->flag == flagstat)
                    save->partner->flag = !flagstat;
                else
                    continue;
            }
            TYPEFUNC(elem, draw, , y, x, map, elem, save);
        }
    
    height *= map->height;
    width *= map->width;
    
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
        neobox_draw_rect(width, 0, neobox.fb.vinfo.yres-width,
            neobox.fb.vinfo.xres, color, DENSITY, 0);
    else
        neobox_draw_rect(height, 0, neobox.fb.vinfo.yres-height,
            neobox.fb.vinfo.xres, color, DENSITY, 0);
    
    neobox.flagstat[neobox.parser.map] = !flagstat;
    
    // notify framebuffer for redraw
    SIMV(send(neobox.fb.sock, &sim_tmp, 1, 0));
}

int neobox_init_custom(struct neobox_options options)
{
    int ret;
    struct sigaction sa;
    SIMV(struct sockaddr_un addr);
    
    neobox.verbose = options.verbose;
    
    if(options.verbose)
    {
        printf("[NEOBOX] init\n");
        printf("[NEOBOX] fb: %s\n", options.fb);
        printf("[NEOBOX] iod: %s\n", options.iod);
        printf("[NEOBOX] format: %s\n", options.format
            == NEOBOX_FORMAT_LANDSCAPE ? "landscape" : "portrait");
        printf("[NEOBOX] options:\n");
        printf("[NEOBOX]   print: %s\n",
            options.options & NEOBOX_OPTION_FORCE_PRINT ? "forced" :
            options.options & NEOBOX_OPTION_NO_PRINT ? "invisible" :
            "normal");
        printf("[NEOBOX]   adminmap: %s\n",
            options.options & NEOBOX_OPTION_ADMINMAP ? "enabled" : "disabled");
    }
    
    neobox.iod.usock = options.iod;
    neobox.iod.sock = 0;
    
    // open iod socket
    if((ret = neobox_iod_connect(-1)))
        return ret;
    
    // register
    if(neobox_iod_cmd(IOD_CMD_REGISTER, getpid(), 0))
        return NEOBOX_ERROR_REGISTER;
    
    // open framebuffer
#ifndef SIM
    VERBOSE(printf("[NEOBOX] Opening framebuffer\n"));
    if((neobox.fb.fd = open(options.fb, O_RDWR)) == -1)
    {
        perror("[NEOBOX] Failed to open framebuffer");
        close(neobox.iod.sock);
        return NEOBOX_ERROR_FB_OPEN;
    }
    if(ioctl(neobox.fb.fd, FBIOGET_VSCREENINFO, &(neobox.fb.vinfo)) == -1)
    {
        perror("[NEOBOX] Failed to get variable screeninfo");
        close(neobox.iod.sock);
        close(neobox.fb.fd);
        return NEOBOX_ERROR_FB_VINFO;
    }
    if(ioctl(neobox.fb.fd, FBIOGET_FSCREENINFO, &(neobox.fb.finfo)) == -1)
    {
        perror("[NEOBOX] Failed to get fixed screeninfo");
        close(neobox.iod.sock);
        close(neobox.fb.fd);
        return NEOBOX_ERROR_FB_FINFO;
    }
#else
    VERBOSE(printf("[NEOBOX] Simulating framebuffer\n"));
    neobox.fb.sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(neobox.fb.sock == -1)
    {
        perror("[NEOBOX] Failed to open framebuffer socket");
        return NEOBOX_ERROR_FB_OPEN;
    }
    
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, options.fb);
    
    if(connect(neobox.fb.sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("[NEOBOX] Failed to connect framebuffer socket");
        return NEOBOX_ERROR_FB_OPEN;
    }
    
    recv(neobox.fb.sock, neobox.fb.shm, sizeof(neobox.fb.shm), 0);
    recv(neobox.fb.sock, &neobox.fb.vinfo, sizeof(struct fb_var_screeninfo), 0);
    recv(neobox.fb.sock, &neobox.fb.finfo, sizeof(struct fb_fix_screeninfo), 0);
    
    if((neobox.fb.fd = shm_open(neobox.fb.shm, O_CREAT|O_RDWR, 0644)) == -1)
    {
        perror("[NEOBOX] Failed to open shared memory");
        return NEOBOX_ERROR_FB_OPEN;
    }
#endif
    
    neobox.fb.bpp = neobox.fb.vinfo.bits_per_pixel/8;
    neobox.fb.size = neobox.fb.vinfo.xres*neobox.fb.vinfo.yres*neobox.fb.bpp;
    
    if(options.verbose)
    {
        printf("[NEOBOX] framebuffer info:\n");
        printf("[NEOBOX]   size %i x %i\n", neobox.fb.vinfo.xres,neobox.fb.vinfo.yres);
        printf("[NEOBOX]   bytes per pixel %i\n", neobox.fb.bpp);
        printf("[NEOBOX]   line length %i\n", neobox.fb.finfo.line_length);
    }
    

#ifdef SIM
    if(ftruncate(neobox.fb.fd, neobox.fb.size) == -1)
    {
        perror("[NEOBOX] Failed to truncate shared memory");
        return NEOBOX_ERROR_FB_OPEN;
    }
#endif
    
    if((neobox.fb.ptr = (unsigned char*) mmap(0, neobox.fb.size,
        PROT_READ|PROT_WRITE, MAP_SHARED, neobox.fb.fd, 0)) == MAP_FAILED)
    {
        perror("[NEOBOX] Failed to mmap framebuffer");
        close(neobox.iod.sock);
        close(neobox.fb.fd);
        return NEOBOX_ERROR_FB_MMAP;
    }
    
    neobox.fb.ptr += neobox.fb.vinfo.xoffset*neobox.fb.bpp
                  + neobox.fb.vinfo.yoffset*neobox.fb.finfo.line_length;
    
    // save layout, format, options
    neobox.layout = options.layout;
    neobox.format = options.format;
    neobox.options = options.options;
    
    // calculate partner connects, init save data
    neobox_init_partner();
    neobox_init_save_data();
    
    // init parser
    if(options.map == NEOBOX_MAP_DEFAULT)
        neobox.parser.map = neobox.parser.map_main = options.layout.start;
    else
        neobox.parser.map = neobox.parser.map_main = options.map;
    
    neobox.parser.toggle = 0;
    neobox.parser.hold = 0;
    neobox.parser.pressed = 0;
    
    // init config
    neobox.config.rj = 0;
    if((ret = neobox_config_open(options.config)) && (options.config || ret != ENOENT))
    {
        fprintf(stderr, "Failed to open config: %s\n",
            neobox_config_strerror(ret));
        return NEOBOX_ERROR_CONFIG;
    }
    
    // else
    neobox.pause = 0;
    neobox.filter = 0;
    neobox.flagstat = calloc(sizeof(char), neobox.layout.size);
    CIRCLEQ_INIT(&neobox.queue);
    CIRCLEQ_INIT(&neobox.timer);
    
    // signals
    sa.sa_handler = neobox_signal_handler;
    sigfillset(&sa.sa_mask);
    
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGALRM, &sa, 0) == -1)
    {
        perror("Failed to catch SIGARLM");
        return NEOBOX_ERROR_SIGNAL;
    }
    
    sa.sa_flags = 0;
    if(sigaction(SIGINT, &sa, 0) == -1)
    {
        perror("Failed to catch SIGINT");
        return NEOBOX_ERROR_SIGNAL;
    }
    
    VERBOSE(printf("[NEOBOX] init done\n"));
    
    // return socket for manual polling
    return neobox.iod.sock;
}

struct neobox_options neobox_options_default(int *argc, char *argv[])
{
    struct neobox_options options;
    options.fb = "/dev/fb0";
    options.iod = IOD_PWD "/" IOD_SOCK;
    options.config = NEOBOX_CONFIG_DEFAULT;
    options.layout = tkbLayoutDefault;
    options.format = NEOBOX_FORMAT_LANDSCAPE;
    options.options = NEOBOX_OPTION_NORM_PRINT;
    options.verbose = 0;
    options.map = NEOBOX_MAP_DEFAULT;
    return neobox_args(argc, argv, options);
}

int neobox_init_default(int *argc, char *argv[])
{
    return neobox_init_custom(neobox_options_default(argc, argv));
}

int neobox_init_layout(struct neobox_layout layout, int *argc, char *argv[])
{
    struct neobox_options options = neobox_options_default(argc, argv);
    options.layout = layout;
    return neobox_init_custom(options);
}

void neobox_finish()
{
    int i, j, k, size;
    struct neobox_point *p;
    struct neobox_partner *partner;
    struct neobox_save *save;
    const struct neobox_mapelem *elem;
    const struct neobox_map *map;
    struct neobox_chain_queue *cq;
    struct neobox_chain_timer *ct;
    sigset_t set;
    
    VERBOSE(printf("[NEOBOX] finish\n"));
    
    neobox_iod_cmd(IOD_CMD_REMOVE, 0, 0);
    close(neobox.iod.sock);
    
    close(neobox.fb.fd);
    SIMV(close(neobox.fb.sock));
    
    for(i=0; i<neobox.layout.size; i++)
    {
        map = &neobox.layout.maps[i];
        size = map->height * map->width;
        for(j=0; j<size; j++)
        {
            save = &neobox.save[i][j];
            partner = save->partner;
            elem = &map->map[j];
            
            TYPEFUNC(elem, finish, , save);
            
            if(!save->partner)
                continue;
            
            for(k=0; k<vector_size(partner->connect); k++)
            {
                p = vector_at(k, partner->connect);
                neobox.save[i][p->y*map->width+p->x].partner = 0;
            }
            vector_finish(partner->connect);
            free(partner);
        }
        free(neobox.save[i]);
    }
    free(neobox.save);
    free(neobox.flagstat);
    
    neobox_config_close();
    
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, 0);
    
    while((cq = neobox.queue.cqh_first) != (void*)&neobox.queue)
    {
        CIRCLEQ_REMOVE(&neobox.queue, cq, chain);
        free(cq);
    }
    while((ct = neobox.timer.cqh_first) != (void*)&neobox.timer)
    {
        CIRCLEQ_REMOVE(&neobox.timer, ct, chain);
        free(ct);
    }
}

int neobox_add_timer(unsigned char id, unsigned char type, unsigned int sec, unsigned int msec)
{
    struct neobox_chain_timer *ct;
    struct neobox_chain_timer *nct = malloc(sizeof(struct neobox_chain_timer));
    struct timeval *tv = &nct->tv;
    struct itimerval it;
    sigset_t set, oldset;
    
    nct->id = id;
    nct->type = type;
    gettimeofday(tv, 0);
    sigfillset(&set);
    
    sec += msec/1000;
    msec %= 1000;
    
    tv->tv_sec += sec;
    tv->tv_sec += (tv->tv_usec+msec*1000)/1000000;
    tv->tv_usec = (tv->tv_usec+msec*1000)%1000000;
    
    sigprocmask(SIG_BLOCK, &set, &oldset);
    
    if(neobox.verbose && type == TIMER_USER)
        printf("[NEOBOX] add timer %i [%u.%u]\n", id, sec, msec);
    
    ct = neobox.timer.cqh_first;
    
    while(ct != (void*)&neobox.timer &&
        (tv->tv_sec > ct->tv.tv_sec ||
        (tv->tv_sec == ct->tv.tv_sec && tv->tv_usec >= ct->tv.tv_usec)))
    {
        ct = ct->chain.cqe_next;
    }
    
    if(ct == neobox.timer.cqh_first)
    {
        it.it_interval.tv_sec = 0;
        it.it_interval.tv_usec = 0;
        it.it_value.tv_sec = sec;
        it.it_value.tv_usec = msec*1000;
        if(setitimer(ITIMER_REAL, &it, 0) == -1)
        {
            VERBOSE(fprintf(stderr, "[NEOBOX] Failed to set %s timer %i [%u.%u]: %s\n",
                type == TIMER_SYSTEM ? "system" : "user",
                id, sec, msec, strerror(errno)));
            free(nct);
            sigprocmask(SIG_SETMASK, &oldset, 0);
            return -1;
        }
        CIRCLEQ_INSERT_HEAD(&neobox.timer, nct, chain);
    }
    else
    {
        if(ct == (void*)&neobox.timer)
            CIRCLEQ_INSERT_TAIL(&neobox.timer, nct, chain);
        else
            CIRCLEQ_INSERT_BEFORE(&neobox.timer, ct, nct, chain);
    }
    
    sigprocmask(SIG_SETMASK, &oldset, 0);
    
    return 0;
}

int neobox_handle_timer(unsigned char *id, unsigned char *type)
{
    struct neobox_chain_timer *ct = neobox.timer.cqh_first;
    struct itimerval it;
    struct timeval *tv = &it.it_value;
    struct timespec tsp;
    char buf[10];
    int i, x;
    
    if(ct == (void*)&neobox.timer)
        return 0;
    
    i = *id = ct->id;
    *type = ct->type;
    
    CIRCLEQ_REMOVE(&neobox.timer, ct, chain);
    free(ct);
    
    if(neobox.verbose && *type == TIMER_USER)
    {
        WRITE_STR(0, "[NEOBOX] timer ");
        WRITE_INT_STR(0, i, x, buf, 10, " event\n");
    }
    
    if((ct = neobox.timer.cqh_first) == (void*)&neobox.timer)
        return 0;
    
    clock_gettime(CLOCK_REALTIME, &tsp);
    
    tv->tv_sec = tsp.tv_sec;
    tv->tv_usec = tsp.tv_nsec/1000;
    
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    
    if(tv->tv_sec >= ct->tv.tv_sec && tv->tv_usec >= ct->tv.tv_usec)
        return 1;
    
    tv->tv_sec = ct->tv.tv_sec - tv->tv_sec;
    if(tv->tv_usec <= ct->tv.tv_usec)
        tv->tv_usec = ct->tv.tv_usec - tv->tv_usec;
    else
    {
        tv->tv_sec--;
        tv->tv_usec = 1000000 - tv->tv_usec + ct->tv.tv_usec;
    }
    
    if(setitimer(ITIMER_REAL, &it, 0) == -1)
    {
        if(neobox.verbose)
        {
            WRITE_STR(1, "[NEOBOX] Failed to set ");
            WRITE_STR(1, ct->type == TIMER_SYSTEM ? "system timer " : "user timer ");
            WRITE_INT_STR(1, ct->id, x, buf, 10, " [");
            WRITE_INT_STR(1, tv->tv_sec, x, buf, 10, ".");
            tv->tv_usec /= 1000;
            WRITE_INT_STR(1, tv->tv_usec, x, buf, 10, "]: ");
            WRITE_STR(1, strerror(errno));
            WRITE_STR(1, "\n");
        }
        CIRCLEQ_REMOVE(&neobox.timer, ct, chain);
        free(ct);
        return -1;
    }
    
    return 0;
}

struct neobox_event neobox_parse_iod_event(struct iod_event iod_event)
{
    struct neobox_event event, event2;
    const struct neobox_map *map;
    const struct neobox_mapelem *elem_last, *elem_curr;
    struct neobox_save *save_last, *save_curr;
    struct neobox_partner *partner_last;
    struct neobox_point *p;
    
    int y, x, button_y, button_x, i, ev_y, ev_x, map_prev;
    int width, height, fb_height, fb_width, scr_height, scr_width;
    SIMV(char sim_tmp = 'x');
    
    event.type = NEOBOX_EVENT_NOP;
    event.id = 0;
    event.value.i = 0;
    
    switch(iod_event.event)
    {
    case IOD_EVENT_ACTIVATED:
        VERBOSE(printf("[NEOBOX] activate\n"));
        event.type = NEOBOX_EVENT_ACTIVATE;
        return event;
    case IOD_EVENT_DEACTIVATED:
        VERBOSE(printf("[NEOBOX] deactivate\n"));
        event.type = NEOBOX_EVENT_DEACTIVATE;
        return event;
    case IOD_EVENT_REMOVED:
        VERBOSE(printf("[NEOBOX] removed\n"));
        event.type = NEOBOX_EVENT_REMOVE;
        return event;
    case IOD_EVENT_AUX:
        VERBOSE(printf("[NEOBOX] AUX %s\n",
            iod_event.value.status ? "pressed" : "released"));
        event.type = NEOBOX_EVENT_BUTTON;
        event.id = NEOBOX_BUTTON_AUX;
        event.value.i = iod_event.value.status;
        return event;
    case IOD_EVENT_POWER:
        VERBOSE(printf("[NEOBOX] Power %s\n",
            iod_event.value.status ? "pressed" : "released"));
        event.type = NEOBOX_EVENT_BUTTON;
        event.id = NEOBOX_BUTTON_POWER;
        event.value.i = iod_event.value.status;
        return event;
    case IOD_EVENT_LOCK:
        return event;
    case IOD_EVENT_GRAB:
        return event;
    case IOD_EVENT_POWERSAVE:
        VERBOSE(printf("[NEOBOX] Powersave %s\n",
            iod_event.value.status ? "on" : "off"));
        event.type = NEOBOX_EVENT_POWERSAVE;
        event.value.i = iod_event.value.status;
        return event;
    case IOD_EVENT_MOVED:
    case IOD_EVENT_RELEASED:
    case IOD_EVENT_PRESSED:
        break;
    default:
        VERBOSE(printf("[NEOBOX] Unrecognized iod event 0x%02hhx\n", iod_event.event));
        return event;
    }
    
    ev_y = iod_event.value.cord.y;
    ev_x = iod_event.value.cord.x;
    
    if(ev_x >= SCREENMAX || ev_y >= SCREENMAX)
        return event;
    
    // current map
    map = &neobox.layout.maps[neobox.parser.map];
    
    // calculate height and width
    neobox_get_sizes(&height, &width, &fb_height, &fb_width,
        &scr_height, &scr_width, map);
    
    // screen coordinates to button coordinates
    button_y = fb_height*(((SCREENMAX-ev_y)%scr_height)/(scr_height*1.0));
    button_x = fb_width*((ev_x%scr_width)/(scr_width*1.0));
    neobox_fb_to_layout_pos_width(&button_y, &button_x, fb_width);
    
    // last mapelem
    elem_last = &map->map[neobox.parser.y*map->width+neobox.parser.x];
    
    // broader functions increase type dependent the actual size
    // of the current button
    // if a type broader doesnt hit calculate position of new button
    i = 1;
    if(neobox.parser.pressed)
        TYPEFUNC(elem_last, broader, i=, &y, &x, ev_y, ev_x, map, elem_last);
    if(!neobox.parser.pressed || i)
    {
        x = ev_x / scr_width;
        y = (SCREENMAX - ev_y) / scr_height;
        neobox_fb_to_layout_cords(&y, &x, map);
    }
    
    // current mapelem
    elem_curr = &map->map[y*map->width+x];
    
    // save of last/current button
    save_last = &neobox.save[neobox.parser.map]
        [neobox.parser.y*map->width+neobox.parser.x];
    save_curr = &neobox.save[neobox.parser.map][y*map->width+x];
    
    // partner of last button
    partner_last = save_last->partner;
    
    switch(iod_event.event)
    {
    case IOD_EVENT_MOVED:
        if( neobox.parser.pressed
            && !neobox.pause) // avoid toggling between two buttons
        {
            // move on same button
            if(neobox.parser.y == y && neobox.parser.x == x)
move:           TYPEFUNC(elem_last, move, event=, y, x, button_y,
                    button_x, map, elem_last, save_last);
            else
            {
                // check if moved to partner
                if(partner_last)
                    for(i=0; i<vector_size(partner_last->connect); i++)
                    {
                        p = vector_at(i, partner_last->connect);
                        if(p->y == y && p->x == x)
                            goto move;
                    }
                
                // move to other button
                TYPEFUNC(elem_last, focus_out, event=, neobox.parser.y,
                    neobox.parser.x, map, elem_last, save_last);
                if(event.type == NEOBOX_EVENT_NOP)
                    TYPEFUNC(elem_curr, focus_in, event=, y, x,
                        button_y, button_x, map, elem_curr, save_curr);
                else
                {
                    TYPEFUNC(elem_curr, focus_in, event2=, y, x,
                        button_y, button_x, map, elem_curr, save_curr);
                    neobox_queue_event(STASH_NEOBOX, &event2);
                }
            }
        }
        break;
    case IOD_EVENT_RELEASED:
        if(neobox.parser.pressed)
        {
            map_prev = neobox.parser.map;
            
            TYPEFUNC(elem_curr, release, event=, y, x, button_y,
                button_x, map, elem_curr, save_curr);
            
            if(elem_curr->options & NEOBOX_LAYOUT_OPTION_SET_MAP)
                neobox.parser.map_main = neobox.parser.map;
            
            if(neobox.options & NEOBOX_OPTION_PRINT_MASK &&
                map_prev != neobox.parser.map &&
                (neobox.options & NEOBOX_OPTION_FORCE_PRINT ||
                !neobox.layout.maps[neobox.parser.map].invisible))
            {
                neobox_init_screen();
            }
            
            neobox.parser.pressed = 0;
        }
        break;
    case IOD_EVENT_PRESSED:
        if(!neobox.parser.pressed)
        {
            TYPEFUNC(elem_curr, press, event=, y, x, button_y, button_x,
                map, elem_curr, save_curr);
            
            neobox.parser.pressed = 1;
            
            neobox_add_timer(0, TIMER_SYSTEM, 0, DELAY);
        }
        break;
    }
    
    neobox.parser.y = y;
    neobox.parser.x = x;
    
    // notify framebuffer for redraw
    SIMV(send(neobox.fb.sock, &sim_tmp, 1, 0));
    
    // filter events
    if(neobox.filter)
        event = neobox.filter(event);
    
    return event;
}

int neobox_handle_return(int ret, struct neobox_event event, neobox_handler *handler, void *state)
{
    struct iod_cmd iod = { .cmd = IOD_CMD_REGISTER, .pid = 0 };
    
    switch(ret)
    {
    case NEOBOX_HANDLER_SUCCESS:
    case NEOBOX_HANDLER_QUIT:
        break;
    case NEOBOX_HANDLER_DEFER:
        switch(event.type)
        {
        case NEOBOX_EVENT_QUIT:
            return NEOBOX_HANDLER_QUIT;
        case NEOBOX_EVENT_SIGNAL:
            if(event.value.i == SIGINT)
            {
                event.type = NEOBOX_EVENT_QUIT;
                return neobox_handle_return(handler(event, state), event, 0, 0);
            }
            return NEOBOX_HANDLER_SUCCESS;
        case NEOBOX_EVENT_ACTIVATE:
            if(neobox.options & NEOBOX_OPTION_PRINT_MASK &&
                (neobox.options & NEOBOX_OPTION_FORCE_PRINT ||
                !neobox.layout.maps[neobox.parser.map].invisible))
            {
                neobox_init_screen();
            }
            return NEOBOX_HANDLER_SUCCESS;
        case NEOBOX_EVENT_DEACTIVATE:
            neobox_iod_cmd(IOD_CMD_ACK, 0, IOD_EVENT_DEACTIVATED);
            return NEOBOX_HANDLER_SUCCESS;
        case NEOBOX_EVENT_REMOVE:
            neobox_iod_cmd(IOD_CMD_ACK, 0, IOD_EVENT_REMOVED);
            event.type = NEOBOX_EVENT_QUIT;
            return neobox_handle_return(handler(event, state), event, 0, 0);
        case NEOBOX_EVENT_SYSTEM:
            event.type = NEOBOX_EVENT_NOP;
            switch(event.value.i)
            {
            case NEOBOX_SYSTEM_NEXT:
                iod.cmd = IOD_CMD_SWITCH;
                iod.value = IOD_SWITCH_NEXT;
                break;
            case NEOBOX_SYSTEM_PREV:
                iod.cmd = IOD_CMD_SWITCH;
                iod.value = IOD_SWITCH_PREV;
                break;
            case NEOBOX_SYSTEM_QUIT:
                event.type = NEOBOX_EVENT_QUIT;
                break;
            case NEOBOX_SYSTEM_ACTIVATE:
                break;
            case NEOBOX_SYSTEM_MENU:
                iod.cmd = IOD_CMD_SWITCH;
                iod.value = IOD_SWITCH_HIDDEN;
                break;
            }
            if(event.type != NEOBOX_EVENT_NOP)
                return neobox_handle_return(handler(event, state), event, 0, 0);
            else if(iod.cmd != IOD_CMD_REGISTER)
                if(neobox_iod_cmd(iod.cmd, 0, iod.value))
                    neobox_iod_reconnect(-1);
            return NEOBOX_HANDLER_SUCCESS;
        default:
            return NEOBOX_HANDLER_SUCCESS;
        }
        break;
    default:
        if(ret & NEOBOX_HANDLER_ERROR)
            break;
        else
        {
            VERBOSE(printf("[NEOBOX] Unrecognized return code %i\n", ret));
            return NEOBOX_HANDLER_SUCCESS;
        }
    }
    return ret;
}

int neobox_handle_event(neobox_handler *handler, void *state)
{
    struct iod_event iod_event;
    struct neobox_event event;
    
    if(neobox_iod_recv(&iod_event))
        return NEOBOX_HANDLER_SUCCESS;
    
    event = neobox_parse_iod_event(iod_event);
    
    if(event.type != NEOBOX_EVENT_NOP)
        return neobox_handle_return(handler(event, state), event, handler, state);
    else
        return NEOBOX_HANDLER_SUCCESS;
}

int neobox_handle_queue(neobox_handler *handler, void *state, sigset_t *oldset)
{
    struct neobox_stash stash;
    struct neobox_event event;
    int ret;
    
    while((stash = neobox_get_event(oldset)).type != STASH_NOP)
    {
        if(oldset)
            sigprocmask(SIG_SETMASK, oldset, 0);
        switch(stash.type)
        {
        case STASH_IOD:
            event = neobox_parse_iod_event(stash.event.iod);
            break;
        case STASH_NEOBOX:
            event = stash.event.neobox;
            break;
        }
        ret = neobox_handle_return(handler(event, state), event, handler, state);
        if(ret == NEOBOX_HANDLER_SUCCESS)
            continue;
        else
            return ret;
    }
    
    return NEOBOX_HANDLER_SUCCESS;
}

int neobox_run_pfds(neobox_handler *handler, void *state, struct pollfd *pfds, int count)
{
    struct neobox_event event;
    int ret, i;
    sigset_t set;
    
    pfds[0].fd = neobox.iod.sock;
    pfds[0].events = POLLIN;
    
    VERBOSE(printf("[NEOBOX] run\n"));
    
    while(1)
    {
        switch((ret = neobox_handle_queue(handler, state, &set)))
        {
        case NEOBOX_HANDLER_SUCCESS:
            break;
        case NEOBOX_HANDLER_QUIT:
            return 0;
        default:
            return ret & ~NEOBOX_HANDLER_ERROR;
        }
        
        if(ppoll(pfds, count, 0, &set) == -1)
        {
            sigprocmask(SIG_SETMASK, &set, 0);
            
            if(errno == EINTR)
                continue;
            
            VERBOSE(perror("[NEOBOX] Failed to poll"));
            return NEOBOX_ERROR_POLL;
        }
        
        sigprocmask(SIG_SETMASK, &set, 0);
        
        if(pfds[0].revents & POLLHUP || pfds[0].revents & POLLERR)
        {
            VERBOSE(printf("[NEOBOX] Failed to poll iod socket\n"));
            neobox_iod_reconnect(-1);
        }
        else if(pfds[0].revents & POLLIN)
        {
            ret = neobox_handle_event(handler, state);
handle:     switch(ret)
            {
            case NEOBOX_HANDLER_SUCCESS:
                break;
            case NEOBOX_HANDLER_QUIT:
                return 0;
            default:
                return ret & ~NEOBOX_HANDLER_ERROR;
            }
        }
        else
        {
            event.type = NEOBOX_EVENT_NOP;
            
            for(i=1; i<count; i++)
            {
                event.id = i;
                event.value.i = pfds[i].fd;
                
                if(pfds[i].revents & POLLHUP || pfds[i].revents & POLLERR)
                {
                    pfds[i].events = 0;
                    event.type = NEOBOX_EVENT_POLLHUPERR;
                }
                else if(pfds[i].revents & POLLIN)
                    event.type = NEOBOX_EVENT_POLLIN;
                else if(pfds[i].revents & POLLOUT)
                    event.type = NEOBOX_EVENT_POLLOUT;
                
                if(event.type != NEOBOX_EVENT_NOP)
                {
                    ret = neobox_handle_return(handler(event, state),
                        event, handler, state);
                    goto handle;
                }
            }
        }
    }
    
    return 0;
}

int neobox_run(neobox_handler *handler, void *state)
{
    struct pollfd pfds[1];
    return neobox_run_pfds(handler, state, pfds, 1);
}

void neobox_switch(pid_t pid)
{
    while(neobox_iod_cmd(IOD_CMD_SWITCH, pid, IOD_SWITCH_PID))
        neobox_iod_reconnect(-1);
}

void neobox_hide(pid_t pid, int priority, int hide)
{
    while(neobox_iod_cmd(IOD_CMD_HIDE, pid,
        hide ? IOD_HIDE_MASK|priority : priority))
    {
        neobox_iod_reconnect(-1);
    }
    neobox.iod.hide = hide;
    neobox.iod.priority = priority;
}

void neobox_powersave(int powersave)
{
    while(neobox_iod_cmd(IOD_CMD_POWERSAVE, 0, powersave))
        neobox_iod_reconnect(-1);
}

int neobox_lock(int lock)
{
    struct iod_event event;
    
    while(neobox_iod_cmd(IOD_CMD_LOCK, 0, lock))
        neobox_iod_reconnect(-1);
    
    neobox.iod.lock = lock;
    
    while(1)
    {
        while(neobox_iod_recv(&event))
            neobox_iod_reconnect(-1);
        
        if(event.event != IOD_EVENT_LOCK)
        {
            neobox_queue_event(STASH_IOD, &event);
            continue;
        }
        
        if(event.value.status & IOD_SUCCESS_MASK)
        {
            VERBOSE(printf("[NEOBOX] %s success\n", lock ? "lock" : "unlock"));
            return NEOBOX_SET_SUCCESS;
        }
        else
        {
            VERBOSE(printf("[NEOBOX] %s failure\n", lock ? "lock" : "unlock"));
            neobox.iod.lock = 0;
            return NEOBOX_SET_FAILURE;
        }
    }
}

int neobox_grab(int button, int grab)
{
    struct iod_event event;
    int value = 0;
    
    if(grab)
        value |= IOD_GRAB_MASK;
    switch(button)
    {
    case NEOBOX_BUTTON_AUX:
        value |= IOD_GRAB_AUX;
        break;
    case NEOBOX_BUTTON_POWER:
        value |= IOD_GRAB_POWER;
        break;
    default:
        return NEOBOX_SET_FAILURE;
    }
    
    while(neobox_iod_cmd(IOD_CMD_GRAB, 0, value))
        neobox_iod_reconnect(-1);
    
    if(grab)
        neobox.iod.grab |= button;
    else
        neobox.iod.grab &= ~button;
        
    while(1)
    {
        while(neobox_iod_recv(&event))
            neobox_iod_reconnect(-1);
        
        if(event.event != IOD_EVENT_GRAB ||
            (event.value.status & ~IOD_SUCCESS_MASK) !=
            (value & ~IOD_GRAB_MASK))
        {
            neobox_queue_event(STASH_IOD, &event);
            continue;
        }
        
        if(event.value.status & IOD_SUCCESS_MASK)
        {
            VERBOSE(printf("[NEOBOX] %s success\n", grab ? "grab" : "ungrab"));
            return NEOBOX_SET_SUCCESS;
        }
        else
        {
            VERBOSE(printf("[NEOBOX] %s failure\n", grab ? "grab" : "ungrab"));
            neobox.iod.grab &= ~button;
            return NEOBOX_SET_FAILURE;
        }
    }
}

int neobox_timer(unsigned char id, unsigned int sec, unsigned int msec)
{
    return neobox_add_timer(id, TIMER_USER, sec, msec);
}

void neobox_timer_remove(unsigned char id)
{
    struct neobox_chain_timer *ct;
    sigset_t sig, oldset;
    
    sigfillset(&sig);
    sigprocmask(SIG_BLOCK, &sig, &oldset);
    
    ct = neobox.timer.cqh_first;
    
    while(ct != (void*)&neobox.timer &&
        (ct->type != TIMER_USER || ct->id != id))
    {
        ct = ct->chain.cqe_next;
    }
    
    if(ct != (void*)&neobox.timer)
    {
        VERBOSE(printf("[NEOBOX] timer %i removed\n", id));
        CIRCLEQ_REMOVE(&neobox.timer, ct, chain);
        free(ct);
    }
    
    sigprocmask(SIG_SETMASK, &oldset, 0);
}

int neobox_sleep(unsigned int sec, unsigned int msec)
{
    sigset_t set, oldset;
    int ret;
    
    neobox.sleep = 1;
    
    if((ret = neobox_add_timer(1, TIMER_SYSTEM, sec, msec)))
        return ret;
    
    VERBOSE(printf("[NEOBOX] sleep %u.%u\n", sec, msec));
    
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, &oldset);
    
    while(neobox.sleep)
        sigsuspend(&oldset);
    
    sigprocmask(SIG_SETMASK, &oldset, 0);
    
    return 0;
}

void neobox_map_set(int map)
{
    neobox.parser.map = neobox.parser.map_main = map;
}

void neobox_map_reset()
{
    neobox.parser.map = neobox.parser.map_main = neobox.layout.start;
}
