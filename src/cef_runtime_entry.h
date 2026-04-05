#pragma once

#include "cef_app_host.h"
#include "cef_linux_main_runner.h"

struct CefRuntimeEntryConfig {
    CefLaunchConfig launch;
    CefLinuxMainRunnerOptions runner;
};

class CefRuntimeHost {
public:
    CefRuntimeHost() = default;
    explicit CefRuntimeHost(CefRuntimeEntryConfig config) : config_(std::move(config)) {}

    void set_config(const CefRuntimeEntryConfig& config) { config_ = config; }
    const CefRuntimeEntryConfig& config() const { return config_; }

    int Run(int argc, char* argv[]) const;
    static void RequestQuit();

private:
    CefRuntimeEntryConfig config_{};
};

int RunCefRuntimeEntry(int argc, char* argv[], const CefRuntimeEntryConfig& config);
