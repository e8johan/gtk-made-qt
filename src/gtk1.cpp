/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GTK 1.2 API: the GtkType/GtkSignal/GtkArg layer on GObject, GDK 1
 * windows, fonts, cursors, XPM pixmaps, bitmaps, GdkImage, GdkRGB, input
 * sources, and small GTK 1 calls.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QBitmap>
#include <QScreen>
#include <gobject/gvaluecollector.h>

#include <clocale>
#include <cwchar>
#include <cwctype>

/* ======================================================================
 * Types
 * ====================================================================== */
/* A GType cut to 32 bits by a guint variable: find the type with those
 * low bits (type ids of registered types are pointers). */
static void collect_types (GType t, QHash<quint32, GType> &h)
{
  h.insert ((quint32) t, t);
  guint n = 0;
  GType *kids = g_type_children (t, &n);
  for (guint i = 0; i < n; i++) collect_types (kids[i], h);
  g_free (kids);
}

extern "C" {

GType gtkqt_type_fix (GType t)
{
  if (t <= G_TYPE_FUNDAMENTAL_MAX || (t >> 32) != 0 || sizeof (GType) <= 4) return t;
  static QHash<quint32, GType> types;
  auto it = types.constFind ((quint32) t);
  if (it != types.constEnd ()) return *it;
  types.clear ();
  for (GType f : { G_TYPE_OBJECT, G_TYPE_INTERFACE, G_TYPE_BOXED, G_TYPE_ENUM, G_TYPE_FLAGS, G_TYPE_POINTER, G_TYPE_PARAM })
    collect_types (f, types);
  return types.value ((quint32) t, t);
}

gpointer gtkqt_check_cast (gpointer instance, GType type)
{
  return instance ? (gpointer) g_type_check_instance_cast ((GTypeInstance *) instance, gtkqt_type_fix (type)) : nullptr;
}
gpointer gtkqt_check_class_cast (gpointer klass, GType type)
{
  return klass ? (gpointer) g_type_check_class_cast ((GTypeClass *) klass, gtkqt_type_fix (type)) : nullptr;
}

GtkType gtk_type_unique (GtkType parent_type, const GtkTypeInfo *info)
{
  parent_type = gtkqt_type_fix (parent_type);
  if (!info || !info->type_name) return G_TYPE_INVALID;
  if (GType existing = g_type_from_name (info->type_name)) return existing;
  GTypeInfo ti = {};
  ti.class_size = info->class_size;
  ti.base_init = (GBaseInitFunc) info->base_class_init_func;
  ti.class_init = (GClassInitFunc) info->class_init_func;
  ti.instance_size = info->object_size;
  ti.instance_init = (GInstanceInitFunc) info->object_init_func;
  return g_type_register_static (parent_type, info->type_name, &ti, (GTypeFlags) 0);
}

gpointer gtk_type_class (GtkType type) { return g_type_class_ref (gtkqt_type_fix (type)); }
gpointer gtk_type_new (GtkType type) { return g_object_new (gtkqt_type_fix (type), nullptr); }
const gchar *gtk_type_name (GtkType type) { return g_type_name (gtkqt_type_fix (type)); }
GtkType gtk_type_from_name (const gchar *name) { return g_type_from_name (name); }
GtkType gtk_type_parent (GtkType type) { return g_type_parent (gtkqt_type_fix (type)); }
gboolean gtk_type_is_a (GtkType type, GtkType is_a_type) { return g_type_is_a (gtkqt_type_fix (type), gtkqt_type_fix (is_a_type)); }
void gtk_type_init (void) {}

/* ======================================================================
 * Signals
 * ====================================================================== */
guint gtk_signal_newv (const gchar *name, GtkSignalRunType flags, GtkType object_type, guint function_offset,
                       GtkSignalMarshaller, GtkType return_val, guint n_args, GtkType *args)
{
  object_type = gtkqt_type_fix (object_type);
  return_val = gtkqt_type_fix (return_val);
  QVector<GType> params (n_args);
  for (guint i = 0; i < n_args; i++) {
    GType t = gtkqt_type_fix (args[i]);
    /* GTK 1 passed boxed values (events ...) by pointer: never copy them */
    if (G_TYPE_FUNDAMENTAL (t) == G_TYPE_BOXED) t |= G_SIGNAL_TYPE_STATIC_SCOPE;
    params[i] = t;
  }
  GClosure *closure = function_offset ? g_signal_type_cclosure_new (object_type, function_offset) : nullptr;
  return g_signal_newv (name, object_type, (GSignalFlags) flags, closure, nullptr, nullptr, nullptr, return_val, n_args,
                        n_args ? params.data () : nullptr);
}

guint gtk_signal_new (const gchar *name, GtkSignalRunType flags, GtkType object_type, guint function_offset,
                      GtkSignalMarshaller marshaller, GtkType return_val, guint n_args, ...)
{
  QVector<GtkType> args (n_args);
  va_list ap;
  va_start (ap, n_args);
  for (guint i = 0; i < n_args; i++) args[i] = va_arg (ap, GtkType);
  va_end (ap);
  return gtk_signal_newv (name, flags, object_type, function_offset, marshaller, return_val, n_args, args.data ());
}

void gtk_signal_emit (GtkObject *object, guint signal_id, ...)
{
  va_list ap;
  va_start (ap, signal_id);
  g_signal_emit_valist (object, signal_id, 0, ap);
  va_end (ap);
}

guint gtk_signal_lookup (const gchar *name, GtkType object_type) { return g_signal_lookup (name, gtkqt_type_fix (object_type)); }
const gchar *gtk_signal_name (guint signal_id) { return g_signal_name (signal_id); }
void gtk_signal_emit_stop (GtkObject *object, guint signal_id) { g_signal_stop_emission (object, signal_id, 0); }
void gtk_object_class_add_signals (GtkObjectClass *, guint *, guint) {}

gulong gtk_signal_connect_full (GtkObject *object, const gchar *name, GCallback func, GtkCallbackMarshal, gpointer data,
                                GtkDestroyNotify destroy_func, gint object_signal, gint after)
{
  int flags = (object_signal ? G_CONNECT_SWAPPED : 0) | (after ? G_CONNECT_AFTER : 0);
  return g_signal_connect_data (object, name, func, data, (GClosureNotify) (void (*) (void)) destroy_func, (GConnectFlags) flags);
}

struct AliveData { GObject *object; gulong id; };
static void alive_died (gpointer p, GObject *)
{
  auto *d = static_cast<AliveData *> (p);
  if (d->object && g_signal_handler_is_connected (d->object, d->id)) g_signal_handler_disconnect (d->object, d->id);
  if (d->object) g_object_remove_weak_pointer (d->object, (gpointer *) &d->object);
  delete d;
}

void gtk_signal_connect_while_alive (GtkObject *object, const gchar *name, GCallback func, gpointer data, GtkObject *alive)
{
  auto *d = new AliveData { G_OBJECT (object), g_signal_connect (object, name, func, data) };
  g_object_add_weak_pointer (d->object, (gpointer *) &d->object);
  g_object_weak_ref (G_OBJECT (alive), alive_died, d);
}

void gtk_signal_connect_object_while_alive (GtkObject *object, const gchar *name, GCallback func, GtkObject *alive)
{
  g_signal_connect_object (object, name, func, alive, G_CONNECT_SWAPPED);
}

void gtk_signal_handlers_destroy (GtkObject *object) { g_signal_handlers_destroy (object); }

} /* extern "C" */

/* ======================================================================
 * GtkArg: "GtkWidget::width", "signal::clicked" ... on GObject properties
 * ====================================================================== */
static bool set_args (GObject *o, const gchar *first, va_list ap)
{
  for (const gchar *name = first; name; name = va_arg (ap, const gchar *)) {
    QByteArray n (name);
    if (n.startsWith ("signal::") || n.startsWith ("signal_after::") || n.startsWith ("object_signal::") ||
        n.startsWith ("object_signal_after::")) {
      QByteArray sig = n.mid (n.indexOf ("::") + 2);
      GCallback func = va_arg (ap, GCallback);
      gpointer data = va_arg (ap, gpointer);
      int flags = (n.startsWith ("object") ? G_CONNECT_SWAPPED : 0) | (n.contains ("after") ? G_CONNECT_AFTER : 0);
      g_signal_connect_data (o, sig.constData (), func, data, nullptr, (GConnectFlags) flags);
      continue;
    }
    int sep = n.indexOf ("::");
    if (sep >= 0) n = n.mid (sep + 2);
    n.replace ('_', '-');
    if (n == "user-data") { g_object_set_data (o, "user_data", va_arg (ap, gpointer)); continue; }
    if (GTK_IS_WIDGET (o)) {
      GtkWidget *w = GTK_WIDGET (o);
      if (n == "x" || n == "y") {
        int v = va_arg (ap, int);
        QWidget *qw = gtkqt_qw (w);
        if (qw) qw->move (n == "x" ? v : qw->x (), n == "y" ? v : qw->y ());
        continue;
      }
      if (n == "width") n = "width-request";
      else if (n == "height") n = "height-request";
      else if (n == "parent") {
        auto *parent = (GtkWidget *) va_arg (ap, gpointer);
        if (parent) gtk_container_add (GTK_CONTAINER (parent), w);
        continue;
      }
    }
    GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (o), n.constData ());
    if (!pspec) {
      g_warning ("gtk-made-qt: %s has no argument '%s'", G_OBJECT_TYPE_NAME (o), name);
      return false;   /* the value's type is unknown: the rest cannot be read */
    }
    GValue v = G_VALUE_INIT;
    gchar *err = nullptr;
    G_VALUE_COLLECT_INIT (&v, pspec->value_type, ap, 0, &err);
    if (err) { g_warning ("%s", err); g_free (err); return false; }
    g_object_set_property (o, n.constData (), &v);
    g_value_unset (&v);
  }
  return true;
}

