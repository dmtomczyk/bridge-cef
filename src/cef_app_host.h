#pragma once

#include <memory>
#include <string>
#include <vector>

#include "cef_backend.h"
#include "cef_browser_handler.h"
#include "cef_integration_bridge.h"
#include "cef_runtime_host.h"
#include "include/cef_app.h"

struct CefLaunchConfig {
    std::string initial_url;
    std::string home_url;
    std::string profile_label;
    std::vector<std::string> dev_open_tab_urls{};
    std::vector<std::string> dev_popup_tab_urls{};
    int dev_active_tab_index = 0;  // 1-based; 0 disables forced activation.
    bool use_osr = false;
    bool use_alloy_style = true;
    bool use_native = false;
    bool quit_after_first_frame = false;
    bool verify_presentation_v2 = false;
};

class CefAppHost : public CefApp, public CefBrowserProcessHandler {
public:
    struct ClosedTabEntry {
        int tab_id = 0;
        CefTabPageKind page_kind = CefTabPageKind::normal;
        std::string url;
        std::string title;
    };

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
    std::string DefaultNewTabUrl() const;
    CefRefPtr<CefBrowserHandler> CreateTabHandler(bool active_on_host, CefTabPageKind page_kind);
    bool CreateTab(const std::string& url, bool activate, CefTabPageKind page_kind = CefTabPageKind::normal);
    bool HandlePopupRequest(const std::string& url, cef_window_open_disposition_t disposition);
    bool ActivateTab(std::size_t index);
    bool AdvanceActiveTab(int delta);
    bool CloseTab(std::size_t index);
    bool CloseActiveTab();
    bool ReopenClosedTab();
    bool CreateNewTab();
    void RememberClosedTab(const CefBrowserHandler& handler);
    void OnTabClosed(int tab_id);
    void SyncTabUi();
    void RunStartupValidationHooks();

    CefLaunchConfig launch_config_{};
    std::string last_runtime_error_;
    bridge::cef::CefIntegrationBridge::Ptr bridge_ = std::make_shared<bridge::cef::CefIntegrationBridge>();
    std::unique_ptr<CefRuntimeWindowHost> osr_host_{};
    std::vector<CefRefPtr<CefBrowserHandler>> tab_handlers_{};
    std::vector<ClosedTabEntry> closed_tabs_{};
    std::size_t active_tab_index_ = 0;
    int next_tab_id_ = 1;
    bool context_initialized_ = false;

    IMPLEMENT_REFCOUNTING(CefAppHost);
};
