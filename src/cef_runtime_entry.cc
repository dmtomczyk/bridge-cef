#include "cef_runtime_entry.h"

#include "include/cef_app.h"

void CefRuntimeHost::EnsureApp() const {
    if (!app_) {
        app_ = new CefAppHost;
    }
}

std::shared_ptr<bridge::cef::IIntegrationBridge> CefRuntimeHost::bridge() const {
    EnsureApp();
    return app_->bridge();
}

int CefRuntimeHost::Run(int argc, char* argv[]) const {
    EnsureApp();
    app_->set_launch_config(config_.launch);
    return CefLinuxMainRunner::Run(argc, argv, app_, config_.runner);
}

void CefRuntimeHost::RequestQuit() {
    CefQuitMessageLoop();
}

int RunCefRuntimeEntry(int argc, char* argv[], const CefRuntimeEntryConfig& config) {
    return CefRuntimeHost(config).Run(argc, argv);
}
