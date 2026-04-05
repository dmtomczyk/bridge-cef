#pragma once

#include <memory>
#include <string>

#include "cef_backend.h"
#include "cef_integration_bridge.h"
#include "cef_osr_host_gtk.h"
#include "include/cef_app.h"

struct CefLaunchConfig {
    std::string initial_url = "https://example.com";
    bool use_osr = false;
    bool use_alloy_style = true;
    bool use_native = false;
    bool quit_after_first_frame = false;
    bool verify_presentation_v2 = false;
};

class CefAppHost : public CefApp, public CefBrowserProcessHandler {
public:
    CefAppHost() = default;

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

    void OnBeforeCommandLineProcessing(const CefString& process_type,
                                       CefRefPtr<CefCommandLine> command_line) override;
    void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) override;
    void OnContextInitialized() override;
    CefRefPtr<CefClient> GetDefaultClient() override;

    bool CreateInitialBrowser();

    void set_launch_config(const CefLaunchConfig& config) { launch_config_ = config; }
    const CefLaunchConfig& launch_config() const { return launch_config_; }
    const std::string& last_runtime_error() const { return last_runtime_error_; }

    bridge::cef::CefBackend::Ptr backend() const { return bridge_->backend(); }
    std::shared_ptr<bridge::cef::IIntegrationBridge> bridge() const { return bridge_; }

    void set_runtime_error(std::string error) { last_runtime_error_ = std::move(error); }

private:
    CefLaunchConfig launch_config_{};
    std::string last_runtime_error_;
    bridge::cef::CefIntegrationBridge::Ptr bridge_ = std::make_shared<bridge::cef::CefIntegrationBridge>();
    std::unique_ptr<CefOsrHostGtk> osr_host_{};
    bool context_initialized_ = false;

    IMPLEMENT_REFCOUNTING(CefAppHost);
};
