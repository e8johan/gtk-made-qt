# Real-World Target Applications & Build Instructions

This document catalogs self-contained C/C++ GTK applications (spanning GTK 1, GTK 2, and GTK 3) suited for testing and validating `gtk-made-qt`, along with build-system redirection instructions using the bundled `pkg-config` files.

Because `gtk-made-qt` preserves native **GLib**, **GObject**, **GIO**, and **Cairo** while replacing GDK and GTK with a GObject-backed Qt 6 (or Qt 5.15) wrapper, the best test candidates are applications that rely on standard GLib/Cairo primitives without external GNOME, Bonobo, or heavy custom X11/GDK drawing dependencies.

---

## 1. Quick-Start Build Redirection

`gtk-made-qt` provides drop-in `pkg-config` files (`gtk+-2.0.pc`, `gtk+-3.0.pc`, `gtk4.pc`) in `<build-dir>/pkgconfig` (or `<prefix>/lib/gtk-made-qt/pkgconfig` after `cmake --install`). These automatically inject `-DGTKQT_API_VERSION=2|3|4` and link against the shim and Qt Widgets.

### Standard `pkg-config` Override (Autotools, Meson, Custom Makefiles)

```bash
export PKG_CONFIG_PATH="/path/to/gtk-made-qt/build/pkgconfig"
```

Verify that `pkg-config` resolves to the shim before configuring a target project:

```bash
pkg-config --cflags --libs gtk+-2.0
# Expected: includes -I/path/to/gtk-made-qt/include -DGTKQT_API_VERSION=2 and links libgtk-made-qt + Qt6Widgets
```

### Useful Runtime Environment Variables

When running compiled test applications against `gtk-made-qt`:

* `QT_QPA_PLATFORM=offscreen` — Run GUI smoke tests headlessly (e.g., in CI).
* `GTKQT_AUTOQUIT=<ms>` — Automatically exit the event loop after `<ms>` milliseconds.
* `GTKQT_SCREENSHOT=<file.png>` — Capture every visible window to `<file.png>` right before `GTKQT_AUTOQUIT` triggers.

---

## 2. Curated Target Applications

### Tier 1: Minimal & Self-Contained C Targets (Best for Rapid Bring-Up)

These applications have zero external GNOME dependencies and compile quickly with `gcc`.

#### 1. mtPaint (GTK 1 / GTK 2 / GTK 3)
* **Why it fits:** A lightweight pixel-art and image editor written in pure C that supports compiling against GTK 1, GTK 2, and GTK 3 from the same source tree. It relies heavily on GLib data structures, custom drawing areas, toolbars, and dialogs.
* **Key widgets exercised:** `GtkDrawingArea` (expose/draw), `GtkToolbar`, `GtkMenuBar`, `GtkScrolledWindow`, `GtkSpinButton`, `GtkColorButton`, `GtkFileChooserDialog`.
* **Watch-outs:** Custom pixel blitting paths may call legacy `gdk_draw_rgb_image()` or `GdkPixbuf` helpers; check `src/canvas.c` if migrating beyond Cairo-backed paths.
* **Build instructions:**
  ```bash
  export PKG_CONFIG_PATH="/path/to/gtk-made-qt/build/pkgconfig"
  ./configure gtk2 --prefix=/tmp/mtpaint-qt
  make -j$(nproc)
  ```

#### 2. Xarchiver (GTK 2 / GTK 3)
* **Why it fits:** A clean, lightweight archive manager written in C with zero desktop environment dependencies. It relies on `g_spawn_async_with_pipes()` and `GIOChannel` (handled natively by GLib) paired with standard list views and toolbars.
* **Key widgets exercised:** `GtkListStore`, `GtkTreeView`, `GtkTreeViewColumn`, `GtkCellRendererText`, `GtkCellRendererPixbuf`, `GtkProgressBar`, `GtkToolbar`, `GtkNotebook`.
* **Build instructions:**
  ```bash
  export PKG_CONFIG_PATH="/path/to/gtk-made-qt/build/pkgconfig"
  ./autogen.sh --disable-gtk3   # Targets gtk+-2.0.pc; omit flag for gtk+-3.0.pc
  make -j$(nproc)
  ```

