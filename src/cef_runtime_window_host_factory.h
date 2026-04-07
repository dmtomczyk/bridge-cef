#pragma once

#include <memory>

#include "cef_runtime_host.h"

std::unique_ptr<CefRuntimeWindowHost> CreatePlatformRuntimeWindowHost();
