#pragma once

#include "cef_app_host.h"
#include "cef_linux_main_runner.h"

struct CefRuntimeEntryConfig {
    CefLaunchConfig launch;
    CefLinuxMainRunnerOptions runner;
};

int RunCefRuntimeEntry(int argc, char* argv[], const CefRuntimeEntryConfig& config);
