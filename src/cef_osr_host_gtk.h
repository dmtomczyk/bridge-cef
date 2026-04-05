#pragma once

#include <cstdint>
#include <vector>

#include "include/cef_client.h"

#if defined(CEF_X11)
typedef struct _GtkWidget GtkWidget;
typedef struct _cairo cairo_t;
typedef struct _GdkEventButton GdkEventButton;
typedef struct _GdkEventMotion GdkEventMotion;
typedef struct _GdkEventScroll GdkEventScroll;
typedef struct _GdkEventKey GdkEventKey;
typedef struct _GdkEventCrossing GdkEventCrossing;
typedef struct _GdkEventConfigure GdkEventConfigure;
#endif

class CefOsrHostGtk {
public:
    CefOsrHostGtk();
    ~CefOsrHostGtk();

    bool Initialize();
    bool PresentFrame(const std::uint32_t* argb, int width, int height, int stride_bytes);

    CefWindowHandle parent_handle() const { return parent_handle_; }

    bool GetRootScreenRect(CefRect& rect) const;
    void GetViewRect(CefRect& rect) const;
    bool GetScreenPoint(int view_x, int view_y, int& screen_x, int& screen_y) const;
    bool GetScreenInfo(CefScreenInfo& screen_info) const;

    void NotifyBrowserCreated(CefRefPtr<CefBrowser> browser);

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
    int Draw(cairo_t* cr);
    int HandleButton(GdkEventButton* event, bool mouse_up);
    int HandleMotion(GdkEventMotion* event, bool mouse_leave);
    int HandleScroll(GdkEventScroll* event);
    int HandleKey(GdkEventKey* event, bool key_up);
    uint32_t CurrentModifiers() const;
#endif

    int width_ = 1280;
    int height_ = 800;
    int stride_bytes_ = 0;
    std::vector<std::uint32_t> frame_argb_{};
    CefWindowHandle parent_handle_ = 0;
#if defined(CEF_X11)
    GtkWidget* window_ = nullptr;
    GtkWidget* drawing_area_ = nullptr;
#endif
    CefRefPtr<CefBrowser> browser_{};
};
