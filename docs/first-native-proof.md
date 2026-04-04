# First native CEF proof

## Result

The CEF embedder spike successfully:

- configured against a Linux x64 CEF binary distribution
- built `libcef_dll_wrapper`
- built `cef_embedder_spike`
- launched a native browser window
- loaded `https://example.com`
- allowed manual clicking/interacting

This is the first successful native embedded-browser proof for the long-term Chromium pivot.

## Local dependency layout used for the proof

Example local SDK path:

- `~/code/external_deps/sdks/cef/cef_binary_146.0.10+g8219561+chromium-146.0.7680.179_linux64/`

The exact location is not important as long as `CEF_ROOT` points at the extracted CEF binary distribution root.

## Configure/build commands

```bash
export CEF_ROOT=~/code/external_deps/sdks/cef/cef_binary_146.0.10+g8219561+chromium-146.0.7680.179_linux64

cmake -S spikes/cef-embedder -B spikes/cef-embedder/build \
  -DCEF_EMBEDDER_SPIKE_ENABLE_CEF=ON \
  -DCEF_ROOT="$CEF_ROOT" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build spikes/cef-embedder/build -j2
```

## Linux sandbox note

After building, CEF required the usual Linux `chrome-sandbox` permissions:

```bash
EXE="/home/snakedoctor/code/foss/bridge/spikes/cef-embedder/build/Release/chrome-sandbox" && \
  sudo -- chown root:root "$EXE" && \
  sudo -- chmod 4755 "$EXE"
```

## Run command

```bash
./spikes/cef-embedder/build/Release/cef_embedder_spike --url=https://example.com
```

## Known build fixes discovered during first proof

Two project-side fixes were required during bring-up:

1. enable both `C` and `CXX` in the spike CMake project so `FindCEF.cmake` can run C compiler flag checks
2. include `include/base/cef_callback.h` in `src/spike_handler.cc` so `base::BindOnce(...)` / `CefPostTask(...)` compile cleanly

## Why this matters

This proof demonstrates a much healthier path than the current screenshot/DevTools backend for the long-term direction:

- no Chromium source rebuild loop
- no PNG frame transport in the hot path
- real native embedded browser window
- much faster iteration than the Chromium binary-patching experiments
