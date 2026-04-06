#include "cef_osr_host_gtk.h"

#include <algorithm>
#include <cctype>

#include "include/base/cef_logging.h"

#if defined(CEF_X11)
#include <X11/Xutil.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

namespace {

constexpr int kTopStripHeight = 48;
constexpr double kChromeBgR = 0.08;
constexpr double kChromeBgG = 0.10;
constexpr double kChromeBgB = 0.14;
constexpr double kChromeBorder = 0.22;
constexpr double kBackButtonX = 112.0;
constexpr double kForwardButtonX = 148.0;
constexpr double kReloadButtonX = 184.0;
constexpr double kButtonY = 11.0;
constexpr double kButtonHeight = 26.0;
constexpr double kSmallButtonWidth = 30.0;
constexpr double kReloadButtonWidth = 42.0;
constexpr double kAddressX = 238.0;
constexpr double kAddressY = 10.0;
constexpr double kAddressHeight = 28.0;
constexpr double kGoButtonWidth = 40.0;
constexpr double kAddressTextInsetX = 12.0;
constexpr int kAddressGlyphAdvance = 7;
constexpr double kContextMenuWidth = 150.0;
constexpr double kContextMenuItemHeight = 26.0;

struct Rect {
    double x;
    double y;
    double w;
    double h;
};

bool Contains(const Rect& rect, int x, int y) {
    return x >= rect.x && y >= rect.y && x < (rect.x + rect.w) && y < (rect.y + rect.h);
}

Rect BackButtonRect() { return {kBackButtonX, kButtonY, kSmallButtonWidth, kButtonHeight}; }
Rect ForwardButtonRect() { return {kForwardButtonX, kButtonY, kSmallButtonWidth, kButtonHeight}; }
Rect ReloadButtonRect() { return {kReloadButtonX, kButtonY, kReloadButtonWidth, kButtonHeight}; }
Rect AddressRect(int draw_width) {
    const double address_w = std::max(160.0, static_cast<double>(draw_width) - 414.0);
    return {kAddressX, kAddressY, address_w, kAddressHeight};
}
Rect GoButtonRect(int draw_width) {
    const Rect address = AddressRect(draw_width);
    return {address.x + address.w + 10.0, 11.0, kGoButtonWidth, 26.0};
}
Rect AddressContextMenuRect(int x, int y) {
    return {static_cast<double>(x), static_cast<double>(y), kContextMenuWidth, kContextMenuItemHeight * 4.0};
}
Rect AddressContextMenuItemRect(int x, int y, int index) {
    return {static_cast<double>(x), static_cast<double>(y) + index * kContextMenuItemHeight, kContextMenuWidth, kContextMenuItemHeight};
}

double MeasureAddressTextWidth(std::string_view text) {
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
    cairo_t* cr = cairo_create(surface);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    cairo_text_extents_t extents{};
    const std::string owned(text);
    cairo_text_extents(cr, owned.c_str(), &extents);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return extents.x_advance;
}

std::string EllipsizeMiddle(const std::string& text, std::size_t max_chars) {
    if (text.size() <= max_chars || max_chars < 8) {
        return text;
    }
    const std::size_t left = (max_chars - 3) / 2;
    const std::size_t right = max_chars - 3 - left;
    return text.substr(0, left) + "..." + text.substr(text.size() - right);
}

std::string TrimAscii(std::string text) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(), text.end());
    return text;
}

std::string NormalizeTypedUrl(const std::string& raw) {
    std::string text = TrimAscii(raw);
    if (text.empty()) {
        return text;
    }
    if (text.rfind("http://", 0) == 0 || text.rfind("https://", 0) == 0 || text.rfind("file://", 0) == 0 ||
        text.rfind("about:", 0) == 0 || text.rfind("data:", 0) == 0) {
        return text;
    }
    if (text.find("://") != std::string::npos) {
        return text;
    }
    return std::string("https://") + text;
}

std::string EffectiveWindowTitle(const std::string& page_title, const std::string& profile_label) {
    const std::string bridge_suffix = profile_label.empty() ? "BRIDGE" : "BRIDGE [" + profile_label + "]";
    return page_title.empty() ? bridge_suffix + " — CEF Runtime Host" : page_title + " — " + bridge_suffix;
}

}  // namespace

CefOsrHostGtk::CefOsrHostGtk() = default;

