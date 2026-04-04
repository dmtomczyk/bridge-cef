#include "cef_app_host.h"

#include <string>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "cef_browser_handler.h"

namespace {

class CefWindowDelegateImpl : public CefWindowDelegate {
public:
    CefWindowDelegateImpl(CefRefPtr<CefBrowserView> browser_view,
                        cef_runtime_style_t runtime_style,
                        cef_show_state_t initial_show_state)
        : browser_view_(browser_view),
          runtime_style_(runtime_style),
          initial_show_state_(initial_show_state) {}

    void OnWindowCreated(CefRefPtr<CefWindow> window) override {
        window->AddChildView(browser_view_);
        if (initial_show_state_ != CEF_SHOW_STATE_HIDDEN) {
            window->Show();
        }
    }

    void OnWindowDestroyed(CefRefPtr<CefWindow> window) override {
        browser_view_ = nullptr;
    }

    bool CanClose(CefRefPtr<CefWindow> window) override {
        auto browser = browser_view_->GetBrowser();
        return browser ? browser->GetHost()->TryCloseBrowser() : true;
    }

    CefSize GetPreferredSize(CefRefPtr<CefView> view) override {
        return CefSize(1280, 800);
    }

    cef_show_state_t GetInitialShowState(CefRefPtr<CefWindow> window) override {
        return initial_show_state_;
    }

    cef_runtime_style_t GetWindowRuntimeStyle() override {
        return runtime_style_;
    }

private:
    CefRefPtr<CefBrowserView> browser_view_;
    const cef_runtime_style_t runtime_style_;
    const cef_show_state_t initial_show_state_;

    IMPLEMENT_REFCOUNTING(CefWindowDelegateImpl);
};

class CefBrowserViewDelegateImpl : public CefBrowserViewDelegate {
public:
    explicit CefBrowserViewDelegateImpl(cef_runtime_style_t runtime_style)
        : runtime_style_(runtime_style) {}

    bool OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView> browser_view,
                                   CefRefPtr<CefBrowserView> popup_browser_view,
                                   bool is_devtools) override {
        CefWindow::CreateTopLevelWindow(
            new CefWindowDelegateImpl(popup_browser_view, runtime_style_, CEF_SHOW_STATE_NORMAL));
        return true;
    }

    cef_runtime_style_t GetBrowserRuntimeStyle() override {
        return runtime_style_;
    }

private:
    const cef_runtime_style_t runtime_style_;

    IMPLEMENT_REFCOUNTING(CefBrowserViewDelegateImpl);
};

}  // namespace

void CefAppHost::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();

    auto command_line = CefCommandLine::GetGlobalCommandLine();
    const bool use_alloy_style = !command_line->HasSwitch("disable-alloy-style");
    const cef_runtime_style_t runtime_style =
        use_alloy_style ? CEF_RUNTIME_STYLE_ALLOY : CEF_RUNTIME_STYLE_DEFAULT;

    CefRefPtr<CefBrowserHandler> handler(new CefBrowserHandler(use_alloy_style));
    CefBrowserSettings browser_settings;

    std::string url = command_line->GetSwitchValue("url");
    if (url.empty()) {
        url = "https://example.com";
    }

    const bool use_views = !command_line->HasSwitch("use-native");
    if (use_views) {
        CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
            handler, url, browser_settings, nullptr, nullptr,
            new CefBrowserViewDelegateImpl(runtime_style));
        CefWindow::CreateTopLevelWindow(
            new CefWindowDelegateImpl(browser_view, runtime_style, CEF_SHOW_STATE_NORMAL));
    } else {
        CefWindowInfo window_info;
        window_info.runtime_style = runtime_style;
        CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, nullptr, nullptr);
    }
}

CefRefPtr<CefClient> CefAppHost::GetDefaultClient() {
    return CefBrowserHandler::GetInstance();
}
