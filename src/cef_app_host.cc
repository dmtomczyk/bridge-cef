#include "cef_app_host.h"

#include <string>

#if defined(CEF_X11)
#include <X11/Xlib.h>
#endif

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "cef_browser_handler.h"
#include "cef_osr_host_gtk.h"

namespace {

void ApplyOsrSafeSwitches(CefRefPtr<CefCommandLine> command_line) {
    if (!command_line) {
        return;
    }
    bool use_osr = command_line->HasSwitch("use-osr");
    if (!use_osr) {
        auto global = CefCommandLine::GetGlobalCommandLine();
        use_osr = global && global->HasSwitch("use-osr");
    }
    if (!use_osr) {
        return;
    }
    command_line->AppendSwitch("disable-gpu");
    command_line->AppendSwitch("disable-gpu-compositing");
    command_line->AppendSwitch("disable-surfaces");
    command_line->AppendSwitch("disable-gpu-shader-disk-cache");
    command_line->AppendSwitch("disable-background-networking");
    command_line->AppendSwitch("disable-component-update");
    command_line->AppendSwitch("disable-default-apps");
    command_line->AppendSwitch("disable-extensions");
    command_line->AppendSwitch("disable-sync");
    command_line->AppendSwitch("metrics-recording-only");
    command_line->AppendSwitch("no-first-run");
    command_line->AppendSwitch("no-default-browser-check");
    command_line->AppendSwitch("allow-pre-commit-input");
    command_line->AppendSwitchWithValue("password-store", "basic");
    command_line->AppendSwitchWithValue("use-angle", "swiftshader");
}

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

void CefAppHost::OnBeforeCommandLineProcessing(const CefString& process_type,
                                               CefRefPtr<CefCommandLine> command_line) {
    (void)process_type;
    ApplyOsrSafeSwitches(command_line);
}

void CefAppHost::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) {
    ApplyOsrSafeSwitches(command_line);
}

void CefAppHost::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();
    context_initialized_ = true;
}

void CefAppHost::CreateInitialBrowser() {
    CEF_REQUIRE_UI_THREAD();
    if (!context_initialized_) {
        LOG(ERROR) << "CreateInitialBrowser called before OnContextInitialized";
        return;
    }

    auto command_line = CefCommandLine::GetGlobalCommandLine();
    const bool use_alloy_style = !command_line->HasSwitch("disable-alloy-style");
    const bool use_osr = command_line->HasSwitch("use-osr");
    const cef_runtime_style_t runtime_style =
        use_alloy_style ? CEF_RUNTIME_STYLE_ALLOY : CEF_RUNTIME_STYLE_DEFAULT;

    CefBrowserSettings browser_settings;
    if (use_osr) {
        browser_settings.windowless_frame_rate = 30;
    }

    std::string url = command_line->GetSwitchValue("url");
    if (url.empty()) {
        url = "https://example.com";
    }

    bridge::cef::InitParams params;
    params.initial_url = url;
    params.use_alloy_style = use_alloy_style;
    std::string init_error;
    bridge_->initialize(params, &init_error);

    if (use_osr) {
        if (!osr_host_) {
            osr_host_ = std::make_unique<CefOsrHostGtk>();
        }
        if (!osr_host_->Initialize()) {
            LOG(ERROR) << "Failed to initialize GTK/X11 OSR host";
            return;
        }
    }

    CefRefPtr<CefBrowserHandler> handler(
        new CefBrowserHandler(use_alloy_style, use_osr, bridge_->backend(), bridge_, osr_host_.get()));

    const bool use_views = !command_line->HasSwitch("use-native");
    if (use_osr) {
        CefWindowInfo window_info;
        window_info.SetAsWindowless(osr_host_->parent_handle());
        window_info.shared_texture_enabled = false;
        window_info.external_begin_frame_enabled = false;
        window_info.runtime_style = runtime_style;
        CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, nullptr, nullptr);
    } else if (use_views) {
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
