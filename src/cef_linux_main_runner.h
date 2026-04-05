#pragma once

#include <string>

#include "include/cef_base.h"

class CefAppHost;

struct CefLinuxMainRunnerOptions {
    bool use_osr = false;
    bool force_x11_backend_for_osr = true;
    std::string cache_root;
};

class CefLinuxMainRunner {
public:
    static int Run(int argc, char* argv[], CefRefPtr<CefAppHost> app, const CefLinuxMainRunnerOptions& options = {});
};
