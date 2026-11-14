# Testing

## Unit of truth: real GTK

Build the same program against the system GTK (Debian/Ubuntu
`libgtk2.0-dev`, `libgtk-3-dev`, `libgtk-4-dev`) and run it under Xvfb;
compare screenshots side by side. `./bgtk.sh gtk2/<example>` and
`./bgqt.sh gtk2/<example>` do this for the examples.

```bash
Xvfb :46 -screen 0 1024x768x24 &
```

```bash
DISPLAY=:46 ./prog-real & sleep 3; DISPLAY=:46 import -window root real.png; convert real.png -trim +repage real.png
```

`xdotool key/type` drives the real program. Restart Xvfb between captures.

## Headless runs of our build

- `QT_QPA_PLATFORM=offscreen` no display needed
- `GTKQT_AUTOQUIT=<ms>` exit after a delay; `GTKQT_SCREENSHOT=<png>` saves
  the windows first (`-1.png` ... for further windows)
- `GTKQT_PRINT_TO=<file.pdf>` makes GtkPrintOperation print to that file
  without dialogs
- `GTKQT_SCRIPT="..."` - steps separated by `;`:
  `wait <ms>`, `key <keys>` (QKeySequence syntax; accelerators fire),
  `type <text>`, `menu File/Save As...`, `click <widget> [button] [x y]`,
  `focus <widget>`, `file <path>` (accept the file dialog with that file),
  `accept` / `reject` (press the dialog's OK / Cancel response button),
  `shot <png>`, `dump` (widget tree), `quit`. Widgets are found by name
  (`gtk_widget_set_name`), button/label text, or kind (`textview`, `entry`,
  `treeview`, `window`); `click <tab text>` also switches notebook pages.

ctest runs `tests/` at all API levels plus every example (`ctest --test-dir build`).

## Leafpad (real application)

Source: a release tarball or the GitHub mirror `tarot231/leafpad`.

```bash
tools/build-leafpad.sh ~/src/leafpad
```

Things worth scripting: Options/Line Numbers (border window drawn with
GdkGC), typing and Edit/Undo (buffer signals), Search/Find (highlight tags,
selection), Search/Jump To (spin button), File/Save As (file chooser with
Leafpad's charset widget), Options/Font, Help/About, File/Print with
`GTKQT_PRINT_TO`.

```bash
QT_QPA_PLATFORM=offscreen GTKQT_SCRIPT="wait 400; menu Options/Line Numbers; menu Search/Find...; type indented; accept; shot find.png; quit" src/leafpad README
```

## GTK 1.2 tutorial programs

The GTK 1.2 sources ship the tutorial programs (`examples/`). This builds
each against the GTK 1.2 API level, runs it headless and saves a
screenshot:

```bash
tools/build-gtk1-examples.sh ~/src/gtk+-1.2.10 /tmp/gtk1-shots
```

`packbox` needs an argument; `tree` uses GtkTree, which is not provided.

## XMMS 1.2.11 (GTK 1.2 application)

Build as in the PKGBUILD in [packaging.md](packaging.md) (shared library,
time-stamp fix, configure flags). Use a scratch HOME with
`output_plugin=.../Output/libdisk_writer.so` in `~/.xmms/config`, so that
playback writes a WAV file instead of needing a sound card; uninstalled
plugins can be copied to `~/.xmms/Plugins/<type>/`. XMMS draws its own
buttons, so scripts click by window title and position:

```bash
GTKQT_SCRIPT="wait 1500; click XMMS 1 225 64; click XMMS 3 100 50; wait 500; quit"
```

(EQ button, then the right-click menu.) Ctrl+P opens Preferences,
`click Configure#2` the output plugin's settings; `key L` opens the file
browser and `file /path/song.wav` chooses a file and presses Play, after
which the disk writer's output must have the file's full length. For the
docked windows as a window manager-less X server shows them, run under
Xvfb with `QT_QPA_PLATFORM=xcb` and `import -window root`.

## Memory

```bash
QT_QPA_PLATFORM=offscreen GTKQT_AUTOQUIT=1500 valgrind -q ./build/test-gtk2
```
