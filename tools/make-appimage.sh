#!/usr/bin/env bash
# Packs a Linux release build of Drum Programmer into an AppImage (AP11) with linuxdeploy.
# The program's samples, icons and texts are built into it; JACK is loaded from the system at runtime.
# For AppImageHub (AP25) the AppImage carries update information that points to the newest GitHub release,
# and a .zsync file is written next to it, which the release has to contain as well.
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
# AppImageHub takes the summary and description only from a file named *.appdata.xml (AP25).
mv "$appdir/usr/share/metainfo/$app_id.metainfo.xml" "$appdir/usr/share/metainfo/$app_id.appdata.xml"

mkdir -p "$output_dir"
cd "$output_dir"
appimage="DrumProgrammer-$version-x86_64.AppImage"
# AppImageUpdate looks for the .zsync file in the newest release of the repository.
update_information="gh-releases-zsync|ladych|DrumProgrammer|latest|DrumProgrammer-*x86_64.AppImage.zsync"
# Runs without FUSE, e.g. in containers.
APPIMAGE_EXTRACT_AND_RUN=1 LINUXDEPLOY_OUTPUT_VERSION="$version" ARCH=x86_64 \
    LDAI_OUTPUT="$appimage" LDAI_UPDATE_INFORMATION="$update_information" "$linuxdeploy" \
    --appdir "$appdir" \
    --desktop-file "$appdir/usr/share/applications/$app_id.desktop" \
    --icon-file "$appdir/usr/share/icons/hicolor/512x512/apps/$app_id.png" \
    --output appimage
if [[ ! -f "$appimage.zsync" ]]; then
    echo "linuxdeploy wrote no $appimage.zsync" >&2
    exit 1
fi
echo "$output_dir/$appimage"
