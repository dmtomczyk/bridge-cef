# Initial `engine-cef` backend contract

## Purpose

Define the smallest shell-facing contract that `engine-cef` should satisfy first.

This contract is intentionally **smaller** than the current screenshot-oriented `engine_api` surface used by the `engine-chromium` reference backend. The goal is to avoid baking the old MVP transport assumptions into the new long-term backend.

## Principles

1. **Do not inherit screenshot-era baggage by default.**
2. **Start with browser behavior, not custom frame transport.**
3. **Prefer shell-facing intent/state over backend-specific internals.**
4. **Migrate in slices.**

## First slice responsibilities

The first shell-facing integration slice for `engine-cef` should cover:

- initialize / shutdown
- browser bootstrap options
- navigate / reload / back / forward
- resize hook
- periodic tick hook (even if light)
- current page state
  - current URL
  - title
  - can-go-back
  - can-go-forward
- basic load state
  - loading
  - first page shown
  - last error

## Explicitly *not* in the first slice

The first slice should **not** attempt to preserve or recreate the old `engine-chromium` transitional hooks for:

- screenshot/display-list transport
- shell-owned DOM/document snapshot synchronization
- shell-driven JS/runtime patch-up hooks from the old MVP path

If any of those later prove necessary, they should be reintroduced deliberately, not by inertia.

## Proposed contract shape

A minimal backend contract should look roughly like:

- `initialize(params)`
- `shutdown()`
- `navigate(url)`
- `reload()`
- `go_back()`
- `go_forward()`
- `resize(width, height)`
- `tick()`
- `page_state()`
- `load_state()`
- `debug_summary()`

## Why this shape

This is the minimum useful contract that lets the shell start integrating against a real browser backend without forcing us to decide the final presentation model on day one.

## Migration slices

### Slice 1 — standalone browser lifecycle

- engine bootstrap
- browser window opens
- page loads
- page state is observable

### Slice 2 — shell-facing navigation bridge

- shell can tell backend to navigate/reload/back/forward
- shell can observe title/url/load state changes

### Slice 3 — input/focus basics

- click/input/focus integration
- enough to drive normal browsing flows from the shell layer

### Slice 4 — advanced shell/backend concerns

Only after the basics are stable:

- deeper window/tab ownership decisions
- richer debug/state reporting
- optional inspection/devtools integration
- any custom presentation/embedding concerns beyond the default native path

## Current decision

For now, `engine-cef` should implement the **first slice contract** and avoid inheriting the old reference-backend transport assumptions.
