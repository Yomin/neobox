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

#include <neobox.h>
#include <neobox_nop.h>
#include <neobox_text.h>
#include <neobox_button.h>
#include <neobox_config.h>

#include <string.h>
#include <stdint.h>
#include <openssl/hmac.h>

#include "totp_layout.h"

#define DEFAULT_HASH    "sha1"
#define DEFAULT_DIGITS  "6"
#define DEFAULT_STEP    "30"
#define DEFAULT_OFFSET  "0"

#define B32(X) ((X)<'A' ? (X)-'2'+'Z'-'A'+1 : \
                (X)>'Z' ? (X)-'a' : \
                (X)-'A')

struct totp
{
    char *name, *key, totp[20];
    int keylen, digits, step, t0, valid;
    const EVP_MD *hash;
} totp;


int base32_check(char *str)
{
    if(!str)
        return 0;
    
    for(; *str; str++)
    {
        if((*str < '2' || *str > '7') &&
           (*str < 'A' || *str > 'Z') &&
           (*str < 'a' || *str > 'z'))
        {
            for(; *str == '='; str++);
            return !*str;
        }
    }
    
    return 1;
}

char* base32_decode(char *src, int *len)
{
    char *dst;
    int d, s;
    
    if(!src)
        return 0;
    
    *len = strlen(src)*5/8;
    dst = malloc(*len);
    
    for(d=0,s=0; d<*len; d+=5,s+=8)
    {
        dst[d+0] = B32(src[s+0]) << 3 | B32(src[s+1]) >> 2;
        if(d+1 == *len) break;
        dst[d+1] = B32(src[s+1]) << 6 | B32(src[s+2]) << 1 | B32(src[s+3]) >> 4;
        if(d+2 == *len) break;
        dst[d+2] = B32(src[s+3]) << 4 | B32(src[s+4]) >> 1;
        if(d+3 == *len) break;
        dst[d+3] = B32(src[s+4]) << 7 | B32(src[s+5]) << 2 | B32(src[s+6]) >> 3;
        if(d+4 == *len) break;
        dst[d+4] = B32(src[s+6]) << 5 | B32(src[s+7]);
    }
    
    return dst;
}

unsigned long int power(int base, int exp)
{
    unsigned long int result = base;
    
    while(--exp>0)
        result *= base;
    
    return result;
}

char* generate_totp()
{
    unsigned char data[sizeof(uint64_t)], *md;
    unsigned int dlen, offset, i;
    time_t unixtime;
    uint64_t steps;
    uint32_t token;
    
    time(&unixtime);
    
    steps = (unixtime-totp.t0)/totp.step;
    totp.valid = totp.step - (unixtime-totp.t0)%totp.step;
    
    for(i=0; i<sizeof(uint64_t); i++)
        data[i] = (steps >> ((sizeof(uint64_t)-i-1)*8)) & 0xFF;
    
    md = HMAC(totp.hash, totp.key, totp.keylen,
        data, sizeof(uint64_t), 0, &dlen);
    
    offset = md[dlen-1] & 0xF;
    token = (md[offset+0] & 0x7F) << 24
          | (md[offset+1] & 0xFF) << 16
          | (md[offset+2] & 0xFF) <<  8
          | (md[offset+3] & 0xFF) <<  0;
    
    sprintf(totp.totp, "%0*li",
        totp.digits, token % power(10, totp.digits));
    
    return totp.totp;
}

int set_value(const char *name, char **value, char *def)
{
    if(strspn(*value, "1234567890") != strlen(*value))
    {
        printf("malformed %s, default to %s\n", name, def);
        *value = def;
        return atoi(def);
    }
    else
        return atoi(*value);
}

const EVP_MD* set_hash(char **hash, char *def)
{
    while(1)
    {
        if(!strcmp("sha1", *hash))
            return EVP_sha1();
        else if(!strcmp("sha256", *hash))
            return EVP_sha256();
        else if(!strcmp("sha512", *hash))
            return EVP_sha512();
        else
        {
            printf("unknown hash algorithm, default to %s\n", def);
            *hash = def;
        }
    }
}

void load_config()
{
    char *key = 0, *hash = 0;
    char *digits = 0, *step = 0, *offset = 0;
    
    totp.name = neobox_config("name", 0);
    key = neobox_config("key", 0);
    hash = neobox_config("hash", DEFAULT_HASH);
    digits = neobox_config("digits", DEFAULT_DIGITS);
    step = neobox_config("step", DEFAULT_STEP);
    offset = neobox_config("offset", DEFAULT_OFFSET);
    
    if(!base32_check(key))
    {
        printf("malformed base32 key\n");
        key = 0;
    }
    
    totp.key = base32_decode(key, &totp.keylen);
    totp.hash = set_hash(&hash, DEFAULT_HASH);
    totp.digits = set_value("digits", &digits, DEFAULT_DIGITS);
    totp.step = set_value("step", &step, DEFAULT_STEP);
    totp.t0 = set_value("offset", &offset, DEFAULT_OFFSET);
    
    neobox_text_set(INPUT_NAME, 1, totp.name, 0);
    neobox_text_set(INPUT_KEY, 1, key, 0);
    
    neobox_text_set(INPUT_HASH, 1, hash, 0);
    neobox_text_set(INPUT_DIGITS, 1, digits, 0);
    neobox_text_set(INPUT_STEP, 1, step, 0);
    neobox_text_set(INPUT_OFFSET, 1, offset, 0);
}

