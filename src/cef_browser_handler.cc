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

CefBrowserHandler::CefBrowserHandler(bool is_alloy_style) : is_alloy_style_(is_alloy_style) {
    g_instance = this;
}

CefBrowserHandler::~CefBrowserHandler() {
    g_instance = nullptr;
}

CefBrowserHandler* CefBrowserHandler::GetInstance() {
    return g_instance;
}

void CefBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    CEF_REQUIRE_UI_THREAD();
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

void CefBrowserHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               ErrorCode errorCode,
                               const CefString& errorText,
                               const CefString& failedUrl) {
    CEF_REQUIRE_UI_THREAD();
    if (!is_alloy_style_ || errorCode == ERR_ABORTED) {
        return;
    }

    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\"><h2>Failed to load URL "
       << std::string(failedUrl) << " with error " << std::string(errorText)
       << " (" << errorCode << ").</h2></body></html>";
    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void CefBrowserHandler::CloseAllBrowsers(bool force_close) {
    if (!CefCurrentlyOn(TID_UI)) {
        CefPostTask(TID_UI, base::BindOnce(&CefBrowserHandler::CloseAllBrowsers, this, force_close));
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
