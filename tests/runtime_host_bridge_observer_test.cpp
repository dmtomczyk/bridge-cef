#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "cef_integration_bridge.h"
#include "cef_runtime_entry.h"

namespace {

class RecordingObserver final : public bridge::cef::IIntegrationBridgeObserver {
public:
    void on_snapshot_changed(const bridge::cef::BackendSnapshot& snapshot) override {
        ++event_count;
        last_snapshot = snapshot;
    }

    int event_count = 0;
    bridge::cef::BackendSnapshot last_snapshot{};
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

    auto observer = std::make_shared<RecordingObserver>();
    public_bridge->set_observer(observer);

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

    if (observer->event_count == 0) {
        std::cerr << "observer saw no bridge snapshot changes\n";
        return 1;
    }
    if (!observer->last_snapshot.presentation.has_frame) {
        std::cerr << "observer did not see presentation.has_frame\n";
        return 1;
    }
    if (observer->last_snapshot.presentation.width != 2 || observer->last_snapshot.presentation.height != 1) {
        std::cerr << "observer presentation size mismatch\n";
        return 1;
    }
    if (observer->last_snapshot.presentation.frame_generation == 0) {
        std::cerr << "observer presentation generation not advanced\n";
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

    public_bridge->shutdown();
    return 0;
}
