#include "cef_runtime_runner.h"

#include "cef_app_host.h"

#if defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_LINUX)
#include "cef_linux_main_runner.h"
#elif defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_WINDOWS)
#include "win/cef_runtime_runner_win.h"
#endif

namespace {

#if defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_LINUX)
class LinuxRuntimeRunner final : public ICefRuntimeRunner {
public:
    int Run(int argc, char* argv[], CefRefPtr<CefAppHost> app, const CefRuntimeRunnerOptions& options) const override {
        CefLinuxMainRunnerOptions linux_options;
        linux_options.use_osr = options.use_osr;
        linux_options.force_x11_backend_for_osr = options.force_platform_osr_backend;
        linux_options.cache_root = options.cache_root;
        return CefLinuxMainRunner::Run(argc, argv, app, linux_options);
    }
};
#endif

}  // namespace

std::unique_ptr<ICefRuntimeRunner> CreatePlatformRuntimeRunner() {
#if defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_LINUX)
    return std::make_unique<LinuxRuntimeRunner>();
#elif defined(ENGINE_CEF_RUNTIME_TARGET_PLATFORM_WINDOWS)
    return std::make_unique<CefRuntimeRunnerWin>();
#else
    return nullptr;
#endif
}
