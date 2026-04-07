#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_dialog_handler.h"

class CefRuntimeWindowHost {
public:
    struct TabUiItem {
        int tab_id = 0;
        std::string title;
        std::string url;
        bool active = false;
        bool loading = false;
        bool can_close = true;
    };

    struct Callbacks {
        std::function<bool(int)> tab_advance;
        std::function<bool(std::size_t)> tab_activate;
        std::function<bool(std::size_t)> tab_close;
        std::function<bool()> tab_close_active;
        std::function<bool()> tab_reopen_closed;
        std::function<bool()> tab_new;
    };

    struct FileDialogParams {
        CefDialogHandler::FileDialogMode mode;
        std::string title;
        std::string default_file_path;
        std::vector<std::string> accept_extensions;
        std::vector<std::string> accept_descriptions;
        CefRefPtr<CefFileDialogCallback> callback;
    };

    virtual ~CefRuntimeWindowHost() = default;

    // Host lifecycle / native surface
    virtual bool Initialize() = 0;
    virtual CefWindowHandle parent_handle() const = 0;

    // Shared-runtime -> host UI hydration
    virtual void SetProfileLabel(const std::string& profile_label) = 0;
    virtual void SetWindowTitle(const std::string& title) = 0;
    virtual void SetCurrentUrl(const std::string& url) = 0;
    virtual void SetLoadingState(bool is_loading, bool can_go_back, bool can_go_forward) = 0;
    virtual void SetLoadError(const std::string& error_text, const std::string& failed_url) = 0;
    virtual void SetTabStatus(std::size_t active_tab_index, std::size_t tab_count) = 0;
    virtual void SetTabStripItems(std::vector<TabUiItem> items) = 0;
    virtual void SetCallbacks(Callbacks callbacks) = 0;

    // Shared-runtime -> host browser/OSR synchronization
    virtual void NotifyBrowserCreated(CefRefPtr<CefBrowser> browser) = 0;
    virtual void NotifyBrowserClosing(CefRefPtr<CefBrowser> browser) = 0;
    virtual bool PresentFrame(const std::uint32_t* argb, int width, int height, int stride_bytes) = 0;

    // CEF render-host geometry queries
    virtual bool GetRootScreenRect(CefRect& rect) const = 0;
    virtual void GetViewRect(CefRect& rect) const = 0;
    virtual bool GetScreenPoint(int view_x, int view_y, int& screen_x, int& screen_y) const = 0;
    virtual bool GetScreenInfo(CefScreenInfo& screen_info) const = 0;

    // Platform services used by shared runtime/browser code
    virtual bool RunFileDialog(const FileDialogParams& params) = 0;
    virtual bool OpenUrlExternally(const std::string& url) = 0;
};
