#pragma once

#include <string>

#include "cef_backend_types.h"

namespace bridge::cef {

class IBackend {
public:
    virtual ~IBackend() = default;

    virtual bool initialize(const InitParams& params, std::string* error_out = nullptr) = 0;
    virtual void shutdown() = 0;

    virtual void navigate(const std::string& url) = 0;
    virtual void reload() = 0;
    virtual void go_back() = 0;
    virtual void go_forward() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void tick() = 0;

    virtual PageState page_state() const = 0;
    virtual LoadState load_state() const = 0;
    virtual std::string debug_summary() const = 0;
};

class CefBackend final : public IBackend {
public:
    CefBackend() = default;

    bool initialize(const InitParams& params, std::string* error_out = nullptr) override;
    void shutdown() override;

    void navigate(const std::string& url) override;
    void reload() override;
    void go_back() override;
    void go_forward() override;
    void resize(int width, int height) override;
    void tick() override;

    PageState page_state() const override;
    LoadState load_state() const override;
    std::string debug_summary() const override;

private:
    InitParams init_params_{};
    PageState page_state_{};
    LoadState load_state_{};
    bool initialized_ = false;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace bridge::cef
