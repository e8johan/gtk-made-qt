# User guide

GTK made Qt lets you build an old (or new) GTK 2, 3 or 4 program from its
unmodified sources and run it on any system with Qt 6 - without GTK. The
program becomes an ordinary Qt application: it follows the desktop's Qt
style, runs natively on Wayland, X11, Windows and macOS, and has no GTK
runtime dependency.

## Install

From a distribution package (`gtk-made-qt`), or from source:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DGTKQT_BUILD_EXAMPLES=OFF -DGTKQT_BUILD_TESTS=OFF
```

```bash
cmake --build build
```

```bash
sudo cmake --install build
```

This installs a static library, headers and link helpers under
`/usr/lib/gtk-made-qt` and `/usr/include/gtk-made-qt` (see packaging.md).
None of it is needed to *run* what you build. Build dependencies: Qt 6
(Widgets, PrintSupport), GLib, cairo, Pango, CMake.

## Build a GTK program

`gtk-made-qt-env` points a build at GTK made Qt: `pkg-config gtk+-2.0`
(`gtk+-3.0`, `gtk4`, `gdk-2.0` ...) answers with our headers and library,
and `-lgtk-x11-2.0` etc. resolve to it.

**configure scripts (autotools):**

```bash
gtk-made-qt-env ./configure --prefix=/usr
```

```bash
gtk-made-qt-env make
```

**Meson / CMake projects** find GTK through pkg-config too:

```bash
gtk-made-qt-env meson setup build
```

**By hand:**

```bash
eval "$(gtk-made-qt-env)"
```

```bash
cc prog.c -o prog $(pkg-config --cflags --libs gtk+-2.0)
```

Only `gtk-made-qt-env` (or `PKG_CONFIG_PATH`) decides whether a build uses
the real GTK or GTK made Qt, so both can be installed side by side.

## Run

- Qt style and theme: `-style fusion` or `QT_STYLE_OVERRIDE=fusion`.
- Platform: `QT_QPA_PLATFORM=wayland`, `xcb`, `offscreen` ...
- File, font and print dialogs are Qt's; printing goes through Qt
  (printers, PDF).
- GTK resource files (`gtkrc`) and CSS are not interpreted: the program
  takes the desktop's Qt look.

## What works

See the README's list. Real programs verified: Leafpad (GTK 2 text editor:
menus from an item factory, line numbers drawn with GdkGC, search, undo,
file chooser with extra widgets, font dialog, printing), plus the GTK 2/3/4
examples in `examples/`.

## When a build fails

- *undefined reference to `gtk_...`* or *implicit declaration*: a function
  not implemented yet. Please report it with the program's name.
- *errors about struct members* (`->allocation.width` works, private
  `->priv` members of GTK internals do not): the program reads internals.
- The build finds the system GTK: make sure it runs under
  `gtk-made-qt-env`, and that nothing adds `-I/usr/include/gtk-2.0`.

## Debugging and testing helpers

- `GTKQT_AUTOQUIT=<ms>`, `GTKQT_SCREENSHOT=<file.png>`: save the windows
  and exit.
- `GTKQT_SCRIPT="..."`: drive menus, keys and dialogs (docs/testing.md).
- `GTKQT_PRINT_TO=<file.pdf>`: print without dialogs.
