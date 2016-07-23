/*
 * Copyright (c) 2014 Martin Rödel aka Yomin
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

#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <neobox.h>
#include <neobox_util.h>
#include <neobox_button.h>
#include <neobox_config.h>
#include <neobox_log.h>

#include "login_layout.h"

#define LEN 9

unsigned int seeds[26];
char tile[LEN], rand_tile[LEN*2], current_tile;
char *pass, *passptr, **cmd, *brightness_dev, brightness_value[20];
pid_t cmd_pid;
int powersave, brightness_len;

int gen_tile(char c)
{
    const char *ptr = pass;
    int i = 0;
    unsigned int seed = seeds[c-'a'];
    char r;
    
    memset(tile, 0, LEN);
    
    srandom(seed);
    
    current_tile = c;
    
    tile[i++] = (seed = random())%26+'a';
    
    while((ptr = strchr(ptr, c)) && ptr[1])
    {
        if(i == LEN || c == ptr[1])
            return 0;
        if(!strchr(tile, ptr[1]))
            tile[i++] = ptr[1];
        ptr += 2;
    }
    
    while(i < LEN)
        if(!strchr(tile, (r = (seed = random())%26+'a')) && r != c)
            tile[i++] = r;
    
    return seed;
}

void draw_tile(char c, int draw)
{
    int i, j;
    
    gen_tile(c);
    
    srandom(time(0));
    memset(rand_tile, 0, LEN*2);
    rand_tile[0] = tile[0];
    
    for(i=1; i<LEN; i++)
    {
        while(rand_tile[(j = (random()%(LEN-1)+1)*2)]);
        rand_tile[j] = tile[i];
    }
    
    for(i=0; i<LEN; i++)
        neobox_button_set_name(i, 0, &rand_tile[i*2], draw);
}

void login()
{
    if(cmd_pid != -1)
    {
        neobox_switch(cmd_pid);
        return;
    }
    
    switch((cmd_pid = fork()))
    {
    case -1:
        neobox_app_perror("Failed to fork");
        break;
    case 0:
        if(execvp(cmd[0], cmd) == -1)
            neobox_app_perror("Failed to exec cmd");
        return;
    }
}

void zero_brightness()
{
    int fd;
    
    if((fd = open(brightness_dev, O_RDWR)) == -1)
    {
        neobox_app_perror("Failed to open brightness device");
        brightness_len = 0;
        return;
    }
    
    if((brightness_len = read(fd, brightness_value, 20)) <= 0)
    {
        neobox_app_perror("Failed to read brightness");
        brightness_len = 0;
        close(fd);
        return;
    }
    
    brightness_len--;
    
    if(write(fd, "0", 1) == -1)
        neobox_app_perror("Failed to zero brightness");
    
    close(fd);
}

void restore_brightness()
{
    int fd;
    
    if(!brightness_len)
        return;
    
    if((fd = open(brightness_dev, O_RDWR)) == -1)
    {
        neobox_app_perror("Failed to open brightness device");
        return;
    }
    
    if(write(fd, brightness_value, brightness_len) == -1)
        neobox_app_perror("Failed to restore brightness");
    
    close(fd);
}

int handler(struct neobox_event event, void *state)
{
    char c;
    
    switch(event.type)
    {
    case NEOBOX_EVENT_CHAR:
        c = rand_tile[event.id*2];
        if(event.id)
        {
            passptr = c == *passptr ? passptr+1 : pass;
            if(!*passptr)
            {
                login();
                return NEOBOX_HANDLER_SUCCESS;
            }
        }
        else
        {
            srandom(time(0)+random());
            while(current_tile == (c = random()%26+'a'));
        }
        draw_tile(c, 1);
        break;
    case NEOBOX_EVENT_ACTIVATE:
        passptr = pass;
        srandom(time(0)+random());
        draw_tile(random()%26+'a', 0);
        return NEOBOX_HANDLER_DEFER;
    case NEOBOX_EVENT_BUTTON:
        switch(event.id)
        {
        case NEOBOX_BUTTON_POWER:
            if(!event.value.i)
                break;
powersave:  if(!powersave)
            {
                if(neobox_lock(1))
                    neobox_app_printf("Failed to lock screen\n");
                zero_brightness();
                neobox_powersave(1);
                powersave = 1;
            }
            else
            {
                if(neobox_lock(0))
                    neobox_app_printf("Failed to unlock screen\n");
                restore_brightness();
                neobox_switch(0);
                neobox_powersave(0);
                powersave = 0;
            }
            break;
        }
        return NEOBOX_HANDLER_SUCCESS;
    case NEOBOX_EVENT_SIGNAL:
        switch(event.value.i)
        {
        case SIGCHLD:
            wait(0);
            cmd_pid = -1;
            break;
        case SIGHUP:
            goto powersave;
        }
    default:
        return NEOBOX_HANDLER_DEFER;
    }
    
    return NEOBOX_HANDLER_SUCCESS;
}

int main(int argc, char* argv[])
{
    struct neobox_options options = neobox_options_default(&argc, argv);
    int ret, i;
    unsigned int seed;
    char *config_cmd, *config_seed;
    
    options.format = NEOBOX_FORMAT_PORTRAIT;
    options.layout = loginLayout;
    
    if((ret = neobox_init_custom(options)) < 0)
        return ret;
    
    cmd_pid = -1;
    cmd = 0;
    powersave = 0;
    brightness_dev = 0;
    
    if(!(config_cmd = neobox_config("cmd", 0)))
    {
        neobox_app_fprintf(stderr, "Cmd not configured\n");
        neobox_finish();
        return 1;
    }
    
    if(!(brightness_dev = neobox_config("brightness", 0)))
    {
        neobox_app_fprintf(stderr, "Brightness device not configured\n");
        neobox_finish();
        return 2;
    }
    
    if((ret = open(brightness_dev, O_RDWR)) == -1)
    {
        neobox_app_perror("Failed to open brightness device");
        neobox_finish();
        return 3;
    }
    
    close(ret);
    
    if(!(config_seed = neobox_config("seed", 0)))
    {
        neobox_app_fprintf(stderr, "Seed not configured\n");
        neobox_finish();
        return 4;
    }
    
    if(strspn(config_seed, "1234567890\n") != strlen(config_seed))
    {
        fprintf(stderr, "Seed not numeric\n");
        neobox_finish();
        return 5;
    }
    
    seed = atoi(config_seed);
    
    if(!(pass = neobox_config("password", 0)))
    {
        fprintf(stderr, "Password not configured\n");
        neobox_finish();
        return 6;
    }
    
    for(i=0; i<26; i++)
    {
        seeds[i] = seed;
        if(!(seed = gen_tile(i+'a')))
        {
            printf("Passphrase not usable\n");
            neobox_finish();
            return 7;
        }
    }
    
    cmd = neobox_util_parse_cmd(config_cmd);
    
    srandom(time(0));
    
    neobox_hide(0, 0, 1);
    neobox_grab(NEOBOX_BUTTON_POWER, 1);
    neobox_catch_signal(SIGCHLD, SA_NOCLDSTOP);
    neobox_catch_signal(SIGHUP, 0);
    
    ret = neobox_run(handler, 0);
    
    free(cmd);
    
    neobox_finish();
    
    return ret;
}

