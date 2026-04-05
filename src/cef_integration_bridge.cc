#include "cef_integration_bridge.h"

namespace bridge::cef {

std::shared_ptr<IIntegrationBridge> create_integration_bridge() {
    return std::make_shared<CefIntegrationBridge>();
}

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

PresentationState CefIntegrationBridge::presentation_state() const {
    return backend_->presentation_state();
}

bool CefIntegrationBridge::copy_latest_frame(std::uint32_t* dst_argb,
                                             int width,
                                             int height,
                                             int stride_bytes,
                                             std::string* error_out) const {
    return backend_->copy_latest_frame(dst_argb, width, height, stride_bytes, error_out);
}

std::string CefIntegrationBridge::debug_summary() const {
    return backend_->debug_summary();
}

void CefIntegrationBridge::set_observer(std::shared_ptr<IIntegrationBridgeObserver> observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    bridge_observer_ = std::move(observer);
}

void CefIntegrationBridge::on_page_state_changed(const PageState& state) {
    std::shared_ptr<IIntegrationBridgeObserver> observer;
    BackendSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot_cache_.page = state;
        snapshot = snapshot_cache_;
        observer = bridge_observer_;
    }
    if (observer) {
        observer->on_snapshot_changed(snapshot);
    }
}

void CefIntegrationBridge::on_load_state_changed(const LoadState& state) {
    std::shared_ptr<IIntegrationBridgeObserver> observer;
    BackendSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot_cache_.load = state;
        snapshot = snapshot_cache_;
        observer = bridge_observer_;
    }
    if (observer) {
        observer->on_snapshot_changed(snapshot);
    }
}

void CefIntegrationBridge::on_presentation_state_changed(const PresentationState& state) {
    std::shared_ptr<IIntegrationBridgeObserver> observer;
    BackendSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot_cache_.presentation = state;
        snapshot = snapshot_cache_;
        observer = bridge_observer_;
    }
    if (observer) {
        observer->on_snapshot_changed(snapshot);
    }
}

void CefIntegrationBridge::attach_observer() {
    backend_->set_observer(shared_from_this());
}

}  // namespace bridge::cef
