#pragma once

#include <queue>
#include <Luau/Compiler.h>
#include <mutex>

#include "../taskscheduler/taskscheduler.hpp"
#include "../exploit.hpp"
#include "../roblox/update.hpp"

namespace exploit
{
	static std::uintptr_t DummyLScript = 0;
	class execution {
		public:
		static auto get_singleton( ) -> execution* {
			static execution* _thiz = nullptr;

			if (_thiz == nullptr)
				_thiz = new execution( );

			return _thiz;
		}
		
		auto hook_script_job( ) -> void;
	
	    auto execute(const std::string& script) -> void;
	    auto schedule(const std::string& script) -> void;
	
	    auto squeue_empty( ) -> bool;
		auto squeue_top( ) -> std::string;
		
		auto yqueue_empty( ) -> bool;
		auto yqueue_top( ) -> std::pair<roblox::structs::live_thread_ref*, int>;
		
		auto clearqueues( ) -> void;
		
	    private:
	    std::queue<std::string> script_queue;
	    std::queue<std::pair<roblox::structs::live_thread_ref*, int>> yield_queue; //reference, nargs
	};
}