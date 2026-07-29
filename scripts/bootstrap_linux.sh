#!/usr/bin/env bash
set -euo pipefail

mode="install"
case "${1:-}" in
    --print-only|--no-install) mode="print" ;;
    "") ;;
    *)
        echo "Usage: $0 [--print-only|--no-install]" >&2
        exit 2
        ;;
esac

if [[ ! -r /etc/os-release ]]; then
    echo "Unable to detect a supported Linux distribution (/etc/os-release is missing)." >&2
    exit 1
fi
. /etc/os-release

manager=""
packages=()
case "${ID:-}" in
    ubuntu|debian|linuxmint|pop)
        manager="apt"
        packages=(build-essential cmake ninja-build pkg-config qt6-base-dev
                  qt6-declarative-dev qt6-tools-dev libqt6opengl6-dev
                  libmpv-dev mpv ffmpeg)
        ;;
    fedora)
        manager="dnf"
        packages=(gcc-c++ cmake ninja-build pkgconf-pkg-config qt6-qtbase-devel
                  qt6-qtdeclarative-devel mpv-libs-devel mpv ffmpeg)
        ;;
    arch|manjaro)
        manager="pacman"
        packages=(base-devel cmake ninja pkgconf qt6-base qt6-declarative mpv ffmpeg)
        ;;
    *)
        echo "Unsupported distribution '${ID:-unknown}'." >&2
        echo "Install CMake 3.24+, Ninja, Qt 6.5+ development packages, libmpv headers, and FFmpeg." >&2
        exit 1
        ;;
esac

echo "FrameViewer needs the following development and runtime packages:"
printf '  %s\n' "${packages[@]}"
echo "Package manager: $manager"
echo "No graphics drivers or system configuration will be changed."

if [[ "$mode" == "print" ]]; then
    exit 0
fi

case "$manager" in
    apt)
        sudo apt-get update
        sudo apt-get install "${packages[@]}"
        ;;
    dnf)
        sudo dnf install "${packages[@]}"
        ;;
    pacman)
        sudo pacman -S --needed "${packages[@]}"
        ;;
esac
