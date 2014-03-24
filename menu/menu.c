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

#include <tkbio.h>
#include <tkbio_select.h>

#include "menu_layout.h"

struct app
{
    char *name, *cmd, **argv;
    pid_t pid;
};

struct app *apps;
int appcap, appcount, apppos;
int active, verbose;

void exec(int id)
{
    pid_t pid;
    
    switch((pid = fork()))
    {
    case -1:
        perror("Failed to fork child\n");
        break;
    case 0:
        if(execvp(apps[id].cmd, apps[id].argv) == -1)
            perror("Failed to start app");
        break;
    default:
        if(verbose)
            printf("'%s' started (%i)\n", apps[id].name, pid);
        apps[id].pid = pid;
        break;
    }
}

void redraw()
{
    int i, j;
    
    for(i=0,j=apppos; i<10; i++,j++)
    {
        if(j < appcount)
        {
            if(apps[j].pid)
            {
                tkbio_select_set_active(i, 0, 1, 0);
                tkbio_select_set_locked(i, 0, 1);
            }
            else
            {
                tkbio_select_set_active(i, 0, 0, 0);
                tkbio_select_set_locked(i, 0, 0);
            }
            tkbio_select_set_name(i, 0, apps[j].name, 1);
        }
        else
        {
            tkbio_select_set_active(i, 0, 0, 0);
            tkbio_select_set_locked(i, 0, 1);
            tkbio_select_set_name(i, 0, 0, 1);
        }
    }
}

int handler(struct tkbio_return ret, void *state)
{
    switch(ret.type)
    {
    case TKBIO_RETURN_CHAR:
        switch(ret.value.c.c[0])
        {
        case 'k':
            break;
        case 'u':
            if(apppos > 0)
            {
                apppos -= apppos > 5 ? 5 : apppos;
                redraw();
            }
            break;
        case 'd':
            if(apppos+10 < appcount)
            {
                apppos += appcount-apppos-10 > 5 ? 5 : appcount-apppos-10;
                redraw();
            }
            break;
        }
        break;
    case TKBIO_RETURN_INT:
        if(!ret.value.i)
            break;
        if(!apps[apppos+ret.id].pid)
        {
            exec(apppos+ret.id);
            tkbio_select_set_locked(ret.id, 0, 1);
        }
        else
            tkbio_switch(apps[apppos+ret.id].pid);
        break;
    case TKBIO_RETURN_SWITCH:
        active = 0;
        break;
    case TKBIO_RETURN_ACTIVATE:
        active = 1;
        break;
    }
    return 0;
}

int init_apps(const char *file)
{
    FILE *f;
    char *line = 0, *ptr, *ptr2;
    int i, count, err;
    size_t size;
    
    if(!(f = fopen(file, "r")))
    {
        err = errno;
        perror("Failed to open app file");
        return err;
    }
    
    appcount = 0;
    appcap = 10;
    apppos = 0;
    apps = malloc(appcap*sizeof(struct app));
    
    while((count = getline(&line, &size, f)) != -1)
    {
        if(line[0] == '\n')
            continue;
        
        if(!(ptr = strchr(line, ':')))
            continue;
        
        if(appcount == appcap)
        {
            appcap += 10;
            apps = realloc(apps, appcap*sizeof(struct app));
        }
        
        if(line[count-1] == '\n')
            line[count-1] = 0;
        
        *ptr = 0;
        ptr++;
        apps[appcount].name = line;
        apps[appcount].cmd = ptr;
        
        if(!(ptr = strchr(ptr, ' ')))
        {
            apps[appcount].argv = malloc(sizeof(char*));
            apps[appcount].argv[0] = apps[appcount].name;
            appcount++;
            line = 0;
            continue;
        }
        
        *ptr = 0;
        ptr++;
        ptr2 = ptr;
        count = 0;
        
        while(*(ptr2++))
            if(isspace(ptr2[0]) && !isspace(ptr2[-1]))
                count++;
        
        if(isspace(ptr2[-2]))
            count--;
        
        i = 0;
        apps[appcount].argv = malloc((count+3)*sizeof(char*));
        apps[appcount].argv[i++] = apps[appcount].name;
        
        ptr = strtok(ptr, " ");
        apps[appcount].argv[i++] = ptr;
        
        while((ptr = strtok(0, " ")))
            apps[appcount].argv[i++] = ptr;
        
        apps[appcount].argv[i] = 0;
        appcount++;
        line = 0;
    }
    
    if(line)
        free(line);
    
    if(ferror(f))
    {
        fprintf(stderr, "Failed to read app file\n");
        fclose(f);
        return 1;
    }
    
    fclose(f);
    
    return 0;
}

void free_apps()
{
    int i;
    
    for(i=0; i<appcount; i++)
    {
        free(apps[i].argv);
        free(apps[i].name);
    }
    
    free(apps);
}

void sighandler(int signal)
{
    pid_t pid;
    int i;
    
    pid = wait(0);
    
    for(i=0; i<appcount; i++)
        if(apps[i].pid == pid)
        {
            if(verbose)
                printf("'%s' terminated (%i)\n", apps[i].name, pid);
            apps[i].pid = 0;
            if(i-apppos < 10)
            {
                tkbio_select_set_locked(i-apppos, 0, 0);
                tkbio_select_set_active(i-apppos, 0, 0, active);
            }
            break;
        }
}

int usage(const char *name)
{
    printf("Usage: %s [-v] <file>\n", name);
    return 0;
}

int main(int argc, char* argv[])
{
    struct tkbio_config config = tkbio_config_default(&argc, argv);
    int err, opt;
    
    config.format = TKBIO_FORMAT_PORTRAIT;
    config.layout = menuLayout;
    
    tkbio_init_custom(config);
    
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
    
    if(optind == argc)
        return usage(argv[0]);
    
    if((err = init_apps(argv[optind])))
    {
        tkbio_finish();
        return err;
    }
    
    redraw();
    
    tkbio_signal_set_handler(SIGCHLD, SA_NOCLDSTOP, sighandler);
    
    tkbio_run(handler, 0);
    tkbio_finish();
    
    free_apps();
    
    return 0;
}