CefOsrHostGtk::~CefOsrHostGtk() {
#if defined(CEF_X11)
    if (brand_overlay_pixbuf_ != nullptr) {
        g_object_unref(brand_overlay_pixbuf_);
        brand_overlay_pixbuf_ = nullptr;
    }
    if (brand_icon_pixbuf_ != nullptr) {
        g_object_unref(brand_icon_pixbuf_);
        brand_icon_pixbuf_ = nullptr;
    }
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

    g_set_prgname("bridge");
    g_set_application_name("BRIDGE");
    gdk_set_program_class("BRIDGE");
#if defined(ENGINE_CEF_BRIDGE_ICON_PNG_PATH)
    {
        GError* icon_error = nullptr;
        brand_icon_pixbuf_ = gdk_pixbuf_new_from_file(ENGINE_CEF_BRIDGE_ICON_PNG_PATH, &icon_error);
        if (icon_error != nullptr) {
            LOG(WARNING) << "CefOsrHostGtk: failed to load app icon: " << icon_error->message;
            g_error_free(icon_error);
            icon_error = nullptr;
        }
        if (brand_icon_pixbuf_ != nullptr) {
            brand_overlay_pixbuf_ = gdk_pixbuf_scale_simple(brand_icon_pixbuf_, 18, 18, GDK_INTERP_BILINEAR);
            gtk_window_set_default_icon(brand_icon_pixbuf_);
            gtk_window_set_default_icon_name("bridge");
        }
    }
#endif

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

    gtk_widget_set_can_focus(drawing_area_, TRUE);
    gtk_widget_set_hexpand(drawing_area_, TRUE);
    gtk_widget_set_vexpand(drawing_area_, TRUE);
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
    g_signal_connect(drawing_area_, "size-allocate", G_CALLBACK(CefOsrHostGtk::SizeAllocateCallback), this);
    g_signal_connect(window_, "delete-event", G_CALLBACK(CefOsrHostGtk::DeleteEventCallback), this);
    g_signal_connect(window_, "configure-event", G_CALLBACK(CefOsrHostGtk::ConfigureCallback), this);
    g_signal_connect(window_, "map-event", G_CALLBACK(CefOsrHostGtk::MapEventCallback), this);

    gtk_window_set_default_size(GTK_WINDOW(window_), width_, height_);
    gtk_window_set_title(GTK_WINDOW(window_), "BRIDGE — CEF Runtime Host");
    gtk_window_set_icon_name(GTK_WINDOW(window_), "bridge");
    if (brand_icon_pixbuf_ != nullptr) {
        gtk_window_set_icon(GTK_WINDOW(window_), brand_icon_pixbuf_);
    }
    gtk_window_set_decorated(GTK_WINDOW(window_), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window_), TRUE);
    gtk_widget_show_all(window_);
    gtk_widget_realize(window_);
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    SyncViewSizeFromAllocation(false, width_, height_);

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

    XClassHint class_hint;
    class_hint.res_name = const_cast<char*>("bridge");
    class_hint.res_class = const_cast<char*>("BRIDGE");
    XSetClassHint(GDK_WINDOW_XDISPLAY(gdk_window), GDK_WINDOW_XID(gdk_window), &class_hint);

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

    frame_width_ = width;
    frame_height_ = height;
    frame_stride_bytes_ = width * static_cast<int>(sizeof(std::uint32_t));
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
    const int fallback_width = event != nullptr ? event->width : 0;
    const int fallback_height = event != nullptr ? event->height : 0;
    self->SyncViewSizeFromAllocation(true, fallback_width, fallback_height);
    return 0;
}

void CefOsrHostGtk::SizeAllocateCallback(GtkWidget* widget, void* allocation, void* user_data) {
    (void)widget;
    (void)allocation;
    auto* self = static_cast<CefOsrHostGtk*>(user_data);
    self->SyncViewSizeFromAllocation(self->browser_ != nullptr, self->width_, self->height_);
}

int CefOsrHostGtk::MapEventCallback(GtkWidget* widget, void* event, void* user_data) {
    (void)widget;
    (void)event;
    auto* self = static_cast<CefOsrHostGtk*>(user_data);
    self->QueueDeferredResizeSync();
    return 0;
}

int CefOsrHostGtk::DeferredResizeIdle(void* user_data) {
    auto* self = static_cast<CefOsrHostGtk*>(user_data);
    self->deferred_resize_pending_ = false;
    self->SyncViewSizeFromAllocation(true, self->width_, self->height_);
    return 0;
}

