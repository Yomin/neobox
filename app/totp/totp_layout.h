/*
 * Copyright (c) 2015 Martin Rödel aka Yomin
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

#ifndef __TOTP_LAYOUT_H__
#define __TOTP_LAYOUT_H__

#include "neobox_layout_default.h"

const unsigned char totp_colors[][4] =
    {
        {0x00, 0x00, 0x00, 0x00}, // bg
        {0xFF, 0x00, 0x00, 0x00}, // setup
        {0x00, 0xFF, 0x00, 0x00}, // name
        {0xB9, 0xA4, 0xFD, 0x00}, // token
        {0x55, 0x55, 0x55, 0x00}, // select
        {0xFF, 0xFF, 0x00, 0x00}, // text
        {0x00, 0xCC, 0xCC, 0x00}  // admin
   };

#define COLOR_BG     0
#define COLOR_FG     1
#define COLOR_NAME   2
#define COLOR_TOKEN  3
#define COLOR_SELECT 4
#define COLOR_TEXT   5
#define COLOR_ADMIN  6

#define TEXT_NAME    1
#define TEXT_TOKEN   2
#define TEXT_MSG     3

#define INPUT_NAME   1
#define INPUT_KEY    2
#define INPUT_HASH   3
#define INPUT_DIGITS 4
#define INPUT_STEP   5
#define INPUT_OFFSET 6

#define BUTTON_OK     1
#define BUTTON_CANCEL 2

#define VALUE(V) { .i = V }
#define ONE(C)   { .c = {{C,  0,  0,  0}} }

#define COLOR(C)        C,COLOR_BG,C
#define TEXTCOLOR(C)    C,COLOR_BG,COLOR_TEXT
#define DEFAULT_OPTIONS NEOBOX_LAYOUT_OPTION_BORDER

#define CHAR(N, C, I)       {N, NEOBOX_LAYOUT_TYPE_CHAR, I, ONE(C), COLOR(COLOR_FG), DEFAULT_OPTIONS}
#define LCHAR(N, C, I)      {N, NEOBOX_LAYOUT_TYPE_CHAR, I, ONE(C), COLOR(COLOR_FG), DEFAULT_OPTIONS|NEOBOX_LAYOUT_OPTION_CONNECT_LEFT}
#define NOP                 {0, NEOBOX_LAYOUT_TYPE_NOP, 0, ONE(0), COLOR(0), 0}
#define TEXT(T, C, O, I)    {T, NEOBOX_LAYOUT_TYPE_NOP, I, ONE(0), COLOR(C), O}
#define LTEXT(T, C, O, I)   {T, NEOBOX_LAYOUT_TYPE_NOP, I, ONE(0), COLOR(C), O|NEOBOX_LAYOUT_OPTION_CONNECT_LEFT}
#define GOTO(N, V, C)       {N, NEOBOX_LAYOUT_TYPE_GOTO, 0, VALUE(V), COLOR(C), DEFAULT_OPTIONS}
#define LGOTO(N, V, C)      {N, NEOBOX_LAYOUT_TYPE_GOTO, 0, VALUE(V), COLOR(C), DEFAULT_OPTIONS|NEOBOX_LAYOUT_OPTION_CONNECT_LEFT}
#define INPUT(N, I)         {N, NEOBOX_LAYOUT_TYPE_TEXT, I, VALUE(3), TEXTCOLOR(COLOR_SELECT), NEOBOX_LAYOUT_OPTION_ALIGN_LEFT}
#define LINPUT(N, I)        {N, NEOBOX_LAYOUT_TYPE_TEXT, I, VALUE(3), TEXTCOLOR(COLOR_SELECT), NEOBOX_LAYOUT_OPTION_ALIGN_LEFT|NEOBOX_LAYOUT_OPTION_CONNECT_LEFT}

#define NAME         TEXT(0, COLOR_NAME, 0, TEXT_NAME)
#define LNAME        LTEXT(0, COLOR_NAME, 0, TEXT_NAME)
#define TOKEN        TEXT(0, COLOR_TOKEN, 0, TEXT_TOKEN)
#define LTOKEN       LTEXT(0, COLOR_TOKEN, 0, TEXT_TOKEN)
#define SETUP        GOTO("Setup", 1, COLOR_FG)
#define LSETUP       LGOTO("Setup", 1, COLOR_FG)
#define ADMIN        GOTO("Admn", 2, COLOR_ADMIN)
#define LADMIN       LGOTO("Admn", 2, COLOR_ADMIN)
#define OPTION(N)    TEXT(N, COLOR_NAME, NEOBOX_LAYOUT_OPTION_ALIGN_LEFT, 0)
#define OPTGRP(N, I) OPTION(N),INPUT(N, I),LINPUT(N, I),LINPUT(N, I)
#define MSG          TEXT(0, COLOR_FG, NEOBOX_LAYOUT_OPTION_ALIGN_LEFT, TEXT_MSG)
#define LMSG         LTEXT(0, COLOR_FG, NEOBOX_LAYOUT_OPTION_ALIGN_LEFT, TEXT_MSG)
#define OK           CHAR("OK", 'k', BUTTON_OK)
#define CANCEL       CHAR("Cancel", 'c', BUTTON_CANCEL)

const struct neobox_mapelem totp_map[] =
    {
        NOP,   NOP,    NOP,    NOP,    NOP,    NOP,
        NOP,   NAME,   LNAME,  LNAME,  LNAME,  NOP,
        NOP,   TOKEN,  LTOKEN, LTOKEN, LTOKEN, NOP,
        NOP,   NOP,    NOP,    NOP,    NOP,    NOP,
        SETUP, LSETUP, NOP,    NOP,    ADMIN,  LADMIN
    };

const struct neobox_mapelem setup_map[] =
    {
        OPTGRP("Name", INPUT_NAME),
        OPTGRP("Key", INPUT_KEY),
        OPTGRP("Hash", INPUT_HASH),
        OPTGRP("Digits", INPUT_DIGITS),
        OPTGRP("Step", INPUT_STEP),
        OPTGRP("Offset", INPUT_OFFSET),
        MSG, LMSG, OK, CANCEL
    };

const struct neobox_map totp_maps[] =
    {
        {5, 6, totp_map, totp_colors},
        {7, 4, setup_map, totp_colors},
        ADMIN_MAP(2),
        NEOBOX_MAPS(3)
    };

const struct neobox_layout totpLayout =
    {
        .start  = 0,
        .size   = sizeof(totp_maps)/sizeof(struct neobox_map),
        .maps   = totp_maps,
        .fun    = 0
    };

#undef COLOR_BG
#undef COLOR_FG
#undef COLOR_NAME
#undef COLOR_TOKEN
#undef COLOR_SELECT
#undef COLOR_TEXT
#undef COLOR_ADMIN

#undef ONE
#undef VALUE

#undef COLOR
#undef TEXTCOLOR
#undef DEFAULT_OPTIONS

#undef CHAR
#undef LCHAR
#undef NOP
#undef TEXT
#undef LTEXT
#undef GOTO
#undef LGOTO
#undef INPUT
#undef LINPUT

#undef NAME
#undef LNAME
#undef TOKEN
#undef LTOKEN
#undef SETUP
#undef LSETUP
#undef ADMIN
#undef LADMIN
#undef OPTION
#undef OPTGRP
#undef MSG
#undef LMSG
#undef OK
#undef CANCEL

#endif

