#include "cef_runtime_runner_win.h"

#include <filesystem>

#include "../cef_app_host.h"
#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

int CefRuntimeRunnerWin::Run(int argc, char* argv[], CefRefPtr<CefAppHost> app, const CefRuntimeRunnerOptions& options) const {
#if defined(_WIN32)
    (void)argc;
    (void)argv;
    LOG(WARNING) << "CefRuntimeRunnerWin: Run begin use_osr=" << (options.use_osr ? 1 : 0)
                 << " cache_root=" << options.cache_root;
    if (!app) {
        LOG(ERROR) << "CefRuntimeRunnerWin: app is null";
        return 1;
    }

    CefMainArgs main_args(GetModuleHandleW(nullptr));
    const int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0) {
        LOG(WARNING) << "CefRuntimeRunnerWin: secondary process exit_code=" << exit_code;
        return exit_code;
    }

    CefSettings settings;
#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif
    const std::filesystem::path root_cache = options.cache_root.empty()
                                                 ? (std::filesystem::temp_directory_path() / "bridge-engine-cef-win-cache")
                                                 : std::filesystem::path(options.cache_root);
    std::error_code ec;
    std::filesystem::create_directories(root_cache, ec);
    if (ec) {
        app->set_runtime_error(std::string("Failed to create Windows runtime cache root: ") + ec.message());
        return 1;
    }
    LOG(WARNING) << "CefRuntimeRunnerWin: runtime profile root=" << root_cache.string();
    CefString(&settings.root_cache_path) = root_cache.string();
    CefString(&settings.cache_path) = root_cache.string();
    settings.persist_session_cookies = 1;
    if (options.use_osr) {
        settings.windowless_rendering_enabled = true;
    }

    if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
        app->set_runtime_error("CefInitialize failed in Windows runtime runner");
        LOG(ERROR) << "CefRuntimeRunnerWin: CefInitialize failed";
        return CefGetExitCode();
    }
    LOG(WARNING) << "CefRuntimeRunnerWin: CefInitialize succeeded";

    if (!app->CreateInitialBrowser()) {
        if (app->last_runtime_error().empty()) {
            app->set_runtime_error("CreateInitialBrowser failed in Windows runtime runner");
        }
        LOG(ERROR) << "CefRuntimeRunnerWin: CreateInitialBrowser failed: " << app->last_runtime_error();
        CefShutdown();
        return 1;
    }
    LOG(WARNING) << "CefRuntimeRunnerWin: initial browser created";

    CefRunMessageLoop();
    LOG(WARNING) << "CefRuntimeRunnerWin: message loop exited";
    CefShutdown();
    LOG(WARNING) << "CefRuntimeRunnerWin: shutdown complete";
    return 0;
#else
    (void)argc;
    (void)argv;
    (void)options;
    if (app) {
        app->set_runtime_error("Windows runtime runner is selected, but this build is not running on Win32");
    }
    return 1;
#endif
}
