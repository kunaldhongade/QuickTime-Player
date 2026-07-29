#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$(uname -s)" != "Linux" ]]; then
    echo "AppImage packaging must run on Linux." >&2
    exit 1
fi
if ! command -v linuxdeploy >/dev/null 2>&1; then
    echo "linuxdeploy is required. Download and verify it from https://github.com/linuxdeploy/linuxdeploy." >&2
    exit 1
fi

"$repo_dir/scripts/build.sh" release
cmake --install "$repo_dir/build/release" --prefix "$repo_dir/build/AppDir/usr"

echo "Packaging is intentionally gated until exact Qt/mpv/FFmpeg license configurations are recorded."
echo "Complete THIRD_PARTY_NOTICES.md, then invoke linuxdeploy for build/AppDir."
exit 2
