#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "cef_backend.h"
#include "cef_integration_bridge.h"

int main() {
    auto backend = std::make_shared<bridge::cef::CefBackend>();
    auto bridge = std::make_shared<bridge::cef::CefIntegrationBridge>(backend);

    bridge::cef::InitParams params;
    params.initial_width = 4;
    params.initial_height = 2;
    params.initial_url = "about:blank";
    std::string error;
    if (!bridge->initialize(params, &error)) {
        std::cerr << "bridge initialize failed: " << error << "\n";
        return 1;
    }

    const std::uint32_t src[] = {
        0xFF0000FFu, 0xFF00FF00u, 0xFFFF0000u, 0xFFFFFFFFu,
        0xFF112233u, 0xFF445566u, 0xFF778899u, 0xFFAABBCCu,
    };
    backend->observe_frame(src, 4, 2, static_cast<int>(4 * sizeof(std::uint32_t)));

    const auto snapshot = bridge->snapshot();
    if (!snapshot.presentation.has_frame) {
        std::cerr << "presentation.has_frame false\n";
        return 1;
    }
    if (snapshot.presentation.width != 4 || snapshot.presentation.height != 2) {
        std::cerr << "presentation size mismatch\n";
        return 1;
    }
    if (snapshot.presentation.stride_bytes != static_cast<int>(4 * sizeof(std::uint32_t))) {
        std::cerr << "presentation stride mismatch\n";
        return 1;
    }
    if (snapshot.presentation.frame_generation == 0) {
        std::cerr << "presentation generation not advanced\n";
        return 1;
    }

    std::vector<std::uint32_t> copied(8, 0);
    if (!bridge->copy_latest_frame(copied.data(), 4, 2, static_cast<int>(4 * sizeof(std::uint32_t)), &error)) {
        std::cerr << "copy_latest_frame failed: " << error << "\n";
        return 1;
    }
    for (std::size_t i = 0; i < copied.size(); ++i) {
        if (copied[i] != src[i]) {
            std::cerr << "copied pixel mismatch at " << i << "\n";
            return 1;
        }
    }

    bridge->shutdown();
    return 0;
}
