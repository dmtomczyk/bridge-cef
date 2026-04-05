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
