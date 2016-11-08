#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct mynode;
struct myval_t;
mypkg* mpnew();
mypkg* mpdup(mypkg *rhs);
void mpfree(mypkg *pkg);
void mpclear(mypkg *pkg);
int mpexist(mypkg *pkg, const char *key);
void mpprint(mypkg *pkg, const char *msg);
// rbtree add
myval_t* mpaddnil(mypkg *pkg, const char *key);
int mpaddboolean(mypkg *pkg, const char *key, integer_t val);
int mpaddinteger(mypkg *pkg, const char *key, integer_t val);
int mpaddnumber(mypkg *pkg, const char *key, number_t val);
int mpaddstring(mypkg *pkg, const char *key, const char *val, size_t len);
int mpaddobject(mypkg *pkg, const char *key, mypkg *rhs);
// array append
myval_t* mpappendnil(mypkg *pkg);
int mpappendbool(mypkg *pkg, integer_t val);
int mpappendint(mypkg *pkg, integer_t val);
int mpappendnum(mypkg *pkg, number_t val);
int mpappendobj(mypkg *pkg, mypkg *val);
int mpappendstr(mypkg *pkg, const char *val, size_t len);
// array assignment 
myval_t* mpassignnil(mypkg *pkg, int index);
int mpassignint(mypkg *pkg, int index, integer_t val);
int mpassignnum(mypkg *pkg, int index, number_t val);
int mpassignobj(mypkg *pkg, int index, mypkg *val);
int mpassignstr(mypkg *pkg, int index, const char *val, size_t len);
// get node value
mynode* mpgetnode(mypkg *pkg, const char *key);
int mpnodetype(mynode *node);
const char* mpnodekey(mynode *node);
mypkg* mpnodevalobj(mynode *node);
integer_t mpnodevalint(mynode *node);
number_t mpnodevalnum(mynode *node);
const char* mpnodevalstr(mynode *node, size_t *len);
// get value interface
myval_t* mpgetval(mypkg *pkg, const char *key);
myval_t* mpgetaval(mypkg *pkg, int index);
int mpvaltype(myval_t *val);
mypkg* mpvalobj(myval_t *val);
integer_t mpvalint(myval_t *val);
number_t mpvalnum(myval_t *val);
const char* mpvalstr(myval_t *val, size_t *len);
// set value interface
void mpsetvalobj(myval_t *lhs, mypkg *val);
void mpsetvalint(myval_t *lhs, integer_t val);
void mpsetvalbool(myval_t *lhs, integer_t val);
void mpsetvalnum(myval_t *lhs, number_t val);
void mpsetvalstr(myval_t *lhs, const char *val, size_t len);
// get string
const char* mpgetstring(mypkg *pkg, const char *key);
const char* mpgetlstring(mypkg *pkg, const char *key, size_t *len);
int mpgetarraylen(mypkg *pkg);
// del node
int mpdelnode(mypkg *pkg, const char *key);
// iterator
myiter* mpiternew(mypkg *pkg);
myiter* mpiterend();
void mpiterfree(myiter *iter);
void mpiternext(myiter *iter);
int mpitercmp(myiter *lhs, myiter *rhs);
const char* mpiterkey(myiter *iter);
int mpitertype(myiter *iter);
myval_t* mpiterval(myiter *iter, size_t *len);
// cjson api
mypkg* mpparse_json(const char *text);
mypkg* mpparse_json_file(const char *file);
void mpprint_json(mypkg *pkg);
void mpprint_json_unformat(mypkg *pkg);

#ifdef __cplusplus
} // extern "C"
#endif
