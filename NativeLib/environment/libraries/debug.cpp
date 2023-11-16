#include "../environment.hpp"

#include <lua.h>
#include <lualib.h>
#include <lstate.h>
#include <lapi.h>
#include <lfunc.h>

#include "../../roblox/update.hpp"

static int GderefES(lua_State* ls)
{
	auto off = luaL_checkinteger(ls, 1);
	auto ES = reinterpret_cast<uintptr_t>(ls->userdata);
    if ( ES ) 
    {
		lua_pushinteger(ls, *reinterpret_cast<int*>(ES + off));
		return 1;
	}
	return 0;
}

static int SderefES(lua_State* ls)
{
	auto off = luaL_checkinteger(ls, 1);
	auto val = luaL_checkinteger(ls, 2);
	
	auto ES = reinterpret_cast<uintptr_t>(ls->userdata);
    if ( ES ) 
    {
		lua_pushinteger(ls, *reinterpret_cast<int*>(ES + off));
		*reinterpret_cast<int*>(ES + off) = val;
		return 1;
	}
	return 0;
}

static int GderefCx(lua_State* ls)
{
	auto off = luaL_checkinteger(ls, 1);
	auto context = reinterpret_cast<uintptr_t>(roblox::functions::rbx_getthreadcontext(ls));
    if ( context ) 
    {
		lua_pushinteger(ls, *reinterpret_cast<int*>(context + off));
		return 1;
	}
	return 0;
}

static int SderefCx(lua_State* ls)
{
	auto off = luaL_checkinteger(ls, 1);
	auto val = luaL_checkinteger(ls, 2);
	
	auto context = reinterpret_cast<uintptr_t>(roblox::functions::rbx_getthreadcontext(ls));
    if ( context ) 
    {
		lua_pushinteger(ls, *reinterpret_cast<int*>(context + off));
		*reinterpret_cast<int*>(context + off) = val;
		return 1;
	}
	return 0;
}

static int Faddr(lua_State* ls)
{
	auto val = lua_tocfunction(ls, 1);

	lua_pushinteger(ls, reinterpret_cast<uintptr_t>(val));
	return 1;
}

static int Oaddr(lua_State* ls)
{
	auto val = reinterpret_cast<std::uintptr_t>(lua_touserdata(ls, 1));

	lua_pushinteger(ls, val);
	lua_pushinteger(ls, *reinterpret_cast<uintptr_t*>(val));
	return 2;
}

static int RbxBase(lua_State* ls)
{
	lua_pushinteger(ls, utils::memory::find_lib("libroblox.so"));
	return 1;
}

static int debug_getregistry(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getregistry - CallingThread -> %p", ls);
	
	lua_pushvalue(ls, LUA_REGISTRYINDEX);
	return 1;
}

static int debug_getmetatable(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getmetatable - CallingThread -> %p", ls);
	
	luaL_checkany(ls, 1);
	
    if ( !lua_getmetatable(ls, 1) )
	    lua_pushnil( ls ); 
    return 1;
 }

static int debug_setmetatable(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_setmetatable - CallingThread -> %p", ls);
	
	luaL_checkany(ls, 1);
	
	auto t = lua_type(ls, 2);
    luaL_argcheck(ls, t == LUA_TNIL || t == LUA_TTABLE, 2, "nil or table expected");
    
    lua_settop(ls, 2);
    lua_setmetatable(ls, 1);
    return 1;
}

static int debug_getupvalues(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getupvalues - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) ) {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
	
	if (lua_isnumber(ls, 1)) {
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar)) {
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else {
		lua_pushvalue(ls, 1); // cl
	}
	
	if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
	auto cl = clvalue(luaA_toobject(ls, -1));
    if (cl->nupvalues)
    {
    	lua_createtable(ls, cl->nupvalues, 0); // t cl
        int i = 0;
        do
        {
            if (lua_getupvalue(ls, -2, ++i)) lua_rawseti(ls, -2, i);
        } while (i < cl->nupvalues);
    }
    else { lua_newtable(ls); }
    return 1;
}

