#pragma once

#include <list>
#include <memory>

#include "cef_backend.h"
#include "include/cef_client.h"

class CefBrowserHandler : public CefClient,
                     public CefDisplayHandler,
                     public CefLifeSpanHandler,
                     public CefLoadHandler,
                     public CefRenderHandler {
public:
    CefBrowserHandler(bool is_alloy_style, bool use_osr, bridge::cef::CefBackend::Ptr backend);
    ~CefBrowserHandler() override;

    static CefBrowserHandler* GetInstance();

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return use_osr_ ? this : nullptr; }

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
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType type,
                 const RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height) override;

    void CloseAllBrowsers(bool force_close);

private:
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);
    void PlatformShowWindow(CefRefPtr<CefBrowser> browser);

    const bool is_alloy_style_;
    const bool use_osr_;
    const bool quit_after_first_frame_;
    bool saw_first_frame_ = false;
    bridge::cef::CefBackend::Ptr backend_;
    using BrowserList = std::list<CefRefPtr<CefBrowser>>;
    BrowserList browser_list_;
    bool is_closing_ = false;

    IMPLEMENT_REFCOUNTING(CefBrowserHandler);
};
