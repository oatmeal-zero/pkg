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
#include "cJSON.h"

/// mynode
mynode* mynode_search(rb_root *root, const char *key)
{
    rb_node *node = root->rb_node;
    while (node) {
        mynode *data = container_of(node, mynode, node);
        int result = strcmp(key, sdskey(data->key));
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return data;
    }
    return NULL;
}

int mynode_insert(rb_root *root, mynode *data)
{
    rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) {
        mynode *this = container_of(*new, mynode, node);
        int result = keycmp(data->key, this->key);

        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return 0;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root);

    return 1;
}

// myprint
myprint* myprint_search(rb_root *root, object_t pkg)
{
    rb_node *node = root->rb_node;
    while (node) {
        myprint *data = container_of(node, myprint, node);
        if (pkg < data->pkg)
            node = node->rb_left;
        else if (pkg > data->pkg)
            node = node->rb_right;
        else
            return data;
    }
    return NULL;
}

int myprint_insert(rb_root *root, myprint *data)
{
    rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) {
        myprint *this = container_of(*new, myprint, node);
        parent = *new;
        if (data->pkg < this->pkg)
            new = &((*new)->rb_left);
        else if (data->pkg > this->pkg)
            new = &((*new)->rb_right);
        else
            return 0;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root);

    return 1;
}

myprint* printnew(object_t pkg)
{
    myprint *node = zmalloc(sizeof(myprint));
    if (node == NULL) return NULL;
    node->pkg = pkg;
    return node;
}

int myprint_find(rb_root *root, object_t pkg)
{
    myprint *node = myprint_search(root, pkg);
    return (node != NULL);
}

void myprint_set(rb_root *root, object_t pkg)
{
    myprint *node = printnew(pkg);
    myprint_insert(root, node);
}

void myprint_free(rb_root *root)
{
    rb_node *node;
    for (node = rb_first(root); node; node = rb_next(node))
    {
        myprint *n = rb_entry(node, myprint, node);
        rb_erase(&n->node, root);
        zfree(n);
    }
}

/// mp api
mynode* nodenewnil(const char *key)
{
    mynode *node = zmalloc(sizeof(mynode));
    if (node == NULL) return NULL;
    sdskey(node->key) = sdsnew(key);
    valtype(node->val) = MP_T_NIL;
    intval(node->val) = 0;
    return node;
}

mynode* nodenewstr(const char *key, const char *val, size_t len)
{
    mynode *node = zmalloc(sizeof(mynode));
    if (node == NULL) return NULL;
    sdskey(node->key) = sdsnew(key);
    valtype(node->val) = MP_T_STRING;
    sdsval(node->val) = sdsnewlen(val, len);
    return node;
}

mynode* nodenewint(const char *key, type_t type, integer_t val)
{
    mynode *node = zmalloc(sizeof(mynode));
    if (node == NULL) return NULL;
    sdskey(node->key) = sdsnew(key);
    valtype(node->val) = type;
    intval(node->val) = val;
    return node;
}

mynode* nodenewnum(const char *key, number_t val)
{
    mynode *node = zmalloc(sizeof(mynode));
    if (node == NULL) return NULL;
    sdskey(node->key) = sdsnew(key);
    valtype(node->val) = MP_T_DOUBLE;
    numval(node->val) = val;
    return node;
}

mynode* nodenewobj(const char *key, mypkg *val)
{
    mynode *node = zmalloc(sizeof(mynode));
    if (node == NULL) return NULL;
    sdskey(node->key) = sdsnew(key);
    valtype(node->val) = MP_T_OBJECT;
    objval(node->val) = val;
    return node;
}

void valfree(myval_t val)
{
    switch (valtype(val)) {
        case MP_T_STRING:
            sdsfree(sdsval(val));
            break;
        case MP_T_OBJECT:
            mpfree(objval(val));
            break;
    }
}

void nodefree(mynode *node)
{
    sdsfree(sdskey(node->key));
    valfree(node->val);
    zfree(node);
}

