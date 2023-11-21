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
    
    static auto prop_table = utils::memory::rebase("libroblox.so", roblox::addresses::prop_table);
    auto ktablecode = *reinterpret_cast<std::uintptr_t*>(prop_table + 4 * strhash);

    if ( !ktablecode )
        luaL_argerror(ls, 1, "This property is not a member of Instance.");
    
    // bruh?
    auto prop_desc_ptr = inst.GetPropertyDescriptor( ktablecode );
    if (!prop_desc_ptr) luaL_argerror(ls, 2, "Bad Property.");
    auto prop_desc = *prop_desc_ptr;
    if (!prop_desc) luaL_argerror(ls, 2, "Bad Property.");

    auto scriptable = lua_toboolean(ls, 3);
    
    auto oldScriptable = *reinterpret_cast<std::uintptr_t*>(prop_desc + 40);
    *reinterpret_cast<std::uintptr_t*>(prop_desc + 40) = (scriptable << 5);
    
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
    
    static auto prop_table = utils::memory::rebase("libroblox.so", roblox::addresses::prop_table);
    auto ktablecode = *reinterpret_cast<std::uintptr_t*>(prop_table + 4 * strhash);

    if ( !ktablecode )
        luaL_argerror(ls, 1, "This property is not a member of Instance.");
    
    // bruh?
    auto prop_desc_ptr = inst.GetPropertyDescriptor( ktablecode );
    if (!prop_desc_ptr) luaL_argerror(ls, 2, "Bad Property.");
    auto prop_desc = *prop_desc_ptr;
    if (!prop_desc) luaL_argerror(ls, 2, "Bad Property.");
    
    auto scriptable = *reinterpret_cast<std::uintptr_t*>(prop_desc + 40);
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
    
    static auto prop_table = utils::memory::rebase("libroblox.so", roblox::addresses::prop_table);
    auto ktablecode = *reinterpret_cast<std::uintptr_t*>(prop_table + 4 * strhash);

    if ( !ktablecode )
        luaL_argerror(ls, 1, "This property is not a member of Instance.");
    
    // bruh?
    auto prop_desc_ptr = inst.GetPropertyDescriptor( ktablecode );
    if (!prop_desc_ptr) luaL_argerror(ls, 2, "Bad Property.");
    auto prop_desc = *prop_desc_ptr;
    if (!prop_desc) luaL_argerror(ls, 2, "Bad Property.");
    
    auto scriptable = *reinterpret_cast<std::uintptr_t*>(prop_desc + 40);
    if ( (bool)((scriptable >> 5) & 1) )
    {
        *reinterpret_cast<std::uintptr_t*>(prop_desc + 40) = (1 << 5);
        lua_getfield(ls, 1, prop_name);
        *reinterpret_cast<std::uintptr_t*>(prop_desc + 40) = scriptable;
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
    
    static auto prop_table = utils::memory::rebase("libroblox.so", roblox::addresses::prop_table);
    auto ktablecode = *reinterpret_cast<std::uintptr_t*>(prop_table + 4 * strhash);

    if ( !ktablecode )
        luaL_argerror(ls, 1, "This property is not a member of Instance.");
    
    // bruh?
    auto prop_desc_ptr = inst.GetPropertyDescriptor( ktablecode );
    if (!prop_desc_ptr) luaL_argerror(ls, 2, "Bad Property.");
    auto prop_desc = *prop_desc_ptr;
    if (!prop_desc) luaL_argerror(ls, 2, "Bad Property.");
    
    auto scriptable = *reinterpret_cast<std::uintptr_t*>(prop_desc + 40);
    if ( (bool)((scriptable >> 5) & 1) )
    {
        *reinterpret_cast<std::uintptr_t*>(prop_desc + 40) = (1 << 5);
        lua_pushvalue(ls, 3);
        lua_setfield(ls, 1, prop_name);
        *reinterpret_cast<std::uintptr_t*>(prop_desc + 40) = scriptable;
    }
    else 
    {
        lua_pushvalue(ls, 3);
        lua_setfield(ls, 1, prop_name);
    }
    lua_pushboolean(ls, ((scriptable & 0x20) >> 5) ^ 1);
    return 2;
}

