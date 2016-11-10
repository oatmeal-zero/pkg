#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "define.h"
#include "pkgapi.h"
#include "zmalloc.h"

/* Check if float or double can be an integer without loss of precision */
#define IS_INT_TYPE_EQUIVALENT(x, T) (!isinf(x) && (T)(x) == (x))

#define IS_INT64_EQUIVALENT(x) IS_INT_TYPE_EQUIVALENT(x, int64_t)
#define IS_INT_EQUIVALENT(x) IS_INT_TYPE_EQUIVALENT(x, int)

/* If size of pointer is equal to a 4 byte integer, we're on 32 bits. */
#if UINTPTR_MAX == UINT_MAX
    #define BITS_32 1
#else
    #define BITS_32 0
#endif

/* -------------------------- Endian conversion --------------------------------
 * We use it only for floats and doubles, all the other conversions performed
 * in an endian independent fashion. So the only thing we need is a function
 * that swaps a binary string if arch is little endian (and left it untouched
 * otherwise). */

/* Reverse memory bytes if arch is little endian. Given the conceptual
 * simplicity of the Lua build system we prefer check for endianess at runtime.
 * The performance difference should be acceptable. */
void memrevifle(void *ptr, size_t len) {
    unsigned char   *p = (unsigned char *)ptr,
                    *e = (unsigned char *)p+len-1,
                    aux;
    int test = 1;
    unsigned char *testp = (unsigned char*) &test;

    if (testp[0] == 0) return; /* Big endian, nothing to do. */
    len /= 2;
    while(len--) {
        aux = *p;
        *p = *e;
        *e = aux;
        p++;
        e--;
    }
}

/* ---------------------------- String buffer ----------------------------------
 * This is a simple implementation of string buffers. The only operation
 * supported is creating empty buffers and appending bytes to it.
 * The string buffer uses 2x preallocation on every realloc for O(N) append
 * behavior.  */
typedef struct mp_buf {
    unsigned char *b;
    size_t len, free;
} mp_buf;

mp_buf *mp_buf_new() {
    mp_buf *buf = zmalloc(sizeof(mp_buf));
    buf->b = NULL;
    buf->len = buf->free = 0;
    return buf;
}

void mp_buf_append(mp_buf *buf, const unsigned char *s, size_t len) {
    if (buf->free < len) {
        size_t newsize = (buf->len+len)*2;
        buf->b = zrealloc(buf->b, newsize);
        buf->free = newsize - buf->len;
    }
    memcpy(buf->b+buf->len,s,len);
    buf->len += len;
    buf->free -= len;
}

void mp_buf_free(mp_buf *buf) {
    zfree(buf->b);
    zfree(buf);
}

/* ---------------------------- String cursor ----------------------------------
 * This simple data structure is used for parsing. Basically you create a cursor
 * using a string pointer and a length, then it is possible to access the
 * current string position with cursor->p, check the remaining length
 * in cursor->left, and finally consume more string using
 * mp_cur_consume(cursor,len), to advance 'p' and subtract 'left'.
 * An additional field cursor->error is set to zero on initialization and can
 * be used to report errors. */

#define MP_CUR_ERROR_NONE   0
#define MP_CUR_ERROR_EOF    1   /* Not enough data to complete operation. */
#define MP_CUR_ERROR_BADFMT 2   /* Bad data format */

typedef struct mp_cur {
    const unsigned char *p;
    size_t left;
    int err;
} mp_cur;

void mp_cur_init(mp_cur *cursor, const unsigned char *s, size_t len) {
    cursor->p = s;
    cursor->left = len;
    cursor->err = MP_CUR_ERROR_NONE;
}

#define mp_cur_consume(_c,_len) do { _c->p += _len; _c->left -= _len; } while(0)

/* When there is not enough room we set an error in the cursor and return. This
 * is very common across the code so we have a macro to make the code look
 * a bit simpler. */
