cmake_minimum_required(VERSION 3.21)
project(engine_cef LANGUAGES C CXX)

set(BRIDGE_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../core" CACHE PATH "Sibling path to bridge-core repository")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENGINE_CEF_ENABLE_CEF "Build against a local CEF binary distribution" OFF)

enable_testing()

add_library(engine_cef_core STATIC
  src/cef_backend.cc
  src/cef_backend.h
  src/cef_backend_types.h
  src/cef_integration_bridge.cc
  src/cef_integration_bridge.h
)
target_compile_features(engine_cef_core PUBLIC cxx_std_20)
target_include_directories(engine_cef_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${BRIDGE_CORE_DIR}/include
)

add_executable(engine_cef_bridge_frame_contract_test tests/bridge_frame_contract_test.cpp)
target_compile_features(engine_cef_bridge_frame_contract_test PRIVATE cxx_std_20)
target_link_libraries(engine_cef_bridge_frame_contract_test PRIVATE engine_cef_core)
add_test(NAME engine_cef_bridge_frame_contract_test COMMAND engine_cef_bridge_frame_contract_test)

if(ENGINE_CEF_ENABLE_CEF)
  if(NOT DEFINED CEF_ROOT AND DEFINED ENV{CEF_ROOT})
    set(CEF_ROOT "$ENV{CEF_ROOT}")
  endif()

  if(NOT DEFINED CEF_ROOT)
    message(FATAL_ERROR "ENGINE_CEF_ENABLE_CEF=ON requires -DCEF_ROOT=/path/to/cef_binary_... or CEF_ROOT in the environment")
  endif()

  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
  find_package(CEF REQUIRED)

  set(ENGINE_CEF_RUNTIME_TARGET_PLATFORM "" CACHE STRING "Runtime-host platform target (auto|linux|windows)")
  set_property(CACHE ENGINE_CEF_RUNTIME_TARGET_PLATFORM PROPERTY STRINGS auto linux windows)
  if(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "" OR ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "auto")
    if(OS_LINUX)
      set(ENGINE_CEF_RUNTIME_TARGET_PLATFORM "linux")
    elseif(WIN32)
      set(ENGINE_CEF_RUNTIME_TARGET_PLATFORM "windows")
    else()
      message(FATAL_ERROR "engine-cef runtime target platform auto-detection failed for this host")
    endif()
  endif()

  if(NOT ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "linux" AND NOT ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "windows")
    message(FATAL_ERROR "ENGINE_CEF_RUNTIME_TARGET_PLATFORM must be one of: auto, linux, windows")
  endif()

  if(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "linux" AND NOT OS_LINUX)
    message(FATAL_ERROR "engine-cef runtime target platform 'linux' requires a Linux host/bootstrap environment")
  endif()
  if(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "windows" AND NOT WIN32 AND NOT OS_LINUX)
    message(FATAL_ERROR "engine-cef runtime target platform 'windows' currently supports native Windows builds or Linux-side source validation only")
  endif()

  add_logical_target("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
  set_cef_target_out_dir()
  add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

  set(ENGINE_CEF_RUNTIME_PLATFORM_SOURCES
    src/cef_runtime_window_host_factory.cc
    src/cef_runtime_window_host_factory.h
    src/cef_runtime_runner.h
    src/cef_runtime_runner_factory.cc
  )
  set(ENGINE_CEF_RUNTIME_PLATFORM_LINK_LIBS)

  if(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
    list(APPEND ENGINE_CEF_RUNTIME_PLATFORM_SOURCES
      src/cef_osr_host_gtk.cc
      src/cef_osr_host_gtk.h
      src/cef_linux_main_runner.cc
      src/cef_linux_main_runner.h
    )
    list(APPEND ENGINE_CEF_RUNTIME_PLATFORM_LINK_LIBS PkgConfig::GTK3)
  elseif(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "windows")
    list(APPEND ENGINE_CEF_RUNTIME_PLATFORM_SOURCES
      src/win/cef_runtime_window_host_win.cc
      src/win/cef_runtime_window_host_win.h
      src/win/cef_runtime_runner_win.cc
      src/win/cef_runtime_runner_win.h
    )
    message(STATUS "engine-cef runtime target platform 'windows' uses scaffold-only host/runner sources in this workspace")
  endif()

  add_library(engine_cef_runtime_host STATIC
    src/cef_app_host.cc
    src/cef_app_host.h
    src/cef_browser_handler.cc
    src/cef_browser_handler.h
    src/cef_runtime_entry.cc
    src/cef_runtime_entry.h
    ${ENGINE_CEF_RUNTIME_PLATFORM_SOURCES}
  )
  target_compile_features(engine_cef_runtime_host PUBLIC cxx_std_20)
  target_link_libraries(engine_cef_runtime_host PUBLIC engine_cef_core libcef_lib libcef_dll_wrapper ${ENGINE_CEF_RUNTIME_PLATFORM_LINK_LIBS} ${CEF_STANDARD_LIBS})
  target_include_directories(engine_cef_runtime_host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CEF_INCLUDE_PATH}
  )
  target_compile_definitions(engine_cef_runtime_host PUBLIC
    ENGINE_CEF_BRIDGE_ICON_PNG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../browser/assets/bridge.png"
  )
  if(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "linux")
    target_compile_definitions(engine_cef_runtime_host PUBLIC ENGINE_CEF_RUNTIME_TARGET_PLATFORM_LINUX=1)
  elseif(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "windows")
    target_compile_definitions(engine_cef_runtime_host PUBLIC ENGINE_CEF_RUNTIME_TARGET_PLATFORM_WINDOWS=1 NOMINMAX WIN32_LEAN_AND_MEAN)
  endif()

  add_executable(engine_cef_runtime_host_bridge_observer_test tests/runtime_host_bridge_observer_test.cpp)
  target_compile_features(engine_cef_runtime_host_bridge_observer_test PRIVATE cxx_std_20)
  target_link_libraries(engine_cef_runtime_host_bridge_observer_test PRIVATE engine_cef_runtime_host)
  add_test(NAME engine_cef_runtime_host_bridge_observer_test COMMAND engine_cef_runtime_host_bridge_observer_test)

  if(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "linux")
    add_executable(engine_cef_proof
      src/main_linux.cc
    )
    set_executable_target_properties(engine_cef_proof)
    add_dependencies(engine_cef_proof libcef_dll_wrapper)
    target_link_libraries(engine_cef_proof engine_cef_runtime_host)
    set_target_properties(engine_cef_proof PROPERTIES
      INSTALL_RPATH "$ORIGIN"
      BUILD_WITH_INSTALL_RPATH TRUE
      RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    )

    add_executable(engine_cef_runtime_host_probe
      src/runtime_host_probe_linux.cc
    )
    set_executable_target_properties(engine_cef_runtime_host_probe)
    add_dependencies(engine_cef_runtime_host_probe libcef_dll_wrapper)
    target_link_libraries(engine_cef_runtime_host_probe engine_cef_runtime_host)
    set_target_properties(engine_cef_runtime_host_probe PROPERTIES
      INSTALL_RPATH "$ORIGIN"
      BUILD_WITH_INSTALL_RPATH TRUE
      RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    )
  endif()

  if(TARGET engine_cef_runtime_host_probe)
    add_executable(engine_cef_runtime_host_status_transition_test tests/runtime_host_status_transition_test.cpp)
    target_compile_features(engine_cef_runtime_host_status_transition_test PRIVATE cxx_std_20)
    target_compile_definitions(engine_cef_runtime_host_status_transition_test PRIVATE ENGINE_CEF_RUNTIME_HOST_PROBE_PATH="$<TARGET_FILE:engine_cef_runtime_host_probe>")
    add_dependencies(engine_cef_runtime_host_status_transition_test engine_cef_runtime_host_probe)
    add_test(NAME engine_cef_runtime_host_status_transition_test COMMAND engine_cef_runtime_host_status_transition_test)
  endif()

  if(TARGET engine_cef_proof)
    copy_files("engine_cef_proof" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
    copy_files("engine_cef_proof" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
  endif()
  if(TARGET engine_cef_runtime_host_probe)
    copy_files("engine_cef_runtime_host_probe" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
    copy_files("engine_cef_runtime_host_probe" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
  endif()
  if(ENGINE_CEF_RUNTIME_TARGET_PLATFORM STREQUAL "linux")
    if(EXISTS "${CEF_BINARY_DIR}/libminigbm.so")
      if(TARGET engine_cef_proof)
        copy_files("engine_cef_proof" "libminigbm.so" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
      endif()
      if(TARGET engine_cef_runtime_host_probe)
        copy_files("engine_cef_runtime_host_probe" "libminigbm.so" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
      endif()
    endif()
    if(TARGET engine_cef_proof)
      set_linux_suid_permissions("engine_cef_proof" "${CEF_TARGET_OUT_DIR}/chrome-sandbox")
    endif()
    if(TARGET engine_cef_runtime_host_probe)
      set_linux_suid_permissions("engine_cef_runtime_host_probe" "${CEF_TARGET_OUT_DIR}/chrome-sandbox")
    endif()
  endif()
else()
  message(STATUS "engine-cef scaffold only. Reconfigure with -DENGINE_CEF_ENABLE_CEF=ON once a CEF distribution is available.")
  add_executable(engine_cef_proof src/main_stub.cpp)
  target_compile_features(engine_cef_proof PRIVATE cxx_std_20)
  target_link_libraries(engine_cef_proof engine_cef_core)
endif()
