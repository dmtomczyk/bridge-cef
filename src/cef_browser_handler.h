#pragma once

#include <list>
#include <memory>

#include "cef_backend.h"
#include "include/cef_client.h"

class CefBrowserHandler : public CefClient,
                     public CefDisplayHandler,
                     public CefLifeSpanHandler,
                     public CefLoadHandler {
public:
    CefBrowserHandler(bool is_alloy_style, bridge::cef::CefBackend::Ptr backend);
    ~CefBrowserHandler() override;

    static CefBrowserHandler* GetInstance();

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }

    void OnAddressChange(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         const CefString& url) override;
    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                              bool isLoading,
                              bool canGoBack,
                              bool canGoForward) override;
    void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   int httpStatusCode) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) override;

    void CloseAllBrowsers(bool force_close);

private:
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);
    void PlatformShowWindow(CefRefPtr<CefBrowser> browser);

    const bool is_alloy_style_;
    bridge::cef::CefBackend::Ptr backend_;
    using BrowserList = std::list<CefRefPtr<CefBrowser>>;
    BrowserList browser_list_;
    bool is_closing_ = false;

    IMPLEMENT_REFCOUNTING(CefBrowserHandler);
};
