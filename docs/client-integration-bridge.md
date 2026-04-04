# `engine-cef` client integration bridge

## Purpose

Provide a tiny façade that is closer to what the Bridge shell/client actually wants to talk to than the raw proof-app classes.

The shell should not know about:

- `CefAppHost`
- `CefBrowserHandler`
- specific CEF callback wiring

Instead, it should be able to talk to a small integration bridge that:

- owns or references the backend
- receives observer updates
- exposes the latest snapshot
- forwards simple control commands

## Role in the migration

This bridge is **not** the final client/backend integration layer.

It is the first adapter that turns `engine-cef` from:

- a proof app plus backend core

into:

- something that a future `client` integration can consume more directly.

## Initial responsibilities

- create / attach to a `CefBackend`
- register itself as the backend observer
- cache the latest `BackendSnapshot`
- expose convenience methods:
  - `initialize`
  - `navigate`
  - `reload`
  - `go_back`
  - `go_forward`
  - `resize`
  - `snapshot`
  - `debug_summary`

## Non-goals

- final shell/backend lifecycle ownership
- multi-window/tab coordination
- proof-app replacement
- final client renderer selection wiring