static int debug_getupvalue(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getupvalue - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
	
	if (lua_isnumber(ls, 1))
    {
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
		lua_pushvalue(ls, 1); // cl
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    auto cl = clvalue(luaA_toobject(ls, -1));
    auto idx = luaL_checkinteger(ls, 2);
    
    if (cl->nupvalues < idx) luaL_argerror(ls, 2, "index out of range.");
    if (idx < 1) luaL_argerror(ls, 2, "index cannot be negative or 0.");
    
    if (!lua_getupvalue(ls, -1, idx)) lua_pushnil(ls);
    return 1;
}

static int debug_setupvalue(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_setupvalue - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
	
	if (lua_isnumber(ls, 1))
    {
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
		lua_pushvalue(ls, 1); // cl
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    auto cl = clvalue(luaA_toobject(ls, -1));
    auto idx = luaL_checkinteger(ls, 2);
    
    if (cl->nupvalues < idx) luaL_argerror(ls, 2, "index out of range.");
    if (idx < 1) luaL_argerror(ls, 2, "index cannot be negative or 0.");
    
    lua_pushvalue(ls, 3);
    lua_setupvalue(ls, -2, idx);
    return 1;
}

static int debug_getconstants(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getconstants - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
	
	if (lua_isnumber(ls, 1))
    {
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
		lua_pushvalue(ls, 1); // cl
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    auto cl = clvalue(luaA_toobject(ls, -1));;
    auto p = cl->l.p;
    
    int i = 0;
    lua_createtable(ls, p->sizek, 0);
    do {
    	TValue k;
        k = p->k[i];

        if (k.tt == LUA_TFUNCTION || k.tt == LUA_TTABLE){
            lua_pushnil(ls);
        }
        else {
            ls->top->value = k.value;
            ls->top->tt = k.tt;
            ls->top++;
        }

        lua_rawseti(ls, -2, i+1);
        i++;
    } while(i < p->sizek);
    return 1;
}

static int debug_getconstant(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getconstant - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
	
	if (lua_isnumber(ls, 1))
    {
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
		lua_pushvalue(ls, 1); // cl
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    auto cl = clvalue(luaA_toobject(ls, -1));;
    auto idx = luaL_checkinteger(ls, 2);
    auto p = cl->l.p;
    
    if (!p->sizek) luaL_argerror(ls, 1, "function does not have constants.");
    if (idx > p->sizek) luaL_argerror(ls, 2, "index out of range.");
    if (idx < 1) luaL_argerror(ls, 2, "index cannot be negative or 0.");
    
	TValue k;
    k = p->k[idx - 1];

    if (k.tt == LUA_TFUNCTION || k.tt == LUA_TTABLE){
        lua_pushnil(ls);
    }
    else {
        ls->top->value = k.value;
        ls->top->tt = k.tt;
        ls->top++;
    }
    return 1;
}

static int debug_setconstant(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_setconstant - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
	
	if (lua_isnumber(ls, 1))
    {
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
		lua_pushvalue(ls, 1); // cl
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    luaL_checkany(ls, 3);
    
    auto cl = clvalue(luaA_toobject(ls, -1));;
    auto idx = luaL_checkinteger(ls, 2);
    auto o = luaA_toobject(ls, 3);
    auto p = cl->l.p;
    
    if (!p->sizek) luaL_argerror(ls, 1, "function does not have constants.");
    if (idx > p->sizek) luaL_argerror(ls, 2, "index out of range.");
    if (idx < 1) luaL_argerror(ls, 2, "index cannot be negative or 0.");
    
	TValue* k = nullptr;
    k = &p->k[idx - 1];

    if (k->tt == LUA_TFUNCTION || k->tt == LUA_TTABLE){
        return 0;
    }
    else {
        k->value = o->value;
        k->tt = o->tt;
    }
    return 0;
}

static int debug_getprotos(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getprotos - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
	
	if (lua_isnumber(ls, 1))
    {
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
		lua_pushvalue(ls, 1); // cl
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    auto cl = clvalue(luaA_toobject(ls, -1));;
    auto p = cl->l.p;
    
    int i = 1;
	lua_createtable(ls, p->sizep, 0);
    do{
    	Proto* proto = p->p[i - 1];
        Closure* pcl = luaF_newLclosure(ls, proto->nups, ls->gt, proto);

        ls->top->tt = LUA_TFUNCTION;
        ls->top->value.p = pcl;
        ls->top++;

        lua_rawseti(ls, -2, i);
    	i++;
    }
    while (i < p->sizep);
    return 1;
}

static int debug_getproto(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getproto - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
	
	if (lua_isnumber(ls, 1))
    {
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
		lua_pushvalue(ls, 1); // cl
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    auto cl = clvalue(luaA_toobject(ls, -1));;
    auto p = cl->l.p;
    int idx = luaL_checkinteger(ls, 2);
    
    if (!p->sizep) luaL_argerror(ls, 1, "Function does not have protos.");
    if (idx > p->sizep) luaL_argerror(ls, 2, "index out of range.");
    if (idx < 1) luaL_argerror(ls, 2, "index cannot be negative or 0.");
    
	Proto* proto = p->p[idx - 1];
    Closure* pcl = luaF_newLclosure(ls, proto->nups, ls->gt, proto);

    if (lua_isboolean(ls, 3) && lua_toboolean(ls, 3) == true){
      // todo: actually scan gc
      lua_createtable(ls, 1, 0);
      
      ls->top->tt = LUA_TFUNCTION;
      ls->top->value.p = pcl;
      ls->top++;
      
      lua_rawseti(ls, -2, 1);
    }
    else {
        ls->top->tt = LUA_TFUNCTION;
        ls->top->value.p = pcl;
        ls->top++;
    }
    return 1;
}

static int debug_getstack(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getstack - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
    
    auto level = 0;
	if (lua_isnumber(ls, 1))
    {
    	level = lua_tointeger(ls, 1);
    
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
    	level = -lua_gettop(ls);
        
		lua_pushvalue(ls, 1);
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    auto ci = ls->ci[-level];
    
    if (lua_isnumber(ls, 2))
    {
        auto idx = lua_tointeger(ls, 2) - 1;
        if (idx > (ci.top - ci.base) || idx < 0)
        {
            luaL_argerror(ls,2, "Invalid stack index.");
            return 0;
        }

        auto v = ci.base + level;
        ls->top->value = v->value;
        ls->top->tt = v->tt;
        ls->top++;
    }
    else 
    {
        int idx = 0;
        lua_newtable(ls);
        for (auto i = ci.base; i < ci.top; i++){
            lua_pushinteger(ls, idx++ + 1);
            
            ls->top->value = i->value;
            ls->top->tt = i->tt;
            ls->top++;
            
            lua_settable(ls, -3);
        }
    }
    return 1;
}

static int debug_setstack(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_setstack - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
    
    auto level = 0;
	if (lua_isnumber(ls, 1))
    {
    	level = lua_tointeger(ls, 1);
    
		lua_Debug ar;

		if (!lua_getinfo(ls, lua_tonumber(ls, 1), "f", &ar))
		{
			luaL_argerror(ls, 1, "level out of range");
			return 0;
		}
	}
	else
    {
    	level = -lua_gettop(ls);
        
		lua_pushvalue(ls, 1);
	}
    
    if (lua_iscfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "Expected Lua Closure");
        return 0;
    }
    
    luaL_checkany(ls, 3);
    
    auto ci = ls->ci[-level];
    
    auto idx = luaL_checkinteger(ls, 2) - 1;
    if (idx > (ci.top - ci.base) || idx < 0) 
    {
        luaL_argerror(ls, 2, "Invalid stack index.");
        return 0;
    }

    auto o = luaA_toobject(ls, 3);
    
    (ci.base + idx)->value = o->value;
    (ci.base + idx)->tt = o->tt;
    return 0;
}

static int debug_getinfo(lua_State* ls)
{
	LOGD(" LuauEnvCall -> debug_getinfo - CallingThread -> %p", ls);
	
	if ( !lua_isfunction(ls, 1) && !lua_isnumber(ls, 1) )
    {
		luaL_typeerror(ls, 1, "function or number");
		return 0;
	}
    
    auto level = 0;
	if (lua_isnumber(ls, 1))
    {
    	level = lua_tointeger(ls, 1);
	}
	else
    {
    	level = -lua_gettop(ls);
	}
    
    auto desc = lua_isstring(ls, 2) ? lua_tostring(ls, 2) : "sluanf";
    lua_Debug ar;

	if (!lua_getinfo(ls, level, desc, &ar))
	{
		luaL_argerror(ls, 1, "level out of range");
		return 0;
	}
	
	if (!lua_isfunction(ls, -1))
    {
        luaL_argerror(ls, 1, "stack does not point to a function.");
        return 0;
    }
    
    lua_newtable(ls);
    {
        if (std::strchr(desc, 's'))
        {
            lua_pushstring(ls, ar.source);
            lua_setfield(ls, -2, "source");

            lua_pushstring(ls, ar.short_src);
            lua_setfield(ls, -2, "short_src");

            lua_pushstring(ls, ar.what);
            lua_setfield(ls, -2, "what");

            lua_pushinteger(ls, ar.linedefined);
            lua_setfield(ls, -2, "linedefined");
        }

        if (std::strchr(desc, 'l'))
        {
            lua_pushinteger(ls, ar.currentline);
            lua_setfield(ls, -2, "currentline");
        }

        if (std::strchr(desc, 'u'))
        {
            lua_pushinteger(ls, ar.nupvals);
            lua_setfield(ls, -2, "nups");
        }

        if (std::strchr(desc, 'a'))
        {
            lua_pushinteger(ls, ar.isvararg);
            lua_setfield(ls, -2, "is_vararg");

            lua_pushinteger(ls, ar.nparams);
            lua_setfield(ls, -2, "numparams");
        }

        if (std::strchr(desc, 'n'))
        {
            lua_pushstring(ls, ar.name);
            lua_setfield(ls, -2, "name");
        }

        if (std::strchr(desc, 'f'))
        {
            lua_pushvalue(ls,-2);
            lua_remove(ls,-3);
            lua_setfield(ls, -2, "func");
        }
    }
    return 1;
}

static const luaL_Reg funcs[ ] = {
    // debugging utils
    {"GderefES", GderefES},
    {"SderefES", SderefES},
    {"GderefCx", GderefCx},
    {"SderefCx", SderefCx},
    {"Faddr", Faddr},
    {"Oaddr", Oaddr},
    {"RbxBase", RbxBase},
    
    {"getregistry", debug_getregistry},
    {"getmetatable", debug_getmetatable},
    {"setmetatable", debug_setmetatable},
    
    {"getupvalues", debug_getupvalues},
    {"getupvalue", debug_getupvalue},
    {"setupvalue", debug_setupvalue},
    
    {"getconstants", debug_getconstants},
    {"getconstant", debug_getconstant},
    {"setconstant", debug_setconstant},
    
    {"getprotos", debug_getprotos},
    {"getproto", debug_getproto},
    
    {"getstack", debug_getstack},
    {"setstack", debug_setstack},
    
    {"getinfo", debug_getinfo},
    
    {nullptr, nullptr}
};

auto exploit::environment::debug( lua_State* ls ) -> void
{
	lua_pushvalue(ls, LUA_GLOBALSINDEX);
	register_lib(ls, "debug", funcs);
	lua_pop(ls, 1);
}