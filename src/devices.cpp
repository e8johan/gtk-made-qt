/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Input devices: the core pointer and keyboard, and the tools of graphics
 * tablets Qt reports (pen, eraser, puck) as GdkDevices with GTK 2's public
 * fields. A widget that asks for extension events (gtk_widget_set_
 * extension_events) gets a tool's presses and motion as GDK events from
 * that device, with x, y, pressure and tilt axes, and proximity events;
 * other widgets get the mouse events Qt makes of them, from the core
 * pointer, as with GTK.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QTabletEvent>
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
#include <QPointingDevice>
#endif

enum { AXIS_X, AXIS_Y, AXIS_PRESSURE, AXIS_XTILT, AXIS_YTILT, N_AXES };

struct DeviceExtra {
  QByteArray key;              /* the Qt tool it stands for */
  gdouble last[N_AXES] = {};   /* its last position (gdk_device_get_state) */
  guint last_state = 0;
};

typedef struct { GObjectClass parent_class; } GdkDeviceClass;
G_DEFINE_TYPE (GdkDevice, gdk_device, G_TYPE_OBJECT)
static void gdk_device_init (GdkDevice *d) { d->mode = GDK_MODE_SCREEN; }
static void device_finalize (GObject *o)
{
  auto *d = GDK_DEVICE (o);
  g_free (d->name);
  g_free (d->axes);
  g_free (d->keys);
  delete static_cast<DeviceExtra *> (g_object_get_data (o, "gtkqt-device"));
  G_OBJECT_CLASS (gdk_device_parent_class)->finalize (o);
}
static void gdk_device_class_init (GdkDeviceClass *klass) { G_OBJECT_CLASS (klass)->finalize = device_finalize; }

static DeviceExtra *extra (GdkDevice *d) { return static_cast<DeviceExtra *> (g_object_get_data (G_OBJECT (d), "gtkqt-device")); }

static GdkDevice *device_new (const char *name, GdkInputSource source, bool tablet, const QByteArray &key)
{
  auto *d = (GdkDevice *) g_object_new (gdk_device_get_type (), nullptr);
  d->name = g_strdup (name);
  d->source = source;
  d->has_cursor = !tablet;
  d->mode = tablet ? GDK_MODE_DISABLED : GDK_MODE_SCREEN;   /* GTK 2: programs enable extension devices */
  d->num_axes = tablet ? N_AXES : 2;
  d->axes = g_new0 (GdkDeviceAxis, d->num_axes);
  static const GdkAxisUse uses[N_AXES] = { GDK_AXIS_X, GDK_AXIS_Y, GDK_AXIS_PRESSURE, GDK_AXIS_XTILT, GDK_AXIS_YTILT };
  for (int i = 0; i < d->num_axes; i++) {
    d->axes[i].use = uses[i];
    d->axes[i].min = i >= AXIS_XTILT ? -1 : 0;
    d->axes[i].max = 1;
  }
  auto *e = new DeviceExtra;
  e->key = key;
  g_object_set_data (G_OBJECT (d), "gtkqt-device", e);
  return d;
}

static QList<GdkDevice *> &devices ()
{
  static QList<GdkDevice *> list;
  if (list.isEmpty ()) {
    list << device_new ("Core Pointer", GDK_SOURCE_MOUSE, false, "core-pointer");
    list << device_new ("Core Keyboard", GDK_SOURCE_KEYBOARD, false, "core-keyboard");
  }
  return list;
}

GdkDevice *gtkqt_core_pointer (void) { return devices ().at (0); }
static GdkDevice *core_keyboard () { return devices ().at (1); }

/* the GdkDevice of a tablet tool: one per tool (stylus, eraser ...) of a tablet */
static GdkDevice *tool_device (const QString &tablet, int pointer_type)
{
  /* Qt's pointer types: pen, eraser, cursor (puck) */
  GdkInputSource source = pointer_type == 2 ? GDK_SOURCE_ERASER : pointer_type == 3 ? GDK_SOURCE_CURSOR : GDK_SOURCE_PEN;
  QByteArray key = tablet.toUtf8 () + '/' + QByteArray::number (source);
  for (GdkDevice *d : devices ())
    if (extra (d)->key == key) return d;
  QString name = tablet.isEmpty () ? QStringLiteral ("Tablet") : tablet;
  name += source == GDK_SOURCE_ERASER ? QStringLiteral (" eraser") : source == GDK_SOURCE_CURSOR ? QStringLiteral (" cursor")
                                                                    : QStringLiteral (" stylus");
  GdkDevice *d = device_new (name.toUtf8 ().constData (), source, true, key);
  devices () << d;
  return d;
}

