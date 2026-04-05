#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifndef ENGINE_CEF_RUNTIME_HOST_PROBE_PATH
#error "ENGINE_CEF_RUNTIME_HOST_PROBE_PATH must be defined"
#endif

namespace {

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

bool ContainsInOrder(const std::string& text,
                     const std::string& first,
                     const std::string& second,
                     const std::string& third) {
    const auto a = text.find(first);
    if (a == std::string::npos) {
        return false;
    }
    const auto b = text.find(second, a + first.size());
    if (b == std::string::npos) {
        return false;
    }
    const auto c = text.find(third, b + second.size());
    return c != std::string::npos;
}

}  // namespace

int main() {
    const std::filesystem::path probe = ENGINE_CEF_RUNTIME_HOST_PROBE_PATH;
    const auto temp_dir = std::filesystem::temp_directory_path();
    const auto stdout_path = temp_dir / "engine-cef-runtime-host-status-transition.stdout";
    const auto stderr_path = temp_dir / "engine-cef-runtime-host-status-transition.stderr";

    std::error_code ec;
    std::filesystem::remove(stdout_path, ec);
    std::filesystem::remove(stderr_path, ec);

    const std::string command = "timeout 20s \"" + probe.string() +
                                "\" --url='data:text/html,probe' >\"" + stdout_path.string() +
                                "\" 2>\"" + stderr_path.string() + "\"";
    const int code = std::system(command.c_str());
    const std::string stderr_text = ReadFile(stderr_path);

    if (code != 0) {
        std::cerr << "probe command failed with code " << code << "\n";
        std::cerr << stderr_text;
        return 1;
    }

    const std::string running = "engine-cef runtime-host probe status running";
    const std::string ready = "engine-cef runtime-host probe status first_frame_ready";
    const std::string stopped = "engine-cef runtime-host probe status stopped exit=0";
    if (!ContainsInOrder(stderr_text, running, ready, stopped)) {
        std::cerr << "missing ordered runtime status transitions\n";
        std::cerr << stderr_text;
        return 1;
    }
    if (stderr_text.find("engine-cef runtime-host probe observed first frame") == std::string::npos) {
        std::cerr << "missing first-frame observation log\n";
        std::cerr << stderr_text;
        return 1;
    }

    return 0;
}
