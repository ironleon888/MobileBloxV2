#include <jni.h>
#include <errno.h>

#include <string>
#include <unistd.h>
#include <sys/resource.h>

#include <android/log.h>

// maybe add a debug if to enable or disable these?
#define TAG "robloxheck"

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))

// easier access
namespace exploit_configuration
{
    const std::string exploit_name = "MobileBlox";
    const std::string exploit_luaChunk = "@MB";
    const std::string exploit_version = "2.0.0";
    const std::string roblox_exploit_version = "2.600.713"; // lets try to be consistent here
}