#define mp_cur_need(_c,_len) do { \
    if (_c->left < _len) { \
        _c->err = MP_CUR_ERROR_EOF; \
        return; \
    } \
} while(0)

/* ------------------------- Low level MP encoding -------------------------- */

void mp_encode_bytes(mp_buf *buf, const unsigned char *s, size_t len) {
    unsigned char hdr[5];
    int hdrlen;

    if (len < 32) {
        hdr[0] = 0xa0 | (len&0xff); /* fix raw */
        hdrlen = 1;
    } else if (len <= 0xff) {
        hdr[0] = 0xd9;
        hdr[1] = len;
        hdrlen = 2;
    } else if (len <= 0xffff) {
        hdr[0] = 0xda;
        hdr[1] = (len&0xff00)>>8;
        hdr[2] = len&0xff;
        hdrlen = 3;
    } else {
        hdr[0] = 0xdb;
        hdr[1] = (len&0xff000000)>>24;
        hdr[2] = (len&0xff0000)>>16;
        hdr[3] = (len&0xff00)>>8;
        hdr[4] = len&0xff;
        hdrlen = 5;
    }
    mp_buf_append(buf,hdr,hdrlen);
    mp_buf_append(buf,s,len);
}

/* we assume IEEE 754 internal format for single and double precision floats. */
void mp_encode_double(mp_buf *buf, double d) {
    unsigned char b[9];
    float f = d;

    assert(sizeof(f) == 4 && sizeof(d) == 8);
    if (d == (double)f) {
        b[0] = 0xca;    /* float IEEE 754 */
        memcpy(b+1,&f,4);
        memrevifle(b+1,4);
        mp_buf_append(buf,b,5);
    } else if (sizeof(d) == 8) {
        b[0] = 0xcb;    /* double IEEE 754 */
        memcpy(b+1,&d,8);
        memrevifle(b+1,8);
        mp_buf_append(buf,b,9);
    }
}

void mp_encode_int(mp_buf *buf, integer_t n) {
    unsigned char b[9];
    int enclen;

    if (n >= 0) {
        if (n <= 127) {
            b[0] = n & 0x7f;    /* positive fixnum */
            enclen = 1;
        } else if (n <= 0xff) {
            b[0] = 0xcc;        /* uint 8 */
            b[1] = n & 0xff;
            enclen = 2;
        } else if (n <= 0xffff) {
            b[0] = 0xcd;        /* uint 16 */
            b[1] = (n & 0xff00) >> 8;
            b[2] = n & 0xff;
            enclen = 3;
        } else if (n <= 0xffffffffLL) {
            b[0] = 0xce;        /* uint 32 */
            b[1] = (n & 0xff000000) >> 24;
            b[2] = (n & 0xff0000) >> 16;
            b[3] = (n & 0xff00) >> 8;
            b[4] = n & 0xff;
            enclen = 5;
        } else {
            b[0] = 0xcf;        /* uint 64 */
            b[1] = (n & 0xff00000000000000LL) >> 56;
            b[2] = (n & 0xff000000000000LL) >> 48;
            b[3] = (n & 0xff0000000000LL) >> 40;
            b[4] = (n & 0xff00000000LL) >> 32;
            b[5] = (n & 0xff000000) >> 24;
            b[6] = (n & 0xff0000) >> 16;
            b[7] = (n & 0xff00) >> 8;
            b[8] = n & 0xff;
            enclen = 9;
        }
    } else {
        if (n >= -32) {
            b[0] = ((signed char)n);   /* negative fixnum */
            enclen = 1;
        } else if (n >= -128) {
            b[0] = 0xd0;        /* int 8 */
            b[1] = n & 0xff;
            enclen = 2;
        } else if (n >= -32768) {
            b[0] = 0xd1;        /* int 16 */
            b[1] = (n & 0xff00) >> 8;
            b[2] = n & 0xff;
            enclen = 3;
        } else if (n >= -2147483648LL) {
            b[0] = 0xd2;        /* int 32 */
            b[1] = (n & 0xff000000) >> 24;
            b[2] = (n & 0xff0000) >> 16;
            b[3] = (n & 0xff00) >> 8;
            b[4] = n & 0xff;
            enclen = 5;
        } else {
            b[0] = 0xd3;        /* int 64 */
            b[1] = (n & 0xff00000000000000LL) >> 56;
            b[2] = (n & 0xff000000000000LL) >> 48;
            b[3] = (n & 0xff0000000000LL) >> 40;
            b[4] = (n & 0xff00000000LL) >> 32;
            b[5] = (n & 0xff000000) >> 24;
            b[6] = (n & 0xff0000) >> 16;
            b[7] = (n & 0xff00) >> 8;
            b[8] = n & 0xff;
            enclen = 9;
        }
    }
    mp_buf_append(buf,b,enclen);
}

