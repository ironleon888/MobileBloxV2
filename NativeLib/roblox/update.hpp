#pragma once

#include <lstate.h>
#include <cstdint>
#include <string>

/*
Messy as fuck i know but welp
Luau Files modified:

* luaconf.h -> addresses of luauAPI funcs
#include "../../../../utils/utils.hpp"

namespace roblox::addresses {
    // luau VM
    constexpr std::uintptr_t luau_execute = 0x0 + 1; // luaD_call
    
    // luaD_* functions
    constexpr std::uintptr_t luaD_throw = 0x0 + 1; // lua_error <- luaL_error // B0 B5 02 AF 05 46 0C 20 0C 46 6D
    constexpr std::uintptr_t luaD_rawrunprotected = 0x0 + 1; // luaD_pcall
    
    // luaC_* functions
    // lua_newthread -> B0 B5 02 AF 04 46 00 69 D0
    constexpr std::uintptr_t luaC_step = 0x0 + 1; // lua_newthread // F0 B5 03 AF 2D E9 00 0F 81 B0 2D ED 02 8B 84 B0 04 69
    constexpr std::uintptr_t luaC_barriertable = 0x0 + 1; // 00 69 43 7D 02
    
    // luaV_* functions
    constexpr std::uintptr_t luaV_gettable = 0x0 + 1;
    constexpr std::uintptr_t luaV_settable = 0x0 + 1;
    
    constexpr std::uintptr_t luaO_nilobj = 0x0; // any luau push or get or to function
    constexpr std::uintptr_t dummynode = 0x0; // luaH_getnum <- usually inlined in luaH_get
}

* luaconf.h

* lobject.h
//#define luaO_nilobject (&luaO_nilobject_)
#define luaO_nilobject (TValue*)utils::memory::rebase( "libroblox.so", roblox::addresses::luaO_nilobj )
*lobject.h

* ltable.cpp
//#define dummynode (&luaH_dummynode)
#define dummynode (LuaNode*)utils::memory::rebase( "libroblox.so", roblox::addresses::dummynode )
*ltable.cpp

* lvmexecute.cpp -> luau_execute <- or change the name of the func if too lazy to remove "Moduleluau_execute"
void luau_execute(lua_State* L)
{
    static auto rluau_exec = *reinterpret_cast< decltype(luau_execute)* >( utils::memory::rebase( "libroblox.so", roblox::addresses::luau_execute ) );
    return rluau_exec( L );
}

* lvmutils.cpp -> luaV_gettable, luaV_settable
void luaV_gettable(lua_State* L, const TValue* t, TValue* key, StkId val)
{
    static auto rgettable = *reinterpret_cast< decltype(luaV_gettable)* >( utils::memory::rebase( "libroblox.so", roblox::addresses::luaV_gettable ) );
    return rgettable( L, t, key, val );
}

void luaV_settable(lua_State* L, const TValue* t, TValue* key, StkId val)
{
    static auto rsettable = *reinterpret_cast< decltype(luaV_settable)* >( utils::memory::rebase( "libroblox.so", roblox::addresses::luaV_settable ) );
    return rsettable( L, t, key, val );
}

* ldo.cpp -> luaD_throw, luaD_rawrunprotected
int luaD_rawrunprotected(lua_State* L, Pfunc f, void* ud)
{
    static auto rluau_rawrunprot = *reinterpret_cast< decltype(luaD_rawrunprotected)* >( utils::memory::rebase( "libroblox.so", roblox::addresses::luaD_rawrunprotected ) );
    return rluau_rawrunprot( L, f, ud );
}

l_noret luaD_throw(lua_State* L, int errcode)
{
    static auto rluau_throw = *reinterpret_cast< decltype(luaD_throw)* >( utils::memory::rebase( "libroblox.so", roblox::addresses::luaD_throw ) );
    rluau_throw( L, errcode );
}

* lgc.cpp -> luaC_step, luaC_barriertable
size_t luaC_step(lua_State* L, bool assist)
{
    static auto rluacvarier = *reinterpret_cast< decltype(luaC_step)* >( utils::memory::rebase( "libroblox.so", roblox::addresses::luaC_step ) );
    return rluacvarier( L, assist);
}

void luaC_barriertable(lua_State* L, Table* t, GCObject* v)
{
    static auto rluacvarier = *reinterpret_cast< decltype(luaC_barriertable)* >( utils::memory::rebase( "libroblox.so", roblox::addresses::luaC_barriertable ) );
    rluacvarier( L, t, v );
}

*/

