#include "update.hpp"
#include "../utils/utils.hpp"

auto roblox::addresses::rLEnc( std::uintptr_t sc ) -> std::uintptr_t
{
	return *(std::uintptr_t*)( sc + 268 ) ^ ( sc + 268 );
}

auto roblox::functions::get_tasksched( ) -> std::uintptr_t
{
	return *reinterpret_cast<std::uintptr_t*>( utils::memory::rebase("libroblox.so", addresses::tasksched) );
}
		
void roblox::functions::init( )
{
	rbx_getthreadcontext = reinterpret_cast< decltype( rbx_getthreadcontext ) >( utils::memory::rebase( "libroblox.so", addresses::rbx_getthreadcontext ) );
	scriptcontext_resume = reinterpret_cast< decltype( scriptcontext_resume ) >( utils::memory::rebase( "libroblox.so", addresses::scriptcontext_resume ) );
	sandboxthreadandsetidentity = reinterpret_cast< decltype( sandboxthreadandsetidentity ) >( utils::memory::rebase( "libroblox.so", addresses::sandboxthreadandsetidentity ) );
	rlua_pushinstance = reinterpret_cast< decltype( rlua_pushinstance ) >(utils::memory::rebase( "libroblox.so", addresses::rlua_pushinstance ) );
	rlua_pushinstanceSP = reinterpret_cast< decltype( rlua_pushinstanceSP ) >(utils::memory::rebase( "libroblox.so", addresses::rlua_pushinstance ) );
	rlua_newthread = reinterpret_cast< decltype( rlua_newthread ) >(utils::memory::rebase( "libroblox.so", addresses::rlua_newthread ) );
	rbxspawn = reinterpret_cast< decltype( rbxspawn ) >(utils::memory::rebase( "libroblox.so", addresses::rbxspawn ) );
}

auto get_context_level_permissions( int identity ) -> std::uintptr_t
{
	unsigned int v1; // r2
    int result; // r0

    v1 = identity - 1;
	result = 0;
	if ( v1 <= 8 )
	{
		switch ( v1 )
	    {
		    case 0u:
	        case 3u:
		        result = 3;
		        break;
	        case 1u:
		        return result;
	        case 2u:
	        case 5u:
		        result = 11;
		        break;
	        case 4u:
		        result = 1;
		        break;
	        case 6u:
	        case 7u:
		        result = 63;
		        break;
	        case 8u:
		        result = 12;
		        break;
	    }
    }

    return result;
}

auto roblox::functions::set_identity( lua_State* ls, int identity ) -> void
{
	auto permissions = get_context_level_permissions( identity ) | 0x1FF00;
	auto ES = reinterpret_cast<uintptr_t>(ls->userdata);
    if ( ES ) 
    {
		*reinterpret_cast<uintptr_t*>(ES + 48) = permissions;
		*reinterpret_cast<uintptr_t*>(ES + 24) = identity;
		*reinterpret_cast<uintptr_t*>(ES + 72) = 0xFFFFFFFF; // -1
	}
	
	auto context = reinterpret_cast<uintptr_t>(rbx_getthreadcontext(ls));
	if ( context )
	{
		int v17; // r5
		switch ( identity )
	    {
	    case 1:
	    case 4:
	      v17 = 3;
	      break;
	    case 3:
	    case 6:
	      v17 = 11;
	      break;
	    case 5:
	      v17 = 1;
	      break;
	    case 7:
	    case 8:
	      v17 = 63;
	      break;
	    case 9:
	      v17 = 12;
	      break;
	    default:
	      break;
	    }
		
		std::uintptr_t v18 = v17 | permissions & 0xFFFFFF00;
	    *reinterpret_cast<uintptr_t*>(context + 32) = v18; // capabilities
	    //*reinterpret_cast<uintptr_t*>(context + 36) = 0; // ES + 52
	    *reinterpret_cast<uintptr_t*>(context + 40) = 0; // function
	    *reinterpret_cast<uintptr_t*>(context) = identity; // identity
    }
}