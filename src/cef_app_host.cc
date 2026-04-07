#include "cef_app_host.h"

#include <algorithm>
#include <cstddef>
#include <string>

#include "include/base/cef_callback.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "cef_browser_handler.h"
#include "cef_runtime_window_host_factory.h"

namespace {

void ApplyOsrSafeSwitches(CefRefPtr<CefCommandLine> command_line, bool force_use_osr) {
    if (!command_line) {
        return;
    }
    bool use_osr = force_use_osr || command_line->HasSwitch("use-osr");
    if (!use_osr) {
        auto global = CefCommandLine::GetGlobalCommandLine();
        use_osr = global && global->HasSwitch("use-osr");
    }
    if (!use_osr) {
        return;
    }
    command_line->AppendSwitch("use-osr");
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
    command_line->AppendSwitchWithValue("ozone-platform", "x11");
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
    ApplyOsrSafeSwitches(command_line, launch_config_.use_osr);
}

void CefAppHost::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) {
    ApplyOsrSafeSwitches(command_line, launch_config_.use_osr);
}

void CefAppHost::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();
    context_initialized_ = true;
}

std::string CefAppHost::DefaultNewTabUrl() const {
    return launch_config_.home_url.empty() ? std::string("about:blank") : launch_config_.home_url;
}

CefRefPtr<CefBrowserHandler> CefAppHost::CreateTabHandler(bool active_on_host, CefTabPageKind page_kind) {
    CEF_REQUIRE_UI_THREAD();
    const int tab_id = next_tab_id_++;
    CefRefPtr<CefBrowserHandler> handler(new CefBrowserHandler(launch_config_.use_alloy_style,
                                                               launch_config_.use_osr,
                                                               launch_config_.quit_after_first_frame,
                                                               launch_config_.verify_presentation_v2,
                                                               bridge_->backend(),
                                                               bridge_,
                                                               osr_host_.get(),
                                                               tab_id,
                                                               page_kind,
                                                               launch_config_.home_url,
                                                               active_on_host));
    handler->SetPopupRequestHandler([this](const std::string& url, auto disposition) {
        return HandlePopupRequest(url, disposition);
    });
    handler->SetAllBrowsersClosedHandler([this](int tab_id) { OnTabClosed(tab_id); });
    tab_handlers_.push_back(handler);
    if (active_on_host) {
        active_tab_index_ = tab_handlers_.empty() ? 0 : (tab_handlers_.size() - 1);
    }
    return handler;
}

bool CefAppHost::CreateTab(const std::string& url, bool activate, CefTabPageKind page_kind) {
    CEF_REQUIRE_UI_THREAD();
    if (!launch_config_.use_osr) {
        LOG(WARNING) << "engine-cef runtime-host create-tab rejected because OSR is disabled";
        return false;
    }
    if (!osr_host_) {
        LOG(WARNING) << "engine-cef runtime-host create-tab rejected because OSR host is missing";
        return false;
    }

    const cef_runtime_style_t runtime_style =
        launch_config_.use_alloy_style ? CEF_RUNTIME_STYLE_ALLOY : CEF_RUNTIME_STYLE_DEFAULT;
    CefBrowserSettings browser_settings;
    browser_settings.windowless_frame_rate = 60;

    const std::size_t previous_active_index = active_tab_index_;
    const std::size_t previous_tab_count = tab_handlers_.size();
    LOG(WARNING) << "engine-cef runtime-host create-tab request url=" << url
                 << " activate=" << (activate ? 1 : 0)
                 << " previous_tab_count=" << previous_tab_count
                 << " previous_active_index=" << previous_active_index;
    if (activate && active_tab_index_ < tab_handlers_.size() && tab_handlers_[active_tab_index_]) {
        tab_handlers_[active_tab_index_]->SetActiveOnHost(false, false);
    }

    CefRefPtr<CefBrowserHandler> handler = CreateTabHandler(activate, page_kind);
    CefWindowInfo window_info;
    window_info.SetAsWindowless(osr_host_->parent_handle());
    window_info.shared_texture_enabled = false;
    window_info.external_begin_frame_enabled = false;
    window_info.runtime_style = runtime_style;
    if (!CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, nullptr, nullptr)) {
        if (!tab_handlers_.empty() && tab_handlers_.back() && tab_handlers_.back()->tab_id() == handler->tab_id()) {
            tab_handlers_.pop_back();
        }
        active_tab_index_ = std::min(previous_active_index, tab_handlers_.empty() ? std::size_t{0} : (tab_handlers_.size() - 1));
        if (activate && active_tab_index_ < tab_handlers_.size() && tab_handlers_[active_tab_index_]) {
            tab_handlers_[active_tab_index_]->SetActiveOnHost(true, true);
        }
        SyncTabUi();
        LOG(WARNING) << "engine-cef runtime-host create-tab failed url=" << url
                     << " restored_active_index=" << active_tab_index_
                     << " tab_count=" << tab_handlers_.size();
        return false;
    }

    SyncTabUi();
    LOG(WARNING) << "engine-cef runtime-host create-tab started url=" << url
                 << " tab_id=" << handler->tab_id()
                 << " activate=" << (activate ? 1 : 0)
                 << " tab_count=" << tab_handlers_.size()
                 << " active_index=" << active_tab_index_;
    return true;
}

