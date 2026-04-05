#include "cef_browser_handler.h"

#include <sstream>
#include <string>

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

}  // namespace

CefBrowserHandler::CefBrowserHandler(bool is_alloy_style,
                                     bool use_osr,
                                     bridge::cef::CefBackend::Ptr backend,
                                     CefOsrHostGtk* osr_host)
    : is_alloy_style_(is_alloy_style),
      use_osr_(use_osr),
      quit_after_first_frame_(CefCommandLine::GetGlobalCommandLine()->HasSwitch("quit-after-first-frame")),
      verify_presentation_v2_(CefCommandLine::GetGlobalCommandLine()->HasSwitch("verify-presentation-v2")),
      backend_(std::move(backend)),
      osr_host_(osr_host) {
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
    if (frame && frame->IsMain() && backend_) {
        backend_->observe_address_change(url.ToString());
    }
}

void CefBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    CEF_REQUIRE_UI_THREAD();
    if (backend_) {
        backend_->observe_title_change(title.ToString());
    }
    if (auto browser_view = CefBrowserView::GetForBrowser(browser)) {
        if (auto window = browser_view->GetWindow()) {
            window->SetTitle(title);
        }
    } else if (is_alloy_style_) {
        PlatformTitleChange(browser, title);
    }
}

void CefBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    browser_list_.push_back(browser);
    if (use_osr_) {
        if (osr_host_) {
            osr_host_->NotifyBrowserCreated(browser);
        } else {
            browser->GetHost()->NotifyScreenInfoChanged();
            browser->GetHost()->WasHidden(false);
            browser->GetHost()->SetFocus(true);
            browser->GetHost()->WasResized();
        }
    }
}

bool CefBrowserHandler::DoClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    if (browser_list_.size() == 1) {
        is_closing_ = true;
    }
    return false;
}

void CefBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    for (auto it = browser_list_.begin(); it != browser_list_.end(); ++it) {
        if ((*it)->IsSame(browser)) {
            browser_list_.erase(it);
            break;
        }
    }
    if (browser_list_.empty()) {
        CefQuitMessageLoop();
    }
}

void CefBrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                             bool isLoading,
                                             bool canGoBack,
                                             bool canGoForward) {
    CEF_REQUIRE_UI_THREAD();
    if (backend_) {
        backend_->observe_loading_state(isLoading, canGoBack, canGoForward);
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
    if (errorCode == ERR_ABORTED) {
        return;
    }
    if (backend_ && frame && frame->IsMain()) {
        backend_->observe_load_error(errorText.ToString());
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
    if (osr_host_) {
        return osr_host_->GetRootScreenRect(rect);
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
    if (osr_host_) {
        return osr_host_->GetScreenPoint(viewX, viewY, screenX, screenY);
    }
    screenX = viewX;
    screenY = viewY;
    return true;
}

bool CefBrowserHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) {
    CEF_REQUIRE_UI_THREAD();
    if (osr_host_) {
        return osr_host_->GetScreenInfo(screen_info);
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
    if (osr_host_) {
        osr_host_->GetViewRect(rect);
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
    (void)dirtyRects;
    if (type != PET_VIEW || !backend_ || buffer == nullptr) {
        return;
    }
    backend_->observe_frame(static_cast<const std::uint32_t*>(buffer),
                            width,
                            height,
                            width * static_cast<int>(sizeof(std::uint32_t)));
    if (!saw_first_frame_) {
        LOG(WARNING) << "engine-cef osr first frame " << width << "x" << height;
    }
    if (verify_presentation_v2_ && !verified_presentation_v2_) {
        const auto presentation = backend_->presentation_state();
        std::string copy_error;
        std::vector<std::uint32_t> copied(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
        const bool copy_ok = backend_->copy_latest_frame(copied.data(),
                                                         width,
                                                         height,
                                                         width * static_cast<int>(sizeof(std::uint32_t)),
                                                         &copy_error);
        if (presentation.has_frame && presentation.width == width && presentation.height == height && copy_ok) {
            verified_presentation_v2_ = true;
            fprintf(stderr,
                    "engine-cef verified presentation-v2 frame copy %dx%d generation=%llu\n",
                    presentation.width,
                    presentation.height,
                    static_cast<unsigned long long>(presentation.frame_generation));
        } else {
            fprintf(stderr,
                    "engine-cef failed presentation-v2 frame verification has_frame=%d size=%dx%d copy_ok=%d error=%s\n",
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
    for (const auto& browser : browser_list_) {
        browser->GetHost()->CloseBrowser(force_close);
    }
}

void CefBrowserHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    (void)browser;
    (void)title;
}

void CefBrowserHandler::PlatformShowWindow(CefRefPtr<CefBrowser> browser) {
    (void)browser;
}
