#!/usr/bin/env bash
# Writes the Flathub manifest of Drum Programmer (AP24): packaging/flatpak/io.github.ladych.DrumProgrammer.yml
# with the release tag as source instead of the local checkout. The tag has to exist on GitHub.
# Usage: tools/make-flathub-manifest.sh <tag> [output-file]
set -euo pipefail

tag="${1:?release tag required, e.g. v1.0.1}"
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
app_id=io.github.ladych.DrumProgrammer
output="${2:-$app_id.yml}"
commit="$(git -C "$repo_root" ls-remote https://github.com/ladych/DrumProgrammer.git "refs/tags/$tag^{}" "refs/tags/$tag" \
    | sort -k2 -r | head -n1 | cut -f1)"
if [[ -z "$commit" ]]; then
    echo "tag $tag not found on GitHub" >&2
    exit 1
fi

# Everything up to the source of the module stays as it is.
sed -n '/^      - type: dir$/q;p' "$repo_root/packaging/flatpak/$app_id.yml" | grep -v '^#' > "$output"
cat >> "$output" <<SOURCE
      - type: git
        url: https://github.com/ladych/DrumProgrammer.git
        tag: $tag
        commit: $commit
        x-checker-data:
          type: git
          tag-pattern: ^v([\\d.]+)\$
SOURCE
echo "$output"
