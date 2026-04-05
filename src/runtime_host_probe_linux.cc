#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "cef_runtime_entry.h"

namespace {

class FirstFrameObserver final : public ICefRuntimeObserver {
public:
    void on_runtime_status_changed(const CefRuntimeStatus& status) override {
        if (status.phase != CefRuntimePhase::first_frame_ready || !status.last_snapshot.presentation.has_frame) {
            return;
        }
        std::fprintf(stderr,
                     "engine-cef runtime-host probe observed first frame %dx%d generation=%llu\n",
                     status.last_snapshot.presentation.width,
                     status.last_snapshot.presentation.height,
                     static_cast<unsigned long long>(status.last_snapshot.presentation.frame_generation));
        CefRuntimeHost::RequestQuit();
    }
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
    host.set_runtime_observer(std::make_shared<FirstFrameObserver>());
    return host.Run(argc, argv);
}
