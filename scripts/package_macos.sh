#!/usr/bin/env bash

set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(sed -n 's/^project(FrameViewer VERSION \([^ ]*\).*/\1/p' "$repo_dir/CMakeLists.txt")"
dist_dir="$repo_dir/dist/macos"
work_dir="$(mktemp -d /tmp/frameviewer-macos.XXXXXX)"
app_path="$work_dir/FrameViewer.app"
development=0

cleanup() {
    rm -r "$work_dir"
}
trap cleanup EXIT

if [[ "${1:-}" == "--development" ]]; then
    development=1
elif [[ -n "${1:-}" ]]; then
    echo "Usage: $0 [--development]" >&2
    exit 2
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "macOS packaging must run on macOS." >&2
    exit 1
fi

for tool in cmake macdeployqt hdiutil codesign otool qtpaths; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing required tool: $tool" >&2
        exit 1
    fi
done

if (( development == 0 )); then
    : "${FRAMEVIEWER_CODESIGN_IDENTITY:?Set FRAMEVIEWER_CODESIGN_IDENTITY to a Developer ID Application identity}"
    : "${FRAMEVIEWER_FFMPEG_DIR:?Set FRAMEVIEWER_FFMPEG_DIR to an approved redistributable ffmpeg/ffprobe build}"
    if [[ "${FRAMEVIEWER_LICENSES_VERIFIED:-}" != "YES" ]]; then
        echo "Refusing a distribution package until FRAMEVIEWER_LICENSES_VERIFIED=YES." >&2
        echo "Complete THIRD_PARTY_NOTICES.md and LICENSES/ first." >&2
        exit 1
    fi
fi

ffmpeg_dir="${FRAMEVIEWER_FFMPEG_DIR:-}"
if [[ -z "$ffmpeg_dir" ]]; then
    ffmpeg_dir="$(dirname "$(command -v ffmpeg)")"
fi
for helper in ffmpeg ffprobe; do
    if [[ ! -x "$ffmpeg_dir/$helper" ]]; then
        echo "Missing executable $ffmpeg_dir/$helper" >&2
        exit 1
    fi
done

cmake --preset release -S "$repo_dir"
cmake --build --preset release --parallel
cp -R "$repo_dir/build/release/frameviewer.app" "$app_path"

# Deploy the executable's Qt/libmpv dependency graph first. QML imports are copied
# selectively because scanning Homebrew's complete QML tree pulls unrelated modules.
macdeployqt "$app_path" -always-overwrite -no-codesign -verbose=1

qml_source="$(qtpaths --query QT_INSTALL_QML)"
plugin_source="$(qtpaths --query QT_INSTALL_PLUGINS)"
qml_dest="$app_path/Contents/Resources/qml"
mkdir -p "$qml_dest/QtQuick/Controls" "$app_path/Contents/Helpers" \
    "$app_path/Contents/PlugIns/imageformats"
cp -RL "$qml_source/QtQuick/Controls/Basic" "$qml_dest/QtQuick/Controls/"
cp -L "$qml_source/QtQuick/Controls/libqtquickcontrols2plugin.dylib" \
      "$qml_source/QtQuick/Controls/qmldir" "$qml_dest/QtQuick/Controls/"
cp -RL "$qml_source/QtQuick/Dialogs" "$qml_dest/QtQuick/"
cp -RL "$qml_source/QtQuick/Layouts" "$qml_dest/QtQuick/"
cp -RL "$qml_source/QtQuick/Templates" "$qml_dest/QtQuick/"
cp "$ffmpeg_dir/ffmpeg" "$ffmpeg_dir/ffprobe" "$app_path/Contents/Helpers/"
cp "$plugin_source/imageformats/libqsvg.dylib" "$app_path/Contents/PlugIns/imageformats/"

deploy_args=(
    -no-plugins
    -always-overwrite
    -no-codesign
    -verbose=1
    "-executable=$qml_dest/QtQuick/Controls/libqtquickcontrols2plugin.dylib"
    "-executable=$qml_dest/QtQuick/Dialogs/libqtquickdialogsplugin.dylib"
    "-executable=$qml_dest/QtQuick/Layouts/libqquicklayoutsplugin.dylib"
    "-executable=$qml_dest/QtQuick/Templates/libqtquicktemplates2plugin.dylib"
    "-executable=$app_path/Contents/Helpers/ffmpeg"
    "-executable=$app_path/Contents/Helpers/ffprobe"
    "-executable=$app_path/Contents/PlugIns/imageformats/libqsvg.dylib"
)
macdeployqt "$app_path" "${deploy_args[@]}"

nonportable_file="$work_dir/nonportable.txt"
while IFS= read -r -d '' binary; do
    if otool -L "$binary" 2>/dev/null | grep -E '/opt/homebrew|/Users/' > /dev/null; then
        printf '%s\n' "$binary" >> "$nonportable_file"
    fi
done < <(find "$app_path/Contents" -type f -perm -111 -print0)
if [[ -s "$nonportable_file" ]]; then
    echo "Package still contains machine-local library references:" >&2
    cat "$nonportable_file" >&2
    exit 1
fi

if (( development == 1 )); then
    while IFS= read -r -d '' candidate; do
        if file "$candidate" | grep -q 'Mach-O'; then
            codesign --force --sign - "$candidate" >/dev/null
        fi
    done < <(find "$app_path/Contents" -type f -print0)
    codesign --force --sign - "$app_path"
    artifact_suffix="development-adhoc"
else
    while IFS= read -r -d '' candidate; do
        if file "$candidate" | grep -q 'Mach-O'; then
            codesign --force --options runtime --timestamp \
                --sign "$FRAMEVIEWER_CODESIGN_IDENTITY" "$candidate" >/dev/null
        fi
    done < <(find "$app_path/Contents" -type f -print0)
    codesign --force --options runtime --timestamp \
        --sign "$FRAMEVIEWER_CODESIGN_IDENTITY" "$app_path"
    artifact_suffix="macos"
fi
codesign --verify --deep --strict --verbose=2 "$app_path"
plutil -lint "$app_path/Contents/Info.plist"

mkdir -p "$dist_dir"
dmg_root="$work_dir/dmg"
mkdir -p "$dmg_root"
cp -R "$app_path" "$dmg_root/FrameViewer.app"
ln -s /Applications "$dmg_root/Applications"
dmg_path="$dist_dir/FrameViewer-${version}-${artifact_suffix}-arm64.dmg"
if [[ -e "$dmg_path" ]]; then
    dmg_path="$dist_dir/FrameViewer-${version}-${artifact_suffix}-arm64-$(date +%Y%m%d-%H%M%S).dmg"
fi
hdiutil create -volname "FrameViewer" -srcfolder "$dmg_root" \
    -format UDZO -ov "$dmg_path"
shasum -a 256 "$dmg_path" > "$dmg_path.sha256"

echo "Created $dmg_path"
if (( development == 1 )); then
    echo "Development-only: ad-hoc signed and built from local Homebrew dependencies."
    echo "Do not attach this artifact to a public GitHub release."
fi
