#include "cef_runtime_entry.h"

int RunCefRuntimeEntry(int argc, char* argv[], const CefRuntimeEntryConfig& config) {
    CefRefPtr<CefAppHost> app(new CefAppHost);
    app->set_launch_config(config.launch);
    return CefLinuxMainRunner::Run(argc, argv, app, config.runner);
}
