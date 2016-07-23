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

#ifdef NEOBOX
#   include "neobox_def.h"
#endif

#include "neobox_config.h"
#include "neobox_log.h"

#include <rj_config.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#define PATH_MAX 256

#define FLAG_MALLOCED 1
#define FLAG_MODIFIED 2

static char **path;
static char *flags;
static struct recordjar **config;

#ifdef NEOBOX
extern struct neobox_global neobox;
#else
char *config_path, config_flags;
static struct recordjar *config_rj;
#endif

int neobox_config_open(const char *file)
{
    char self[PATH_MAX];
    int ret;
    ssize_t len, path_len;
    struct recordjar rj;
    
#ifdef NEOBOX
    path = &neobox.config.file;
    flags = &neobox.config.flags;
    config = (struct recordjar**)&neobox.config.rj;
#else
    path = &config_file;
    flags = &config_flags;
    config = &config_rj;
#endif
    
    if(*config)
    {
        if(*flags&FLAG_MALLOCED)
            free(*path);
        rj_free(*config);
        free(*config);
        *config = 0;
    }
    
    *path = (char*)file;
    
    *config = malloc(sizeof(struct recordjar));
    rj_init(*config); // init in case of fail to load
    
    if(!file || file[0] != '/')
    {
        if((len = readlink("/proc/self/exe", self, PATH_MAX-1)) == -1)
            return errno;
        path[len] = 0;
        if(!file)
            file = NEOBOX_CONFIG_FILE;
        path_len = snprintf(0, 0, "%s/%s", dirname(self), file);
        *path = malloc(path_len+1);
        *flags |= FLAG_MALLOCED;
        snprintf(*path, PATH_MAX, "%s/%s",
            strlen(self) == len ? dirname(self) : self, file);
        if((ret = rj_load(*path, &rj)))
            return ret;
    }
    else if((ret = rj_load(file, &rj)))
        return ret;
    
    memcpy(*config, &rj, sizeof(struct recordjar));
    
    return 0;
}

int neobox_config_save()
{
    if(!*config)
        return 0;
    
#ifdef NEOBOX
    neobox_printf(1, "config saved\n");
#endif
    
    return rj_save(*path, *config);
}

void neobox_config_close()
{
    if(!*config)
        return;
    
    if(*flags&FLAG_MODIFIED)
        neobox_config_save();
    
    if(*flags&FLAG_MALLOCED)
        free(*path);
    
    rj_free(*config);
    free(*config);
    *config = 0;
}

const char* neobox_config_strerror(int error)
{
    return rj_strerror(error);
}

char* neobox_config(const char *key, const char *def)
{
    return *config ? rj_config_get("main", key, def, *config) : (char*) def;
}

char* neobox_config_section(const char *section, const char *key, const char *def)
{
    return *config ? rj_config_get(section, key, def, *config) : (char*) def;
}

int neobox_config_list(const char *section)
{
    return *config ? rj_config_list(section, *config) : 1;
}

void neobox_config_next(char **key, char **value)
{
    if(!*config)
    {
        *key = 0;
        *value = 0;
        return;
    }
    rj_config_next(key, value, *config);
}

void neobox_config_set(const char *key, const char *value)
{
    if(*config)
    {
        rj_config_set("main", key, value, *config);
        *flags |= FLAG_MODIFIED;
    }
}

void neobox_config_set_section(const char *section, const char *key, const char *value)
{
    if(*config)
    {
        rj_config_set(section, key, value, *config);
        *flags |= FLAG_MODIFIED;
    }
}
