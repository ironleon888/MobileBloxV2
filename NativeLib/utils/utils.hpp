#pragma once

#include <lz4/lz4.h>
#include <cstdint>
#include <random>
#include <string>
#include <sstream>

namespace utils {
	const std::string random_str( std::size_t length );
	const std::string random_bytes( std::size_t length );
	
	namespace memory {
		std::uintptr_t find_lib(const char* const library);
		void wait_for_lib(const char* const libname);
		std::uintptr_t rebase(const char* const libraryName, std::uintptr_t relativeAddr);
		bool is_lib_loaded(const char* const libraryName);
	}
	
	namespace backtrace {
	    void logcat( std::size_t max );
	}
	
	namespace lz4
	{
	    std::string compress(const std::string& data);
	    std::string decompress(const std::string& data, int size);
	}
	
	namespace JNI {
		extern JNIEnv* env;
		
		jobject getGlobalContext( );
		void set_clipboard_data(const std::string& text);
	}
}