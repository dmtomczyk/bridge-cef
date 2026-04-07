#pragma once

#include <memory>
#include <string>

#include "include/cef_base.h"

class CefAppHost;

struct CefRuntimeRunnerOptions {
    bool use_osr = false;
    bool force_platform_osr_backend = true;
    std::string cache_root;
};

class ICefRuntimeRunner {
public:
    virtual ~ICefRuntimeRunner() = default;
    virtual int Run(int argc, char* argv[], CefRefPtr<CefAppHost> app, const CefRuntimeRunnerOptions& options) const = 0;
};

std::unique_ptr<ICefRuntimeRunner> CreatePlatformRuntimeRunner();
