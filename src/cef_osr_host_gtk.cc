#include "cef_osr_host_gtk.h"

#include <algorithm>

#include "include/base/cef_logging.h"

#if defined(CEF_X11)
#include <gdk/gdkkeysyms.h>
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

    drawing_area_ = gtk_drawing_area_new();
    if (drawing_area_ == nullptr) {
        LOG(ERROR) << "CefOsrHostGtk: gtk_drawing_area_new returned null";
        return false;
    }

    gtk_widget_set_size_request(drawing_area_, width_, height_);
    gtk_widget_set_can_focus(drawing_area_, TRUE);
    gtk_widget_add_events(drawing_area_,
                          GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK |
                              GDK_POINTER_MOTION_MASK |
                              GDK_SCROLL_MASK |
                              GDK_KEY_PRESS_MASK |
                              GDK_KEY_RELEASE_MASK |
                              GDK_LEAVE_NOTIFY_MASK);
    gtk_container_add(GTK_CONTAINER(window_), drawing_area_);
    g_signal_connect(drawing_area_, "draw", G_CALLBACK(CefOsrHostGtk::DrawCallback), this);
    g_signal_connect(drawing_area_, "button-press-event", G_CALLBACK(CefOsrHostGtk::ButtonPressCallback), this);
    g_signal_connect(drawing_area_, "button-release-event", G_CALLBACK(CefOsrHostGtk::ButtonReleaseCallback), this);
    g_signal_connect(drawing_area_, "motion-notify-event", G_CALLBACK(CefOsrHostGtk::MotionCallback), this);
    g_signal_connect(drawing_area_, "scroll-event", G_CALLBACK(CefOsrHostGtk::ScrollCallback), this);
    g_signal_connect(drawing_area_, "key-press-event", G_CALLBACK(CefOsrHostGtk::KeyPressCallback), this);
    g_signal_connect(drawing_area_, "key-release-event", G_CALLBACK(CefOsrHostGtk::KeyReleaseCallback), this);
    g_signal_connect(drawing_area_, "leave-notify-event", G_CALLBACK(CefOsrHostGtk::LeaveNotifyCallback), this);
    g_signal_connect(window_, "delete-event", G_CALLBACK(CefOsrHostGtk::DeleteEventCallback), this);
    g_signal_connect(window_, "configure-event", G_CALLBACK(CefOsrHostGtk::ConfigureCallback), this);

    gtk_window_set_default_size(GTK_WINDOW(window_), width_, height_);
    gtk_window_set_title(GTK_WINDOW(window_), "engine-cef-osr-host");
    gtk_window_set_decorated(GTK_WINDOW(window_), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window_), TRUE);
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

bool CefOsrHostGtk::PresentFrame(const std::uint32_t* argb, int width, int height, int stride_bytes) {
#if defined(CEF_X11)
    if (drawing_area_ == nullptr || argb == nullptr || width <= 0 || height <= 0 || stride_bytes <= 0) {
        return false;
    }

    width_ = width;
    height_ = height;
    stride_bytes_ = width * static_cast<int>(sizeof(std::uint32_t));
    frame_argb_.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
    const int stride_pixels = std::max(1, stride_bytes / static_cast<int>(sizeof(std::uint32_t)));
    for (int y = 0; y < height; ++y) {
        const auto* src_row = argb + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride_pixels);
        auto* dst_row = frame_argb_.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width);
        std::copy_n(src_row, width, dst_row);
    }

    gtk_widget_queue_draw(drawing_area_);
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    return true;
#else
    (void)argb;
    (void)width;
    (void)height;
    (void)stride_bytes;
    return false;
#endif
}

#if defined(CEF_X11)
int CefOsrHostGtk::DrawCallback(GtkWidget* widget, cairo_t* cr, void* user_data) {
    (void)widget;
    return static_cast<CefOsrHostGtk*>(user_data)->Draw(cr);
}

int CefOsrHostGtk::ButtonPressCallback(GtkWidget* widget, GdkEventButton* event, void* user_data) {
    (void)widget;
    return static_cast<CefOsrHostGtk*>(user_data)->HandleButton(event, false);
}

int CefOsrHostGtk::ButtonReleaseCallback(GtkWidget* widget, GdkEventButton* event, void* user_data) {
    (void)widget;
    return static_cast<CefOsrHostGtk*>(user_data)->HandleButton(event, true);
}

