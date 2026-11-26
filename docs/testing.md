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
  `drag <widget> x0 y0 x1 y1 [button]` (press, ten moves, release),
  `focus <widget>`, `file <path>` (accept the file dialog with that file),
  `accept` / `reject` (press the dialog's OK / Cancel response button),
  `shot <png>`, `dump` (widget tree), `quit`. Widgets are found by name
  (`gtk_widget_set_name`), button/label text, or kind (`textview`, `entry`,
  `treeview`, `layout` - a GtkLayout such as GnomeCanvas, `window`); `click <tab text>` also switches notebook pages.
  `type` delivers all its keys at once; programs with timers between key
  presses (Galculator) need `type 1; wait 150; type 2 ...`.

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

Emacs key theme: configure Leafpad with `--enable-emacs` and use a scratch
HOME whose `.gtkrc-2.0` says `gtk-key-theme-name = "Emacs"` (the theme is
read from `/usr/share/themes/Emacs/gtk-2.0-key/gtkrc`). Use Ctrl keys only:
Alt+F opens the File menu, as with real GTK 2.

```bash
printf 'gtk-key-theme-name = "Emacs"\n' > /tmp/h/.gtkrc-2.0
HOME=/tmp/h QT_QPA_PLATFORM=offscreen GTKQT_SCRIPT="wait 500; key Ctrl+E; type X; key Ctrl+N; key Ctrl+A; key Ctrl+K; key Ctrl+K; key Ctrl+P; shot emacs.png; quit" src/leafpad file.txt
```

## CSS against real GTK 3

`tests/test-css.c` covers the matching; for the look, build a small GTK 3
program with a CSS provider both ways and compare (real GTK under Xvfb,
`import -window root`). Expected difference: Adwaita paints a gradient
over buttons, so real GTK hides a button's `background-color`.

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

## Xournal 0.4.8 (GTK 2, on libgnomecanvas)

libgnomecanvas 2.30.3 is built first, static, into a scratch prefix (it
needs libart, and ATK for its accessibility code, which links libgailutil):

```bash
gtk-made-qt-env sh -c './configure --prefix=/tmp/xo-prefix --disable-shared --enable-static && make && make install'
```

then Xournal against it (`PKG_CONFIG="pkg-config --static"`,
`PKG_CONFIG_PATH=/tmp/xo-prefix/lib/pkgconfig`, poppler-glib from the
system). The canvas is found by kind (`layout`), strokes are drawn with
`drag`:

```bash
GTKQT_SCRIPT="wait 1500; drag layout 200 250 600 400; click Red; click Text; click layout 1 400 450; type Note; click Select Rectangle; drag layout 150 120 650 520; key Ctrl+C; key Ctrl+V; menu File/Export to PDF; file /tmp/x.pdf; wait 800; quit" ./xournal doc.pdf
```

The reference is Ubuntu's own `xournal` under Xvfb, driven with `xdotool`
(click the tool button, click on the page, `xdotool type`).

## Memory

```bash
QT_QPA_PLATFORM=offscreen GTKQT_AUTOQUIT=1500 valgrind -q ./build/test-gtk2
```