void save_config()
{
    totp.name = neobox_text_get(INPUT_NAME, 1);
    char *key = neobox_text_get(INPUT_KEY, 1);
    char *hash = neobox_text_get(INPUT_HASH, 1);
    char *digits = neobox_text_get(INPUT_DIGITS, 1);
    char *step = neobox_text_get(INPUT_STEP, 1);
    char *offset = neobox_text_get(INPUT_OFFSET, 1);
    
    totp.key = base32_decode(key, &totp.keylen);
    totp.hash = set_hash(&hash, 0);
    totp.digits = atoi(digits);
    totp.step = atoi(step);
    totp.t0 = atoi(offset);
    
    neobox_config_set("name", totp.name);
    neobox_config_set("key", key);
    neobox_config_set("hash", hash);
    neobox_config_set("digits", digits);
    neobox_config_set("step", step);
    neobox_config_set("offset", offset);
    
    neobox_config_save();
}

int handler(struct neobox_event event, void *state)
{
    switch(event.type)
    {
    case NEOBOX_EVENT_CHAR:
        switch(event.value.c.c[0])
        {
        case 'k': // setup ok
            save_config();
            neobox_nop_set_name(TEXT_MSG, 1, 0, 1);
            neobox_timer_remove(0);
            break; // fallthrough
        case 'c': // setup cancel
            load_config();
            neobox_nop_set_name(TEXT_MSG, 1, 0, 1);
            return NEOBOX_HANDLER_SUCCESS;
        }
    case NEOBOX_EVENT_TIMER:
        neobox_nop_set_name(TEXT_TOKEN, 0, generate_totp(), 1);
        neobox_timer(0, totp.valid, 0);
        return NEOBOX_HANDLER_SUCCESS;
    default:
        return NEOBOX_HANDLER_DEFER;
    }
}

int check(int id)
{
    char *name = neobox_text_get(INPUT_NAME, 1);
    char *key = neobox_text_get(INPUT_KEY, 1);
    char *hash = neobox_text_get(INPUT_HASH, 1);
    char *digits = neobox_text_get(INPUT_DIGITS, 1);
    char *step = neobox_text_get(INPUT_STEP, 1);
    char *offset = neobox_text_get(INPUT_OFFSET, 1);
    
    if(!*name || !*key || !*hash || !*digits || !*step || !*offset)
        neobox_nop_set_name(TEXT_MSG, 1, "Option unset", 1);
    else if(!base32_check(key))
        neobox_nop_set_name(TEXT_MSG, 1, "Key malformed", 1);
    else if(strcmp("sha1", hash) && strcmp("sha256", hash) && strcmp("sha512", hash))
        neobox_nop_set_name(TEXT_MSG, 1, "Hash unknown", 1);
    else if(strspn(digits, "1234567890") != strlen(digits))
        neobox_nop_set_name(TEXT_MSG, 1, "Digits malformed", 1);
    else if(strspn(step, "1234567890") != strlen(step))
        neobox_nop_set_name(TEXT_MSG, 1, "Step malformed", 1);
    else if(strspn(offset, "1234567890") != strlen(offset))
        neobox_nop_set_name(TEXT_MSG, 1, "Offset malformed", 1);
    else
    {
        neobox_nop_set_name(TEXT_MSG, 1, 0, 1);
        return NEOBOX_BUTTON_CHECK_SUCCESS;
    }
    
    return NEOBOX_BUTTON_CHECK_FAIL;
}

int main(int argc, char* argv[])
{
    int ret;
    
    if((ret = neobox_init_layout(totpLayout, &argc, argv)) < 0)
        return ret;
    
    load_config();
    
    neobox_button_set_check(BUTTON_OK, 1, check, 0);
    
    if(totp.name && totp.key)
    {
        neobox_nop_set_name(TEXT_NAME, 0, totp.name, 0);
        neobox_nop_set_name(TEXT_TOKEN, 0, generate_totp(), 0);
        neobox_timer(0, totp.valid, 0);
    }
    else
        neobox_nop_set_name(TEXT_NAME, 0, "setup required", 0);
    
    neobox_run(handler, 0);
    
    neobox_finish();
    
    if(totp.key)
        free(totp.key);
    
    return 0;
}
