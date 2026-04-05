#pragma once

#include <memory>

#include "cef_backend.h"
#include "cef_integration_bridge.h"
#include "cef_osr_host_gtk.h"
#include "include/cef_app.h"

class CefAppHost : public CefApp, public CefBrowserProcessHandler {
public:
    CefAppHost() = default;

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

    void OnBeforeCommandLineProcessing(const CefString& process_type,
                                       CefRefPtr<CefCommandLine> command_line) override;
    void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) override;
    void OnContextInitialized() override;
    CefRefPtr<CefClient> GetDefaultClient() override;

    void CreateInitialBrowser();

    void set_launch_config(const CefLaunchConfig& config) { launch_config_ = config; }
    const CefLaunchConfig& launch_config() const { return launch_config_; }

    bridge::cef::CefBackend::Ptr backend() const { return bridge_->backend(); }
    std::shared_ptr<bridge::cef::IIntegrationBridge> bridge() const { return bridge_; }

private:
    bridge::cef::CefIntegrationBridge::Ptr bridge_ = std::make_shared<bridge::cef::CefIntegrationBridge>();
    std::unique_ptr<CefOsrHostGtk> osr_host_{};
    bool context_initialized_ = false;

    IMPLEMENT_REFCOUNTING(CefAppHost);
};
