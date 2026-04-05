#pragma once

#include <cstdint>
#include <string>

namespace bridge::cef {

struct InitParams {
    int initial_width = 1280;
    int initial_height = 800;
    std::string initial_url = "https://example.com";
    bool use_alloy_style = true;
    bool use_views = true;
};

struct PageState {
    std::string current_url;
    std::string title;
    bool can_go_back = false;
    bool can_go_forward = false;
};

struct LoadState {
    bool loading = false;
    bool first_page_shown = false;
    std::string last_error;
};

struct PresentationState {
    bool has_frame = false;
    int width = 0;
    int height = 0;
    int stride_bytes = 0;
    std::uint64_t frame_generation = 0;
};

struct BackendSnapshot {
    PageState page;
    LoadState load;
    PresentationState presentation;
};

}  // namespace bridge::cef