extern "C" {

void gtk_widget_set (GtkWidget *widget, const gchar *first, ...)
{
  va_list ap;
  va_start (ap, first);
  set_args (G_OBJECT (widget), first, ap);
  va_end (ap);
}

void gtk_object_set (GtkObject *object, const gchar *first, ...)
{
  va_list ap;
  va_start (ap, first);
  set_args (G_OBJECT (object), first, ap);
  va_end (ap);
}

void gtk_object_get (GtkObject *object, const gchar *first, ...)
{
  va_list ap;
  va_start (ap, first);
  for (const gchar *name = first; name; name = va_arg (ap, const gchar *)) {
    QByteArray n (name);
    int sep = n.indexOf ("::");
    if (sep >= 0) n = n.mid (sep + 2);
    n.replace ('_', '-');
    GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), n.constData ());
    if (!pspec) break;
    GValue v = G_VALUE_INIT;
    g_value_init (&v, pspec->value_type);
    g_object_get_property (G_OBJECT (object), n.constData (), &v);
    gchar *err = nullptr;
    G_VALUE_LCOPY (&v, ap, 0, &err);
    g_value_unset (&v);
    if (err) { g_free (err); break; }
  }
  va_end (ap);
}

static QByteArray arg_name (const gchar *name)
{
  QByteArray n (name);
  int sep = n.indexOf ("::");
  if (sep >= 0) n = n.mid (sep + 2);
  return n.replace ('_', '-');
}

void gtk_object_arg_get (GtkObject *object, GtkArg *arg, gpointer)
{
  QByteArray n = arg_name (arg->name);
  memset (&arg->d, 0, sizeof arg->d);
  arg->type = G_TYPE_INVALID;
  if (n == "parent" && GTK_IS_WIDGET (object)) { arg->type = GTK_TYPE_WIDGET; arg->d.object_data = (GtkObject *) GTK_WIDGET (object)->parent; return; }
  if (n == "user-data") { arg->type = G_TYPE_POINTER; arg->d.pointer_data = g_object_get_data (G_OBJECT (object), "user_data"); return; }
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), n.constData ());
  if (!pspec) return;
  GValue v = G_VALUE_INIT;
  g_value_init (&v, pspec->value_type);
  g_object_get_property (G_OBJECT (object), n.constData (), &v);
  arg->type = pspec->value_type;
  switch (G_TYPE_FUNDAMENTAL (pspec->value_type)) {
  case G_TYPE_BOOLEAN: arg->d.bool_data = g_value_get_boolean (&v); break;
  case G_TYPE_INT: arg->d.int_data = g_value_get_int (&v); break;
  case G_TYPE_UINT: arg->d.uint_data = g_value_get_uint (&v); break;
  case G_TYPE_LONG: arg->d.long_data = g_value_get_long (&v); break;
  case G_TYPE_ULONG: arg->d.ulong_data = g_value_get_ulong (&v); break;
  case G_TYPE_FLOAT: arg->d.float_data = g_value_get_float (&v); break;
  case G_TYPE_DOUBLE: arg->d.double_data = g_value_get_double (&v); break;
  case G_TYPE_ENUM: arg->d.int_data = g_value_get_enum (&v); break;
  case G_TYPE_FLAGS: arg->d.uint_data = g_value_get_flags (&v); break;
  case G_TYPE_STRING: arg->d.string_data = g_value_dup_string (&v); break;
  case G_TYPE_OBJECT: arg->d.object_data = (GtkObject *) g_value_get_object (&v); break;
  case G_TYPE_POINTER: arg->d.pointer_data = g_value_get_pointer (&v); break;
  default: break;
  }
  g_value_unset (&v);
}

void gtk_object_arg_set (GtkObject *object, GtkArg *arg, gpointer)
{
  QByteArray n = arg_name (arg->name);
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), n.constData ());
  if (!pspec) return;
  GValue v = G_VALUE_INIT;
  g_value_init (&v, pspec->value_type);
  switch (G_TYPE_FUNDAMENTAL (pspec->value_type)) {
  case G_TYPE_BOOLEAN: g_value_set_boolean (&v, arg->d.bool_data); break;
  case G_TYPE_INT: g_value_set_int (&v, arg->d.int_data); break;
  case G_TYPE_UINT: g_value_set_uint (&v, arg->d.uint_data); break;
  case G_TYPE_FLOAT: g_value_set_float (&v, arg->d.float_data); break;
  case G_TYPE_DOUBLE: g_value_set_double (&v, arg->d.double_data); break;
  case G_TYPE_ENUM: g_value_set_enum (&v, arg->d.int_data); break;
  case G_TYPE_STRING: g_value_set_string (&v, arg->d.string_data); break;
  case G_TYPE_OBJECT: g_value_set_object (&v, arg->d.object_data); break;
  default: g_value_unset (&v); return;
  }
  g_object_set_property (G_OBJECT (object), n.constData (), &v);
  g_value_unset (&v);
}

GtkObject *gtk_object_new (GtkType type, const gchar *first, ...)
{
  GObject *o = (GObject *) g_object_new (gtkqt_type_fix (type), nullptr);
  va_list ap;
  va_start (ap, first);
  set_args (o, first, ap);
  va_end (ap);
  return (GtkObject *) o;
}

GtkWidget *gtk_widget_new (GtkType type, const gchar *first, ...)
{
  GObject *o = (GObject *) g_object_new (gtkqt_type_fix (type), nullptr);
  va_list ap;
  va_start (ap, first);
  set_args (o, first, ap);
  va_end (ap);
  return GTK_WIDGET (o);
}

/* ======================================================================
 * Small GTK 1 calls
 * ====================================================================== */
gchar *gtk_set_locale (void) { return setlocale (LC_ALL, ""); }
gchar *gdk_set_locale (void) { return setlocale (LC_ALL, ""); }
void gtk_rc_init (void) {}
void gtk_widget_reparent (GtkWidget *widget, GtkWidget *new_parent)
{
  g_object_ref (widget);
  if (widget->parent) gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
  gtk_container_add (GTK_CONTAINER (new_parent), widget);
  g_object_unref (widget);
}
void gtk_window_position (GtkWindow *window, GtkWindowPosition position) { gtk_window_set_position (window, position); }
void gtk_paned_set_handle_size (GtkPaned *, guint16) {}
void gtk_paned_set_gutter_size (GtkPaned *, guint16) {}
GdkFont *gtk_font_selection_dialog_get_font (GtkFontSelectionDialog *fsd)
{
  gchar *name = gtk_font_selection_dialog_get_font_name (fsd);
  PangoFontDescription *d = pango_font_description_from_string (name ? name : "Sans 10");
  GdkFont *f = gdk_font_from_description (d);
  pango_font_description_free (d);
  g_free (name);
  return f;
}
GList *gtk_container_children (GtkContainer *container) { return gtk_container_get_children (container); }
void gtk_exit (gint error_code) { exit (error_code); }

void gtk_widget_draw (GtkWidget *widget, const GdkRectangle *area)
{
  QWidget *qw = gtkqt_qw (widget);
  if (!qw || !qw->isVisible ()) return;
  if (area) qw->repaint (area->x, area->y, area->width, area->height);
  else qw->repaint ();
}
void gtk_widget_draw_focus (GtkWidget *widget) { gtk_widget_queue_draw (widget); }
void gtk_widget_draw_default (GtkWidget *widget) { gtk_widget_queue_draw (widget); }

void gtk_widget_set_uposition (GtkWidget *widget, gint x, gint y)
{
  QWidget *qw = gtkqt_qw (widget);
  if (!qw) return;
  if (x == -1) x = qw->x ();
  if (y == -1) y = qw->y ();
  if (x == -2 || y == -2) return;
  qw->move (x, y);
  if (GTK_IS_WINDOW (widget)) g_object_set_data (G_OBJECT (widget), "gtkqt-positioned", GINT_TO_POINTER (1));
}

static QRegion mask_region (GdkBitmap *mask, int ox, int oy)
{
  if (!mask || !mask->surface) return QRegion ();
  cairo_surface_t *s = mask->surface;
  cairo_surface_flush (s);
  int w = cairo_image_surface_get_width (s), h = cairo_image_surface_get_height (s), stride = cairo_image_surface_get_stride (s);
  const uchar *data = cairo_image_surface_get_data (s);
  bool a8 = cairo_image_surface_get_format (s) == CAIRO_FORMAT_A8;
  QBitmap bm (w, h);
  bm.fill (Qt::color0);
  QImage img (w, h, QImage::Format_Mono);
  img.fill (0);
  for (int y = 0; y < h; y++) {
    const uchar *row = data + y * stride;
    for (int x = 0; x < w; x++) {
      bool on = a8 ? row[x] != 0 : (reinterpret_cast<const quint32 *> (row)[x] & 0xffffff) != 0;
      if (on) img.setPixel (x, y, 1);
    }
  }
  return QRegion (QBitmap::fromImage (img)).translated (ox, oy);
}