void valprint(myval_t val)
{
    switch (valtype(val)) {
        case MP_T_NIL:
            printf("nil, ");
            break;
        case MP_T_BOOLEAN:
            printf("%s, ", (intval(val)==0) ? "false" : "true");
            break;
        case MP_T_INTEGER:
            printf("%lld, ", intval(val));
            break;
        case MP_T_DOUBLE:
            printf("%lf, ", numval(val));
            break;
        case MP_T_STRING:
            printf("%s, ", sdsval(val));
            break;
        case MP_T_OBJECT:
            printf("%p, ", objval(val));
            break;
    }
}

void nodeprint(mynode *node)
{
    switch (valtype(node->val)) {
        case MP_T_NIL:
            printf("%s = nil\n", sdskey(node->key));
            break;
        case MP_T_BOOLEAN:
            printf("%s = %s\n", sdskey(node->key), (intval(node->val)==0) ? "false" : "true");
            break;
        case MP_T_INTEGER:
            printf("%s = %lld\n", sdskey(node->key), intval(node->val));
            break;
        case MP_T_DOUBLE:
            printf("%s = %lf\n", sdskey(node->key), numval(node->val));
            break;
        case MP_T_STRING:
            printf("%s = %s\n", sdskey(node->key), sdsval(node->val));
            break;
        case MP_T_OBJECT:
            printf("%s = %p\n", sdskey(node->key), objval(node->val));
            break;
    }
}

mypkg* mpnew()
{
    mypkg *pkg = zmalloc(sizeof(mypkg));
    if (pkg == NULL) return NULL;
    pkg->array = ARRAY_ROOT;
    pkg->root = RB_ROOT;
    pkg->ref_count = 1;
    return pkg;
}

mypkg* mpdup(mypkg *rhs)
{
    ++rhs->ref_count;
    return rhs;
}

void mpfree(mypkg *pkg)
{
    if (--pkg->ref_count != 0) return;

    rb_node *node;
    for (node = rb_first(&pkg->root); node; node = rb_next(node))
    {
        mynode *n = rb_entry(node, mynode, node);
        rb_erase(&n->node, &pkg->root);
        nodefree(n);
    }

    myval_t *array = pkg->array.va;
    if (array != NULL) {
        u_short num = pkg->array.num;
        u_short idx = 0;
        for (; idx < num; idx++) {
            valfree(array[idx]);
        }
        zfree(array);
    }

    zfree(pkg);
}

size_t mpsize(mypkg *pkg)
{
    size_t num = 0;
    rb_node *node;
    for (node = rb_first(&pkg->root); node; node = rb_next(node))
    {
        ++num;
    }
    return num;
}

void mpclear(mypkg *pkg)
{
    myval_t *array = pkg->array.va;
    if (array != NULL) {
        u_short num = pkg->array.num;
        u_short idx = 0;
        for (; idx < num; idx++) {
            valfree(array[idx]);
        }
        zfree(array);
    }
    pkg->array = ARRAY_ROOT;

    rb_node *node;
    for (node = rb_first(&pkg->root); node; node = rb_next(node))
    {
        mynode *n = rb_entry(node, mynode, node);
        rb_erase(&n->node, &pkg->root);
        nodefree(n);
    }
}

int mpexist(mypkg *pkg, const char *key)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) return 0;

    return 1;
}

void _print(mypkg *pkg, rb_root *root)
{
    if (myprint_find(root, pkg)) return;
    myprint_set(root, pkg);

    rb_root unprint = RB_ROOT;

    myval_t *array = pkg->array.va;
    u_short num = pkg->array.num;
    u_short idx = 0;
    printf("%p::\n", pkg);
    if (num > 0) {
        printf("  [%d][ ", num);
        for (; idx < num; idx++) {
            valprint(array[idx]);
            if (valtype(array[idx]) == MP_T_OBJECT 
                    && !myprint_find(root, objval(array[idx])))
            {
                myprint_set(&unprint, objval(array[idx]));
            }
        }
        printf("]\n");
    }

    rb_node *node;
    for (node = rb_first(&pkg->root); node; node = rb_next(node))
    {
        mynode *n = rb_entry(node, mynode, node);
        printf("  ");
        nodeprint(n);
        if (valtype(n->val) == MP_T_OBJECT 
                && !myprint_find(root, objval(n->val)))
        {
            myprint_set(&unprint, objval(n->val));
        }
    }

    for (node = rb_first(&unprint); node; node = rb_next(node))
    {
        myprint *p = rb_entry(node, myprint, node);
        _print(p->pkg, root);
    }
    myprint_free(&unprint);
}

