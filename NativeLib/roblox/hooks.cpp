#include "hooks.hpp"

#include <dobby/dobby.h>
#include <cstdint>
#include <unordered_set>

#include "../exploit.hpp"
#include "../utils/utils.hpp"
#include "update.hpp"
#include "../execution/execution.hpp"

static std::uintptr_t startscript_orig = 0x0;
static std::uintptr_t startscript_hook( std::uintptr_t sc, std::uintptr_t script )
{
    const auto orig = *reinterpret_cast<decltype(startscript_hook)*>( startscript_orig );
    /*
    // rL doesnt change + not called
    static lua_State* curr_rL{ nullptr };
    auto rL = reinterpret_cast<lua_State*>(roblox::addresses::rLEnc( sc ));
    if ( curr_rL != rL )
    {
        curr_rL = rL;
        exploit::start( rL );
    }
    */
    
    return orig( sc, script );
}

static std::uintptr_t ongameloaded_original = 0x0;
static int ongameloaded_hook( std::uintptr_t a1, int a2, int placeid, int a4 )
{
    const auto original = *reinterpret_cast<decltype( ongameloaded_hook )*>( ongameloaded_original );
    
    LOGD(" [ongameloaded_hook] 0x%X, 0x%X, %i, 0x%X.", a1, a2, placeid, a4);
    /*
    if (placeid)
    {
        // inf loop because rl doesnt change 
initenv: // maybe a better way than this could be used
        // failed so we restart
        if (!exploit::start( ))
        {
            goto initenv;
        }
    }
    */
    return original( a1, a2, placeid, a4 );
}

static std::unordered_set<std::uintptr_t> startedJobs;
static std::uintptr_t jobstart_original = 0x0;
static int jobstart_hook( std::uintptr_t job )
{
    const auto original = *reinterpret_cast<decltype( jobstart_hook )*>( jobstart_original );
    
    // Pseudocode from Roblox's functions. std string stuff 
    auto job_name = *reinterpret_cast<const char**>(job + 128);
    if ( !(*(std::uint8_t*)(job + 120) << 31) )
    {
        job_name = (const char*)(job + 121);
    }
     
     // memcmp as Mr Corruption is always haunting us
    if ( memcmp(job_name, "WaitingHybridScriptsJob", 23) == 0 )
    {
        // Check if the job has already been started
        if (startedJobs.find(job) == startedJobs.end( ))
        {
            // Mark the job as started to avoid duplicate calls
            startedJobs.insert(job);
            
            // Start our exploit using this job
            exploit::start( job );
        }
    }
    
    return original( job );
}

static std::uintptr_t jobstop_original = 0x0;
static int jobstop_hook( std::uintptr_t job )
{
    const auto original = *reinterpret_cast<decltype( jobstop_hook )*>( jobstop_original );
    
    // Pseudocode from Roblox's functions. std string stuff 
    auto job_name = *reinterpret_cast<const char**>(job + 128);
    if ( !(*(std::uint8_t*)(job + 120) << 31) )
    {
        job_name = (const char*)(job + 121);
    }
     
     // memcmp as Mr Corruption is always haunting us
    if ( memcmp(job_name, "WaitingHybridScriptsJob", 23) == 0 )
    {
        exploit::leave( );
    }
    
    return original( job );
}

static std::uintptr_t ongameleave_original = 0x0;
static int ongameleave_hook( std::uintptr_t a1, int placeid ) 
{
    const auto original = *reinterpret_cast<decltype( ongameleave_hook )*>( ongameleave_original );
    
    exploit::leave( );
    
    return original( a1, placeid );
}

// we can simply avoid the exception like here but thats a huge vulnerability
// so instead lets do it in the proper way shall we? except this is open src so whatever
// already spent a good amount of time reversing
static std::uintptr_t RobloxContextSystem_CapabilityErr_orig = 0x0;
static void RobloxContextSystem_CapabilityErr_hook(int a1, int a2, const char* a3, const char* a4)
{
    const auto original = *reinterpret_cast< decltype( RobloxContextSystem_CapabilityErr_hook )* >( RobloxContextSystem_CapabilityErr_orig );
    
    //LOGD(" [RobloxContextSystem_CapabilityErr_hook]: 0x%X, 0x%X, %s, %s", a1, a2, a3, a4);
    
   // original( a1, a2, a3, a4 );
}