void gtk_widget_shape_combine_mask (GtkWidget *widget, GdkBitmap *mask, gint ox, gint oy)
{
  QWidget *qw = gtkqt_qw (widget);
  if (!qw) return;
  if (!mask) qw->clearMask ();
  else qw->setMask (mask_region (mask, ox, oy));
}

gboolean gtk_widget_event (GtkWidget *widget, GdkEvent *event)
{
  const char *sig = nullptr;
  switch (event->type) {
  case GDK_BUTTON_PRESS: case GDK_2BUTTON_PRESS: case GDK_3BUTTON_PRESS: sig = "button-press-event"; break;
  case GDK_BUTTON_RELEASE: sig = "button-release-event"; break;
  case GDK_MOTION_NOTIFY: sig = "motion-notify-event"; break;
  case GDK_KEY_PRESS: sig = "key-press-event"; break;
  case GDK_KEY_RELEASE: sig = "key-release-event"; break;
  case GDK_ENTER_NOTIFY: sig = "enter-notify-event"; break;
  case GDK_LEAVE_NOTIFY: sig = "leave-notify-event"; break;
  case GDK_EXPOSE: sig = "expose-event"; break;
  case GDK_CONFIGURE: sig = "configure-event"; break;
  case GDK_DELETE: sig = "delete-event"; break;
  case GDK_FOCUS_CHANGE: sig = event->focus_change.in ? "focus-in-event" : "focus-out-event"; break;
  case GDK_SCROLL: sig = "scroll-event"; break;
  default: return FALSE;
  }
  return gtkqt_emit_event (widget, event, sig);
}

void gtk_widget_set_default_colormap (GdkColormap *) {}
void gtk_widget_set_default_visual (GdkVisual *) {}
void gtk_widget_push_colormap (GdkColormap *) {}
void gtk_widget_pop_colormap (void) {}
void gtk_widget_push_visual (GdkVisual *) {}
void gtk_widget_pop_visual (void) {}
void gtk_widget_push_style (GtkStyle *) {}
void gtk_widget_pop_style (void) {}
GdkColormap *gtk_widget_get_colormap (GtkWidget *) { return gdk_colormap_get_system (); }
GdkVisual *gtk_widget_get_visual (GtkWidget *) { return gdk_visual_get_system (); }
void gtk_widget_set_rc_style (GtkWidget *) {}
void gtk_widget_restore_default_style (GtkWidget *) {}

void gtk_window_set_policy (GtkWindow *window, gint, gint allow_grow, gint)
{
  gtk_window_set_resizable (window, allow_grow);
}

void gtk_window_set_geometry_hints (GtkWindow *window, GtkWidget *, GdkGeometry *g, GdkWindowHints mask)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window)); qw && g) {
    if (mask & GDK_HINT_MIN_SIZE) qw->setMinimumSize (qMax (0, g->min_width), qMax (0, g->min_height));
    if (mask & GDK_HINT_MAX_SIZE) qw->setMaximumSize (g->max_width > 0 ? g->max_width : QWIDGETSIZE_MAX, g->max_height > 0 ? g->max_height : QWIDGETSIZE_MAX);
    if (mask & GDK_HINT_RESIZE_INC) qw->setSizeIncrement (g->width_inc, g->height_inc);
    if (mask & GDK_HINT_BASE_SIZE) qw->setBaseSize (g->base_width, g->base_height);
  }
}

void gtk_label_get (GtkLabel *label, gchar **str) { if (str) *str = (gchar *) gtk_label_get_text (label); }
void gtk_label_set (GtkLabel *label, const gchar *str) { gtk_label_set_text (label, str); }
guint gtk_label_parse_uline (GtkLabel *label, const gchar *string)
{
  gtk_label_set_text_with_mnemonic (label, string);
  const char *u = string ? strchr (string, '_') : nullptr;
  return u && u[1] ? gdk_keyval_to_lower ((guchar) u[1]) : GDK_KEY_VoidSymbol;
}
GSList *gtk_radio_button_group (GtkRadioButton *rb) { return gtk_radio_button_get_group (rb); }
GSList *gtk_radio_menu_item_group (GtkRadioMenuItem *item) { return gtk_radio_menu_item_get_group (item); }
void gtk_notebook_set_page (GtkNotebook *notebook, gint page) { gtk_notebook_set_current_page (notebook, page); }

GtkWidget *gtk_menu_get_active (GtkMenu *menu)
{
  auto *w = (GtkWidget *) g_object_get_data (G_OBJECT (menu), "gtkqt-menu-active");
  if (!w) {
    GList *l = gtk_container_get_children (GTK_CONTAINER (menu));
    w = l ? (GtkWidget *) l->data : nullptr;
    g_list_free (l);
  }
  return w;
}
void gtk_menu_set_active (GtkMenu *menu, guint index)
{
  GList *l = gtk_container_get_children (GTK_CONTAINER (menu));
  g_object_set_data (G_OBJECT (menu), "gtkqt-menu-active", g_list_nth_data (l, index));
  g_list_free (l);
}
void gtk_option_menu_remove_menu (GtkOptionMenu *option_menu) { (void) option_menu; }
void gtk_button_box_set_spacing (GtkButtonBox *widget, gint spacing) { gtk_box_set_spacing (GTK_BOX (widget), spacing); }
void gtk_button_box_set_child_size (GtkButtonBox *, gint, gint) {}
void gtk_button_box_set_child_ipadding (GtkButtonBox *, gint, gint) {}
void gtk_item_factory_parse_rc (const gchar *) {}
void gtk_item_factory_dump_rc (const gchar *, gpointer, gboolean) {}
void gtk_item_factory_add_foreign (GtkWidget *accel_widget, const gchar *, GtkAccelGroup *group, guint key, GdkModifierType mods)
{
  if (key) gtk_widget_add_accelerator (accel_widget, "activate", group, key, mods, GTK_ACCEL_VISIBLE);
}
void gtk_combo_set_use_arrows_always (GtkCombo *, gboolean) {}
void gtk_spin_button_set_shadow_type (GtkSpinButton *, GtkShadowType) {}
void gtk_spin_button_set_snap_to_ticks (GtkSpinButton *, gboolean) {}
gfloat gtk_spin_button_get_value_as_float (GtkSpinButton *spin) { return (gfloat) gtk_spin_button_get_value (spin); }
void gtk_container_set_resize_mode (GtkContainer *, gint) {}
void gtk_container_set_focus_vadjustment (GtkContainer *, GtkAdjustment *) {}
void gtk_container_set_focus_hadjustment (GtkContainer *, GtkAdjustment *) {}
void gtk_grab_add (GtkWidget *) {}
void gtk_grab_remove (GtkWidget *) {}
/* Selections: an owner's text goes to Qt's clipboard (PRIMARY to the X
 * selection where Qt has one) when it claims the selection; it loses it,
 * with a "selection-clear-event", when another owner or program claims it. */
static QHash<quintptr, GtkWidget *> selection_owners;
static bool selection_exporting = false;

static QClipboard::Mode selection_mode (GdkAtom selection)
{
  QClipboard *cb = QGuiApplication::clipboard ();
  return selection == GDK_SELECTION_PRIMARY && cb->supportsSelection () ? QClipboard::Selection : QClipboard::Clipboard;
}

static void selection_owner_gone (gpointer key, GObject *)
{
  selection_owners.remove ((quintptr) key);
}

static void selection_clear (GtkWidget *owner, GdkAtom selection)
{
  g_object_weak_unref (G_OBJECT (owner), selection_owner_gone, (gpointer) (quintptr) selection);
  selection_owners.remove ((quintptr) selection);
  if (!owner->priv || owner->priv->destroyed) return;
  GdkEvent ev = {};
  ev.selection.type = GDK_SELECTION_CLEAR;
  ev.selection.window = owner->window;
  ev.selection.selection = selection;
  ev.selection.time = GDK_CURRENT_TIME;
  gtkqt_emit_event (owner, &ev, "selection-clear-event");
}

GdkWindow *gdk_selection_owner_get (GdkAtom selection)
{
  GtkWidget *w = selection_owners.value ((quintptr) selection);
  return w ? w->window : nullptr;
}

void gtk_selection_add_target (GtkWidget *widget, GdkAtom selection, GdkAtom target, guint info)
{
  if (!widget) return;
  QByteArray key = "gtkqt-sel-info-" + QByteArray::number ((quintptr) selection) + "-" + QByteArray::number ((quintptr) target);
  g_object_set_data (G_OBJECT (widget), key.constData (), GUINT_TO_POINTER (info));
}

