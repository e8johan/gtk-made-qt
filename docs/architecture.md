# Architecture

## Headers

`include/gtk/gtk.h` is plain C. It includes GLib, GIO, cairo and Pango (the
real ones: they are not GTK) and our declarations in `gtkqt-*.h`.
`-DGTKQT_API_VERSION=2|3|4` (set by the bundled `gtk+-2.0.pc`, `gtk+-3.0.pc`,
`gtk4.pc`) picks each generation's spelling (`gtk_init()` arguments,
`gtk_window_new()` ...) in `gtkqt-compat.h`. All the GTK 2/3 sub-header names
(`gtk/gtktextiter.h` ...) exist and include the umbrella.

## Widgets

Every GtkWidget is a GObject of the right type (GtkWidget -> GtkContainer ->
GtkBox ...), so signals, properties, refcounting, floating references and
type checks are GTK's. Each widget owns one Qt object, created on first use
by its class's create hook: a QWidget, or a QAction for menu items. A bridge
QObject turns Qt signals and events into GTK signals and GdkEvents; Qt
objects are deleted from an idle callback, so handlers may destroy their own
widget. Containers place children through per-class place/unplace hooks into
Qt layouts that follow GTK's packing rules (expand/fill/padding, tables,
grids).

## Text

GtkTextBuffer is a QTextDocument shown by any number of GtkTextViews
(QTextEdit subclasses). As in GTK, every change goes through the buffer's
`insert-text` and `delete-range` signals, whose default handlers edit the
document and revalidate the iters; typing, deleting, cut and paste in the
view call the buffer, wrapped in user actions. GTK 2/3 buffers have no undo
(programs bring their own). Iterators are character offsets. GTK 2 border
windows (line numbers) are QWidgets in the scroll area's viewport margins;
painting one emits `expose-event` with its GdkWindow, which the program draws
on.

## Drawing (GDK)

GdkWindow, GdkPixmap and GdkDrawable are one GObject type (`_GdkWindow`):
a widget's window, a text view border window, or an image (pixmap). While a
window is being painted it has a cairo image surface; `gdk_cairo_create`,
`gdk_draw_*` with a GdkGC (colours, line styles, clip, XOR via difference)
and `gtk_paint_layout` draw on it, then the image is shown. Drawing outside
an expose handler goes nowhere (like X without backing store); pixmaps keep
their contents.

GtkStyle is filled from the Qt palette and the widget's font; Pango contexts
for widgets use the same font at the screen's DPI, so text measured with
Pango matches what Qt draws.

## Menus, dialogs, printing

GtkMenuBar/GtkMenu are QMenuBar/QMenu, items QActions; accelerators are
QAction shortcuts adopted by the windows of the accel group. GtkItemFactory
and GtkUIManager build on them. File choosers and GtkFileSelection are
QFileDialogs (non-native, so extra widgets fit in), font selection a
QFontDialog. GtkPrintOperation draws each page with cairo in points onto an
image at the printer's resolution and gives it to QPrinter (print dialog,
preview, or PDF).

## GTK 1.2

API level 1 adds GTK 1's type and signal layer on GObject: `GtkTypeInfo` and
`gtk_type_unique` register GTypes, `gtk_signal_new` creates signals with
class-offset closures, `gtk_widget_set ("GtkWidget::width", ...)` maps onto
properties. For API levels 1 and 2 `GtkObject` and `GtkObjectClass` are
declared so that `object->klass`, `klass->type` and `object_class->destroy`
are the GObject fields at the same place. GTK 1 code often keeps a type in a
`guint`, which truncates a 64-bit GType; the `gtk_type_*` calls and
`GTK_CHECK_*` macros recover such types from their low 32 bits.

Program-defined widget classes that draw themselves (overriding
`expose_event`, `size_request` or `draw`) get a canvas: their `realize` runs
before the first show (its `gdk_window_new` + `gdk_window_set_user_data`
binds the program's GdkWindow to the canvas), `size_request` is the size
hint, and GTK's event signals call the class methods. Canvases keep an
X-style backing store: a real exposure clears to the window background
(colour or `gdk_window_set_back_pixmap`) and emits `expose-event`; drawing
outside expose handlers lands in the backing image and is shown right after.

GDK 1 fonts are X font names mapped to system fonts and drawn with Pango;
XPM data and files load through Qt; masks become widget masks (shaped
windows); GdkImage and GdkRGB are 32-bit images.

## Linking

`libgtk-made-qt.a` with hidden visibility. `lib/libgtk-x11-2.0.so`,
`libgtk-3.so`, `libgtk-4.so`, `libgtk.so` ... are GNU ld scripts naming the
archive and Qt, so `-lgtk-x11-2.0` works unchanged. A finished program
exports no GTK symbol (even with `--export-dynamic`, which GTK's pkg-config
files add), so a real GTK loaded into the process by something else (Qt's
GTK platform theme, an input method) cannot meet ours; the library also
tells Qt not to use the GTK platform theme.

A static library cannot serve a program whose shared libraries or plugins
call GTK too: each would get its own copy. The objects are therefore built
a second time with the API exported, as `libgtk-made-qt.so.0`;
`<libdir>/gtk-made-qt/shared` has pkg-config files, `gtk-config` and
`lib<name>.so` links for it (`gtk-made-qt-env --shared`).