bool CefAppHost::HandlePopupRequest(const std::string& url, cef_window_open_disposition_t disposition) {
    CEF_REQUIRE_UI_THREAD();
    const bool activate = disposition != CEF_WOD_NEW_BACKGROUND_TAB;
    LOG(WARNING) << "engine-cef runtime-host popup-request url=" << url
                 << " disposition=" << static_cast<int>(disposition)
                 << " activate=" << (activate ? 1 : 0);
    const bool created = CreateTab(url, activate, CefTabPageKind::normal);
    LOG(WARNING) << "engine-cef runtime-host popup-request result url=" << url
                 << " created=" << (created ? 1 : 0)
                 << " tab_count=" << tab_handlers_.size()
                 << " active_index=" << active_tab_index_;
    return created;
}

bool CefAppHost::ActivateTab(std::size_t index) {
    CEF_REQUIRE_UI_THREAD();
    if (index >= tab_handlers_.size() || !tab_handlers_[index]) {
        LOG(WARNING) << "engine-cef runtime-host activate-tab rejected index=" << index
                     << " tab_count=" << tab_handlers_.size();
        return false;
    }
    if (index == active_tab_index_) {
        LOG(WARNING) << "engine-cef runtime-host activate-tab noop index=" << index;
        return true;
    }
    const std::size_t previous_index = active_tab_index_;
    if (active_tab_index_ < tab_handlers_.size() && tab_handlers_[active_tab_index_]) {
        tab_handlers_[active_tab_index_]->SetActiveOnHost(false, false);
    }
    active_tab_index_ = index;
    tab_handlers_[active_tab_index_]->SetActiveOnHost(true, true);
    SyncTabUi();
    LOG(WARNING) << "engine-cef runtime-host activate-tab switched previous_index=" << previous_index
                 << " new_index=" << active_tab_index_
                 << " tab_count=" << tab_handlers_.size();
    return true;
}

bool CefAppHost::AdvanceActiveTab(int delta) {
    CEF_REQUIRE_UI_THREAD();
    if (tab_handlers_.size() < 2 || delta == 0) {
        return false;
    }
    const std::size_t count = tab_handlers_.size();
    const std::size_t current = active_tab_index_ < count ? active_tab_index_ : 0;
    const int wrapped = static_cast<int>(current) + delta;
    const int normalized = ((wrapped % static_cast<int>(count)) + static_cast<int>(count)) % static_cast<int>(count);
    return ActivateTab(static_cast<std::size_t>(normalized));
}

bool CefAppHost::CloseTab(std::size_t index) {
    CEF_REQUIRE_UI_THREAD();
    if (index >= tab_handlers_.size() || !tab_handlers_[index]) {
        return false;
    }

    auto handler = tab_handlers_[index];
    LOG(WARNING) << "engine-cef runtime-host close-tab request index=" << index
                 << " tab_id=" << handler->tab_id()
                 << " tab_count=" << tab_handlers_.size()
                 << " active_index=" << active_tab_index_;
    RememberClosedTab(*handler);
    const bool closing_active = (index == active_tab_index_);
    if (closing_active && tab_handlers_.size() > 1) {
        const std::size_t next_index = (index + 1 < tab_handlers_.size()) ? (index + 1) : (index - 1);
        if (next_index < tab_handlers_.size() && tab_handlers_[next_index]) {
            handler->SetActiveOnHost(false, false);
            active_tab_index_ = next_index;
            tab_handlers_[active_tab_index_]->SetActiveOnHost(true, true);
            SyncTabUi();
        }
    }

    handler->CloseAllBrowsers(false);
    return true;
}

bool CefAppHost::CloseActiveTab() {
    CEF_REQUIRE_UI_THREAD();
    return CloseTab(active_tab_index_);
}

bool CefAppHost::ReopenClosedTab() {
    CEF_REQUIRE_UI_THREAD();
    while (!closed_tabs_.empty()) {
        const ClosedTabEntry entry = closed_tabs_.back();
        closed_tabs_.pop_back();
        if (entry.page_kind == CefTabPageKind::bridge_home || entry.url.empty()) {
            continue;
        }
        if (CreateTab(entry.url, true, entry.page_kind)) {
            return true;
        }
    }
    return false;
}

