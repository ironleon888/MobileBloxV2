#include "execution.hpp"

std::uintptr_t job_cache{ };
std::mutex mutex;

auto exploit::execution::execute(const std::string& script) -> void
{
    // Make a new thread off of our exploit lstate for each ran script
    auto ls = roblox::functions::rlua_newthread(taskscheduler::get_singleton( )->get_ExploitState( ));
    lua_pop(taskscheduler::get_singleton( )->get_ExploitState( ), 1);
    
    static auto encoder = bytecode_encoder_t( );
    auto bytecode = Luau::compile(script, { 2, 1, 2 }, { true, true }, &encoder);
    
    // necessary? could move to separate function 
    /*
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
    */
    //std::uintptr_t id[2] = { 8, 0 };
    //auto perms = roblox::functions::sandboxthreadandsetidentity(ls, reinterpret_cast<uintptr_t>(id), DummyLScript);
    roblox::functions::set_identity(ls, 8);
    
    // task.defer(LClosure)
    lua_getglobal(ls, "task");
    lua_getfield(ls, -1, "defer");
    if (luau_load(ls, exploit_configuration::exploit_luaChunk.c_str( ), bytecode.c_str( ), bytecode.size( ), 0) != 0)
    {
        auto err = lua_tostring(ls, -1);
        
        lua_getglobal(ls, "warn");
        lua_pushstring(ls, err);
        lua_call(ls, 1, 0);
        
        LOGE(" [Execution] Error during LClosure Loading: %s", err);
        return;
    }
    
    lua_pcall(ls, 1, 0, 0); // could add a bit of err handling
    
    /*
    roblox::structs::live_thread_ref* ref = new roblox::structs::live_thread_ref;
    ref->th = ls;
    lua_pushthread(ls);
    ref->thread_id = lua_ref(ls, -1);
    lua_pop(ls, 1);
    
    std::uintptr_t unk[4] = { 0, 0, 0 };
    roblox::functions::scriptcontext_resume(reinterpret_cast<uintptr_t>(unk), taskscheduler::get_singleton( )->get_CurrentSC( ), reinterpret_cast<std::uintptr_t*>( &ref ), 0, 0, 0);
    */
}

auto whsj_step_hook( std::uintptr_t job ) -> std::uintptr_t
{
    std::unique_lock<std::mutex> guard{ mutex };
    static auto exploit_exec = exploit::execution::get_singleton( );
    
    // script_queue
    if ( !exploit_exec->squeue_empty( ) )
    {
        guard.unlock( );
        exploit_exec->execute(exploit_exec->squeue_top( ));
    }
    
    // yield_queue
    if ( !exploit_exec->yqueue_empty( ) )
    {
        auto yielded = exploit_exec->yqueue_top( );
        std::uintptr_t unk[4] = { 0, 0, 0 };
        roblox::functions::scriptcontext_resume(reinterpret_cast<uintptr_t>(unk), taskscheduler::get_singleton( )->get_CurrentSC( ), reinterpret_cast<std::uintptr_t*>(&yielded.first), yielded.second, 0, 0);
    }

    return reinterpret_cast<std::uintptr_t(*)(std::uintptr_t)>(job_cache)(job);
}

auto exploit::execution::schedule(const std::string& script) -> void
{
    std::unique_lock<std::mutex> guard{ mutex };
    script_queue.push(script);
}

auto exploit::execution::schedule_thread(lua_State* ls, int nargs) -> void
{
    std::unique_lock<std::mutex> guard{ mutex };
    
    roblox::structs::live_thread_ref* ref = new roblox::structs::live_thread_ref;
    ref->th = ls;
    lua_pushthread(ls);
    ref->thread_id = lua_ref(ls, -1);
    lua_pop(ls, 1);
    
    auto yielded = std::make_pair(ref, nargs);
    yield_queue.push(yielded);
}

auto exploit::execution::hook_script_job( std::uintptr_t job ) -> void
{
    // if it gets hooked twice then job cache changes to whsj_step_hook and we get a recursive call to nothingness which breaks roblox and our exec
    if ( !job_cache )
    {
        job_cache = taskscheduler::get_singleton( )->hook_job(job, (void*)&whsj_step_hook);
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