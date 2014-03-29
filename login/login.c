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
#include <sys/wait.h>

#include <tkbio.h>
#include <tkbio_util.h>
#include <tkbio_button.h>

#include "login_layout.h"

#define LEN 9

unsigned int seeds[26];
char tile[LEN], randTile[LEN*2], currentTile;
const char *pass, *passptr;
char **cmd;
pid_t cmdPid;

int gen_tile(char c)
{
    const char *ptr = pass;
    int i = 0;
    unsigned int seed = seeds[c-'a'];
    char r;
    
    memset(tile, 0, LEN);
    
    srandom(seed);
    
    currentTile = c;
    
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
    memset(randTile, 0, LEN*2);
    randTile[0] = tile[0];
    
    for(i=1; i<LEN; i++)
    {
        while(randTile[(j = (random()%(LEN-1)+1)*2)]);
        randTile[j] = tile[i];
    }
    
    for(i=0; i<LEN; i++)
        tkbio_button_set_name(i, 0, &randTile[i*2], draw);
}

void login()
{
    if(cmdPid != -1)
    {
        tkbio_switch(cmdPid);
        return;
    }
    
    switch((cmdPid = fork()))
    {
    case -1:
        perror("Failed to fork");
        break;
    case 0:
        if(execvp(cmd[0], cmd) == -1)
            perror("Failed to exec cmd");
        return;
    }
}

int handler(struct tkbio_return ret, void *state)
{
    char c;
    
    switch(ret.type)
    {
    case TKBIO_RETURN_CHAR:
        c = randTile[ret.id*2];
        if(ret.id)
        {
            passptr = c == *passptr ? passptr+1 : pass;
            if(!*passptr)
            {
                login();
                return TKBIO_HANDLER_SUCCESS;
            }
        }
        else
        {
            srandom(time(0)+random());
            while(currentTile == (c = random()%26+'a'));
        }
        draw_tile(c, 1);
        break;
    case TKBIO_RETURN_ACTIVATE:
        passptr = pass;
        srandom(time(0)+random());
        draw_tile(random()%26+'a', 0);
        return TKBIO_HANDLER_DEFER;
    case TKBIO_RETURN_SIGNAL:
        switch(ret.value.i)
        {
        case SIGCHLD:
            wait(0);
            cmdPid = -1;
            break;
        case SIGHUP:
            tkbio_switch(0);
            break;
        }
    default:
        return TKBIO_HANDLER_DEFER;
    }
    
    return TKBIO_HANDLER_SUCCESS;
}

int main(int argc, char* argv[])
{
    struct tkbio_config config = tkbio_config_default(&argc, argv);
    int ret, i;
    unsigned int seed;
    FILE *file;
    size_t size;
    char *line = 0;
    
    if(argc != 2)
    {
        printf("Usage %s <file>\n", argv[0]);
        return 0;
    }
    
    if(!(file = fopen(argv[1], "r")))
    {
        perror("Failed to open passfile");
        return 1;
    }
    
    if(getline(&line, &size, file) == -1)
    {
        printf("Failed to read cmd\n");
        if(line)
            free(line);
        fclose(file);
        return 2;
    }
    
    cmdPid = -1;
    cmd = tkbio_util_parse_cmd(line);
    line = 0;
    
    if(getline(&line, &size, file) == -1)
    {
        printf("Failed to read seed\n");
        free(line);
        fclose(file);
        return 3;
    }
    
    if(strspn(line, "1234567890\n") != strlen(line))
    {
        printf("Seed not numeric\n");
        free(line);
        fclose(file);
        return 4;
    }
    
    seed = atoi(line);
    
    if(getline(&line, &size, file) == -1)
    {
        printf("Failed to read passphrase\n");
        free(line);
        fclose(file);
        return 5;
    }
    
    line[strlen(line)-1] = 0;
    pass = passptr = line;
    
    fclose(file);
    
    for(i=0; i<26; i++)
    {
        seeds[i] = seed;
        if(!(seed = gen_tile(i+'a')))
        {
            printf("Passphrase not usable\n");
            free(line);
            return 6;
        }
    }
    
    config.format = TKBIO_FORMAT_PORTRAIT;
    config.layout = loginLayout;
    if((ret = tkbio_init_custom(config)) < 0)
        return ret;
    
    srandom(time(0));
    
    tkbio_hide(0, 0, 1);
    tkbio_catch_signal(SIGCHLD, SA_NOCLDSTOP);
    tkbio_catch_signal(SIGHUP, 0);
    
    ret = tkbio_run(handler, 0);
    
    tkbio_finish();
    
    free(line);
    free(cmd[0]);
    free(cmd);
    
    return ret;
}

