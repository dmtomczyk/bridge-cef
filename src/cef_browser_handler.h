#pragma once

#include <list>

#include "include/cef_client.h"

class CefBrowserHandler : public CefClient,
                     public CefDisplayHandler,
                     public CefLifeSpanHandler,
                     public CefLoadHandler {
public:
    explicit CefBrowserHandler(bool is_alloy_style);
    ~CefBrowserHandler() override;

    static CefBrowserHandler* GetInstance();

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }

    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
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
    using BrowserList = std::list<CefRefPtr<CefBrowser>>;
    BrowserList browser_list_;
    bool is_closing_ = false;

    IMPLEMENT_REFCOUNTING(CefBrowserHandler);
};
