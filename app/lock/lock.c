/*
 * Copyright (c) 2016 Martin Rödel aka Yomin
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
#include <neobox_text.h>

#include "lock_layout.h"

#define BRIGHTNESS_LEN_MAX 20

char *pass, **cmd, *brightness_dev, brightness_value[BRIGHTNESS_LEN_MAX];
pid_t cmd_pid;
int powersave, brightness_len, password, verbose;

void unlock()
{
    if(cmd_pid != -1)
    {
        neobox_switch(cmd_pid);
        return;
    }
    
    switch((cmd_pid = fork()))
    {
    case -1:
        perror("Failed to fork");
        break;
    case 0:
        if(execvp(cmd[0], cmd) == -1)
            perror("Failed to exec cmd");
        break;
    }
}

void set_brightness(int on)
{
    int fd;
    
    if((fd = open(brightness_dev, O_RDWR)) == -1)
    {
        perror("Failed to open brightness device");
        brightness_len = 0;
        return;
    }
    
    if(on)
    {
        if(!brightness_len)
        {
            fprintf(stderr, "Failed to restore brightness: unset\n");
            return;
        }
        
        if(write(fd, brightness_value, brightness_len) == -1)
            perror("Failed to restore brightness");
    }
    else
    {
        if((brightness_len = read(fd, brightness_value, BRIGHTNESS_LEN_MAX)) <= 0)
        {
            perror("Failed to read brightness");
            brightness_len = 0;
            close(fd);
            return;
        }
        
        if(brightness_len == BRIGHTNESS_LEN_MAX)
        {
            fprintf(stderr, "Failed to read brightness: too large\n");
            brightness_len = 0;
            close(fd);
            return;
        }
        
        brightness_len--;
        
        if(write(fd, "0", 1) == -1)
            perror("Failed to zero brightness");
    }
    
    close(fd);
}

int handler(struct neobox_event event, void *state)
{
    switch(event.type)
    {
    case NEOBOX_EVENT_CHAR:
        switch(event.value.c.c[0])
        {
        case 's':
            if(password)
            {
                neobox_text_password(ID_PASS, 0, NEOBOX_TEXT_PASSWORD_OFF, 1);
                neobox_button_set_name(ID_SHOW, 0, "hide", 1);
            }
            else
            {
                neobox_text_password(ID_PASS, 0, NEOBOX_TEXT_PASSWORD_ALLBUTONE, 1);
                neobox_button_set_name(ID_SHOW, 0, "show", 1);
            }
            password = !password;
            break;
        case 'u':
            if(!strcmp(pass, neobox_text_get(ID_PASS, 0)))
            {
                if(verbose)
                    printf("unlocked\n");
                neobox_text_reset(ID_PASS, 0, 1);
                if(!password)
                {
                    neobox_text_password(ID_PASS, 0, NEOBOX_TEXT_PASSWORD_ALLBUTONE, 1);
                    neobox_button_set_name(ID_SHOW, 0, "show", 1);
                }
                unlock();
            }
            else if(verbose)
                printf("wrong password\n");
            break;
        }
        return NEOBOX_HANDLER_SUCCESS;
    case NEOBOX_EVENT_BUTTON:
        switch(event.id)
        {
        case NEOBOX_BUTTON_POWER:
            if(!event.value.i)
                break;
            powersave = !powersave;
powersave:  if(neobox_lock(powersave))
                printf("Failed to %s screen\n", powersave?"lock":"unlock");
            set_brightness(!powersave);
            neobox_powersave(powersave);
            break;
        }
        return NEOBOX_HANDLER_SUCCESS;
    case NEOBOX_EVENT_SIGNAL:
        switch(event.value.i)
        {
        case SIGCHLD:
            wait(0);
            cmd_pid = -1;
            return NEOBOX_HANDLER_SUCCESS;
        case SIGHUP:
            goto powersave;
        }
    }
    
    return NEOBOX_HANDLER_DEFER;
}

int usage(const char *name)
{
    printf("Usage: %s [-v]\n", name);
    return 0;
}

int main(int argc, char* argv[])
{
    int ret, opt;
    char *config_cmd;
    
    if((ret = neobox_init_layout(lockLayout, &argc, argv)) < 0)
        return ret;
    
    cmd_pid = -1;
    cmd = 0;
    powersave = 0;
    brightness_dev = 0;
    password = 1;
    verbose = 0;
    
    while((opt = getopt(argc, argv, "v")) != -1)
    {
        switch(opt)
        {
        case 'v':
            verbose = 1;
            break;
        default:
            return usage(argv[0]);
        }
    }
    
    if(!(config_cmd = neobox_config("cmd", 0)))
    {
        fprintf(stderr, "Cmd not configured\n");
        neobox_finish();
        return 1;
    }
    
    if(!(brightness_dev = neobox_config("brightness", 0)))
    {
        fprintf(stderr, "Brightness device not configured\n");
        neobox_finish();
        return 2;
    }
    
    if((ret = open(brightness_dev, O_RDWR)) == -1)
    {
        perror("Failed to open brightness device");
        neobox_finish();
        return 3;
    }
    
    close(ret);
    
    if(!(pass = neobox_config("password", 0)))
    {
        fprintf(stderr, "Password not configured\n");
        neobox_finish();
        return 4;
    }
    
    cmd = neobox_util_parse_cmd(config_cmd);
    
    neobox_hide(0, 0, 1);
    neobox_grab(NEOBOX_BUTTON_POWER, 1);
    neobox_catch_signal(SIGCHLD, SA_NOCLDSTOP);
    neobox_catch_signal(SIGHUP, 0);
    
    ret = neobox_run(handler, 0);
    
    free(cmd);
    
    neobox_finish();
    
    return ret;
}

