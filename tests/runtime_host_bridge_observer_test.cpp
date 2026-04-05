#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "cef_integration_bridge.h"
#include "cef_runtime_entry.h"

namespace {

class RecordingBridgeObserver final : public bridge::cef::IIntegrationBridgeObserver {
public:
    void on_snapshot_changed(const bridge::cef::BackendSnapshot& snapshot) override {
        ++event_count;
        last_snapshot = snapshot;
    }

    int event_count = 0;
    bridge::cef::BackendSnapshot last_snapshot{};
};

class RecordingRuntimeObserver final : public ICefRuntimeObserver {
public:
    void on_runtime_status_changed(const CefRuntimeStatus& status) override {
        ++event_count;
        last_status = status;
    }

    int event_count = 0;
    CefRuntimeStatus last_status{};
};

}  // namespace

int main() {
    CefRuntimeEntryConfig config;
    config.launch.initial_url = "about:blank";
    config.launch.use_osr = true;
    config.runner.use_osr = true;

    CefRuntimeHost host(config);
    auto public_bridge = host.bridge();
    if (!public_bridge) {
        std::cerr << "host.bridge() returned null\n";
        return 1;
    }
    if (host.bridge().get() != public_bridge.get()) {
        std::cerr << "host.bridge() not stable across repeated access\n";
        return 1;
    }

    auto bridge = std::dynamic_pointer_cast<bridge::cef::CefIntegrationBridge>(public_bridge);
    if (!bridge) {
        std::cerr << "host bridge is not a CefIntegrationBridge\n";
        return 1;
    }

    auto bridge_observer = std::make_shared<RecordingBridgeObserver>();
    auto runtime_observer = std::make_shared<RecordingRuntimeObserver>();
    host.set_bridge_observer(bridge_observer);
    host.set_runtime_observer(runtime_observer);

    bridge::cef::InitParams params;
    params.initial_width = 2;
    params.initial_height = 1;
    params.initial_url = "about:blank";
    std::string error;
    if (!public_bridge->initialize(params, &error)) {
        std::cerr << "bridge initialize failed: " << error << "\n";
        return 1;
    }

    const std::uint32_t src[] = {0xFF010203u, 0xFF112233u};
    bridge->backend()->observe_frame(src, 2, 1, static_cast<int>(2 * sizeof(std::uint32_t)));

    if (bridge_observer->event_count == 0) {
        std::cerr << "bridge observer saw no bridge snapshot changes\n";
        return 1;
    }
    if (!bridge_observer->last_snapshot.presentation.has_frame) {
        std::cerr << "bridge observer did not see presentation.has_frame\n";
        return 1;
    }

    if (runtime_observer->event_count == 0) {
        std::cerr << "runtime observer saw no runtime status changes\n";
        return 1;
    }
    if (runtime_observer->last_status.phase != CefRuntimePhase::first_frame_ready) {
        std::cerr << "runtime observer did not reach first_frame_ready\n";
        return 1;
    }
    if (!runtime_observer->last_status.saw_first_frame) {
        std::cerr << "runtime observer did not record saw_first_frame\n";
        return 1;
    }
    if (!runtime_observer->last_status.last_snapshot.presentation.has_frame) {
        std::cerr << "runtime status did not carry presentation.has_frame\n";
        return 1;
    }

    std::vector<std::uint32_t> copied(2, 0);
    if (!public_bridge->copy_latest_frame(copied.data(), 2, 1, static_cast<int>(2 * sizeof(std::uint32_t)), &error)) {
        std::cerr << "copy_latest_frame failed: " << error << "\n";
        return 1;
    }
    if (copied[0] != src[0] || copied[1] != src[1]) {
        std::cerr << "copied frame mismatch\n";
        return 1;
    }

    const auto status = host.runtime_status();
    if (status.phase != CefRuntimePhase::first_frame_ready || !status.saw_first_frame) {
        std::cerr << "host.runtime_status() did not preserve first-frame readiness\n";
        return 1;
    }

    public_bridge->shutdown();
    return 0;
}
