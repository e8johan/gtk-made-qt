/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkDrawingArea (cairo drawing into a QImage), GTK 4 event controllers,
 * and the GDK helpers: events, key values, colours, pixbufs, screens.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QScreen>
#include <QRegularExpression>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

GdkWindow *gtkqt_gdk_window (GtkWidget *w)
{
  if (!w->window) w->window = gtkqt_drawable_new (GTKQT_DRAWABLE_WINDOW, w);
  return w->window;
}

/* ======================================================================
 * GtkDrawingArea
 * ====================================================================== */
struct DrawData { GtkDrawingAreaDrawFunc func = nullptr; gpointer data = nullptr; GDestroyNotify destroy = nullptr; int cw = 0, ch = 0; };

static DrawData *draw_data (GtkWidget *w)
{
  auto *d = static_cast<DrawData *> (g_object_get_data (G_OBJECT (w), "gtkqt-draw"));
  if (!d) {
    d = new DrawData;
    g_object_set_data_full (G_OBJECT (w), "gtkqt-draw", d, [] (gpointer p) {
      auto *dd = static_cast<DrawData *> (p);
      if (dd->destroy) dd->destroy (dd->data);
      delete dd;
    });
  }
  return d;
}

/* A widget the program draws on: GtkDrawingArea, and program-defined
 * widget classes (GTK 1/2 custom widgets). Like an X window with backing
 * store it keeps its pixels: a real exposure clears the area to the window
 * background (colour or background pixmap) and emits expose-event / draw;
 * drawing at any other time lands in the backing image and is shown
 * straight after. */
class GtkQtDrawingWidget : public QWidget {
public:
  QImage backing;
  cairo_surface_t *surface = nullptr;
  bool blit_only = false, flush_pending = false, painting = false;

  ~GtkQtDrawingWidget () override { if (surface) cairo_surface_destroy (surface); }

  /* does the program draw on this widget? (otherwise a window with children
   * paints like any other QWidget) */
  bool program_draws (GtkWidget *w) const
  {
    if (GTK_IS_DRAWING_AREA (w) || !GTK_IS_WINDOW (w)) return true;
    if (g_object_get_data (G_OBJECT (w), "gtkqt-app-paintable")) return true;
    static guint expose_id = g_signal_lookup ("expose-event", GTK_TYPE_WIDGET);
    static guint draw_id = g_signal_lookup ("draw", GTK_TYPE_WIDGET);
    return g_signal_has_handler_pending (w, expose_id, 0, FALSE) || g_signal_has_handler_pending (w, draw_id, 0, FALSE);
  }

  QSize sizeHint () const override
  {
    if (layout ()) return QWidget::sizeHint ();
    GtkWidget *w = gtkqt_from_qobject (const_cast<GtkQtDrawingWidget *> (this));
    if (!w || !w->priv || w->priv->destroyed) return QSize (0, 0);
    if (GTK_IS_DRAWING_AREA (w)) {
      DrawData *d = static_cast<DrawData *> (g_object_get_data (G_OBJECT (w), "gtkqt-draw"));
      if (d && (d->cw > 0 || d->ch > 0)) return QSize (qMax (d->cw, 1), qMax (d->ch, 1));
    }
    if (auto fn = GTK_WIDGET_GET_CLASS (w)->size_request) {
      GtkRequisition r = { 0, 0 };
      fn (w, &r);
      w->requisition = r;
      return QSize (r.width, r.height);
    }
    return QSize (0, 0);
  }
  QSize minimumSizeHint () const override
  {
    if (layout ()) return QWidget::minimumSizeHint ();
    GtkWidget *w = gtkqt_from_qobject (const_cast<GtkQtDrawingWidget *> (this));
    return w && !GTK_IS_DRAWING_AREA (w) ? sizeHint () : QSize (0, 0);
  }

  void ensure_backing ()
  {
    qreal dpr = devicePixelRatioF ();
    QSize want = size () * dpr;
    if (want.isEmpty ()) want = QSize (1, 1);
    if (backing.size () == want && surface) return;
    QImage n (want, QImage::Format_ARGB32_Premultiplied);
    n.setDevicePixelRatio (dpr);
    n.fill (background ());
    if (!backing.isNull ()) { QPainter p (&n); p.drawImage (0, 0, backing); }
    backing = n;
    if (surface) cairo_surface_destroy (surface);
    surface = cairo_image_surface_create_for_data (backing.bits (), CAIRO_FORMAT_ARGB32, backing.width (), backing.height (),
                                                   backing.bytesPerLine ());
    cairo_surface_set_device_scale (surface, dpr, dpr);
  }

  QColor background () const
  {
    GtkWidget *w = gtkqt_from_qobject (const_cast<GtkQtDrawingWidget *> (this));
    GdkWindow *win = w ? w->window : nullptr;
    if (win && win->has_background)
      return QColor (win->background.red >> 8, win->background.green >> 8, win->background.blue >> 8);
    return palette ().color (QPalette::Window);
  }

