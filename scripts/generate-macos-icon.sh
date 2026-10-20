#!/bin/bash
set -euo pipefail
source_dir="$(cd "$(dirname "$0")/.." && pwd)"
qt_prefix="$(brew --prefix qt@5)"
scratch_dir="$(mktemp -d)"
trap 'rm -rf "$scratch_dir"' EXIT
xcrun clang++ -std=c++11 -fPIC -F"$qt_prefix/lib" \
  -I"$qt_prefix/lib/QtCore.framework/Headers" \
  -I"$qt_prefix/lib/QtGui.framework/Headers" \
  -I"$qt_prefix/lib/QtSvg.framework/Headers" \
  -framework QtCore -framework QtGui -framework QtSvg \
  -Wl,-rpath,"$qt_prefix/lib" \
  "$source_dir/scripts/render-macos-icon.cpp" -o "$scratch_dir/render"
QT_QPA_PLATFORM=offscreen "$scratch_dir/render" "$source_dir/src/macosx/app-icon.svg" "$scratch_dir/app.iconset"
iconutil -c icns "$scratch_dir/app.iconset" -o "$source_dir/src/macosx/klatexformula-macos.icns"
