#!/bin/sh
# Build and run (headless, with screenshots) the GTK 1.2 tutorial programs
# from a GTK 1.2 source tree against GTK made Qt.
#   tools/build-gtk1-examples.sh <gtk+-1.2 source dir> [<out dir>]
set -e
src=${1:?usage: $0 <gtk+-1.2 source dir> [out dir]}/examples
out=${2:-gtk1-examples}
here=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$out"
out=$(cd "$out" && pwd)
cfg="$here/build/lib/bin/gtk-config"
ok=0; bad=""
cd "$src"
for d in */; do
  d=${d%/}
  srcs=$(ls $d/*.c 2>/dev/null) || continue
  progs=$(grep -l "main *(" $srcs)
  others=$(for f in $srcs; do grep -q "main *(" $f || echo $f; done)
  for p in $progs; do
    name=$d; [ $(echo $progs | wc -w) -gt 1 ] && name=$(basename $p .c)
    if gcc -w -o "$out/$name" $p $others -I$d $($cfg --cflags --libs) -lm > "$out/$name.log" 2>&1; then
      ok=$((ok+1))
      (cd $d && QT_QPA_PLATFORM=offscreen GTKQT_AUTOQUIT=700 GTKQT_SCREENSHOT="$out/$name.png" timeout 15 "$out/$name" > "$out/$name.out" 2>&1) || echo "$name: exit $?"
    else
      bad="$bad $name"
    fi
  done
done
echo "built $ok; failed:$bad"
