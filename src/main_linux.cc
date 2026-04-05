#include "cef_app_host.h"

#include <filesystem>

#if defined(CEF_X11)
#include <X11/Xlib.h>
#endif

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"

#if defined(CEF_X11)
namespace {

int XErrorHandlerImpl(Display* display, XErrorEvent* event) {
    LOG(WARNING) << "X error received: type " << event->type << ", serial " << event->serial
                 << ", error_code " << static_cast<int>(event->error_code)
                 << ", request_code " << static_cast<int>(event->request_code)
                 << ", minor_code " << static_cast<int>(event->minor_code);
    return 0;
}

int XIOErrorHandlerImpl(Display* display) {
    return 0;
}

}  // namespace
#endif

NO_STACK_PROTECTOR
int main(int argc, char* argv[]) {
    CefMainArgs main_args(argc, argv);

    CefRefPtr<CefAppHost> app(new CefAppHost);
    const int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

#if defined(CEF_X11)
    XSetErrorHandler(XErrorHandlerImpl);
    XSetIOErrorHandler(XIOErrorHandlerImpl);
#endif

    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    CefSettings settings;
#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif
    const std::filesystem::path root_cache = std::filesystem::path("/tmp") / "bridge-engine-cef-proof-cache";
    std::filesystem::create_directories(root_cache);
    CefString(&settings.root_cache_path) = root_cache.string();
    CefString(&settings.cache_path) = root_cache.string();
    if (command_line->HasSwitch("use-osr")) {
        settings.windowless_rendering_enabled = true;
    }

    if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
        return CefGetExitCode();
    }

    CefRunMessageLoop();
    CefShutdown();
    return 0;
}
