local cmsgpack = require "cmsgpack"

function printtable(t)
    for k,v in pairs(t) do
        print(k, v)
        if type(v) == "table" then
            printtable(v)
        end
    end
end

function test(msg)
    print("msg len:", #msg)
    local t = cmsgpack.unpack(msg)
    printtable(t)
end

function test2()
    local t = {
        lua_pushvfstring = "Equivalent to lua_pushfstring, except that it receives a va_list instead of a variable number of arguments.",
        lua_rawequal = "Returns 1 if the two values in indices index1 and index2 are primitively equal (that is, without calling metamethods). Otherwise returns 0. Also returns 0 if any of the indices are non valid. ",
    }
    return cmsgpack.pack(t)
end