gboolean gtk_selection_owner_set (GtkWidget *widget, GdkAtom selection, guint32 time_)
{
  GtkWidget *old = selection_owners.value ((quintptr) selection);
  if (old == widget) return TRUE;
  if (old) selection_clear (old, selection);
  if (!widget) return TRUE;
  selection_owners.insert ((quintptr) selection, widget);
  g_object_weak_ref (G_OBJECT (widget), selection_owner_gone, (gpointer) (quintptr) selection);
  /* ask the owner for its text now: Qt's clipboard holds data, not a callback */
  GdkAtom target = GDK_TARGET_STRING;
  QByteArray key = "gtkqt-sel-info-" + QByteArray::number ((quintptr) selection) + "-" + QByteArray::number ((quintptr) target);
  GtkSelectionData sd = {};
  sd.selection = selection;
  sd.target = target;
  sd.length = -1;
  g_signal_emit_by_name (widget, "selection-get", &sd, GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget), key.constData ())),
                         (guint) time_);
  if (sd.data && sd.length >= 0) {
    QClipboard *cb = QGuiApplication::clipboard ();
    static bool watching = false;
    if (!watching) {   /* someone else took the selection: our owner loses it */
      watching = true;
      QObject::connect (cb, &QClipboard::changed, cb, [] (QClipboard::Mode mode) {
        if (selection_exporting) return;
        for (GdkAtom s : { GDK_SELECTION_PRIMARY, GDK_SELECTION_CLIPBOARD })
          if (selection_mode (s) == mode)
            if (GtkWidget *o = selection_owners.value ((quintptr) s)) selection_clear (o, s);
      });
    }
    selection_exporting = true;
    cb->setText (QString::fromUtf8 ((const char *) sd.data, sd.length), selection_mode (selection));
    selection_exporting = false;
  }
  g_free (sd.data);
  return TRUE;
}
/* the selection's text from Qt's clipboard, delivered as "selection-received"
   from the main loop as on X; anything but text arrives with length -1 */
gboolean gtk_selection_convert (GtkWidget *widget, GdkAtom selection, GdkAtom target, guint32 time_)
{
  if (!widget) return FALSE;
  QClipboard *cb = QGuiApplication::clipboard ();
  QClipboard::Mode mode = selection_mode (selection);
  gchar *tname = gdk_atom_name (target);
  bool text = tname && (!strcmp (tname, "STRING") || !strcmp (tname, "UTF8_STRING") || !strcmp (tname, "TEXT") ||
                        !strcmp (tname, "COMPOUND_TEXT") || !strcmp (tname, "text/plain"));
  g_free (tname);
  QByteArray bytes = text ? cb->text (mode).toUtf8 () : QByteArray ();
  g_object_ref (widget);
  QTimer::singleShot (0, qApp, [widget, selection, target, time_, text, bytes] {
    if (widget->priv && !widget->priv->destroyed) {
      GtkSelectionData sd = {};
      sd.selection = selection;
      sd.target = target;
      sd.type = text && !bytes.isEmpty () ? GDK_SELECTION_TYPE_STRING : GDK_NONE;
      sd.format = 8;
      QByteArray copy = bytes;
      sd.data = text && !bytes.isEmpty () ? (guchar *) copy.data () : nullptr;   /* NUL-terminated, as GTK's */
      sd.length = text && !bytes.isEmpty () ? copy.size () : -1;
      g_signal_emit_by_name (widget, "selection-received", &sd, (guint) time_);
    }
    g_object_unref (widget);
  });
  return TRUE;
}

} /* extern "C" */

/* ======================================================================
 * GDK 1 windows
 * ====================================================================== */
static QWidget *win_qwidget (GdkWindow *window)
{
  if (!window) return nullptr;
  if (window->qw) return window->qw;
  return window->widget && window->widget->priv && !window->widget->priv->destroyed ? gtkqt_qw (window->widget) : nullptr;
}
/* the top-level QWidget when the window is a top-level window */
static QWidget *win_toplevel (GdkWindow *window)
{
  QWidget *qw = win_qwidget (window);
  return qw && qw->isWindow () ? qw : nullptr;
}

static void apply_event_mask (GtkWidget *w, gint mask)
{
  QWidget *qw = gtkqt_qw (w);
  if (qw && (mask & (GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK))) qw->setMouseTracking (true);
}

