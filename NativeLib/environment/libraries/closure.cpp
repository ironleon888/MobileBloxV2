#include "../environment.hpp"

#include <lua.h>
#include "../../callcheck/callcheck.hpp"
#include <lualib.h>
#include <lstate.h>
#include <Luau/Compiler.h> 
#include <lobject.h>
#include <lapi.h>
#include <lfunc.h>
#include <lgc.h>

#include "../../roblox/update.hpp"
#include <unordered_set>

static int newcclosure_handler(lua_State* ls)
{
    const auto nargs = lua_gettop(ls);

    // get this cl as identifier and get it's linked lclsoure
    void* LClosure = reinterpret_cast<void*>(callcheck::newcclosure_map.find(clvalue(ls->ci->func))->second);

    if (LClosure == nullptr) {
        return 0;
    }    

    ls->top->value.p = LClosure;
    ls->top->tt = LUA_TFUNCTION;
    ls->top++;
    
    lua_insert(ls, 1); // inserts cl above args
    
    const auto res = lua_pcall(ls, nargs, LUA_MULTRET, 0);
    if ( res && res != LUA_YIELD && !std::strcmp(lua_tostring(ls, -1), "attempt to yield across metamethod/C-call boundary") )
    {
        return lua_yield(ls, 0);
    }
    
    return lua_gettop(ls);
}

static int newcclosure(lua_State* ls)
{
    LOGD(" LuauEnvCall -> newcclosure - CallingThread -> %p", ls);
    
    luaL_checktype(ls, 1, LUA_TFUNCTION);
    
    if ( lua_iscfunction(ls, 1) ) { return 1; }
    
    lua_ref(ls, 1);
    lua_pushcclosure(ls, newcclosure_handler, 0, 0);
    callcheck::newcclosure_map[ clvalue(luaA_toobject(ls, -1)) ] = clvalue(luaA_toobject(ls, 1));
    
    return 1;
}

std::unordered_set<std::string> chunknamesbruh;
static int loadstring(lua_State* ls)
{
    LOGD(" LuauEnvCall -> loadstring - CallingThread -> %p", ls);
    
    auto str = luaL_checkstring(ls, 1);
    std::string chunkname = lua_isstring(ls, 2) ? lua_tostring(ls, 2) : utils::random_str(4);
    
    auto bytecode = Luau::compile(str, { 2, 1, 2 }, { true, true });
    if ( luau_load(ls, chunkname.c_str( ), bytecode.c_str( ), bytecode.size( ), 0) ) 
    {
        lua_pushnil(ls);
        lua_pushstring(ls, lua_tostring(ls, -2));
        return 2;
    }
    
    chunknamesbruh.insert(std::string(chunkname)); 
    return 1;
}

static int islclosure(lua_State* ls)
{
    LOGD(" LuauEnvCall -> islclosure - CallingThread -> %p", ls);
    
    lua_pushboolean(ls, lua_isLfunction(ls, 1));
    return 1; 
}

static int iscclosure(lua_State* ls)
{
    LOGD(" LuauEnvCall -> iscclosure - CallingThread -> %p", ls);
    
    lua_pushboolean(ls, lua_iscfunction(ls, 1));
    return 1; 
}

static int checkcaller(lua_State* ls)
{
    LOGD(" LuauEnvCall -> checkcaller - CallingThread -> %p", ls);
    
    auto ES = static_cast<roblox::structs::ExtraSpace_t*>(lua_getthreaddata(ls));
    if (ES == nullptr){
        lua_pushboolean(ls, false);
        return 1;
    }
    
    // maybe check if script is 0?
    lua_pushboolean(ls, (ES->context_level > 5));
    return 1;
}

static int getcallingscript(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getcallingscript - CallingThread -> %p", ls);
    
    auto ES = static_cast<roblox::structs::ExtraSpace_t*>(lua_getthreaddata(ls));
    if (ES == nullptr) {
        lua_pushnil(ls);
        return 1;
    }
    
    auto caller = ES->script;
     
    if ( !caller.expired( ) ) roblox::functions::rlua_pushinstanceSP(ls, caller.lock( ));
    else lua_pushnil(ls);
    return 1;
}

static int getscriptfromthread(lua_State* ls)
{
    LOGD(" LuauEnvCall -> getscriptfromthread - CallingThread -> %p", ls);
    
    auto th = lua_tothread(ls, 1);
    auto ES = static_cast<roblox::structs::ExtraSpace_t*>(lua_getthreaddata(th));
    if (ES == nullptr) {
        lua_pushnil(ls);
        return 1;
    }
    
    auto caller = ES->script;
    
    if ( !caller.expired( ) ) roblox::functions::rlua_pushinstanceSP(ls, caller.lock( ));
    else lua_pushnil(ls);
    return 1;
}