int CefOsrHostGtk::Draw(cairo_t* cr) {
    const int draw_width = std::max(1, gtk_widget_get_allocated_width(drawing_area_));
    const int draw_height = std::max(1, gtk_widget_get_allocated_height(drawing_area_));
    const int content_height = std::max(1, draw_height - kTopStripHeight);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    if (!frame_argb_.empty() && frame_width_ > 0 && frame_height_ > 0 && frame_stride_bytes_ > 0) {
        cairo_surface_t* surface = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(frame_argb_.data()),
            CAIRO_FORMAT_ARGB32,
            frame_width_,
            frame_height_,
            frame_stride_bytes_);

        cairo_save(cr);
        cairo_translate(cr, 0.0, static_cast<double>(kTopStripHeight));
        cairo_scale(cr,
                    static_cast<double>(draw_width) / static_cast<double>(frame_width_),
                    static_cast<double>(content_height) / static_cast<double>(frame_height_));
        cairo_set_source_surface(cr, surface, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
        cairo_surface_destroy(surface);
    }

    cairo_save(cr);
    cairo_set_source_rgb(cr, kChromeBgR, kChromeBgG, kChromeBgB);
    cairo_rectangle(cr, 0.0, 0.0, draw_width, kTopStripHeight);
    cairo_fill(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.12);
    cairo_rectangle(cr, 0.0, static_cast<double>(kTopStripHeight - 1), draw_width, 1.0);
    cairo_fill(cr);

    if (brand_overlay_pixbuf_ != nullptr) {
        gdk_cairo_set_source_pixbuf(cr, brand_overlay_pixbuf_, 12.0, 15.0);
        cairo_paint(cr);
    }
    cairo_set_source_rgb(cr, 0.95, 0.98, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);
    cairo_move_to(cr, 38.0, 31.0);
    cairo_show_text(cr, "BRIDGE");

    auto draw_button = [&](const Rect& rect, const char* label, bool enabled) {
        cairo_set_source_rgba(cr, enabled ? 0.16 : 0.11, enabled ? 0.20 : 0.14, enabled ? 0.27 : 0.18, 0.98);
        cairo_rectangle(cr, rect.x, rect.y, rect.w, rect.h);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, kChromeBorder);
        cairo_rectangle(cr, rect.x, rect.y, rect.w, rect.h);
        cairo_stroke(cr);
        cairo_set_source_rgba(cr, 0.95, 0.98, 1.0, enabled ? 1.0 : 0.45);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 13.0);
        cairo_move_to(cr, rect.x + 12.0, rect.y + 18.0);
        cairo_show_text(cr, label);
    };

    draw_button(BackButtonRect(), "<", can_go_back_);
    draw_button(ForwardButtonRect(), ">", can_go_forward_);
    draw_button(ReloadButtonRect(), "R", true);

    const Rect address_rect = AddressRect(draw_width);
    const Rect go_rect = GoButtonRect(draw_width);
    const double address_x = address_rect.x;
    const double address_y = address_rect.y;
    const double address_w = address_rect.w;
    const double address_h = address_rect.h;
    cairo_set_source_rgba(cr, 0.11, 0.14, 0.19, 0.98);
    cairo_rectangle(cr, address_x, address_y, address_w, address_h);
    cairo_fill(cr);
    cairo_set_source_rgba(cr,
                          address_focused_ ? 0.46 : 1.0,
                          address_focused_ ? 0.72 : 1.0,
                          address_focused_ ? 1.0 : 1.0,
                          address_focused_ ? 0.82 : 0.16);
    cairo_rectangle(cr, address_x, address_y, address_w, address_h);
    cairo_stroke(cr);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    const std::size_t address_visible_start = AddressVisibleStart(draw_width);
    const std::size_t address_visible_chars = AddressMaxVisibleChars(draw_width);
    const std::string address_text = address_focused_
                                         ? address_edit_buffer_.substr(address_visible_start, address_visible_chars)
                                         : (current_url_.empty() ? std::string("Address field") : EllipsizeMiddle(current_url_, 90));
    cairo_save(cr);
    cairo_rectangle(cr, address_x + 8.0, address_y + 4.0, address_w - 16.0, address_h - 8.0);
    cairo_clip(cr);
    cairo_text_extents_t extents{};
    cairo_text_extents(cr, address_text.c_str(), &extents);
    if (address_focused_ && address_replace_on_type_ && !address_edit_buffer_.empty()) {
        const double highlight_x = address_x + 10.0;
        const double highlight_y = address_y + 6.0;
        const double highlight_w = std::min(address_w - 20.0, extents.x_advance + 8.0);
        const double highlight_h = address_h - 12.0;
        cairo_set_source_rgba(cr, 0.34, 0.59, 1.0, 0.88);
        cairo_rectangle(cr, highlight_x, highlight_y, highlight_w, highlight_h);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0.07, 0.10, 0.14);
    } else {
        cairo_set_source_rgba(cr, 0.93, 0.97, 1.0, 0.92);
    }
    cairo_move_to(cr, address_x + kAddressTextInsetX, address_y + 18.0);
    cairo_show_text(cr, address_text.c_str());
    if (address_focused_ && !address_replace_on_type_) {
        const std::size_t caret_offset = address_cursor_ >= address_visible_start ? address_cursor_ - address_visible_start : 0;
        const double caret_advance = MeasureAddressTextWidth(std::string_view(address_text).substr(0, caret_offset));
        const double caret_x = std::min(address_x + address_w - 10.0,
                                        address_x + kAddressTextInsetX + caret_advance);
        cairo_set_source_rgba(cr, 0.92, 0.97, 1.0, 0.92);
        cairo_move_to(cr, caret_x, address_y + 7.0);
        cairo_line_to(cr, caret_x, address_y + address_h - 7.0);
        cairo_stroke(cr);
    }
    cairo_restore(cr);

    draw_button(go_rect, "Go", true);

    if (address_context_menu_open_) {
        const Rect menu_rect = AddressContextMenuRect(address_context_menu_x_, address_context_menu_y_);
        cairo_set_source_rgba(cr, 0.11, 0.14, 0.19, 0.98);
        cairo_rectangle(cr, menu_rect.x, menu_rect.y, menu_rect.w, menu_rect.h);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, kChromeBorder);
        cairo_rectangle(cr, menu_rect.x, menu_rect.y, menu_rect.w, menu_rect.h);
        cairo_stroke(cr);

        static constexpr const char* kItems[] = {"Copy", "Paste", "Cut", "Select All"};
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        for (int i = 0; i < 4; ++i) {
            const Rect item = AddressContextMenuItemRect(address_context_menu_x_, address_context_menu_y_, i);
            if (i != 0) {
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.08);
                cairo_rectangle(cr, item.x, item.y, item.w, 1.0);
                cairo_fill(cr);
            }
            cairo_set_source_rgba(cr, 0.93, 0.97, 1.0, 0.94);
            cairo_move_to(cr, item.x + 12.0, item.y + 17.0);
            cairo_show_text(cr, kItems[i]);
        }
    }

    const char* status_label = "Ready";
    double status_r = 0.36;
    double status_g = 0.78;
    double status_b = 0.48;
    if (!load_error_text_.empty()) {
        status_label = "Load failed";
        status_r = 0.87;
        status_g = 0.31;
        status_b = 0.31;
    } else if (is_loading_) {
        status_label = "Loading";
        status_r = 0.90;
        status_g = 0.74;
        status_b = 0.25;
    }

    double status_x = static_cast<double>(draw_width) - 114.0;
    if (!profile_label_.empty()) {
        const std::string profile_text = EllipsizeMiddle(profile_label_, 18);
        cairo_set_source_rgba(cr, 0.16, 0.20, 0.27, 0.98);
        cairo_rectangle(cr, draw_width - 236.0, 11.0, 108.0, 26.0);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, kChromeBorder);
        cairo_rectangle(cr, draw_width - 236.0, 11.0, 108.0, 26.0);
        cairo_stroke(cr);
        cairo_set_source_rgba(cr, 0.88, 0.92, 0.98, 0.92);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 11.0);
        cairo_move_to(cr, draw_width - 226.0, 28.0);
        cairo_show_text(cr, profile_text.c_str());
        status_x = static_cast<double>(draw_width) - 114.0;
    }

    cairo_set_source_rgb(cr, status_r, status_g, status_b);
    cairo_arc(cr, status_x, 24.0, 4.0, 0.0, 6.28318530718);
    cairo_fill(cr);
    cairo_set_source_rgba(cr, 0.88, 0.92, 0.98, 0.86);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11.0);
    cairo_move_to(cr, status_x + 10.0, 28.0);
    cairo_show_text(cr, status_label);

    cairo_set_source_rgba(cr, 0.80, 0.85, 0.92, 0.78);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11.0);
    cairo_move_to(cr, 14.0, 44.0);
    cairo_show_text(cr, "Official runtime-host browser");
    cairo_restore(cr);
    return 0;
}