void mpprint(mypkg *pkg, const char *msg)
{
    if (msg != NULL) printf("%s\n", msg);
    rb_root root = RB_ROOT;
    _print(pkg, &root);
    myprint_free(&root);
}

myiter* mpiternew(mypkg *pkg)
{
    myiter *iter = zmalloc(sizeof(myiter));
    if (iter == NULL) return NULL;
    iter->node = rb_first(&pkg->root);
    return iter;
}

myiter* mpiterend()
{
    myiter *iter = zmalloc(sizeof(myiter));
    if (iter == NULL) return NULL;
    iter->node = NULL;
    return iter;
}

void mpiterfree(myiter *iter)
{
    zfree(iter);
}

void mpiternext(myiter *iter) 
{
    iter->node = rb_next(iter->node);
    return;
}

int mpitercmp(myiter *lhs, myiter *rhs)
{
    return (lhs->node == rhs->node);
}

const char* mpiterkey(myiter *iter)
{
    if (iter->node == NULL) return NULL;
    mynode *node = rb_entry(iter->node, mynode, node);
    return sdskey(node->key);
}

int mpitertype(myiter *iter)
{
    if (iter->node == NULL) return MP_T_NIL;
    mynode *node = rb_entry(iter->node, mynode, node);
    return valtype(node->val);
}

myval_t* mpiterval(myiter *iter, size_t *len)
{
    if (iter->node == NULL) return NULL;
    mynode *node = rb_entry(iter->node, mynode, node);
    if (valtype(node->val) == MP_T_STRING && len != NULL)
    {
        *len = sdslen(sdsval(node->val));
    }
    return &node->val;
}

int mpaddstring(mypkg *pkg, const char *key, const char *val, size_t len)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) {
        node = nodenewstr(key, val, len);
        return mynode_insert(&pkg->root, node);
    }

    return 0;
}

myval_t* mpaddnil(mypkg *pkg, const char *key)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) {
        node = nodenewnil(key);
        mynode_insert(&pkg->root, node);
    }
    return &node->val;
}

int mpaddboolean(mypkg *pkg, const char *key, integer_t val)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) {
        node = nodenewint(key, MP_T_BOOLEAN, val);
        return mynode_insert(&pkg->root, node);
    }

    return 0;
}

int mpaddinteger(mypkg *pkg, const char *key, integer_t val)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) {
        node = nodenewint(key, MP_T_INTEGER, val);
        return mynode_insert(&pkg->root, node);
    }

    return 0;
}

int mpaddnumber(mypkg *pkg, const char *key, number_t val)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) {
        node = nodenewnum(key, val);
        return mynode_insert(&pkg->root, node);
    }

    return 0;
}

int mpaddobject(mypkg *pkg, const char *key, mypkg *rhs)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) {
        // a circular reference
        if (pkg != rhs) ++rhs->ref_count;
        node = nodenewobj(key, rhs);
        return mynode_insert(&pkg->root, node);
    }

    return 0;
}

void _expand_array_if_need(mypkg *pkg)
{
    u_short num = pkg->array.num;
    u_short max = pkg->array.max;
    myval_t *array = pkg->array.va;
    if (num < max) return;

    max = (max > 0) ? max * 2 : ARRAY_INIT_SIZE;
    pkg->array.va = zrealloc(array, sizeof(myval_t) * max);
    pkg->array.max = max;
}

