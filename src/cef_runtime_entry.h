#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "cef_app_host.h"
#include "cef_linux_main_runner.h"
#include "engine_cef/integration_bridge.h"

struct CefRuntimeEntryConfig {
    CefLaunchConfig launch;
    CefLinuxMainRunnerOptions runner;
};

enum class CefRuntimePhase {
    idle,
    running,
    first_frame_ready,
    stopped,
    failed,
};

struct CefRuntimeStatus {
    CefRuntimePhase phase = CefRuntimePhase::idle;
    bool saw_first_frame = false;
    int last_exit_code = 0;
    bridge::cef::BackendSnapshot last_snapshot{};
    std::string last_error;
};

class ICefRuntimeObserver {
public:
    virtual ~ICefRuntimeObserver() = default;
    virtual void on_runtime_status_changed(const CefRuntimeStatus& status) = 0;
};

class CefRuntimeHost {
public:
    struct RuntimeState;

    CefRuntimeHost() = default;
    explicit CefRuntimeHost(CefRuntimeEntryConfig config) : config_(std::move(config)) {}

    void set_config(const CefRuntimeEntryConfig& config) { config_ = config; }
    const CefRuntimeEntryConfig& config() const { return config_; }

    std::shared_ptr<bridge::cef::IIntegrationBridge> bridge() const;
    void set_bridge_observer(std::shared_ptr<bridge::cef::IIntegrationBridgeObserver> observer) const;
    void set_runtime_observer(std::shared_ptr<ICefRuntimeObserver> observer) const;
    CefRuntimeStatus runtime_status() const;
    int Run(int argc, char* argv[]) const;
    static void RequestQuit();

private:
    void EnsureApp() const;

    std::shared_ptr<RuntimeState> EnsureState() const;

    CefRuntimeEntryConfig config_{};
    mutable CefRefPtr<CefAppHost> app_{};
    mutable std::shared_ptr<RuntimeState> state_{};
};

int RunCefRuntimeEntry(int argc, char* argv[], const CefRuntimeEntryConfig& config);
