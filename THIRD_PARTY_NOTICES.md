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
