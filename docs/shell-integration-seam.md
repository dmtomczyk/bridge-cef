# `engine-cef` shell integration seam

## Purpose

Define the first practical seam between `engine-cef` and the Bridge shell/client.

This seam is intentionally narrow. It is not trying to solve the full migration in one step.

## First seam goals

The first shell-facing seam should let the shell:

- construct an `engine-cef` backend
- initialize it with startup parameters
- drive basic navigation intent
- observe page/load state changes without reaching into proof-app internals
- query the latest backend snapshot on demand

## Initial control surface

The current core already provides:

- `initialize(params)`
- `shutdown()`
- `navigate(url)`
- `reload()`
- `go_back()`
- `go_forward()`
- `resize(width, height)`
- `tick()`

## Initial observation surface

The next useful addition is:

- `snapshot()` — returns the latest combined page/load state
- observer registration — lets the shell react when state changes instead of blindly polling

## Why this seam matters

The current proof app proved that CEF can launch and show real content. The shell, however, does not want to know about proof-specific classes like:

- `CefAppHost`
- `CefBrowserHandler`
- CEF callback plumbing details

The shell wants a backend-shaped object with:

- control methods
- state snapshots
- change notifications

## Initial observer model

A minimal observer model is enough for the next slice:

- one optional observer
- page-state changed callback
- load-state changed callback

This can evolve later into a richer event stream if needed.

## Non-goals

This seam still does **not** decide:

- the final multi-window/tab ownership model
- deep input/focus/IME strategy
- whether the shell will ultimately host native CEF views directly or wrap them more heavily
- how `renderer=cef` is exposed in the client

Those are later slices.

## Decision

For now, `engine-cef` should expose a small snapshot/observer seam and use that as the first deliberate integration boundary with the shell.