  /* drawing outside an expose handler: show it once the program is done */
  cairo_surface_t *surface_for_drawing ()
  {
    ensure_backing ();
    if (!painting && !flush_pending) {
      flush_pending = true;
      QPointer<GtkQtDrawingWidget> self (this);
      QTimer::singleShot (0, this, [self] {
        if (!self) return;
        self->flush_pending = false;
        self->blit_only = true;
        self->repaint ();
        self->blit_only = false;
      });
    }
    return surface;
  }

protected:
  void paintEvent (QPaintEvent *e) override
  {
    GtkWidget *w = gtkqt_from_qobject (this);
    if (!w || w->priv->destroyed) return;
    if (!program_draws (w) && backing.isNull ()) {
      QPainter p (this);
      p.fillRect (e->rect (), palette ().color (QPalette::Window));
      return;
    }
    ensure_backing ();
    if (!blit_only) {
      /* clear the exposed area to the window background, as X does */
      {
        QPainter bp (&backing);
        GdkWindow *win = w->window;
        if (win && win->back_pixmap && win->back_pixmap->surface) {
          bp.setClipRegion (e->region ());
          bp.setCompositionMode (QPainter::CompositionMode_Source);
          bp.drawTiledPixmap (rect (), QPixmap::fromImage (gtkqt_surface_image (win->back_pixmap->surface)));
        } else {
          bp.setCompositionMode (QPainter::CompositionMode_Source);
          for (const QRect &r : e->region ()) bp.fillRect (r, background ());
        }
      }
      cairo_surface_mark_dirty (surface);
      w->priv->paint_surface = surface;
      painting = true;
      {
        GtkQtEmitGuard guard;
        g_object_ref (w);
        if (GTK_IS_DRAWING_AREA (w)) {
          DrawData *d = draw_data (w);
          if (d->func) {
            cairo_t *cr = cairo_create (surface);
                        d->func (GTK_DRAWING_AREA (w), cr, width (), height (), d->data);
            cairo_destroy (cr);
          }
        }
        if (!w->priv->destroyed) {
          cairo_t *cr = cairo_create (surface);
          for (const QRect &r : e->region ()) cairo_rectangle (cr, r.x (), r.y (), r.width (), r.height ());
          cairo_clip (cr);
          gboolean handled = FALSE;
          g_signal_emit_by_name (w, "draw", cr, &handled);
          cairo_destroy (cr);
        }
        if (!w->priv->destroyed) {
          GdkEvent ev = {};
          ev.expose.type = GDK_EXPOSE;
          ev.expose.window = w->window;
          ev.expose.area = { e->rect ().x (), e->rect ().y (), e->rect ().width (), e->rect ().height () };
          gtkqt_emit_event (w, &ev, "expose-event");
        }
        g_object_unref (w);
      }
      painting = false;
      if (w->priv) w->priv->paint_surface = nullptr;
    }
    cairo_surface_flush (surface);
    QPainter p (this);
    p.drawImage (0, 0, backing);
   
  }
  void resizeEvent (QResizeEvent *e) override
  {
    QWidget::resizeEvent (e);
    ensure_backing ();
    if (GtkWidget *w = gtkqt_from_qobject (this); w && GTK_IS_DRAWING_AREA (w)) {
      GtkQtEmitGuard guard;
      g_signal_emit_by_name (w, "resize", width (), height ());
    }
  }
};

QObject *gtkqt_canvas_create (GtkWidget *)
{
  auto *d = new GtkQtDrawingWidget;
  d->setAttribute (Qt::WA_OpaquePaintEvent);
  return d;
}

cairo_surface_t *gtkqt_canvas_surface (GtkWidget *w)
{
  auto *c = dynamic_cast<GtkQtDrawingWidget *> (gtkqt_qw (w));
  return c ? c->surface_for_drawing () : nullptr;
}

static QObject *drawing_create (GtkWidget *)
{
  auto *d = new GtkQtDrawingWidget;
  d->setAttribute (Qt::WA_OpaquePaintEvent);
  d->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  return d;
}

G_DEFINE_TYPE (GtkDrawingArea, gtk_drawing_area, GTK_TYPE_WIDGET)
static void gtk_drawing_area_init (GtkDrawingArea *) {}
static void gtk_drawing_area_class_init (GtkDrawingAreaClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), drawing_create);
  g_signal_new ("resize", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  gtkqt_prop (klass, g_param_spec_int ("content-width", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("content-height", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (o), g_value_get_int (v)); }, nullptr);
}

extern "C" {
GtkWidget *gtk_drawing_area_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_DRAWING_AREA, nullptr); }
void gtk_drawing_area_size (GtkDrawingArea *darea, gint width, gint height) { gtk_widget_set_size_request (GTK_WIDGET (darea), width, height); }