extern "C" {

GdkWindow *gdk_window_new (GdkWindow *, GdkWindowAttr *attr, gint mask)
{
  GdkWindow *win = gtkqt_drawable_new (GTKQT_DRAWABLE_WINDOW, nullptr);
  if (attr) {
    win->width = attr->width;
    win->height = attr->height;
    win->event_mask = attr->event_mask;
    (void) mask;
  }
  return win;
}

void gdk_window_destroy (GdkWindow *window)
{
  if (!window) return;
  if (window->widget && window->widget->window == window) return;   /* the widget owns it */
  g_object_unref (window);
}

void gdk_window_reparent (GdkWindow *window, GdkWindow *new_parent, gint x, gint y)
{
  QWidget *qw = win_qwidget (window), *np = new_parent ? win_qwidget (new_parent) : nullptr;
  if (!qw || qw == np) return;
  bool vis = qw->isVisible ();
  qw->setParent (np);
  qw->move (x, y);
  if (vis) qw->show ();
}

void gdk_window_set_user_data (GdkWindow *window, gpointer user_data)
{
  if (!window || window->kind != GTKQT_DRAWABLE_WINDOW) return;
  if (user_data && G_TYPE_CHECK_INSTANCE_TYPE (user_data, GTK_TYPE_WIDGET)) {
    window->widget = GTK_WIDGET (user_data);
    if (window->event_mask) apply_event_mask (window->widget, window->event_mask);
  }
}

void gdk_window_set_events (GdkWindow *window, gint mask)
{
  if (!window) return;
  window->event_mask = mask;
  if (window->widget) apply_event_mask (window->widget, mask);
}
gint gdk_window_get_events (GdkWindow *window) { return window ? window->event_mask : 0; }

void gdk_window_show (GdkWindow *window) { if (QWidget *qw = win_toplevel (window)) qw->show (); }
void gdk_window_hide (GdkWindow *window) { if (QWidget *qw = win_toplevel (window)) qw->hide (); }
void gdk_window_move (GdkWindow *window, gint x, gint y) { if (QWidget *qw = win_toplevel (window)) qw->move (x, y); }
void gdk_window_resize (GdkWindow *window, gint w, gint h) { if (QWidget *qw = win_toplevel (window)) qw->resize (qMax (1, w), qMax (1, h)); }
void gdk_window_move_resize (GdkWindow *window, gint x, gint y, gint w, gint h)
{
  if (QWidget *qw = win_toplevel (window)) { qw->move (x, y); qw->resize (qMax (1, w), qMax (1, h)); }
}

gboolean gdk_window_get_root_origin (GdkWindow *window, gint *x, gint *y)
{
  QWidget *qw = win_qwidget (window);
  QPoint p = qw ? qw->window ()->frameGeometry ().topLeft () : QPoint ();
  if (x) *x = p.x ();
  if (y) *y = p.y ();
  return qw != nullptr;
}
gboolean gdk_window_get_deskrelative_origin (GdkWindow *window, gint *x, gint *y) { return gdk_window_get_origin (window, x, y); }

void gdk_window_set_hints (GdkWindow *window, gint, gint, gint min_width, gint min_height, gint max_width, gint max_height, gint flags)
{
  QWidget *qw = win_toplevel (window);
  if (!qw) return;
  if (flags & GDK_HINT_MIN_SIZE) qw->setMinimumSize (min_width, min_height);
  if (flags & GDK_HINT_MAX_SIZE) qw->setMaximumSize (max_width, max_height);
}

void gdk_window_set_geometry_hints (GdkWindow *window, GdkGeometry *g, GdkWindowHints flags)
{
  if (!g) return;
  gdk_window_set_hints (window, 0, 0, g->min_width, g->min_height, g->max_width, g->max_height, flags);
}

static void set_window_flags (QWidget *qw, Qt::WindowFlags add, bool on)
{
  bool vis = qw->isVisible ();
  QPoint pos = qw->pos ();
  qw->setWindowFlags (on ? qw->windowFlags () | add : qw->windowFlags () & ~add);
  if (vis) { qw->move (pos); qw->show (); }
}

void gdk_window_set_decorations (GdkWindow *window, gint decorations)
{
  if (QWidget *qw = win_toplevel (window)) set_window_flags (qw, Qt::FramelessWindowHint, decorations == 0);
}
void gdk_window_set_functions (GdkWindow *, gint) {}
void gdk_window_set_override_redirect (GdkWindow *window, gboolean on)
{
  if (QWidget *qw = win_toplevel (window)) set_window_flags (qw, Qt::FramelessWindowHint | Qt::BypassWindowManagerHint, on);
}

void gdk_window_set_icon (GdkWindow *window, GdkWindow *, GdkPixmap *pixmap, GdkBitmap *mask)
{
  QWidget *qw = win_toplevel (window);
  if (!qw || !pixmap || !pixmap->surface) return;
  cairo_surface_t *s = pixmap->surface;
  QPixmap pm = QPixmap::fromImage (gtkqt_surface_image (s));
  if (mask) pm.setMask (QBitmap::fromImage (QImage ()));   /* mask applied below */
  if (mask && mask->surface) {
    QRegion r = mask_region (mask, 0, 0);
    QBitmap bm (pm.size ());
    bm.fill (Qt::color0);
    QPainter p (&bm);
    p.setClipRegion (r);
    p.fillRect (bm.rect (), Qt::color1);
    p.end ();
    pm.setMask (bm);
  }
  qw->setWindowIcon (QIcon (pm));
}
void gdk_window_set_icon_name (GdkWindow *window, const gchar *name) { if (QWidget *qw = win_toplevel (window)) qw->setWindowIconText (gtkqt_str (name)); }
void gdk_window_set_title (GdkWindow *window, const gchar *title) { if (QWidget *qw = win_toplevel (window)) qw->setWindowTitle (gtkqt_str (title)); }
void gdk_window_set_group (GdkWindow *, GdkWindow *) {}

void gdk_window_shape_combine_mask (GdkWindow *window, GdkBitmap *mask, gint x, gint y)
{
  QWidget *qw = win_qwidget (window);
  if (!qw) return;
  if (!mask) qw->clearMask ();
  else qw->setMask (mask_region (mask, x, y));
}

GdkWindow *gdk_window_get_parent (GdkWindow *window)
{
  if (window && window->widget && window->widget->parent) return gtk_widget_get_window (window->widget->parent);
  return gdk_get_default_root_window ();
}
GdkWindow *gdk_window_get_toplevel (GdkWindow *window)
{
  if (window && window->widget) return gtk_widget_get_window (gtk_widget_get_toplevel (window->widget));
  return window;
}
gboolean gdk_window_is_visible (GdkWindow *window) { QWidget *qw = win_qwidget (window); return qw && qw->isVisible (); }
gboolean gdk_window_is_viewable (GdkWindow *window) { return gdk_window_is_visible (window); }
void gdk_window_add_filter (GdkWindow *, gpointer, gpointer) {}

static void set_state (GdkWindow *window, Qt::WindowState st, bool on)
{
  QWidget *qw = win_qwidget (window);
  if (!qw) return;
  qw = qw->window ();
  qw->setWindowState (on ? qw->windowState () | st : qw->windowState () & ~st);
}
void gdk_window_iconify (GdkWindow *window) { set_state (window, Qt::WindowMinimized, true); }
void gdk_window_deiconify (GdkWindow *window) { set_state (window, Qt::WindowMinimized, false); }
void gdk_window_maximize (GdkWindow *window) { set_state (window, Qt::WindowMaximized, true); }
void gdk_window_unmaximize (GdkWindow *window) { set_state (window, Qt::WindowMaximized, false); }
void gdk_window_fullscreen (GdkWindow *window) { set_state (window, Qt::WindowFullScreen, true); }
void gdk_window_unfullscreen (GdkWindow *window) { set_state (window, Qt::WindowFullScreen, false); }
void gdk_window_stick (GdkWindow *) {}
void gdk_window_unstick (GdkWindow *) {}
void gdk_window_set_keep_above (GdkWindow *window, gboolean on)
{
  if (QWidget *qw = win_qwidget (window)) set_window_flags (qw->window (), Qt::WindowStaysOnTopHint, on);
}
void gdk_window_set_keep_below (GdkWindow *window, gboolean on)
{
  if (QWidget *qw = win_qwidget (window)) set_window_flags (qw->window (), Qt::WindowStaysOnBottomHint, on);
}
void gdk_window_set_skip_taskbar_hint (GdkWindow *, gboolean) {}
void gdk_window_set_skip_pager_hint (GdkWindow *, gboolean) {}
void gdk_window_focus (GdkWindow *window, guint32)
{
  if (QWidget *qw = win_qwidget (window)) { qw->window ()->raise (); qw->window ()->activateWindow (); }
}
/* the window manager moves or resizes the window (skinned windows drag themselves this way) */
void gdk_window_begin_move_drag (GdkWindow *window, gint, gint, gint, guint32)
{
  QWidget *qw = win_qwidget (window);
  if (qw && qw->window ()->windowHandle ()) qw->window ()->windowHandle ()->startSystemMove ();
}
void gdk_window_begin_resize_drag (GdkWindow *window, GdkWindowEdge edge, gint, gint, gint, guint32)
{
  QWidget *qw = win_qwidget (window);
  if (!qw || !qw->window ()->windowHandle ()) return;
  static const Qt::Edges edges[] = { Qt::TopEdge | Qt::LeftEdge, Qt::TopEdge, Qt::TopEdge | Qt::RightEdge, Qt::LeftEdge,
                                     Qt::RightEdge, Qt::BottomEdge | Qt::LeftEdge, Qt::BottomEdge, Qt::BottomEdge | Qt::RightEdge };
  qw->window ()->windowHandle ()->startSystemResize (edges[qBound (0, (int) edge, 7)]);
}
void gdk_window_remove_filter (GdkWindow *, gpointer, gpointer) {}
void gdk_add_client_message_filter (GdkAtom, gpointer, gpointer) {}
void gdk_property_change (GdkWindow *, GdkAtom, GdkAtom, gint, GdkPropMode, const guchar *, gint) {}
void gdk_set_sm_client_id (const gchar *) {}
void gdk_error_trap_push (void) {}
gint gdk_error_trap_pop (void) { return 0; }
gint gdk_pointer_grab (GdkWindow *, gboolean, gint, GdkWindow *, GdkCursor *, guint32) { return GDK_GRAB_SUCCESS; }
void gdk_pointer_ungrab (guint32) {}
gint gdk_keyboard_grab (GdkWindow *, gboolean, guint32) { return GDK_GRAB_SUCCESS; }
void gdk_keyboard_ungrab (guint32) {}
gboolean gdk_pointer_is_grabbed (void) { return FALSE; }
gint gdk_screen_width_mm (void) { gtkqt_ensure_app (); QScreen *s = QGuiApplication::primaryScreen (); return s ? (gint) s->physicalSize ().width () : 0; }
gint gdk_screen_height_mm (void) { gtkqt_ensure_app (); QScreen *s = QGuiApplication::primaryScreen (); return s ? (gint) s->physicalSize ().height () : 0; }
gchar *gdk_get_display (void) { const char *d = g_getenv ("DISPLAY"); return g_strdup (d ? d : ":0"); }

/* ======================================================================
 * Input sources
 * ====================================================================== */
struct InputData { GdkInputFunction func; gpointer data; GDestroyNotify destroy; int fd; };

static gboolean input_cb (GIOChannel *, GIOCondition cond, gpointer p)
{
  auto *d = static_cast<InputData *> (p);
  int c = 0;
  if (cond & (G_IO_IN | G_IO_HUP | G_IO_ERR)) c |= GDK_INPUT_READ;
  if (cond & G_IO_OUT) c |= GDK_INPUT_WRITE;
  if (cond & G_IO_PRI) c |= GDK_INPUT_EXCEPTION;
  d->func (d->data, d->fd, (GdkInputCondition) c);
  return TRUE;
}

gint gdk_input_add_full (gint source, GdkInputCondition condition, GdkInputFunction function, gpointer data, GDestroyNotify destroy)
{
  int cond = 0;
  if (condition & GDK_INPUT_READ) cond |= G_IO_IN | G_IO_HUP | G_IO_ERR;
  if (condition & GDK_INPUT_WRITE) cond |= G_IO_OUT | G_IO_ERR;
  if (condition & GDK_INPUT_EXCEPTION) cond |= G_IO_PRI;
  GIOChannel *ch = g_io_channel_unix_new (source);
  auto *d = new InputData { function, data, destroy, source };
  guint id = g_io_add_watch_full (ch, G_PRIORITY_DEFAULT, (GIOCondition) cond, input_cb, d, [] (gpointer p) {
    auto *dd = static_cast<InputData *> (p);
    if (dd->destroy) dd->destroy (dd->data);
    delete dd;
  });
  g_io_channel_unref (ch);
  return (gint) id;
}
gint gdk_input_add (gint source, GdkInputCondition condition, GdkInputFunction function, gpointer data)
{
  return gdk_input_add_full (source, condition, function, data, nullptr);
}
void gdk_input_remove (gint tag) { g_source_remove ((guint) tag); }

/* ======================================================================
 * Cursors
 * ====================================================================== */
} /* extern "C" */
struct _GdkCursor { QCursor cursor; bool blank; };
extern "C" {

GdkCursor *gdk_cursor_new (GdkCursorType type)
{
  gtkqt_ensure_app ();
  Qt::CursorShape shape = Qt::ArrowCursor;
  switch (type) {
  case GDK_CROSSHAIR: case GDK_TCROSS: case GDK_PLUS: shape = Qt::CrossCursor; break;
  case GDK_FLEUR: shape = Qt::SizeAllCursor; break;
  case GDK_HAND1: case GDK_HAND2: shape = Qt::PointingHandCursor; break;
  case GDK_QUESTION_ARROW: shape = Qt::WhatsThisCursor; break;
  case GDK_SB_H_DOUBLE_ARROW: shape = Qt::SizeHorCursor; break;
  case GDK_SB_V_DOUBLE_ARROW: shape = Qt::SizeVerCursor; break;
  case GDK_TOP_LEFT_CORNER: case GDK_BOTTOM_RIGHT_CORNER: case GDK_SIZING: shape = Qt::SizeFDiagCursor; break;
  case GDK_WATCH: shape = Qt::WaitCursor; break;
  case GDK_XTERM: shape = Qt::IBeamCursor; break;
  case GDK_CIRCLE: shape = Qt::ForbiddenCursor; break;
  case GDK_BLANK_CURSOR: shape = Qt::BlankCursor; break;
  default: break;
  }
  return new _GdkCursor { QCursor (shape), type == GDK_BLANK_CURSOR };
}
GdkCursor *gdk_cursor_new_from_pixmap (GdkPixmap *, GdkPixmap *, const GdkColor *, const GdkColor *, gint, gint)
{
  return gdk_cursor_new (GDK_LEFT_PTR);
}
void gdk_cursor_destroy (GdkCursor *cursor) { delete cursor; }
void gdk_window_set_cursor (GdkWindow *window, GdkCursor *cursor)
{
  QWidget *qw = win_qwidget (window);
  if (!qw) return;
  if (cursor) qw->setCursor (cursor->cursor); else qw->unsetCursor ();
}

} /* extern "C" */

