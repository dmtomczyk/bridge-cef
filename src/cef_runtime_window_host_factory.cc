#include "cef_runtime_window_host_factory.h"

#if defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_LINUX)
#include "cef_osr_host_gtk.h"
#elif defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_WINDOWS)
#include "win/cef_runtime_window_host_win.h"
#endif

std::unique_ptr<CefRuntimeWindowHost> CreatePlatformRuntimeWindowHost() {
#if defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_LINUX)
    return std::make_unique<CefOsrHostGtk>();
#elif defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_WINDOWS)
    return std::make_unique<CefRuntimeWindowHostWin>();
#else
    return nullptr;
#endif
}
