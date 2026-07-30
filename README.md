# FrameViewer

FrameViewer is a small native desktop video player for accurate frame-by-frame inspection. It
uses Qt 6 and QML for the interface, libmpv's Render API for playback inside a Qt-owned OpenGL
framebuffer, and FFprobe for an exact source-frame index.

The current development build supports local video opening and drag-and-drop, play/pause,
one-frame and ten-frame stepping, frame-based seeking, previous/next video navigation within the
open file's folder, marked frame-range PNG export, a hideable control overlay, fullscreen,
volume/mute, screenshots, and cached exact frame counts. Linux is the first production target;
the same playback and indexing code is kept platform-neutral for macOS and Windows.

## Keyboard controls

| Key | Action |
|---|---|
| Space | Play or pause |
| Left / Right | Tap for one source frame; hold for reverse / forward playback at 1× |
| Shift+Left / Shift+Right | Move 10 indexed frames |
| Ctrl+Left / Ctrl+Right | Previous / next video in the current folder |
| Home / End | First / final frame |
| Ctrl+O or Command+O | Open a video |
| F | Toggle fullscreen |
| H | Hide or show the control overlay |
| Escape | Leave fullscreen |
| M | Toggle mute |
| Up / Down | Change volume by 5% |
| S | Save the displayed video frame as PNG |

Arrow-key auto-repeat is ignored. A distinct arrow tap made during playback pauses first and is
serialized through the frame-step controller. Hold an arrow for about 320 ms to play at 1× in that
direction; releasing it pauses again. Backward decoding depends on the source codec and may be
less smooth than forward playback. Exact frame counters stay hidden until FFprobe has completed
indexing.

## Frame-range export

Pause on the first wanted frame and choose **Set Start**, move to the last wanted frame and choose
**Set End**, then choose **Export PNGs**. The endpoints are inclusive: selecting frames 400
through 700 writes 301 PNG files.

The first export for `video.mp4` is written beside the source as:

```text
video/
  video-frame-000400.png
  video-frame-000401.png
  …
  video-frame-000700.png
```

If that output folder already contains files, FrameViewer creates `video-2`, `video-3`, and so on
instead of overwriting an earlier export. FFmpeg performs the decoding in a background process,
and the player remains usable while progress is displayed.

## Linux setup

The bootstrap script prints the packages before it changes anything:

```bash
./scripts/bootstrap_linux.sh --print-only
./scripts/bootstrap_linux.sh
./scripts/build.sh
./scripts/test.sh
./scripts/run.sh
```

The build needs CMake 3.24+, Ninja, Qt 6.4+ (Core, Gui, Quick, QML, Quick Controls, OpenGL and
Test), libmpv development headers, FFprobe, and pkg-config.

Native Wayland rendering does not use an X11 window ID or a child mpv window. Qt Quick is forced
to its OpenGL backend before `QGuiApplication` is created, and libmpv renders into the
`QQuickFramebufferObject` supplied by Qt.

## macOS development

With Homebrew dependencies installed:

```bash
brew install cmake ninja qt mpv ffmpeg pkg-config
./scripts/build.sh
./scripts/test.sh
./scripts/run.sh /path/to/video.mov
```

This is a development path, not a signed or notarized macOS distribution.

## Build presets

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

cmake --preset release
cmake --build --preset release

cmake --preset sanitizers
cmake --build --preset sanitizers
ctest --preset sanitizers
```

Generate deterministic media fixtures with:

```bash
./scripts/generate_test_media.sh
```

## Architecture

- `MpvEngine` owns the `mpv_handle`, observes playback properties, and transfers mpv callbacks
  safely onto Qt's event loop.
- `MpvVideoItem` and its renderer create the mpv OpenGL render context only while the Qt scene
  graph context is current.
- `FrameIndexer` starts alongside media opening, incrementally parses compact FFprobe output in a
  cancellable `QProcess`, and reports timestamp-based progress.
- `FrameIndexCache` keys cached indexes by canonical path, size, modification time, stream, and
  cache schema.
- `FramePositionResolver` maps playback time to indexed presentation timestamps with binary
  search.
- `PlaybackController` serializes requested frame moves and corrects native mpv frame steps with
  exact indexed seeks when required.
- `ApplicationController` exposes a small typed state model and application commands to QML.

## Current packaging status

AppImage packaging is deliberately gated. Before distributing binaries, record the exact Qt,
mpv, and FFmpeg sources and build configurations in `THIRD_PARTY_NOTICES.md`, include their
applicable license texts, and decide whether FFprobe will be bundled. The Homebrew FFmpeg build
commonly used for development enables GPL components and must not silently become a distribution
dependency.

`scripts/package_appimage.sh` prepares the release install tree on Linux but stops at this
licensing gate. This is an engineering safeguard, not legal advice.

For a native Ubuntu 24.04 `amd64` package, run:

```bash
./scripts/package_deb_ubuntu.sh
sudo apt install ./dist/frameviewer_0.2.2_amd64.deb
```

The `.deb` declares Ubuntu's Qt, libmpv, and FFmpeg packages as dynamic runtime dependencies;
it does not bundle a second copy of those codec libraries.

## Known limitations

- `QQuickFramebufferObject` is the requested initial integration and requires Qt Quick's OpenGL
  backend. A future renderer should migrate to `QSGRenderNode`/QRhi as Qt evolves.
- The primary non-attached-picture stream is currently selected with FFprobe's `V:0` stream
  specifier. A future track selector can share the exact selected mpv video stream index.
- Frame entries with missing timestamps use a marked fallback derived from the previous frame.
- Timestamp-identical source frames cannot always be distinguished after an arbitrary exact seek;
  native frame-step remains the preferred path for adjacent frames.
- A first exact index still requires FFprobe to scan every decoded source frame, so its duration is
  proportional to video length and codec complexity. Reopening an unchanged file uses the cache.
- Linux X11/Wayland, high-DPI, GPU-vendor, AppImage clean-room, and sanitizer acceptance passes
  must be run on Linux hardware before a production release.
- Windows packaging and macOS signing/notarization have not started.

## Privacy

FrameViewer contains no analytics, telemetry, accounts, advertisements, or network playback
features. Logs and frame-index caches remain local.
