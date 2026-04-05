# Runtime integration boundary v1

_Date: 2026-04-05_

## Purpose

Turn the previous checkpoint note into a concrete near-term boundary for the first non-proof caller.

This is intentionally small and honest. It does **not** claim that Linux OSR is broadly hardened, and it does **not** invent a full long-term embedding API yet.

## Decision summary

### 1. What object should the caller own?

For the next slice, the caller should own **`CefRuntimeHost`**.

That means:

- the caller should **not** own `CefAppHost` directly
- the caller should **not** call `CefLinuxMainRunner` directly
- the caller should treat `CefRuntimeHost` as the smallest engine-cef-owned runtime wrapper

This is still an internal `engine-cef` seam, not yet a frozen cross-repo public API.

## 2. What should be observable?

For the first non-proof caller, the observable surface should be:

- the existing integration bridge for snapshot/presentation data
- a tiny host-owned runtime status surface for phase/readiness

Near-term rule:

- the caller can obtain the host bridge from `CefRuntimeHost`
- the caller can attach a bridge observer through `CefRuntimeHost`
- the caller can also attach a small runtime observer through `CefRuntimeHost`
- runtime readiness should be treated as **"first useful presentation snapshot"**, not as an abstract startup-complete event

In practice, the first readiness milestone is:

- `status.phase == CefRuntimePhase::first_frame_ready`
- which corresponds to `status.last_snapshot.presentation.has_frame == true`

That is the first useful moment for the current CEF path.

## 3. Lifecycle v1

The lifecycle should stay deliberately small:

1. construct/configure `CefRuntimeHost`
2. optionally attach a bridge observer before running
3. run the host
4. observe bridge snapshots while it is running
5. request shutdown through the host
6. treat `Run(...)` return as terminal completion

Important constraint:

- `Run(...)` is still a blocking call in v1
- async start/join/thread ownership is **not** part of this first boundary yet

## 4. What is supported vs proof-only?

### Supported in the near-term boundary

- `CefRuntimeHost`
- runtime config (`CefRuntimeEntryConfig`)
- bridge observation through `IIntegrationBridgeObserver`
- first-frame readiness inferred from presentation snapshots
- shutdown request through the host wrapper

### Still proof-only / not yet promised as runtime API

- proof CLI flags like `--quit-after-first-frame`
- proof verification helpers like `--verify-presentation-v2`
- Linux/X11 bootstrap details
- any claim that Linux OSR is generally production-hardened
- any broader multi-window/tab embedding API

## 5. First real client/runtime join point

The first non-proof join point should be:

- one narrow caller that owns a `CefRuntimeHost`
- attaches a bridge observer
- treats bridge presentation snapshots as runtime readiness

That is enough to test the runtime seam honestly without pretending we already have a complete embedding architecture.

## What we are explicitly *not* doing yet

- no new stable public runtime API under `include/engine_cef/` yet
- no deep client embedding
- no async runtime manager abstraction yet
- no generalized startup-state machine yet
- no attempt to hide all Linux OSR quirks behind a fake "done" abstraction

## Immediate implementation guidance

The next implementation step should be small:

1. make `CefRuntimeHost` the real proof entry path instead of an unused wrapper
2. expose the integration bridge from `CefRuntimeHost`
3. let a future non-proof caller attach an `IIntegrationBridgeObserver`
4. use first frame (`presentation.has_frame`) as the first readiness signal

The first guardrail for this seam should stay cheap: a CEF-enabled test can construct `CefRuntimeHost`, attach an observer through `host.bridge()`, feed a synthetic frame into the internal backend in-test, and verify that the observer sees `presentation.has_frame=true`.

## Plain-English summary

For now, the runtime boundary should be:

**caller owns `CefRuntimeHost`; caller observes readiness through the bridge; first frame is readiness.**

That is small, true, and aligned with the code we already have.

A first narrow non-proof caller path can now be expressed as a tiny host-owned probe: construct `CefRuntimeHost`, attach a bridge observer, and call `CefRuntimeHost::RequestQuit()` when the observer sees `presentation.has_frame=true`.