/* ======================================================================
 * Fonts: X font names mapped to system fonts, drawn with Pango
 * ====================================================================== */
struct GtkQtFont : _GdkFont {
  int ref = 1;
  QFont font;
  PangoFontDescription *desc = nullptr;
};

static PangoContext *font_context ()
{
  static PangoContext *ctx = nullptr;
  if (!ctx) ctx = gdk_pango_context_get ();
  return ctx;
}

static QString font_text (const gchar *text, gint len)
{
  if (!text) return QString ();
  if (len < 0) len = strlen (text);
  /* GTK 1 strings are in the locale's encoding; most are UTF-8 today, old
   * files are often Latin-1 */
  if (g_utf8_validate (text, len, nullptr)) return QString::fromUtf8 (text, len);
  return QString::fromLocal8Bit (text, len).isEmpty () ? QString::fromLatin1 (text, len) : QString::fromLocal8Bit (text, len);
}

static PangoLayout *font_layout (GtkQtFont *f, const QString &s)
{
  PangoLayout *l = pango_layout_new (font_context ());
  pango_layout_set_font_description (l, f->desc);
  QByteArray u = s.toUtf8 ();
  pango_layout_set_text (l, u.constData (), u.size ());
  return l;
}

static GtkQtFont *font_from_qfont (const QFont &qf)
{
  auto *f = new GtkQtFont;
  f->type = GDK_FONT_FONT;
  f->font = qf;
  f->desc = gtkqt_pango_from_font (qf);
  PangoFontMetrics *m = pango_context_get_metrics (font_context (), f->desc, nullptr);
  f->ascent = PANGO_PIXELS (pango_font_metrics_get_ascent (m));
  f->descent = PANGO_PIXELS (pango_font_metrics_get_descent (m));
  pango_font_metrics_unref (m);
  return f;
}

/* "-adobe-helvetica-bold-r-normal--12-120-75-75-p-70-iso8859-1", "fixed", "9x15" */
static QFont xlfd_to_qfont (const QString &spec)
{
  gtkqt_ensure_app ();
  QFont f = QApplication::font ();
  QFont fixed = QFontDatabase::systemFont (QFontDatabase::FixedFont);
  QString name = spec.trimmed ();
  static const QRegularExpression cell ("^(\\d+)x(\\d+)(bold)?$");
  if (name == "fixed" || name == "cursor") { fixed.setPixelSize (13); return fixed; }
  if (auto m = cell.match (name); m.hasMatch ()) {
    fixed.setPixelSize (m.captured (2).toInt ());
    if (!m.captured (3).isEmpty ()) fixed.setBold (true);
    return fixed;
  }
  if (!name.startsWith ('-')) {
    PangoFontDescription *d = pango_font_description_from_string (name.toUtf8 ().constData ());
    QFont q = gtkqt_font_from_pango (d);
    pango_font_description_free (d);
    return q;
  }
  QStringList fld = name.split ('-');   /* fld[0] is empty */
  auto at = [&] (int i) { return i < fld.size () ? fld[i] : QStringLiteral ("*"); };
  QString family = at (2), weight = at (3).toLower (), slant = at (4).toLower (), spacing = at (11).toLower ();
  QString px = at (7), pt = at (8);
  if (family != "*" && !family.isEmpty ()) {
    static const QHash<QString, QString> map = {
      { "helvetica", "Sans Serif" }, { "arial", "Sans Serif" }, { "lucida", "Sans Serif" }, { "lucidatypewriter", "Monospace" },
      { "times", "Serif" }, { "new century schoolbook", "Serif" }, { "courier", "Monospace" }, { "fixed", "Monospace" },
      { "clean", "Monospace" }, { "terminal", "Monospace" }, { "misc", "Monospace" } };
    QString fam = map.value (family.toLower (), family);
    if (fam == "Monospace") f = fixed; else f.setFamilies ({ fam });
  } else if (spacing == "m" || spacing == "c") {
    f = fixed;
  }
  bool ok = false;
  int pxs = px.toInt (&ok);
  if (ok && pxs > 0) f.setPixelSize (pxs);
  else if (int pts = pt.toInt (&ok); ok && pts > 0) f.setPointSizeF (pts / 10.0);
  if (weight == "bold" || weight == "demibold" || weight == "black") f.setBold (true);
  else if (weight == "medium" || weight == "regular" || weight == "normal") f.setBold (false);
  if (slant == "i" || slant == "o") f.setItalic (true);
  return f;
}