bool CefAppHost::CreateNewTab() {
    CEF_REQUIRE_UI_THREAD();
    return CreateTab(DefaultNewTabUrl(), true, CefTabPageKind::bridge_home);
}

void CefAppHost::RememberClosedTab(const CefBrowserHandler& handler) {
    CEF_REQUIRE_UI_THREAD();
    const std::string& url = handler.current_url();
    if (handler.page_kind() == CefTabPageKind::bridge_home ||
        url.empty() || url == "about:blank" || url.rfind("javascript:", 0) == 0) {
        return;
    }
    closed_tabs_.push_back(ClosedTabEntry{handler.tab_id(), handler.page_kind(), url, handler.current_title()});
    constexpr std::size_t kMaxClosedTabs = 64;
    if (closed_tabs_.size() > kMaxClosedTabs) {
        closed_tabs_.erase(closed_tabs_.begin(),
                           closed_tabs_.begin() + static_cast<std::ptrdiff_t>(closed_tabs_.size() - kMaxClosedTabs));
    }
}

void CefAppHost::OnTabClosed(int tab_id) {
    CEF_REQUIRE_UI_THREAD();
    LOG(WARNING) << "engine-cef runtime-host tab-closed notification tab_id=" << tab_id
                 << " tab_count_before=" << tab_handlers_.size()
                 << " active_index_before=" << active_tab_index_;
    std::size_t removed_index = tab_handlers_.size();
    for (std::size_t i = 0; i < tab_handlers_.size(); ++i) {
        if (tab_handlers_[i] && tab_handlers_[i]->tab_id() == tab_id) {
            removed_index = i;
            break;
        }
    }
    if (removed_index == tab_handlers_.size()) {
        return;
    }

    const bool removed_active = removed_index == active_tab_index_;
    tab_handlers_.erase(tab_handlers_.begin() + static_cast<std::ptrdiff_t>(removed_index));
    if (tab_handlers_.empty()) {
        active_tab_index_ = 0;
        SyncTabUi();
        CefQuitMessageLoop();
        return;
    }

    if (removed_active) {
        const std::size_t next_index = std::min(removed_index, tab_handlers_.size() - 1);
        active_tab_index_ = next_index;
        if (tab_handlers_[active_tab_index_]) {
            tab_handlers_[active_tab_index_]->SetActiveOnHost(true, true);
        }
    } else if (removed_index < active_tab_index_) {
        --active_tab_index_;
    }
    SyncTabUi();
}

void CefAppHost::SyncTabUi() {
    CEF_REQUIRE_UI_THREAD();
    if (!osr_host_) {
        return;
    }
    osr_host_->SetTabStatus(active_tab_index_, tab_handlers_.empty() ? 1 : tab_handlers_.size());

    std::vector<CefRuntimeWindowHost::TabUiItem> items;
    items.reserve(tab_handlers_.size());
    for (std::size_t i = 0; i < tab_handlers_.size(); ++i) {
        const auto& handler = tab_handlers_[i];
        if (!handler) {
            continue;
        }
        CefRuntimeWindowHost::TabUiItem item;
        item.tab_id = handler->tab_id();
        item.title = handler->current_title();
        item.url = handler->current_url();
        item.active = (i == active_tab_index_);
        item.loading = handler->is_loading();
        item.can_close = tab_handlers_.size() > 1;
        items.push_back(std::move(item));
    }
    osr_host_->SetTabStripItems(std::move(items));
}

void CefAppHost::RunStartupValidationHooks() {
    CEF_REQUIRE_UI_THREAD();
    for (const auto& url : launch_config_.dev_open_tab_urls) {
        if (url.empty()) {
            continue;
        }
        const bool created = CreateTab(url, false);
        LOG(WARNING) << "engine-cef runtime-host dev startup tab url=" << url
                     << " created=" << (created ? 1 : 0);
    }
    for (const auto& url : launch_config_.dev_popup_tab_urls) {
        if (url.empty()) {
            continue;
        }
        const bool created = HandlePopupRequest(url, CEF_WOD_NEW_FOREGROUND_TAB);
        LOG(WARNING) << "engine-cef runtime-host dev startup popup url=" << url
                     << " created=" << (created ? 1 : 0);
    }
    if (launch_config_.dev_active_tab_index > 0) {
        const std::size_t requested_index = static_cast<std::size_t>(launch_config_.dev_active_tab_index - 1);
        const bool activated = ActivateTab(requested_index);
        LOG(WARNING) << "engine-cef runtime-host dev active tab request index="
                     << launch_config_.dev_active_tab_index << " activated=" << (activated ? 1 : 0);
    } else {
        SyncTabUi();
    }
}

