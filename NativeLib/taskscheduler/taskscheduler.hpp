#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <lua.h>

// this was increased by + 112.
// so datamodeljobs also now have sc at + 360 ( 248 + 112 = 360 )
// this not used because string name issues but kept as reference 
struct rbxjob_t
{
	uint8_t gap0[120];
	std::string name; // now at 120
	uint8_t gap1[16];
	double time;
	uint8_t gap2[16];
	double time_spend;
	uint8_t gap3[8];
	uintptr_t state;
};

class taskscheduler
{
private:
	lua_State* mbState;
	std::uintptr_t ScriptContext;

public:
	static auto get_singleton( ) -> taskscheduler* {
		static taskscheduler* _thiz = nullptr;

		if (_thiz == nullptr)
			_thiz = new taskscheduler( );

		return _thiz;
	}
	
	auto get_ExploitState( ) -> lua_State* { return this->mbState; }
	auto set_ExploitState(lua_State* v) -> void { this->mbState = v; }
	
	auto get_CurrentSC( ) -> std::uintptr_t { return this->ScriptContext; }
	auto set_CurrentSC(uintptr_t v) -> void { this->ScriptContext = v; }
	
	auto get_jobs( ) -> std::vector<std::uintptr_t>;
	auto log_jobs( ) -> void;
	auto get_job_by_name(const std::string& name) -> std::uintptr_t;
	auto hook_job(uintptr_t job, void* hook) -> std::uintptr_t;
	
    auto get_scriptcontext( ) -> std::uintptr_t;
    auto get_mainstate( ) -> lua_State*;
    
    auto find_fpscap( ) -> std::uintptr_t;
	auto set_fpscap( double cap ) -> void;
	auto get_fpscap( ) -> double;
};