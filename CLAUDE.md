# GTK made Qt (gtk-made-qt)

The GTK 2/3/4 C API implemented on Qt 6 (or 5.15), so unmodified GTK
programs compile as plain C and run without GTK. Static library; programs
depend on Qt, GLib/GObject/GIO, cairo and Pango at run time - never GTK.

## Commands

```bash
cmake -B build && cmake --build build -j8 && ctest --test-dir build
```

Headless runs: `QT_QPA_PLATFORM=offscreen GTKQT_AUTOQUIT=800
GTKQT_SCREENSHOT=out.png ./prog`, or `GTKQT_SCRIPT="wait 500; menu
File/Open...; file /tmp/x.txt; shot a.png; quit"` (see docs/testing.md).

## Layout

- `include/gtk/gtk.h` umbrella; `gtkqt-*.h` hold the declarations by area;
  every other `gtk/gtkxxx.h` / `gdk/gdkxxx.h` name just includes the umbrella
- `src/core.cpp` GtkWidget, bridge (Qt events -> GTK signals), init/main loop,
  stock items; `containers.cpp`, `controls.cpp`, `range.cpp`, `menus.cpp`,
  `text.cpp`, `treeview.cpp`, `dialogs.cpp`, `drawing.cpp`, `builder.cpp`,
  `uimanager.cpp`, `application.cpp`
- `src/gtk2.cpp` GTK 1/2 era API: GdkDrawable/GdkGC on cairo, GtkStyle,
  Pango glue, item factory, clipboard, DnD, file/font selection
- `src/print.cpp` GtkPrintOperation on QPrinter; `src/script.cpp` GTKQT_SCRIPT
- `examples/gtk2|3|4` programs built unmodified; `tests/` regression tests

## Conventions

- Every widget is a real GObject; its Qt object is created by the class's
  `gtkqt_set_create` hook; reach it with `gtkqt_qw`/`gtkqt_q<T>`
- GTK signals with GTK semantics: changes go through the signal's default
  handler (text buffer insert-text/delete-range, editable insert-text) so
  programs can veto them
- Public GTK 2 struct fields programs read (widget->style, ->window,
  GtkTextView->buffer, GtkDialog->vbox ...) are kept current
- `GTKQT_API_VERSION` (2/3/4, from the .pc files) selects per-generation
  spellings in `gtkqt-compat.h`; runtime checks use `gtkqt_api`
- Verify against real GTK: same program built with the system GTK under
  Xvfb, screenshots side by side (docs/testing.md)
- Commits end with the session attribution trailers; no model names in
  commits; branch `claude/hopeful-ritchie-p2uwbj`

## More

- docs/architecture.md - how it works
- docs/testing.md - headless testing, comparison with real GTK, Leafpad
- docs/lessons.md - pitfalls found the hard way
- docs/user-guide.md, docs/packaging.md - for users and packagers
- docs/target-applications.md - programs worth porting
