#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "cef_backend.h"
#include "cef_runtime_host.h"
#include "engine_cef/integration_bridge.h"
#include "include/cef_client.h"

enum class CefTabPageKind {
    normal,
    bridge_home,
};

class CefBrowserHandler : public CefClient,
                     public CefDialogHandler,
                     public CefDisplayHandler,
                     public CefDownloadHandler,
                     public CefLifeSpanHandler,
                     public CefLoadHandler,
                     public CefPermissionHandler,
                     public CefRenderHandler {
public:
    CefBrowserHandler(bool is_alloy_style,
                      bool use_osr,
                      bool quit_after_first_frame,
                      bool verify_presentation_v2,
                      bridge::cef::CefBackend::Ptr backend,
                      std::shared_ptr<bridge::cef::IIntegrationBridge> bridge = nullptr,
                      CefRuntimeWindowHost* runtime_host = nullptr,
                      int tab_id = 0,
                      CefTabPageKind page_kind = CefTabPageKind::normal,
                      std::string home_url = {},
                      bool active_on_host = true);
    ~CefBrowserHandler() override;

    static CefBrowserHandler* GetInstance();

    int tab_id() const { return tab_id_; }
    CefTabPageKind page_kind() const { return page_kind_; }
    bool active_on_host() const { return active_on_host_; }
    const std::string& current_url() const { return current_url_; }
    const std::string& current_title() const { return current_title_; }
    bool is_loading() const { return is_loading_; }
    bool can_go_back() const { return can_go_back_; }
    bool can_go_forward() const { return can_go_forward_; }
    const std::string& load_error_text() const { return load_error_text_; }
    CefRefPtr<CefBrowser> primary_browser() const {
        return browser_list_.empty() ? nullptr : browser_list_.front();
    }
    void SetActiveOnHost(bool active, bool hydrate_host = true);
    void SetPopupRequestHandler(
        std::function<bool(const std::string&, WindowOpenDisposition)> popup_request_handler) {
        popup_request_handler_ = std::move(popup_request_handler);
    }
    void SetAllBrowsersClosedHandler(std::function<void(int)> all_browsers_closed_handler) {
        all_browsers_closed_handler_ = std::move(all_browsers_closed_handler);
    }

    CefRefPtr<CefDialogHandler> GetDialogHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefPermissionHandler> GetPermissionHandler() override { return this; }
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
    bool OnBeforeDownload(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefDownloadItem> download_item,
                          const CefString& suggested_name,
                          CefRefPtr<CefBeforeDownloadCallback> callback) override;
    void OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefDownloadItem> download_item,
                           CefRefPtr<CefDownloadItemCallback> callback) override;
    bool OnRequestMediaAccessPermission(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        const CefString& requesting_origin,
                                        uint32_t requested_permissions,
                                        CefRefPtr<CefMediaAccessCallback> callback) override;
    bool OnShowPermissionPrompt(CefRefPtr<CefBrowser> browser,
                                uint64_t prompt_id,
                                const CefString& requesting_origin,
                                uint32_t requested_permissions,
                                CefRefPtr<CefPermissionPromptCallback> callback) override;
    void OnDismissPermissionPrompt(CefRefPtr<CefBrowser> browser,
                                   uint64_t prompt_id,
                                   cef_permission_request_result_t result) override;
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
    const int tab_id_;
    CefTabPageKind page_kind_;
    const std::string home_url_;
    bool active_on_host_ = true;
    bool saw_first_frame_ = false;
    bool verified_presentation_v2_ = false;
    bridge::cef::CefBackend::Ptr backend_;
    std::shared_ptr<bridge::cef::IIntegrationBridge> bridge_{};
    CefRuntimeWindowHost* runtime_host_ = nullptr;
    std::string current_url_{};
    std::string current_title_{};
    std::string load_error_text_{};
    bool is_loading_ = false;
    bool can_go_back_ = false;
    bool can_go_forward_ = false;
    int last_frame_width_ = 0;
    int last_frame_height_ = 0;
    int last_frame_stride_bytes_ = 0;
    std::vector<std::uint32_t> last_frame_argb_{};
    std::function<bool(const std::string&, WindowOpenDisposition)> popup_request_handler_{};
    std::function<void(int)> all_browsers_closed_handler_{};
    using BrowserList = std::list<CefRefPtr<CefBrowser>>;
    BrowserList browser_list_;
    bool is_closing_ = false;

    IMPLEMENT_REFCOUNTING(CefBrowserHandler);
};