int CefOsrHostGtk::MotionCallback(GtkWidget* widget, GdkEventMotion* event, void* user_data) {
    (void)widget;
    return static_cast<CefOsrHostGtk*>(user_data)->HandleMotion(event, false);
}

int CefOsrHostGtk::ScrollCallback(GtkWidget* widget, GdkEventScroll* event, void* user_data) {
    (void)widget;
    return static_cast<CefOsrHostGtk*>(user_data)->HandleScroll(event);
}

int CefOsrHostGtk::KeyPressCallback(GtkWidget* widget, GdkEventKey* event, void* user_data) {
    (void)widget;
    return static_cast<CefOsrHostGtk*>(user_data)->HandleKey(event, false);
}

int CefOsrHostGtk::KeyReleaseCallback(GtkWidget* widget, GdkEventKey* event, void* user_data) {
    (void)widget;
    return static_cast<CefOsrHostGtk*>(user_data)->HandleKey(event, true);
}

int CefOsrHostGtk::LeaveNotifyCallback(GtkWidget* widget, GdkEventCrossing* event, void* user_data) {
    (void)widget;
    GdkEventMotion synthetic{};
    synthetic.x = event->x;
    synthetic.y = event->y;
    synthetic.state = event->state;
    return static_cast<CefOsrHostGtk*>(user_data)->HandleMotion(&synthetic, true);
}

int CefOsrHostGtk::DeleteEventCallback(GtkWidget* widget, void* event, void* user_data) {
    (void)widget;
    (void)event;
    auto* self = static_cast<CefOsrHostGtk*>(user_data);
    if (self->browser_ != nullptr) {
        self->browser_->GetHost()->CloseBrowser(false);
        return 1;
    }
    return 0;
}

int CefOsrHostGtk::ConfigureCallback(GtkWidget* widget, GdkEventConfigure* event, void* user_data) {
    (void)widget;
    auto* self = static_cast<CefOsrHostGtk*>(user_data);
    if (event == nullptr) {
        return 0;
    }
    self->width_ = std::max(1, event->width);
    self->height_ = std::max(1, event->height);
    if (self->browser_ != nullptr) {
        self->browser_->GetHost()->NotifyScreenInfoChanged();
        self->browser_->GetHost()->WasResized();
    }
    return 0;
}

int CefOsrHostGtk::Draw(cairo_t* cr) {
    if (frame_argb_.empty() || width_ <= 0 || height_ <= 0 || stride_bytes_ <= 0) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        return 0;
    }

    cairo_surface_t* surface = cairo_image_surface_create_for_data(
        reinterpret_cast<unsigned char*>(frame_argb_.data()),
        CAIRO_FORMAT_ARGB32,
        width_,
        height_,
        stride_bytes_);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(surface);
    return 0;
}

uint32_t CefOsrHostGtk::CurrentModifiers() const {
    if (drawing_area_ == nullptr) {
        return EVENTFLAG_NONE;
    }
    GdkModifierType state = static_cast<GdkModifierType>(0);
    if (gtk_widget_get_window(drawing_area_) != nullptr) {
        gdk_window_get_device_position(gtk_widget_get_window(drawing_area_),
                                       gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_display_get_default())),
                                       nullptr,
                                       nullptr,
                                       &state);
    }

    uint32_t modifiers = EVENTFLAG_NONE;
    if ((state & GDK_SHIFT_MASK) != 0) {
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    }
    if ((state & GDK_CONTROL_MASK) != 0) {
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    }
    if ((state & GDK_MOD1_MASK) != 0) {
        modifiers |= EVENTFLAG_ALT_DOWN;
    }
    if ((state & GDK_BUTTON1_MASK) != 0) {
        modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    }
    if ((state & GDK_BUTTON2_MASK) != 0) {
        modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    }
    if ((state & GDK_BUTTON3_MASK) != 0) {
        modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    }
    return modifiers;
}

