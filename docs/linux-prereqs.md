# Linux prerequisites for the CEF embedder spike

## Baseline build tools

Install or verify the following are available:

- `cmake` (3.21+)
- `g++` / `build-essential`
- `python3`
- `make` or `ninja`
- `git`

## Linux GUI / CEF-related packages

Minimum expected packages:

- `libgtk-3-dev`
- `libx11-dev`
- `libx11-xcb-dev`
- `libxcb1-dev`
- `libxcomposite-dev`
- `libxdamage-dev`
- `libxext-dev`
- `libxfixes-dev`
- `libxrandr-dev`
- `libgbm-dev`
- `libdrm-dev`
- `libglib2.0-dev`
- `libnss3-dev`
- `libatk1.0-dev`
- `libatk-bridge2.0-dev`
- `libasound2-dev`
- `libcups2-dev`
- `libdbus-1-dev`
- `libxkbcommon-dev`

## Suggested Debian/Ubuntu install command

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake python3 git ninja-build \
  libgtk-3-dev libx11-dev libx11-xcb-dev libxcb1-dev \
  libxcomposite-dev libxdamage-dev libxext-dev libxfixes-dev libxrandr-dev \
  libgbm-dev libdrm-dev libglib2.0-dev libnss3-dev \
  libatk1.0-dev libatk-bridge2.0-dev libasound2-dev \
  libcups2-dev libdbus-1-dev libxkbcommon-dev
```

## Runtime note

Some additional runtime libraries/resources will come from the CEF binary distribution itself.

That is expected.

## Spike policy

If a missing dependency blocks Phase 2, document it in the spike notes rather than papering over it silently.
