#pragma once

#include <memory>
#include <string>

#include "engine_cef/types.h"

namespace bridge::cef {

class IIntegrationBridgeObserver {
public:
    virtual ~IIntegrationBridgeObserver() = default;
    virtual void on_snapshot_changed(const BackendSnapshot& snapshot) = 0;
};

class IIntegrationBridge {
public:
    virtual ~IIntegrationBridge() = default;

    virtual bool initialize(const InitParams& params, std::string* error_out = nullptr) = 0;
    virtual void shutdown() = 0;

    virtual void navigate(const std::string& url) = 0;
    virtual void reload() = 0;
    virtual void go_back() = 0;
    virtual void go_forward() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void tick() = 0;

    virtual BackendSnapshot snapshot() const = 0;
    virtual std::string debug_summary() const = 0;

    virtual void set_observer(std::shared_ptr<IIntegrationBridgeObserver> observer) = 0;
};

std::shared_ptr<IIntegrationBridge> create_integration_bridge();

}  // namespace bridge::cef