myval_t* mpappendnil(mypkg *pkg)
{
    _expand_array_if_need(pkg);
    u_short num = pkg->array.num;
    valtype(pkg->array.va[num]) = MP_T_NIL;
    intval(pkg->array.va[num]) = 0;
    pkg->array.num++;
    return &pkg->array.va[num];
}

int mpappendbool(mypkg *pkg, integer_t val)
{
    _expand_array_if_need(pkg);
    u_short num = pkg->array.num;
    valtype(pkg->array.va[num]) = MP_T_BOOLEAN;
    intval(pkg->array.va[num]) = val;
    pkg->array.num++;
    return 0;
}

int mpappendint(mypkg *pkg, integer_t val)
{
    _expand_array_if_need(pkg);
    u_short num = pkg->array.num;
    valtype(pkg->array.va[num]) = MP_T_INTEGER;
    intval(pkg->array.va[num]) = val;
    pkg->array.num++;
    return 0;
}

int mpappendnum(mypkg *pkg, number_t val)
{
    _expand_array_if_need(pkg);
    u_short num = pkg->array.num;
    valtype(pkg->array.va[num]) = MP_T_DOUBLE;
    numval(pkg->array.va[num]) = val;
    pkg->array.num++;
    return 0;
}

int mpappendobj(mypkg *pkg, mypkg *val)
{
    _expand_array_if_need(pkg);
    // a circular reference
    if (pkg != val) ++val->ref_count;
    u_short num = pkg->array.num;
    valtype(pkg->array.va[num]) = MP_T_OBJECT;
    objval(pkg->array.va[num]) = val;
    pkg->array.num++;
    return 0;
}

int mpappendstr(mypkg *pkg, const char *val, size_t len)
{
    _expand_array_if_need(pkg);
    u_short num = pkg->array.num;
    valtype(pkg->array.va[num]) = MP_T_STRING;
    sdsval(pkg->array.va[num]) = sdsnewlen(val, len);
    pkg->array.num++;
    return 0;
}

void _fill_nil_val(mypkg *pkg, int index)
{
    if (index > pkg->array.max) return;
    u_short idx = pkg->array.num;
    for (; idx <= index; idx++) {
        mpappendnil(pkg);
    }

    _expand_array_if_need(pkg);
}

myval_t* mpassignnil(mypkg *pkg, int index)
{
    if (index < 0 || index > pkg->array.max) return NULL;
    _fill_nil_val(pkg, index);
    valfree(pkg->array.va[index]);
    valtype(pkg->array.va[index]) = MP_T_NIL;
    intval(pkg->array.va[index]) = 0;
    if (index >= pkg->array.num) pkg->array.num = index + 1;
    return &pkg->array.va[index];
}

int mpassignint(mypkg *pkg, int index, integer_t val)
{
    if (index < 0 || index > pkg->array.max) return -1;
    _fill_nil_val(pkg, index);
    valfree(pkg->array.va[index]);
    valtype(pkg->array.va[index]) = MP_T_INTEGER;
    intval(pkg->array.va[index]) = val;
    if (index >= pkg->array.num) pkg->array.num = index + 1;
    return 0;
}

int mpassignnum(mypkg *pkg, int index, number_t val)
{
    if (index < 0 || index > pkg->array.max) return -1;
    _fill_nil_val(pkg, index);
    valfree(pkg->array.va[index]);
    valtype(pkg->array.va[index]) = MP_T_DOUBLE;
    numval(pkg->array.va[index]) = val;
    if (index >= pkg->array.num) pkg->array.num = index + 1;
    return 0;
}

int mpassignobj(mypkg *pkg, int index, mypkg *val)
{
    if (index < 0 || index > pkg->array.max) return -1;
    _fill_nil_val(pkg, index);
    valfree(pkg->array.va[index]);
    // a circular reference
    if (pkg != val) ++val->ref_count;
    valtype(pkg->array.va[index]) = MP_T_OBJECT;
    objval(pkg->array.va[index]) = val;
    if (index >= pkg->array.num) pkg->array.num = index + 1;
    return 0;
}

