#include "cef_runtime_entry.h"

#include <utility>

#include "include/cef_app.h"

namespace {

class RuntimeBridgeObserver final : public bridge::cef::IIntegrationBridgeObserver {
public:
    explicit RuntimeBridgeObserver(std::shared_ptr<CefRuntimeHost::RuntimeState> state)
        : state_(std::move(state)) {}

    void on_snapshot_changed(const bridge::cef::BackendSnapshot& snapshot) override;

private:
    std::shared_ptr<CefRuntimeHost::RuntimeState> state_;
};

}  // namespace

struct CefRuntimeHost::RuntimeState {
    mutable std::mutex mutex;
    CefRuntimeStatus status{};
    std::shared_ptr<bridge::cef::IIntegrationBridgeObserver> external_bridge_observer{};
    std::shared_ptr<ICefRuntimeObserver> runtime_observer{};
    std::shared_ptr<bridge::cef::IIntegrationBridgeObserver> bridge_mux_observer{};
};

void RuntimeBridgeObserver::on_snapshot_changed(const bridge::cef::BackendSnapshot& snapshot) {
    std::shared_ptr<bridge::cef::IIntegrationBridgeObserver> external_bridge_observer;
    std::shared_ptr<ICefRuntimeObserver> runtime_observer;
    CefRuntimeStatus status;
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->status.last_snapshot = snapshot;
        if (snapshot.presentation.has_frame && !state_->status.saw_first_frame) {
            state_->status.saw_first_frame = true;
            state_->status.phase = CefRuntimePhase::first_frame_ready;
        }
        status = state_->status;
        external_bridge_observer = state_->external_bridge_observer;
        runtime_observer = state_->runtime_observer;
    }
    if (external_bridge_observer) {
        external_bridge_observer->on_snapshot_changed(snapshot);
    }
    if (runtime_observer) {
        runtime_observer->on_runtime_status_changed(status);
    }
}

std::shared_ptr<CefRuntimeHost::RuntimeState> CefRuntimeHost::EnsureState() const {
    if (!state_) {
        state_ = std::make_shared<RuntimeState>();
    }
    return state_;
}

void CefRuntimeHost::EnsureApp() const {
    auto state = EnsureState();
    if (!app_) {
        app_ = new CefAppHost;
    }
    if (!state->bridge_mux_observer) {
        state->bridge_mux_observer = std::make_shared<RuntimeBridgeObserver>(state);
        app_->bridge()->set_observer(state->bridge_mux_observer);
    }
}

std::shared_ptr<bridge::cef::IIntegrationBridge> CefRuntimeHost::bridge() const {
    EnsureApp();
    return app_->bridge();
}

void CefRuntimeHost::set_bridge_observer(std::shared_ptr<bridge::cef::IIntegrationBridgeObserver> observer) const {
    auto state = EnsureState();
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->external_bridge_observer = std::move(observer);
    }
    EnsureApp();
}

void CefRuntimeHost::set_runtime_observer(std::shared_ptr<ICefRuntimeObserver> observer) const {
    auto state = EnsureState();
    std::lock_guard<std::mutex> lock(state->mutex);
    state->runtime_observer = std::move(observer);
}

CefRuntimeStatus CefRuntimeHost::runtime_status() const {
    auto state = EnsureState();
    std::lock_guard<std::mutex> lock(state->mutex);
    return state->status;
}

int CefRuntimeHost::Run(int argc, char* argv[]) const {
    EnsureApp();
    auto state = EnsureState();
    std::shared_ptr<ICefRuntimeObserver> runtime_observer;
    CefRuntimeStatus status;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->status.phase = CefRuntimePhase::running;
        state->status.last_error.clear();
        state->status.last_exit_code = 0;
        runtime_observer = state->runtime_observer;
        status = state->status;
    }
    if (runtime_observer) {
        runtime_observer->on_runtime_status_changed(status);
    }

    app_->set_launch_config(config_.launch);
    const int exit_code = CefLinuxMainRunner::Run(argc, argv, app_, config_.runner);

    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->status.last_exit_code = exit_code;
        state->status.phase = (exit_code == 0) ? CefRuntimePhase::stopped : CefRuntimePhase::failed;
        if (exit_code != 0) {
            state->status.last_error = "engine-cef runtime exited with non-zero code";
        }
        runtime_observer = state->runtime_observer;
        status = state->status;
    }
    if (runtime_observer) {
        runtime_observer->on_runtime_status_changed(status);
    }
    return exit_code;
}

void CefRuntimeHost::RequestQuit() {
    CefQuitMessageLoop();
}

int RunCefRuntimeEntry(int argc, char* argv[], const CefRuntimeEntryConfig& config) {
    return CefRuntimeHost(config).Run(argc, argv);
}