static std::uintptr_t RobloxContextSystem_CheckCapabilities_orig = 0x0;
static int RobloxContextSystem_CheckCapabilities_hook(std::int64_t a1, int a2)
{
    const auto original = *reinterpret_cast<decltype( RobloxContextSystem_CheckCapabilities_hook )*>( RobloxContextSystem_CheckCapabilities_orig );
    
    LOGD(" [RobloxContextSystem_CheckCapabilities_hook]: 0x%X, 0x%X", a1, a2);
    
    int inst = a1; // lowdworrd
    auto required_cpbs = *reinterpret_cast<std::uint8_t*>(inst + 37);
    
    LOGD(" [RobloxContextSystem_CheckCapabilities_hook]: Required capabilities -> 0x%X", required_cpbs);
    
    return original( a1, a2 );
}

static std::uintptr_t MemAdviceinit_orig = 0x0;
static int MemAdviceinit_hook(JNIEnv* jenv, jobject globalcontext)
{
    const auto original = *reinterpret_cast<decltype( MemAdviceinit_hook )*>( MemAdviceinit_orig );
    
    LOGD(" [MemAdviceinit_hook]: %p, 0x%X", jenv, globalcontext);
    
    utils::JNI::GlobalContext = globalcontext;
    
    LOGD(" [MemAdviceinit_hook]: %p, 0x%X", jenv, globalcontext);
    
    return original( jenv, globalcontext );
}

void roblox::hooks::init( )
{
    //const auto startscript = utils::memory::rebase( "libroblox.so", addresses::startscript );
    //const auto ongameloaded = utils::memory::rebase( "libroblox.so", addresses::ongameloaded );
    const auto jobstart = utils::memory::rebase( "libroblox.so", addresses::jobstart );
    //const auto jobstop = utils::memory::rebase( "libroblox.so", addresses::jobstop );
    const auto ongameleave = utils::memory::rebase( "libroblox.so", addresses::ongameleave );
    const auto RobloxContextSystem_CapabilityErr = utils::memory::rebase( "libroblox.so", 0x38BDC04 + 1 );
    //const auto RobloxContextSystem_CheckCapabilities = utils::memory::rebase( "libroblox.so", 0x0 + 1 );
    //const auto MemAdviceinit = utils::memory::rebase( "libroblox.so", 0x22DF31C + 1 );
    
    //DobbyHook( reinterpret_cast<void*>( startscript ), reinterpret_cast<void*>( &startscript_hook ), reinterpret_cast<void**>( &startscript_orig ) );
    //DobbyHook( reinterpret_cast<void*>( ongameloaded ), reinterpret_cast<void*>( &ongameloaded_hook ), reinterpret_cast<void**>( &ongameloaded_original ) );
    DobbyHook( reinterpret_cast<void*>( jobstart ), reinterpret_cast<void*>( &jobstart_hook ), reinterpret_cast<void**>( &jobstart_original ) );
    DobbyHook( reinterpret_cast<void*>( ongameleave ), reinterpret_cast<void*>( &ongameleave_hook ), reinterpret_cast<void**>( &ongameleave_original ) );
    DobbyHook( reinterpret_cast<void*>( RobloxContextSystem_CapabilityErr ), reinterpret_cast<void*>( &RobloxContextSystem_CapabilityErr_hook ), reinterpret_cast<void**>( &RobloxContextSystem_CapabilityErr_orig ) );
    //DobbyHook( reinterpret_cast<void*>( RobloxContextSystem_CheckCapabilities ), reinterpret_cast<void*>( &RobloxContextSystem_CheckCapabilities_hook ), reinterpret_cast<void**>( &RobloxContextSystem_CheckCapabilities_orig ) );
    //DobbyHook( reinterpret_cast<void*>( MemAdviceinit ), reinterpret_cast<void*>( &MemAdviceinit_hook ), reinterpret_cast<void**>( &MemAdviceinit_orig ) );
}