void mp_encode_array(mp_buf *buf, int64_t n) {
    unsigned char b[5];
    int enclen;

    if (n <= 15) {
        b[0] = 0x90 | (n & 0xf);    /* fix array */
        enclen = 1;
    } else if (n <= 65535) {
        b[0] = 0xdc;                /* array 16 */
        b[1] = (n & 0xff00) >> 8;
        b[2] = n & 0xff;
        enclen = 3;
    } else {
        b[0] = 0xdd;                /* array 32 */
        b[1] = (n & 0xff000000) >> 24;
        b[2] = (n & 0xff0000) >> 16;
        b[3] = (n & 0xff00) >> 8;
        b[4] = n & 0xff;
        enclen = 5;
    }
    mp_buf_append(buf,b,enclen);
}

void mp_encode_dict(mp_buf *buf, int64_t n) {
    unsigned char b[5];
    int enclen;

    if (n <= 15) {
        b[0] = 0x80 | (n & 0xf);    /* fix map */
        enclen = 1;
    } else if (n <= 65535) {
        b[0] = 0xde;                /* map 16 */
        b[1] = (n & 0xff00) >> 8;
        b[2] = n & 0xff;
        enclen = 3;
    } else {
        b[0] = 0xdf;                /* map 32 */
        b[1] = (n & 0xff000000) >> 24;
        b[2] = (n & 0xff0000) >> 16;
        b[3] = (n & 0xff00) >> 8;
        b[4] = n & 0xff;
        enclen = 5;
    }
    mp_buf_append(buf,b,enclen);
}

/* --------------------------- pkg types encoding --------------------------- */

void mp_encode_string(mp_buf *buf, sds val) {
    mp_encode_bytes(buf, (const unsigned char*)val, sdslen(val));
}

void mp_encode_bool(mp_buf *buf, int val) {
    unsigned char b = val ? 0xc3 : 0xc2;
    mp_buf_append(buf,&b,1);
}

void mp_encode_null(mp_buf *buf) {
    unsigned char b[1];
    b[0] = 0xc0;
    mp_buf_append(buf,b,1);
}

void mp_encode_pkg(mp_buf *buf, mypkg *pkg, rb_root *root);
void mp_encode_value(myval_t val, mp_buf *buf, rb_root *root)
{
    switch (valtype(val)) {
        case MP_T_NIL:
            mp_encode_null(buf);
            break;
        case MP_T_BOOLEAN:
            mp_encode_bool(buf, intval(val));
            break;
        case MP_T_INTEGER:
            mp_encode_int(buf, intval(val));
            break;
        case MP_T_DOUBLE:
            mp_encode_double(buf, numval(val));
            break;
        case MP_T_STRING:
            mp_encode_string(buf, sdsval(val));
            break;
        case MP_T_OBJECT:
            mp_encode_pkg(buf, objval(val), root);
            break;
    }
}

