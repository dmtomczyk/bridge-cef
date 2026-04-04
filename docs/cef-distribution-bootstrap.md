# CEF binary distribution bootstrap flow

## Decision

For the first embedder spike, use the **official-style CEF binary distribution workflow** rather than building Chromium/CEF from source.

## Source of truth

The spike is intentionally modeled on the standard CEF binary-distribution workflow documented by:

- CEF general usage docs (`Using a Binary Distribution`)
- `cef-project` sample project README

Those docs describe:

- downloading a binary distribution for the target platform
- building a host app with CMake
- linking against the files included in the binary distribution

## Platform target

- Linux x64
- windowed rendering first
- Alloy-style runtime preferred once the browser window is created

## Acquisition strategy

### Preferred path

Use a released Linux x64 CEF binary distribution from the official automated builds index:

- <https://cef-builds.spotifycdn.com/index.html>

### Local layout convention

For the spike, expect the extracted distribution to live outside the build tree, for example:

- `~/code/third_party/cef/cef_binary_<version>_linux64/`

Then configure the spike with:

```bash
cmake -S spikes/cef-embedder -B spikes/cef-embedder/build \
  -DCEF_EMBEDDER_SPIKE_ENABLE_CEF=ON \
  -DCEF_ROOT=$HOME/code/third_party/cef/cef_binary_<version>_linux64
```

## Version policy for the spike

Use a **released, supported Linux x64 binary distribution** that is recent enough to validate the integration path.

For the spike we care more about:

- fast bring-up
- health of the iteration loop
- viability of the embedder model

than about matching the current reference backend's pinned `headless_shell` revision exactly.

### Why this is acceptable

The spike is validating the *integration model*, not trying to preserve a 1:1 Chromium revision match with the MVP reference backend.

If the embedder route proves viable, we can decide later whether version alignment matters for migration.

## Build expectations

The intended flow is:

1. acquire/extract CEF binary distribution
2. point `CEF_ROOT` at it
3. use CMake/sample-project style integration
4. build a tiny host app
5. open a window and load `https://example.com`

## Notes

- Do **not** start by cloning/building all of CEF or Chromium source for this spike.
- Do **not** patch Chromium binaries for this spike.
- If the binary distribution route is unworkable on this machine, record exactly why before changing direction.
