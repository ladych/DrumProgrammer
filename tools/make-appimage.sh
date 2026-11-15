#!/usr/bin/env bash
# Packs a Linux release build of Drum Programmer into an AppImage (AP11) with linuxdeploy.
# The program's samples, icons and texts are built into it; JACK is loaded from the system at runtime.
# Usage: tools/make-appimage.sh <release-build-dir> [output-dir]
# linuxdeploy is taken from $LINUXDEPLOY, else downloaded into the build directory.
set -euo pipefail

build_dir="$(cd "${1:?release build directory required}" && pwd)"
output_dir="${2:-$build_dir}"
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
executable="$build_dir/src/DrumProgrammer_artefacts/Release/Drum Programmer"
version="$(sed -n 's/^project(DrumProgrammer VERSION \([0-9.]*\).*/\1/p' "$repo_root/CMakeLists.txt")"

if [[ ! -x "$executable" ]]; then
    echo "missing $executable, build with -DCMAKE_BUILD_TYPE=Release first" >&2
    exit 1
fi

linuxdeploy="${LINUXDEPLOY:-$build_dir/linuxdeploy-x86_64.AppImage}"
if [[ ! -x "$linuxdeploy" ]]; then
    curl -fsSL -o "$linuxdeploy" \
        https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x "$linuxdeploy"
fi

appdir="$build_dir/AppDir"
app_id=io.github.ladych.DrumProgrammer
rm -rf "$appdir"
cmake --install "$build_dir" --prefix "$appdir/usr"

mkdir -p "$output_dir"
cd "$output_dir"
# Runs without FUSE, e.g. in containers.
APPIMAGE_EXTRACT_AND_RUN=1 LINUXDEPLOY_OUTPUT_VERSION="$version" ARCH=x86_64 "$linuxdeploy" \
    --appdir "$appdir" \
    --desktop-file "$appdir/usr/share/applications/$app_id.desktop" \
    --icon-file "$appdir/usr/share/icons/hicolor/512x512/apps/$app_id.png" \
    --output appimage
mv -f "Drum_Programmer-$version-x86_64.AppImage" "DrumProgrammer-$version-x86_64.AppImage"
echo "$output_dir/DrumProgrammer-$version-x86_64.AppImage"
