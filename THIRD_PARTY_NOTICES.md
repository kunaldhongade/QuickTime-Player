# Third-party dependency record

This file is a packaging checklist and is not legal advice. Development builds dynamically use
the dependencies below. No distributable FrameViewer package should be published until every
placeholder is replaced with the exact build that is being shipped.

## Qt 6

- Purpose: application framework, Qt Quick/QML interface, OpenGL integration
- Upstream: https://www.qt.io/
- Version and source: **record for release**
- Build configuration: **record for release**
- License selection and obligations: **verify for release**
- Dynamic-linking/relinking notes: **record for release**

## mpv / libmpv

- Purpose: media demux, decode, audio playback, and Render API
- Upstream: https://mpv.io/
- Version and source: **record for release**
- Build configuration and linked FFmpeg: **record for release**
- LGPL/GPL status of the exact build: **verify for release**

## FFmpeg / FFprobe

- Purpose: exact decoded-frame indexing and deterministic test media
- Upstream: https://ffmpeg.org/
- Version and source: **record for release**
- Enabled configure options: **record for release**
- LGPL/GPL status of the exact build: **verify for release**
- Bundled or system-located: **decide for release**

Applicable license texts must be placed under `LICENSES/` before packaging. Development package
manager installations are not proof that a dependency combination is suitable for redistribution.

## Local macOS QA snapshot (not approved for public redistribution)

The ad-hoc development DMG built on 2026-08-02 used Qt 6.11.1, mpv 0.41.0 and Homebrew FFmpeg
8.1.2 on Apple Silicon. Homebrew's FFmpeg formula enables GPL components, including x264/x265,
so this snapshot is recorded for reproducibility only and must not be attached to a public release.
`scripts/package_macos.sh` requires an explicit development flag for this combination and uses a
different artifact name. Distribution mode separately requires an approved FFmpeg directory,
completed license verification and a Developer ID identity.