void mp_encode_node(mynode *node, mp_buf *buf, rb_root *root)
{
    switch (valtype(node->val)) {
        case MP_T_NIL:
            mp_encode_string(buf, sdskey(node->key));
            mp_encode_null(buf);
            break;
        case MP_T_BOOLEAN:
            mp_encode_string(buf, sdskey(node->key));
            mp_encode_bool(buf, intval(node->val));
            break;
        case MP_T_INTEGER:
            mp_encode_string(buf, sdskey(node->key));
            mp_encode_int(buf, intval(node->val));
            break;
        case MP_T_DOUBLE:
            mp_encode_string(buf, sdskey(node->key));
            mp_encode_double(buf, numval(node->val));
            break;
        case MP_T_STRING:
            mp_encode_string(buf, sdskey(node->key));
            mp_encode_string(buf, sdsval(node->val));
            break;
        case MP_T_OBJECT:
            mp_encode_string(buf, sdskey(node->key));
            mp_encode_pkg(buf, objval(node->val), root);
            break;
    }
}

void mp_encode_pkg_array(mypkg *pkg, mp_buf *buf, rb_root *root) {
    myval_t *array = pkg->array.va;
    u_short num = pkg->array.num;
    u_short idx = 0;
    if (num > 0) {
        mp_encode_array(buf,num);
        for (; idx < num; idx++) {
            mp_encode_value(array[idx], buf, root);
        }
    }
}

void mp_encode_pkg_dict(mypkg *pkg, mp_buf *buf, rb_root *root) {
    size_t num = mpsize(pkg);
    if (num > 0) {
        mp_encode_dict(buf, num);

        rb_node *node;
        for (node = rb_first(&pkg->root); node; node = rb_next(node))
        {
            mynode *n = rb_entry(node, mynode, node);
            mp_encode_node(n, buf, root);
        }
    }
}

void mp_encode_pkg(mp_buf *buf, mypkg *pkg, rb_root *root)
{
    if (myprint_find(root, pkg)) {
        mp_encode_null(buf);
        return;
    }
    myprint_set(root, pkg);

    mp_encode_pkg_array(pkg, buf, root);
    mp_encode_pkg_dict(pkg, buf, root);
}

/* ------------------------------- Decoding --------------------------------- */
typedef void* (*set_val_func)(void *ud, myval_t val);
void mp_decode_pkg_array(mypkg *pkg, mp_cur *c);
void mp_decode_pkg_dict(mypkg *pkg, mp_cur *c);

/* Decode a Message Pack raw object pointed by the string cursor 'c' to
 * a Lua type, that is left as the only result on the stack. */
void mp_decode_key(mypkg *pkg, mp_cur *c, myval_t **v) {
    mp_cur_need(c,1);
    switch(c->p[0]) {
    case 0xd9:  /* raw 8 */
        mp_cur_need(c,2);
        {
            size_t l = c->p[1];
            mp_cur_need(c,2+l);
            sds key = sdsnewlen((char*)c->p+2, l);
            *v = mpaddnil(pkg, key);
            sdsfree(key);
            mp_cur_consume(c,2+l);
        }
        break;  
    case 0xda:  /* raw 16 */
        mp_cur_need(c,3);
        {
            size_t l = (c->p[1] << 8) | c->p[2];
            mp_cur_need(c,3+l);
            sds key = sdsnewlen((char*)c->p+3, l);
            *v = mpaddnil(pkg, key);
            sdsfree(key);
            mp_cur_consume(c,3+l);
        }   
        break;
    case 0xdb:  /* raw 32 */
        mp_cur_need(c,5);
        {
            size_t l = ((size_t)c->p[1] << 24) |
                       ((size_t)c->p[2] << 16) |
                       ((size_t)c->p[3] << 8) |
                       (size_t)c->p[4];
            mp_cur_consume(c,5);
            mp_cur_need(c,l);
            sds key = sdsnewlen((char*)c->p, l);
            *v = mpaddnil(pkg, key);
            sdsfree(key);
            mp_cur_consume(c,l);
        }
        break;
    default:
        if ((c->p[0] & 0xe0) == 0xa0) {  /* fix raw */
            size_t l = c->p[0] & 0x1f;
            mp_cur_need(c,1+l);
            sds key = sdsnewlen((char*)c->p+1, l);
            *v = mpaddnil(pkg, key);
            sdsfree(key);
            mp_cur_consume(c,1+l);
        }
    }
}