#### 3. Aumix (GTK 1 / GTK 2)
* **Why it fits:** An ultra-minimalist ALSA/OSS audio mixer in C. Ideal for verifying basic box packing, sliders, and menu bar actions without complex custom rendering.
* **Key widgets exercised:** `GtkHScale`, `GtkVScale`, `GtkAdjustment`, `GtkMenuBar`, `GtkToggleButton`, `GtkTable` / `GtkBox`.
* **Build instructions:**
  ```bash
  export PKG_CONFIG_PATH="/path/to/gtk-made-qt/build/pkgconfig"
  ./configure --without-ncurses
  make -j$(nproc)
  ```

---

### Tier 2: Real-World Desktop Applications (GTK 2 & GTK 3)

These widely used C applications exercise complex layouts, multi-view text buffers, and nested event loops (`gtk_dialog_run`).

#### 4. Sylpheed (GTK 2)
* **Why it fits:** A fast, self-contained C email client that remains on GTK 2. It uses `GIOChannel` for non-blocking network sockets inside the GLib main loop alongside rich tree and text widgets.
* **Key widgets exercised:** `GtkTreeStore`, `GtkTreeView`, `GtkTextView`, `GtkTextBuffer`, `GtkPaned`, `GtkNotebook`, `GtkStatusbar`, `GtkComboBoxText`.
* **Watch-outs:** Uses some legacy GTK 1/2 widgets (`GtkCList` in older dialogs, `GdkPixmap` icon loaders) and `pango_layout_*` calls in message header rendering.
* **Build instructions:**
  ```bash
  export PKG_CONFIG_PATH="/path/to/gtk-made-qt/build/pkgconfig"
  ./configure --disable-ssl --disable-gpgme   # Minimal dependency build for UI testing
  make -j$(nproc)
  ```

#### 5. Geany (GTK 2 / GTK 3)
* **Why it fits:** A lightweight C/C++ IDE with no GNOME dependencies. Geany bundles Scintilla, which renders text and margins through GDK/Cairo inside a custom widget container.
* **Key widgets exercised:** `GtkNotebook` (tabbed editor), `GtkTreeView` (symbol list & file browser), `GtkUIManager` / `GtkActionGroup` (GTK 2) or `GtkBuilder` (GTK 3), `GtkInfoBar`, `GtkDialog`.
* **Watch-outs:** Scintilla's GTK backend uses `pango_layout_*` and `GdkPixbuf` extensively; great stress test for Pango/Cairo integration.
* **Build instructions:**
  ```bash
  export PKG_CONFIG_PATH="/path/to/gtk-made-qt/build/pkgconfig"
  ./configure --enable-gtk3=no   # Use --enable-gtk3=yes to test against gtk+-3.0.pc
  make -j$(nproc)
  ```

#### 6. ROX-Filer (GTK 2)
* **Why it fits:** A classic single-binary C file manager with zero external desktop libraries.
* **Key widgets exercised:** Custom icon grid containers, `GtkEntry` (minibuffer), `GtkMenu` (context menus), `GdkPixbuf` thumbnail scaling, and drag-and-drop.
* **Watch-outs:** Heavy user of drag-and-drop APIs (`gtk_drag_source_set`, `gtk_drag_dest_set`) and `gdk_draw_*` primitives, making it a strong benchmark for expanding GDK coverage.
* **Build instructions:**
  ```bash
  export PKG_CONFIG_PATH="/path/to/gtk-made-qt/build/pkgconfig"
  cd ROX-Filer/src
  ./configure
  make -j$(nproc)
  ```

---

### Tier 3: Legacy GTK 1.2 Applications (Requires GLib 1.2 -> 2.0 Glue)

