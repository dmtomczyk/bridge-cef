#include "cef_runtime_window_host_win.h"

#include <algorithm>

#include "include/base/cef_logging.h"
#include "include/cef_app.h"

namespace {

void WinHostLog(const std::string& message) {
#if defined(_WIN32)
    LOG(WARNING) << "CefRuntimeWindowHostWin: " << message;
    std::string line = std::string("CefRuntimeWindowHostWin: ") + message + "\n";
    OutputDebugStringA(line.c_str());
#else
    LOG(WARNING) << "CefRuntimeWindowHostWin: " << message;
#endif
}

#if defined(_WIN32)

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return std::wstring();
    }
    const int needed = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (needed <= 1) {
        return std::wstring(text.begin(), text.end());
    }
    std::wstring wide(static_cast<std::size_t>(needed - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), needed);
    return wide;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }
    const int needed = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 1) {
        return std::string(text.begin(), text.end());
    }
    std::string utf8(static_cast<std::size_t>(needed - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8.data(), needed, nullptr, nullptr);
    return utf8;
}

std::wstring BuildWin32Filter(const CefRuntimeWindowHost::FileDialogParams& params) {
    std::wstring filter;
    bool added_any = false;
    for (std::size_t i = 0; i < params.accept_extensions.size(); ++i) {
        const std::string& extensions = params.accept_extensions[i];
        if (extensions.empty()) {
            continue;
        }
        std::wstring description = Utf8ToWide(i < params.accept_descriptions.size() && !params.accept_descriptions[i].empty()
                                                  ? params.accept_descriptions[i]
                                                  : extensions);
        std::wstring patterns;
        std::size_t start = 0;
        while (start < extensions.size()) {
            const std::size_t end = extensions.find(';', start);
            const std::string ext = extensions.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (!ext.empty()) {
                if (!patterns.empty()) {
                    patterns += L';';
                }
                if (ext[0] == '.') {
                    patterns += L'*';
                    patterns += Utf8ToWide(ext);
                } else {
                    patterns += L"*.";
                    patterns += Utf8ToWide(ext);
                }
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
        if (patterns.empty()) {
            continue;
        }
        filter += description;
        filter.push_back(L'\0');
        filter += patterns;
        filter.push_back(L'\0');
        added_any = true;
    }
    if (!added_any) {
        filter += L"All Files";
        filter.push_back(L'\0');
        filter += L"*.*";
        filter.push_back(L'\0');
    }
    filter.push_back(L'\0');
    return filter;
}
#endif

}  // namespace

#if defined(_WIN32)
LRESULT CALLBACK CefRuntimeWindowHostWin::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    CefRuntimeWindowHostWin* self = reinterpret_cast<CefRuntimeWindowHostWin*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<CefRuntimeWindowHostWin*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    switch (message) {
        case WM_SIZE:
            self->SyncViewSizeFromClientRect();
            self->NotifyBrowserWasResized();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_SETFOCUS:
            self->NotifyBrowserFocus(true);
            return 0;
        case WM_KILLFOCUS:
            self->NotifyBrowserFocus(false);
            return 0;
        case WM_MOUSEMOVE:
        case WM_MOUSELEAVE:
            if (self->HandleMouseMove(message, wparam, lparam)) {
                return 0;
            }
            break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
            if (self->HandleMouseButton(message, wparam, lparam, false)) {
                return 0;
            }
            break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            if (self->HandleMouseButton(message, wparam, lparam, true)) {
                return 0;
            }
            break;
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            if (self->HandleMouseWheel(message, wparam, lparam)) {
                return 0;
            }
            break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (self->HandleKey(message, wparam, lparam, false)) {
                return 0;
            }
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (self->HandleKey(message, wparam, lparam, true)) {
                return 0;
            }
            break;
        case WM_CHAR:
        case WM_SYSCHAR:
            if (self->HandleChar(message, wparam, lparam)) {
                return 0;
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            self->PaintFrameToWindow(dc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CLOSE:
            WinHostLog("WM_CLOSE received");
            self->close_requested_ = true;
            if (self->browser_ && self->browser_->GetHost()) {
                WinHostLog("requesting browser-driven close");
                self->browser_->GetHost()->CloseBrowser(false);
                return 0;
            }
            WinHostLog("destroying native window without browser host");
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            WinHostLog("WM_DESTROY received");
            if (self->window_ == hwnd) {
                self->window_ = nullptr;
                self->parent_handle_ = 0;
                self->tracking_mouse_leave_ = false;
            }
            if (!self->quit_requested_) {
                self->quit_requested_ = true;
                WinHostLog("requesting CefQuitMessageLoop from WM_DESTROY");
                CefQuitMessageLoop();
            }
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

void CefRuntimeWindowHostWin::SyncViewSizeFromClientRect() {
    if (!window_) {
        return;
    }
    RECT rect{};
    if (!GetClientRect(window_, &rect)) {
        return;
    }
    view_width_ = std::max(1L, rect.right - rect.left);
    view_height_ = std::max(1L, rect.bottom - rect.top);
}

void CefRuntimeWindowHostWin::NotifyBrowserWasResized() {
    if (browser_ && browser_->GetHost()) {
        browser_->GetHost()->WasResized();
        browser_->GetHost()->NotifyScreenInfoChanged();
    }
}

void CefRuntimeWindowHostWin::NotifyBrowserFocus(bool focused) {
    if (browser_ && browser_->GetHost()) {
        browser_->GetHost()->SetFocus(focused);
        browser_->GetHost()->SendCaptureLostEvent();
    }
}

uint32_t CefRuntimeWindowHostWin::CurrentModifiers() const {
    uint32_t modifiers = 0;
    if (GetKeyState(VK_SHIFT) & 0x8000) modifiers |= EVENTFLAG_SHIFT_DOWN;
    if (GetKeyState(VK_CONTROL) & 0x8000) modifiers |= EVENTFLAG_CONTROL_DOWN;
    if (GetKeyState(VK_MENU) & 0x8000) modifiers |= EVENTFLAG_ALT_DOWN;
    if (GetKeyState(VK_LBUTTON) & 0x8000) modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    if (GetKeyState(VK_MBUTTON) & 0x8000) modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    if (GetKeyState(VK_RBUTTON) & 0x8000) modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    return modifiers;
}

bool CefRuntimeWindowHostWin::HandleMouseMove(UINT message, WPARAM wparam, LPARAM lparam) {
    (void)wparam;
    if (!browser_ || !browser_->GetHost()) {
        return false;
    }
    CefMouseEvent event;
    event.x = GET_X_LPARAM(lparam);
    event.y = GET_Y_LPARAM(lparam);
    event.modifiers = CurrentModifiers();
    const bool mouse_leave = (message == WM_MOUSELEAVE);
    if (!mouse_leave && !tracking_mouse_leave_ && window_) {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = window_;
        TrackMouseEvent(&track);
        tracking_mouse_leave_ = true;
    }
    if (mouse_leave) {
        tracking_mouse_leave_ = false;
    }
    browser_->GetHost()->SendMouseMoveEvent(event, mouse_leave);
    return true;
}

bool CefRuntimeWindowHostWin::HandleMouseButton(UINT message, WPARAM wparam, LPARAM lparam, bool mouse_up) {
    (void)wparam;
    if (!browser_ || !browser_->GetHost()) {
        return false;
    }
    CefBrowserHost::MouseButtonType button_type = MBT_LEFT;
    switch (message) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            button_type = MBT_LEFT;
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
            button_type = MBT_MIDDLE;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
            button_type = MBT_RIGHT;
            break;
        default:
            return false;
    }
    CefMouseEvent event;
    event.x = GET_X_LPARAM(lparam);
    event.y = GET_Y_LPARAM(lparam);
    event.modifiers = CurrentModifiers();
    const bool double_click =
        message == WM_LBUTTONDBLCLK || message == WM_MBUTTONDBLCLK || message == WM_RBUTTONDBLCLK;
    if (!mouse_up && window_) {
        SetCapture(window_);
    } else if (mouse_up && GetCapture() == window_) {
        ReleaseCapture();
    }
    browser_->GetHost()->SendMouseClickEvent(event, button_type, mouse_up, double_click ? 2 : 1);
    return true;
}

bool CefRuntimeWindowHostWin::HandleMouseWheel(UINT message, WPARAM wparam, LPARAM lparam) {
    if (!browser_ || !browser_->GetHost()) {
        return false;
    }
    POINT pt{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    if (window_) {
        ScreenToClient(window_, &pt);
    }
    CefMouseEvent event;
    event.x = pt.x;
    event.y = pt.y;
    event.modifiers = CurrentModifiers();
    const int delta = GET_WHEEL_DELTA_WPARAM(wparam);
    const int delta_x = (message == WM_MOUSEHWHEEL) ? delta : 0;
    const int delta_y = (message == WM_MOUSEWHEEL) ? delta : 0;
    browser_->GetHost()->SendMouseWheelEvent(event, delta_x, delta_y);
    return true;
}

bool CefRuntimeWindowHostWin::HandleAccelerator(UINT message, WPARAM wparam, LPARAM lparam, bool key_up) {
    (void)lparam;
    if (key_up || !browser_ || !browser_->GetHost()) {
        return false;
    }

    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool alt = (message == WM_SYSKEYDOWN) || ((GetKeyState(VK_MENU) & 0x8000) != 0);

    if (wparam == VK_F5 || (ctrl && wparam == 'R')) {
        browser_->Reload();
        return true;
    }
    if (alt && wparam == VK_LEFT) {
        if (browser_->CanGoBack()) {
            browser_->GoBack();
        }
        return true;
    }
    if (alt && wparam == VK_RIGHT) {
        if (browser_->CanGoForward()) {
            browser_->GoForward();
        }
        return true;
    }
    if (ctrl && !shift && wparam == 'T') {
        return callbacks_.tab_new && callbacks_.tab_new();
    }
    if (ctrl && wparam == 'W') {
        return callbacks_.tab_close_active && callbacks_.tab_close_active();
    }
    if (ctrl && shift && wparam == 'T') {
        return callbacks_.tab_reopen_closed && callbacks_.tab_reopen_closed();
    }
    if (ctrl && wparam == VK_TAB) {
        return callbacks_.tab_advance && callbacks_.tab_advance(shift ? -1 : 1);
    }
    if (alt && wparam >= '1' && wparam <= '9') {
        const std::size_t index = (wparam == '9') ? 8u : static_cast<std::size_t>(wparam - '1');
        return callbacks_.tab_activate && callbacks_.tab_activate(index);
    }
    return false;
}

bool CefRuntimeWindowHostWin::HandleKey(UINT message, WPARAM wparam, LPARAM lparam, bool key_up) {
    if (!browser_ || !browser_->GetHost()) {
        return false;
    }
    if (HandleAccelerator(message, wparam, lparam, key_up)) {
        return true;
    }
    CefKeyEvent event;
    event.type = key_up ? KEYEVENT_KEYUP : KEYEVENT_RAWKEYDOWN;
    event.windows_key_code = static_cast<int>(wparam);
    event.native_key_code = static_cast<int>(lparam);
    event.modifiers = CurrentModifiers();
    event.is_system_key = (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP);
    browser_->GetHost()->SendKeyEvent(event);
    return true;
}

bool CefRuntimeWindowHostWin::HandleChar(UINT message, WPARAM wparam, LPARAM lparam) {
    if (!browser_ || !browser_->GetHost()) {
        return false;
    }
    CefKeyEvent event;
    event.type = KEYEVENT_CHAR;
    event.windows_key_code = static_cast<int>(wparam);
    event.native_key_code = static_cast<int>(lparam);
    event.modifiers = CurrentModifiers();
    event.is_system_key = (message == WM_SYSCHAR);
    browser_->GetHost()->SendKeyEvent(event);
    return true;
}

void CefRuntimeWindowHostWin::PaintFrameToWindow(HDC dc) {
    RECT rect{0, 0, view_width_, view_height_};
    FillRect(dc, &rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    if (!saw_frame_ || last_frame_argb_.empty() || last_frame_width_ <= 0 || last_frame_height_ <= 0) {
        return;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = last_frame_width_;
    bmi.bmiHeader.biHeight = -last_frame_height_;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(dc,
                  0,
                  0,
                  view_width_,
                  view_height_,
                  0,
                  0,
                  last_frame_width_,
                  last_frame_height_,
                  last_frame_argb_.data(),
                  &bmi,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}
#endif

CefRuntimeWindowHostWin::~CefRuntimeWindowHostWin() {
#if defined(_WIN32)
    if (window_) {
        DestroyWindow(window_);
        window_ = nullptr;
        parent_handle_ = 0;
    }
#endif
    browser_ = nullptr;
}

bool CefRuntimeWindowHostWin::Initialize() {
    WinHostLog("Initialize begin");
    initialized_ = true;
    close_requested_ = false;
    quit_requested_ = false;
    view_width_ = 1280;
    view_height_ = 800;
    last_frame_width_ = 0;
    last_frame_height_ = 0;
    last_frame_stride_bytes_ = 0;
    saw_frame_ = false;
    last_frame_argb_.clear();
    parent_handle_ = 0;
#if defined(_WIN32)
    instance_ = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &CefRuntimeWindowHostWin::WindowProc;
    wc.hInstance = instance_;
    wc.lpszClassName = kWindowClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);

    RECT frame_rect{0, 0, view_width_, view_height_};
    AdjustWindowRect(&frame_rect, WS_OVERLAPPEDWINDOW, FALSE);
    window_ = CreateWindowExW(0,
                              kWindowClassName,
                              L"BRIDGE — Windows Runtime Host",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              frame_rect.right - frame_rect.left,
                              frame_rect.bottom - frame_rect.top,
                              nullptr,
                              nullptr,
                              instance_,
                              this);
    if (!window_) {
        initialized_ = false;
        WinHostLog("CreateWindowExW failed");
        return false;
    }
    parent_handle_ = reinterpret_cast<CefWindowHandle>(window_);
    ShowWindow(window_, SW_SHOW);
    UpdateWindow(window_);
    SyncViewSizeFromClientRect();
    WinHostLog("native window created and shown");
#endif
    WinHostLog("Initialize success");
    return true;
}

CefWindowHandle CefRuntimeWindowHostWin::parent_handle() const {
    return parent_handle_;
}

void CefRuntimeWindowHostWin::SetProfileLabel(const std::string& profile_label) {
    profile_label_ = profile_label;
}

void CefRuntimeWindowHostWin::SetWindowTitle(const std::string& title) {
    window_title_ = title;
#if defined(_WIN32)
    if (window_) {
        const std::wstring wide = Utf8ToWide(title.empty() ? std::string("BRIDGE — Windows Runtime Host") : title);
        SetWindowTextW(window_, wide.c_str());
    }
#endif
}

void CefRuntimeWindowHostWin::SetCurrentUrl(const std::string& url) {
    current_url_ = url;
}

void CefRuntimeWindowHostWin::SetLoadingState(bool is_loading, bool can_go_back, bool can_go_forward) {
    is_loading_ = is_loading;
    can_go_back_ = can_go_back;
    can_go_forward_ = can_go_forward;
}

void CefRuntimeWindowHostWin::SetLoadError(const std::string& error_text, const std::string& failed_url) {
    load_error_text_ = error_text;
    failed_url_ = failed_url;
}

void CefRuntimeWindowHostWin::SetTabStatus(std::size_t active_tab_index, std::size_t tab_count) {
    active_tab_index_ = active_tab_index;
    tab_count_ = tab_count > 0 ? tab_count : 1;
}

void CefRuntimeWindowHostWin::SetTabStripItems(std::vector<TabUiItem> items) {
    tab_strip_items_ = std::move(items);
}

void CefRuntimeWindowHostWin::SetCallbacks(Callbacks callbacks) {
    callbacks_ = std::move(callbacks);
}

void CefRuntimeWindowHostWin::NotifyBrowserCreated(CefRefPtr<CefBrowser> browser) {
    browser_ = browser;
#if defined(_WIN32)
    WinHostLog("browser attached to runtime window host");
    NotifyBrowserWasResized();
    NotifyBrowserFocus(window_ != nullptr && GetFocus() == window_);
#endif
}

void CefRuntimeWindowHostWin::NotifyBrowserClosing(CefRefPtr<CefBrowser> browser) {
    if (browser_ && browser && browser_->IsSame(browser)) {
        browser_ = nullptr;
    }
#if defined(_WIN32)
    WinHostLog("browser closing notification received");
    if (close_requested_ && window_) {
        WinHostLog("destroying native window after browser close notification");
        DestroyWindow(window_);
    }
#endif
}

bool CefRuntimeWindowHostWin::PresentFrame(const std::uint32_t* argb, int width, int height, int stride_bytes) {
    if (!initialized_ || !argb || width <= 0 || height <= 0 || stride_bytes <= 0) {
        return false;
    }
    const bool first_frame = !saw_frame_;
    const std::size_t pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    last_frame_argb_.assign(argb, argb + pixel_count);
    last_frame_width_ = width;
    last_frame_height_ = height;
    last_frame_stride_bytes_ = stride_bytes;
    saw_frame_ = true;
#if defined(_WIN32)
    if (first_frame) {
        WinHostLog("first frame received: " + std::to_string(width) + "x" + std::to_string(height));
    }
    if (window_) {
        InvalidateRect(window_, nullptr, FALSE);
    }
#endif
    return true;
}

bool CefRuntimeWindowHostWin::GetRootScreenRect(CefRect& rect) const {
    rect = CefRect(0, 0, view_width_, view_height_);
    return true;
}

void CefRuntimeWindowHostWin::GetViewRect(CefRect& rect) const {
    rect = CefRect(0, 0, view_width_, view_height_);
}

bool CefRuntimeWindowHostWin::GetScreenPoint(int view_x, int view_y, int& screen_x, int& screen_y) const {
#if defined(_WIN32)
    if (window_) {
        POINT pt{view_x, view_y};
        if (ClientToScreen(window_, &pt)) {
            screen_x = pt.x;
            screen_y = pt.y;
            return true;
        }
    }
#endif
    screen_x = view_x;
    screen_y = view_y;
    return true;
}

bool CefRuntimeWindowHostWin::GetScreenInfo(CefScreenInfo& screen_info) const {
    screen_info.device_scale_factor = 1.0f;
    screen_info.depth = 32;
    screen_info.depth_per_component = 8;
    screen_info.is_monochrome = false;
    screen_info.rect = CefRect(0, 0, view_width_, view_height_);
    screen_info.available_rect = screen_info.rect;
    return true;
}

bool CefRuntimeWindowHostWin::RunFileDialog(const FileDialogParams& params) {
#if defined(_WIN32)
    WinHostLog("RunFileDialog invoked");
    if (!params.callback) {
        return false;
    }

    if (params.mode == FILE_DIALOG_OPEN_FOLDER) {
        BROWSEINFOW bi{};
        std::wstring title = Utf8ToWide(params.title);
        bi.hwndOwner = window_;
        bi.lpszTitle = title.empty() ? nullptr : title.c_str();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
        if (!pidl) {
            params.callback->Cancel();
            return true;
        }
        wchar_t path[MAX_PATH] = {0};
        std::vector<CefString> selected_paths;
        if (SHGetPathFromIDListW(pidl, path)) {
            selected_paths.emplace_back(WideToUtf8(path));
            params.callback->Continue(selected_paths);
        } else {
            params.callback->Cancel();
        }
        CoTaskMemFree(pidl);
        return true;
    }

    std::wstring title = Utf8ToWide(params.title);
    std::wstring default_path = Utf8ToWide(params.default_file_path);
    std::wstring filter = BuildWin32Filter(params);
    std::vector<wchar_t> buffer(static_cast<std::size_t>(MAX_PATH) * 16, L'\0');
    if (!default_path.empty()) {
        const std::size_t copy_len = std::min(buffer.size() - 1, default_path.size());
        std::copy_n(default_path.data(), copy_len, buffer.data());
        buffer[copy_len] = L'\0';
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = window_;
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = static_cast<DWORD>(buffer.size());
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrTitle = title.empty() ? nullptr : title.c_str();
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
    if (params.mode == FILE_DIALOG_OPEN_MULTIPLE) {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    }
    if (params.mode == FILE_DIALOG_SAVE) {
        ofn.Flags |= OFN_OVERWRITEPROMPT;
    }

    const BOOL ok = (params.mode == FILE_DIALOG_SAVE) ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (!ok) {
        params.callback->Cancel();
        return true;
    }

    std::vector<CefString> selected_paths;
    const wchar_t* cursor = buffer.data();
    const std::wstring first = cursor;
    cursor += first.size() + 1;
    if (*cursor == L'\0') {
        selected_paths.emplace_back(WideToUtf8(first));
    } else {
        const std::wstring directory = first;
        while (*cursor != L'\0') {
            const std::wstring name = cursor;
            selected_paths.emplace_back(WideToUtf8(directory + L"\\" + name));
            cursor += name.size() + 1;
        }
    }
    params.callback->Continue(selected_paths);
    return true;
#else
    (void)params;
    return false;
#endif
}

bool CefRuntimeWindowHostWin::OpenUrlExternally(const std::string& url) {
#if defined(_WIN32)
    WinHostLog(std::string("OpenUrlExternally invoked: ") + url);
    if (url.empty()) {
        return false;
    }
    const std::wstring wide = Utf8ToWide(url);
    const HINSTANCE result = ShellExecuteW(window_, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
#else
    (void)url;
    return false;
#endif
}