extern "C" {

GdkFont *gdk_font_load (const gchar *name) { return font_from_qfont (xlfd_to_qfont (gtkqt_str (name))); }
GdkFont *gdk_fontset_load (const gchar *name)
{
  GdkFont *f = gdk_font_load (gtkqt_str (name).section (',', 0, 0).toUtf8 ().constData ());
  f->type = GDK_FONT_FONTSET;
  return f;
}
GdkFont *gdk_font_from_description (PangoFontDescription *desc) { return font_from_qfont (gtkqt_font_from_pango (desc)); }
GdkFont *gdk_font_ref (GdkFont *font) { if (font) static_cast<GtkQtFont *> (font)->ref++; return font; }
void gdk_font_unref (GdkFont *font)
{
  auto *f = static_cast<GtkQtFont *> (font);
  if (f && --f->ref == 0) { pango_font_description_free (f->desc); delete f; }
}
gint gdk_font_id (const GdkFont *font) { return (gint) (gintptr) font; }
gboolean gdk_font_equal (const GdkFont *a, const GdkFont *b)
{
  return a == b || (a && b && static_cast<const GtkQtFont *> (a)->font == static_cast<const GtkQtFont *> (b)->font);
}

gint gdk_text_width (GdkFont *font, const gchar *text, gint len)
{
  if (!font || !text) return 0;
  PangoLayout *l = font_layout (static_cast<GtkQtFont *> (font), font_text (text, len));
  int w;
  pango_layout_get_pixel_size (l, &w, nullptr);
  g_object_unref (l);
  return w;
}
gint gdk_string_width (GdkFont *font, const gchar *s) { return gdk_text_width (font, s, -1); }
gint gdk_char_width (GdkFont *font, gchar c) { return gdk_text_width (font, &c, 1); }
gint gdk_string_measure (GdkFont *font, const gchar *s) { return gdk_text_width (font, s, -1); }
gint gdk_text_measure (GdkFont *font, const gchar *s, gint len) { return gdk_text_width (font, s, len); }
gint gdk_text_width_wc (GdkFont *font, const GdkWChar *text, gint len)
{
  gchar *u = g_ucs4_to_utf8 ((const gunichar *) text, len, nullptr, nullptr, nullptr);
  gint w = gdk_text_width (font, u, -1);
  g_free (u);
  return w;
}
gint gdk_string_height (GdkFont *font, const gchar *) { return font ? font->ascent + font->descent : 0; }
gint gdk_text_height (GdkFont *font, const gchar *, gint) { return font ? font->ascent + font->descent : 0; }
gint gdk_char_height (GdkFont *font, gchar) { return font ? font->ascent + font->descent : 0; }

void gdk_text_extents (GdkFont *font, const gchar *text, gint len, gint *lbearing, gint *rbearing, gint *width, gint *ascent, gint *descent)
{
  int w = gdk_text_width (font, text, len);
  if (lbearing) *lbearing = 0;
  if (rbearing) *rbearing = w;
  if (width) *width = w;
  if (ascent) *ascent = font ? font->ascent : 0;
  if (descent) *descent = font ? font->descent : 0;
}
void gdk_string_extents (GdkFont *font, const gchar *s, gint *lb, gint *rb, gint *w, gint *a, gint *d)
{
  gdk_text_extents (font, s, -1, lb, rb, w, a, d);
}

void gdk_draw_text (GdkDrawable *d, GdkFont *font, GdkGC *gc, gint x, gint y, const gchar *text, gint len)
{
  if (!font || !text) return;
  auto *f = static_cast<GtkQtFont *> (font);
  PangoLayout *l = font_layout (f, font_text (text, len));
  int baseline = pango_layout_get_baseline (l) / PANGO_SCALE;
  gdk_draw_layout (d, gc, x, y - baseline, l);
  g_object_unref (l);
}
void gdk_draw_string (GdkDrawable *d, GdkFont *font, GdkGC *gc, gint x, gint y, const gchar *s) { gdk_draw_text (d, font, gc, x, y, s, -1); }
void gdk_draw_text_wc (GdkDrawable *d, GdkFont *font, GdkGC *gc, gint x, gint y, const GdkWChar *text, gint len)
{
  gchar *u = g_ucs4_to_utf8 ((const gunichar *) text, len, nullptr, nullptr, nullptr);
  gdk_draw_text (d, font, gc, x, y, u, -1);
  g_free (u);
}

gint gdk_mbstowcs (GdkWChar *dest, const gchar *src, gint dest_max)
{
  QString s = font_text (src, -1);
  QVector<uint> u = s.toUcs4 ();
  int n = qMin ((int) u.size (), dest_max);
  for (int i = 0; i < n; i++) dest[i] = u[i];
  return n;
}
gchar *gdk_wcstombs (const GdkWChar *src)
{
  int n = 0;
  while (src[n]) n++;
  return g_ucs4_to_utf8 ((const gunichar *) src, n, nullptr, nullptr, nullptr);
}
gboolean gdk_iswalnum (GdkWChar wc) { return g_unichar_isalnum (wc); }
gboolean gdk_iswspace (GdkWChar wc) { return g_unichar_isspace (wc); }

/* GtkStyle font (GTK 1 widgets drew text with style->font) */
GdkFont *gtk_style_get_font (GtkStyle *style)
{
  if (!style) return nullptr;
  if (!style->font) style->font = gdk_font_from_description (style->font_desc);
  return style->font;
}
void gtk_paint_string (GtkStyle *style, GdkWindow *window, GtkStateType state, const GdkRectangle *, GtkWidget *, const gchar *,
                       gint x, gint y, const gchar *string)
{
  gdk_draw_string (window, gtk_style_get_font (style), style->fg_gc[state], x, y, string);
}
void gtk_draw_string (GtkStyle *style, GdkWindow *window, GtkStateType state, gint x, gint y, const gchar *string)
{
  gtk_paint_string (style, window, state, nullptr, nullptr, nullptr, x, y, string);
}

void gtk_paint_polygon (GtkStyle *style, GdkWindow *window, GtkStateType state, GtkShadowType shadow, const GdkRectangle *,
                        GtkWidget *, const gchar *, const GdkPoint *points, gint n, gboolean fill)
{
  if (!style || n < 2) return;
  if (fill) gdk_draw_polygon (window, style->bg_gc[state], TRUE, points, n);
  bool in = shadow == GTK_SHADOW_IN || shadow == GTK_SHADOW_ETCHED_IN;
  for (int i = 0; i < n - 1; i++) {
    /* light on the upper-left edges, dark on the lower-right, like GTK 1 */
    bool upper = points[i + 1].y < points[i].y || (points[i + 1].y == points[i].y && points[i + 1].x > points[i].x) ||
                 points[i + 1].x < points[i].x;
    GdkGC *gc = (upper != in) ? style->light_gc[state] : style->dark_gc[state];
    if (shadow == GTK_SHADOW_NONE) gc = style->fg_gc[state];
    gdk_draw_line (window, gc, points[i].x, points[i].y, points[i + 1].x, points[i + 1].y);
  }
}
void gtk_draw_polygon (GtkStyle *style, GdkWindow *window, GtkStateType state, GtkShadowType shadow, GdkPoint *points, gint n, gboolean fill)
{
  gtk_paint_polygon (style, window, state, shadow, nullptr, nullptr, nullptr, points, n, fill);
}
void gtk_paint_shadow (GtkStyle *style, GdkWindow *window, GtkStateType state, GtkShadowType shadow, const GdkRectangle *,
                       GtkWidget *, const gchar *, gint x, gint y, gint width, gint height)
{
  if (!style || shadow == GTK_SHADOW_NONE) return;
  if (width < 0 || height < 0) { int w, h; gdk_drawable_get_size (window, &w, &h); if (width < 0) width = w; if (height < 0) height = h; }
  bool in = shadow == GTK_SHADOW_IN || shadow == GTK_SHADOW_ETCHED_IN;
  GdkGC *tl = in ? style->dark_gc[state] : style->light_gc[state], *br = in ? style->light_gc[state] : style->dark_gc[state];
  gdk_draw_line (window, tl, x, y, x + width - 1, y);
  gdk_draw_line (window, tl, x, y, x, y + height - 1);
  gdk_draw_line (window, br, x, y + height - 1, x + width - 1, y + height - 1);
  gdk_draw_line (window, br, x + width - 1, y, x + width - 1, y + height - 1);
}
void gtk_draw_shadow (GtkStyle *style, GdkWindow *window, GtkStateType state, GtkShadowType shadow, gint x, gint y, gint w, gint h)
{
  gtk_paint_shadow (style, window, state, shadow, nullptr, nullptr, nullptr, x, y, w, h);
}
void gtk_draw_hline (GtkStyle *style, GdkWindow *window, GtkStateType state, gint x1, gint x2, gint y)
{
  gtk_paint_hline (style, window, state, nullptr, nullptr, nullptr, x1, x2, y);
}
void gtk_draw_vline (GtkStyle *style, GdkWindow *window, GtkStateType state, gint y1_, gint y2_, gint x)
{
  gtk_paint_vline (style, window, state, nullptr, nullptr, nullptr, y1_, y2_, x);
}

} /* extern "C" */

/* ======================================================================
 * Pixmaps from XPM and bitmap data
 * ====================================================================== */
static GdkPixmap *pixmap_from_image (const QImage &src, GdkBitmap **mask, const GdkColor *transparent)
{
  if (src.isNull ()) { if (mask) *mask = nullptr; return nullptr; }
  QImage img = src.convertToFormat (QImage::Format_ARGB32);
  GdkPixmap *pm = gdk_pixmap_new (nullptr, img.width (), img.height (), 24);
  cairo_surface_t *s = pm->surface;
  cairo_surface_flush (s);
  uchar *data = cairo_image_surface_get_data (s);
  int stride = cairo_image_surface_get_stride (s);
  quint32 bg = transparent ? ((transparent->red >> 8) << 16) | ((transparent->green >> 8) << 8) | (transparent->blue >> 8) : 0xffffff;
  GdkBitmap *bm = nullptr;
  uchar *mdata = nullptr;
  int mstride = 0;
  if (mask) {
    bm = gdk_pixmap_new (nullptr, img.width (), img.height (), 1);
    cairo_surface_flush (bm->surface);
    mdata = cairo_image_surface_get_data (bm->surface);
    mstride = cairo_image_surface_get_stride (bm->surface);
  }
  for (int y = 0; y < img.height (); y++) {
    const quint32 *in = reinterpret_cast<const quint32 *> (img.constScanLine (y));
    quint32 *out = reinterpret_cast<quint32 *> (data + y * stride);
    for (int x = 0; x < img.width (); x++) {
      bool opaque = qAlpha (in[x]) >= 128;
      out[x] = opaque ? (in[x] & 0xffffff) : bg;
      if (mdata) mdata[y * mstride + x] = opaque ? 255 : 0;
    }
  }
  cairo_surface_mark_dirty (s);
  if (bm) cairo_surface_mark_dirty (bm->surface);
  if (mask) *mask = bm;
  return pm;
}

