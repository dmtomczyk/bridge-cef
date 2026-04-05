# engine-cef build / quickstart

This is the small practical guide for working with `engine-cef` in the split Bridge workspace.

## What this repo is

`engine-cef` is the active long-term Chromium backend target for Bridge.

It owns:

- the public CEF integration bridge contract
- Linux runtime-host / proof / probe bring-up
- the presentation-v2 frame contract
- the current runtime-boundary work for future deeper shell/backend integration

## External dependency requirement

This repo depends on an external CEF binary distribution via `CEF_ROOT`.

Typical pattern:

```bash
export CEF_ROOT=~/code/external_deps/sdks/cef/cef_binary_<version>_linux64
```

Do **not** check the CEF SDK/runtime into this repo.

## Scaffold build (no real CEF)

```bash
cmake -S . -B build -DENGINE_CEF_ENABLE_CEF=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure -R '^engine_cef_bridge_frame_contract_test$'
```

## Real CEF build

```bash
cmake -S . -B build-cef \
  -DENGINE_CEF_ENABLE_CEF=ON \
  -DCEF_ROOT="$CEF_ROOT"

cmake --build build-cef -j
```

## Useful targets

### Proof app
```bash
cmake --build build-cef -j --target engine_cef_proof
```

### Runtime-host probe
```bash
cmake --build build-cef -j --target engine_cef_runtime_host_probe
```

### Focused tests
```bash
cmake --build build-cef -j --target \
  engine_cef_runtime_host_bridge_observer_test \
  engine_cef_runtime_host_status_transition_test

ctest --test-dir build-cef --output-on-failure \
  -R 'engine_cef_bridge_frame_contract_test|engine_cef_runtime_host_bridge_observer_test|engine_cef_runtime_host_status_transition_test'
```

## Useful runtime commands

### Verify proof-side bridge frame copy
```bash
./build-cef/Release/engine_cef_proof \
  --use-osr \
  --verify-presentation-v2 \
  --quit-after-first-frame \
  --url='data:text/html,<html><body>hi</body></html>'
```

### Run the non-proof runtime-host probe
```bash
./build-cef/Release/engine_cef_runtime_host_probe \
  --url='data:text/html,<html><body>probe</body></html>'
```

## Notes

- `build/` is the scaffold/no-CEF build dir.
- `build-cef/` is the real-CEF local build dir.
- `engine-cef` is the active long-term Chromium lane; `engine-chromium` remains the runnable reference/demo lane.
- For the client-side real-CEF smoke/probe path, see the workspace root helper `../scripts/cef-runtime-smoke.sh`.
