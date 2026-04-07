#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <utility>

#include "include/cef_client.h"

#include "browser/browser_action.h"
#include "cef_runtime_host.h"

#if defined(CEF_X11)
typedef struct _GtkWidget GtkWidget;
typedef struct _cairo cairo_t;
typedef struct _GdkEventButton GdkEventButton;
typedef struct _GdkEventMotion GdkEventMotion;
typedef struct _GdkEventScroll GdkEventScroll;
typedef struct _GdkEventKey GdkEventKey;
typedef struct _GdkEventCrossing GdkEventCrossing;
typedef struct _GdkEventConfigure GdkEventConfigure;
typedef struct _GdkPixbuf GdkPixbuf;
#endif

class CefOsrHostGtk : public CefRuntimeWindowHost {
public:
    CefOsrHostGtk();
    ~CefOsrHostGtk() override;

    bool Initialize() override;
    bool PresentFrame(const std::uint32_t* argb, int width, int height, int stride_bytes) override;
    void SetWindowTitle(const std::string& title) override;
    void SetProfileLabel(const std::string& profile_label) override;
    void SetCurrentUrl(const std::string& url) override;
    void SetLoadingState(bool is_loading, bool can_go_back, bool can_go_forward) override;
    void SetLoadError(const std::string& error_text, const std::string& failed_url) override;
    void SetTabStatus(std::size_t active_tab_index, std::size_t tab_count) override;
    void SetTabStripItems(std::vector<TabUiItem> items) override;
    void SetCallbacks(Callbacks callbacks) override {
        tab_advance_handler_ = std::move(callbacks.tab_advance);
        tab_activate_handler_ = std::move(callbacks.tab_activate);
        tab_close_handler_ = std::move(callbacks.tab_close);
        tab_close_active_handler_ = std::move(callbacks.tab_close_active);
        tab_reopen_closed_handler_ = std::move(callbacks.tab_reopen_closed);
        tab_new_handler_ = std::move(callbacks.tab_new);
    }
    void SetTabAdvanceHandler(std::function<bool(int)> tab_advance_handler);
    void SetTabActivateHandler(std::function<bool(std::size_t)> tab_activate_handler);
    void SetTabCloseHandler(std::function<bool(std::size_t)> tab_close_handler);
    void SetTabCloseActiveHandler(std::function<bool()> tab_close_active_handler);
    void SetTabReopenClosedHandler(std::function<bool()> tab_reopen_closed_handler);
    void SetTabNewHandler(std::function<bool()> tab_new_handler);

    CefWindowHandle parent_handle() const override { return parent_handle_; }

    bool GetRootScreenRect(CefRect& rect) const override;
    void GetViewRect(CefRect& rect) const override;
    bool GetScreenPoint(int view_x, int view_y, int& screen_x, int& screen_y) const override;
    bool GetScreenInfo(CefScreenInfo& screen_info) const override;
    bool RunFileDialog(const FileDialogParams& params) override;
    bool OpenUrlExternally(const std::string& url) override;

    void NotifyBrowserCreated(CefRefPtr<CefBrowser> browser) override;
    void NotifyBrowserClosing(CefRefPtr<CefBrowser> browser) override;

private:
#if defined(CEF_X11)
    static int DrawCallback(GtkWidget* widget, cairo_t* cr, void* user_data);
    static int ButtonPressCallback(GtkWidget* widget, GdkEventButton* event, void* user_data);
    static int ButtonReleaseCallback(GtkWidget* widget, GdkEventButton* event, void* user_data);
    static int MotionCallback(GtkWidget* widget, GdkEventMotion* event, void* user_data);
    static int ScrollCallback(GtkWidget* widget, GdkEventScroll* event, void* user_data);
    static int KeyPressCallback(GtkWidget* widget, GdkEventKey* event, void* user_data);
    static int KeyReleaseCallback(GtkWidget* widget, GdkEventKey* event, void* user_data);
    static int LeaveNotifyCallback(GtkWidget* widget, GdkEventCrossing* event, void* user_data);
    static int DeleteEventCallback(GtkWidget* widget, void* event, void* user_data);
    static int ConfigureCallback(GtkWidget* widget, GdkEventConfigure* event, void* user_data);
    static void SizeAllocateCallback(GtkWidget* widget, void* allocation, void* user_data);
    static int MapEventCallback(GtkWidget* widget, void* event, void* user_data);
    static int DeferredResizeIdle(void* user_data);
    int Draw(cairo_t* cr);
    int HandleButton(GdkEventButton* event, bool mouse_up);
    int HandleMotion(GdkEventMotion* event, bool mouse_leave);
    int HandleScroll(GdkEventScroll* event);
    int HandleKey(GdkEventKey* event, bool key_up);
    uint32_t CurrentModifiers() const;
    bool SyncViewSizeFromAllocation(bool notify_browser, int fallback_width = 0, int fallback_height = 0);
    void QueueDeferredResizeSync();
    bool HandleChromeClick(int x, int y);
    bool HandleAddressContextMenuClick(int x, int y);
    bool HandleTabStripClick(int x, int y);
    bool ExecuteBrowserAction(browz::core::BrowserAction action);
    void FocusAddressField(bool select_all = true, std::size_t cursor = std::string::npos);
    void BlurAddressField();
    bool NavigateAddressBuffer();
    CefRefPtr<CefFrame> ActiveFrame() const;
    void AddressSelectAll();
    bool AddressHasSelection() const;
    void AddressCopySelection();
    void AddressCutSelection();
    void AddressPasteClipboard();
    std::size_t AddressMaxVisibleChars(int draw_width) const;
    std::size_t AddressVisibleStart(int draw_width) const;
    std::size_t AddressCursorFromPoint(int draw_width, int x) const;
#endif

    int width_ = 1280;
    int height_ = 800;
    int frame_width_ = 0;
    int frame_height_ = 0;
    int frame_stride_bytes_ = 0;
    int browser_view_width_ = 0;
    int browser_view_height_ = 0;
    std::vector<std::uint32_t> frame_argb_{};
    CefWindowHandle parent_handle_ = 0;
#if defined(CEF_X11)
    GtkWidget* window_ = nullptr;
    GtkWidget* drawing_area_ = nullptr;
    GdkPixbuf* brand_icon_pixbuf_ = nullptr;
    GdkPixbuf* brand_overlay_pixbuf_ = nullptr;
#endif
    std::string window_title_{};
    std::string profile_label_{};
    std::string current_url_{};
    std::string address_edit_buffer_{};
    std::size_t address_cursor_ = 0;
    std::string load_error_text_{};
    std::string failed_url_{};
    bool is_loading_ = false;
    bool can_go_back_ = false;
    bool can_go_forward_ = false;
    bool address_focused_ = false;
    bool address_replace_on_type_ = false;
    bool address_context_menu_open_ = false;
    int address_context_menu_x_ = 0;
    int address_context_menu_y_ = 0;
    bool deferred_resize_pending_ = false;
    double pending_scroll_x_ = 0.0;
    double pending_scroll_y_ = 0.0;
    std::size_t active_tab_index_ = 0;
    std::size_t tab_count_ = 1;
    std::vector<TabUiItem> tab_strip_items_{};
    std::function<bool(int)> tab_advance_handler_{};
    std::function<bool(std::size_t)> tab_activate_handler_{};
    std::function<bool(std::size_t)> tab_close_handler_{};
    std::function<bool()> tab_close_active_handler_{};
    std::function<bool()> tab_reopen_closed_handler_{};
    std::function<bool()> tab_new_handler_{};
    CefRefPtr<CefBrowser> browser_{};
};
