#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-debug}"

if [[ "$(uname -s)" == "Darwin" ]] && command -v brew >/dev/null 2>&1; then
    qt_prefix="$(brew --prefix qt 2>/dev/null || true)"
    mpv_prefix="$(brew --prefix mpv 2>/dev/null || true)"
    if [[ -d "$qt_prefix" ]]; then
        export CMAKE_PREFIX_PATH="${qt_prefix}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
    fi
    if [[ -d "$mpv_prefix/lib/pkgconfig" ]]; then
        export PKG_CONFIG_PATH="${mpv_prefix}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"
    fi
fi

cmake --preset "$preset"
cmake --build --preset "$preset"
