#include "cef_backend.h"

#include <sstream>

namespace bridge::cef {

bool CefBackend::initialize(const InitParams& params, std::string* error_out) {
    init_params_ = params;
    width_ = params.initial_width;
    height_ = params.initial_height;
    page_state_.current_url = params.initial_url;
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
    initialized_ = false;
    page_state_ = PageState{};
    load_state_ = LoadState{};
}

void CefBackend::navigate(const std::string& url) {
    page_state_.current_url = url;
    load_state_.loading = true;
    load_state_.last_error.clear();
}

void CefBackend::reload() {
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
    width_ = width;
    height_ = height;
}

void CefBackend::tick() {
    // Placeholder for future message-pump/state sync work.
}

PageState CefBackend::page_state() const {
    return page_state_;
}

LoadState CefBackend::load_state() const {
    return load_state_;
}

std::string CefBackend::debug_summary() const {
    std::ostringstream out;
    out << "engine-cef initialized=" << (initialized_ ? 1 : 0)
        << " size=" << width_ << "x" << height_
        << " url=" << page_state_.current_url
        << " loading=" << (load_state_.loading ? 1 : 0)
        << " first_page_shown=" << (load_state_.first_page_shown ? 1 : 0);
    if (!load_state_.last_error.empty()) {
        out << " error=" << load_state_.last_error;
    }
    return out.str();
}

}  // namespace bridge::cef
