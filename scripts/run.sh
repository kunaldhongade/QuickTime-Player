#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${FRAMEVIEWER_PRESET:-debug}"
"$repo_dir/scripts/build.sh" "$preset"

if [[ "$(uname -s)" == "Darwin" ]]; then
    exec "$repo_dir/build/$preset/frameviewer.app/Contents/MacOS/frameviewer" "$@"
fi
exec "$repo_dir/build/$preset/frameviewer" "$@"