Original GTK 1.2 codebases expect `gtk-config` and `glib-config` (GLib 1.2) rather than `pkg-config` and GLib 2.0. To compile them against `gtk-made-qt`:

1. **Provide a `gtk-config` wrapper script** in `PATH` that delegates `--cflags` and `--libs` to `pkg-config --cflags --libs gtk+-2.0` (which sets `-DGTKQT_API_VERSION=2` and enables `gtk_signal_connect`, `GTK_OBJECT`, `gtk_timeout_add`, and `GtkCList`/`GtkCombo`/`GtkOptionMenu` compatibility).
2. **Bridge removed GLib 1.2 symbols** via a small force-included compatibility header (`-include glib1-compat.h`) mapping deprecated calls such as:
   * `g_strcasecmp` / `g_strncasecmp` → `g_ascii_strcasecmp` / `g_ascii_strncasecmp`
   * `g_string_sprintf` / `g_string_sprintfa` → `g_string_printf` / `g_string_append_printf`
   * `GMemChunk` → `g_slice_alloc` / `g_malloc`

#### 7. XMMS 1.2.11 (GTK 1.2)
* **Why it fits:** The classic Winamp-style audio player in pure C. Uses `GModule` for audio plugins and `GThread` for playback (both native in GLib 2.0), leaving only the skinned main window, playlist, and equalizer on the GTK shim.
* **Key widgets exercised:** `GtkWindow` (borderless/shaped), `GtkDrawingArea`, `GtkMenu`, `GtkFileSelection` / `GtkFileChooserDialog`, `GtkScale`.
* **Watch-outs:** Relies heavily on `GdkPixmap`, `GdkBitmap` window shape masks (`gtk_widget_shape_combine_mask`), and `gdk_draw_pixmap()` for skin blitting.

#### 8. Gentoo File Manager 0.11.x (GTK 1.2)
* **Why it fits:** A two-pane Norton Commander clone written purely in C against GTK 1.2 without any GNOME dependencies.
* **Key widgets exercised:** `GtkPaned`, `GtkCList`, `GtkTable`, `GtkNotebook`, `GtkEntry`, `GtkButton`.

---

### Tier 4: Cross-Toolkit Multiplier

#### 9. wxWidgets (`wxGTK2` / `wxGTK3`)
* **Why it fits:** Compiling wxWidgets' `src/gtk/` backend against `gtk-made-qt` exercises hundreds of GTK widget creation paths, signal hooks, and sizing queries in a single build. Once `libwx_gtk2u` or `libwx_gtk3u` builds, C++ wxWidgets applications (such as **FileZilla**, **Audacity**, or wxWidgets' own `samples/widgets` test suite) can link directly against it.
* **Build instructions:**
  ```bash
  export PKG_CONFIG_PATH="/path/to/gtk-made-qt/build/pkgconfig"
  mkdir build-wx && cd build-wx
  ../configure --with-gtk=2 --disable-shared --enable-monolithic
  make -j$(nproc)
  ```

---

## 3. Summary Matrix by Missing-Feature Coverage

Use this matrix to select a target application when implementing remaining items from the `gtk-made-qt` roadmap:

| Roadmap Area | Best Target Application(s) |
| :--- | :--- |
| **Core Layouts, Menus, Dialogs, `GtkTreeView`** | Xarchiver, Aumix |
| **Multi-view `GtkTextBuffer`, `GIOChannel` Sockets** | Sylpheed |
| **Custom `GtkDrawingArea` + Cairo** | mtPaint |
| **Pango Layout (`pango_layout_*`) in Cairo** | Geany, Sylpheed |
| **Legacy `gdk_draw_*` / `GdkGC` / `GdkPixmap`** | XMMS 1.2, ROX-Filer |
| **Drag-and-Drop & Clipboard APIs** | ROX-Filer, Geany |
| **Comprehensive Widget/Signal Audit** | wxWidgets (`samples/widgets`) |
