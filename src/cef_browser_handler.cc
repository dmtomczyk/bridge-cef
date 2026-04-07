#include "cef_browser_handler.h"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>

#if defined(__linux__)
#include <unistd.h>
#endif

#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

namespace {

CefBrowserHandler* g_instance = nullptr;

std::string GetDataURI(const std::string& data, const std::string& mime_type) {
    return "data:" + mime_type + ";base64," +
           CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}

bool IsMeaningfulPopupTarget(const std::string& url) {
    if (url.empty()) {
        return false;
    }
    if (url == "about:blank") {
        return false;
    }
    if (url.rfind("javascript:", 0) == 0) {
        return false;
    }
    return true;
}

bool IsTabOrPopupDisposition(CefLifeSpanHandler::WindowOpenDisposition disposition) {
    switch (disposition) {
        case CEF_WOD_NEW_FOREGROUND_TAB:
        case CEF_WOD_NEW_BACKGROUND_TAB:
        case CEF_WOD_NEW_POPUP:
        case CEF_WOD_NEW_WINDOW:
        case CEF_WOD_OFF_THE_RECORD:
            return true;
        default:
            return false;
    }
}

std::filesystem::path DefaultDownloadDirectory() {
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return std::filesystem::path(home) / "Downloads" / "BRIDGE";
    }
#if defined(__unix__) || defined(__APPLE__)
    return std::filesystem::path("/tmp") /
           (std::string("bridge-downloads-user-") + std::to_string(::getuid()));
#else
    return std::filesystem::temp_directory_path() / "bridge-downloads";
#endif
}

std::string SanitizedDownloadName(const std::string& suggested_name) {
    const std::string filename = std::filesystem::path(suggested_name).filename().string();
    return filename.empty() ? std::string("bridge-download") : filename;
}

std::string DescribeMediaPermissions(uint32_t permissions) {
    std::vector<std::string> labels;
    if ((permissions & CEF_MEDIA_PERMISSION_DEVICE_AUDIO_CAPTURE) != 0) {
        labels.emplace_back("mic");
    }
    if ((permissions & CEF_MEDIA_PERMISSION_DEVICE_VIDEO_CAPTURE) != 0) {
        labels.emplace_back("camera");
    }
    if ((permissions & CEF_MEDIA_PERMISSION_DESKTOP_AUDIO_CAPTURE) != 0) {
        labels.emplace_back("desktop-audio");
    }
    if ((permissions & CEF_MEDIA_PERMISSION_DESKTOP_VIDEO_CAPTURE) != 0) {
        labels.emplace_back("desktop-video");
    }
    if (labels.empty()) {
        return std::string("none/unknown(") + std::to_string(permissions) + ")";
    }
    std::ostringstream out;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << labels[i];
    }
    return out.str();
}

std::string DescribePermissionRequests(uint32_t permissions) {
    struct PermissionBit {
        uint32_t bit;
        const char* label;
    };
    static constexpr PermissionBit kKnown[] = {
        {CEF_PERMISSION_TYPE_GEOLOCATION, "geolocation"},
        {CEF_PERMISSION_TYPE_NOTIFICATIONS, "notifications"},
        {CEF_PERMISSION_TYPE_CLIPBOARD, "clipboard"},
        {CEF_PERMISSION_TYPE_MIC_STREAM, "mic-stream"},
        {CEF_PERMISSION_TYPE_CAMERA_STREAM, "camera-stream"},
        {CEF_PERMISSION_TYPE_MULTIPLE_DOWNLOADS, "multiple-downloads"},
        {CEF_PERMISSION_TYPE_POINTER_LOCK, "pointer-lock"},
        {CEF_PERMISSION_TYPE_FILE_SYSTEM_ACCESS, "file-system-access"},
        {CEF_PERMISSION_TYPE_LOCAL_NETWORK_ACCESS, "local-network-access"},
        {CEF_PERMISSION_TYPE_LOCAL_NETWORK, "local-network"},
        {CEF_PERMISSION_TYPE_LOOPBACK_NETWORK, "loopback-network"},
    };
    std::vector<std::string> labels;
    for (const auto& item : kKnown) {
        if ((permissions & item.bit) != 0) {
            labels.emplace_back(item.label);
        }
    }
    if (labels.empty()) {
        return std::string("none/unknown(") + std::to_string(permissions) + ")";
    }
    std::ostringstream out;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << labels[i];
    }
    return out.str();
}

}  // namespace

