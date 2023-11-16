#include "../environment.hpp"

#include <lua.h>
#include <lualib.h>
#include "../../reflection/instance.hpp"
#include "../../utils/utils.hpp"
#include "../../roblox/update.hpp"

static int gethui(lua_State* ls)
{
	LOGD(" LuauEnvCall -> gethui - CallingThread -> %p", ls);
	
	lua_getglobal(ls, "game");
    lua_getfield(ls, -1, "GetService");
    lua_pushvalue(ls, -2);
    lua_pushstring(ls, "CoreGui");
    lua_call(ls, 2, 1);
    return 1;
}

static int getproperties(lua_State* ls)
{
	LOGD(" LuauEnvCall -> getproperties - CallingThread -> %p", ls);
	
	luaL_checktype(ls, 1, LUA_TUSERDATA);
	
	const auto inst = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(ls, 1));
	const auto class_descriptor = *reinterpret_cast<std::uintptr_t*>(inst + 0xC);

	lua_newtable(ls);

	const auto start = *reinterpret_cast<std::uintptr_t*>(class_descriptor + 0x24);
	const auto end = *reinterpret_cast<std::uintptr_t*>(class_descriptor + 0x28);

	int iteration = 0u;

	for (auto i = start; i < end; i += 8)
	{
		const char* prop_name = *reinterpret_cast<const char**>(i);

		if (prop_name != nullptr && strlen(prop_name) > 0)
		{
			lua_pushinteger(ls, ++iteration);
			lua_pushstring(ls, prop_name);
			lua_settable(ls, -3);
		}
	}
	return 1;
}

static int setscriptable(lua_State* ls)
{
	LOGD(" LuauEnvCall -> setscriptable - CallingThread -> %p", ls);
	
	luaL_checktype(ls, 1, LUA_TUSERDATA);
    luaL_checktype(ls, 2, LUA_TSTRING);
    luaL_checktype(ls, 3, LUA_TBOOLEAN);
    
    int strhash;
    if ( !lua_tostringatom(ls, 2, &strhash) )
        luaL_argerror(ls, 2, "Invalid property name.");
    
    auto inst = reflection::RbxInstance{ *reinterpret_cast<std::uintptr_t*>(lua_touserdata(ls, 1)) };
    
    auto prop_table = utils::memory::rebase("libroblox.so", roblox::addresses::prop_table);
    auto ktablecode = *reinterpret_cast<std::uintptr_t*>(prop_table + 4 * strhash);

    if ( !ktablecode )
        luaL_argerror(ls, 1, "This property is not a member of Instance.");
    
    // bruh?
    auto prop_desc_ptr = inst.GetPropertyDescriptor( ktablecode );
    if (!prop_desc_ptr) luaL_argerror(ls, 2, "Bad Property.");
    auto prop_desc = *prop_desc_ptr;
    if (!prop_desc) luaL_argerror(ls, 2, "Bad Property.");

    auto scriptable = lua_toboolean(ls, 3);
    
    auto oldScriptable = *reinterpret_cast<std::uintptr_t*>(prop_desc + 32);
    *reinterpret_cast<std::uintptr_t*>(prop_desc + 32) = (scriptable << 5);
    
    lua_pushboolean(ls, (bool)((oldScriptable >> 5) & 1));
    return 1;
}

static int isscriptable(lua_State* ls)
{
	LOGD(" LuauEnvCall -> isscriptable - CallingThread -> %p", ls);
	
	luaL_checktype(ls, 1, LUA_TUSERDATA);
    luaL_checktype(ls, 2, LUA_TSTRING);
    
    int strhash;
    if ( !lua_tostringatom(ls, 2, &strhash) )
        luaL_argerror(ls, 2, "Invalid property name.");
    
    auto inst = reflection::RbxInstance{ *reinterpret_cast<std::uintptr_t*>(lua_touserdata(ls, 1)) };
    
    auto prop_table = utils::memory::rebase("libroblox.so", roblox::addresses::prop_table);
    auto ktablecode = *reinterpret_cast<std::uintptr_t*>(prop_table + 4 * strhash);

    if ( !ktablecode )
        luaL_argerror(ls, 1, "This property is not a member of Instance.");
    
    // bruh?
    auto prop_desc_ptr = inst.GetPropertyDescriptor( ktablecode );
    if (!prop_desc_ptr) luaL_argerror(ls, 2, "Bad Property.");
    auto prop_desc = *prop_desc_ptr;
    if (!prop_desc) luaL_argerror(ls, 2, "Bad Property.");
    
    auto scriptable = *reinterpret_cast<std::uintptr_t*>(prop_desc + 32);
    lua_pushboolean(ls, (bool)((scriptable >> 5) & 1));
    return 1;
}