void gtk_drawing_area_set_draw_func (GtkDrawingArea *self, GtkDrawingAreaDrawFunc draw_func, gpointer user_data, GDestroyNotify destroy)
{
  DrawData *d = draw_data (GTK_WIDGET (self));
  if (d->destroy) d->destroy (d->data);
  d->func = draw_func;
  d->data = user_data;
  d->destroy = destroy;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void gtk_drawing_area_set_content_width (GtkDrawingArea *self, int width)
{
  draw_data (GTK_WIDGET (self))->cw = width;
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (self))) qw->updateGeometry ();
}

void gtk_drawing_area_set_content_height (GtkDrawingArea *self, int height)
{
  draw_data (GTK_WIDGET (self))->ch = height;
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (self))) qw->updateGeometry ();
}
}

/* ======================================================================
 * GTK 4 event controllers
 * ====================================================================== */
enum CtrlKind { K_CLICK, K_DRAG, K_KEY, K_MOTION, K_SCROLL, K_FOCUS };
struct CtrlData { GtkWidget *widget = nullptr; CtrlKind kind; guint button = 1; guint current_button = 0;
                  bool dragging = false; QPointF start, offset; int scroll_flags = 0; };

static CtrlData *ctrl (gpointer c) { return static_cast<CtrlData *> (((GtkEventController *) c)->priv); }

G_DEFINE_TYPE (GtkEventController, gtk_event_controller, G_TYPE_OBJECT)
static void gtk_event_controller_init (GtkEventController *c) { c->priv = new CtrlData; }
static void ctrl_finalize (GObject *o)
{
  delete ctrl (o);
  G_OBJECT_CLASS (gtk_event_controller_parent_class)->finalize (o);
}
static void gtk_event_controller_class_init (GtkEventControllerClass *klass) { G_OBJECT_CLASS (klass)->finalize = ctrl_finalize; }

static GType ctrl_subtype (const char *name, void (*class_init) (gpointer, gpointer))
{
  return g_type_register_static_simple (GTK_TYPE_EVENT_CONTROLLER, g_intern_static_string (name), sizeof (GtkEventControllerClass),
                                        (GClassInitFunc) class_init, sizeof (GtkEventController), nullptr, (GTypeFlags) 0);
}

#define SIG(name, ret, ...) g_signal_new (name, G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, \
  ret == G_TYPE_BOOLEAN ? g_signal_accumulator_true_handled : nullptr, nullptr, nullptr, ret, __VA_ARGS__)

