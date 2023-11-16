#include "taskscheduler.hpp"
#include "../roblox/update.hpp"

uintptr_t new_vftable[7];

auto taskscheduler::get_jobs( ) -> std::vector<std::uintptr_t>
{
    std::vector<std::uintptr_t> job_list{ };
    
    auto TS = roblox::functions::get_tasksched( );
    auto current_job = *reinterpret_cast<std::uintptr_t**>(TS + 236);
    do {
        job_list.push_back(*current_job);
        current_job += 2;
    } while (current_job != *reinterpret_cast<std::uintptr_t**>(TS + 240));
    
    return job_list;
}

// testing purposes 
auto taskscheduler::log_jobs( ) -> void
{
    auto job_list = this->get_jobs( );
    
    int i = 0;
    for ( auto job : job_list )
    {
        auto job_name = *reinterpret_cast<const char**>(job + 128);

        if ( !(*(std::uint8_t*)(job + 120) << 31) ) {
            job_name = (const char*)(job + 121);
        }
        
        LOGI(" taskscheduler::log_jobs | Job[ %s ] 0x%X ", job_name, job);
        i++;
    }
    
    LOGI(" taskscheduler::log_jobs | Logged %i Jobs ", i);
}

auto taskscheduler::get_job_by_name(const std::string& name) -> std::uintptr_t
{
    auto job_list = this->get_jobs( );
    
    for ( auto job : job_list )
    {
        auto job_name = *reinterpret_cast<const char**>(job + 128);

        if ( !(*(std::uint8_t*)(job + 120) << 31) ) {
            job_name = (const char*)(job + 121);
        }
        
        if ( memcmp(job_name, name.c_str( ), name.size( )) == 0 ) return job;
    }
    
    return 0;
}

auto taskscheduler::hook_job(uintptr_t job, void* dst) -> std::uintptr_t
{
    memcpy(new_vftable, *reinterpret_cast<void**>(job), 4u * 7u);

    auto old_vftable = *reinterpret_cast<uintptr_t**>(job);
    new_vftable[6] = reinterpret_cast<uintptr_t>(dst);

    *reinterpret_cast<uintptr_t**>(job) = new_vftable;
    return old_vftable[6];
}

auto taskscheduler::get_scriptcontext( ) -> std::uintptr_t
{
    auto WHSJ = get_job_by_name("WaitingHybridScriptsJob");
    return *reinterpret_cast<uintptr_t*>(WHSJ + 360);
}

auto taskscheduler::get_mainstate( ) -> lua_State*
{
    auto sc = this->get_scriptcontext( );
    auto rL = roblox::addresses::rLEnc( sc );
    return reinterpret_cast<lua_State*>( rL ); 
}

auto taskscheduler::find_fpscap( ) -> std::uintptr_t
{
    static auto off = 0;
    auto TS = roblox::functions::get_tasksched( );
    if ( !off )
    {
        for ( int i = 0; i < 123456; i += 4 )
        {
            auto val = *reinterpret_cast<double*>(TS + i);
            if (1.0 / val == 60.0)
            {
                LOGD(" FPS Cap Offset: %i", i);
                off = i;
                break;
            }
        }
    }
    
    return off;
}

auto taskscheduler::set_fpscap( double cap ) -> void
{
    static auto off = this->find_fpscap( );
    
    static const double min_frame_delay = 1.0 / 1000.0;
    double frame_delay = cap <= 0.0 ? min_frame_delay : 1.0 / cap;
    *reinterpret_cast<double*>(roblox::functions::get_tasksched( ) + off) = frame_delay;
}

auto taskscheduler::get_fpscap( ) -> double
{
    static auto off = this->find_fpscap( );
    
    auto cap = *reinterpret_cast<double*>(roblox::functions::get_tasksched( ) + off);
    return 1.0 / cap;
}