extern "C" {

GdkPixmap *gdk_pixmap_create_from_xpm_d (GdkWindow *, GdkBitmap **mask, const GdkColor *transparent, gchar **data)
{
  gtkqt_ensure_app ();
  return pixmap_from_image (QImage ((const char *const *) data), mask, transparent);
}
GdkPixmap *gdk_pixmap_colormap_create_from_xpm_d (GdkWindow *w, GdkColormap *, GdkBitmap **mask, const GdkColor *t, gchar **data)
{
  return gdk_pixmap_create_from_xpm_d (w, mask, t, data);
}
GdkPixmap *gdk_pixmap_create_from_xpm (GdkWindow *, GdkBitmap **mask, const GdkColor *transparent, const gchar *filename)
{
  gtkqt_ensure_app ();
  return pixmap_from_image (QImage (QFile::decodeName (filename)), mask, transparent);
}
GdkPixmap *gdk_pixmap_colormap_create_from_xpm (GdkWindow *w, GdkColormap *, GdkBitmap **mask, const GdkColor *t, const gchar *filename)
{
  return gdk_pixmap_create_from_xpm (w, mask, t, filename);
}

GdkBitmap *gdk_bitmap_create_from_data (GdkWindow *, const gchar *data, gint width, gint height)
{
  GdkBitmap *bm = gdk_pixmap_new (nullptr, width, height, 1);
  cairo_surface_flush (bm->surface);
  uchar *out = cairo_image_surface_get_data (bm->surface);
  int stride = cairo_image_surface_get_stride (bm->surface), bpl = (width + 7) / 8;
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
      out[y * stride + x] = (((const guchar *) data)[y * bpl + x / 8] >> (x % 8)) & 1 ? 255 : 0;
  cairo_surface_mark_dirty (bm->surface);
  return bm;
}

GdkPixmap *gdk_pixmap_create_from_data (GdkWindow *, const gchar *data, gint width, gint height, gint depth,
                                        const GdkColor *fg, const GdkColor *bg)
{
  (void) depth;
  GdkPixmap *pm = gdk_pixmap_new (nullptr, width, height, 24);
  cairo_surface_flush (pm->surface);
  auto *out = cairo_image_surface_get_data (pm->surface);
  int stride = cairo_image_surface_get_stride (pm->surface), bpl = (width + 7) / 8;
  auto rgb = [] (const GdkColor *c, quint32 dflt) { return c ? (quint32) (((c->red >> 8) << 16) | ((c->green >> 8) << 8) | (c->blue >> 8)) : dflt; };
  quint32 f = rgb (fg, 0), b = rgb (bg, 0xffffff);
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
      reinterpret_cast<quint32 *> (out + y * stride)[x] = (((const guchar *) data)[y * bpl + x / 8] >> (x % 8)) & 1 ? f : b;
  cairo_surface_mark_dirty (pm->surface);
  return pm;
}

/* ======================================================================
 * Visuals, GdkImage, GdkRGB
 * ====================================================================== */
GdkVisual *gdk_visual_get_best (void) { return gdk_visual_get_system (); }
GdkVisual *gdk_visual_get_best_with_depth (gint) { return gdk_visual_get_system (); }
gint gdk_visual_get_best_depth (void) { return 24; }

GdkImage *gdk_image_new (GdkImageType type, GdkVisual *visual, gint width, gint height)
{
  auto *img = g_new0 (GdkImage, 1);
  img->type = type;
  img->visual = visual ? visual : gdk_visual_get_system ();
  img->byte_order = G_BYTE_ORDER == G_LITTLE_ENDIAN ? GDK_LSB_FIRST : GDK_MSB_FIRST;
  img->width = width;
  img->height = height;
  img->depth = 24;
  img->bpp = 4;
  img->bpl = width * 4;
  img->mem = g_malloc0 ((gsize) img->bpl * MAX (1, height));
  return img;
}

GdkImage *gdk_image_get (GdkWindow *window, gint x, gint y, gint width, gint height)
{
  GdkImage *img = gdk_image_new (GDK_IMAGE_NORMAL, nullptr, width, height);
  cairo_surface_t *s = gtkqt_drawable_target (window);
  if (!s) return img;
  cairo_surface_t *dst = cairo_image_surface_create_for_data ((uchar *) img->mem, CAIRO_FORMAT_RGB24, width, height, img->bpl);
  cairo_t *cr = cairo_create (dst);
  cairo_set_source_surface (cr, s, -x, -y);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (dst);
  return img;
}

void gdk_image_put_pixel (GdkImage *image, gint x, gint y, guint32 pixel)
{
  if (x < 0 || y < 0 || x >= image->width || y >= image->height) return;
  reinterpret_cast<guint32 *> ((guchar *) image->mem + y * image->bpl)[x] = pixel;
}
guint32 gdk_image_get_pixel (GdkImage *image, gint x, gint y)
{
  if (x < 0 || y < 0 || x >= image->width || y >= image->height) return 0;
  return reinterpret_cast<guint32 *> ((guchar *) image->mem + y * image->bpl)[x] & 0xffffff;
}
void gdk_image_destroy (GdkImage *image) { if (image) { g_free (image->mem); g_free (image); } }

} /* extern "C" */

/* paint an image (RGB32 rows) onto a drawable through a GC's clip */
static void draw_rgb32 (GdkDrawable *d, GdkGC *gc, gint x, gint y, const uchar *data, int w, int h, int stride)
{
  cairo_surface_t *target = gtkqt_drawable_target (d);
  if (!target || w <= 0 || h <= 0) return;
  cairo_surface_t *src = cairo_image_surface_create_for_data ((uchar *) data, CAIRO_FORMAT_RGB24, w, h, stride);
  cairo_t *cr = cairo_create (target);
  GdkGCValues v;
  if (gc) {
    gdk_gc_get_values (gc, &v);
    (void) v;
  }
  cairo_rectangle (cr, x, y, w, h);
  cairo_clip (cr);
  cairo_set_source_surface (cr, src, x, y);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (src);
}

extern "C" {

void gdk_draw_image (GdkDrawable *d, GdkGC *gc, GdkImage *image, gint xsrc, gint ysrc, gint xdest, gint ydest, gint width, gint height)
{
  if (!image) return;
  if (width < 0) width = image->width - xsrc;
  if (height < 0) height = image->height - ysrc;
  draw_rgb32 (d, gc, xdest, ydest, (const uchar *) image->mem + ysrc * image->bpl + xsrc * 4, width, height, image->bpl);
}

void gdk_rgb_init (void) {}
GdkVisual *gdk_rgb_get_visual (void) { return gdk_visual_get_system (); }
GdkColormap *gdk_rgb_get_cmap (void) { return gdk_colormap_get_system (); }
gulong gdk_rgb_xpixel_from_rgb (guint32 rgb) { return rgb & 0xffffff; }
void gdk_rgb_gc_set_foreground (GdkGC *gc, guint32 rgb)
{
  GdkColor c = { rgb & 0xffffff, (guint16) (((rgb >> 16) & 0xff) * 257), (guint16) (((rgb >> 8) & 0xff) * 257), (guint16) ((rgb & 0xff) * 257) };
  gdk_gc_set_foreground (gc, &c);
}
void gdk_rgb_gc_set_background (GdkGC *gc, guint32 rgb)
{
  GdkColor c = { rgb & 0xffffff, (guint16) (((rgb >> 16) & 0xff) * 257), (guint16) (((rgb >> 8) & 0xff) * 257), (guint16) ((rgb & 0xff) * 257) };
  gdk_gc_set_background (gc, &c);
}
void gdk_rgb_set_install (gboolean) {}
void gdk_rgb_set_min_colors (gint) {}
void gdk_rgb_set_verbose (gboolean) {}
GdkRgbCmap *gdk_rgb_cmap_new (guint32 *colors, gint n)
{
  auto *c = g_new0 (GdkRgbCmap, 1);
  c->n_colors = qBound (0, n, 256);
  memcpy (c->colors, colors, c->n_colors * sizeof (guint32));
  return c;
}
void gdk_rgb_cmap_free (GdkRgbCmap *cmap) { g_free (cmap); }

void gdk_draw_rgb_image_dithalign (GdkDrawable *d, GdkGC *gc, gint x, gint y, gint w, gint h, GdkRgbDither,
                                   const guchar *buf, gint rowstride, gint, gint)
{
  QVector<quint32> px ((size_t) qMax (0, w) * qMax (0, h));
  for (int j = 0; j < h; j++) {
    const guchar *row = buf + j * rowstride;
    for (int i = 0; i < w; i++) px[j * w + i] = (row[i * 3] << 16) | (row[i * 3 + 1] << 8) | row[i * 3 + 2];
  }
  draw_rgb32 (d, gc, x, y, (const uchar *) px.constData (), w, h, w * 4);
}
void gdk_draw_rgb_image (GdkDrawable *d, GdkGC *gc, gint x, gint y, gint w, gint h, GdkRgbDither dith, const guchar *buf, gint rowstride)
{
  gdk_draw_rgb_image_dithalign (d, gc, x, y, w, h, dith, buf, rowstride, 0, 0);
}
void gdk_draw_rgb_32_image (GdkDrawable *d, GdkGC *gc, gint x, gint y, gint w, gint h, GdkRgbDither, const guchar *buf, gint rowstride)
{
  QVector<quint32> px ((size_t) qMax (0, w) * qMax (0, h));
  for (int j = 0; j < h; j++) {
    const guchar *row = buf + j * rowstride;
    for (int i = 0; i < w; i++) px[j * w + i] = (row[i * 4] << 16) | (row[i * 4 + 1] << 8) | row[i * 4 + 2];
  }
  draw_rgb32 (d, gc, x, y, (const uchar *) px.constData (), w, h, w * 4);
}
void gdk_draw_gray_image (GdkDrawable *d, GdkGC *gc, gint x, gint y, gint w, gint h, GdkRgbDither, const guchar *buf, gint rowstride)
{
  QVector<quint32> px ((size_t) qMax (0, w) * qMax (0, h));
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) { guchar g = buf[j * rowstride + i]; px[j * w + i] = (g << 16) | (g << 8) | g; }
  draw_rgb32 (d, gc, x, y, (const uchar *) px.constData (), w, h, w * 4);
}
void gdk_draw_indexed_image (GdkDrawable *d, GdkGC *gc, gint x, gint y, gint w, gint h, GdkRgbDither, const guchar *buf,
                             gint rowstride, GdkRgbCmap *cmap)
{
  QVector<quint32> px ((size_t) qMax (0, w) * qMax (0, h));
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) px[j * w + i] = cmap ? cmap->colors[buf[j * rowstride + i]] & 0xffffff : 0;
  draw_rgb32 (d, gc, x, y, (const uchar *) px.constData (), w, h, w * 4);
}

gboolean gtk_accel_group_activate (GtkAccelGroup *accel_group, guint key, GdkModifierType mods)
{
  gchar *name = gtk_accelerator_name (key, mods);
  QKeySequence seq = gtkqt_accel_to_keyseq (name);
  g_free (name);
  return gtkqt_accel_group_trigger (accel_group, seq);
}

} /* extern "C" */
