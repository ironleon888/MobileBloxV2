#include "utils.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

#include <iostream>
#include <iomanip>
#include <unwind.h>
#include <dlfcn.h>

namespace utils {
const std::string random_str( std::size_t length )
{
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
    
    std::string str; 
    str.reserve(length); 
    std::generate_n(std::back_inserter(str), length, 
        [&]() { return alphabet[dist(rng)];});
 
    return str;
}

const std::string random_bytes( std::size_t length )
{
    static const char alphabet[] =
        "ABCDEF"
        "0123456789";
    
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
    
    std::string str; 
    str.reserve(length); 
    std::generate_n(std::back_inserter(str), length, 
        [&]() { return alphabet[dist(rng)];});
 
    return str;
}

namespace memory {
    std::uintptr_t find_lib(const char* const library)
    {
        char filename[0xFF] = { 0 }, buffer[1024] = { 0 };
    
        FILE* fp = nullptr;
        std::uintptr_t address = 0;
    
        sprintf(filename, "/proc/self/maps");
    
        fp = fopen(filename, "rt");
    
        if (fp == nullptr) 
        {
            perror("fopen");
            goto done;
        }
    
        while (fgets(buffer, sizeof(buffer), fp)) 
        {
            if (strstr(buffer, library))
            {
                address = static_cast<std::uintptr_t>(strtoul(buffer, NULL, 16));
                goto done;
            }
        }
    
    done:
    
        if (fp) 
            fclose(fp);
    
        return address;
    }
    
    void wait_for_lib(const char* const libname) 
    {
        while ( !is_lib_loaded(libname) )
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::uintptr_t rebase(const char* const libraryName, std::uintptr_t relativeAddr)
    {
        wait_for_lib(libraryName);
        std::uintptr_t libBase = find_lib(libraryName);
        
        if (libBase == 0)
            return 0;
    
        return libBase + relativeAddr;
    }
    
    bool is_lib_loaded(const char* const libraryName)
    {
        char line[512] = { 0 };
    
        FILE* fp = std::fopen("/proc/self/maps", "rt");
    
        if (fp != 0) 
        {
            while (std::fgets(line, sizeof(line), fp)) 
            {
                if (std::strstr(line, libraryName))
                    return true;
            }
    
            fclose(fp);
        }
    
        return false;
    }
}

namespace backtrace {
    struct state
    {
        void** current;
        void** end;
    };
    
    static _Unwind_Reason_Code unwind_callback( struct _Unwind_Context* context, void* arg )
    {
        state* st = static_cast< state* >( arg );
        uintptr_t pc = _Unwind_GetIP( context );
        if ( pc ) {
            const void* addr = reinterpret_cast< void* >( pc );
            const char* symbol = "";
    
            Dl_info info;
            if ( dladdr( addr, &info ) && info.dli_sname ) {
                symbol = info.dli_sname;
            }
    
            std::ostringstream os;
            os << "  #" << std::setw( 2 ) << st->end - st->current << ": " << addr << " (" <<
                reinterpret_cast< void* >( reinterpret_cast< std::uintptr_t >( addr ) - memory::rebase( "libroblox.so", 0 ) ) << ") " << symbol << "\n";
            LOGE( "%s", os.str( ).c_str( ) );
    
            if ( st->current == st->end ) {
                return _URC_NORMAL_STOP;
            }
            else {
                *st->current++ = reinterpret_cast< void* >( pc );
            }
        }
        return _URC_NO_REASON;
    }
    
    void logcat( std::size_t max ) {
        char* buffer = new char[ max ];
        state s = { reinterpret_cast< void** >( &buffer ), reinterpret_cast< void** >( &buffer ) + max };
        _Unwind_Backtrace( unwind_callback, &s );
    }
}

namespace lz4 {
    std::string compress(const std::string& data)
    {
        std::string compressed;
        compressed.resize(LZ4_compressBound(data.length()));
    
        auto compressed_size = LZ4_compress_default(data.c_str(), compressed.data(), data.size(), compressed.length());
    
        compressed.resize(compressed_size);
        return compressed;
    }
    
    std::string decompress(const std::string& data, int size)
    {
        std::string decompressed;
        decompressed.resize(size);
    
        auto decompressed_size = LZ4_decompress_safe(data.c_str(), decompressed.data(), data.size(), size);
    
        decompressed.resize(decompressed_size);
        return decompressed;
    }
}

namespace JNI {
    JNIEnv* env = { };
    
    jobject getGlobalContext( )
    {
        jclass activityThread = env->FindClass(("android/app/ActivityThread"));
        jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, ("currentActivityThread"), ("()Landroid/app/ActivityThread;"));
        jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);
    
        jmethodID getApplication = env->GetMethodID(activityThread, ("getApplication"), ("()Landroid/app/Application;"));
        jobject context = env->CallObjectMethod(at, getApplication);
        return context;
    }
    
    void set_clipboard_data(const std::string& text)
    {
       jobject context_obj = getGlobalContext( );
    
       if( !context_obj )
       {
           LOGE(" [ JNI ERR ] Failed to get Context. wtf?");
           exit(999);
        }
    
        // Classes
        auto context_class = env->FindClass("android/content/Context");
        if (context_class == nullptr){
            LOGE("[ JNI ERR ] context_class was nullptr");
            return;
        }
        
        auto clipboardmanager_class = env->FindClass("android/content/ClipboardManager");
        if (clipboardmanager_class == nullptr){
            LOGE("[ JNI ERR ] clipboardmanager_class was nullptr");
            return;
        }
        
        auto clipdata_class = env->FindClass("android/content/ClipData");
        if (clipdata_class == nullptr){
            LOGE("[ JNI ERR ] clipdata_class was nullptr");
            return;
        }
        
        // Methods
        auto newPlainText_method = env->GetStaticMethodID(clipdata_class, "newPlainText", "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;");
        auto setPrimaryClip_method = env->GetMethodID(clipboardmanager_class, "setPrimaryClip", "(Landroid/content/ClipData;)V");
        auto getSystemService_method = env->GetMethodID(context_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    
        // Calls
        auto clipboardService = env->CallObjectMethod(context_obj, getSystemService_method, env->NewStringUTF("clipboard"));
        
        auto data = env->NewStringUTF(text.c_str());
        auto clip = env->CallStaticObjectMethod(clipdata_class, newPlainText_method, env->NewStringUTF("KryClip"), data);
        env->CallVoidMethod(clipboardService, setPrimaryClip_method, clip);
    }
}
}