/*
 * Copyright (c) 2012-2014 Martin Rödel aka Yomin
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

#include <string.h>

#include "neobox_def.h"
#include "neobox_fb.h"
#include "neobox_layout.h"

#include "iso_font.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct text_control
{
    int control, len, left, right, cursor;
    const char *text;
};

typedef void draw_string_func(char c, unsigned char **base, unsigned char color_fg[4], unsigned char color_bg[4], int bg);

extern struct neobox_global neobox;
static unsigned char colorbuf[4];

unsigned char* neobox_color(unsigned char dst[4], int color, const struct neobox_map *map)
{
    const unsigned char *src = map->colors[color];
    
    if(!dst)
        dst = colorbuf;
    
    if(neobox.fb.bpp == 2)
    {
        dst[0] = (src[0]>>0 & ~7)  | (src[1]>>5 & 7);
        dst[1] = (src[1]<<3 & ~31) | (src[2]>>3 & 31);
    }
    else
        memcpy(dst, src, 4);
    
    return dst;
}

void neobox_get_sizes(int *height, int *width, int *fb_height, int *fb_width, int *screen_height, int *screen_width, const struct neobox_map *map)
{
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        if(height)        *height = neobox.fb.vinfo.xres/(map->height);
        if(width)         *width  = neobox.fb.vinfo.yres/(map->width);
        if(fb_height)     *fb_height = neobox.fb.vinfo.yres/(map->width);
        if(fb_width)      *fb_width  = neobox.fb.vinfo.xres/(map->height);
        if(screen_height) *screen_height = SCREENMAX/(map->width);
        if(screen_width)  *screen_width  = SCREENMAX/(map->height);
    }
    else
    {
        if(height)        *height = neobox.fb.vinfo.yres/(map->height);
        if(width)         *width  = neobox.fb.vinfo.xres/(map->width);
        if(fb_height)     *fb_height = neobox.fb.vinfo.yres/(map->height);
        if(fb_width)      *fb_width  = neobox.fb.vinfo.xres/(map->width);
        if(screen_height) *screen_height = SCREENMAX/(map->height);
        if(screen_width)  *screen_width  = SCREENMAX/(map->width);
    }
}

void neobox_layout_to_fb_sizes(int *height, int *width)
{
    int tmp;
    
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        tmp = *height;
        *height = *width;
        *width = tmp;
    }
}

void neobox_fb_to_layout_sizes(int *height, int *width)
{
    int tmp;
    
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        tmp = *height;
        *height = *width;
        *width = tmp;
    }
}

void neobox_layout_to_fb_cords(int *cord_y, int *cord_x, const struct neobox_map *map)
{
    int tmp;
    
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        tmp = *cord_y;
        *cord_y  = *cord_x;
        *cord_x  = map->height-tmp-1;
    }
}

void neobox_fb_to_layout_cords(int *cord_y, int *cord_x, const struct neobox_map *map)
{
    int tmp;
    
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        tmp = *cord_y;
        *cord_y  = map->height-*cord_x-1;
        *cord_x  = tmp;
    }
}

void neobox_layout_to_fb_pos_width(int *pos_y, int *pos_x, int width)
{
    int tmp;
    
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        tmp = *pos_y;
        *pos_y = *pos_x;
        *pos_x = width - tmp;
    }
}

void neobox_fb_to_layout_pos_width(int *pos_y, int *pos_x, int width)
{
    int tmp;
    
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        tmp = *pos_y;
        *pos_y = width - *pos_x;
        *pos_x = tmp;
    }
}

void neobox_layout_to_fb_pos(int *pos_y, int *pos_x)
{
    neobox_layout_to_fb_pos_width(pos_y, pos_x, neobox.fb.vinfo.xres);
}

void neobox_fb_to_layout_pos(int *pos_y, int *pos_x)
{
    neobox_fb_to_layout_pos_width(pos_y, pos_x, neobox.fb.vinfo.xres);
}

void neobox_layout_to_fb_pos_rel(int *pos_y, int *pos_x, int height)
{
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        neobox_layout_to_fb_pos(pos_y, pos_x);
        *pos_x -= height;
    }
}

void neobox_fb_to_layout_pos_rel(int *pos_y, int *pos_x, int width)
{
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        neobox_fb_to_layout_pos(pos_y, pos_x);
        *pos_y -= width;
    }
}

int neobox_string_size(int width)
{
    return width/(ISO_CHAR_WIDTH*FONTMULT)-2;
}

unsigned char neobox_layout_connect_to_borders(int cord_y, int cord_x, unsigned char connect, const struct neobox_map *map)
{
    neobox_layout_to_fb_cords(&cord_y, &cord_x, map);
    return neobox_fb_connect_to_borders(cord_y, cord_x, connect, map);
}

unsigned char neobox_fb_connect_to_borders(int cord_y, int cord_x, unsigned char connect, const struct neobox_map *map)
{
    unsigned char borders = NEOBOX_BORDER_ALL;
    
    if(neobox.format == NEOBOX_FORMAT_LANDSCAPE)
    {
        if(connect & NEOBOX_LAYOUT_OPTION_CONNECT_LEFT)
            borders &= ~NEOBOX_BORDER_TOP;
        if(connect & NEOBOX_LAYOUT_OPTION_CONNECT_UP)
            borders &= ~NEOBOX_BORDER_RIGHT;
        if(cord_y < map->width-1 &&
            map->map[(map->height-cord_x-1)*map->width+cord_y+1]
            .options & NEOBOX_LAYOUT_OPTION_CONNECT_LEFT)
        {
            borders &= ~NEOBOX_BORDER_BOTTOM;
        }
        if(cord_x > 0 &&
            map->map[(map->height-cord_x)*map->width+cord_y]
            .options & NEOBOX_LAYOUT_OPTION_CONNECT_UP)
        {
            borders &= ~NEOBOX_BORDER_LEFT;
        }
    }
    else
    {
        if(connect & NEOBOX_LAYOUT_OPTION_CONNECT_LEFT)
            borders &= ~NEOBOX_BORDER_LEFT;
        if(connect & NEOBOX_LAYOUT_OPTION_CONNECT_UP)
            borders &= ~NEOBOX_BORDER_TOP;
        if(cord_x < map->width-1 &&
            map->map[cord_y*map->width+cord_x+1]
            .options & NEOBOX_LAYOUT_OPTION_CONNECT_LEFT)
        {
            borders &= ~NEOBOX_BORDER_RIGHT;
        }
        if(cord_y < map->height-1 &&
            map->map[(cord_y+1)*map->width+cord_x]
            .options & NEOBOX_LAYOUT_OPTION_CONNECT_UP)
        {
            borders &= ~NEOBOX_BORDER_BOTTOM;
        }
    }
    
    return borders;
}

void neobox_layout_draw_rect(int pos_y, int pos_x, int height, int width, int color, int density, const struct neobox_map *map, unsigned char **copy)
{
    neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
    neobox_layout_to_fb_sizes(&height, &width);
    neobox_draw_rect(pos_y, pos_x, height, width, neobox_color(0, color, map), density, copy);
}

void neobox_draw_rect(int pos_y, int pos_x, int height, int width, unsigned char color[4], int density, unsigned char **copy)
{
    unsigned char *base = neobox.fb.ptr + pos_y*neobox.fb.finfo.line_length + pos_x*neobox.fb.bpp;
    unsigned char *ptr = base, *cpy = copy ? *copy : 0;
    int i, j, k;
    
    for(i=0; i<height; i+=density, ptr = base)
    {
        for(j=0; j<width; j+=density, ptr+=neobox.fb.bpp*(density-1))
        {
            for(k=neobox.fb.bpp-1; k>=0; k--)
            {
                if(cpy)
                    *(cpy++) = *ptr;
                *(ptr++) = color[k];
            }
        }
        base += density*neobox.fb.finfo.line_length;
    }
    
    if(cpy)
        *copy = cpy;
}

void neobox_layout_draw_border(int pos_y, int pos_x, int height, int width, int color, unsigned char borders, int density, const struct neobox_map *map, unsigned char **copy)
{
    neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
    neobox_layout_to_fb_sizes(&height, &width);
    neobox_draw_border(pos_y, pos_x, height, width, neobox_color(0, color, map), borders, density, copy);
}

void neobox_layout_draw_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, int color, unsigned char connect, int density, const struct neobox_map *map, unsigned char **copy)
{
    unsigned char borders;
    
    neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
    neobox_layout_to_fb_sizes(&height, &width);
    borders = neobox_layout_connect_to_borders(cord_y, cord_x, connect, map);
    neobox_draw_border(pos_y, pos_x, height, width, neobox_color(0, color, map), borders, density, copy);
}

void neobox_draw_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char color[4], unsigned char connect, int density, const struct neobox_map *map, unsigned char **copy)
{
    unsigned char borders;
    
    borders = neobox_fb_connect_to_borders(cord_y, cord_x, connect, map);
    neobox_draw_border(pos_y, pos_x, height, width, color, borders, density, copy);
}

void neobox_draw_border(int pos_y, int pos_x, int height, int width, unsigned char color[4], unsigned char borders, int density, unsigned char **copy)
{
    unsigned char *base = neobox.fb.ptr + pos_y*neobox.fb.finfo.line_length + pos_x*neobox.fb.bpp;
    unsigned char *ptr = base, *base2 = base, *cpy = copy ? *copy : 0;
    int i, j, k, skipped;
    
    for(i=0, skipped=0; i<(borders&NEOBOX_BORDER_BOTTOM?2:1); i++, ptr = base2)
    {
        if(!skipped && !(borders & NEOBOX_BORDER_TOP))
            skipped = 1;
        else
            for(j=0; j<width; j+=density, ptr+=neobox.fb.bpp*(density-1))
            {
                for(k=neobox.fb.bpp-1; k>=0; k--)
                {
                    if(cpy)
                        *(cpy++) = *ptr;
                    *(ptr++) = color[k];
                }
            }
        base2 += (height-1)*neobox.fb.finfo.line_length;
    }
    base2 = base;
    ptr   = base;
    for(i=0; i<height; i++, ptr = base2)
    {
        for(j=0, skipped=0; j<(borders&NEOBOX_BORDER_RIGHT?2:1); j++, ptr+=neobox.fb.bpp*(width-2))
        {
            if(!skipped && !(borders & NEOBOX_BORDER_LEFT))
            {
                skipped = 1;
                continue;
            }
            for(k=neobox.fb.bpp-1; k>=0; k--)
            {
                if(cpy)
                    *(cpy++) = *ptr;
                *(ptr++) = color[k];
            }
        }
        base2 += density*neobox.fb.finfo.line_length;
    }
    
    if(cpy)
        *copy = cpy;
}

void neobox_layout_draw_rect_border(int pos_y, int pos_x, int height, int width, int color_fg, int color_bg, unsigned char borders, int density, const struct neobox_map *map, unsigned char **copy)
{
    unsigned char fg[4];
    neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
    neobox_layout_to_fb_sizes(&height, &width);
    neobox_draw_rect_border(pos_y, pos_x, height, width, neobox_color(fg, color_fg, map), neobox_color(0, color_bg, map), borders, density, copy);
}

void neobox_layout_draw_rect_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, int color_fg, int color_bg, unsigned char connect, int density, const struct neobox_map *map, unsigned char **copy)
{
    unsigned char borders, fg[4];
    
    neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
    neobox_layout_to_fb_sizes(&height, &width);
    borders = neobox_layout_connect_to_borders(cord_y, cord_x, connect, map);
    neobox_draw_rect_border(pos_y, pos_x, height, width, neobox_color(fg, color_fg, map), neobox_color(0, color_bg, map), borders, density, copy);
}

void neobox_draw_rect_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char color_fg[4], unsigned char color_bg[4], unsigned char connect, int density, const struct neobox_map *map, unsigned char **copy)
{
    unsigned char borders;
    
    borders = neobox_fb_connect_to_borders(cord_y, cord_x, connect, map);
    neobox_draw_rect_border(pos_y, pos_x, height, width, color_fg, color_bg, borders, density, copy);
}

void neobox_draw_rect_border(int pos_y, int pos_x, int height, int width, unsigned char color_fg[4], unsigned char color_bg[4], unsigned char borders, int density, unsigned char **copy)
{
    unsigned char *base = neobox.fb.ptr + pos_y*neobox.fb.finfo.line_length + pos_x*neobox.fb.bpp;
    unsigned char *ptr = base, *cpy = copy ? *copy : 0;
    unsigned char *col = color_bg;
    int i, j, k, cset;
    
    for(i=0; i<height; i+=density, ptr = base, col = color_bg)
    {
        if(!i && borders & NEOBOX_BORDER_TOP)
            col = color_fg;
        if(i == height-1 && borders & NEOBOX_BORDER_BOTTOM)
            col = color_fg;
        
        for(j=0; j<width; j+=density, ptr+=neobox.fb.bpp*(density-1))
        {
            if(col == color_fg)
                cset = 1;
            else
            {
                cset = 0;
                if(!j && borders & NEOBOX_BORDER_LEFT)
                    col = color_fg;
                if(j == width-1 && borders & NEOBOX_BORDER_RIGHT)
                    col = color_fg;
            }
            
            for(k=neobox.fb.bpp-1; k>=0; k--)
            {
                if(cpy)
                    *(cpy++) = *ptr;
                *(ptr++) = col[k];
            }
            
            if(!cset)
                col = color_bg;
        }
        base += density*neobox.fb.finfo.line_length;
    }
    
    if(cpy)
        *copy = cpy;
}

void neobox_layout_fill_rect(int pos_y, int pos_x, int height, int width, int density, unsigned char **fill)
{
    neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
    neobox_layout_to_fb_sizes(&height, &width);
    neobox_fill_rect(pos_y, pos_x, height, width, density, fill);
}

void neobox_fill_rect(int pos_y, int pos_x, int height, int width, int density, unsigned char **fill)
{
    unsigned char *base = neobox.fb.ptr + pos_y*neobox.fb.finfo.line_length + pos_x*neobox.fb.bpp;
    unsigned char *ptr = base, *fll = *fill;
    int i, j, k;
    
    for(i=0; i<height; i+=density, ptr = base)
    {
        for(j=0; j<width; j+=density, ptr+=neobox.fb.bpp*(density-1))
        {
            for(k=neobox.fb.bpp-1; k>=0; k--)
            {
                *(ptr++) = *fll;
                fll++;
            }
        }
        base += density*neobox.fb.finfo.line_length;
    }
    
    *fill = fll;
}

void neobox_layout_fill_border(int pos_y, int pos_x, int height, int width, unsigned char borders, int density, unsigned char **fill)
{
    neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
    neobox_layout_to_fb_sizes(&height, &width);
    neobox_fill_border(pos_y, pos_x, height, width, borders, density, fill);
}

void neobox_layout_fill_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char connect, int density, const struct neobox_map *map, unsigned char **fill)
{
    unsigned char borders;
    
    neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
    neobox_layout_to_fb_sizes(&height, &width);
    borders = neobox_layout_connect_to_borders(cord_y, cord_x, connect, map);
    neobox_fill_border(pos_y, pos_x, height, width, borders, density, fill);
}

void neobox_fill_connect(int pos_y, int pos_x, int cord_y, int cord_x, int height, int width, unsigned char connect, int density, const struct neobox_map *map, unsigned char **fill)
{
    unsigned char borders;
    
    borders = neobox_fb_connect_to_borders(cord_y, cord_x, connect, map);
    neobox_fill_border(pos_y, pos_x, height, width, borders, density, fill);
}

void neobox_fill_border(int pos_y, int pos_x, int height, int width, unsigned char borders, int density, unsigned char **fill)
{
    unsigned char *base = neobox.fb.ptr + pos_y*neobox.fb.finfo.line_length + pos_x*neobox.fb.bpp;
    unsigned char *ptr = base, *base2 = base, *fll = *fill;
    int i, j, k, skipped;
    
    for(i=0, skipped=0; i<(borders&NEOBOX_BORDER_BOTTOM?2:1); i++, ptr = base2)
    {
        if(!skipped && !(borders & NEOBOX_BORDER_TOP))
            skipped = 1;
        else
            for(j=0; j<width; j+=density, ptr+=neobox.fb.bpp*(density-1))
            {
                for(k=neobox.fb.bpp-1; k>=0; k--)
                {
                    *(ptr++) = *fll;
                    fll++;
                }
            }
        base2 += (height-1)*neobox.fb.finfo.line_length;
    }
    base2 = base;
    ptr   = base;
    for(i=0; i<height; i++, ptr = base2)
    {
        for(j=0, skipped=0; j<(borders&NEOBOX_BORDER_RIGHT?0:1); j++, ptr+=neobox.fb.bpp*(width-2))
        {
            if(!skipped && !(borders & NEOBOX_BORDER_LEFT))
            {
                skipped = 1;
                continue;
            }
            for(k=neobox.fb.bpp-1; k>=0; k--)
            {
                *(ptr++) = *fll;
                fll++;
            }
        }
        base2 += density*neobox.fb.finfo.line_length;
    }
    
    *fill = fll;
}

static unsigned char* align_base(int align, int height, int width, int text_height, int text_width, int char_height, int char_width, int offset, unsigned char *base)
{
    switch(align)
    {
    case NEOBOX_LAYOUT_OPTION_ALIGN_CENTER:
    case NEOBOX_LAYOUT_OPTION_ALIGN_LEFT:
    case NEOBOX_LAYOUT_OPTION_ALIGN_RIGHT:
        base += (height/2-text_height/2)*neobox.fb.finfo.line_length;
        break;
    case NEOBOX_LAYOUT_OPTION_ALIGN_TOP:
        base += offset*neobox.fb.finfo.line_length;
        break;
    case NEOBOX_LAYOUT_OPTION_ALIGN_BOTTOM:
        base += (height-text_height-offset)*neobox.fb.finfo.line_length;
        break;
    }
    switch(align)
    {
    case NEOBOX_LAYOUT_OPTION_ALIGN_CENTER:
    case NEOBOX_LAYOUT_OPTION_ALIGN_TOP:
    case NEOBOX_LAYOUT_OPTION_ALIGN_BOTTOM:
        base += (width/2-text_width/2)*neobox.fb.bpp;
        break;
    case NEOBOX_LAYOUT_OPTION_ALIGN_LEFT:
        base += offset*neobox.fb.bpp;
        break;
    case NEOBOX_LAYOUT_OPTION_ALIGN_RIGHT:
        base += (width-text_width-offset)*neobox.fb.bpp;
        break;
    }
    
    return base;
}

static void draw_string_horz(char c, unsigned char **base, unsigned char color_fg[4], unsigned char color_bg[4], int bg)
{
    int w, h, k, pos = c*ISO_CHAR_HEIGHT;
    unsigned char *ptr = *base, *base2 = *base;
    
    for(h=0; h<ISO_CHAR_HEIGHT*FONTMULT; h++)
    {
        for(w=0; w<ISO_CHAR_WIDTH*FONTMULT; w++)
        {
            if(iso_font[pos+(h/FONTMULT)] & (1<<(w/FONTMULT)))
                for(k=neobox.fb.bpp-1; k>=0; k--)
                    *(ptr++) = color_fg[k];
            else if(bg)
                for(k=neobox.fb.bpp-1; k>=0; k--)
                    *(ptr++) = color_bg[k];
            else
                ptr += neobox.fb.bpp;
        }
        ptr = base2 += neobox.fb.finfo.line_length;
    }
    
    *base += ISO_CHAR_WIDTH*FONTMULT*neobox.fb.bpp;
}

static void draw_string_vert(char c, unsigned char **base, unsigned char color_fg[4], unsigned char color_bg[4], int bg)
{
    int w, h, k, pos = c*ISO_CHAR_HEIGHT;
    unsigned char *ptr = *base;
    
    for(w=0; w<ISO_CHAR_WIDTH*FONTMULT; w++)
    {
        for(h=ISO_CHAR_HEIGHT*FONTMULT-1; h>=0; h--)
        {
            if(iso_font[pos+(h/FONTMULT)] & (1<<(w/FONTMULT)))
                for(k=neobox.fb.bpp-1; k>=0; k--)
                    *(ptr++) = color_fg[k];
            else if(bg)
                for(k=neobox.fb.bpp-1; k>=0; k--)
                    *(ptr++) = color_bg[k];
            else
                ptr += neobox.fb.bpp;
        }
        ptr = *base += neobox.fb.finfo.line_length;
    }
}

static struct text_control prepare_control(const char *str, int max)
{
    struct neobox_text_control *control = (void*)str;
    struct text_control ctrl;
    
    if(control->soh != 1)
    {
        ctrl.control = 0;
        ctrl.text = str;
        ctrl.len = strlen(str);
        return ctrl;
    }
    
    ctrl.control = 1;
    ctrl.left = control->left?1:0;
    ctrl.cursor = control->cursor-ctrl.left;
    ctrl.text = control->text+ctrl.left;
    ctrl.len = strlen(ctrl.text)+ctrl.left;
    
    if(ctrl.len > max)
    {
        ctrl.right = 1;
        ctrl.len = max;
    }
    else
        ctrl.right = 0;
    
    if(ctrl.len == ctrl.cursor+ctrl.left)
        ctrl.len++;
    
    return ctrl;
}

static void draw_string(unsigned char *base, int height, int width, unsigned char color[3][4], struct text_control *control, draw_string_func draw)
{
    unsigned char *color_text_fg = color[2];
    unsigned char *color_text_bg = color[1];
    unsigned char *color_cursor_fg = color[1];
    unsigned char *color_cursor_bg = color[2];
//    unsigned char *color_mark_fg = color[0];
//    unsigned char *color_mark_bg = color[2];
    unsigned char *color_tx_fg = color[0];
    unsigned char *color_tx_bg = color[1];
    
    if(!control->control)
    {
        for(; *control->text; control->text++)
            draw(*control->text, &base, color_text_fg, color_text_bg, 0);
        return;
    }
    
    if(control->left)
    {
        draw('<', &base, color_tx_fg, color_tx_bg, 1);
        control->len--;
    }
    
    for(; control->len > control->right && control->cursor; control->text++, control->len--, control->cursor--)
        draw(*control->text, &base, color_text_fg, color_text_bg, 0);
    
    if(!control->cursor)
    {
        if(!*control->text)
        {
            draw(' ', &base, color_cursor_fg, color_cursor_bg, 1);
            return;
        }
        
        draw(*control->text, &base, color_cursor_fg, color_cursor_bg, 1);
        control->text++;
        control->len--;
    }
    
    for(; control->len > control->right; control->text++, control->len--)
        draw(*control->text, &base, color_text_fg, color_text_bg, 0);
    
    if(control->right)
        draw('>', &base, color_tx_fg, color_tx_bg, 1);
}

void neobox_layout_draw_string(int pos_y, int pos_x, int height, int width, int color_fg, int color_bg, int color_txt, int align, const char *str, const struct neobox_map *map)
{
    unsigned char color[3][4];
    
    neobox_color(color[0], color_fg, map);
    neobox_color(color[1], color_bg, map);
    neobox_color(color[2], color_txt, map);
    
    if(neobox.format == NEOBOX_FORMAT_PORTRAIT)
        neobox_draw_string_horz(pos_y, pos_x, height, width, color, align, str);
    else
    {
        neobox_layout_to_fb_pos_rel(&pos_y, &pos_x, height);
        neobox_layout_to_fb_sizes(&height, &width);
        if(align)
        {
            if(align == NEOBOX_LAYOUT_OPTION_ALIGN_TOP)
                align = NEOBOX_LAYOUT_OPTION_ALIGN_RIGHT;
            else
                align--;
        }
        neobox_draw_string_vert(pos_y, pos_x, height, width, color, align, str);
    }
}

void neobox_draw_string_horz(int pos_y, int pos_x, int height, int width, unsigned char color[3][4], int align, const char *str)
{
    unsigned char *base = neobox.fb.ptr + pos_y*neobox.fb.finfo.line_length + pos_x*neobox.fb.bpp;
    struct text_control control = prepare_control(str, width/(ISO_CHAR_WIDTH*FONTMULT)-2);
    
    base = align_base(align, height, width,
        ISO_CHAR_HEIGHT*FONTMULT, control.len*ISO_CHAR_WIDTH*FONTMULT,
        ISO_CHAR_HEIGHT*FONTMULT, ISO_CHAR_WIDTH*FONTMULT,
        ISO_CHAR_WIDTH*FONTMULT, base);
    
    draw_string(base, height, width, color, &control, draw_string_horz);
}

void neobox_draw_string_vert(int pos_y, int pos_x, int height, int width, unsigned char color[3][4], int align, const char *str)
{
    unsigned char *base = neobox.fb.ptr + pos_y*neobox.fb.finfo.line_length + pos_x*neobox.fb.bpp;
    struct text_control control = prepare_control(str, height/(ISO_CHAR_WIDTH*FONTMULT)-2);
    
    base = align_base(align, height, width,
        control.len*ISO_CHAR_WIDTH*FONTMULT, ISO_CHAR_HEIGHT*FONTMULT,
        ISO_CHAR_WIDTH*FONTMULT, ISO_CHAR_HEIGHT*FONTMULT,
        ISO_CHAR_WIDTH*FONTMULT, base);
    
    draw_string(base, height, width, color, &control, draw_string_vert);
}