static GType click_type ()
{
  static GType t = ctrl_subtype ("GtkGestureClick", [] (gpointer klass, gpointer) {
    SIG ("pressed", G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
    SIG ("released", G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
    SIG ("stopped", G_TYPE_NONE, 0);
  });
  return t;
}
static GType drag_type ()
{
  static GType t = ctrl_subtype ("GtkGestureDrag", [] (gpointer klass, gpointer) {
    SIG ("drag-begin", G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
    SIG ("drag-update", G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
    SIG ("drag-end", G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  });
  return t;
}
static GType key_type ()
{
  static GType t = ctrl_subtype ("GtkEventControllerKey", [] (gpointer klass, gpointer) {
    SIG ("key-pressed", G_TYPE_BOOLEAN, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
    SIG ("key-released", G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
    SIG ("modifiers", G_TYPE_BOOLEAN, 1, G_TYPE_UINT);
  });
  return t;
}
static GType motion_type ()
{
  static GType t = ctrl_subtype ("GtkEventControllerMotion", [] (gpointer klass, gpointer) {
    SIG ("motion", G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
    SIG ("enter", G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
    SIG ("leave", G_TYPE_NONE, 0);
  });
  return t;
}
static GType scroll_type ()
{
  static GType t = ctrl_subtype ("GtkEventControllerScroll", [] (gpointer klass, gpointer) {
    SIG ("scroll", G_TYPE_BOOLEAN, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  });
  return t;
}
static GType focus_type ()
{
  static GType t = ctrl_subtype ("GtkEventControllerFocus", [] (gpointer klass, gpointer) {
    SIG ("enter", G_TYPE_NONE, 0);
    SIG ("leave", G_TYPE_NONE, 0);
  });
  return t;
}

static GtkEventController *ctrl_new (GType t, CtrlKind kind)
{
  auto *c = (GtkEventController *) g_object_new (t, nullptr);
  ctrl (c)->kind = kind;
  return c;
}

void gtkqt_controllers_dispatch (GtkWidget *widget, QEvent *e, bool *consumed)
{
  GList *copy = g_list_copy_deep (widget->priv->controllers, (GCopyFunc) g_object_ref, nullptr);
  for (GList *l = copy; l && !widget->priv->destroyed; l = l->next) {
    auto *c = (GtkEventController *) l->data;
    CtrlData *d = ctrl (c);
    switch (e->type ()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove: {
      auto *me = static_cast<QMouseEvent *> (e);
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
      QPointF p = me->position ();
#else
      QPointF p = me->localPos ();
#endif
      guint b = me->button () == Qt::RightButton ? 3 : me->button () == Qt::MiddleButton ? 2 : 1;
      if (d->kind == K_CLICK && e->type () != QEvent::MouseMove && (d->button == 0 || d->button == b)) {
        d->current_button = b;
        int n = e->type () == QEvent::MouseButtonDblClick ? 2 : 1;
        g_signal_emit_by_name (c, e->type () == QEvent::MouseButtonRelease ? "released" : "pressed", n, p.x (), p.y ());
      } else if (d->kind == K_DRAG) {
        if (e->type () == QEvent::MouseButtonPress && (d->button == 0 || d->button == b)) {
          d->dragging = true; d->start = p; d->offset = QPointF ();
          d->current_button = b;
          g_signal_emit_by_name (c, "drag-begin", p.x (), p.y ());
        } else if (e->type () == QEvent::MouseMove && d->dragging) {
          d->offset = p - d->start;
          g_signal_emit_by_name (c, "drag-update", d->offset.x (), d->offset.y ());
        } else if (e->type () == QEvent::MouseButtonRelease && d->dragging) {
          d->dragging = false;
          d->offset = p - d->start;
          g_signal_emit_by_name (c, "drag-end", d->offset.x (), d->offset.y ());
        }
      } else if (d->kind == K_MOTION && e->type () == QEvent::MouseMove) {
        g_signal_emit_by_name (c, "motion", p.x (), p.y ());
      }
      break;
    }
    case QEvent::Enter:
      if (d->kind == K_MOTION) {
        QPoint p = gtkqt_qw (widget)->mapFromGlobal (QCursor::pos ());
        g_signal_emit_by_name (c, "enter", (double) p.x (), (double) p.y ());
      }
      break;
    case QEvent::Leave:
      if (d->kind == K_MOTION) g_signal_emit_by_name (c, "leave");
      break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
      if (d->kind == K_KEY) {
        auto *ke = static_cast<QKeyEvent *> (e);
        guint keyval = gtkqt_qt_key_to_keyval (ke->key (), ke->text (), ke->modifiers ());
        guint state = gtkqt_qt_mods_to_gdk (ke->modifiers (), Qt::NoButton);
        if (e->type () == QEvent::KeyPress) {
          gboolean handled = FALSE;
          g_signal_emit_by_name (c, "key-pressed", keyval, (guint) ke->nativeScanCode (), state, &handled);
          if (handled) *consumed = true;
        } else {
          g_signal_emit_by_name (c, "key-released", keyval, (guint) ke->nativeScanCode (), state);
        }
      }
      break;
    case QEvent::Wheel:
      if (d->kind == K_SCROLL) {
        QPoint delta = static_cast<QWheelEvent *> (e)->angleDelta ();
        gboolean handled = FALSE;
        g_signal_emit_by_name (c, "scroll", -delta.x () / 120.0, -delta.y () / 120.0, &handled);
        if (handled) *consumed = true;
      }
      break;
    case QEvent::FocusIn:
    case QEvent::FocusOut:
      if (d->kind == K_FOCUS) g_signal_emit_by_name (c, e->type () == QEvent::FocusIn ? "enter" : "leave");
      break;
    default:
      break;
    }
  }
  g_list_free_full (copy, g_object_unref);
}

extern "C" {

GtkGesture *gtk_gesture_click_new (void) { return ctrl_new (click_type (), K_CLICK); }
GtkGesture *gtk_gesture_drag_new (void) { return ctrl_new (drag_type (), K_DRAG); }
GtkEventController *gtk_event_controller_key_new (void) { return ctrl_new (key_type (), K_KEY); }
GtkEventController *gtk_event_controller_motion_new (void) { return ctrl_new (motion_type (), K_MOTION); }
GtkEventController *gtk_event_controller_focus_new (void) { return ctrl_new (focus_type (), K_FOCUS); }

GtkEventController *gtk_event_controller_scroll_new (GtkEventControllerScrollFlags flags)
{
  GtkEventController *c = ctrl_new (scroll_type (), K_SCROLL);
  ctrl (c)->scroll_flags = flags;
  return c;
}

GtkWidget *gtk_event_controller_get_widget (GtkEventController *controller) { return ctrl (controller)->widget; }
void gtk_gesture_single_set_button (GtkGestureSingle *gesture, guint button) { ctrl (gesture)->button = button; }
guint gtk_gesture_single_get_current_button (GtkGestureSingle *gesture) { return ctrl (gesture)->current_button; }

gboolean gtk_gesture_drag_get_start_point (GtkGestureDrag *gesture, double *x, double *y)
{
  CtrlData *d = ctrl (gesture);
  if (x) *x = d->start.x ();
  if (y) *y = d->start.y ();
  return d->dragging;
}

gboolean gtk_gesture_drag_get_offset (GtkGestureDrag *gesture, double *x, double *y)
{
  CtrlData *d = ctrl (gesture);
  if (x) *x = d->offset.x ();
  if (y) *y = d->offset.y ();
  return d->dragging;
}

void gtk_widget_add_controller (GtkWidget *widget, GtkEventController *controller)
{
  CtrlData *d = ctrl (controller);
  d->widget = widget;
  widget->priv->controllers = g_list_append (widget->priv->controllers, controller);   /* takes ownership */
  if (QWidget *qw = gtkqt_qw (widget)) {
    if (d->kind == K_MOTION || d->kind == K_DRAG) qw->setMouseTracking (true);
    if ((d->kind == K_KEY || d->kind == K_FOCUS) && qw->focusPolicy () == Qt::NoFocus) qw->setFocusPolicy (Qt::StrongFocus);
  }
}

void gtk_widget_remove_controller (GtkWidget *widget, GtkEventController *controller)
{
  if (!g_list_find (widget->priv->controllers, controller)) return;
  widget->priv->controllers = g_list_remove (widget->priv->controllers, controller);
  ctrl (controller)->widget = nullptr;
  g_object_unref (controller);
}

} /* extern "C" */

/* ======================================================================
 * GDK: events, key values, colours
 * ====================================================================== */
extern "C" {

GType gdk_event_get_type (void)
{
  static GType t = g_boxed_type_register_static ("GdkEvent", (GBoxedCopyFunc) gdk_event_copy, (GBoxedFreeFunc) gdk_event_free);
  return t;
}

GType gdk_rectangle_get_type (void)
{
  static GType t = g_boxed_type_register_static ("GdkRectangle",
    [] (gpointer p) -> gpointer { return g_memdup2 (p, sizeof (GdkRectangle)); }, g_free);
  return t;
}

GType gdk_rgba_get_type (void)
{
  static GType t = g_boxed_type_register_static ("GdkRGBA",
    [] (gpointer p) -> gpointer { return g_memdup2 (p, sizeof (GdkRGBA)); }, g_free);
  return t;
}

GdkEventType gdk_event_get_event_type (GdkEvent *event) { return event->type; }

gboolean gdk_event_get_coords (GdkEvent *e, gdouble *x, gdouble *y)
{
  double ex, ey;
  switch (e->type) {
  case GDK_BUTTON_PRESS: case GDK_2BUTTON_PRESS: case GDK_3BUTTON_PRESS: case GDK_BUTTON_RELEASE: ex = e->button.x; ey = e->button.y; break;
  case GDK_MOTION_NOTIFY: ex = e->motion.x; ey = e->motion.y; break;
  case GDK_SCROLL: ex = e->scroll.x; ey = e->scroll.y; break;
  case GDK_ENTER_NOTIFY: case GDK_LEAVE_NOTIFY: ex = e->crossing.x; ey = e->crossing.y; break;
  default: return FALSE;
  }
  if (x) *x = ex;
  if (y) *y = ey;
  return TRUE;
}

gboolean gdk_event_get_root_coords (GdkEvent *e, gdouble *x, gdouble *y)
{
  switch (e->type) {
  case GDK_BUTTON_PRESS: case GDK_2BUTTON_PRESS: case GDK_3BUTTON_PRESS: case GDK_BUTTON_RELEASE:
    if (x) *x = e->button.x_root;
    if (y) *y = e->button.y_root;
    return TRUE;
  case GDK_MOTION_NOTIFY:
    if (x) *x = e->motion.x_root;
    if (y) *y = e->motion.y_root;
    return TRUE;
  default: return FALSE;
  }
}

gboolean gdk_event_get_button (GdkEvent *e, guint *button)
{
  if (e->type < GDK_BUTTON_PRESS || e->type > GDK_BUTTON_RELEASE) return FALSE;
  if (button) *button = e->button.button;
  return TRUE;
}

gboolean gdk_event_get_keyval (GdkEvent *e, guint *keyval)
{
  if (e->type != GDK_KEY_PRESS && e->type != GDK_KEY_RELEASE) return FALSE;
  if (keyval) *keyval = e->key.keyval;
  return TRUE;
}

gboolean gdk_event_get_state (GdkEvent *e, GdkModifierType *state)
{
  guint s;
  switch (e->type) {
  case GDK_BUTTON_PRESS: case GDK_2BUTTON_PRESS: case GDK_3BUTTON_PRESS: case GDK_BUTTON_RELEASE: s = e->button.state; break;
  case GDK_MOTION_NOTIFY: s = e->motion.state; break;
  case GDK_KEY_PRESS: case GDK_KEY_RELEASE: s = e->key.state; break;
  case GDK_SCROLL: s = e->scroll.state; break;
  default: return FALSE;
  }
  if (state) *state = (GdkModifierType) s;
  return TRUE;
}

guint32 gdk_event_get_time (GdkEvent *e)
{
  switch (e->type) {
  case GDK_BUTTON_PRESS: case GDK_2BUTTON_PRESS: case GDK_3BUTTON_PRESS: case GDK_BUTTON_RELEASE: return e->button.time;
  case GDK_MOTION_NOTIFY: return e->motion.time;
  case GDK_KEY_PRESS: case GDK_KEY_RELEASE: return e->key.time;
  default: return GDK_CURRENT_TIME;
  }
}

GdkEvent *gdk_event_copy (GdkEvent *event)
{
  auto *c = (GdkEvent *) g_memdup2 (event, sizeof (GdkEvent));
  if ((c->type == GDK_KEY_PRESS || c->type == GDK_KEY_RELEASE) && c->key.string) c->key.string = g_strdup (c->key.string);
  return c;
}

void gdk_event_free (GdkEvent *event)
{
  if (!event) return;
  if (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE) g_free (event->key.string);
  g_free (event);
}

guint gdk_unicode_to_keyval (guint32 wc)
{
  if ((wc >= 0x20 && wc <= 0x7e) || (wc >= 0xa0 && wc <= 0xff)) return wc;
  switch (wc) { case '\b': return GDK_KEY_BackSpace; case '\t': return GDK_KEY_Tab; case '\r': case '\n': return GDK_KEY_Return;
                case 0x1b: return GDK_KEY_Escape; case 0x7f: return GDK_KEY_Delete; default: break; }
  return wc | 0x01000000;
}

guint32 gdk_keyval_to_unicode (guint keyval)
{
  if ((keyval >= 0x20 && keyval <= 0x7e) || (keyval >= 0xa0 && keyval <= 0xff)) return keyval;
  if ((keyval & 0xff000000) == 0x01000000) return keyval & 0x00ffffff;
  switch (keyval) {
  case GDK_KEY_BackSpace: return '\b';
  case GDK_KEY_Tab: return '\t';
  case GDK_KEY_Return: case GDK_KEY_KP_Enter: return '\r';
  case GDK_KEY_Escape: return 0x1b;
  case GDK_KEY_Delete: return 0x7f;
  case GDK_KEY_KP_Space: return ' ';
  default: break;
  }
  if (keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9) return '0' + (keyval - GDK_KEY_KP_0);
  return 0;
}

static const struct { const char *name; guint val; } key_names[] = {
  { "BackSpace", GDK_KEY_BackSpace }, { "Tab", GDK_KEY_Tab }, { "Return", GDK_KEY_Return }, { "Escape", GDK_KEY_Escape },
  { "Delete", GDK_KEY_Delete }, { "Insert", GDK_KEY_Insert }, { "Home", GDK_KEY_Home }, { "End", GDK_KEY_End },
  { "Left", GDK_KEY_Left }, { "Up", GDK_KEY_Up }, { "Right", GDK_KEY_Right }, { "Down", GDK_KEY_Down },
  { "Page_Up", GDK_KEY_Page_Up }, { "Page_Down", GDK_KEY_Page_Down }, { "space", GDK_KEY_space },
  { "KP_Enter", GDK_KEY_KP_Enter }, { "Shift_L", GDK_KEY_Shift_L }, { "Shift_R", GDK_KEY_Shift_R },
  { "Control_L", GDK_KEY_Control_L }, { "Control_R", GDK_KEY_Control_R }, { "Alt_L", GDK_KEY_Alt_L },
  { "Alt_R", GDK_KEY_Alt_R }, { "Super_L", GDK_KEY_Super_L }, { "Menu", GDK_KEY_Menu }, { "plus", GDK_KEY_plus },
  { "minus", GDK_KEY_minus }, { "equal", GDK_KEY_equal }, { "comma", GDK_KEY_comma }, { "period", GDK_KEY_period },
  { "slash", GDK_KEY_slash }, { "question", GDK_KEY_question },
};

const gchar *gdk_keyval_name (guint keyval)
{
  for (auto &k : key_names) if (k.val == keyval) return k.name;
  if (keyval >= GDK_KEY_F1 && keyval <= GDK_KEY_F24) {
    static char buf[24][4];
    int n = keyval - GDK_KEY_F1;
    g_snprintf (buf[n], 4, "F%d", n + 1);
    return buf[n];
  }
  if (keyval > 0x20 && keyval < 0x7f) {
    static char ascii[128][2];
    ascii[keyval][0] = (char) keyval;
    return ascii[keyval];
  }
  return nullptr;
}

guint gdk_keyval_from_name (const gchar *keyval_name)
{
  if (!keyval_name) return GDK_KEY_VoidSymbol;
  for (auto &k : key_names) if (!g_ascii_strcasecmp (k.name, keyval_name)) return k.val;
  if (keyval_name[0] && !keyval_name[1]) return (guchar) keyval_name[0];
  if ((keyval_name[0] == 'F' || keyval_name[0] == 'f') && g_ascii_isdigit (keyval_name[1])) {
    int n = atoi (keyval_name + 1);
    if (n >= 1 && n <= 24) return GDK_KEY_F1 + n - 1;
  }
  return GDK_KEY_VoidSymbol;
}

guint gdk_keyval_to_lower (guint keyval) { return keyval < 0x100 ? (guint) g_unichar_tolower (keyval) : keyval; }
guint gdk_keyval_to_upper (guint keyval) { return keyval < 0x100 ? (guint) g_unichar_toupper (keyval) : keyval; }

static bool parse_color (const gchar *spec, QColor *out)
{
  if (!spec) return false;
  QString s = QString::fromUtf8 (spec).trimmed ();
  static const QRegularExpression rgb (QStringLiteral ("^rgba?\\(\\s*([\\d.]+%?)\\s*,\\s*([\\d.]+%?)\\s*,\\s*([\\d.]+%?)\\s*(?:,\\s*([\\d.]+)\\s*)?\\)$"));
  auto m = rgb.match (s);
  if (m.hasMatch ()) {
    auto comp = [] (const QString &v) { return v.endsWith ('%') ? v.chopped (1).toDouble () / 100.0 : v.toDouble () / 255.0; };
    *out = QColor::fromRgbF (comp (m.captured (1)), comp (m.captured (2)), comp (m.captured (3)),
                             m.captured (4).isEmpty () ? 1.0 : m.captured (4).toDouble ());
    return true;
  }
  QColor c (s);
  if (!c.isValid ()) return false;
  *out = c;
  return true;
}

gboolean gdk_rgba_parse (GdkRGBA *rgba, const gchar *spec)
{
  QColor c;
  if (!parse_color (spec, &c)) return FALSE;
  rgba->red = c.redF (); rgba->green = c.greenF (); rgba->blue = c.blueF (); rgba->alpha = c.alphaF ();
  return TRUE;
}

gchar *gdk_rgba_to_string (const GdkRGBA *rgba)
{
  if (rgba->alpha > 0.999)
    return g_strdup_printf ("rgb(%d,%d,%d)", (int) (rgba->red * 255 + .5), (int) (rgba->green * 255 + .5), (int) (rgba->blue * 255 + .5));
  return g_strdup_printf ("rgba(%d,%d,%d,%g)", (int) (rgba->red * 255 + .5), (int) (rgba->green * 255 + .5),
                          (int) (rgba->blue * 255 + .5), rgba->alpha);
}

gboolean gdk_color_parse (const gchar *spec, GdkColor *color)
{
  QColor c;
  if (!parse_color (spec, &c)) return FALSE;
  color->red = c.red () * 257; color->green = c.green () * 257; color->blue = c.blue () * 257; color->pixel = 0;
  return TRUE;
}

void gdk_cairo_set_source_rgba (cairo_t *cr, const GdkRGBA *c) { cairo_set_source_rgba (cr, c->red, c->green, c->blue, c->alpha); }
void gdk_cairo_set_source_color (cairo_t *cr, const GdkColor *c) { cairo_set_source_rgb (cr, c->red / 65535.0, c->green / 65535.0, c->blue / 65535.0); }
void gdk_cairo_rectangle (cairo_t *cr, const GdkRectangle *r) { cairo_rectangle (cr, r->x, r->y, r->width, r->height); }

cairo_t *gdk_cairo_create (GdkWindow *window)
{
  if (cairo_surface_t *target = window ? gtkqt_drawable_target (window) : nullptr)
    return cairo_create (target);
  /* Outside a draw handler there is nothing to draw on (GTK 2 drew straight
   * to the window); hand out a scratch surface so the code still runs. */
  cairo_surface_t *s = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t *cr = cairo_create (s);
  cairo_surface_destroy (s);
  return cr;
}

int gdk_window_get_width (GdkWindow *window) { int w = 0; gdk_drawable_get_size (window, &w, nullptr); return w; }
int gdk_window_get_height (GdkWindow *window) { int h = 0; gdk_drawable_get_size (window, nullptr, &h); return h; }

void gdk_window_invalidate_rect (GdkWindow *window, const GdkRectangle *rect, gboolean)
{
  if (!window) return;
  if (window->qw) {
    if (rect) window->qw->update (rect->x, rect->y, rect->width, rect->height);
    else window->qw->update ();
    return;
  }
  if (!window->widget) return;
  if (rect) gtk_widget_queue_draw_area (window->widget, rect->x, rect->y, rect->width, rect->height);
  else gtk_widget_queue_draw (window->widget);
}

cairo_surface_t *gdk_window_create_similar_surface (GdkWindow *, cairo_content_t content, int width, int height)
{
  return cairo_image_surface_create (content == CAIRO_CONTENT_COLOR ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_ARGB32, width, height);
}

} /* extern "C" */

/* ======================================================================
 * GdkPixbuf (minimal, backed by QImage)
 * ====================================================================== */
struct _GdkPixbuf { GObject parent_instance; gpointer image; };
typedef struct { GObjectClass parent_class; } GdkPixbufClass;
G_DEFINE_TYPE (GdkPixbuf, gdk_pixbuf, G_TYPE_OBJECT)
static void gdk_pixbuf_init (GdkPixbuf *) {}
static void pixbuf_finalize (GObject *o)
{
  delete static_cast<QImage *> (((GdkPixbuf *) o)->image);
  G_OBJECT_CLASS (gdk_pixbuf_parent_class)->finalize (o);
}
static void gdk_pixbuf_class_init (GdkPixbufClass *klass) { G_OBJECT_CLASS (klass)->finalize = pixbuf_finalize; }

GdkPixbuf *gtkqt_pixbuf_from_image (const QImage &image)
{
  auto *p = (GdkPixbuf *) g_object_new (gdk_pixbuf_get_type (), nullptr);
  p->image = new QImage (image);
  return p;
}

QImage gtkqt_pixbuf_image (GdkPixbuf *pixbuf)
{
  return pixbuf && pixbuf->image ? *static_cast<QImage *> (pixbuf->image) : QImage ();
}

extern "C" {

GdkPixbuf *gdk_pixbuf_new_from_file (const char *filename, GError **error)
{
  QImage img (QString::fromUtf8 (filename));
  if (img.isNull ()) {
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT, "Failed to load image '%s'", filename);
    return nullptr;
  }
  return gtkqt_pixbuf_from_image (img);
}

GdkPixbuf *gdk_pixbuf_new_from_file_at_size (const char *filename, int width, int height, GError **error)
{
  GdkPixbuf *p = gdk_pixbuf_new_from_file (filename, error);
  if (p) {
    QImage *img = static_cast<QImage *> (p->image);
    *img = img->scaled (width > 0 ? width : img->width (), height > 0 ? height : img->height (), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }
  return p;
}

int gdk_pixbuf_get_width (const GdkPixbuf *pixbuf) { return pixbuf && pixbuf->image ? static_cast<QImage *> (pixbuf->image)->width () : 0; }
int gdk_pixbuf_get_height (const GdkPixbuf *pixbuf) { return pixbuf && pixbuf->image ? static_cast<QImage *> (pixbuf->image)->height () : 0; }

void gdk_cairo_set_source_pixbuf (cairo_t *cr, const GdkPixbuf *pixbuf, double x, double y)
{
  QImage img = gtkqt_pixbuf_image ((GdkPixbuf *) pixbuf).convertToFormat (QImage::Format_ARGB32_Premultiplied);
  cairo_surface_t *s = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, img.width (), img.height ());
  cairo_surface_flush (s);
  unsigned char *dst = cairo_image_surface_get_data (s);
  int stride = cairo_image_surface_get_stride (s);
  for (int row = 0; row < img.height (); row++) memcpy (dst + row * stride, img.constScanLine (row), img.width () * 4);
  cairo_surface_mark_dirty (s);
  cairo_set_source_surface (cr, s, x, y);
  cairo_surface_destroy (s);
}

/* ---------- display / screen / threads ---------- */
GdkDisplay *gdk_display_get_default (void) { gtkqt_ensure_app (); return (GdkDisplay *) qApp; }
GdkScreen *gdk_screen_get_default (void) { gtkqt_ensure_app (); return (GdkScreen *) QGuiApplication::primaryScreen (); }

gint gdk_screen_get_width (GdkScreen *) { QScreen *s = QGuiApplication::primaryScreen (); return s ? s->geometry ().width () : 0; }
gint gdk_screen_get_height (GdkScreen *) { QScreen *s = QGuiApplication::primaryScreen (); return s ? s->geometry ().height () : 0; }
gint gdk_screen_width (void) { gtkqt_ensure_app (); return gdk_screen_get_width (nullptr); }
gint gdk_screen_height (void) { gtkqt_ensure_app (); return gdk_screen_get_height (nullptr); }
void gdk_beep (void) { QApplication::beep (); }
void gdk_display_beep (GdkDisplay *) { QApplication::beep (); }
void gdk_flush (void) {}
void gdk_threads_init (void) {}
void gdk_threads_enter (void) {}
void gdk_threads_leave (void) {}
guint gdk_threads_add_idle (GSourceFunc function, gpointer data) { return g_idle_add (function, data); }
guint gdk_threads_add_timeout (guint interval, GSourceFunc function, gpointer data) { return g_timeout_add (interval, function, data); }

} /* extern "C" */
