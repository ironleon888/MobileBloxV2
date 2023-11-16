#pragma once

#include <lobject.h>
#include <lua.h>
#include <unordered_map>

namespace callcheck
{
    extern std::unordered_map<Closure*, lua_CFunction> cclosure_map;
    extern std::unordered_map<Closure*, Closure*> newcclosure_map;
    
    auto call_handler(lua_State* ls) -> int;
    auto cc_pushcclosure(lua_State* ls, lua_CFunction fn, const char* debugname, int nups) -> void;
}