#include "cef_linux_main_runner.h"

#include <cstdlib>
#include <filesystem>
#include <unistd.h>

#include "cef_app_host.h"

#if defined(CEF_X11)
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#endif

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"
#include "include/cef_sandbox_linux.h"

namespace {

#if defined(CEF_X11)
int XErrorHandlerImpl(Display* display, XErrorEvent* event) {
    LOG(WARNING) << "X11 error type=" << event->type
                 << " serial=" << event->serial
                 << " error_code=" << static_cast<int>(event->error_code)
                 << " request_code=" << static_cast<int>(event->request_code)
                 << " minor_code=" << static_cast<int>(event->minor_code);
    return 0;
}

int XIOErrorHandlerImpl(Display* display) {
    LOG(WARNING) << "X11 IO error";
    return 0;
}
#endif

}  // namespace

int CefLinuxMainRunner::Run(int argc, char* argv[], CefRefPtr<CefAppHost> app) {
    CefMainArgs main_args(argc, argv);

    const int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    CefSettings settings;
#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif
    const std::filesystem::path root_cache =
        std::filesystem::path("/tmp") /
        (std::string("bridge-engine-cef-proof-cache-") + std::to_string(::getpid()));
    std::filesystem::create_directories(root_cache);
    CefString(&settings.root_cache_path) = root_cache.string();
    CefString(&settings.cache_path) = root_cache.string();
    if (command_line->HasSwitch("use-osr")) {
        settings.windowless_rendering_enabled = true;
    }

#if defined(CEF_X11)
    if (command_line->HasSwitch("use-osr")) {
        gtk_disable_setlocale();
        setenv("GDK_BACKEND", "x11", 1);
        gdk_set_allowed_backends("x11");
        gtk_init(&argc, &argv);
    }
    XSetErrorHandler(XErrorHandlerImpl);
    XSetIOErrorHandler(XIOErrorHandlerImpl);
#endif

    if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
        return CefGetExitCode();
    }

    app->CreateInitialBrowser();
    CefRunMessageLoop();
    CefShutdown();
    return 0;
}
