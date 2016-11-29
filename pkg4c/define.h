#pragma once

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
#include "sds.h"
#include "rbtree.h"

typedef int     type_t;
typedef long long int integer_t;
typedef double  number_t;
typedef sds     string_t;
typedef struct mypkg*  object_t;
typedef struct array   array_t;
typedef string_t _key;
typedef unsigned short u_short;

typedef union 
{
    integer_t   i;
    number_t    n;
    string_t    s;
    object_t    o;
} _val;

typedef struct 
{
    type_t  t;
    _val    v;
} _value;

typedef _key mykey_t;
typedef _value myval_t;
typedef struct rb_node rb_node;
typedef struct rb_root rb_root;

typedef struct array
{
    u_short     num;
    u_short     max;
    myval_t    *va;
} array;

typedef struct mynode 
{
    rb_node     node;
    mykey_t     key;
    myval_t     val;
} mynode;

typedef struct myiter
{
    myval_t    *val; // array
    rb_node    *node;// dict
    short       index;
    u_short     maxidx;
    type_t      type; // key type
} myiter;

typedef struct mypkg
{
    array_t     array;
    rb_root     root;
    int         ref_count;
} mypkg;

typedef struct myprint
{
    rb_node     node;
    object_t    pkg;
} myprint;

#define sdskey(key) (key)
#define valtype(val) ((val).t)
#define intval(val) ((val).v.i)
#define numval(val) ((val).v.n)
#define sdsval(val) ((val).v.s)
#define objval(val) ((val).v.o)
#define keycmp(lkey, rkey) strcmp(sdskey(lkey), sdskey(rkey))

#define ARRAY_ROOT (array_t) {0,0,NULL}

#define ARRAY_INIT_SIZE 4

#define MP_T_NIL        0
#define MP_T_BOOLEAN    1
#define MP_T_INTEGER    2
#define MP_T_DOUBLE     3
#define MP_T_STRING     4
#define MP_T_OBJECT     5

int myprint_find(rb_root *root, object_t pkg);
void myprint_set(rb_root *root, object_t pkg);
void myprint_free(rb_root *root);
