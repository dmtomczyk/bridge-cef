#include "cef_runtime_entry.h"

#include "include/cef_command_line.h"

NO_STACK_PROTECTOR
int main(int argc, char* argv[]) {
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    CefRuntimeEntryConfig config;
    config.launch.initial_url = command_line->GetSwitchValue("url");
    if (config.launch.initial_url.empty()) {
        config.launch.initial_url = "https://example.com";
    }
    config.launch.use_osr = command_line->HasSwitch("use-osr");
    config.launch.use_alloy_style = !command_line->HasSwitch("disable-alloy-style");
    config.launch.use_native = command_line->HasSwitch("use-native");
    config.launch.quit_after_first_frame = command_line->HasSwitch("quit-after-first-frame");
    config.launch.verify_presentation_v2 = command_line->HasSwitch("verify-presentation-v2");
    config.runner.use_osr = config.launch.use_osr;

    CefRuntimeHost host(config);
    return host.Run(argc, argv);
}
