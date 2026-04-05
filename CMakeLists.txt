cmake_minimum_required(VERSION 3.21)
project(engine_cef LANGUAGES C CXX)

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

  if(NOT OS_LINUX)
    message(FATAL_ERROR "engine-cef bootstrap currently targets Linux only")
  endif()

  add_logical_target("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
  set_cef_target_out_dir()
  add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

  add_library(engine_cef_runtime_host STATIC
    src/cef_app_host.cc
    src/cef_app_host.h
    src/cef_browser_handler.cc
    src/cef_browser_handler.h
    src/cef_osr_host_gtk.cc
    src/cef_osr_host_gtk.h
    src/cef_linux_main_runner.cc
    src/cef_linux_main_runner.h
    src/cef_runtime_entry.cc
    src/cef_runtime_entry.h
  )
  target_compile_features(engine_cef_runtime_host PUBLIC cxx_std_20)
  target_link_libraries(engine_cef_runtime_host PUBLIC engine_cef_core libcef_lib libcef_dll_wrapper PkgConfig::GTK3 ${CEF_STANDARD_LIBS})
  target_include_directories(engine_cef_runtime_host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

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

  copy_files("engine_cef_proof" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
  copy_files("engine_cef_proof" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
  if(EXISTS "${CEF_BINARY_DIR}/libminigbm.so")
    copy_files("engine_cef_proof" "libminigbm.so" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
  endif()
  set_linux_suid_permissions("engine_cef_proof" "${CEF_TARGET_OUT_DIR}/chrome-sandbox")
else()
  message(STATUS "engine-cef scaffold only. Reconfigure with -DENGINE_CEF_ENABLE_CEF=ON once a CEF distribution is available.")
  add_executable(engine_cef_proof src/main_stub.cpp)
  target_compile_features(engine_cef_proof PRIVATE cxx_std_20)
  target_link_libraries(engine_cef_proof engine_cef_core)
endif()
