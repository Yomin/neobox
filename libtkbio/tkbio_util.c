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

#include <ctype.h>
#include <stdlib.h>

char** tkbio_util_parse_cmd(char *cmd)
{
    char **argv, *ptr = cmd;
    int i = 0, count = 0, offset, start, escape;
    
    while(*ptr)
    {
        if(isspace(*ptr))
        {
            start = ptr == cmd;
            *(ptr++) = 0;
            while(isspace(*ptr))
                *(ptr++) = 0;
            if(start)
                cmd = ptr;
            else if(*ptr)
                count++;
        }
        else if(*ptr == '\\')
        {
            offset = 0;
            escape = 0;
            while(ptr[offset] && (escape || !isspace(ptr[offset])))
            {
                if(!escape && ptr[offset] == '\\' && !ptr[++offset])
                    break;
                if(ptr[offset] == '"')
                {
                    escape = !escape;
                    offset++;
                    continue;
                }
                *ptr = ptr[offset];
                ptr++;
            }
            while(offset--)
                *(ptr++) = 0;
        }
        else if(*ptr == '"')
        {
            while(ptr[1] && ptr[1] != '"')
            {
                *ptr = ptr[1];
                ptr++;
            }
            *(ptr++) = 0;
            *(ptr++) = 0;
            if(ptr-cmd == 2)
                cmd = ptr;
        }
        else
            ptr++;
    }
    
    argv = malloc((count+2)*sizeof(char*));
    argv[i++] = cmd;
    
    while(++cmd != ptr)
        if(!*cmd)
        {
            cmd++;
            while(!*cmd && cmd != ptr)
                cmd++;
            if(cmd == ptr)
                break;
            else
                argv[i++] = cmd;
        }
    
    argv[i] = 0;
    
    return argv;
}
