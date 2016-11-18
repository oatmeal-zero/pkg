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

/// cjson api
static void parse_array(mypkg *pkg, cJSON *json);
static void parse_object(mypkg *pkg, cJSON *json);
static cJSON* gen_json(mypkg *pkg);

static void parse_number(mypkg *pkg, cJSON *item)
{
    double d = item->valuedouble;
    if (d == 0)
    {
        if (item->string) mpaddinteger(pkg, item->string, 0);
        else mpappendint(pkg, 0);
    }
    else if (fabs(((double)item->valueint) - d) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN)
    {
        if (item->string) mpaddinteger(pkg, item->string, item->valueint);
        else mpappendint(pkg, item->valueint);
    }
    else
    {
        if (item->string) mpaddnumber(pkg, item->string, d);
        else mpappendnum(pkg, d);
    }
}

static void parse_array(mypkg *pkg, cJSON *json)
{
    cJSON *item = json->child;
    while (item)
    {
        switch ((item->type) & 255)
        {
            case cJSON_NULL:
                mpappendnil(pkg);
                break;
            case cJSON_False:   
                mpappendbool(pkg, 0);
                break;
            case cJSON_True:    
                mpappendbool(pkg, 1);
                break;
            case cJSON_Number:  
                parse_number(pkg, item);
                break;
            case cJSON_String:  
                mpappendstr(pkg, item->valuestring, strlen(item->valuestring));
                break;
            case cJSON_Array:   
                {
                mypkg* obj = mpnew();
                parse_array(obj, item);
                mpappendobj(pkg, obj);
                mpfree(obj);
                }
                break;
            case cJSON_Object:  
                {
                mypkg* obj = mpnew();
                parse_object(obj, item);
                mpappendobj(pkg, obj);
                mpfree(obj);
                }
                break;
        }
        item = item->next;
    }
}

static void parse_object(mypkg *pkg, cJSON *json)
{
    cJSON *item = json->child;
    while (item)
    {
        switch ((item->type) & 255)
        {
            case cJSON_NULL:
                mpaddnil(pkg, item->string);
                break;
            case cJSON_False:   
                mpaddboolean(pkg, item->string, 0);
                break;
            case cJSON_True:    
                mpaddboolean(pkg, item->string, 1);
                break;
            case cJSON_Number:  
                parse_number(pkg, item);
                break;
            case cJSON_String:  
                mpaddstring(pkg, item->string, item->valuestring, strlen(item->valuestring));
                break;
            case cJSON_Array:   
                {
                mypkg* obj = mpnew();
                parse_array(obj, item);
                mpaddobject(pkg, item->string, obj);
                mpfree(obj);
                }
                break;
            case cJSON_Object:  
                {
                mypkg* obj = mpnew();
                parse_object(obj, item);
                mpaddobject(pkg, item->string, obj);
                mpfree(obj);
                }
                break;
        }
        item = item->next;
    }
}

// api
mypkg* mpparse_json(const char *text)
{
    cJSON *json = cJSON_Parse(text);
    if (!json) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return NULL;
    }

    mypkg* pkg = mpnew();
    if (json->type == cJSON_Object)
        parse_object(pkg, json);
    else if (json->type == cJSON_Array)
        parse_array(pkg, json);
    else 
        printf("Unknown json value type(%d).", json->type);
    cJSON_Delete(json);
    return pkg;
}

// api
mypkg* mpparse_json_file(const char *file)
{
    struct stat buf;
    int ret = stat(file, &buf);
    if (ret) {
        printf("open file %s error: %s\n", file, strerror(errno));
        return NULL;
    }

    int len = buf.st_size;
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        printf("open file %s error: %s\n", file, strerror(errno));
        return NULL;
    }

    char *text = (char*)malloc(len + 1);
    fread(text, 1, len, fp);
    text[len] = 0;
    fclose(fp);
    mypkg *pkg = mpparse_json(text);
    free(text);
    return pkg;
}

static void _print_json_node(cJSON *root, mynode *node)
{
    switch (valtype(node->val)) {
        case MP_T_NIL:
            cJSON_AddNullToObject(root, sdskey(node->key));
            break;
        case MP_T_BOOLEAN:
            cJSON_AddBoolToObject(root, sdskey(node->key), intval(node->val));
            break;
        case MP_T_INTEGER:
            cJSON_AddNumberToObject(root, sdskey(node->key), intval(node->val));
            break;
        case MP_T_DOUBLE:
            cJSON_AddNumberToObject(root, sdskey(node->key), numval(node->val));
            break;
        case MP_T_STRING:
            cJSON_AddStringToObject(root, sdskey(node->key), sdsval(node->val));
            break;
        case MP_T_OBJECT:
            {
            cJSON *child = gen_json(objval(node->val));
            cJSON_AddItemToObject(root, sdskey(node->key), child);
            }
            break;
    }
}

static void _print_json_val(cJSON *root, myval_t val)
{
    switch (valtype(val)) {
        case MP_T_NIL:
            cJSON_AddItemToArray(root, cJSON_CreateNull());
            break;
        case MP_T_BOOLEAN:
            cJSON_AddItemToArray(root, cJSON_CreateBool(intval(val)));
            break;
        case MP_T_INTEGER:
            cJSON_AddItemToArray(root, cJSON_CreateNumber(intval(val)));
            break;
        case MP_T_DOUBLE:
            cJSON_AddItemToArray(root, cJSON_CreateNumber(numval(val)));
            break;
        case MP_T_STRING:
            cJSON_AddItemToArray(root, cJSON_CreateString(sdsval(val)));
            break;
        case MP_T_OBJECT:
            {
            cJSON *child = gen_json(objval(val));
            cJSON_AddItemToArray(root, child);
            }
            break;
    }
}

static cJSON* gen_json_dict(mypkg *pkg)
{
    cJSON *root = NULL;
    rb_node *node = rb_first(&pkg->root);
    if (node) {
        root = cJSON_CreateObject();
    }

    for (; node; node = rb_next(node))
    {
        mynode *n = rb_entry(node, mynode, node);
        _print_json_node(root, n);
    }

    return root;
}

static cJSON* gen_json_array(mypkg *pkg)
{
    cJSON *root = NULL;
    myval_t *array = pkg->array.va;
    if (array != NULL) {
        root = cJSON_CreateArray();
        u_short num = pkg->array.num;
        u_short idx = 0;
        for (; idx < num; idx++) {
            _print_json_val(root, array[idx]);
        }
    }
    return root;
}

static cJSON* gen_json(mypkg *pkg)
{
    cJSON *root = gen_json_dict(pkg);
    if (!root) {
        root = gen_json_array(pkg);
    }
    return root;
}

// api
void mpprint_json(mypkg *pkg)
{
    cJSON *root = gen_json(pkg);
    if (root) {
        char *out = cJSON_Print(root);
        printf("%s\n", out);
        free(out);
        cJSON_Delete(root);
    }
}

void mpprint_json_unformat(mypkg *pkg)
{
    cJSON *root = gen_json(pkg);
    if (root) {
        char *out = cJSON_PrintUnformatted(root);
        printf("%s\n", out);
        free(out);
        cJSON_Delete(root);
    }
}

char* mpgen_json(mypkg *pkg)
{
    cJSON *root = gen_json(pkg);
    if (root) {
        char *out = cJSON_PrintUnformatted(root);
        sds data = sdsnew(out);
        free(out);
        cJSON_Delete(root);
        return data;
    }
    return NULL;
}