int mpassignstr(mypkg *pkg, int index, const char *val, size_t len)
{
    if (index < 0 || index > pkg->array.max) return -1;
    _fill_nil_val(pkg, index);
    valfree(pkg->array.va[index]);
    valtype(pkg->array.va[index]) = MP_T_STRING;
    sdsval(pkg->array.va[index]) = sdsnewlen(val, len);
    if (index >= pkg->array.num) pkg->array.num = index + 1;
    return 0;
}

mynode* mpgetnode(mypkg *pkg, const char *key)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) return NULL;

    return node;
}

int mpnodetype(mynode *node)
{
    if (node == NULL) return MP_T_NIL;
    return valtype(node->val);
}

const char* mpnodekey(mynode *node)
{
    if (node == NULL) return NULL;
    return sdskey(node->key);
}

mypkg* mpnodevalobj(mynode *node)
{
    if (node == NULL) return NULL;
    mypkg *pkg = objval(node->val);
    return pkg;
}

integer_t mpnodevalint(mynode *node)
{
    if (node == NULL) return 0;
    return intval(node->val);
}

number_t mpnodevalnum(mynode *node)
{
    if (node == NULL) return 0.00;
    return numval(node->val);
}

const char* mpnodevalstr(mynode *node, size_t *len)
{
    *len = 0;
    if (node == NULL) return NULL;
    *len = sdslen(sdsval(node->val));
    return sdsval(node->val);
}

myval_t* mpgetval(mypkg *pkg, const char *key)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) return NULL;
    return &node->val;
}

myval_t* mpgetaval(mypkg *pkg, int index)
{
    if (index < 0 || index >= pkg->array.num) return NULL;
    return &pkg->array.va[index];
}

int mpvaltype(myval_t *val)
{
    if (val == NULL) return MP_T_NIL;
    return valtype(*val);
}

mypkg* mpvalobj(myval_t *val)
{
    if (val == NULL) return NULL;
    mypkg *pkg = objval(*val);
    return pkg;
}

integer_t mpvalint(myval_t *val)
{
    if (val == NULL) return 0;
    return intval(*val);
}

number_t mpvalnum(myval_t *val)
{
    if (val == NULL) return 0.00;
    return numval(*val);
}

const char* mpvalstr(myval_t *val, size_t *len)
{
    *len = 0;
    if (val == NULL) return NULL;
    *len = sdslen(sdsval(*val));
    return sdsval(*val);
}

void mpsetvalobj(myval_t *lhs, mypkg *val)
{
    valfree(*lhs);
    valtype(*lhs) = MP_T_OBJECT;
    objval(*lhs) = val;
    // notice: may be circular reference
    ++val->ref_count;
}

void mpsetvalbool(myval_t *lhs, integer_t val)
{
    valfree(*lhs);
    valtype(*lhs) = MP_T_BOOLEAN;
    intval(*lhs) = val;
}

void mpsetvalint(myval_t *lhs, integer_t val)
{
    valfree(*lhs);
    valtype(*lhs) = MP_T_INTEGER;
    intval(*lhs) = val;
}

void mpsetvalnum(myval_t *lhs, number_t val)
{
    valfree(*lhs);
    valtype(*lhs) = MP_T_DOUBLE;
    numval(*lhs) = val;
}

void mpsetvalstr(myval_t *lhs, const char *val, size_t len)
{
    valfree(*lhs);
    valtype(*lhs) = MP_T_STRING;
    sdsval(*lhs) = sdsnewlen(val, len);
}

const char* mpgetstring(mypkg *pkg, const char *key)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) return NULL;

    return sdsval(node->val);
}

const char* mpgetlstring(mypkg *pkg, const char *key, size_t *len)
{
    *len = 0;
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) return NULL;

    *len = sdslen(sdsval(node->val));
    return sdsval(node->val);
}

int mpgetarraylen(mypkg *pkg)
{
    return pkg->array.num;
}

int mpdelnode(mypkg *pkg, const char *key)
{
    mynode *node = mynode_search(&pkg->root, key);
    if (node == NULL) return 1;

    rb_erase(&node->node, &pkg->root);
    nodefree(node);

    return 0;
}

