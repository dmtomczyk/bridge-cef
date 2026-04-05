# Runtime integration checkpoint and next-step plan

_Date: 2026-04-04_

## Why this note exists

We have pushed the CEF path far enough that the next work is no longer just scaffolding. This note captures the current state and the intended next planning step before deeper client/runtime integration work.

## Current verified state

### `engine-cef`

The `engine-cef` side now has:

- a public client-facing frame contract (`PresentationState`, `presentation_state()`, `copy_latest_frame(...)`)
- a proof host that can reach a real Linux OSR first frame
- proof-side verification that the **public integration bridge** can copy that frame successfully
- a cheap regression test for the bridge frame contract (`tests/bridge_frame_contract_test.cpp`)
- reusable Linux runtime-host/bootstrap code extracted out of the proof `main()`
- explicit runtime/launch config structs
- a small runtime entry helper and a minimal runtime host wrapper

### `client`

The `client` side now has:

- a real `renderer=cef` backend-factory path
- a hybrid CEF adapter that prefers bridge-fed frames when available
- adapter debug output that reports:
  - `frame-source ...`
  - `bridge-presentation has_frame=... size=... stride=... gen=...`
- a focused client test proving bridge-frame preference (`tests/cef_backend_bridge_preference_test.cpp`)
- runtime logging that surfaces bridge-frame activation even when the HUD is not open

## Important caveat

The proof path is meaningful, but Linux OSR is still environment-sensitive.

Important facts:

- our proof host now has a working first-frame recipe
- upstream `cefclient` OSR still crashes in this environment family
- therefore the current Linux OSR path should be treated as **working but fragile**, not yet as a broadly hardened production runtime path

## What we learned from OSR investigation

The biggest proof-side breakthrough came from making our Linux OSR host more faithful to upstream:

- GTK/X11-backed host path
- GTK/X11 setup before browser creation
- forcing GTK to X11 for OSR
- using `CefQuitMessageLoop()` for the first-frame quit probe instead of closing browsers directly

That was enough for our proof host to:

- reach first `OnPaint`
- verify bridge frame copy successfully
- exit cleanly in the controlled proof path

## What we should **not** do next

Do **not** jump straight into broad client embedding or pretend the current Linux OSR path is already a general-purpose runtime.

Also avoid turning proof-specific assumptions into accidental long-term public API.

## Recommended next step

### Short design pause before deeper runtime integration

Write one short note that defines the intended client/runtime boundary.

It should answer:

1. **What object should the client own/use?**
   - `CefRuntimeHost` directly?
   - a future higher-level runtime facade?

2. **What lifecycle should exist?**
   - create/start
   - readiness/first-frame signaling
   - stop/shutdown

3. **What state should be observable to the client runtime?**
   - started / failed / first-frame-ready
   - presentation metadata
   - fatal runtime failure / shutdown state

4. **What remains proof-only vs supported runtime surface?**
   - Linux/X11 OSR specifics
   - proof flags like `--quit-after-first-frame`
   - bridge verification helpers

5. **What is the first real client/runtime join point?**
   - likely a very small runtime launch seam, not full embedding

## Recommended implementation order after that note

1. define the runtime boundary note
2. add a tiny readiness/first-frame observable seam to the runtime host
3. connect one narrow non-proof caller path to that host shape
4. only then consider broader direct client/runtime integration

## Plain-English summary

We are in a good place:

- the frame contract is real
- the bridge path is real
- the proof host can produce and verify real frames
- the client can observe and prefer bridge-fed frames

The next work should be **careful runtime integration design**, not blind continued scaffolding.