bool CefOsrHostGtk::SyncViewSizeFromAllocation(bool notify_browser, int fallback_width, int fallback_height) {
    int view_width = fallback_width;
    int view_height = fallback_height;
    if (drawing_area_ != nullptr) {
        const int allocated_width = gtk_widget_get_allocated_width(drawing_area_);
        const int allocated_height = gtk_widget_get_allocated_height(drawing_area_);
        if (allocated_width > 1) {
            view_width = allocated_width;
        }
        if (allocated_height > 1) {
            view_height = allocated_height;
        }
    }

    view_width = std::max(1, view_width);
    view_height = std::max(1, view_height);
    const int browser_height = std::max(1, view_height - kTopStripHeight);
    const bool changed = (browser_view_width_ != view_width) || (browser_view_height_ != browser_height);
    width_ = view_width;
    height_ = view_height;
    browser_view_width_ = view_width;
    browser_view_height_ = browser_height;

    if (changed && notify_browser && browser_ != nullptr) {
        browser_->GetHost()->NotifyScreenInfoChanged();
        browser_->GetHost()->WasResized();
    }
    return changed;
}

void CefOsrHostGtk::QueueDeferredResizeSync() {
    if (deferred_resize_pending_) {
        return;
    }
    deferred_resize_pending_ = true;
    g_idle_add(&CefOsrHostGtk::DeferredResizeIdle, this);
}

