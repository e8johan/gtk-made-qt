#!/bin/sh
# Build and run one example against the real GTK, for comparison.
#   ./bgtk.sh gtk3/treeview
set -e
ex=${1:-gtk2/helloworld}
api=$(echo "$ex" | sed -n 's/^gtk\([234]\)\/.*/\1/p')
case $api in 2) pc=gtk+-2.0 ;; 3) pc=gtk+-3.0 ;; 4) pc=gtk4 ;; *) echo "usage: $0 gtk2|gtk3|gtk4/<example>"; exit 1 ;; esac
here=$(cd "$(dirname "$0")" && pwd)
out=$(mktemp -d)/$(basename "$ex")
gcc "$here/examples/$ex.c" -o "$out" $(pkg-config --cflags --libs $pc) -rdynamic -lm
exec "$out"
