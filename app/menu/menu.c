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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <neobox.h>
#include <neobox_util.h>
#include <neobox_select.h>
#include <neobox_config.h>

#include "menu_layout.h"

struct app
{
    char *name, *cmd, **argv;
    pid_t pid;
};

struct app *apps;
int appcap, appcount, apppos;
int active, verbose;

int exec(int id)
{
    pid_t pid;
    
    switch((pid = fork()))
    {
    case -1:
        perror("Failed to fork child\n");
        return 1;
        break;
    case 0:
        if(execvp(apps[id].cmd, apps[id].argv) == -1)
        {
            perror("Failed to start app");
            return 1;
        }
        break;
    default:
        if(verbose)
            printf("'%s' started (%i)\n", apps[id].name, pid);
        apps[id].pid = pid;
        break;
    }
    return 0;
}

void set_apps(int draw)
{
    int i, j;
    
    for(i=0,j=apppos; i<10; i++,j++)
    {
        if(j < appcount)
        {
            if(apps[j].pid)
            {
                neobox_select_set_active(i, 0, 1, 0);
                neobox_select_set_locked(i, 0, 1);
            }
            else
            {
                neobox_select_set_active(i, 0, 0, 0);
                neobox_select_set_locked(i, 0, 0);
            }
            neobox_select_set_name(i, 0, apps[j].name, draw);
        }
        else
        {
            neobox_select_set_active(i, 0, 0, 0);
            neobox_select_set_locked(i, 0, 1);
            neobox_select_set_name(i, 0, 0, draw);
        }
    }
}

int handler(struct neobox_event event, void *state)
{
    pid_t pid;
    int i;
    
    switch(event.type)
    {
    case NEOBOX_EVENT_CHAR:
        switch(event.value.c.c[0])
        {
        case 'k':
            break;
        case 'u':
            if(apppos > 0)
            {
                apppos -= apppos > 5 ? 5 : apppos;
                set_apps(1);
            }
            break;
        case 'd':
            if(apppos+10 < appcount)
            {
                apppos += appcount-apppos-10 > 5 ? 5 : appcount-apppos-10;
                set_apps(1);
            }
            break;
        }
        return NEOBOX_HANDLER_SUCCESS;
    case NEOBOX_EVENT_INT:
        if(!event.value.i)
            break;
        if(!apps[apppos+event.id].pid)
        {
            if(exec(apppos+event.id))
                break;
            neobox_select_set_locked(event.id, 0, 1);
        }
        else
            neobox_switch(apps[apppos+event.id].pid);
        return NEOBOX_HANDLER_SUCCESS;
    case NEOBOX_EVENT_SYSTEM:
        if(event.value.i == NEOBOX_SYSTEM_MENU)
            return NEOBOX_HANDLER_SUCCESS;
        break;
    case NEOBOX_EVENT_DEACTIVATE:
        active = 0;
        break;
    case NEOBOX_EVENT_ACTIVATE:
        active = 1;
        break;
    case NEOBOX_EVENT_SIGNAL:
        switch(event.value.i)
        {
        case SIGCHLD:
            pid = wait(0);
            for(i=0; i<appcount; i++)
                if(apps[i].pid == pid)
                {
                    if(verbose)
                        printf("'%s' terminated (%i)\n", apps[i].name, pid);
                    apps[i].pid = 0;
                    if(i-apppos < 10)
                    {
                        neobox_select_set_locked(i-apppos, 0, 0);
                        neobox_select_set_active(i-apppos, 0, 0, active);
                    }
                    break;
                }
            return NEOBOX_HANDLER_SUCCESS;
        }
        break;
    }
    
    return NEOBOX_HANDLER_DEFER;
}

int init_apps()
{
    char *key, *value;
    
    if(neobox_config_list("apps"))
    {
        fprintf(stderr, "Missing 'apps' config section\n");
        return 1;
    }
    
    appcount = 0;
    appcap = 10;
    apppos = 0;
    apps = malloc(appcap*sizeof(struct app));
    
    while(1)
    {
        neobox_config_next(&key, &value);
        if(!key)
            break;
        
        if(appcount == appcap)
        {
            appcap += 10;
            apps = realloc(apps, appcap*sizeof(struct app));
        }
        
        apps[appcount].name = key;
        apps[appcount].cmd = value;
        apps[appcount].argv = neobox_util_parse_cmd(value);
        apps[appcount].argv[0] = key;
        apps[appcount].pid = 0;
        appcount++;
    }
    
    return 0;
}

int usage(const char *name)
{
    printf("Usage: %s [-v]\n", name);
    return 0;
}

int main(int argc, char* argv[])
{
    struct neobox_options options = neobox_options_default(&argc, argv);
    int opt, ret;
    
    options.format = NEOBOX_FORMAT_PORTRAIT;
    options.layout = menuLayout;
    
    if((ret = neobox_init_custom(options)) < 0)
        return ret;
    
    active = 1;
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
    
    if((ret = init_apps()))
    {
        neobox_finish();
        return ret;
    }
    
    set_apps(0);
    
    neobox_hide(0, 1, 1);
    neobox_catch_signal(SIGCHLD, SA_NOCLDSTOP);
    
    ret = neobox_run(handler, 0);
    
    neobox_finish();
    
    free(apps);
    
    return ret;
}