CefRefPtr<CefFrame> CefOsrHostGtk::ActiveFrame() const {
    if (browser_ == nullptr) {
        return nullptr;
    }
    if (auto focused = browser_->GetFocusedFrame(); focused && focused->IsValid()) {
        return focused;
    }
    if (auto main = browser_->GetMainFrame(); main && main->IsValid()) {
        return main;
    }
    return nullptr;
}

std::size_t CefOsrHostGtk::AddressMaxVisibleChars(int draw_width) const {
    const Rect rect = AddressRect(draw_width);
    const int usable_width = std::max(1, static_cast<int>(rect.w) - 20);
    return static_cast<std::size_t>(std::max(1, usable_width / kAddressGlyphAdvance));
}

std::size_t CefOsrHostGtk::AddressVisibleStart(int draw_width) const {
    const std::size_t max_chars = AddressMaxVisibleChars(draw_width);
    return address_cursor_ <= max_chars ? 0 : address_cursor_ - max_chars;
}

std::size_t CefOsrHostGtk::AddressCursorFromPoint(int draw_width, int x) const {
    const Rect rect = AddressRect(draw_width);
    const int relative_x = std::max(0, x - static_cast<int>(rect.x + kAddressTextInsetX));
    const std::size_t visible_start = AddressVisibleStart(draw_width);
    const std::size_t visible_chars = AddressMaxVisibleChars(draw_width);
    const std::string visible_text = address_edit_buffer_.substr(visible_start, visible_chars);

    for (std::size_t i = 0; i <= visible_text.size(); ++i) {
        const double advance = MeasureAddressTextWidth(std::string_view(visible_text).substr(0, i));
        if (advance >= static_cast<double>(relative_x)) {
            return std::min(address_edit_buffer_.size(), visible_start + i);
        }
    }
    return std::min(address_edit_buffer_.size(), visible_start + visible_text.size());
}

