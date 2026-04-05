#pragma once

#include <memory>

#include "cef_backend.h"
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

    bridge::cef::CefBackend::Ptr backend() const { return backend_; }

private:
    bridge::cef::CefBackend::Ptr backend_ = std::make_shared<bridge::cef::CefBackend>();
    std::unique_ptr<CefOsrHostGtk> osr_host_{};
    bool context_initialized_ = false;

    IMPLEMENT_REFCOUNTING(CefAppHost);
};
