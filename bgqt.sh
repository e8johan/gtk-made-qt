#!/bin/sh
# Build and run one example against GTK made Qt.
#   ./bgqt.sh gtk2/button      (GTK 2 API)
#   ./bgqt.sh gtk4/application (GTK 4 API)
# The library is built with CMake into ./build on first use. The example is
# compiled with plain gcc through the shim's drop-in pkg-config files, exactly
# like an unmodified third-party project would be.
set -e
ex=${1:-gtk2/helloworld}
api=$(echo "$ex" | sed -n 's/^gtk\([234]\)\/.*/\1/p')
[ -n "$api" ] || { echo "usage: $0 gtk2|gtk3|gtk4/<example>"; exit 1; }
case $api in 2) pc=gtk+-2.0 ;; 3) pc=gtk+-3.0 ;; 4) pc=gtk4 ;; esac
here=$(cd "$(dirname "$0")" && pwd)
[ -f "$here/build/libgtk-made-qt.so" ] || { cmake -S "$here" -B "$here/build" && cmake --build "$here/build" -j"$(nproc)"; }
out="$here/build/$(echo "$ex" | tr / -)"
PKG_CONFIG_PATH="$here/build/pkgconfig" gcc "$here/examples/$ex.c" -o "$out" \
    $(PKG_CONFIG_PATH="$here/build/pkgconfig" pkg-config --cflags --libs $pc) -lm
exec "$out"
