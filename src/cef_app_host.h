#pragma once

#include "cef_backend.h"
#include "include/cef_app.h"

class CefAppHost : public CefApp, public CefBrowserProcessHandler {
public:
    CefAppHost() = default;

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

    void OnContextInitialized() override;
    CefRefPtr<CefClient> GetDefaultClient() override;

    bridge::cef::CefBackend::Ptr backend() const { return backend_; }

private:
    bridge::cef::CefBackend::Ptr backend_ = std::make_shared<bridge::cef::CefBackend>();

    IMPLEMENT_REFCOUNTING(CefAppHost);
};