void mp_decode_type(mypkg *pkg, mp_cur *c, set_val_func set_func, void *ext) {
    mp_cur_need(c,1);
    myval_t val;
    switch(c->p[0]) {
    case 0xcc:  /* uint 8 */
        mp_cur_need(c,2);
        valtype(val) = MP_T_INTEGER;
        intval(val) = c->p[1];
        mp_cur_consume(c,2);
        break;
    case 0xd0:  /* int 8 */
        mp_cur_need(c,2);
        valtype(val) = MP_T_INTEGER;
        intval(val) = (signed char)c->p[1];
        mp_cur_consume(c,2);
        break;
    case 0xcd:  /* uint 16 */
        mp_cur_need(c,3);
        valtype(val) = MP_T_INTEGER;
        intval(val) = (c->p[1] << 8) | c->p[2];
        mp_cur_consume(c,3);
        break;
    case 0xd1:  /* int 16 */
        mp_cur_need(c,3);
        valtype(val) = MP_T_INTEGER;
        intval(val) = (int16_t) (c->p[1] << 8) | c->p[2];
        mp_cur_consume(c,3);
        break;
    case 0xce:  /* uint 32 */
        mp_cur_need(c,5);
        valtype(val) = MP_T_INTEGER;
        intval(val) = 
            ((uint32_t)c->p[1] << 24) |
            ((uint32_t)c->p[2] << 16) |
            ((uint32_t)c->p[3] << 8) |
            (uint32_t)c->p[4];
        mp_cur_consume(c,5);
        break;
    case 0xd2:  /* int 32 */
        mp_cur_need(c,5);
        valtype(val) = MP_T_INTEGER;
        intval(val) = 
            ((int32_t)c->p[1] << 24) |
            ((int32_t)c->p[2] << 16) |
            ((int32_t)c->p[3] << 8) |
            (int32_t)c->p[4];
        mp_cur_consume(c,5);
        break;
    case 0xcf:  /* uint 64 */
        mp_cur_need(c,9);
        valtype(val) = MP_T_INTEGER;
        intval(val) = 
            ((uint64_t)c->p[1] << 56) |
            ((uint64_t)c->p[2] << 48) |
            ((uint64_t)c->p[3] << 40) |
            ((uint64_t)c->p[4] << 32) |
            ((uint64_t)c->p[5] << 24) |
            ((uint64_t)c->p[6] << 16) |
            ((uint64_t)c->p[7] << 8) |
            (uint64_t)c->p[8];
        mp_cur_consume(c,9);
        break;
    case 0xd3:  /* int 64 */
        mp_cur_need(c,9);
        valtype(val) = MP_T_INTEGER;
        intval(val) =
            ((int64_t)c->p[1] << 56) |
            ((int64_t)c->p[2] << 48) |
            ((int64_t)c->p[3] << 40) |
            ((int64_t)c->p[4] << 32) |
            ((int64_t)c->p[5] << 24) |
            ((int64_t)c->p[6] << 16) |
            ((int64_t)c->p[7] << 8) |
            (int64_t)c->p[8];
        mp_cur_consume(c,9);
        break;
    case 0xc0:  /* nil */
        valtype(val) = MP_T_NIL;
        intval(val) = 0;
        mp_cur_consume(c,1);
        break;
    case 0xc3:  /* true */
        valtype(val) = MP_T_BOOLEAN;
        intval(val) = 1;
        mp_cur_consume(c,1);
        break;
    case 0xc2:  /* false */
        valtype(val) = MP_T_BOOLEAN;
        intval(val) = 0;
        mp_cur_consume(c,1);
        break;
    case 0xca:  /* float */
        mp_cur_need(c,5);
        assert(sizeof(float) == 4);
        {
            float f;
            memcpy(&f,c->p+1,4);
            memrevifle(&f,4);
            valtype(val) = MP_T_DOUBLE;
            numval(val) = f;
            mp_cur_consume(c,5);
        }
        break;
    case 0xcb:  /* double */
        mp_cur_need(c,9);
        assert(sizeof(double) == 8); 
        {   
            double d;
            memcpy(&d,c->p+1,8); 
            memrevifle(&d,8); 
            valtype(val) = MP_T_DOUBLE;
            numval(val) = d;
            mp_cur_consume(c,9);
        }
        break;
    case 0xd9:  /* raw 8 */
        mp_cur_need(c,2);
        {
            size_t l = c->p[1];
            mp_cur_need(c,2+l);
            valtype(val) = MP_T_STRING;
            sdsval(val) = sdsnewlen((char*)c->p+2,l);
            mp_cur_consume(c,2+l);
        }
        break;  
    case 0xda:  /* raw 16 */
        mp_cur_need(c,3);
        {
            size_t l = (c->p[1] << 8) | c->p[2];
            mp_cur_need(c,3+l);
            valtype(val) = MP_T_STRING;
            sdsval(val) = sdsnewlen((char*)c->p+3,l);
            mp_cur_consume(c,3+l);
        }   
        break;
    case 0xdb:  /* raw 32 */
        mp_cur_need(c,5);
        {
            size_t l = ((size_t)c->p[1] << 24) |
                       ((size_t)c->p[2] << 16) |
                       ((size_t)c->p[3] << 8) |
                       (size_t)c->p[4];
            mp_cur_consume(c,5);
            mp_cur_need(c,l);
            valtype(val) = MP_T_STRING;
            sdsval(val) = sdsnewlen((char*)c->p,l);
            mp_cur_consume(c,l);
        }
        break;
    case 0xdc:  /* array 16 */
    case 0xdd:  /* array 32 */
        {
            valtype(val) = MP_T_OBJECT;
            mypkg *child = mpnew();
            objval(val) = child;
            mp_decode_pkg_array(child, c);
        }
        break;
    case 0xde:  /* map 16 */
    case 0xdf:  /* map 32 */
        {
            valtype(val) = MP_T_OBJECT;
            mypkg *child = mpnew();
            objval(val) = child;
            mp_decode_pkg_dict(child, c);
        }
        break;
    default:    /* types that can't be idenitified by first byte value. */
        if ((c->p[0] & 0x80) == 0) {   /* positive fixnum */
            valtype(val) = MP_T_INTEGER;
            intval(val) = c->p[0];
            mp_cur_consume(c,1);
        } else if ((c->p[0] & 0xe0) == 0xe0) {  /* negative fixnum */
            valtype(val) = MP_T_INTEGER;
            intval(val) = (signed char)c->p[0];
            mp_cur_consume(c,1);
        } else if ((c->p[0] & 0xe0) == 0xa0) {  /* fix raw */
            size_t l = c->p[0] & 0x1f;
            mp_cur_need(c,1+l);
            valtype(val) = MP_T_STRING;
            sdsval(val) = sdsnewlen((char*)c->p+1,l);
            mp_cur_consume(c,1+l);
        } else if ((c->p[0] & 0xf0) == 0x90) {  /* fix array */
            valtype(val) = MP_T_OBJECT;
            mypkg *child = mpnew();
            objval(val) = child;
            mp_decode_pkg_array(child, c);
        } else if ((c->p[0] & 0xf0) == 0x80) {  /* fix map */
            valtype(val) = MP_T_OBJECT;
            mypkg *child = mpnew();
            objval(val) = child;
            mp_decode_pkg_dict(child, c);
        } else {
            c->err = MP_CUR_ERROR_BADFMT;
            return;
        }
    }

    set_func(ext, val);
}

