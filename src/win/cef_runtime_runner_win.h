#pragma once

#include "../cef_runtime_runner.h"

class CefRuntimeRunnerWin : public ICefRuntimeRunner {
public:
    ~CefRuntimeRunnerWin() override = default;
    int Run(int argc, char* argv[], CefRefPtr<CefAppHost> app, const CefRuntimeRunnerOptions& options) const override;
};
