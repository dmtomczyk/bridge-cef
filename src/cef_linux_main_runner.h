#pragma once

#include "include/cef_base.h"

class CefAppHost;

class CefLinuxMainRunner {
public:
    static int Run(int argc, char* argv[], CefRefPtr<CefAppHost> app);
};
