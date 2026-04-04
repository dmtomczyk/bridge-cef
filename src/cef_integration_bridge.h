#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "cef_backend.h"

namespace bridge::cef {

class CefIntegrationBridge final : public IBackendObserver,
                                   public std::enable_shared_from_this<CefIntegrationBridge> {
public:
    using Ptr = std::shared_ptr<CefIntegrationBridge>;

    CefIntegrationBridge();
    explicit CefIntegrationBridge(CefBackend::Ptr backend);

    bool initialize(const InitParams& params, std::string* error_out = nullptr);
    void shutdown();

    void navigate(const std::string& url);
    void reload();
    void go_back();
    void go_forward();
    void resize(int width, int height);
    void tick();

    BackendSnapshot snapshot() const;
    std::string debug_summary() const;

    CefBackend::Ptr backend() const { return backend_; }

    void on_page_state_changed(const PageState& state) override;
    void on_load_state_changed(const LoadState& state) override;

private:
    void attach_observer();

    CefBackend::Ptr backend_;
    mutable std::mutex mutex_{};
    BackendSnapshot snapshot_cache_{};
};

}  // namespace bridge::cef
