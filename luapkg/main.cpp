#include <assert.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include "mypkg.h"
#include "lua.hpp"

void test_msg(lua_State *L)
{
    CMyPkg pkg;
    pkg["lua_typename"] = "Returns the name of the type encoded by the value tp, which must be one the values returned by lua_type. ";
    pkg["lua_pushnil"] = "Pushes a nil value onto the stack.";
    CMyPkg pkg2;
    pkg2.appendVal("lua_pushstring");
    pkg2.appendVal("lua_pushthread");
    pkg2.appendVal("lua_pushnumber");
    pkg["pkg"] = pkg2;
    pkg.print("origin pkg content");

    CMyBuffer buf;
    pkg.pack(buf);

    lua_getglobal(L, "test");    // 获取lua函数
    lua_pushlstring(L, buf.c_str(), buf.size());        // 压入参数
    if (lua_pcall(L, 1, 0, 0) != 0) {
        printf("got an exception: %s", lua_tostring(L, -1));
    }
}

void test_msg2(lua_State *L)
{
    lua_getglobal(L, "test2");    // 获取lua函数
    if (lua_pcall(L, 0, 1, 0) != 0) {
        printf("got an exception: %s", lua_tostring(L, -1));
        return;
    }

    size_t len=0;
    const char *data = lua_tolstring(L, -1, &len);
    printf("data len:%lu\n", len);

    CMyPkg pkg;
    pkg.unpack(data, len);
    lua_pop(L, 1);

    pkg.print("test_msg2");
}

int main()
{
    const char *luafile = "test.lua";
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_dofile(L, luafile))
    {
        printf("Load file: %s Failed: %s.\n",
                luafile, lua_tostring(L, -1));
        return -1;
    }

    test_msg(L);
    test_msg2(L);

    return 0;
}
