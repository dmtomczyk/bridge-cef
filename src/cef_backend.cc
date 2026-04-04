#include "cef_backend.h"

#include <sstream>

namespace bridge::cef {

bool CefBackend::initialize(const InitParams& params, std::string* error_out) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_params_ = params;
    width_ = params.initial_width;
    height_ = params.initial_height;
    page_state_.current_url = params.initial_url;
    page_state_.title.clear();
    page_state_.can_go_back = false;
    page_state_.can_go_forward = false;
    load_state_.loading = false;
    load_state_.first_page_shown = false;
    load_state_.last_error.clear();
    initialized_ = true;
    if (error_out) {
        error_out->clear();
    }
    return true;
}

void CefBackend::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = false;
    page_state_ = PageState{};
    load_state_ = LoadState{};
}

void CefBackend::navigate(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    page_state_.current_url = url;
    load_state_.loading = true;
    load_state_.last_error.clear();
}

void CefBackend::reload() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        load_state_.loading = true;
        load_state_.last_error.clear();
    }
}

void CefBackend::go_back() {
    // Placeholder until real browser-state plumbing lands.
}

void CefBackend::go_forward() {
    // Placeholder until real browser-state plumbing lands.
}

void CefBackend::resize(int width, int height) {
    std::lock_guard<std::mutex> lock(mutex_);
    width_ = width;
    height_ = height;
}

void CefBackend::tick() {
    // Placeholder for future message-pump/state sync work.
}

PageState CefBackend::page_state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return page_state_;
}

LoadState CefBackend::load_state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return load_state_;
}

BackendSnapshot CefBackend::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return BackendSnapshot{page_state_, load_state_};
}

void CefBackend::set_observer(std::shared_ptr<IBackendObserver> observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    observer_ = std::move(observer);
}

std::string CefBackend::debug_summary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    out << "engine-cef initialized=" << (initialized_ ? 1 : 0)
        << " size=" << width_ << "x" << height_
        << " url=" << page_state_.current_url
        << " title=" << page_state_.title
        << " loading=" << (load_state_.loading ? 1 : 0)
        << " first_page_shown=" << (load_state_.first_page_shown ? 1 : 0)
        << " can_go_back=" << (page_state_.can_go_back ? 1 : 0)
        << " can_go_forward=" << (page_state_.can_go_forward ? 1 : 0);
    if (!load_state_.last_error.empty()) {
        out << " error=" << load_state_.last_error;
    }
    return out.str();
}

void CefBackend::observe_address_change(const std::string& url) {
    std::shared_ptr<IBackendObserver> observer;
    PageState page;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        page_state_.current_url = url;
        observer = observer_;
        page = page_state_;
    }
    if (observer) {
        observer->on_page_state_changed(page);
    }
}

void CefBackend::observe_title_change(const std::string& title) {
    std::shared_ptr<IBackendObserver> observer;
    PageState page;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        page_state_.title = title;
        observer = observer_;
        page = page_state_;
    }
    if (observer) {
        observer->on_page_state_changed(page);
    }
}

void CefBackend::observe_loading_state(bool loading, bool can_go_back, bool can_go_forward) {
    std::shared_ptr<IBackendObserver> observer;
    PageState page;
    LoadState load;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        load_state_.loading = loading;
        page_state_.can_go_back = can_go_back;
        page_state_.can_go_forward = can_go_forward;
        if (loading) {
            load_state_.last_error.clear();
        }
        observer = observer_;
        page = page_state_;
        load = load_state_;
    }
    if (observer) {
        observer->on_page_state_changed(page);
        observer->on_load_state_changed(load);
    }
}

void CefBackend::observe_load_end() {
    std::shared_ptr<IBackendObserver> observer;
    LoadState load;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        load_state_.loading = false;
        load_state_.first_page_shown = true;
        load_state_.last_error.clear();
        observer = observer_;
        load = load_state_;
    }
    if (observer) {
        observer->on_load_state_changed(load);
    }
}

void CefBackend::observe_load_error(const std::string& error) {
    std::shared_ptr<IBackendObserver> observer;
    LoadState load;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        load_state_.loading = false;
        load_state_.last_error = error;
        observer = observer_;
        load = load_state_;
    }
    if (observer) {
        observer->on_load_state_changed(load);
    }
}

}  // namespace bridge::cef