/*
bytecode localscript + 268
modulescript + 240 
sharedstr + 16 = data

getbytecode func local/modulescript vftable + 232
vftable + 252 = getbytecodedata ( derefs the shared str too ) 
*/
namespace roblox {
    namespace addresses {
        // LuaState Encryption
        auto rLEnc( std::uintptr_t sc ) -> std::uintptr_t;
        
        // Objs
        constexpr std::uintptr_t tasksched = 0x41DC86C; // "Flags must be loaded before taskscheduler can be used"
        constexpr std::uintptr_t prop_table = 0x405F324;
        
        // Funcs
        constexpr std::uintptr_t scriptcontext_resume = 0x18F4760 + 1; // "$Script" // F0 B5 03 AF 2D E9 00 0F 81 B0 2D ED 04 8B AC B0 83 46 E0
        constexpr std::uintptr_t rbxspawn = 0x1932D30 + 1; // "Spawn function requires 1 argument" // F0 B5 03 AF 2D E9 00 0F 81 B0 2D ED 04 8B AC B0 05 46 D7
        constexpr std::uintptr_t sandboxthreadandsetidentity = 0x18E51E8 + 1; // "Unable to create a new thread for %s" // F0 B5 03 AF 2D E9 00 07 92 46 88 46
        constexpr std::uintptr_t rlua_pushinstance = 0x18A2668 + 1; // "InvalidInstance"
        constexpr std::uintptr_t pushinstance_registry = 0x18A20B4 + 1; // "InvalidInstance"
        static std::uintptr_t pushinstance_registry_rebased = 0x0; // basically rebased pushinst reg, inititiated at funcs::init
        constexpr std::uintptr_t rbx_getthreadcontext = 0x18DE6B4 + 1; // printidentity
        constexpr std::uintptr_t rlua_newthread = 0x36CC828 + 1; // "unable to create a new thread for %s"
        constexpr std::uintptr_t fireclickdetector = 0x23CF824 + 1; // ClickDetectorE vftable + 0xE4 First function call with 3 arguments in the case 1 of the switch.
        constexpr std::uintptr_t fireproximityprompt = 0x23D3E78 + 1; // "ProximityPrompt_triggered"
        constexpr std::uintptr_t firetouchinterest = 0x2F44E84 + 1; // "new overlap in different world" function call return
        
        // Func Hooks
        constexpr std::uintptr_t startscript = 0x1920834 + 1; // "Script Start" // F0 B5 03 AF 2D E9 00 0F B5 B0 83 46 DF F8 58 0C
        constexpr std::uintptr_t ongameleave = 0x131ED10 + 1; // "onGameLeaveBegin() SessionReporterState_GameExitRequested placeId:%lld"
        constexpr std::uintptr_t ongameloaded = 0x131EAC8 + 1; // "onGameLoaded() SessionReporterState_GameLoaded placeId:%lld"
        constexpr std::uintptr_t jobstart = 0x3A4C48C + 1; // "[FLog::TaskSchedulerRun] JobStart %s"
        constexpr std::uintptr_t jobstop = 0x3A4C4FC + 1; // "[FLog::TaskSchedulerRun] JobStop %s"
        
