#!/bin/sh
# Build Leafpad (a GTK 2 text editor) unmodified against GTK made Qt.
#   tools/build-leafpad.sh <leafpad source dir> [<gtk-made-qt build dir>]
# The source can be a release tarball or a git checkout (then autotools are
# needed). Uses the build tree's pkg-config files unless gtk-made-qt-env is
# installed.
set -e
src=${1:?usage: $0 <leafpad source dir> [gtk-made-qt build dir]}
here=$(cd "$(dirname "$0")/.." && pwd)
build=${2:-$here/build}
cd "$src"
[ -x configure ] || autoreconf -fi
if command -v gtk-made-qt-env >/dev/null 2>&1 && [ -z "$2" ]; then
  env=gtk-made-qt-env
else
  env="$build/gtk-made-qt-env"
fi
$env ./configure --prefix=/usr
$env make
echo "built: $src/src/leafpad"
