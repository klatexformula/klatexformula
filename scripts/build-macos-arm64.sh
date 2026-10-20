#!/bin/bash
set -euo pipefail

if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
    echo "Run this script from a native Apple Silicon terminal." >&2
    exit 1
fi

source_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${1:-$source_dir/build-arm64}"
brew_prefix="$(brew --prefix)"
qt_prefix="$(brew --prefix qt@5)"
cmake_bin="$brew_prefix/bin/cmake"

"$cmake_bin" -S "$source_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$qt_prefix" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DKLF_MACOSX_BUNDLES=ON \
    -DKLF_MACOSX_BUNDLE_EXTRAS=ON \
    -DKLF_BUNDLE_LIBRARY_DIRS="$brew_prefix/lib" \
    -DKLF_BUILD_TOOLSDESPLUGIN=OFF \
    -DKLF_USE_SPARKLE=OFF \
    -DKLF_WELCOME_MSG_SILENT=ON
"$cmake_bin" --build "$build_dir" --parallel "$(sysctl -n hw.ncpu)"

app="$build_dir/src/klatexformula.app"
python3 "$source_dir/scripts/macos-bundle.py" --sign "$app"
KLF_HOME_CONFIG_DIR="$build_dir/smoke-config" \
    /usr/bin/arch -arm64 "$app/Contents/MacOS/klatexformula" --version
echo "Development app: $app"
