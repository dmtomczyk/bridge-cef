#include "cef_runtime_entry.h"

#include "include/cef_app.h"

int CefRuntimeHost::Run(int argc, char* argv[]) const {
    return RunCefRuntimeEntry(argc, argv, config_);
}

void CefRuntimeHost::RequestQuit() {
    CefQuitMessageLoop();
}

int RunCefRuntimeEntry(int argc, char* argv[], const CefRuntimeEntryConfig& config) {
    CefRefPtr<CefAppHost> app(new CefAppHost);
    app->set_launch_config(config.launch);
    return CefLinuxMainRunner::Run(argc, argv, app, config.runner);
}
