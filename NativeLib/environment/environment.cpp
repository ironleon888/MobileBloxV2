#include "environment.hpp"

#include "../callcheck/callcheck.hpp"

auto exploit::environment::blank_function( lua_State* ls ) -> int
{
	return 0;
}

auto exploit::environment::registry_init( lua_State* ls ) -> void
{
	misc( ls );
	closure( ls );
	debug( ls );
	websocket( ls );
	reflection( ls );
}

static int libsize(const luaL_Reg* l)
{
    int size = 0;
    for (; l->name; l++)
        size++;
    return size;
}

auto exploit::environment::register_lib( lua_State* ls, const char* libname, const luaL_Reg* lib ) -> void
{
	if ( libname ) 
    {
    	int nfunc = libsize(lib);
    	lua_createtable(ls, 0, nfunc);
        
        for (; lib->name; lib++)
	    {
	        callcheck::cc_pushcclosure(ls, lib->func, lib->name, 0);
	        lua_setfield(ls, -2, lib->name);
	        
	        // libname.functioname - functionaddr - threadaddr
	        LOGD(" environment::register_lib -> %s.%s - %p - %p ", libname, lib->name, lib->func, ls);
	    }
	    
	    lua_setreadonly(ls, -1, true);  
	    lua_setfield(ls, -2, libname);
	}
	else if ( libname == nullptr )
	{
		for (; lib->name; lib++)
	    {
	        callcheck::cc_pushcclosure(ls, lib->func, lib->name, 0);
	        lua_setfield(ls, -2, lib->name);
	        
	        // GlobalsIndex.functioname - functionaddr - threadaddr
	        LOGD(" environment::register_lib -> GlobalsIndex.%s - %p - %p ", lib->name, lib->func, ls);
	    }
	}
}