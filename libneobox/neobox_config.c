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

#include "neobox_def.h"
#include "neobox_config.h"

#include <rj_config.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#define PATH_MAX 256

extern struct neobox_global neobox;

int neobox_config_open(const char *file)
{
    char path_self[PATH_MAX], path_config[PATH_MAX];
    int ret;
    ssize_t len;
    struct recordjar rj;
    
    if(neobox.config.rj)
    {
        rj_free(neobox.config.rj);
        free(neobox.config.rj);
        neobox.config.rj = 0;
    }
    
    if(!file || file[0] != '/')
    {
        if((len = readlink("/proc/self/exe", path_self, PATH_MAX-1)) == -1)
            return errno;
        path_self[len] = 0;
        if(!file)
            file = NEOBOX_CONFIG_FILE;
        snprintf(path_config, PATH_MAX, "%s/%s",
            dirname(path_self), file);
        if((ret = rj_load(path_config, &rj)))
            return ret;
    }
    else if((ret = rj_load(file, &rj)))
        return ret;
    
    neobox.config.rj = malloc(sizeof(struct recordjar));
    memcpy(neobox.config.rj, &rj, sizeof(struct recordjar));
    
    return 0;
}

void neobox_config_close()
{
    if(!neobox.config.rj)
        return;
    
    rj_free(neobox.config.rj);
    free(neobox.config.rj);
    neobox.config.rj = 0;
}

const char* neobox_config_strerror(int error)
{
    return rj_strerror(error);
}

char* neobox_config(const char *key, const char *def)
{
    return rj_config_get("main", key, def, neobox.config.rj);
}

char* neobox_config_section(const char *section, const char *key, const char *def)
{
    return rj_config_get(section, key, def, neobox.config.rj);
}

int neobox_config_list(const char *section)
{
    return rj_config_list(section, neobox.config.rj);
}

void neobox_config_next(char **key, char **value)
{
    rj_config_next(key, value, neobox.config.rj);
}

void neobox_config_set(const char *key, const char *value)
{
    rj_config_set("main", key, value, neobox.config.rj);
}

void neobox_config_set_section(const char *section, const char *key, const char *value)
{
    rj_config_set(section, key, value, neobox.config.rj);
}
