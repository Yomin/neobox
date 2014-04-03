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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "tkbio.h"
#include "tkbio_nop.h"
#include "tkbio_button.h"
#include "2048_layout.h"

#define VECTOR_UP    0
#define VECTOR_DOWN  1
#define VECTOR_LEFT  2
#define VECTOR_RIGHT 3

#define MODE_2048       0
#define MODE_NUMBERWANG 1
#define MODE_COUNT      2

struct tile
{
    int num;
    int flag;
    char name[10];
};

struct tile board[4*4] = {{0}}, *vector[4][4*4];
int last_y, last_x, used = 0, action = 1;
int check = 0, mode = 0, rotating = 0, visible = 0;

static inline void draw(int y, int x, int id)
{
    int nop, idy = y, idx = x;
    
    if(id != -1)
    {
        idy = id/4;
        idx = id%4;
    }
    else
        id = y*4+x;
    
    switch(idy)
    {
    case 0: case 3: nop = !idx || idx==3; break;
    case 1: case 2: nop = idx==1 || idx==2; break;
    }
    
    if(nop)
        tkbio_nop_set_name(id, 0, board[y*4+x].name, visible);
    else
        tkbio_button_set_name(id, 0, board[y*4+x].name, visible);
}

static inline void numberwang(int vec, int pos)
{
    char op[4] = {'+', '-', '*', '/'};
    
    switch(random()%8)
    {
    case 0: case 1: case 2:
        snprintf(vector[vec][pos]->name, 10, "%i",
            (random()%5?1:-1) * (int)random()%20);
        break;
    case 3: case 4:
        snprintf(vector[vec][pos]->name, 10, "%i",
            (random()%5?1:-1) * (int)random()%100);
        break;
    case 5:
        snprintf(vector[vec][pos]->name, 10, "%i",
            (random()%5?1:-1) * (int)random()%1000);
        break;
    case 6:
        snprintf(vector[vec][pos]->name, 10, "%i.%i",
            (random()%5?1:-1) * (int)random()%100, (int)random()%100);
        break;
    case 7:
        snprintf(vector[vec][pos]->name, 10, "%i%c%i",
            (random()%2?-1:1)*(int)random()%100,
            op[random()%4], (int)random()%100);
        break;
    }

}

void add()
{
    int value;
    
    while(board[(last_y = random()%4)*4+(last_x = random()%4)].num);
    
    value = board[last_y*4+last_x].num = random()%100 < 90 ? 2 : 4;
    
    switch(mode)
    {
    case MODE_2048:
        snprintf(board[last_y*4+last_x].name, 10, "(%i)", value);
        break;
    case MODE_NUMBERWANG:
        numberwang(VECTOR_UP, last_y*4+last_x);
        break;
    }
    
    used++;
    
    draw(last_y, last_x, -1);
}

static inline void merge(int vec, int pos1, int pos2)
{
    switch(mode)
    {
    case MODE_2048:
        vector[vec][pos1]->num *= 2;
        snprintf(vector[vec][pos1]->name, 10, "%i", vector[vec][pos1]->num);
        if(vector[vec][pos1]->num == 2048)
            tkbio_button_set_name(42, 0, "!! 2048 !!", visible);
        break;
    case MODE_NUMBERWANG:
        tkbio_button_set_name(42, 0, "That's Numberwang!", visible);
        vector[vec][pos1]->num *= 2;
        numberwang(vec, pos1);
        if(vector[vec][pos1]->num == 2048)
            tkbio_button_set_name(42, 0, "You're the Numberwang!", visible);
        break;
    }
    vector[vec][pos1]->flag = vector[vec][pos2]->flag = 1;
    vector[vec][pos2]->num = 0;
    vector[vec][pos2]->name[0] = 0;
    used--;
}

static inline void move(int vec, int pos1, int pos2)
{
    switch(mode)
    {
    case MODE_2048:
        strncpy(vector[vec][pos1]->name, vector[vec][pos2]->name, 10);
        break;
    case MODE_NUMBERWANG:
        numberwang(vec, pos1);
        break;
    }
    vector[vec][pos1]->num = vector[vec][pos2]->num;
    vector[vec][pos1]->flag = vector[vec][pos2]->flag = 1;
    vector[vec][pos2]->num = 0;
    vector[vec][pos2]->name[0] = 0;
}

static inline int redraw()
{
    int y, x, tmp[4], change = 0;
    
    action = 0;
    
    for(y=0; y<4; y++)
        for(x=0; x<4; x++)
        {
            if(board[y*4+x].flag)
            {
                board[y*4+x].flag = 0;
                draw(y, x, -1);
                change = 1;
            }
            if( (y && board[y*4+x].num == tmp[x]) ||
                (x && board[y*4+x].num == board[y*4+x-1].num))
            {
                action = 1;
            }
            tmp[x] = board[y*4+x].num;
        }
    
    return change;
}

void reset()
{
    int y, x;
    
    for(y=0; y<4; y++)
        for(x=0; x<4; x++)
        {
            board[y*4+x].num = 0;
            board[y*4+x].name[0] = 0;
            draw(y, x, -1);
        }
    
    used = 0;
    action = 1;
    
    add();
}

