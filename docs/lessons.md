# Lessons learned

Things that cost time once; check here before debugging something similar.

## GTK semantics that matter to real programs

- Text buffers: programs connect to `insert-text` (before and after),
  `delete-range` and `begin/end-user-action` for undo, auto-indent and
  highlighting. All edits - including typing in the view - must go through
  these signals, and the default handler must revalidate the iters.
- Programs emit `begin-user-action`, `cut-clipboard` ... themselves with
  `g_signal_emit_by_name`: these need class handlers that do the work.
- GtkEditable `insert-text` handlers veto input with
  `g_signal_stop_emission_by_name`; Leafpad's Find button depends on it.
- Window accelerators win over the focused text widget in GTK 2 (except
  plain typing): the text view ignores ShortcutOverride for modified keys.
- `gtk_widget_add_accelerator` adds a second accelerator; it must not
  replace the first.
- GTK focuses the first field of a dialog in geometric order; Qt focuses the
  first widget created - for `gtk_dialog_new_with_buttons` that is a button.
- `gtk_spin_button_get_value` after typing: commit the text first.
- `gtk-xft-dpi` is DPI * 1024; programs divide by it.
- Print contexts work in points (72 dpi) with the origin at the margin;
  programs draw headers at negative y.

## GTK 1.2

- GTK 1 code stores types in `guint` (`static guint my_type`): a 64-bit
  GType is cut in half. Recover it from the low 32 bits of the registered
  types (`gtkqt_type_fix`).
- Custom widgets create their GdkWindow in `realize` and bind it with
  `gdk_window_set_user_data`: realize must run before the first expose.
- A GTK size request is a minimum: widgets need `minimumSizeHint`, or a
  table column of them collapses to nothing.
- GTK's pkg-config files pull in `--export-dynamic` (gmodule-export): with
  default visibility a static program would export the whole API. Only the
  shared build exports it.
- Programs with plugins calling GTK (XMMS) need the shared library: one
  toolkit, one GType registry, one QApplication.
- Cairo leaves the top byte of RGB24 pixels undefined, and pixmaps loaded
  from XPM have 0 there. Qt's `Format_RGB32` copies that byte as alpha, so
  a skin drawn through it came out fully transparent (the test pattern,
  drawn with cairo fills, had 0xff and worked). Convert with
  `gtkqt_surface_image`.
- A childless window with an empty QVBoxLayout and `SetFixedSize` is 0x0:
  windows programs draw on take their size request instead.
- Handlers destroy widgets in the middle of an emission (XMMS's
  configure-event handler destroys its probe window): keep a reference
  during event emission and show/hide, as GTK does.
- `gtk_check_menu_item_set_active` activates the item: XMMS shows its
  equalizer and playlist through item factory callbacks that way.
- GTK 1's `GtkMenuPositionFunc` has no `push_in`; calling it the GTK 2 way
  passes `&push_in` as the user data.
- Programs reach into composite widgets: `GTK_BIN (button)->child` is the
  label, `GTK_BIN (option_menu)->child` the chosen item's label,
  `combo->list` a GtkList they fill, `filesel->file_list` a GtkCList.
  These must be real GTK widgets, even where Qt draws the result.
- GtkCList's `select_row` carries the button event; a double click emits
  it again with `GDK_2BUTTON_PRESS` ("open this").
- An explicit window position (`gtk_widget_set_uposition`) wins over
  centering on the transient parent.
- A GTK notebook asks for the width of all its tabs unless scrollable.

## Xlib

- Never let Xlib functions be visible to Qt's own libX11 use: they live in
  a separate hidden-visibility archive (lesson from Motif on Qt, where a
  shared libX11 replacement intercepted Qt's xcb platform).
- X's predefined atoms have fixed numbers (`XA_WM_NAME` is 39): GDK's atom
  table starts with them, and GDK's CLIPBOARD (69) is the first free one.
- Setting `WM_NAME` with Xlib does not change `gtk_window_get_title`, on real
  GTK either.
- In C, `XWindowAttributes` and `Visual` have `class`; `c_class` is the C++
  spelling.

## Implementation pitfalls

- Static functions in an `extern "C"` block have C linkage: their forward
  declarations must be in one too.
- Variables declared in the private header must not be defined inside
  `extern "C"` (different linkage, undefined reference).
- QFileDialog/QFontDialog draw everything themselves: mark them with the
  `gtkqt-self-drawn` property so GtkDialog does not add its vbox, and keep the
  action buttons alive (programs use the returned pointers).
- QTextEdit's own context-menu Cut and drag-move bypass the buffer signals:
  the view supplies its own menu.

## Build and link

- `pkg-config` files must list pango/pangocairo: programs call Pango
  directly and GTK 2's headers pulled them in.
- Programs include `gtk/gtktextiter.h` and other sub-headers directly.
- Makefiles that do not depend on the archive are not relinked when the
  library changes: delete the program first.