void* append_val(void *ud, myval_t val)
{
    mypkg *pkg = (mypkg*)ud;
    myval_t *v = mpappendnil(pkg);
    *v = val;
    return NULL;
}

void* add_key(void *ud, myval_t val)
{
    mypkg *pkg = (mypkg*)ud;
    const char* key = sdsval(val);
    return mpaddnil(pkg, key);
}

void* assigned_val(void *ud, myval_t val)
{
    myval_t *v = (myval_t*)ud;
    *v = val;
    return NULL;
}

void mp_decode_pkg_array(mypkg *pkg, mp_cur *c)
{
    size_t idx = 0;
    size_t len = 0;
    mp_cur_need(c,1);
    switch(c->p[0]) {
    case 0xdc:  /* array 16 */
        mp_cur_need(c,3);
        {
            len = (c->p[1] << 8) | c->p[2];
            mp_cur_consume(c,3);
        }
        break;
    case 0xdd:  /* array 32 */
        mp_cur_need(c,5);
        {
            len = ((size_t)c->p[1] << 24) |
                ((size_t)c->p[2] << 16) |
                ((size_t)c->p[3] << 8) |
                (size_t)c->p[4];
            mp_cur_consume(c,5);
        }
        break;
    default:
        if ((c->p[0] & 0xf0) == 0x90) {  /* fix array */
            len = c->p[0] & 0xf;
            mp_cur_consume(c,1);
        }
    }

    for (; idx < len; idx++) {
        if (c->err == MP_CUR_ERROR_EOF) {
            printf("Missing bytes in input.\n");
            break;
        } else if (c->err == MP_CUR_ERROR_BADFMT) {
            printf("Bad data format in input.\n");
            break;
        }

        mp_decode_type(pkg, c, append_val, pkg);
    }
}