static int clonefunction(lua_State* ls)
{
    LOGD(" LuauEnvCall -> clonefunction - CallingThread -> %p", ls);
    
    luaL_checktype(ls, 1, LUA_TFUNCTION);
    
    auto cl = clvalue(luaA_toobject(ls, 1));
    
    // ncc ?
    if ( lua_isLfunction(ls,1) )
    {
        lua_clonefunction(ls, 1);
    }
    else if ( lua_iscfunction(ls,1) )
    {
        Closure* newcl = luaF_newCclosure(ls, cl->nupvalues, cl->env);
        newcl->c.f = (lua_CFunction)cl->c.f;
        newcl->c.cont = (lua_Continuation)cl->c.cont;
        newcl->c.debugname = (const char*)cl->c.debugname;
        
        for (int i = 0; i < cl->nupvalues; i++)
            setobj2n(ls, &newcl->c.upvals[i], &cl->c.upvals[i]);
        
        setclvalue(ls, ls->top, newcl);
        ls->top++;
    }
    
    return 1;
}

auto isnewcc(Closure* cl) -> bool
{
    if (cl->isC) {
        if (cl->c.f == newcclosure_handler) return true;
    }
    return false;
}

// TODO: Make a separate file to store custom LuauApi funcs
// TODO: So this doesn't end up like this
static int hookfunction(lua_State* ls)
{
    LOGD(" LuauEnvCall -> hookfunction - CallingThread -> %p", ls);
    
    luaL_checktype(ls, 1, LUA_TFUNCTION);
    luaL_checktype(ls, 2, LUA_TFUNCTION);
    
    auto src = clvalue(luaA_toobject(ls, 1));
    auto dst = clvalue(luaA_toobject(ls, 2));
    
    auto src_nups = src->nupvalues;
    auto dst_nups = dst->nupvalues;
    
    lua_pushvalue(ls, 2);
    lua_setfield(ls, LUA_REGISTRYINDEX, utils::random_str(6).c_str( ));
    
    if ( isnewcc(src) && isnewcc(dst) ) // NCClosure | NCClosure
    {
        // i could switch LClosures or make new ones.
        auto SrcLClosure = callcheck::newcclosure_map.find(src)->second;
        auto DstLClosure = callcheck::newcclosure_map.find(dst)->second;
        
        if (SrcLClosure == nullptr)
        {
            luaL_argerror(ls, 1, "NCCHook: Bad newcclosure_handler passed. Linked Lua Closure not found.");
            return 0;
        }
        
        if (DstLClosure == nullptr)
        {
            luaL_argerror(ls, 2, "NCCHook: Bad newcclosure_handler passed. Linked Lua Closure not found.");
            return 0;
        }
        
        // old closure
        // make a new ncc for the old one.
        lua_pushcclosure(ls, newcclosure_handler, 0, 0);
        callcheck::newcclosure_map[ clvalue(luaA_toobject(ls, -1)) ] = SrcLClosure;
        
        // hook
        // Like a normal hook but changing the Linked LClosures
        callcheck::newcclosure_map[ src ] = DstLClosure;
        
        return 1;
    }
    else if ( isnewcc(src) && lua_isLfunction(ls, 2) ) // NCClosure | LClosure
    {
        auto SrcLClosure = callcheck::newcclosure_map.find(src)->second;
        
        if (SrcLClosure == nullptr)
        {
            luaL_argerror(ls, 1, "NCCHook: Bad newcclosure_handler passed. Linked Lua Closure not found.");
            return 0;
        }
        
        // old closure
        // make a new ncc for the old one.
        lua_pushcclosure(ls, newcclosure_handler, 0, 0);
        callcheck::newcclosure_map[ clvalue(luaA_toobject(ls, -1)) ] = SrcLClosure;
        
        // hook
        // Like a normal hook but changing the Linked LClosures
        lua_ref(ls, 2);
        callcheck::newcclosure_map[ src ] = clvalue(luaA_toobject(ls, 2));
        
        return 1;
    }
    else if ( lua_iscfunction(ls, 1) && isnewcc(dst) ) // CClosure | NCClosure
    {
        auto DstLClosure = callcheck::newcclosure_map.find(dst)->second;
        
        if (DstLClosure == nullptr)
        {
            luaL_argerror(ls, 2, "NCCHook: Bad newcclosure_handler passed. Linked Lua Closure not found.");
            return 0;
        }
        
        // old closure
        Closure* old_cl = luaF_newCclosure(ls, src->nupvalues, src->env);
        old_cl->c.f = (lua_CFunction)src->c.f;
        old_cl->c.cont = (lua_Continuation)src->c.cont;
        old_cl->c.debugname = (const char*)src->c.debugname;
        
        for (int i = 0; i < src->nupvalues; i++)
            setobj2n(ls, &old_cl->c.upvals[i], &src->c.upvals[i]);
        
        setclvalue(ls, ls->top, old_cl);
        ls->top++;
        
        // hook
        // new ncc
        src->c.f = exploit::environment::blank_function;
        for (int i = 0; i < dst->nupvalues; i++)
            setobj2n(ls, &src->c.upvals[i], &dst->c.upvals[i]); // prob 0
        
        src->nupvalues = dst->nupvalues; // prob 0
        src->c.f = newcclosure_handler;
        callcheck::newcclosure_map[ clvalue(luaA_toobject(ls, 1)) ] = DstLClosure;
         
        return 1;
    }
    if ( lua_iscfunction(ls, 1) && lua_iscfunction(ls, 2) ) // CClosure | CClosure
    {
        // Old Cl
        Closure* old_cl = luaF_newCclosure(ls, src->nupvalues, src->env);
        old_cl->c.f = (lua_CFunction)src->c.f;
        old_cl->c.cont = (lua_Continuation)src->c.cont;
        old_cl->c.debugname = (const char*)src->c.debugname;
        
        for (int i = 0; i < src->nupvalues; i++)
            setobj2n(ls, &old_cl->c.upvals[i], &src->c.upvals[i]);
        
        setclvalue(ls, ls->top, old_cl);
        ls->top++;
        
        // upval set
        src->c.f = exploit::environment::blank_function;
        for (int i = 0; i < dst->nupvalues; i++)
            setobj2n(ls, &src->c.upvals[i], &dst->c.upvals[i]);
        
        src->nupvalues = dst->nupvalues;
        src->c.f = dst->c.f;
        
        return 1;
    }
    else if ( lua_isLfunction(ls, 1) && lua_isLfunction(ls, 2) ) // LClosure | LClosure
    {
        if ( dst_nups > src_nups ) luaL_argerror(ls, 2, "Destination Closure has more upvalues than Source Closure.");
        
        lua_clonefunction(ls, 1);
        
        src->l.p = dst->l.p;
        src->nupvalues = dst->nupvalues;
        src->stacksize = dst->stacksize;
        src->preload = dst->preload;
        for (int i = 0; i < dst->nupvalues; i++)
        {
            if (!lua_getupvalue(ls, 2, i)) continue;
            lua_setupvalue(ls, 1, i);
            lua_pop(ls, 1); 
        }
        
        return 1;
    }
    else if ( lua_iscfunction(ls, 1) && lua_isLfunction(ls, 2) ) // CClosure | LClosure
    {
        // Old Cl
        Closure* old_cl = luaF_newCclosure(ls, src->nupvalues, src->env);
        old_cl->c.f = (lua_CFunction)src->c.f;
        old_cl->c.cont = (lua_Continuation)src->c.cont;
        old_cl->c.debugname = (const char*)src->c.debugname;
        
        for (int i = 0; i < src->nupvalues; i++)
            setobj2n(ls, &old_cl->c.upvals[i], &src->c.upvals[i]);
        
        setclvalue(ls, ls->top, old_cl);
        ls->top++;
        
        // upval set
        src->c.f = exploit::environment::blank_function;
        for (int i = 0; i < dst->nupvalues; i++)
            setobj2n(ls, &src->c.upvals[i], &dst->c.upvals[i]);
        
        lua_ref(ls, 2);
        
        src->nupvalues = dst->nupvalues;
        src->c.f = newcclosure_handler;
        callcheck::newcclosure_map[ clvalue(luaA_toobject(ls, 1)) ] = clvalue(luaA_toobject(ls, 2));
         
        return 1;
    }
    else {
        luaL_error(ls, "Unsupported Closure Pair passed to 'hookfunction'. Supported Pairs: CC|CC , LC|LC, CC|LC , NCC|NCC , NCC|LC , CC|NCC");
    }
    
    return 0;
}

