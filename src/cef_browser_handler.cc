#include "cef_browser_handler.h"

#include <sstream>
#include <string>

#if defined(__linux__)
#include <spawn.h>
#include <unistd.h>
extern char** environ;
#endif

#if defined(CEF_X11)
#include <gtk/gtk.h>
#endif

#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

namespace {

CefBrowserHandler* g_instance = nullptr;

std::string GetDataURI(const std::string& data, const std::string& mime_type) {
    return "data:" + mime_type + ";base64," +
           CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}

bool TryOpenExternally(const std::string& url) {
#if defined(__linux__)
    if (url.empty()) {
        return false;
    }
    pid_t pid = 0;
    char* argv[] = {const_cast<char*>("xdg-open"), const_cast<char*>(url.c_str()), nullptr};
    const int rc = posix_spawnp(&pid, "xdg-open", nullptr, nullptr, argv, environ);
    return rc == 0;
#else
    (void)url;
    return false;
#endif
}

bool IsMeaningfulPopupTarget(const std::string& url) {
    if (url.empty()) {
        return false;
    }
    if (url == "about:blank") {
        return false;
    }
    if (url.rfind("javascript:", 0) == 0) {
        return false;
    }
    return true;
}

}  // namespace

CefBrowserHandler::CefBrowserHandler(bool is_alloy_style,
                                     bool use_osr,
                                     bool quit_after_first_frame,
                                     bool verify_presentation_v2,
                                     bridge::cef::CefBackend::Ptr backend,
                                     std::shared_ptr<bridge::cef::IIntegrationBridge> bridge,
                                     CefOsrHostGtk* osr_host)
    : is_alloy_style_(is_alloy_style),
      use_osr_(use_osr),
      quit_after_first_frame_(quit_after_first_frame),
      verify_presentation_v2_(verify_presentation_v2),
      backend_(std::move(backend)),
      bridge_(std::move(bridge)),
      osr_host_(osr_host) {
    g_instance = this;
}

CefBrowserHandler::~CefBrowserHandler() {
    g_instance = nullptr;
}

CefBrowserHandler* CefBrowserHandler::GetInstance() {
    return g_instance;
}

void CefBrowserHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        const CefString& url) {
    CEF_REQUIRE_UI_THREAD();
    if (frame && frame->IsMain() && backend_) {
        backend_->observe_address_change(url.ToString());
    }
    if (frame && frame->IsMain() && osr_host_) {
        osr_host_->SetCurrentUrl(url.ToString());
    }
}

void CefBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    CEF_REQUIRE_UI_THREAD();
    if (backend_) {
        backend_->observe_title_change(title.ToString());
    }
    if (auto browser_view = CefBrowserView::GetForBrowser(browser)) {
        if (auto window = browser_view->GetWindow()) {
            window->SetTitle(title);
        }
    } else if (osr_host_) {
        osr_host_->SetWindowTitle(title.ToString());
    } else if (is_alloy_style_) {
        PlatformTitleChange(browser, title);
    }
}

bool CefBrowserHandler::OnFileDialog(CefRefPtr<CefBrowser> browser,
                                     FileDialogMode mode,
                                     const CefString& title,
                                     const CefString& default_file_path,
                                     const std::vector<CefString>& accept_filters,
                                     const std::vector<CefString>& accept_extensions,
                                     const std::vector<CefString>& accept_descriptions,
                                     CefRefPtr<CefFileDialogCallback> callback) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    (void)accept_filters;

#if !defined(CEF_X11)
    (void)mode;
    (void)title;
    (void)default_file_path;
    (void)accept_extensions;
    (void)accept_descriptions;
    (void)callback;
    return false;