/* ---------- tablet events ---------- */
struct ToolInfo { QString tablet; int type; };

static ToolInfo tool_of (QTabletEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
  const QPointingDevice *pd = e->pointingDevice ();
  int type = 1;
  if (pd) {
    if (pd->pointerType () == QPointingDevice::PointerType::Eraser) type = 2;
    else if (pd->pointerType () == QPointingDevice::PointerType::Cursor) type = 3;
  }
  return { pd ? pd->name () : QString (), type };
#else
  int type = e->pointerType () == QTabletEvent::Eraser ? 2 : e->pointerType () == QTabletEvent::Cursor ? 3 : 1;
  return { QString (), type };
#endif
}

static GtkWidget *proximity_target;   /* the widget the tool was last over */

static bool wants_extension_events (GtkWidget *w)
{
  return w && GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "gtkqt-extension-events")) != GDK_EXTENSION_EVENTS_NONE;
}

bool gtkqt_tablet_event (GtkWidget *w, QWidget *qw, QTabletEvent *e, GdkWindow *window)
{
  if (!wants_extension_events (w)) return false;
  ToolInfo tool = tool_of (e);
  GdkDevice *dev = tool_device (tool.tablet, tool.type);
  if (dev->mode == GDK_MODE_DISABLED) return false;   /* Qt then makes mouse events of it */
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
  QPointF lp = e->position (), gp = e->globalPosition ();
  qreal xtilt = e->xTilt (), ytilt = e->yTilt ();
#else
  QPointF lp = e->posF (), gp = e->globalPosF ();
  qreal xtilt = e->xTilt (), ytilt = e->yTilt ();
#endif
  double x = lp.x () - (window ? window->origin_x : 0), y = lp.y () - (window ? window->origin_y : 0);
  DeviceExtra *ex = extra (dev);
  ex->last[AXIS_X] = x;
  ex->last[AXIS_Y] = y;
  ex->last[AXIS_PRESSURE] = e->pressure ();
  ex->last[AXIS_XTILT] = qBound (-1.0, xtilt / 60.0, 1.0);
  ex->last[AXIS_YTILT] = qBound (-1.0, ytilt / 60.0, 1.0);
  guint state = gtkqt_qt_mods_to_gdk (e->modifiers (), e->buttons ());
  ex->last_state = state;
  proximity_target = w;
  /* the axes go with the event; GDK frees them with it (gdk_event_free) */
  gdouble axes[N_AXES];
  memcpy (axes, ex->last, sizeof axes);
  GdkEvent ev = {};
  bool consumed;
  if (e->type () == QEvent::TabletMove) {
    ev.motion.type = GDK_MOTION_NOTIFY;
    ev.motion.window = window;
    ev.motion.time = e->timestamp ();
    ev.motion.x = x;
    ev.motion.y = y;
    ev.motion.x_root = gp.x ();
    ev.motion.y_root = gp.y ();
    ev.motion.axes = axes;
    ev.motion.state = state;
    ev.motion.device = dev;
    consumed = gtkqt_emit_event (w, &ev, "motion-notify-event");
  } else {
    bool press = e->type () == QEvent::TabletPress;
    ev.button.type = press ? GDK_BUTTON_PRESS : GDK_BUTTON_RELEASE;
    ev.button.window = window;
    ev.button.time = e->timestamp ();
    ev.button.x = x;
    ev.button.y = y;
    ev.button.x_root = gp.x ();
    ev.button.y_root = gp.y ();
    ev.button.axes = axes;
    ev.button.state = state;
    ev.button.button = e->button () == Qt::RightButton ? 3 : e->button () == Qt::MiddleButton ? 2 : 1;
    ev.button.device = dev;
    consumed = gtkqt_emit_event (w, &ev, press ? "button-press-event" : "button-release-event");
  }
  Q_UNUSED (consumed);
  /* GTK delivers the tool's events instead of core ones */
  e->accept ();
  return true;
}

