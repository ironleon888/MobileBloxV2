#include "hooks.hpp"

#include <dobby/dobby.h>
#include <cstdint>

#include "../exploit.hpp"
#include "../utils/utils.hpp"
#include "update.hpp"
#include "../execution/execution.hpp"

// stays in lobby.
static std::uintptr_t startscript_orig = 0x0;
static std::uintptr_t startscript_hook( std::uintptr_t sc, std::uintptr_t script )
{
    const auto orig = *reinterpret_cast<decltype(startscript_hook)*>( startscript_orig );
    
    static std::uintptr_t curr_sc = 0;
    if ( curr_sc != sc )
    {
        curr_sc = sc;
        exploit::start( );
    }
    
    return orig( sc, script );
}

static std::uintptr_t ongameloaded_original = 0x0;
static int ongameloaded_hook( std::uintptr_t a1, int a2, int placeid, int a4 )
{
    const auto original = *reinterpret_cast< decltype( ongameloaded_hook )* >( ongameloaded_original );
    
    // can add a check for not loading in lobby ( placeid == 0 then noload )
    exploit::start( );
    
    return original( a1, a2, placeid, a4 );
}

static std::uintptr_t ongameleave_original = 0x0;
static int ongameleave_hook( std::uintptr_t a1, int placeid ) 
{
    const auto original = *reinterpret_cast< decltype( ongameleave_hook )* >( ongameleave_original );
    
    exploit::execution::get_singleton( )->clearqueues( );
    exploit::DummyLScript = 0;
    
    return original( a1, placeid );
}

// we can simply avoid the exception like here but thats a huge vulnerability
// so instead lets do it in the proper way shall we? except this is open src so whatever
// already spent a good amount of time reversing
static std::uintptr_t RobloxContextSystem_CapabilityErr_orig = 0x0;
static void RobloxContextSystem_CapabilityErr_hook(int a1, int a2, const char* a3, const char* a4)
{
    const auto original = *reinterpret_cast< decltype( RobloxContextSystem_CapabilityErr_hook )* >( RobloxContextSystem_CapabilityErr_orig );
    
    LOGD(" [RobloxContextSystem_CapabilityErr_hook]: 0x%X, 0x%X, %s, %s", a1, a2, a3, a4);
    
   // original( a1, a2, a3, a4 );
}

static std::uintptr_t RobloxContextSystem_CheckCapabilities_orig = 0x0;
static int RobloxContextSystem_CheckCapabilities_hook(std::int64_t a1, int a2)
{
    const auto original = *reinterpret_cast< decltype( RobloxContextSystem_CheckCapabilities_hook )* >( RobloxContextSystem_CheckCapabilities_orig );
    
    LOGD(" [RobloxContextSystem_CheckCapabilities_hook]: 0x%X, 0x%X", a1, a2);
    
    int inst = a1; // lowdworrd
    auto required_cpbs = *reinterpret_cast<std::uint8_t*>(inst + 37);
    
    LOGD(" [RobloxContextSystem_CheckCapabilities_hook]: Required capabilities -> 0x%X", required_cpbs);
    
    return original( a1, a2 );
}

void roblox::hooks::init( )
{
    //const auto startscript = utils::memory::rebase( "libroblox.so", addresses::startscript );
    const auto ongameloaded = utils::memory::rebase( "libroblox.so", addresses::ongameloaded );
    const auto ongameleave = utils::memory::rebase( "libroblox.so", addresses::ongameleave );
    const auto RobloxContextSystem_CapabilityErr = utils::memory::rebase( "libroblox.so", 0x38CBEAC + 1 );
    const auto RobloxContextSystem_CheckCapabilities = utils::memory::rebase( "libroblox.so", 0x2EE8D70 + 1 );
    
    //DobbyHook( reinterpret_cast< void* >( startscript ), reinterpret_cast< void* >( &startscript_hook ), reinterpret_cast< void** >( &startscript_orig ) );
    DobbyHook( reinterpret_cast< void* >( ongameloaded ), reinterpret_cast< void* >( &ongameloaded_hook ), reinterpret_cast< void** >( &ongameloaded_original ) );
    DobbyHook( reinterpret_cast< void* >( ongameleave ), reinterpret_cast< void* >( &ongameleave_hook ), reinterpret_cast< void** >( &ongameleave_original ) );
    DobbyHook( reinterpret_cast< void* >( RobloxContextSystem_CapabilityErr ), reinterpret_cast< void* >( &RobloxContextSystem_CapabilityErr_hook ), reinterpret_cast< void** >( &RobloxContextSystem_CapabilityErr_orig ) );
    //DobbyHook( reinterpret_cast< void* >( RobloxContextSystem_CheckCapabilities ), reinterpret_cast< void* >( &RobloxContextSystem_CheckCapabilities_hook ), reinterpret_cast< void** >( &RobloxContextSystem_CheckCapabilities_orig ) );
}