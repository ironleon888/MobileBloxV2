#include "../environment.hpp"

#include <lua.h>
#include "../../taskscheduler/taskscheduler.hpp"
#include <lualib.h>
#include <lmem.h>
#include <lgc.h>
#include "../../roblox/update.hpp"
#include "../../utils/utils.hpp"
#include "../../execution/execution.hpp"

static int getgenv(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getgenv - CallingThread -> %p", ls);
    
    lua_pushvalue(ls, LUA_GLOBALSINDEX);
    return 1;
}

// get mainthread global env
static int getmtgenv(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getmgenv - CallingThread -> %p", ls);
    
    lua_pushvalue(taskscheduler::get_singleton( )->get_ExploitState( ), LUA_GLOBALSINDEX);
    lua_xmove(taskscheduler::get_singleton( )->get_ExploitState( ), ls, 1);
    return 1;
}

static int getrenv(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getrenv - CallingThread -> %p", ls);
    
    lua_pushvalue(taskscheduler::get_singleton( )->get_mainstate( ), LUA_GLOBALSINDEX);
    lua_xmove(taskscheduler::get_singleton( )->get_mainstate( ), ls, 1);
    return 1;
}

static int getreg(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getreg - CallingThread -> %p", ls);
    
    lua_pushvalue(ls, LUA_REGISTRYINDEX);
    return 1;
}

static int gettenv(lua_State* ls)
{
    LOGD(" LuauEnvCall -> gettenv - CallingThread -> %p", ls);
    
    luaL_checktype(ls, 1, LUA_TTHREAD);

    auto thread = lua_tothread(ls, 1);

    lua_pushvalue(thread, LUA_GLOBALSINDEX);
    lua_xmove(thread, ls, 1);
    return 1;
}

static int getthread(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getthread - CallingThread -> %p", ls);
    
    lua_pushthread(ls);
    return 1;
}

static int identifyexecutor(lua_State* ls)
{
    LOGD(" LuauEnvCall -> identifyexecutor - CallingThread -> %p", ls);
    
    lua_pushstring(ls, exploit_configuration::exploit_name.c_str( ));
    lua_pushstring(ls, exploit_configuration::exploit_version.c_str( ));
    return 2;    
}

static int isLuau(lua_State* ls)
{
    LOGD(" LuauEnvCall -> isLuau - CallingThread -> %p", ls);
    
    lua_pushboolean(ls, true); 
    return 1; 
}

static int isrbxactive(lua_State* ls)
{
    LOGD(" LuauEnvCall -> isrbxactive - CallingThread -> %p", ls);
    
    lua_pushboolean(ls, true);
    return 1; 
}

static int setfpscap(lua_State* ls)
{
    LOGD(" LuauEnvCall -> setfpscap - CallingThread -> %p", ls);
    
    auto cap = luaL_checknumber(ls, 1);
    
    taskscheduler::get_singleton( )->set_fpscap(cap);
    return 0;
}

static int getfpscap(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getfpscap - CallingThread -> %p", ls);
    
    auto fps = taskscheduler::get_singleton( )->get_fpscap( );
    lua_pushnumber(ls, fps);
    return 1;
}

static int getgc(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getgc - CallingThread -> %p", ls);
    
    const auto tables = lua_gettop(ls) ? luaL_optboolean(ls, 1, 0) : true;
    lua_newtable(ls);

    struct gc_t
    {
        lua_State* ls;
        const int tables;
        size_t count;
    } gct{ ls, tables, 0 };

    luaM_visitgco(ls, &gct, [ ](void* context, lua_Page* page, GCObject* gco)
    {
        const auto gct = static_cast<gc_t*>(context);

        // DEADMASK
        if ( !((gco->gch.marked ^ WHITEBITS) & otherwhite(gct->ls->global)) )
            return false;

        const auto tt = gco->gch.tt;

        if (tt == LUA_TFUNCTION || tt == LUA_TUSERDATA || (gct->tables && tt == LUA_TTABLE))
        {
            gct->ls->top->value.gc = gco;
            gct->ls->top->tt = gco->gch.tt;
            gct->ls->top++;

            lua_rawseti(gct->ls, -2, ++gct->count);
        }

        return false;
    });

    return 1;
}

static int getidentity(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getidentity - CallingThread -> %p", ls);
    
    auto ES = static_cast<roblox::structs::ExtraSpace_t*>(lua_getthreaddata(ls));
    if (ES == nullptr) 
    {
        lua_pushinteger(ls, 0); // anonymous thread 
        return 1;
    }
    
    lua_pushinteger(ls, ES->context_level);
    return 1;
}

static int setidentity(lua_State* ls)
{
    LOGD(" LuauEnvCall -> setidentity - CallingThread -> %p", ls);
    
    auto identity = luaL_checkinteger(ls, 1);
    auto ES = static_cast<roblox::structs::ExtraSpace_t*>(lua_getthreaddata(ls));
    if (ES == nullptr) 
    {
        luaL_argerror(ls, 0, "Could not get the ExtraSpace of this thread, identity cannot be set.");
        return 0;
    }
    
    lua_pushinteger(ls, ES->context_level); // old identity
    roblox::functions::set_identity(ls, identity);
    return 1;
}

static int setclipboard(lua_State* ls)
{
    LOGD(" LuauEnvCall -> setclipboard - CallingThread -> %p", ls);
    
    auto text = luaL_checkstring(ls, 1);
    utils::JNI::set_clipboard_data(text);
    return 0;
}

static int mb_schedscript(lua_State* ls)
{
    auto script = luaL_checkstring(ls, 1);
    exploit::execution::get_singleton( )->schedule( script );
    return 0;
}

static const luaL_Reg funcs[ ] = {
    {"getgenv", getgenv},
    {"getmtgenv", getmtgenv},
    {"getrenv", getrenv},
    {"getreg", getreg},
    {"gettenv", gettenv},
    {"getthread", getthread},
    {"identifyexecutor", identifyexecutor},
    {"getexecutorname", identifyexecutor},
    {"isLuau", isLuau},
    {"isrbxactive", isrbxactive},
    {"iswindowactive", isrbxactive},
    {"isgameactive", isrbxactive},
    {"setfpscap", setfpscap},
    {"getfpscap", getfpscap},
    {"getgc", getgc},
    {"getidentity", getidentity},
    {"getthreadidentity", getidentity},
    {"getthreadcontext", getidentity},
    {"setidentity", setidentity},
    {"setthreadidentity", setidentity},
    {"setthreadcontext", setidentity},
    {"setclipboard", setclipboard},
    
    {"mb_schedscript", mb_schedscript},
    
    {nullptr, nullptr}
};

auto exploit::environment::misc( lua_State* ls ) -> void
{
    lua_pushvalue(ls, LUA_GLOBALSINDEX);
    register_lib(ls, nullptr, funcs);
    lua_pop(ls, 1);
}