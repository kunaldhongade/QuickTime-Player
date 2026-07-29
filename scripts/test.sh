#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-debug}"

"$repo_dir/scripts/build.sh" "$preset"
ctest --preset "$preset"
