#pragma once

#include <list>
#include <memory>

#include "cef_backend.h"
#include "cef_osr_host_gtk.h"
#include "engine_cef/integration_bridge.h"
#include "include/cef_client.h"

class CefBrowserHandler : public CefClient,
                     public CefDialogHandler,
                     public CefDisplayHandler,
                     public CefLifeSpanHandler,
                     public CefLoadHandler,
                     public CefRenderHandler {
public:
    CefBrowserHandler(bool is_alloy_style,
                      bool use_osr,
                      bool quit_after_first_frame,
                      bool verify_presentation_v2,
                      bridge::cef::CefBackend::Ptr backend,
                      std::shared_ptr<bridge::cef::IIntegrationBridge> bridge = nullptr,
                      CefOsrHostGtk* osr_host = nullptr);
    ~CefBrowserHandler() override;

    static CefBrowserHandler* GetInstance();

    CefRefPtr<CefDialogHandler> GetDialogHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return use_osr_ ? this : nullptr; }

    void OnAddressChange(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         const CefString& url) override;
    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
    bool OnFileDialog(CefRefPtr<CefBrowser> browser,
                      FileDialogMode mode,
                      const CefString& title,
                      const CefString& default_file_path,
                      const std::vector<CefString>& accept_filters,
                      const std::vector<CefString>& accept_extensions,
                      const std::vector<CefString>& accept_descriptions,
                      CefRefPtr<CefFileDialogCallback> callback) override;
    bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int popup_id,
                       const CefString& target_url,
                       const CefString& target_frame_name,
                       WindowOpenDisposition target_disposition,
                       bool user_gesture,
                       const CefPopupFeatures& popupFeatures,
                       CefWindowInfo& windowInfo,
                       CefRefPtr<CefClient>& client,
                       CefBrowserSettings& settings,
                       CefRefPtr<CefDictionaryValue>& extra_info,
                       bool* no_javascript_access) override;
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
    bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) override;
    bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) override;
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
    const bool verify_presentation_v2_;
    bool saw_first_frame_ = false;
    bool verified_presentation_v2_ = false;
    bridge::cef::CefBackend::Ptr backend_;
    std::shared_ptr<bridge::cef::IIntegrationBridge> bridge_{};
    CefOsrHostGtk* osr_host_ = nullptr;
    using BrowserList = std::list<CefRefPtr<CefBrowser>>;
    BrowserList browser_list_;
    bool is_closing_ = false;

    IMPLEMENT_REFCOUNTING(CefBrowserHandler);
};
