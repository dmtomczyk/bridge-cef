# Presentation seam v2 (`client` <-> `engine-cef`)

## Purpose

Define the next contract step after the current v1 control/state bridge.

v1 proved that `client` can:

- construct `engine-cef`
- drive basic navigation/lifecycle
- observe page/load state

It did **not** define the long-term presentation surface.

This note answers that question.

## Recommendation

For `renderer=cef`, the next real seam should make **`engine-cef` the source of truth for content frames**.

Do **not** keep deepening the public contract around custom-backend display-list or raster internals.

The current hybrid adapter is a good migration scaffold, but it should not become the permanent presentation architecture.

## What the client should consume from `engine-cef`

The client should consume a small, transport-neutral presentation surface:

- latest frame availability / generation
- frame dimensions and stride
- optional damage information
- a way to copy the latest frame into a caller-owned buffer

That is enough for the client to:

- composite browser chrome / HUD / diagnostics around content
- save screenshots
- keep windowing and app-level rendering ownership

without forcing the client to know CEF internals or own the content renderer implementation.

## What `engine-cef` should own

For the CEF path, `engine-cef` should become the owner of:

- content-frame production
- content pixel truth
- frame lifecycle for the page surface
- frame metadata (size / generation / damage)

## What the client should still own

Even after presentation v2, the client should still own:

- browser chrome
- HUD / diagnostics overlays
- app/window lifecycle
- engine selection / fallback UX
- screenshot orchestration and artifact policy
- any final composition that combines client-owned UI with engine-owned content

## Explicit non-goals for v2

This note does **not** require deciding yet:

- final zero-copy vs copy-based transport
- native CEF window embedding vs off-screen rendering forever
- tab/window multi-instance strategy
- final runtime/input contract

Those can follow once the frame/presentation seam is explicit.

## Why this is better than peeling more custom draw internals

If we keep moving custom draw internals into the adapter, we risk turning the migration scaffold into accidental architecture.

That would blur the long-term responsibility line and make it harder to tell which rendering behavior is actually owned by CEF vs temporarily borrowed from the custom backend.

A frame-oriented seam keeps the contract honest:

- `engine-cef` owns content pixels
- `client` owns app UI and composition policy

## Proposed v2 contract shape

The public API should stay small.

A plausible v2 expansion looks like:

### Additional public types

- `PresentationState`
  - `has_frame`
  - `width`
  - `height`
  - `stride_bytes`
  - `frame_generation`
  - optional damage rect metadata

### Additional bridge methods

- `presentation_state() const`
- `copy_latest_frame(uint32_t* dst_argb, int width, int height, int stride_bytes, std::string* error_out = nullptr)`

## Why copy-first is acceptable

A copy-first API is a good v2 step because it:

- is easy to reason about
- avoids locking the public contract to one memory-sharing strategy
- works for screenshots and client composition immediately
- leaves room for a later zero-copy/shared-memory optimization without invalidating the conceptual seam

In other words, the **ownership boundary** matters now more than transport optimization does.

## Initial implementation status

The first v2 slice has now started:

- `engine-cef` public headers expose `PresentationState`
- the bridge exposes `presentation_state()` and `copy_latest_frame(...)`
- the `client` CEF adapter prefers the engine-owned frame path when it is available
- the proof-side CEF wiring now includes an experimental OSR/`OnPaint` frame path intended to feed that contract

Current limitation:

- the producer path is not yet validated as stable
- a first local `--use-osr` runtime probe still crashed, so this path remains experimental
- until that runtime path is hardened, the backend should still be treated as effectively `has_frame=false` for normal client expectations

Additional runtime-hardening work has now started on the proof path:

- child-process OSR-safe switch propagation
- explicit proof cache/root-cache path
- extra OSR render-handler callbacks (`GetRootScreenRect`, `GetScreenPoint`, `GetScreenInfo`)
- proof-only hidden X11 parent window handle on Linux
- safer first-frame shutdown probing

The short plan for making the proof more faithful to upstream Linux OSR hosting now lives in:

- `docs/linux-osr-sample-parity-plan.md`

That sample-parity work has now produced a proof breakthrough: the Linux proof can reach first observed `OnPaint` and exit cleanly when run through the GTK/X11-backed OSR host path with X11-forced GTK backend selection.

That proof-side verification step now also works: the proof can confirm on first OSR paint that the presentation v2 surface is populated (`has_frame=true` in practice) and that `copy_latest_frame(...)` succeeds for the delivered frame.

The verification path now runs through the integration bridge surface, not only the internal backend object, which means the proof is exercising the same frame-copy contract shape that the client-facing path consumes.

There is now also a cheap regression guard for this contract in `tests/bridge_frame_contract_test.cpp`, which verifies in normal builds that a backend-observed frame updates the public bridge presentation state and can be copied back out through `copy_latest_frame(...)`.

Important caveat:

- upstream `cefclient` OSR still crashes in this environment
- so Linux OSR should still be treated as environment-sensitive and not yet broadly solved
- the current working first-frame recipe is proof-host specific, not yet a generic client-runtime guarantee

So the **contract, client preference path, first producer wiring, and first working proof-frame recipe now exist**, but runtime portability/stability is still the next hardening step.

## Recommended migration order after this note

1. Add a minimal presentation v2 surface to `engine-cef` public headers
2. Implement the first copy-based frame export path in `engine-cef`
3. Change the client CEF adapter so frame/screenshot data comes from `engine-cef`, not the custom fallback
4. Only then revisit deeper runtime/input migration

## Runtime/input recommendation

Do **not** make runtime/input the immediate next step.

Those are easier to reason about once the visual/frame source of truth is clear.

Recommended ordering:

- presentation/frame seam first
- runtime/input after that

## Success condition

We should consider presentation v2 successful when:

- `renderer=cef` still builds and runs through the client backend factory
- the client can obtain real content frames from `engine-cef`
- screenshots no longer depend on the custom backend for the CEF path
- the adapter no longer needs custom-backend frame ownership to present CEF content

At that point, the hybrid path becomes materially more honest, and the remaining delegated pieces are much more clearly runtime/input-specific rather than still visually coupled to the custom backend.
