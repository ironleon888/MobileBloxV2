#pragma once

#include <queue>
#include <Luau/Compiler.h>
#include <Luau/BytecodeBuilder.h>
#include <Luau/BytecodeUtils.h>
#include <mutex>

#include "../taskscheduler/taskscheduler.hpp"
#include "../exploit.hpp"
#include "../roblox/update.hpp"

namespace exploit
{
    class bytecode_encoder_t : public Luau::BytecodeEncoder {
        inline void encode(uint32_t* data, size_t count) override 
        {
            for (auto i = 0u; i < count;) 
            {
                auto& opcode = *reinterpret_cast<uint8_t*>(data + i);
                i += Luau::getOpLength(LuauOpcode(opcode));
                opcode *= 227;
            }
        }
    };
    
    static std::uintptr_t DummyLScript = 0;
    class execution {
        public:
        static auto get_singleton( ) -> execution* {
            static execution* _thiz = nullptr;

            if (_thiz == nullptr)
                _thiz = new execution( );

            return _thiz;
        }
        
        auto hook_script_job( std::uintptr_t job ) -> void;
    
        auto execute(const std::string& script) -> void;
        auto schedule(const std::string& script) -> void;
        auto schedule_thread(lua_State* ls, int nargs) -> void;
    
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