static int isourclosure( lua_State* ls )
{
    LOGD(" LuauEnvCall -> isourclosure - CallingThread -> %p", ls);
    
    luaL_checktype(ls, 1, LUA_TFUNCTION);
    
    auto cl = clvalue(luaA_toobject(ls, 1));
    
    if ( lua_iscfunction(ls, 1) )
    {
        if ( isnewcc( cl ) || cl->c.f == callcheck::call_handler )
        {
            lua_pushboolean(ls, 1);
            return 1;
        }
    }
    else 
    {
        auto srcname = getstr(cl->l.p->source);
        
        if ( strcmp(srcname, exploit_configuration::exploit_luaChunk.c_str( )) || chunknamesbruh.count( std::string(srcname) ) )
        {
            lua_pushboolean(ls, 1);
            return 1;
        }
    }
    
    lua_pushboolean(ls, 0);
    return 1;
}

static const luaL_Reg funcs[ ] = {
    {"newcclosure", newcclosure},
    {"loadstring", loadstring},
    {"islclosure", islclosure},
    {"iscclosure", iscclosure},
    {"checkcaller", checkcaller},
    {"getcallingscript", getcallingscript},
    {"getscriptfromthread", getscriptfromthread},
    {"clonefunction", clonefunction},
    {"hookfunction", hookfunction},
    {"replaceclosure", hookfunction},
    {"isourclosure", isourclosure},
    {"checkclosure", isourclosure},
    {"isXclosure", isourclosure},
    {"isexecutorclosure", isourclosure},
    
    {nullptr, nullptr}
};

auto exploit::environment::closure( lua_State* ls ) -> void
{
    lua_pushvalue(ls, LUA_GLOBALSINDEX);
    register_lib(ls, nullptr, funcs);
    lua_pop(ls, 1);
}