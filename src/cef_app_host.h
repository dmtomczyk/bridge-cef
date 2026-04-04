#pragma once

#include "include/cef_app.h"

class CefAppHost : public CefApp, public CefBrowserProcessHandler {
public:
    CefAppHost() = default;

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

    void OnContextInitialized() override;
    CefRefPtr<CefClient> GetDefaultClient() override;

private:
    IMPLEMENT_REFCOUNTING(CefAppHost);
};
