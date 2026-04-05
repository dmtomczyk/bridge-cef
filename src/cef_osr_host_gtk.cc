#include "cef_osr_host_gtk.h"

#include "include/base/cef_logging.h"

#if defined(CEF_X11)
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

CefOsrHostGtk::CefOsrHostGtk() = default;

CefOsrHostGtk::~CefOsrHostGtk() {
#if defined(CEF_X11)
    if (window_ != nullptr) {
        gtk_widget_destroy(window_);
        window_ = nullptr;
    }
#endif
    parent_handle_ = 0;
}

bool CefOsrHostGtk::Initialize() {
#if defined(CEF_X11)
    if (window_ != nullptr && parent_handle_ != 0) {
        return true;
    }

    window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (window_ == nullptr) {
        LOG(ERROR) << "CefOsrHostGtk: gtk_window_new returned null";
        return false;
    }

    gtk_window_set_default_size(GTK_WINDOW(window_), width_, height_);
    gtk_window_set_title(GTK_WINDOW(window_), "engine-cef-osr-host");
    gtk_window_set_decorated(GTK_WINDOW(window_), FALSE);
    gtk_widget_show_all(window_);
    gtk_widget_realize(window_);
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    GdkWindow* gdk_window = gtk_widget_get_window(window_);
    if (gdk_window == nullptr) {
        LOG(ERROR) << "CefOsrHostGtk: gtk_widget_get_window returned null";
        return false;
    }
    if (!GDK_IS_X11_WINDOW(gdk_window)) {
        LOG(ERROR) << "CefOsrHostGtk: realized GdkWindow is not X11-backed";
        return false;
    }

    parent_handle_ = static_cast<CefWindowHandle>(GDK_WINDOW_XID(gdk_window));
    if (parent_handle_ == 0) {
        LOG(ERROR) << "CefOsrHostGtk: GDK_WINDOW_XID returned 0";
        return false;
    }
    LOG(WARNING) << "CefOsrHostGtk: initialized X11 host handle=" << parent_handle_;
    return true;
#else
    return false;
#endif
}

bool CefOsrHostGtk::GetRootScreenRect(CefRect& rect) const {
    GetViewRect(rect);
    return true;
}

void CefOsrHostGtk::GetViewRect(CefRect& rect) const {
    rect = CefRect(0, 0, width_, height_);
}

bool CefOsrHostGtk::GetScreenPoint(int view_x, int view_y, int& screen_x, int& screen_y) const {
    screen_x = view_x;
    screen_y = view_y;
    return true;
}

bool CefOsrHostGtk::GetScreenInfo(CefScreenInfo& screen_info) const {
    CefRect rect;
    GetViewRect(rect);
    screen_info.device_scale_factor = 1.0f;
    screen_info.depth = 32;
    screen_info.depth_per_component = 8;
    screen_info.is_monochrome = false;
    screen_info.rect = rect;
    screen_info.available_rect = rect;
    return true;
}

void CefOsrHostGtk::NotifyBrowserCreated(CefRefPtr<CefBrowser> browser) const {
    if (!browser) {
        return;
    }
    browser->GetHost()->NotifyScreenInfoChanged();
    browser->GetHost()->WasHidden(false);
    browser->GetHost()->SetFocus(true);
    browser->GetHost()->WasResized();
}