CefBrowserHandler::CefBrowserHandler(bool is_alloy_style,
                                     bool use_osr,
                                     bool quit_after_first_frame,
                                     bool verify_presentation_v2,
                                     bridge::cef::CefBackend::Ptr backend,
                                     std::shared_ptr<bridge::cef::IIntegrationBridge> bridge,
                                     CefRuntimeWindowHost* runtime_host,
                                     int tab_id,
                                     CefTabPageKind page_kind,
                                     std::string home_url,
                                     bool active_on_host)
    : is_alloy_style_(is_alloy_style),
      use_osr_(use_osr),
      quit_after_first_frame_(quit_after_first_frame),
      verify_presentation_v2_(verify_presentation_v2),
      tab_id_(tab_id),
      page_kind_(page_kind),
      home_url_(std::move(home_url)),
      active_on_host_(active_on_host),
      backend_(std::move(backend)),
      bridge_(std::move(bridge)),
      runtime_host_(runtime_host) {
    g_instance = this;
}

CefBrowserHandler::~CefBrowserHandler() {
    g_instance = nullptr;
}

CefBrowserHandler* CefBrowserHandler::GetInstance() {
    return g_instance;
}

void CefBrowserHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        const CefString& url) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    if (frame && frame->IsMain()) {
        current_url_ = url.ToString();
        page_kind_ = (!home_url_.empty() && current_url_ == home_url_) ? CefTabPageKind::bridge_home
                                                                        : CefTabPageKind::normal;
    }
    if (frame && frame->IsMain() && backend_) {
        backend_->observe_address_change(current_url_);
    }
    if (frame && frame->IsMain() && runtime_host_ && active_on_host_) {
        runtime_host_->SetCurrentUrl(current_url_);
    }
}

void CefBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    CEF_REQUIRE_UI_THREAD();
    current_title_ = title.ToString();
    if (backend_) {
        backend_->observe_title_change(current_title_);
    }
    if (auto browser_view = CefBrowserView::GetForBrowser(browser)) {
        if (auto window = browser_view->GetWindow()) {
            window->SetTitle(title);
        }
    } else if (runtime_host_ && active_on_host_) {
        runtime_host_->SetWindowTitle(current_title_);
    } else if (is_alloy_style_) {
        PlatformTitleChange(browser, title);
    }
}

bool CefBrowserHandler::OnFileDialog(CefRefPtr<CefBrowser> browser,
                                     FileDialogMode mode,
                                     const CefString& title,
                                     const CefString& default_file_path,
                                     const std::vector<CefString>& accept_filters,
                                     const std::vector<CefString>& accept_extensions,
                                     const std::vector<CefString>& accept_descriptions,
                                     CefRefPtr<CefFileDialogCallback> callback) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    (void)accept_filters;

    if (!use_osr_ || !callback || !runtime_host_) {
        return false;
    }

    CefRuntimeWindowHost::FileDialogParams params{
        .mode = mode,
        .title = title.ToString(),
        .default_file_path = default_file_path.ToString(),
        .accept_extensions = {},
        .accept_descriptions = {},
        .callback = callback,
    };
    params.accept_extensions.reserve(accept_extensions.size());
    for (const auto& item : accept_extensions) {
        params.accept_extensions.push_back(item.ToString());
    }
    params.accept_descriptions.reserve(accept_descriptions.size());
    for (const auto& item : accept_descriptions) {
        params.accept_descriptions.push_back(item.ToString());
    }
    return runtime_host_->RunFileDialog(params);
}

bool CefBrowserHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefDownloadItem> download_item,
                                         const CefString& suggested_name,
                                         CefRefPtr<CefBeforeDownloadCallback> callback) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    if (!use_osr_ || !download_item || !callback) {
        return false;
    }

    const std::filesystem::path download_dir = DefaultDownloadDirectory();
    std::error_code ec;
    std::filesystem::create_directories(download_dir, ec);
    if (ec) {
        LOG(ERROR) << "engine-cef runtime-host download directory creation failed: " << ec.message();
        callback->Continue(CefString(), true);
        return true;
    }

    const std::filesystem::path target = download_dir / SanitizedDownloadName(suggested_name.ToString());
    LOG(WARNING) << "engine-cef runtime-host download target: " << target.string();
    callback->Continue(target.string(), false);
    return true;
}

void CefBrowserHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefDownloadItem> download_item,
                                          CefRefPtr<CefDownloadItemCallback> callback) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    (void)callback;
    if (!download_item) {
        return;
    }
    if (download_item->IsComplete()) {
        LOG(WARNING) << "engine-cef runtime-host download complete: " << download_item->GetFullPath().ToString();
    } else if (download_item->IsCanceled()) {
        LOG(WARNING) << "engine-cef runtime-host download canceled: " << download_item->GetURL().ToString();
    }
}

bool CefBrowserHandler::OnRequestMediaAccessPermission(CefRefPtr<CefBrowser> browser,
                                                       CefRefPtr<CefFrame> frame,
                                                       const CefString& requesting_origin,
                                                       uint32_t requested_permissions,
                                                       CefRefPtr<CefMediaAccessCallback> callback) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    (void)frame;
    if (!use_osr_ || !callback) {
        return false;
    }
    LOG(WARNING) << "engine-cef runtime-host denied media permission for origin "
                 << requesting_origin.ToString() << ": " << DescribeMediaPermissions(requested_permissions);
    callback->Cancel();
    return true;
}

bool CefBrowserHandler::OnShowPermissionPrompt(CefRefPtr<CefBrowser> browser,
                                               uint64_t prompt_id,
                                               const CefString& requesting_origin,
                                               uint32_t requested_permissions,
                                               CefRefPtr<CefPermissionPromptCallback> callback) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    if (!use_osr_ || !callback) {
        return false;
    }
    LOG(WARNING) << "engine-cef runtime-host denied permission prompt " << prompt_id << " for origin "
                 << requesting_origin.ToString() << ": " << DescribePermissionRequests(requested_permissions);
    callback->Continue(CEF_PERMISSION_RESULT_DENY);
    return true;
}

void CefBrowserHandler::OnDismissPermissionPrompt(CefRefPtr<CefBrowser> browser,
                                                  uint64_t prompt_id,
                                                  cef_permission_request_result_t result) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    LOG(WARNING) << "engine-cef runtime-host permission prompt dismissed: id=" << prompt_id
                 << " result=" << static_cast<int>(result);
}

bool CefBrowserHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
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
                                      bool* no_javascript_access) {
    CEF_REQUIRE_UI_THREAD();
    (void)popup_id;
    (void)target_frame_name;
    (void)target_disposition;
    (void)user_gesture;
    (void)popupFeatures;
    (void)windowInfo;
    (void)client;
    (void)settings;
    (void)extra_info;
    (void)no_javascript_access;

    if (!use_osr_) {
        return false;
    }

    const std::string url = target_url.ToString();
    if (!IsMeaningfulPopupTarget(url)) {
        LOG(WARNING) << "engine-cef runtime-host blocked non-meaningful popup target: "
                     << (url.empty() ? std::string("<empty>") : url);
        return true;
    }

    const bool prefers_new_surface = IsTabOrPopupDisposition(target_disposition);
    if (popup_request_handler_ && popup_request_handler_(url, target_disposition)) {
        LOG(WARNING) << "engine-cef runtime-host popup opened as internal tab: " << url;
        return true;
    }

    if (!prefers_new_surface) {
        if (frame && frame->IsValid()) {
            LOG(WARNING) << "engine-cef runtime-host popup redirected into current window after tab creation failed: " << url;
            frame->LoadURL(url);
            return true;
        }
        if (browser && browser->GetMainFrame()) {
            LOG(WARNING) << "engine-cef runtime-host popup redirected into current main frame after tab creation failed: " << url;
            browser->GetMainFrame()->LoadURL(url);
            return true;
        }
    }

    if (runtime_host_ && runtime_host_->OpenUrlExternally(url)) {
        LOG(WARNING) << "engine-cef runtime-host popup fell back to external handoff: " << url;
        return true;
    }

    if (prefers_new_surface) {
        LOG(WARNING) << "engine-cef runtime-host blocked popup after tab creation failed for new-surface disposition: "
                     << url;
        return true;
    }

    if (frame && frame->IsValid()) {
        LOG(WARNING) << "engine-cef runtime-host popup late-redirect into current window after external handoff failed: " << url;
        frame->LoadURL(url);
        return true;
    }
    if (browser && browser->GetMainFrame()) {
        LOG(WARNING) << "engine-cef runtime-host popup late-redirect into current main frame after external handoff failed: " << url;
        browser->GetMainFrame()->LoadURL(url);
        return true;
    }
    LOG(WARNING) << "engine-cef runtime-host blocked popup after tab + in-window + external handling failed: " << url;
    return true;
}

void CefBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    browser_list_.push_back(browser);
    if (use_osr_) {
        if (runtime_host_ && active_on_host_) {
            runtime_host_->NotifyBrowserCreated(browser);
        }
        browser->GetHost()->NotifyScreenInfoChanged();
        browser->GetHost()->WasHidden(!active_on_host_);
        browser->GetHost()->SetFocus(active_on_host_);
        browser->GetHost()->WasResized();
    }
}

bool CefBrowserHandler::DoClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    if (use_osr_ && runtime_host_ && active_on_host_) {
        runtime_host_->NotifyBrowserClosing(browser);
    }
    if (browser_list_.size() == 1) {
        is_closing_ = true;
    }
    return false;
}

void CefBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    LOG(WARNING) << "engine-cef runtime-host OnBeforeClose tab_id=" << tab_id_
                 << " active_on_host=" << (active_on_host_ ? 1 : 0)
                 << " browser_count_before=" << browser_list_.size();
    for (auto it = browser_list_.begin(); it != browser_list_.end(); ++it) {
        if ((*it)->IsSame(browser)) {
            browser_list_.erase(it);
            break;
        }
    }
    if (browser_list_.empty()) {
        if (all_browsers_closed_handler_) {
            all_browsers_closed_handler_(tab_id_);
        } else {
            CefQuitMessageLoop();
        }
    }
}

void CefBrowserHandler::SetActiveOnHost(bool active, bool hydrate_host) {
    CEF_REQUIRE_UI_THREAD();
    active_on_host_ = active;

    const auto browser = primary_browser();
    if (!browser) {
        return;
    }

    browser->GetHost()->WasHidden(!active_on_host_);
    browser->GetHost()->SetFocus(active_on_host_);
    if (!active_on_host_) {
        return;
    }

    browser->GetHost()->NotifyScreenInfoChanged();
    browser->GetHost()->WasResized();

    if (!runtime_host_) {
        return;
    }

    runtime_host_->NotifyBrowserCreated(browser);
    runtime_host_->SetCurrentUrl(current_url_);
    runtime_host_->SetLoadingState(is_loading_, can_go_back_, can_go_forward_);
    if (!load_error_text_.empty()) {
        runtime_host_->SetLoadError(load_error_text_, current_url_);
    }
    runtime_host_->SetWindowTitle(current_title_);
    if (hydrate_host && !last_frame_argb_.empty() && last_frame_width_ > 0 && last_frame_height_ > 0) {
        runtime_host_->PresentFrame(last_frame_argb_.data(),
                                last_frame_width_,
                                last_frame_height_,
                                last_frame_stride_bytes_);
    }
}

void CefBrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                             bool isLoading,
                                             bool canGoBack,
                                             bool canGoForward) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    is_loading_ = isLoading;
    can_go_back_ = canGoBack;
    can_go_forward_ = canGoForward;
    if (!isLoading) {
        load_error_text_.clear();
    }
    if (backend_) {
        backend_->observe_loading_state(isLoading, canGoBack, canGoForward);
    }
    if (runtime_host_ && active_on_host_) {
        runtime_host_->SetLoadingState(isLoading, canGoBack, canGoForward);
    }
}

void CefBrowserHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  int httpStatusCode) {
    CEF_REQUIRE_UI_THREAD();
    if (frame && frame->IsMain() && backend_) {
        backend_->observe_load_end();
    }
}

void CefBrowserHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               ErrorCode errorCode,
                               const CefString& errorText,
                               const CefString& failedUrl) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    if (errorCode == ERR_ABORTED) {
        return;
    }
    if (frame && frame->IsMain()) {
        load_error_text_ = errorText.ToString();
    }
    if (backend_ && frame && frame->IsMain()) {
        backend_->observe_load_error(errorText.ToString());
    }
    if (frame && frame->IsMain() && runtime_host_ && active_on_host_) {
        runtime_host_->SetLoadError(errorText.ToString(), failedUrl.ToString());
    }
    if (!is_alloy_style_) {
        return;
    }

    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\"><h2>Failed to load URL "
       << std::string(failedUrl) << " with error " << std::string(errorText)
       << " (" << errorCode << ").</h2></body></html>";
    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

bool CefBrowserHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    CEF_REQUIRE_UI_THREAD();
    if (runtime_host_) {
        return runtime_host_->GetRootScreenRect(rect);
    }
    GetViewRect(browser, rect);
    return true;
}

