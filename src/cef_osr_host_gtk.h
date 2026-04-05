#pragma once

#include "include/cef_client.h"

#if defined(CEF_X11)
typedef struct _GtkWidget GtkWidget;
#endif

class CefOsrHostGtk {
public:
    CefOsrHostGtk();
    ~CefOsrHostGtk();

    bool Initialize();

    CefWindowHandle parent_handle() const { return parent_handle_; }

    bool GetRootScreenRect(CefRect& rect) const;
    void GetViewRect(CefRect& rect) const;
    bool GetScreenPoint(int view_x, int view_y, int& screen_x, int& screen_y) const;
    bool GetScreenInfo(CefScreenInfo& screen_info) const;

    void NotifyBrowserCreated(CefRefPtr<CefBrowser> browser) const;

private:
    int width_ = 1280;
    int height_ = 800;
    CefWindowHandle parent_handle_ = 0;
#if defined(CEF_X11)
    GtkWidget* window_ = nullptr;
#endif
};
