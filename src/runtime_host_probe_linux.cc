#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "cef_runtime_entry.h"
#include "engine_cef/integration_bridge.h"

namespace {

class FirstFrameObserver final : public bridge::cef::IIntegrationBridgeObserver {
public:
    void on_snapshot_changed(const bridge::cef::BackendSnapshot& snapshot) override {
        if (saw_first_frame_ || !snapshot.presentation.has_frame) {
            return;
        }
        saw_first_frame_ = true;
        std::fprintf(stderr,
                     "engine-cef runtime-host probe observed first frame %dx%d generation=%llu\n",
                     snapshot.presentation.width,
                     snapshot.presentation.height,
                     static_cast<unsigned long long>(snapshot.presentation.frame_generation));
        CefRuntimeHost::RequestQuit();
    }

private:
    bool saw_first_frame_ = false;
};

std::string ParseUrl(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        constexpr std::string_view prefix = "--url=";
        if (arg.rfind(prefix, 0) == 0) {
            return std::string(arg.substr(prefix.size()));
        }
    }
    return "data:text/html,<html><body>runtime host probe</body></html>";
}

}  // namespace

NO_STACK_PROTECTOR
int main(int argc, char* argv[]) {
    CefRuntimeEntryConfig config;
    config.launch.initial_url = ParseUrl(argc, argv);
    config.launch.use_osr = true;
    config.runner.use_osr = true;

    CefRuntimeHost host(config);
    auto bridge = host.bridge();
    bridge->set_observer(std::make_shared<FirstFrameObserver>());
    return host.Run(argc, argv);
}
