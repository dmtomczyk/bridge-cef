# `engine-cef` migration slices

## Goal

Move from a successful standalone native CEF proof to a backend that can be integrated into the Bridge shell in controlled, low-risk slices.

## Slice 0 — proof (done)

Completed proof:

- native CEF browser window launches
- `https://example.com` renders
- manual clicking works

## Slice 1 — backend core

Create a small `engine_cef_core` layer that owns:

- init params
- bootstrap options
- page/load state structs
- the minimal backend contract
- a concrete `CefBackend` class skeleton

This slice is about code shape and contract clarity, not full shell integration yet.

## Slice 2 — event/state plumbing

Connect the proof app's lifecycle callbacks into the backend core so the backend can report:

- current URL
- title
- loading state
- first page shown
- last error

At the end of Slice 2, the standalone app should still work, but the state should be owned by the backend core instead of being trapped inside ad-hoc handler code.

## Slice 3 — shell-facing integration seam

Add the first integration-facing bridge so the shell can:

- construct/select a CEF backend
- call navigate/reload/back/forward
- observe title/url/load changes

Keep the current `engine-chromium` backend as the reference baseline during this phase.

## Slice 4 — input and operational parity

Add enough shell-facing input/focus operations for realistic browsing:

- click
- text input
- key handling
- focus basics

## Slice 5 — migration decision point

Once the above slices are stable, decide:

- when to expose `renderer=cef` in the shell
- what screenshot-era hooks should be deleted or left reference-only
- whether the CEF path is clearly the long-term winner