bool CefBrowserHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser,
                                       int viewX,
                                       int viewY,
                                       int& screenX,
                                       int& screenY) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    if (runtime_host_) {
        return runtime_host_->GetScreenPoint(viewX, viewY, screenX, screenY);
    }
    screenX = viewX;
    screenY = viewY;
    return true;
}

bool CefBrowserHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) {
    CEF_REQUIRE_UI_THREAD();
    if (runtime_host_) {
        return runtime_host_->GetScreenInfo(screen_info);
    }
    CefRect rect;
    GetViewRect(browser, rect);
    screen_info.device_scale_factor = 1.0f;
    screen_info.depth = 32;
    screen_info.depth_per_component = 8;
    screen_info.is_monochrome = false;
    screen_info.rect = rect;
    screen_info.available_rect = rect;
    return true;
}

void CefBrowserHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    if (runtime_host_) {
        runtime_host_->GetViewRect(rect);
        return;
    }
    const auto presentation = backend_ ? backend_->presentation_state() : bridge::cef::PresentationState{};
    const int width = presentation.width > 0 ? presentation.width : 1280;
    const int height = presentation.height > 0 ? presentation.height : 800;
    rect = CefRect(0, 0, width, height);
}

void CefBrowserHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                                PaintElementType type,
                                const RectList& dirtyRects,
                                const void* buffer,
                                int width,
                                int height) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    (void)dirtyRects;
    if (type != PET_VIEW || !backend_ || buffer == nullptr) {
        return;
    }

    const int stride_bytes = width * static_cast<int>(sizeof(std::uint32_t));
    const auto* pixels = static_cast<const std::uint32_t*>(buffer);
    last_frame_width_ = width;
    last_frame_height_ = height;
    last_frame_stride_bytes_ = stride_bytes;
    last_frame_argb_.assign(pixels,
                            pixels + static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

    backend_->observe_frame(last_frame_argb_.data(), width, height, stride_bytes);
    if (runtime_host_ && active_on_host_) {
        runtime_host_->PresentFrame(last_frame_argb_.data(), width, height, stride_bytes);
    }
    if (!saw_first_frame_) {
        LOG(WARNING) << "engine-cef osr first frame " << width << "x" << height;
    }
    if (verify_presentation_v2_ && !verified_presentation_v2_) {
        const auto presentation = bridge_ ? bridge_->presentation_state() : backend_->presentation_state();
        std::string copy_error;
        std::vector<std::uint32_t> copied(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
        const bool copy_ok = bridge_
                                 ? bridge_->copy_latest_frame(copied.data(),
                                                              width,
                                                              height,
                                                              width * static_cast<int>(sizeof(std::uint32_t)),
                                                              &copy_error)
                                 : backend_->copy_latest_frame(copied.data(),
                                                               width,
                                                               height,
                                                               width * static_cast<int>(sizeof(std::uint32_t)),
                                                               &copy_error);
        if (presentation.has_frame && presentation.width == width && presentation.height == height && copy_ok) {
            verified_presentation_v2_ = true;
            fprintf(stderr,
                    "engine-cef verified presentation-v2 bridge frame copy %dx%d generation=%llu\n",
                    presentation.width,
                    presentation.height,
                    static_cast<unsigned long long>(presentation.frame_generation));
        } else {
            fprintf(stderr,
                    "engine-cef failed presentation-v2 bridge verification has_frame=%d size=%dx%d copy_ok=%d error=%s\n",
                    presentation.has_frame ? 1 : 0,
                    presentation.width,
                    presentation.height,
                    copy_ok ? 1 : 0,
                    copy_error.c_str());
        }
    }
    if (quit_after_first_frame_ && !saw_first_frame_) {
        saw_first_frame_ = true;
        CefPostTask(TID_UI, base::BindOnce(&CefQuitMessageLoop));
    }
}

void CefBrowserHandler::CloseAllBrowsers(bool force_close) {
    if (!CefCurrentlyOn(TID_UI)) {
        CefPostTask(TID_UI,
                    base::BindOnce(&CefBrowserHandler::CloseAllBrowsers,
                                   CefRefPtr<CefBrowserHandler>(this),
                                   force_close));
        return;
    }
    std::vector<CefRefPtr<CefBrowser>> browsers(browser_list_.begin(), browser_list_.end());
    for (const auto& browser : browsers) {
        if (browser) {
            browser->GetHost()->CloseBrowser(force_close);
        }
    }
}

void CefBrowserHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    (void)browser;
    (void)title;
}

void CefBrowserHandler::PlatformShowWindow(CefRefPtr<CefBrowser> browser) {
    (void)browser;
}