/* proximity: Qt tells the application; GTK tells the window the tool is over */
class GtkQtProximityFilter : public QObject {
public:
  using QObject::QObject;
  bool eventFilter (QObject *, QEvent *e) override
  {
    if (e->type () != QEvent::TabletEnterProximity && e->type () != QEvent::TabletLeaveProximity) return false;
    auto *te = static_cast<QTabletEvent *> (e);
    ToolInfo tool = tool_of (te);
    GdkDevice *dev = tool_device (tool.tablet, tool.type);
    GtkWidget *w = proximity_target;
    if (!w || w->priv->destroyed) {
      QWidget *under = QApplication::widgetAt (QCursor::pos ());
      w = nullptr;
      for (QWidget *q = under; q && !w; q = q->parentWidget ())
        if (GtkWidget *g = gtkqt_from_qobject (q); g && wants_extension_events (g)) w = g;
    }
    if (!w || dev->mode == GDK_MODE_DISABLED) return false;
    GdkEvent ev = {};
    ev.proximity.type = e->type () == QEvent::TabletEnterProximity ? GDK_PROXIMITY_IN : GDK_PROXIMITY_OUT;
    ev.proximity.window = w->window;
    ev.proximity.time = te->timestamp ();
    ev.proximity.device = dev;
    gtkqt_emit_event (w, &ev, ev.proximity.type == GDK_PROXIMITY_IN ? "proximity-in-event" : "proximity-out-event");
    return false;
  }
};

void gtkqt_devices_init (void)
{
  static bool done;
  if (done || !qApp) return;
  done = true;
  qApp->installEventFilter (new GtkQtProximityFilter (qApp));
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
  /* the tablets Qt knows: their stylus and eraser, listed before first use */
  for (const QInputDevice *d : QInputDevice::devices ())
    if (d->type () == QInputDevice::DeviceType::Stylus || d->type () == QInputDevice::DeviceType::Airbrush) {
      tool_device (d->name (), 1);
      tool_device (d->name (), 2);
    } else if (d->type () == QInputDevice::DeviceType::Puck) {
      tool_device (d->name (), 3);
    }
#endif
}

extern "C" {

GList *gdk_devices_list (void)
{
  gtkqt_ensure_app ();
  gtkqt_devices_init ();
  /* GTK 2 lists the pointing devices; the keyboard is not one */
  GList *l = nullptr;
  for (GdkDevice *d : devices ()) if (d->source != GDK_SOURCE_KEYBOARD) l = g_list_append (l, d);
  return l;
}

GdkDevice *gdk_device_get_core_pointer (void) { return gtkqt_core_pointer (); }

void gdk_device_set_source (GdkDevice *d, GdkInputSource source) { if (d) d->source = source; }
GdkInputMode gdk_device_get_mode (GdkDevice *d) { return d ? d->mode : GDK_MODE_DISABLED; }

gboolean gdk_device_set_mode (GdkDevice *d, GdkInputMode mode)
{
  if (!d) return FALSE;
  if (d == gtkqt_core_pointer () && mode == GDK_MODE_DISABLED) return FALSE;
  d->mode = mode;
  return TRUE;
}

void gdk_device_set_key (GdkDevice *d, guint index_, guint keyval, GdkModifierType modifiers)
{
  if (!d) return;
  if ((gint) index_ >= d->num_keys) {
    d->keys = g_renew (GdkDeviceKey, d->keys, index_ + 1);
    for (guint i = d->num_keys; i <= index_; i++) d->keys[i] = { 0, (GdkModifierType) 0 };
    d->num_keys = index_ + 1;
  }
  d->keys[index_] = { keyval, modifiers };
}

void gdk_device_set_axis_use (GdkDevice *d, guint index_, GdkAxisUse use) { if (d && (gint) index_ < d->num_axes) d->axes[index_].use = use; }
GdkAxisUse gdk_device_get_axis_use (GdkDevice *d, guint index_) { return d && (gint) index_ < d->num_axes ? d->axes[index_].use : GDK_AXIS_IGNORE; }
gint gdk_device_get_n_axes (GdkDevice *d) { return d ? d->num_axes : 0; }
gboolean gdk_device_get_has_cursor (GdkDevice *d) { return d && d->has_cursor; }
const gchar *gdk_device_get_name (GdkDevice *d) { return d ? d->name : nullptr; }
GdkInputSource gdk_device_get_source (GdkDevice *d) { return d ? d->source : GDK_SOURCE_MOUSE; }

/* where the device is: the pointer's position, or the tool's last one */
void gdk_device_get_state (GdkDevice *d, GdkWindow *window, gdouble *axes, GdkModifierType *mask)
{
  if (!d || d->source == GDK_SOURCE_MOUSE || d->num_axes <= 2) {
    gint x = 0, y = 0;
    GdkModifierType m = (GdkModifierType) 0;
    gdk_window_get_pointer (window, &x, &y, &m);
    if (axes && d && d->num_axes >= 2) { axes[0] = x; axes[1] = y; }
    if (mask) *mask = m;
    return;
  }
  DeviceExtra *ex = extra (d);
  if (axes) memcpy (axes, ex->last, sizeof (gdouble) * qMin (d->num_axes, (gint) N_AXES));
  if (mask) *mask = (GdkModifierType) (ex->last_state | gtkqt_qt_mods_to_gdk (QGuiApplication::queryKeyboardModifiers (), Qt::NoButton));
}

gboolean gdk_device_get_axis (GdkDevice *d, gdouble *axes, GdkAxisUse use, gdouble *value)
{
  if (!d || !axes) return FALSE;
  for (int i = 0; i < d->num_axes; i++)
    if (d->axes[i].use == use) {
      if (value) *value = axes[i];
      return TRUE;
    }
  return FALSE;
}

/* motion history is not kept: every motion comes as an event */
gboolean gdk_device_get_history (GdkDevice *, GdkWindow *, guint32, guint32, gpointer **events, gint *n_events)
{
  if (events) *events = nullptr;
  if (n_events) *n_events = 0;
  return FALSE;
}
void gdk_device_free_history (gpointer *events, gint) { g_free (events); }

gboolean gdk_event_get_axis (const GdkEvent *event, GdkAxisUse use, gdouble *value)
{
  if (!event) return FALSE;
  double x, y;
  GdkDevice *dev = nullptr;
  gdouble *axes = nullptr;
  switch (event->type) {
  case GDK_MOTION_NOTIFY: x = event->motion.x; y = event->motion.y; dev = event->motion.device; axes = event->motion.axes; break;
  case GDK_BUTTON_PRESS: case GDK_2BUTTON_PRESS: case GDK_3BUTTON_PRESS: case GDK_BUTTON_RELEASE:
    x = event->button.x; y = event->button.y; dev = event->button.device; axes = event->button.axes; break;
  case GDK_SCROLL: x = event->scroll.x; y = event->scroll.y; break;
  default: return FALSE;
  }
  if (use == GDK_AXIS_X || use == GDK_AXIS_Y) {
    if (value) *value = use == GDK_AXIS_X ? x : y;
    return TRUE;
  }
  return dev && axes ? gdk_device_get_axis (dev, axes, use, value) : FALSE;
}

void gdk_input_set_extension_events (GdkWindow *window, gint, GdkExtensionMode mode)
{
  if (window && window->widget) gtk_widget_set_extension_events (window->widget, mode);
}

void gtk_widget_set_extension_events (GtkWidget *widget, GdkExtensionMode mode)
{
  gtkqt_devices_init ();
  g_object_set_data (G_OBJECT (widget), "gtkqt-extension-events", GINT_TO_POINTER (mode));
  /* the tool's own cursor or the pointer's: Qt draws the pointer either way */
  if (QWidget *qw = gtkqt_qw (widget)) {
    qw->setAttribute (Qt::WA_TabletTracking, mode != GDK_EXTENSION_EVENTS_NONE);
    if (auto *sa = qobject_cast<QAbstractScrollArea *> (qw)) sa->viewport ()->setAttribute (Qt::WA_TabletTracking, mode != GDK_EXTENSION_EVENTS_NONE);
  }
}

GdkExtensionMode gtk_widget_get_extension_events (GtkWidget *widget)
{
  return (GdkExtensionMode) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "gtkqt-extension-events"));
}