#else
    if (!use_osr_ || !callback) {
        return false;
    }

    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    bool select_multiple = false;
    switch (mode) {
        case FILE_DIALOG_OPEN:
            action = GTK_FILE_CHOOSER_ACTION_OPEN;
            break;
        case FILE_DIALOG_OPEN_MULTIPLE:
            action = GTK_FILE_CHOOSER_ACTION_OPEN;
            select_multiple = true;
            break;
        case FILE_DIALOG_OPEN_FOLDER:
            action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
            break;
        case FILE_DIALOG_SAVE:
            action = GTK_FILE_CHOOSER_ACTION_SAVE;
            break;
        default:
            return false;
    }

    GtkFileChooserNative* dialog = gtk_file_chooser_native_new(
        title.empty() ? nullptr : title.ToString().c_str(), nullptr, action,
        action == GTK_FILE_CHOOSER_ACTION_SAVE ? "Save" : "Open", "Cancel");
    if (dialog == nullptr) {
        return false;
    }

    GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_set_select_multiple(chooser, select_multiple ? TRUE : FALSE);
    gtk_file_chooser_set_local_only(chooser, TRUE);
    if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
        gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    }

    const std::string default_path = default_file_path.ToString();
    if (!default_path.empty()) {
        if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
            gtk_file_chooser_set_filename(chooser, default_path.c_str());
        } else {
            gtk_file_chooser_set_current_folder(chooser, default_path.c_str());
            gtk_file_chooser_select_filename(chooser, default_path.c_str());
        }
    }

    for (std::size_t i = 0; i < accept_extensions.size(); ++i) {
        const std::string extensions = accept_extensions[i].ToString();
        const std::string description = i < accept_descriptions.size() ? accept_descriptions[i].ToString() : std::string();
        if (extensions.empty()) {
            continue;
        }
        GtkFileFilter* filter = gtk_file_filter_new();
        if (!description.empty()) {
            gtk_file_filter_set_name(filter, description.c_str());
        }
        std::size_t start = 0;
        while (start < extensions.size()) {
            const std::size_t end = extensions.find(';', start);
            const std::string ext = extensions.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (!ext.empty()) {
                const std::string pattern = ext[0] == '.' ? std::string("*") + ext : std::string("*.") + ext;
                gtk_file_filter_add_pattern(filter, pattern.c_str());
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
        gtk_file_chooser_add_filter(chooser, filter);
    }

    std::vector<CefString> selected_paths;
    const int response = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
        if (select_multiple) {
            GSList* files = gtk_file_chooser_get_filenames(chooser);
            for (GSList* node = files; node != nullptr; node = node->next) {
                if (node->data != nullptr) {
                    selected_paths.emplace_back(static_cast<const char*>(node->data));
                    g_free(node->data);
                }
            }
            g_slist_free(files);
        } else if (char* filename = gtk_file_chooser_get_filename(chooser); filename != nullptr) {
            selected_paths.emplace_back(filename);
            g_free(filename);
        }
        callback->Continue(selected_paths);
    } else {
        callback->Cancel();
    }

    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(dialog));
    g_object_unref(dialog);
    return true;
#endif
}

bool CefBrowserHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      int popup_id,
                                      const CefString& target_url,
                                      const CefString& target_frame_name,
                                      WindowOpenDisposition target_disposition,
                                      bool user_gesture,
                                      const CefPopupFeatures& popupFeatures,
                                      CefWindowInfo& windowInfo,
                                      CefRefPtr<CefClient>& client,
                                      CefBrowserSettings& settings,
                                      CefRefPtr<CefDictionaryValue>& extra_info,
                                      bool* no_javascript_access) {
    CEF_REQUIRE_UI_THREAD();
    (void)popup_id;
    (void)target_frame_name;
    (void)target_disposition;
    (void)user_gesture;
    (void)popupFeatures;
    (void)windowInfo;
    (void)client;
    (void)settings;
    (void)extra_info;
    (void)no_javascript_access;

    if (!use_osr_) {
        return false;
    }

    const std::string url = target_url.ToString();
    if (!IsMeaningfulPopupTarget(url)) {
        LOG(WARNING) << "engine-cef runtime-host blocked non-meaningful popup target: "
                     << (url.empty() ? std::string("<empty>") : url);
        return true;
    }

    if (frame && frame->IsValid()) {
        LOG(WARNING) << "engine-cef runtime-host popup redirected into current window: " << url;
        frame->LoadURL(url);
        return true;
    }
    if (browser && browser->GetMainFrame()) {
        LOG(WARNING) << "engine-cef runtime-host popup redirected into current main frame: " << url;
        browser->GetMainFrame()->LoadURL(url);
        return true;
    }
    if (TryOpenExternally(url)) {
        LOG(WARNING) << "engine-cef runtime-host popup fell back to external handoff: " << url;
        return true;
    }
    LOG(WARNING) << "engine-cef runtime-host blocked popup after failing in-window + external handling: " << url;
    return true;
}

void CefBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    browser_list_.push_back(browser);
    if (use_osr_) {
        if (osr_host_) {
            osr_host_->NotifyBrowserCreated(browser);
        } else {
            browser->GetHost()->NotifyScreenInfoChanged();
            browser->GetHost()->WasHidden(false);
            browser->GetHost()->SetFocus(true);
            browser->GetHost()->WasResized();
        }
    }
}

bool CefBrowserHandler::DoClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    if (browser_list_.size() == 1) {
        is_closing_ = true;
    }
    return false;
}

void CefBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    for (auto it = browser_list_.begin(); it != browser_list_.end(); ++it) {
        if ((*it)->IsSame(browser)) {
            browser_list_.erase(it);
            break;
        }
    }
    if (browser_list_.empty()) {
        CefQuitMessageLoop();
    }
}

void CefBrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                             bool isLoading,
                                             bool canGoBack,
                                             bool canGoForward) {
    CEF_REQUIRE_UI_THREAD();
    if (backend_) {
        backend_->observe_loading_state(isLoading, canGoBack, canGoForward);
    }
    if (osr_host_) {
        osr_host_->SetLoadingState(isLoading, canGoBack, canGoForward);
    }
}

void CefBrowserHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  int httpStatusCode) {
    CEF_REQUIRE_UI_THREAD();
    if (frame && frame->IsMain() && backend_) {
        backend_->observe_load_end();
    }
}

void CefBrowserHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               ErrorCode errorCode,
                               const CefString& errorText,
                               const CefString& failedUrl) {
    CEF_REQUIRE_UI_THREAD();
    if (errorCode == ERR_ABORTED) {
        return;
    }
    if (backend_ && frame && frame->IsMain()) {
        backend_->observe_load_error(errorText.ToString());
    }
    if (frame && frame->IsMain() && osr_host_) {
        osr_host_->SetLoadError(errorText.ToString(), failedUrl.ToString());
    }
    if (!is_alloy_style_) {
        return;
    }

    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\"><h2>Failed to load URL "
       << std::string(failedUrl) << " with error " << std::string(errorText)
       << " (" << errorCode << ").</h2></body></html>";
    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

bool CefBrowserHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    CEF_REQUIRE_UI_THREAD();
    if (osr_host_) {
        return osr_host_->GetRootScreenRect(rect);
    }
    GetViewRect(browser, rect);
    return true;
}

bool CefBrowserHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser,
                                       int viewX,
                                       int viewY,
                                       int& screenX,
                                       int& screenY) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    if (osr_host_) {
        return osr_host_->GetScreenPoint(viewX, viewY, screenX, screenY);
    }
    screenX = viewX;
    screenY = viewY;
    return true;
}

bool CefBrowserHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) {
    CEF_REQUIRE_UI_THREAD();
    if (osr_host_) {
        return osr_host_->GetScreenInfo(screen_info);
    }
    CefRect rect;
    GetViewRect(browser, rect);
    screen_info.device_scale_factor = 1.0f;
    screen_info.depth = 32;
    screen_info.depth_per_component = 8;
    screen_info.is_monochrome = false;
    screen_info.rect = rect;
    screen_info.available_rect = rect;
    return true;
}

void CefBrowserHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    CEF_REQUIRE_UI_THREAD();
    (void)browser;
    if (osr_host_) {
        osr_host_->GetViewRect(rect);
        return;
    }
    const auto presentation = backend_ ? backend_->presentation_state() : bridge::cef::PresentationState{};
    const int width = presentation.width > 0 ? presentation.width : 1280;
    const int height = presentation.height > 0 ? presentation.height : 800;
    rect = CefRect(0, 0, width, height);
}

void CefBrowserHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                                PaintElementType type,
                                const RectList& dirtyRects,
                                const void* buffer,
                                int width,
                                int height) {
    CEF_REQUIRE_UI_THREAD();
    (void)dirtyRects;
    if (type != PET_VIEW || !backend_ || buffer == nullptr) {
        return;
    }
    backend_->observe_frame(static_cast<const std::uint32_t*>(buffer),
                            width,
                            height,
                            width * static_cast<int>(sizeof(std::uint32_t)));
    if (osr_host_) {
        osr_host_->PresentFrame(static_cast<const std::uint32_t*>(buffer),
                                width,
                                height,
                                width * static_cast<int>(sizeof(std::uint32_t)));
    }
    if (!saw_first_frame_) {
        LOG(WARNING) << "engine-cef osr first frame " << width << "x" << height;
    }
    if (verify_presentation_v2_ && !verified_presentation_v2_) {
        const auto presentation = bridge_ ? bridge_->presentation_state() : backend_->presentation_state();
        std::string copy_error;
        std::vector<std::uint32_t> copied(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
        const bool copy_ok = bridge_
                                 ? bridge_->copy_latest_frame(copied.data(),
                                                              width,
                                                              height,
                                                              width * static_cast<int>(sizeof(std::uint32_t)),
                                                              &copy_error)
                                 : backend_->copy_latest_frame(copied.data(),
                                                               width,
                                                               height,
                                                               width * static_cast<int>(sizeof(std::uint32_t)),
                                                               &copy_error);
        if (presentation.has_frame && presentation.width == width && presentation.height == height && copy_ok) {
            verified_presentation_v2_ = true;
            fprintf(stderr,
                    "engine-cef verified presentation-v2 bridge frame copy %dx%d generation=%llu\n",
                    presentation.width,
                    presentation.height,
                    static_cast<unsigned long long>(presentation.frame_generation));
        } else {
            fprintf(stderr,
                    "engine-cef failed presentation-v2 bridge verification has_frame=%d size=%dx%d copy_ok=%d error=%s\n",
                    presentation.has_frame ? 1 : 0,
                    presentation.width,
                    presentation.height,
                    copy_ok ? 1 : 0,
                    copy_error.c_str());
        }
    }
    if (quit_after_first_frame_ && !saw_first_frame_) {
        saw_first_frame_ = true;
        CefPostTask(TID_UI, base::BindOnce(&CefQuitMessageLoop));
    }
}

void CefBrowserHandler::CloseAllBrowsers(bool force_close) {
    if (!CefCurrentlyOn(TID_UI)) {
        CefPostTask(TID_UI,
                    base::BindOnce(&CefBrowserHandler::CloseAllBrowsers,
                                   CefRefPtr<CefBrowserHandler>(this),
                                   force_close));
        return;
    }
    for (const auto& browser : browser_list_) {
        browser->GetHost()->CloseBrowser(force_close);
    }
}

void CefBrowserHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    (void)browser;
    (void)title;
}

void CefBrowserHandler::PlatformShowWindow(CefRefPtr<CefBrowser> browser) {
    (void)browser;
}
