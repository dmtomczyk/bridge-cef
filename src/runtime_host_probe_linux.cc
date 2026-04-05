#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "cef_runtime_entry.h"

namespace {

const char* PhaseName(CefRuntimePhase phase) {
    switch (phase) {
        case CefRuntimePhase::idle:
            return "idle";
        case CefRuntimePhase::running:
            return "running";
        case CefRuntimePhase::first_frame_ready:
            return "first_frame_ready";
        case CefRuntimePhase::stopped:
            return "stopped";
        case CefRuntimePhase::failed:
            return "failed";
    }
    return "unknown";
}

class FirstFrameObserver final : public ICefRuntimeObserver {
public:
    void on_runtime_status_changed(const CefRuntimeStatus& status) override {
        if (status.phase != last_phase_) {
            std::fprintf(stderr,
                         "engine-cef runtime-host probe status %s exit=%d saw_first_frame=%d\n",
                         PhaseName(status.phase),
                         status.last_exit_code,
                         status.saw_first_frame ? 1 : 0);
            last_phase_ = status.phase;
        }
        if (quit_requested_ || status.phase != CefRuntimePhase::first_frame_ready ||
            !status.last_snapshot.presentation.has_frame) {
            return;
        }
        quit_requested_ = true;
        std::fprintf(stderr,
                     "engine-cef runtime-host probe observed first frame %dx%d generation=%llu\n",
                     status.last_snapshot.presentation.width,
                     status.last_snapshot.presentation.height,
                     static_cast<unsigned long long>(status.last_snapshot.presentation.frame_generation));
        CefRuntimeHost::RequestQuit();
    }

private:
    CefRuntimePhase last_phase_ = CefRuntimePhase::idle;
    bool quit_requested_ = false;
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