static int getinstances(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getinstances - CallingThread -> %p", ls);
    
    lua_newtable(ls);
    lua_pushlightuserdata(ls, (void*)roblox::addresses::pushinstance_registry_rebased);
    lua_rawget(ls, -10000);
    if (lua_istable(ls, -1))
    {
        lua_pushnil(ls);
        if ( lua_next(ls, -2) )
        {
            auto idx = 1;
            do
                lua_rawseti(ls, -4, idx++);
            while ( lua_next(ls, -2) );
        }
    }
    lua_pop(ls, 1);
    return 1;
}

static int getnilinstances(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getnilinstances - CallingThread -> %p", ls);
    
    lua_newtable(ls);
    lua_pushlightuserdata(ls, (void*)roblox::addresses::pushinstance_registry_rebased);
    lua_rawget(ls, -10000);
    if (lua_istable(ls, -1))
    {
        lua_pushnil(ls);
        auto idx = 1;
        while ( lua_next(ls, -2) )
        {
            lua_getfield(ls, -1, "Parent");
			if (lua_isnoneornil(ls, -1)) {
				lua_pop(ls, 1);
				lua_rawseti(ls, -4, idx++);
			}
			else {
				lua_pop(ls, 2);// Parent and value
            }
        }
    }
    lua_pop(ls, 1);
    return 1;
}

static int fireclickdetector(lua_State* ls)
{
    LOGD(" LuauEnvCall -> fireclickdetector - CallingThread -> %p", ls);
    
    luaL_checktype(ls, 1, LUA_TUSERDATA);
    
	const auto detector = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(ls, 1));
	const auto distance = static_cast<float>(luaL_optnumber(ls, 2, 0));
	
	lua_getglobal(ls, "game");
	lua_getfield(ls, -1, "GetService");
	lua_pushvalue(ls, -2);
	lua_pushstring(ls, "Players");
	lua_pcall(ls, 2, 1, 0);
	lua_getfield(ls, -1, "LocalPlayer");
	
	const auto player = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(ls, -1));
	lua_pop(ls, 3);
	
	roblox::functions::fireclickdetector(detector, distance, player);
    return 0;
}

static int fireproximityprompt(lua_State* ls)
{
    LOGD(" LuauEnvCall -> fireproximityprompt - CallingThread -> %p", ls);
    
    const auto prompt = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(ls, 1));
	roblox::functions::fireproximityprompt(prompt);
	return 0;
}

static int firetouchinterest(lua_State* ls)
{
    LOGD(" LuauEnvCall -> firetouchinterest - CallingThread -> %p", ls);
    
    luaL_checktype(ls, 1, LUA_TUSERDATA);
	luaL_checktype(ls, 2, LUA_TUSERDATA);
	luaL_checktype(ls, 3, LUA_TNUMBER);
	
	auto p1 = *reinterpret_cast<uintptr_t*>(lua_touserdata(ls, 1));
	auto p2 = *reinterpret_cast<uintptr_t*>(lua_touserdata(ls, 2));
	auto untouch = luaL_optnumber(ls, 3, 0);
	
	auto to_touch = *reinterpret_cast<std::uintptr_t*>(p1 + 184);
	auto transmitter = *reinterpret_cast<std::uintptr_t*>(p2 + 184);
	auto world = *reinterpret_cast<std::uintptr_t*>(to_touch + 376);
	
	// might wanna do more reversing on the last argument. 
    roblox::functions::firetouchinterest(world, to_touch, transmitter, untouch, 0);
    return 0;
}

static const luaL_Reg funcs[ ] = {
    {"gethui", gethui},
    {"get_hidden_ui", gethui},
    {"getproperties", getproperties},
/*    {"setscriptable", setscriptable}, // needs fixing.
    {"isscriptable", isscriptable},
    {"gethiddenproperty", gethiddenproperty},
    {"sethiddenproperty", sethiddenproperty}, */
    {"getinstances", getinstances},
    {"getnilinstances", getnilinstances},
    {"fireclickdetector", fireclickdetector},
    {"fireproximityprompt", fireproximityprompt},
    {"firetouchinterest", firetouchinterest},
    
    {nullptr, nullptr}
};

auto exploit::environment::reflection( lua_State* ls ) -> void
{
    lua_pushvalue(ls, LUA_GLOBALSINDEX);
    register_lib(ls, nullptr, funcs);
    lua_pop(ls, 1);
}