int CefOsrHostGtk::HandleButton(GdkEventButton* event, bool mouse_up) {
    if (browser_ == nullptr || event == nullptr) {
        return 0;
    }
    gtk_widget_grab_focus(drawing_area_);
    browser_->GetHost()->SetFocus(true);

    CefMouseEvent mouse_event;
    mouse_event.x = static_cast<int>(event->x);
    mouse_event.y = static_cast<int>(event->y);
    mouse_event.modifiers = CurrentModifiers();
    if (!mouse_up) {
        switch (event->button) {
            case 1:
                mouse_event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
                break;
            case 2:
                mouse_event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
                break;
            case 3:
                mouse_event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
                break;
            default:
                break;
        }
    }

    cef_mouse_button_type_t button_type = MBT_LEFT;
    if (event->button == 2) {
        button_type = MBT_MIDDLE;
    } else if (event->button == 3) {
        button_type = MBT_RIGHT;
    }
    browser_->GetHost()->SendMouseClickEvent(mouse_event, button_type, mouse_up, 1);
    return 1;
}

int CefOsrHostGtk::HandleMotion(GdkEventMotion* event, bool mouse_leave) {
    if (browser_ == nullptr || event == nullptr) {
        return 0;
    }
    CefMouseEvent mouse_event;
    mouse_event.x = static_cast<int>(event->x);
    mouse_event.y = static_cast<int>(event->y);
    mouse_event.modifiers = CurrentModifiers();
    browser_->GetHost()->SendMouseMoveEvent(mouse_event, mouse_leave);
    return 1;
}

int CefOsrHostGtk::HandleScroll(GdkEventScroll* event) {
    if (browser_ == nullptr || event == nullptr) {
        return 0;
    }
    CefMouseEvent mouse_event;
    mouse_event.x = static_cast<int>(event->x);
    mouse_event.y = static_cast<int>(event->y);
    mouse_event.modifiers = CurrentModifiers();

    int delta_x = 0;
    int delta_y = 0;
    switch (event->direction) {
        case GDK_SCROLL_UP:
            delta_y = 120;
            break;
        case GDK_SCROLL_DOWN:
            delta_y = -120;
            break;
        case GDK_SCROLL_LEFT:
            delta_x = 120;
            break;
        case GDK_SCROLL_RIGHT:
            delta_x = -120;
            break;
        default:
            break;
    }
    browser_->GetHost()->SendMouseWheelEvent(mouse_event, delta_x, delta_y);
    return 1;
}

int CefOsrHostGtk::HandleKey(GdkEventKey* event, bool key_up) {
    if (browser_ == nullptr || event == nullptr) {
        return 0;
    }

    CefKeyEvent key_event;
    key_event.size = sizeof(CefKeyEvent);
    key_event.type = key_up ? KEYEVENT_KEYUP : KEYEVENT_RAWKEYDOWN;
    key_event.windows_key_code = static_cast<int>(event->keyval);
    key_event.native_key_code = static_cast<int>(event->hardware_keycode);
    key_event.is_system_key = 0;
    key_event.character = static_cast<char16_t>(gdk_keyval_to_unicode(event->keyval));
    key_event.unmodified_character = key_event.character;
    key_event.focus_on_editable_field = 1;
    key_event.modifiers = CurrentModifiers();
    if ((event->state & GDK_SHIFT_MASK) != 0) {
        key_event.modifiers |= EVENTFLAG_SHIFT_DOWN;
    }
    if ((event->state & GDK_CONTROL_MASK) != 0) {
        key_event.modifiers |= EVENTFLAG_CONTROL_DOWN;
    }
    if ((event->state & GDK_MOD1_MASK) != 0) {
        key_event.modifiers |= EVENTFLAG_ALT_DOWN;
    }
    browser_->GetHost()->SendKeyEvent(key_event);

    if (!key_up && key_event.character != 0) {
        CefKeyEvent char_event = key_event;
        char_event.type = KEYEVENT_CHAR;
        browser_->GetHost()->SendKeyEvent(char_event);
    }
    return 1;
}
#endif

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

void CefOsrHostGtk::NotifyBrowserCreated(CefRefPtr<CefBrowser> browser) {
    if (!browser) {
        return;
    }
    browser_ = browser;
    gtk_widget_grab_focus(drawing_area_);
    browser->GetHost()->NotifyScreenInfoChanged();
    browser->GetHost()->WasHidden(false);
    browser->GetHost()->SetFocus(true);
    browser->GetHost()->WasResized();
}
