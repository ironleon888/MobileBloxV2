#include "../environment.hpp"

#include <lua.h>
#include <lualib.h>
#include <lapi.h>
#include "../../execution/execution.hpp"

static std::uintptr_t game_userdata;

// could add safety measures
lua_CFunction index_orig = 0x0;
static int index_hook(lua_State* ls)
{
    auto ES = static_cast<roblox::structs::ExtraSpace_t*>(lua_getthreaddata(ls));
    if (ES->context_level > 5)
    {
        auto self = reinterpret_cast<std::uintptr_t>(lua_touserdata(ls, 1));
        auto method = lua_tostring(ls, 2);
        
        if (self == game_userdata)
        {
            if (strcmp(method, "HttpGet") == 0 || strcmp(method, "HttpGetAsync") == 0)
            {
                lua_pushnil(ls);
                return 1;
            }
            
            if (strcmp(method, "HttpPost") == 0 || strcmp(method, "HttpPostAsync") == 0)
            {
                // push HttpPost
            }
            
            if (strcmp(method, "GetObjects") == 0)
            {
                // push GetObjects
            }
        }
    }
    
    return index_orig(ls);
}

lua_CFunction namecall_orig = 0x0;
static int namecall_hook(lua_State* ls)
{
    auto ES = static_cast<roblox::structs::ExtraSpace_t*>(lua_getthreaddata(ls));
    if (ES->context_level > 5)
    {
        auto self = reinterpret_cast<std::uintptr_t>(lua_touserdata(ls, 1));
        auto method = lua_namecallatom(ls, nullptr);
        
        if (self == game_userdata)
        {
            if (strcmp(method, "HttpGet") == 0 || strcmp(method, "HttpGetAsync") == 0)
            {
                // call HttpGet
            }
            
            if (strcmp(method, "HttpPost") == 0 || strcmp(method, "HttpPostAsync") == 0)
            {
                // push HttpPost
            }
            
            if (strcmp(method, "GetObjects") == 0)
            {
                // call GetObjects
            }
        }
    }
    
    return namecall_orig(ls);
}

auto exploit::environment::mthooks( lua_State* ls ) -> void
{
    lua_getglobal(ls, "game");
    game_userdata = reinterpret_cast<std::uintptr_t>(lua_touserdata(ls, -1));
    lua_getmetatable(ls, -1);
    
    lua_rawgetfield(ls, -1, "__index");
	const auto index = clvalue(luaA_toobject(ls, -1));
	index_orig = index->c.f;
	lua_pop(ls, 1);

	lua_rawgetfield(ls, -1, "__namecall");
	const auto namecall = clvalue(luaA_toobject(ls, -1));
	namecall_orig = namecall->c.f;
	lua_pop(ls, 1);

	index->c.f = index_hook;
	namecall->c.f = namecall_hook;

	lua_pop(ls, 2);
}