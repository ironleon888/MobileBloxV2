#pragma once

#include <lua.h>
#include <lualib.h>

namespace exploit::environment
{
    auto blank_function(lua_State* ls) -> int;
	
	auto registry_init( lua_State* ls ) -> void;
	auto register_lib( lua_State* ls, const char* libname, const luaL_Reg* lib ) -> void;
	
	// Registry Functions for each library, prob wont do all of these because laziness
	auto misc( lua_State* ls ) -> void;
	auto closure( lua_State* ls ) -> void;
	auto cache( lua_State* ls ) -> void;
	auto metatable( lua_State* ls ) -> void;
	auto debug( lua_State* ls ) -> void;
	auto websocket( lua_State* ls ) -> void;
	auto reflection( lua_State* ls ) -> void;
	auto crypt( lua_State* ls ) -> void; // absolutely shit at this one
	auto bit( lua_State* ls ) -> void;
	auto connections( lua_State* ls ) -> void;
	auto script( lua_State* ls ) -> void;
}