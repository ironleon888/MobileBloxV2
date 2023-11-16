#include "execution.hpp"

std::uintptr_t job_cache{ };
std::mutex mutex;

// Encoding is done inside BytecodeBuilder.cpp
// ABC: uint32_t insn = uint32_t((uint8_t)(op * 227)) | (a << 8) | (b << 16) | (c << 24);
// AD: uint32_t insn = uint32_t((uint8_t)(op * 227)) | (a << 8) | (uint16_t(d) << 16);
// E: uint32_t insn = uint32_t((uint8_t)(op * 227)) | (uint32_t(e) << 8);
auto exploit::execution::execute(const std::string& script) -> void
{
    auto bytecode = Luau::compile(script, { 2, 1, 2 }, { true, true });

    LOGD(" [Execution] Called.");
    
    auto ls = roblox::functions::rlua_newthread(taskscheduler::get_singleton( )->get_ExploitState( ));
    lua_pop(taskscheduler::get_singleton( )->get_ExploitState( ), 1);
    //auto ls = taskscheduler::get_singleton( )->get_ExploitState( );
    
    if ( DummyLScript == 0 ) 
    {
        lua_getglobal(ls, "Instance");
        lua_getfield(ls, -1, "new");
        lua_pushstring(ls, "LocalScript");
        lua_pcall(ls, 1, 1, 0);
        
        DummyLScript = reinterpret_cast<std::uintptr_t>(lua_touserdata(ls, -1)); // pretty sure this is a shared ptr ( script, reference )
        lua_ref(ls, -1); // don't gc our dummy script
        
        lua_setglobal(ls, "script");
        lua_pop(ls, 1);
        LOGD(" [Execution] Created dummy LocalScript. 0x%X ", DummyLScript);
    }
    
    std::uintptr_t id[2] = { 8, 0 };
    auto perms = roblox::functions::sandboxthreadandsetidentity(ls, reinterpret_cast<uintptr_t>(id), DummyLScript);
    roblox::functions::set_identity(ls, 8);
    
    if (luau_load(ls, exploit_configuration::exploit_luaChunk.c_str( ), bytecode.c_str( ), bytecode.size( ), 0) != 0)
    {
        lua_getglobal(ls, "warn");
        lua_pushstring(ls, bytecode.c_str( ) + 1);
        lua_pcall(ls, 1, 0, 0);
        
        LOGE(" [Execution] Error during LClosure Loading: %s", bytecode.c_str( ) + 1);
        return;
    }
    
    // could get task.spawn or task.defer through lua but meh
    roblox::functions::rbxspawn(ls);
    LOGD(" [Execution] Spawned.");
    
    /*
    roblox::structs::live_thread_ref* ref = new roblox::structs::live_thread_ref;
    ref->th = ls;
    lua_pushthread(ls);
    ref->thread_id = lua_ref(ls, -1);
    lua_pop(ls, 1);
    
    roblox::functions::scriptcontext_resume(taskscheduler::get_singleton( )->get_CurrentSC( ), reinterpret_cast<std::uintptr_t**>( &ref ), 0, 0, 0);
    */
}

auto whsj_step_hook( std::uintptr_t job ) -> std::uintptr_t
{
    std::unique_lock<std::mutex> guard{ mutex };
    static auto exploit_exec = exploit::execution::get_singleton( );
    
    if ( !exploit_exec->squeue_empty( ) )
    {
        guard.unlock( );
        exploit_exec->execute(exploit_exec->squeue_top( ));
    }

    if ( !exploit_exec->yqueue_empty( ) )
    {
        auto yielded = exploit_exec->yqueue_top( );
        roblox::functions::scriptcontext_resume(taskscheduler::get_singleton( )->get_CurrentSC( ), reinterpret_cast<std::uintptr_t**>(&yielded.first), yielded.second, 0, 0);
    }

    return reinterpret_cast<std::uintptr_t(*)(std::uintptr_t)>(job_cache)(job);
}

auto exploit::execution::schedule(const std::string& script) -> void
{
    std::unique_lock<std::mutex> guard{ mutex };
    script_queue.push(script);
}

auto exploit::execution::hook_script_job( ) -> void
{
    auto TS = taskscheduler::get_singleton( );
    
    // if it gets hooked twice then job cache changes to whsj_step_hook and we get a recursive call to nothingness which breaks roblox and our exec
    if ( !job_cache )
    {
        job_cache = TS->hook_job(TS->get_job_by_name("WaitingHybridScriptsJob"), (void*)&whsj_step_hook);
        LOGD(" [hook_script_job] Hooked Job. ");
    }
}

auto exploit::execution::squeue_empty( ) -> bool 
{
    return this->script_queue.empty( );
}

auto exploit::execution::squeue_top( ) -> std::string 
{
    auto result = this->script_queue.front( );
    this->script_queue.pop( );
    return result;
}

auto exploit::execution::yqueue_empty( ) -> bool
{
    return this->yield_queue.empty( );
}

auto exploit::execution::yqueue_top( ) -> std::pair<roblox::structs::live_thread_ref*, int> 
{
    auto result = this->yield_queue.front( );
    this->yield_queue.pop( );
    return result;
}

auto exploit::execution::clearqueues( ) -> void
{
    while ( !this->script_queue.empty( ) ) this->script_queue.pop( );
    while ( !this->yield_queue.empty( ) ) this->yield_queue.pop( );
}