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
        perror("Failed to fork");
        break;
    case 0:
        if(execvp(cmd[0], cmd) == -1)
            perror("Failed to exec cmd");
        return;
    }
}

void zero_brightness()
{
    int fd;
    
    if((fd = open(brightness_dev, O_RDWR)) == -1)
    {
        perror("Failed to open brightness device");
        brightness_len = 0;
        return;
    }
    
    if((brightness_len = read(fd, brightness_value, 20)) <= 0)
    {
        perror("Failed to read brightness");
        brightness_len = 0;
        close(fd);
        return;
    }
    
    brightness_len--;
    
    if(write(fd, "0", 1) == -1)
        perror("Failed to zero brightness");
    
    close(fd);
}

void restore_brightness()
{
    int fd;
    
    if(!brightness_len)
        return;
    
    if((fd = open(brightness_dev, O_RDWR)) == -1)
    {
        perror("Failed to open brightness device");
        return;
    }
    
    if(write(fd, brightness_value, brightness_len) == -1)
        perror("Failed to restore brightness");
    
    close(fd);
}

int handler(struct neobox_return ret, void *state)
{
    char c;
    
    switch(ret.type)
    {
    case NEOBOX_RETURN_CHAR:
        c = rand_tile[ret.id*2];
        if(ret.id)
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
    case NEOBOX_RETURN_ACTIVATE:
        passptr = pass;
        srandom(time(0)+random());
        draw_tile(random()%26+'a', 0);
        return NEOBOX_HANDLER_DEFER;
    case NEOBOX_RETURN_BUTTON:
        switch(ret.id)
        {
        case NEOBOX_BUTTON_POWER:
            if(!ret.value.i)
                break;
powersave:  if(!powersave)
            {
                if(neobox_lock(1))
                    printf("Failed to lock screen\n");
                zero_brightness();
                neobox_powersave(1);
                powersave = 1;
            }
            else
            {
                if(neobox_lock(0))
                    printf("Failed to unlock screen\n");
                restore_brightness();
                neobox_switch(0);
                neobox_powersave(0);
                powersave = 0;
            }
            break;
        }
        return NEOBOX_HANDLER_SUCCESS;
    case NEOBOX_RETURN_SIGNAL:
        switch(ret.value.i)
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
    struct neobox_config config = neobox_config_default(&argc, argv);
    int ret, i, count;
    unsigned int seed;
    FILE *file = 0;
    size_t size;
    char *line = 0;
    
    cmd_pid = -1;
    cmd = 0;
    powersave = 0;
    brightness_dev = 0;
    
    if(argc != 2)
    {
        printf("Usage %s <file>\n", argv[0]);
        return 0;
    }
    
    if(!(file = fopen(argv[1], "r")))
    {
        perror("Failed to open config");
        return 1;
    }
    
    if(getline(&line, &size, file) == -1)
    {
        printf("Failed to read cmd\n");
        ret = 2;
        goto cleanup;
    }
    
    cmd = neobox_util_parse_cmd(line);
    line = 0;
    
    if((count = getline(&brightness_dev, &size, file)) == -1)
    {
        printf("Failed to read brightness device\n");
        ret = 3;
        goto cleanup;
    }
    
    brightness_dev[count-1] = 0;
    if((ret = open(brightness_dev, O_RDWR)) == -1)
    {
        perror("Failed to open brightness device");
        ret = 4;
        goto cleanup;
    }
    
    close(ret);
    
    if(getline(&line, &size, file) == -1)
    {
        printf("Failed to read seed\n");
        ret = 5;
        goto cleanup;
    }
    
    if(strspn(line, "1234567890\n") != strlen(line))
    {
        printf("Seed not numeric\n");
        ret = 6;
        goto cleanup;
    }
    
    seed = atoi(line);
    
    if((count = getline(&pass, &size, file)) == -1)
    {
        printf("Failed to read passphrase\n");
        ret = 7;
        goto cleanup;
    }
    
    pass[count-1] = 0;
    
    fclose(file);
    free(line);
    file = 0;
    line = 0;
    
    for(i=0; i<26; i++)
    {
        seeds[i] = seed;
        if(!(seed = gen_tile(i+'a')))
        {
            printf("Passphrase not usable\n");
            ret = 8;
            goto cleanup;
        }
    }
    
    config.format = NEOBOX_FORMAT_PORTRAIT;
    config.layout = loginLayout;
    if((ret = neobox_init_custom(config)) < 0)
        goto cleanup;
    
    srandom(time(0));
    
    neobox_hide(0, 0, 1);
    neobox_grab(NEOBOX_BUTTON_POWER, 1);
    neobox_catch_signal(SIGCHLD, SA_NOCLDSTOP);
    neobox_catch_signal(SIGHUP, 0);
    
    ret = neobox_run(handler, 0);
    
    neobox_finish();
    
cleanup:
    if(file)
        fclose(file);
    if(line)
        free(line);
    if(cmd)
    {
        free(cmd[0]);
        free(cmd);
    }
    if(brightness_dev)
        free(brightness_dev);
    
    return ret;
}

