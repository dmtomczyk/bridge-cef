# Linux OSR sample-parity plan

## Goal

Make the `engine-cef` Linux OSR proof materially closer to upstream `cefclient` before drawing stronger conclusions from OSR crashes.

## Why

We have already confirmed two things:

- our current Linux OSR proof crashes before first observed `OnPaint`
- upstream `cefclient` OSR also crashes in this environment

That does **not** yet prove our proof is sufficiently faithful to the upstream Linux OSR host path.

The biggest known difference today is that upstream Linux OSR is hosted through a real GTK/X11 window/container path, while our proof still uses a much thinner hand-rolled windowless host.

## Short implementation plan

1. Add a proof-only Linux OSR host layer (`cef_osr_host_gtk.*`)
   - own GTK/X11 host window/widget lifecycle
   - expose native parent handle for `SetAsWindowless(...)`
   - expose view/screen geometry helpers for `CefRenderHandler`

2. Move proof browser creation out of `OnContextInitialized`
   - initialize CEF first
   - initialize GTK/X11 similarly to upstream
   - create the initial browser after GTK/X11 host setup exists

3. Route OSR geometry/screen behavior through the host
   - `GetRootScreenRect`
   - `GetViewRect`
   - `GetScreenPoint`
   - `GetScreenInfo`

4. Keep the current backend/frame bridge intact
   - continue using `OnPaint -> CefBackend::observe_frame(...)`
   - do not change the public presentation v2 contract in this slice

5. Re-test Linux OSR proof after sample-parity improvements
   - if it still crashes, confidence increases that the remaining problem is upstream/environmental rather than proof scaffolding

## Non-goals for this slice

- do not redesign the cross-repo client/engine contract
- do not tackle runtime/input migration
- do not add hacky client-side workarounds

## Current status

This plan has now produced a real proof milestone.

With the current sample-parity slice in place, the `engine-cef` proof can now reach first observed `OnPaint` and exit cleanly when all of the following are true:

- a GTK/X11-backed OSR host is used instead of the earlier hand-rolled hidden X11 window path
- OSR browser creation happens after GTK/X11 host setup exists
- GTK is forced onto the X11 backend for OSR (`GDK_BACKEND=x11` / `gdk_set_allowed_backends("x11")`)
- the proof uses `CefQuitMessageLoop()` for the first-frame exit helper instead of closing browsers inside that probe path

This does **not** mean Linux OSR is fully solved in general. Upstream `cefclient` OSR still crashes in this environment, so the current result should be read as:

- our proof path now has a viable first-frame recipe
- that proof path can also verify successful presentation-v2 frame copy on first paint
- the working Linux OSR bootstrap has now been extracted into a reusable `engine_cef_runtime_host` layer instead of living only in the proof `main()`
- the reusable layer now accepts programmatic launch/runtime configuration instead of depending entirely on proof-only global CLI parsing
- upstream/Linux OSR remains fragile in this environment family
- the sample-parity work was still worth doing because it separated proof-host issues from broader upstream runtime issues

## Success condition

A good outcome for this slice is either:

- the OSR proof reaches first `OnPaint`, or
- we become much more confident that remaining OSR failure is upstream/environmental because the proof now more closely matches upstream Linux OSR hosting.
