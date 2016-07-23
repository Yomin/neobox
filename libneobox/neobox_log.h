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

#ifndef __NEOBOX_LOG_H__
#define __NEOBOX_LOG_H__

#include <stdio.h>
#include <stdarg.h>

int  neobox_printf(int verbosity, const char *format, ...);
int  neobox_vprintf(int verbosity, const char *format, va_list ap);
int  neobox_fprintf(int verbosity, FILE *stream, const char *format, ...);
int  neobox_vfprintf(int verbosity, FILE *stream, const char *format, va_list ap);
void neobox_perror(int verbosity, const char *format, ...);

int  neobox_app_printf(const char *format, ...);
int  neobox_app_vprintf(const char *format, va_list ap);
int  neobox_app_fprintf(FILE *stream, const char *format, ...);
int  neobox_app_vfprintf(FILE *stream, const char *format, va_list ap);
void neobox_app_perror(const char *format, ...);

#endif