int shift(int vec)
{
    int y, x, z;
    
    for(x=0; x<4; x++)
    {
        for(y=0,z=0; y<4; y++,z++)
            if(!vector[vec][y*4+x]->num)
                z--;
            else if(z < y)
                move(vec, z*4+x, y*4+x);
        
        for(y=0; y<3 && vector[vec][(y+1)*4+x]->num; y++)
            if(vector[vec][y*4+x]->num == vector[vec][(y+1)*4+x]->num)
            {
                merge(vec, y*4+x, (y+1)*4+x);
                for(z=y+1; z<3 && vector[vec][(z+1)*4+x]->num; z++)
                    move(vec, z*4+x, (z+1)*4+x);
            }
    }
    
    return redraw();
}

void rotate(int pos)
{
    int i;
    int map_outer[16] = {0,1,2,3,7,11,15,14,13,12,8,4};
    int map_inner[4] = {5,6,10,9};
    
    switch(pos)
    {
    case 0:
        tkbio_button_set_name(42, 0, "Let's rotate the board!", visible);
        tkbio_timer(1, 0, 500000);
        rotating = 1;
        break;
    case 13:
        tkbio_button_set_name(42, 0, "Let's play Numberwang!", visible);
        tkbio_timer(0, 5*60, 0);
        rotating = 0;
        break;
    default:
        for(i=0; i<12; i++)
            draw(map_outer[i]/4, map_outer[i]%4, map_outer[(i+pos)%12]);
        for(i=0; i<4; i++)
            draw(map_inner[i]/4, map_inner[i]%4, map_inner[(i+pos)%4]);
        tkbio_timer(pos+1, 0, 500000);
    }
}

int handler(struct tkbio_return ret, void *state)
{
    int change;
    
    switch(ret.type)
    {
    case TKBIO_RETURN_CHAR:
        if(rotating)
            return TKBIO_HANDLER_SUCCESS;
        
        tkbio_button_set_name(42, 0, 0, visible);
        snprintf(board[last_y*4+last_x].name, 10,
            "%i", board[last_y*4+last_x].num);
        draw(last_y, last_x, -1);
        
        switch(ret.id)
        {
        case 1: case 2: change = shift(VECTOR_UP); break;
        case 4: case 8: change = shift(VECTOR_LEFT); break;
        case 7: case 11: change = shift(VECTOR_RIGHT); break;
        case 13: case 14: change = shift(VECTOR_DOWN); break;
        case 42: // head button
            if(check && used > 1)
            {
                check = 0;
                tkbio_button_set_name(42, 0, "Really?", visible);
            }
            else
            {
                reset();
                tkbio_button_set_name(42, 0, 0, visible);
            }
            return TKBIO_HANDLER_SUCCESS;
        case 23: // mode button
            if(check && used > 1)
            {
                check = 0;
                tkbio_button_set_name(42, 0, "Really?", visible);
            }
            else
            {
                mode = (mode+1)%MODE_COUNT;
                switch(mode)
                {
                case MODE_2048:
                    tkbio_button_set_name(42, 0, "2048", visible);
                    break;
                case MODE_NUMBERWANG:
                    tkbio_button_set_name(42, 0, "Numberwang", visible);
                    tkbio_timer(0, 5*60, 0);
                    break;
                }
                reset();
            }
            return TKBIO_HANDLER_SUCCESS;
        }
        
        check = 1;
        
        if(change)
            add();
        else if(!action && used == 16)
            switch(mode)
            {
            case MODE_2048:
                tkbio_button_set_name(42, 0, "Game Over", visible);
                break;
            case MODE_NUMBERWANG:
                tkbio_button_set_name(42, 0, "You've been Wangernumbed!", visible);
                break;
            }
        return TKBIO_HANDLER_SUCCESS;
    case TKBIO_RETURN_TIMER:
        rotate(ret.id);
        return TKBIO_HANDLER_SUCCESS;
    case TKBIO_RETURN_ACTIVATE:
        visible = 1;
        return TKBIO_HANDLER_DEFER;
    case TKBIO_RETURN_DEACTIVATE:
        visible = 0;
        return TKBIO_HANDLER_DEFER;
    default:
        return TKBIO_HANDLER_DEFER;
    }
}

int main(int argc, char* argv[])
{
    struct tkbio_config config = tkbio_config_default(&argc, argv);
    int ret, y, x;
    
    config.format = TKBIO_FORMAT_PORTRAIT;
    config.layout = app2048Layout;
    
    if((ret = tkbio_init_custom(config)) < 0)
        return ret;
    
    srandom(time(0));
    
    for(y=0; y<4; y++)
        for(x=0; x<4; x++)
        {
            vector[VECTOR_UP][y*4+x] = &board[y*4+x];
            vector[VECTOR_DOWN][y*4+x] = &board[(3-y)*4+x];
            vector[VECTOR_LEFT][y*4+x] = &board[(3-x)*4+y];
            vector[VECTOR_RIGHT][y*4+x] = &board[x*4+3-y];
        }
    
    add();
    
    tkbio_button_set_name(42, 0, "2048", visible);
    
    ret = tkbio_run(handler, 0);
    
    tkbio_finish();
    
    return 0;
}