void mp_decode_pkg_dict(mypkg *pkg, mp_cur *c)
{
    size_t idx = 0;
    size_t len = 0;
    mp_cur_need(c,1);
    switch(c->p[0]) {
    case 0xde:  /* map 16 */
        mp_cur_need(c,3);
        {
            len = (c->p[1] << 8) | c->p[2];
            mp_cur_consume(c,3);
        }
        break;
    case 0xdf:  /* map 32 */
        mp_cur_need(c,5);
        {
            len = ((size_t)c->p[1] << 24) |
                       ((size_t)c->p[2] << 16) |
                       ((size_t)c->p[3] << 8) |
                       (size_t)c->p[4];
            mp_cur_consume(c,5);
        }
        break;
    default:
        if ((c->p[0] & 0xf0) == 0x80) {  /* fix map */
            len = c->p[0] & 0xf;
            mp_cur_consume(c,1);
        }
    }

    for (; idx < len; idx++) {
        if (c->err == MP_CUR_ERROR_EOF) {
            printf("Missing bytes in input.\n");
            break;
        } else if (c->err == MP_CUR_ERROR_BADFMT) {
            printf("Bad data format in input.\n");
            break;
        }

        myval_t *val = NULL;
        mp_decode_key(pkg, c, &val);
        mp_decode_type(pkg, c, assigned_val, val);
    }
}

void mp_decode_pkg(mypkg *pkg, mp_cur *c)
{
    mp_decode_pkg_array(pkg, c);
    mp_decode_pkg_dict(pkg, c);
}

/// api
char* mppack(mypkg *pkg)
{
    mp_buf *buf = mp_buf_new();
    rb_root root = RB_ROOT;
    mp_encode_pkg(buf, pkg, &root);
    myprint_free(&root);

    sds data = sdsnewlen(buf->b, buf->len);
    mp_buf_free(buf);

    return data;
}

int mpunpack(mypkg *pkg, const char *data, size_t len)
{
    mp_cur c;
    mp_cur_init(&c, (const unsigned char *)data, len);
    mp_decode_pkg(pkg, &c);
    return len;
}
