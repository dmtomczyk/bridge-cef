#include "cef_integration_bridge.h"

namespace bridge::cef {

CefIntegrationBridge::CefIntegrationBridge()
    : backend_(std::make_shared<CefBackend>()) {}

CefIntegrationBridge::CefIntegrationBridge(CefBackend::Ptr backend)
    : backend_(backend ? std::move(backend) : std::make_shared<CefBackend>()) {}

bool CefIntegrationBridge::initialize(const InitParams& params, std::string* error_out) {
    attach_observer();
    const bool ok = backend_->initialize(params, error_out);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot_cache_ = backend_->snapshot();
    }
    return ok;
}

void CefIntegrationBridge::shutdown() {
    backend_->shutdown();
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_cache_ = BackendSnapshot{};
}

void CefIntegrationBridge::navigate(const std::string& url) {
    backend_->navigate(url);
}

void CefIntegrationBridge::reload() {
    backend_->reload();
}

void CefIntegrationBridge::go_back() {
    backend_->go_back();
}

void CefIntegrationBridge::go_forward() {
    backend_->go_forward();
}

void CefIntegrationBridge::resize(int width, int height) {
    backend_->resize(width, height);
}

void CefIntegrationBridge::tick() {
    backend_->tick();
}

BackendSnapshot CefIntegrationBridge::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_cache_;
}

std::string CefIntegrationBridge::debug_summary() const {
    return backend_->debug_summary();
}

void CefIntegrationBridge::on_page_state_changed(const PageState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_cache_.page = state;
}

void CefIntegrationBridge::on_load_state_changed(const LoadState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_cache_.load = state;
}

void CefIntegrationBridge::attach_observer() {
    backend_->set_observer(shared_from_this());
}

}  // namespace bridge::cef
