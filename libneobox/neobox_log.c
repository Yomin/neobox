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

#include "neobox_log.h"
#include "neobox_def.h"

#include <string.h>
#include <errno.h>

extern struct neobox_global neobox;

int neobox_printf(int verbosity, const char *format, ...)
{
    int ret;
    va_list ap;
    
    va_start(ap, format);
    ret = neobox_vfprintf(verbosity, stdout, format, ap);
    va_end(ap);
    
    return ret;
}

int neobox_vprintf(int verbosity, const char *format, va_list ap)
{
    return neobox_vfprintf(verbosity, stdout, format, ap);
}

int neobox_fprintf(int verbosity, FILE *stream, const char *format, ...)
{
    int ret;
    va_list ap;
    
    va_start(ap, format);
    ret = neobox_vfprintf(verbosity, stream, format, ap);
    va_end(ap);
    
    return ret;
}

int neobox_vfprintf(int verbosity, FILE *stream, const char *format, va_list ap)
{
    int ret1, ret2;
    
    if(neobox.verbose < verbosity)
        return 0;
    
    if((ret1 = fprintf(stream, "[%s][NEOBOX] ", neobox.appname)) < 0)
        return ret1;
    if((ret2 = vfprintf(stream, format, ap)) < 0)
        return ret2;
    
    return ret1 + ret2;
}

void neobox_perror(int verbosity, const char *format, ...)
{
    va_list ap;
    int err = errno;
    
    if(neobox.verbose < verbosity)
        return;
    
    va_start(ap, format);
    neobox_vfprintf(verbosity, stderr, format, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(err));
}

int neobox_app_printf(const char *format, ...)
{
    int ret;
    va_list ap;
    
    va_start(ap, format);
    ret = neobox_app_vfprintf(stdout, format, ap);
    va_end(ap);
    
    return ret;
}

int neobox_app_vprintf(const char *format, va_list ap)
{
    return neobox_app_vfprintf(stdout, format, ap);
}

int neobox_app_fprintf(FILE *stream, const char *format, ...)
{
    int ret;
    va_list ap;
    
    va_start(ap, format);
    ret = neobox_app_vfprintf(stream, format, ap);
    va_end(ap);
    
    return ret;
}

int neobox_app_vfprintf(FILE *stream, const char *format, va_list ap)
{
    int ret1, ret2;
    
    if((ret1 = fprintf(stream, "[%s] ", neobox.appname)) < 0)
        return ret1;
    if((ret2 = vfprintf(stream, format, ap)) < 0)
        return ret2;
    
    return ret1 + ret2;
}

void neobox_app_perror(const char *format, ...)
{
    va_list ap;
    int err = errno;
    
    va_start(ap, format);
    neobox_app_vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(err));
}