void CefOsrHostGtk::FocusAddressField(bool select_all, std::size_t cursor) {
    const bool was_focused = address_focused_;
    address_focused_ = true;
    if (!was_focused) {
        address_edit_buffer_ = current_url_;
    }
    address_replace_on_type_ = select_all;
    if (cursor == std::string::npos) {
        address_cursor_ = address_edit_buffer_.size();
    } else {
        address_cursor_ = std::min(cursor, address_edit_buffer_.size());
    }
#if defined(CEF_X11)
    if (drawing_area_ != nullptr) {
        gtk_widget_grab_focus(drawing_area_);
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

void CefOsrHostGtk::BlurAddressField() {
    address_focused_ = false;
    address_replace_on_type_ = false;
    address_context_menu_open_ = false;
    address_edit_buffer_ = current_url_;
    address_cursor_ = address_edit_buffer_.size();
#if defined(CEF_X11)
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

bool CefOsrHostGtk::NavigateAddressBuffer() {
    if (browser_ == nullptr) {
        return false;
    }
    const std::string normalized = NormalizeTypedUrl(address_focused_ ? address_edit_buffer_ : current_url_);
    if (normalized.empty()) {
        return false;
    }
    current_url_ = normalized;
    address_edit_buffer_ = normalized;
    address_cursor_ = address_edit_buffer_.size();
    address_focused_ = false;
    address_replace_on_type_ = false;
    load_error_text_.clear();
    failed_url_.clear();
    if (auto frame = browser_->GetMainFrame()) {
        frame->LoadURL(normalized);
    }
#if defined(CEF_X11)
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
    browser_->GetHost()->SetFocus(true);
    return true;
}

void CefOsrHostGtk::AddressSelectAll() {
    address_replace_on_type_ = true;
    address_cursor_ = address_edit_buffer_.size();
#if defined(CEF_X11)
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

bool CefOsrHostGtk::AddressHasSelection() const {
    return address_focused_ && address_replace_on_type_ && !address_edit_buffer_.empty();
}

void CefOsrHostGtk::AddressCopySelection() {
#if defined(CEF_X11)
    if (!AddressHasSelection()) {
        return;
    }
    GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    if (clipboard != nullptr) {
        gtk_clipboard_set_text(clipboard, address_edit_buffer_.c_str(), -1);
    }
#endif
}

void CefOsrHostGtk::AddressCutSelection() {
#if defined(CEF_X11)
    if (!AddressHasSelection()) {
        return;
    }
    AddressCopySelection();
    address_edit_buffer_.clear();
    address_cursor_ = 0;
    address_replace_on_type_ = false;
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

void CefOsrHostGtk::AddressPasteClipboard() {
#if defined(CEF_X11)
    GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    if (clipboard == nullptr) {
        return;
    }
    gchar* clipboard_text = gtk_clipboard_wait_for_text(clipboard);
    if (clipboard_text == nullptr) {
        return;
    }
    if (address_replace_on_type_) {
        address_edit_buffer_ = clipboard_text;
        address_cursor_ = address_edit_buffer_.size();
        address_replace_on_type_ = false;
    } else {
        address_edit_buffer_.insert(address_cursor_, clipboard_text);
        address_cursor_ += std::strlen(clipboard_text);
    }
    g_free(clipboard_text);
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

bool CefOsrHostGtk::HandleAddressContextMenuClick(int x, int y) {
    if (!address_context_menu_open_) {
        return false;
    }
    const Rect menu_rect = AddressContextMenuRect(address_context_menu_x_, address_context_menu_y_);
    if (!Contains(menu_rect, x, y)) {
        address_context_menu_open_ = false;
#if defined(CEF_X11)
        if (drawing_area_ != nullptr) {
            gtk_widget_queue_draw(drawing_area_);
        }
#endif
        return false;
    }

    for (int i = 0; i < 4; ++i) {
        if (!Contains(AddressContextMenuItemRect(address_context_menu_x_, address_context_menu_y_, i), x, y)) {
            continue;
        }
        switch (i) {
            case 0:
                AddressCopySelection();
                break;
            case 1:
                AddressPasteClipboard();
                break;
            case 2:
                AddressCutSelection();
                break;
            case 3:
                AddressSelectAll();
                break;
            default:
                break;
        }
        address_context_menu_open_ = false;
#if defined(CEF_X11)
        if (drawing_area_ != nullptr) {
            gtk_widget_queue_draw(drawing_area_);
        }
#endif
        return true;
    }
    return false;
}

bool CefOsrHostGtk::HandleChromeClick(int x, int y) {
    const int draw_width = drawing_area_ != nullptr ? std::max(1, gtk_widget_get_allocated_width(drawing_area_)) : width_;
    if (Contains(BackButtonRect(), x, y)) {
        if (can_go_back_ && browser_ != nullptr) {
            BlurAddressField();
            browser_->GoBack();
        }
        return true;
    }
    if (Contains(ForwardButtonRect(), x, y)) {
        if (can_go_forward_ && browser_ != nullptr) {
            BlurAddressField();
            browser_->GoForward();
        }
        return true;
    }
    if (Contains(ReloadButtonRect(), x, y)) {
        BlurAddressField();
        if (browser_ != nullptr) {
            browser_->Reload();
        }
        return true;
    }
    if (Contains(AddressRect(draw_width), x, y)) {
        const bool was_focused = address_focused_;
        if (!was_focused) {
            FocusAddressField(true);
        } else {
            const std::size_t cursor = AddressCursorFromPoint(draw_width, x);
            FocusAddressField(false, cursor);
        }
        return true;
    }
    if (Contains(GoButtonRect(draw_width), x, y)) {
        if (!address_focused_) {
            FocusAddressField();
        }
        NavigateAddressBuffer();
        return true;
    }
    BlurAddressField();
    return true;
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
    const int x = static_cast<int>(event->x);
    const int y = static_cast<int>(event->y);
    const int draw_width = drawing_area_ != nullptr ? std::max(1, gtk_widget_get_allocated_width(drawing_area_)) : width_;

    if (address_context_menu_open_) {
        if (!mouse_up) {
            return 1;
        }
        if (HandleAddressContextMenuClick(x, y)) {
            return 1;
        }
        address_context_menu_open_ = false;
        if (drawing_area_ != nullptr) {
            gtk_widget_queue_draw(drawing_area_);
        }
        return 1;
    }

    if (y < kTopStripHeight) {
        if (event->button == 3) {
            if (mouse_up && Contains(AddressRect(draw_width), x, y)) {
                if (!address_focused_) {
                    FocusAddressField(true);
                }
                address_context_menu_open_ = true;
                address_context_menu_x_ = std::max(8, std::min(x, draw_width - static_cast<int>(kContextMenuWidth) - 8));
                address_context_menu_y_ = std::max(8, std::min(y, height_ - static_cast<int>(kContextMenuItemHeight * 4.0) - 8));
                if (drawing_area_ != nullptr) {
                    gtk_widget_queue_draw(drawing_area_);
                }
            }
            return 1;
        }
        if (mouse_up && event->button == 1) {
            return HandleChromeClick(x, y) ? 1 : 1;
        }
        return 1;
    }

    if (event->button == 3) {
        return 1;
    }

    if (address_focused_) {
        BlurAddressField();
    }
    gtk_widget_grab_focus(drawing_area_);
    browser_->GetHost()->SetFocus(true);

    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y - kTopStripHeight;
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
    const int x = static_cast<int>(event->x);
    const int y = static_cast<int>(event->y);
    if (!mouse_leave && y < kTopStripHeight) {
        return 1;
    }
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = std::max(0, y - kTopStripHeight);
    mouse_event.modifiers = CurrentModifiers();
    browser_->GetHost()->SendMouseMoveEvent(mouse_event, mouse_leave);
    return 1;
}

int CefOsrHostGtk::HandleScroll(GdkEventScroll* event) {
    if (browser_ == nullptr || event == nullptr) {
        return 0;
    }
    const int x = static_cast<int>(event->x);
    const int y = static_cast<int>(event->y);
    if (y < kTopStripHeight) {
        return 1;
    }
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y - kTopStripHeight;
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

    const bool control_down = (event->state & GDK_CONTROL_MASK) != 0;
    const bool alt_down = (event->state & GDK_MOD1_MASK) != 0;
    const gunichar unicode = gdk_keyval_to_unicode(event->keyval);

    if (!key_up && control_down && !alt_down && (event->keyval == GDK_KEY_l || event->keyval == GDK_KEY_L)) {
        FocusAddressField();
        return 1;
    }
    if (!key_up && event->keyval == GDK_KEY_Escape && address_context_menu_open_) {
        address_context_menu_open_ = false;
        if (drawing_area_ != nullptr) {
            gtk_widget_queue_draw(drawing_area_);
        }
        return 1;
    }

    if (address_focused_) {
        if (key_up) {
            return 1;
        }
        if (control_down && !alt_down) {
            switch (event->keyval) {
                case GDK_KEY_a:
                case GDK_KEY_A:
                    AddressSelectAll();
                    return 1;
                case GDK_KEY_c:
                case GDK_KEY_C:
                    AddressCopySelection();
                    return 1;
                case GDK_KEY_x:
                case GDK_KEY_X:
                    AddressCutSelection();
                    return 1;
                case GDK_KEY_v:
                case GDK_KEY_V:
                    AddressPasteClipboard();
                    return 1;
                default:
                    break;
            }
        }
        switch (event->keyval) {
            case GDK_KEY_Escape:
                BlurAddressField();
                return 1;
            case GDK_KEY_Return:
            case GDK_KEY_KP_Enter:
                NavigateAddressBuffer();
                return 1;
            case GDK_KEY_Left:
                if (address_replace_on_type_) {
                    address_replace_on_type_ = false;
                    address_cursor_ = 0;
                } else if (address_cursor_ > 0) {
                    --address_cursor_;
                }
                if (drawing_area_ != nullptr) {
                    gtk_widget_queue_draw(drawing_area_);
                }
                return 1;
            case GDK_KEY_Right:
                if (address_replace_on_type_) {
                    address_replace_on_type_ = false;
                    address_cursor_ = address_edit_buffer_.size();
                } else if (address_cursor_ < address_edit_buffer_.size()) {
                    ++address_cursor_;
                }
                if (drawing_area_ != nullptr) {
                    gtk_widget_queue_draw(drawing_area_);
                }
                return 1;
            case GDK_KEY_Home:
                address_replace_on_type_ = false;
                address_cursor_ = 0;
                if (drawing_area_ != nullptr) {
                    gtk_widget_queue_draw(drawing_area_);
                }
                return 1;
            case GDK_KEY_End:
                address_replace_on_type_ = false;
                address_cursor_ = address_edit_buffer_.size();
                if (drawing_area_ != nullptr) {
                    gtk_widget_queue_draw(drawing_area_);
                }
                return 1;
            case GDK_KEY_BackSpace:
                if (address_replace_on_type_) {
                    address_edit_buffer_.clear();
                    address_cursor_ = 0;
                    address_replace_on_type_ = false;
                } else if (address_cursor_ > 0 && !address_edit_buffer_.empty()) {
                    address_edit_buffer_.erase(address_cursor_ - 1, 1);
                    --address_cursor_;
                }
                if (drawing_area_ != nullptr) {
                    gtk_widget_queue_draw(drawing_area_);
                }
                return 1;
            case GDK_KEY_Delete:
                if (address_replace_on_type_) {
                    address_edit_buffer_.clear();
                    address_cursor_ = 0;
                    address_replace_on_type_ = false;
                } else if (address_cursor_ < address_edit_buffer_.size()) {
                    address_edit_buffer_.erase(address_cursor_, 1);
                }
                if (drawing_area_ != nullptr) {
                    gtk_widget_queue_draw(drawing_area_);
                }
                return 1;
            default:
                break;
        }
        if (!control_down && !alt_down && unicode >= 32) {
            char utf8_char[8] = {0};
            const int utf8_len = g_unichar_to_utf8(unicode, utf8_char);
            std::string text(utf8_char, utf8_len);
            if (address_replace_on_type_) {
                address_edit_buffer_ = text;
                address_cursor_ = address_edit_buffer_.size();
                address_replace_on_type_ = false;
            } else {
                address_edit_buffer_.insert(address_cursor_, text);
                address_cursor_ += text.size();
            }
            if (drawing_area_ != nullptr) {
                gtk_widget_queue_draw(drawing_area_);
            }
            return 1;
        }
        return 1;
    }

    if (!key_up && control_down && !alt_down) {
        if (auto frame = ActiveFrame()) {
            switch (event->keyval) {
                case GDK_KEY_c:
                case GDK_KEY_C:
                    frame->Copy();
                    return 1;
                case GDK_KEY_x:
                case GDK_KEY_X:
                    frame->Cut();
                    return 1;
                case GDK_KEY_v:
                case GDK_KEY_V:
                    frame->Paste();
                    return 1;
                case GDK_KEY_a:
                case GDK_KEY_A:
                    frame->SelectAll();
                    return 1;
                case GDK_KEY_z:
                case GDK_KEY_Z:
                    frame->Undo();
                    return 1;
                case GDK_KEY_y:
                case GDK_KEY_Y:
                    frame->Redo();
                    return 1;
                default:
                    break;
            }
        }
    }

    CefKeyEvent key_event;
    key_event.size = sizeof(CefKeyEvent);
    key_event.type = key_up ? KEYEVENT_KEYUP : KEYEVENT_RAWKEYDOWN;
    key_event.windows_key_code = static_cast<int>(event->keyval);
    key_event.native_key_code = static_cast<int>(event->hardware_keycode);
    key_event.is_system_key = 0;
    key_event.character = static_cast<char16_t>(unicode);
    key_event.unmodified_character = key_event.character;
    key_event.focus_on_editable_field = 1;
    key_event.modifiers = CurrentModifiers();
    if ((event->state & GDK_SHIFT_MASK) != 0) {
        key_event.modifiers |= EVENTFLAG_SHIFT_DOWN;
    }
    if (control_down) {
        key_event.modifiers |= EVENTFLAG_CONTROL_DOWN;
    }
    if (alt_down) {
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
    rect = CefRect(0, 0, browser_view_width_ > 0 ? browser_view_width_ : width_,
                   browser_view_height_ > 0 ? browser_view_height_ : std::max(1, height_ - kTopStripHeight));
}

bool CefOsrHostGtk::GetScreenPoint(int view_x, int view_y, int& screen_x, int& screen_y) const {
    screen_x = view_x;
    screen_y = view_y + kTopStripHeight;
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
    browser_view_width_ = 0;
    browser_view_height_ = 0;
    gtk_widget_grab_focus(drawing_area_);
    browser->GetHost()->WasHidden(false);
    browser->GetHost()->SetFocus(true);
    SyncViewSizeFromAllocation(true, width_, height_);
    QueueDeferredResizeSync();
}

void CefOsrHostGtk::SetProfileLabel(const std::string& profile_label) {
    profile_label_ = profile_label;
#if defined(CEF_X11)
    if (window_ != nullptr) {
        const std::string effective = EffectiveWindowTitle(window_title_, profile_label_);
        gtk_window_set_title(GTK_WINDOW(window_), effective.c_str());
    }
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

void CefOsrHostGtk::SetCurrentUrl(const std::string& url) {
    current_url_ = url;
    if (!address_focused_) {
        address_edit_buffer_ = current_url_;
        address_cursor_ = address_edit_buffer_.size();
    }
#if defined(CEF_X11)
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

void CefOsrHostGtk::SetLoadingState(bool is_loading, bool can_go_back, bool can_go_forward) {
    is_loading_ = is_loading;
    can_go_back_ = can_go_back;
    can_go_forward_ = can_go_forward;
    if (is_loading_) {
        load_error_text_.clear();
        failed_url_.clear();
    }
#if defined(CEF_X11)
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

void CefOsrHostGtk::SetLoadError(const std::string& error_text, const std::string& failed_url) {
    load_error_text_ = error_text;
    failed_url_ = failed_url;
    is_loading_ = false;
#if defined(CEF_X11)
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#endif
}

void CefOsrHostGtk::SetWindowTitle(const std::string& title) {
    window_title_ = title;
#if defined(CEF_X11)
    if (window_ == nullptr) {
        return;
    }
    const std::string effective = EffectiveWindowTitle(title, profile_label_);
    gtk_window_set_title(GTK_WINDOW(window_), effective.c_str());
    if (drawing_area_ != nullptr) {
        gtk_widget_queue_draw(drawing_area_);
    }
#else
    (void)title;
#endif
}
