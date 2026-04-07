#include "cef_runtime_entry.h"

#include <charconv>
#include <string>
#include <string_view>
#include <vector>

#include "include/cef_command_line.h"

namespace {

std::vector<std::string> parse_dev_open_tab_urls(int argc, char* argv[]) {
    constexpr std::string_view prefix = "--dev-open-tab=";
    std::vector<std::string> urls;
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }
        const std::string_view arg(argv[i]);
        if (arg.rfind(prefix, 0) == 0) {
            urls.emplace_back(arg.substr(prefix.size()));
        }
    }
    return urls;
}

std::vector<std::string> parse_dev_popup_tab_urls(int argc, char* argv[]) {
    constexpr std::string_view prefix = "--dev-popup-tab=";
    std::vector<std::string> urls;
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }
        const std::string_view arg(argv[i]);
        if (arg.rfind(prefix, 0) == 0) {
            urls.emplace_back(arg.substr(prefix.size()));
        }
    }
    return urls;
}

int parse_dev_active_tab_index(int argc, char* argv[]) {
    constexpr std::string_view prefix = "--dev-active-tab=";
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }
        const std::string_view arg(argv[i]);
        if (arg.rfind(prefix, 0) != 0) {
            continue;
        }
        const std::string_view value_text = arg.substr(prefix.size());
        int value = 0;
        const auto result = std::from_chars(value_text.data(), value_text.data() + value_text.size(), value);
        if (result.ec != std::errc{} || result.ptr != value_text.data() + value_text.size()) {
            return 0;
        }
        return value > 0 ? value : 0;
    }
    return 0;
}

}  // namespace

NO_STACK_PROTECTOR
int main(int argc, char* argv[]) {
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    CefRuntimeEntryConfig config;
    config.launch.initial_url = command_line->GetSwitchValue("url");
    if (config.launch.initial_url.empty()) {
        config.launch.initial_url.clear();
    }
    config.launch.dev_open_tab_urls = parse_dev_open_tab_urls(argc, argv);
    config.launch.dev_popup_tab_urls = parse_dev_popup_tab_urls(argc, argv);
    config.launch.dev_active_tab_index = parse_dev_active_tab_index(argc, argv);
    config.launch.use_osr = command_line->HasSwitch("use-osr");
    config.launch.use_alloy_style = !command_line->HasSwitch("disable-alloy-style");
    config.launch.use_native = command_line->HasSwitch("use-native");
    config.launch.quit_after_first_frame = command_line->HasSwitch("quit-after-first-frame");
    config.launch.verify_presentation_v2 = command_line->HasSwitch("verify-presentation-v2");
    config.runner.use_osr = config.launch.use_osr;

    CefRuntimeHost host(config);
    return host.Run(argc, argv);
}