bool CefAppHost::CreateInitialBrowser() {
    CEF_REQUIRE_UI_THREAD();
    last_runtime_error_.clear();
    if (!context_initialized_) {
        last_runtime_error_ = "CreateInitialBrowser called before OnContextInitialized";
        LOG(ERROR) << last_runtime_error_;
        return false;
    }

    const bool use_alloy_style = launch_config_.use_alloy_style;
    const bool use_osr = launch_config_.use_osr;
    const cef_runtime_style_t runtime_style =
        use_alloy_style ? CEF_RUNTIME_STYLE_ALLOY : CEF_RUNTIME_STYLE_DEFAULT;

    CefBrowserSettings browser_settings;
    if (use_osr) {
        browser_settings.windowless_frame_rate = 60;
    }

    const std::string url = launch_config_.initial_url.empty() ? DefaultNewTabUrl() : launch_config_.initial_url;

    bridge::cef::InitParams params;
    params.initial_url = url;
    params.use_alloy_style = use_alloy_style;
    std::string init_error;
    if (!bridge_->initialize(params, &init_error)) {
        last_runtime_error_ = init_error.empty() ? "engine-cef bridge initialize failed" : init_error;
        LOG(ERROR) << last_runtime_error_;
        return false;
    }

    if (use_osr) {
        if (!osr_host_) {
            osr_host_ = CreatePlatformRuntimeWindowHost();
        }
        if (!osr_host_) {
            last_runtime_error_ = "Failed to create runtime window host";
            LOG(ERROR) << last_runtime_error_;
            return false;
        }
        if (!osr_host_->Initialize()) {
            last_runtime_error_ = "Failed to initialize runtime window host";
            LOG(ERROR) << last_runtime_error_;
            return false;
        }
        osr_host_->SetProfileLabel(launch_config_.profile_label);
        CefRuntimeWindowHost::Callbacks callbacks;
        callbacks.tab_advance = [this](int delta) { return AdvanceActiveTab(delta); };
        callbacks.tab_activate = [this](std::size_t index) { return ActivateTab(index); };
        callbacks.tab_close = [this](std::size_t index) { return CloseTab(index); };
        callbacks.tab_close_active = [this]() { return CloseActiveTab(); };
        callbacks.tab_reopen_closed = [this]() { return ReopenClosedTab(); };
        callbacks.tab_new = [this]() { return CreateNewTab(); };
        osr_host_->SetCallbacks(std::move(callbacks));
        SyncTabUi();
    }

    const CefTabPageKind initial_page_kind = launch_config_.initial_url.empty() ? CefTabPageKind::bridge_home : CefTabPageKind::normal;
    CefRefPtr<CefBrowserHandler> handler = CreateTabHandler(true, initial_page_kind);

    const bool use_views = !launch_config_.use_native;
    if (use_osr) {
        CefWindowInfo window_info;
        window_info.SetAsWindowless(osr_host_->parent_handle());
        window_info.shared_texture_enabled = false;
        window_info.external_begin_frame_enabled = false;
        window_info.runtime_style = runtime_style;
        if (!CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, nullptr, nullptr)) {
            last_runtime_error_ = "CefBrowserHost::CreateBrowser failed for OSR browser";
            LOG(ERROR) << last_runtime_error_;
            return false;
        }
        if (!launch_config_.dev_open_tab_urls.empty() || !launch_config_.dev_popup_tab_urls.empty() ||
            launch_config_.dev_active_tab_index > 0) {
            CefPostTask(TID_UI,
                        base::BindOnce(&CefAppHost::RunStartupValidationHooks,
                                       base::Unretained(this)));
        }
        return true;
    }

    if (use_views) {
        CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
            handler, url, browser_settings, nullptr, nullptr,
            new CefBrowserViewDelegateImpl(runtime_style));
        if (!browser_view) {
            last_runtime_error_ = "CefBrowserView::CreateBrowserView returned null";
            LOG(ERROR) << last_runtime_error_;
            return false;
        }
        CefWindow::CreateTopLevelWindow(
            new CefWindowDelegateImpl(browser_view, runtime_style, CEF_SHOW_STATE_NORMAL));
        return true;
    }

    CefWindowInfo window_info;
    window_info.runtime_style = runtime_style;
    if (!CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, nullptr, nullptr)) {
        last_runtime_error_ = "CefBrowserHost::CreateBrowser failed for native browser";
        LOG(ERROR) << last_runtime_error_;
        return false;
    }
    return true;
}

CefRefPtr<CefClient> CefAppHost::GetDefaultClient() {
    if (active_tab_index_ < tab_handlers_.size() && tab_handlers_[active_tab_index_]) {
        return tab_handlers_[active_tab_index_];
    }
    if (!tab_handlers_.empty()) {
        return tab_handlers_.front();
    }
    return CefBrowserHandler::GetInstance();
}
