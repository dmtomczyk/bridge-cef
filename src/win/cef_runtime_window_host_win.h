#pragma once

#include "../cef_runtime_host.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

class CefRuntimeWindowHostWin : public CefRuntimeWindowHost {
public:
    CefRuntimeWindowHostWin() = default;
    ~CefRuntimeWindowHostWin() override;

    bool Initialize() override;
    CefWindowHandle parent_handle() const override;

    void SetProfileLabel(const std::string& profile_label) override;
    void SetWindowTitle(const std::string& title) override;
    void SetCurrentUrl(const std::string& url) override;
    void SetLoadingState(bool is_loading, bool can_go_back, bool can_go_forward) override;
    void SetLoadError(const std::string& error_text, const std::string& failed_url) override;
    void SetTabStatus(std::size_t active_tab_index, std::size_t tab_count) override;
    void SetTabStripItems(std::vector<TabUiItem> items) override;
    void SetCallbacks(Callbacks callbacks) override;

    void NotifyBrowserCreated(CefRefPtr<CefBrowser> browser) override;
    void NotifyBrowserClosing(CefRefPtr<CefBrowser> browser) override;
    bool PresentFrame(const std::uint32_t* argb, int width, int height, int stride_bytes) override;

    bool GetRootScreenRect(CefRect& rect) const override;
    void GetViewRect(CefRect& rect) const override;
    bool GetScreenPoint(int view_x, int view_y, int& screen_x, int& screen_y) const override;
    bool GetScreenInfo(CefScreenInfo& screen_info) const override;

    bool RunFileDialog(const FileDialogParams& params) override;
    bool OpenUrlExternally(const std::string& url) override;

private:
#if defined(_WIN32)
    static constexpr const wchar_t* kWindowClassName = L"BridgeRuntimeHostWin";
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void SyncViewSizeFromClientRect();
    void NotifyBrowserWasResized();
    void NotifyBrowserFocus(bool focused);
    void PaintFrameToWindow(HDC dc);
    uint32_t CurrentModifiers() const;
    bool HandleMouseMove(UINT message, WPARAM wparam, LPARAM lparam);
    bool HandleMouseButton(UINT message, WPARAM wparam, LPARAM lparam, bool mouse_up);
    bool HandleMouseWheel(UINT message, WPARAM wparam, LPARAM lparam);
    bool HandleKey(UINT message, WPARAM wparam, LPARAM lparam, bool key_up);
    bool HandleChar(UINT message, WPARAM wparam, LPARAM lparam);
    bool HandleAccelerator(UINT message, WPARAM wparam, LPARAM lparam, bool key_up);
#endif

    bool initialized_ = false;
    bool close_requested_ = false;
    bool quit_requested_ = false;
    int view_width_ = 1280;
    int view_height_ = 800;
    int last_frame_width_ = 0;
    int last_frame_height_ = 0;
    int last_frame_stride_bytes_ = 0;
    bool saw_frame_ = false;
    std::vector<std::uint32_t> last_frame_argb_{};
    CefWindowHandle parent_handle_ = 0;
#if defined(_WIN32)
    HINSTANCE instance_ = nullptr;
    HWND window_ = nullptr;
    bool tracking_mouse_leave_ = false;
#endif
    std::string profile_label_{};
    std::string current_url_{};
    std::string window_title_{};
    std::string load_error_text_{};
    std::string failed_url_{};
    bool is_loading_ = false;
    bool can_go_back_ = false;
    bool can_go_forward_ = false;
    std::size_t active_tab_index_ = 0;
    std::size_t tab_count_ = 1;
    std::vector<TabUiItem> tab_strip_items_{};
    Callbacks callbacks_{};
    CefRefPtr<CefBrowser> browser_{};
};