        // Now in luaconf.h
        /*
        // luau VM
        constexpr std::uintptr_t luau_execute = 0x36DD6D4 + 1; // luaD_call
        
        // luaD_* functions
        constexpr std::uintptr_t luaD_throw = 0x36D245C + 1; // lua_error <- luaL_error // B0 B5 02 AF 05 46 0C 20 0C 46 6D
        constexpr std::uintptr_t luaD_rawrunprotected = 0x36D2400 + 1; // luaD_pcall
        
        // luaC_* functions
        // lua_newthread -> B0 B5 02 AF 04 46 00 69 D0
        constexpr std::uintptr_t luaC_step = 0x36D3168 + 1; // lua_newthread // F0 B5 03 AF 2D E9 00 0F 81 B0 2D ED 02 8B 84 B0 04 69
        constexpr std::uintptr_t luaC_barriertable = 0x36D3C44 + 1; // 00 69 43 7D 02
        
        // luaV_* functions
        constexpr std::uintptr_t luaV_gettable = 0x36E7488 + 1;
        constexpr std::uintptr_t luaV_settable = 0x36E75A0 + 1;
        
        // Objs
        constexpr std::uintptr_t luaO_nilobj = 0xD140C8; // any luau push or get or to function
        constexpr std::uintptr_t dummynode = 0xD141D8; // luaH_getnum <- usually inlined in luaH_get
        */
    }
    
    // currently unused in this src. good practice to use these to easily update 
    namespace offsets {
        constexpr std::uintptr_t JobListStart = 236;
        constexpr std::uintptr_t JobListEnd = 240;
        
        constexpr std::uintptr_t Whsj_ScriptCtxt = 360;
        constexpr std::uintptr_t FpsCap = 208; // 
    }
    
    namespace structs {
        struct live_thread_ref
        {
            int unk_0; // 0
            lua_State* th; // 4
            int thread_id; // 8
        };
        
        struct weak_thread_ref_t
        {
            std::uint8_t pad_0[8];
            
            weak_thread_ref_t* previous; // 8
            weak_thread_ref_t* next; // 12
            live_thread_ref* livethreadref; // 16
        };
        
        struct ExtraSpace_t
        {
            struct Shared {
                int threadCount; // 0
                std::uintptr_t script_context; // 4
                ExtraSpace_t* allThreads; // 8
            };
            
            ExtraSpace_t* previous; // 0
            size_t count; // 4
            ExtraSpace_t* next; // 8
            std::shared_ptr<Shared> shared; //12, 16
            
            weak_thread_ref_t* Node; // 20 // shouldnt this be weak_thread_ref_t::Node ?
            int context_level; // 24
            
            std::uint8_t pad_0[12];
            
            std::uintptr_t script_context; //40
            std::uintptr_t unk_0; //44
            std::uintptr_t context_perms; // 48
            std::uintptr_t unk_1; // 52
            std::weak_ptr<uintptr_t> script; //56
            // 60, 64, 68 contain weak refs to script, 72, 76 to a parent of the script
        };
    }
    
    namespace functions {
        auto init( ) -> void;
        auto get_tasksched( ) -> std::uintptr_t;
        auto set_identity(  lua_State* ls, int identity ) -> void;
        
        inline int ( *rbxspawn )( lua_State* rL ) = nullptr;
        inline lua_State* ( *rlua_newthread )( lua_State* rL ) = nullptr;
        inline std::uintptr_t ( *rbx_getthreadcontext )( lua_State* thread ) = nullptr;
        inline int ( *scriptcontext_resume )( int unk, std::uintptr_t sc, std::uintptr_t* ref, int nargs, int, int) = nullptr;
        inline std::uintptr_t ( *sandboxthreadandsetidentity )( lua_State* ls, std::uintptr_t identity, std::uintptr_t script ) = nullptr;
        inline std::uintptr_t ( *rlua_pushinstance )( lua_State* ls, std::uintptr_t inst ) = nullptr;
        inline std::uintptr_t ( *rlua_pushinstanceSP )( lua_State* ls, std::weak_ptr<uintptr_t> inst ) = nullptr;
        inline std::uintptr_t ( *fireclickdetector )( std::uintptr_t detector, float distance, std::uintptr_t player ) = nullptr;
        inline std::uintptr_t ( *fireproximityprompt )( std::uintptr_t prompt ) = nullptr;
        inline std::uintptr_t ( *firetouchinterest )( std::uintptr_t world, std::uintptr_t to_touch, std::uintptr_t transmitter, int untouch, int) = nullptr;
    }
}