/* ---------- GTK 3's view of the same devices ---------- */
struct _GdkSeat { int unused; };
struct _GdkDeviceManager { int unused; };
static GdkSeat default_seat;
static GdkDeviceManager device_manager;

GdkDevice *gtk_get_current_event_device (void) { return gtkqt_core_pointer (); }

GdkWindow *gdk_window_get_device_position (GdkWindow *window, GdkDevice *, gint *x, gint *y, GdkModifierType *mask)
{
  return gdk_window_get_pointer (window, x, y, mask);
}

void gdk_device_get_position (GdkDevice *, GdkScreen **screen, gint *x, gint *y)
{
  QPoint p = QCursor::pos ();
  if (screen) *screen = gdk_screen_get_default ();
  if (x) *x = p.x ();
  if (y) *y = p.y ();
}

GdkDevice *gdk_device_get_associated_device (GdkDevice *d) { return d == gtkqt_core_pointer () ? core_keyboard () : gtkqt_core_pointer (); }
GdkSeat *gdk_display_get_default_seat (GdkDisplay *) { return &default_seat; }
GdkDevice *gdk_seat_get_pointer (GdkSeat *) { return gtkqt_core_pointer (); }
GdkDevice *gdk_seat_get_keyboard (GdkSeat *) { return core_keyboard (); }
GdkDeviceManager *gdk_display_get_device_manager (GdkDisplay *) { return &device_manager; }
GdkDevice *gdk_device_manager_get_client_pointer (GdkDeviceManager *) { return gtkqt_core_pointer (); }

} /* extern "C" */
