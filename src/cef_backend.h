#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "cef_backend_types.h"

namespace bridge::cef {

class IBackendObserver {
public:
    virtual ~IBackendObserver() = default;
    virtual void on_page_state_changed(const PageState& state) = 0;
    virtual void on_load_state_changed(const LoadState& state) = 0;
};

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
    virtual BackendSnapshot snapshot() const = 0;
    virtual PresentationState presentation_state() const = 0;
    virtual bool copy_latest_frame(std::uint32_t* dst_argb,
                                   int width,
                                   int height,
                                   int stride_bytes,
                                   std::string* error_out = nullptr) const = 0;
    virtual std::string debug_summary() const = 0;

    virtual void set_observer(std::shared_ptr<IBackendObserver> observer) = 0;
};

class CefBackend final : public IBackend {
public:
    using Ptr = std::shared_ptr<CefBackend>;

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
    BackendSnapshot snapshot() const override;
    PresentationState presentation_state() const override;
    bool copy_latest_frame(std::uint32_t* dst_argb,
                           int width,
                           int height,
                           int stride_bytes,
                           std::string* error_out = nullptr) const override;
    std::string debug_summary() const override;

    void set_observer(std::shared_ptr<IBackendObserver> observer) override;

    // Browser-observed state hooks used by the current standalone proof and the
    // next shell-integration slices.
    void observe_address_change(const std::string& url);
    void observe_title_change(const std::string& title);
    void observe_loading_state(bool loading, bool can_go_back, bool can_go_forward);
    void observe_load_end();
    void observe_load_error(const std::string& error);

private:
    InitParams init_params_{};
    PageState page_state_{};
    LoadState load_state_{};
    PresentationState presentation_state_{};
    std::vector<std::uint32_t> latest_frame_argb_{};
    bool initialized_ = false;
    int width_ = 0;
    int height_ = 0;
    mutable std::mutex mutex_{};
    std::shared_ptr<IBackendObserver> observer_{};
};

}  // namespace bridge::cef
