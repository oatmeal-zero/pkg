#include "mypkg.h"
#include "lua.hpp"

void 
pkg2lua(CMyPkg& pkg, lua_State *L)
{
    lua_newtable(L);

    // array
    /*
    int size = pkg.getArraySize();
    for (int idx = 0; idx < size; idx++)
    {
        auto element = pkg.getVal(idx);
        switch (element.type()) {
        }
    }
    */

    // dict
    for (auto iter = pkg.begin(); iter != pkg.end(); ++iter) 
    {
        switch (iter.type()) {
            case MP_T_INTEGER:
                {
                    integer_t val = iter.getVal();
                    lua_pushstring(L, iter.key());
                    lua_pushinteger(L, val);
                    lua_settable(L, -3);
                    break;
                }
            case MP_T_NUMBER:
                {
                    number_t val = iter.getVal();
                    lua_pushstring(L, iter.key());
                    lua_pushnumber(L, val);
                    lua_settable(L, -3);
                    break;
                }
            case MP_T_STRING:
                {
                    size_t len;
                    const char* val = iter.getString(len);
                    lua_pushstring(L, iter.key());
                    lua_pushlstring(L, val, len);
                    lua_settable(L, -3);
                    break;
                }
            case MP_T_OBJECT:
                {
                    CMyPkg tmp = iter.getVal();
                    lua_pushstring(L, iter.key());
                    pkg2lua(tmp, L);
                    lua_settable(L, -3);
                    break;
                }
            default:
                printf("undefine type:%d\n", iter.type());
        }
    }

}

void
lua2pkg(lua_State *L, CMyPkg& pkg)
{
}