static int gethiddenproperty(lua_State* ls)
{
	LOGD(" LuauEnvCall -> gethiddenproperty - CallingThread -> %p", ls);
	
	luaL_checktype(ls, 1, LUA_TUSERDATA);
    luaL_checktype(ls, 2, LUA_TSTRING);
    
    int strhash;
    auto prop_name = lua_tostringatom(ls, 2, &strhash);
    if ( !prop_name )
    {
        luaL_argerror(ls, 2, "Invalid property name.");
        return 0;
    }
    
    auto inst = reflection::RbxInstance{ *reinterpret_cast<std::uintptr_t*>(lua_touserdata(ls, 1)) };
    
    auto prop_table = utils::memory::rebase("libroblox.so", roblox::addresses::prop_table);
    auto ktablecode = *reinterpret_cast<std::uintptr_t*>(prop_table + 4 * strhash);

    if ( !ktablecode )
        luaL_argerror(ls, 1, "This property is not a member of Instance.");
    
    // bruh?
    auto prop_desc_ptr = inst.GetPropertyDescriptor( ktablecode );
    if (!prop_desc_ptr) luaL_argerror(ls, 2, "Bad Property.");
    auto prop_desc = *prop_desc_ptr;
    if (!prop_desc) luaL_argerror(ls, 2, "Bad Property.");
    
    auto scriptable = *reinterpret_cast<std::uintptr_t*>(prop_desc + 32);
    if ( (bool)((scriptable >> 5) & 1) )
    {
	    *reinterpret_cast<std::uintptr_t*>(prop_desc + 32) = (1 << 5);
	    lua_getfield(ls, 1, prop_name);
	    *reinterpret_cast<std::uintptr_t*>(prop_desc + 32) = scriptable;
    }
    else 
    {
    	lua_getfield(ls, 1, prop_name);
    }
    lua_pushboolean(ls, ((scriptable & 0x20) >> 5) ^ 1);
    return 2;
}

static int sethiddenproperty(lua_State* ls)
{
	LOGD(" LuauEnvCall -> sethiddenproperty - CallingThread -> %p", ls);
	
	luaL_checktype(ls, 1, LUA_TUSERDATA);
    luaL_checktype(ls, 2, LUA_TSTRING);
    luaL_checkany(ls, 3);
    
    int strhash;
    auto prop_name = lua_tostringatom(ls, 2, &strhash);
    if ( !prop_name )
    {
        luaL_argerror(ls, 2, "Invalid property name.");
        return 0;
    }
    
    auto inst = reflection::RbxInstance{ *reinterpret_cast<std::uintptr_t*>(lua_touserdata(ls, 1)) };
    
    auto prop_table = utils::memory::rebase("libroblox.so", roblox::addresses::prop_table);
    auto ktablecode = *reinterpret_cast<std::uintptr_t*>(prop_table + 4 * strhash);

    if ( !ktablecode )
        luaL_argerror(ls, 1, "This property is not a member of Instance.");
    
    // bruh?
    auto prop_desc_ptr = inst.GetPropertyDescriptor( ktablecode );
    if (!prop_desc_ptr) luaL_argerror(ls, 2, "Bad Property.");
    auto prop_desc = *prop_desc_ptr;
    if (!prop_desc) luaL_argerror(ls, 2, "Bad Property.");
    
    auto scriptable = *reinterpret_cast<std::uintptr_t*>(prop_desc + 32);
    if ( (bool)((scriptable >> 5) & 1) )
    {
	    *reinterpret_cast<std::uintptr_t*>(prop_desc + 32) = (1 << 5);
	    lua_pushvalue(ls, 3);
		lua_setfield(ls, 1, prop_name);
	    *reinterpret_cast<std::uintptr_t*>(prop_desc + 32) = scriptable;
    }
    else 
    {
    	lua_pushvalue(ls, 3);
		lua_setfield(ls, 1, prop_name);
    }
    lua_pushboolean(ls, ((scriptable & 0x20) >> 5) ^ 1);
    return 2;
}

static const luaL_Reg funcs[ ] = {
    {"gethui", gethui},
	{"get_hidden_ui", gethui},
	{"getproperties", getproperties},
	{"setscriptable", setscriptable},
	{"isscriptable", isscriptable},
	{"gethiddenproperty", gethiddenproperty},
	{"sethiddenproperty", sethiddenproperty},
    
    {nullptr, nullptr}
};

auto exploit::environment::reflection( lua_State* ls ) -> void
{
	lua_pushvalue(ls, LUA_GLOBALSINDEX);
	register_lib(ls, nullptr, funcs);
	lua_pop(ls, 1);
}