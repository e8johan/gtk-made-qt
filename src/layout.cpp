/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkLayout: a QAbstractScrollArea whose viewport shows the layout's
 * bin_window. As in GTK the bin_window is the whole layout: programs draw
 * on it (in expose handlers and from idle handlers, as GnomeCanvas does)
 * and get its events in layout coordinates. The viewport keeps a backing
 * store of what is visible; its cairo surface carries the scroll offset.
 * The adjustments are the model: programs set their values and emit
 * "value-changed", the scroll bars follow them and set them.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

static void layout_scroll_to (GtkLayout *layout, int x, int y);
static void layout_allocate_children (GtkLayout *layout);

/* ---------- the viewport: the bin_window's backing store ---------- */
class GtkQtLayoutViewport : public QWidget, public GtkQtBackedWindow {
public:
  GtkLayout *layout;
  QImage backing;
  cairo_surface_t *surface = nullptr;
  bool painting = false, flush_pending = false, blit_only = false;

  explicit GtkQtLayoutViewport (GtkLayout *l) : layout (l)
  {
    setAttribute (Qt::WA_OpaquePaintEvent);
    setAutoFillBackground (false);
  }
  ~GtkQtLayoutViewport () override { if (surface) cairo_surface_destroy (surface); }

  QColor background () const
  {
    GdkWindow *bw = layout ? layout->bin_window : nullptr;
    if (bw && bw->has_background) return QColor (bw->background.red >> 8, bw->background.green >> 8, bw->background.blue >> 8);
    return palette ().color (QPalette::Base);
  }

  void ensure_backing ()
  {
    QSize want = size ().expandedTo (QSize (1, 1));
    if (backing.size () == want && surface) return;
    QImage n (want, QImage::Format_ARGB32_Premultiplied);
    n.fill (background ());
    if (!backing.isNull ()) { QPainter p (&n); p.drawImage (0, 0, backing); }
    backing = n;
    if (surface) cairo_surface_destroy (surface);
    surface = cairo_image_surface_create_for_data (backing.bits (), CAIRO_FORMAT_ARGB32, backing.width (), backing.height (),
                                                   backing.bytesPerLine ());
  }

  /* drawing on the bin_window: layout coordinates, shown once the program is done */
  cairo_surface_t *gdk_surface () override
  {
    ensure_backing ();
    cairo_surface_set_device_offset (surface, -layout->scroll_x, -layout->scroll_y);
    if (!painting && !flush_pending) {
      flush_pending = true;
      QPointer<GtkQtLayoutViewport> self (this);
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

  /* a child's size request changed (updateGeometry): GTK allocates it again */
  bool event (QEvent *e) override
  {
    if (e->type () == QEvent::LayoutRequest && layout) layout_allocate_children (layout);
    return QWidget::event (e);
  }

  /* QAbstractScrollArea hands the viewport's paint events to its own
     paintEvent, which calls this */
  void paint (QPaintEvent *e)
  {
    GtkWidget *w = GTK_WIDGET (layout);
    ensure_backing ();
    if (!blit_only && w && w->priv && !w->priv->destroyed) {
      {
        QPainter bp (&backing);
        bp.setCompositionMode (QPainter::CompositionMode_Source);
        for (const QRect &r : e->region ()) bp.fillRect (r, background ());
      }
      cairo_surface_mark_dirty (surface);
      cairo_surface_set_device_offset (surface, -layout->scroll_x, -layout->scroll_y);
      painting = true;
      {
        GtkQtEmitGuard guard;
        g_object_ref (w);
        /* the exposed area in the bin_window's (layout) coordinates */
        QRegion area = e->region ().translated (layout->scroll_x, layout->scroll_y);
        GdkRegion *region = gdk_region_new ();
        for (const QRect &r : area) {
          GdkRectangle g = { r.x (), r.y (), r.width (), r.height () };
          gdk_region_union_with_rect (region, &g);
        }
        QRect b = area.boundingRect ();
        GdkEvent ev = {};
        ev.expose.type = GDK_EXPOSE;
        ev.expose.window = layout->bin_window;
        ev.expose.area = { b.x (), b.y (), b.width (), b.height () };
        ev.expose.region = region;
        gtkqt_emit_event (w, &ev, "expose-event");
        gdk_region_destroy (region);
        g_object_unref (w);
      }
      painting = false;
    }
    cairo_surface_flush (surface);
    QPainter p (this);
    p.drawImage (0, 0, backing);
  }
};

class GtkQtLayoutArea : public QAbstractScrollArea {
public:
  GtkLayout *layout;
  GtkQtLayoutViewport *vp;
  explicit GtkQtLayoutArea (GtkLayout *l) : layout (l)
  {
    vp = new GtkQtLayoutViewport (l);
    setViewport (vp);
    setFrameShape (QFrame::NoFrame);
    setFocusPolicy (Qt::StrongFocus);
  }
  /* the adjustments say where the view is */
  void scrollContentsBy (int, int) override {}
  QSize sizeHint () const override { return QSize (0, 0); }
  QSize minimumSizeHint () const override { return QSize (0, 0); }
protected:
  void paintEvent (QPaintEvent *e) override { vp->paint (e); }
  /* the pointer events are the bin_window's (delivered from the viewport);
     QAbstractScrollArea ignores them, and Qt would hand them on to the
     area itself, in its own coordinates */
  void mousePressEvent (QMouseEvent *e) override { e->accept (); }
  void mouseReleaseEvent (QMouseEvent *e) override { e->accept (); }
  void mouseDoubleClickEvent (QMouseEvent *e) override { e->accept (); }
  void mouseMoveEvent (QMouseEvent *e) override { e->accept (); }
  void resizeEvent (QResizeEvent *e) override
  {
    QAbstractScrollArea::resizeEvent (e);
    /* GTK's size_allocate: a page is what is visible */
    GtkLayout *l = layout;
    QSize s = viewport ()->size ();
    for (int i = 0; i < 2; i++) {
      GtkAdjustment *adj = i == 0 ? l->hadjustment : l->vadjustment;
      if (!adj) continue;
      double page = i == 0 ? s.width () : s.height (), size = i == 0 ? l->width : l->height;
      adj->page_size = page;
      adj->page_increment = page * 0.9;
      adj->step_increment = page * 0.1;
      adj->lower = 0;
      adj->upper = qMax (size, page);
      g_signal_emit_by_name (adj, "changed");
      if (adj->value > adj->upper - adj->page_size) gtk_adjustment_set_value (adj, qMax (0.0, adj->upper - adj->page_size));
    }
  }
};

static GtkQtLayoutArea *layout_area (GtkLayout *l) { return dynamic_cast<GtkQtLayoutArea *> (gtkqt_qw (GTK_WIDGET (l))); }

/* ---------- adjustments ---------- */
static void layout_sync_bar (GtkLayout *l, GtkAdjustment *adj)
{
  GtkQtLayoutArea *a = layout_area (l);
  if (!a || !adj) return;
  QScrollBar *bar = adj == l->hadjustment ? a->horizontalScrollBar () : a->verticalScrollBar ();
  QSignalBlocker block (bar);
  bar->setRange ((int) adj->lower, (int) qMax (adj->lower, adj->upper - adj->page_size));
  bar->setPageStep ((int) qMax (1.0, adj->page_size));
  bar->setSingleStep ((int) qMax (1.0, adj->step_increment));
  bar->setValue (qRound (adj->value));
}

static void layout_adjustment_changed (GtkAdjustment *adj, GtkLayout *l) { layout_sync_bar (l, adj); }

static void layout_adjustment_value_changed (GtkAdjustment *adj, GtkLayout *l)
{
  layout_sync_bar (l, adj);
  layout_scroll_to (l, l->hadjustment ? (int) l->hadjustment->value : 0, l->vadjustment ? (int) l->vadjustment->value : 0);
}

static void layout_set_adjustment (GtkLayout *l, GtkAdjustment **slot, GtkAdjustment *adj, const char *prop)
{
  if (adj && adj == *slot) return;
  if (*slot) {
    g_signal_handlers_disconnect_by_data (*slot, l);
    g_object_unref (*slot);
  }
  if (!adj) adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 0, 0, 0));
  *slot = (GtkAdjustment *) g_object_ref_sink (adj);
  g_signal_connect (adj, "changed", G_CALLBACK (layout_adjustment_changed), l);
  g_signal_connect (adj, "value-changed", G_CALLBACK (layout_adjustment_value_changed), l);
  layout_sync_bar (l, adj);
  g_object_notify (G_OBJECT (l), prop);
}

/* children move with the layout, the view is drawn again */
static void layout_scroll_to (GtkLayout *l, int x, int y)
{
  if (x == l->scroll_x && y == l->scroll_y) return;
  l->scroll_x = x;
  l->scroll_y = y;
  l->bin_window->origin_x = -x;
  l->bin_window->origin_y = -y;
  for (GList *c = l->children; c; c = c->next) {
    auto *lc = static_cast<GtkLayoutChild *> (c->data);
    if (QWidget *cw = gtkqt_qw (lc->widget)) cw->move (lc->x - x, lc->y - y);
  }
  if (GtkQtLayoutArea *a = layout_area (l)) a->viewport ()->update ();
}

/* ---------- the type ---------- */
static QObject *layout_create (GtkWidget *w)
{
  auto *l = GTK_LAYOUT (w);
  auto *a = new GtkQtLayoutArea (l);
  l->bin_window->qw = a->vp;
  a->vp->installEventFilter (gtkqt_bridge (w));
  a->vp->setProperty ("_gtkqt_gdkwindow", QVariant::fromValue ((void *) l->bin_window));
  a->vp->setMouseTracking (true);
  QObject::connect (a->horizontalScrollBar (), &QScrollBar::valueChanged, a, [l] (int v) {
    if (l->hadjustment && qRound (l->hadjustment->value) != v) gtk_adjustment_set_value (l->hadjustment, v);
  });
  QObject::connect (a->verticalScrollBar (), &QScrollBar::valueChanged, a, [l] (int v) {
    if (l->vadjustment && qRound (l->vadjustment->value) != v) gtk_adjustment_set_value (l->vadjustment, v);
  });
  layout_sync_bar (l, l->hadjustment);
  layout_sync_bar (l, l->vadjustment);
  return a;
}

/* a child gets its requisition, as in GTK: the size request where one is
   set, its natural size otherwise */
static QSize layout_child_size (GtkWidget *child, QWidget *cw)
{
  QSize hint = cw->sizeHint ();
  int w = child->priv->width_request > 0 ? child->priv->width_request : hint.width ();
  int h = child->priv->height_request > 0 ? child->priv->height_request : hint.height ();
  return QSize (qMax (1, w), qMax (1, h));
}

static void layout_allocate_children (GtkLayout *l)
{
  for (GList *i = l->children; i; i = i->next) {
    auto *lc = static_cast<GtkLayoutChild *> (i->data);
    QWidget *cw = lc->widget && !lc->widget->priv->destroyed ? qobject_cast<QWidget *> (lc->widget->priv->obj.data ()) : nullptr;
    if (cw && cw->parentWidget ()) cw->resize (layout_child_size (lc->widget, cw));
  }
}

static void layout_place (GtkContainer *c, GtkWidget *child)
{
  GtkLayout *l = GTK_LAYOUT (c);
  GtkQtLayoutArea *a = layout_area (l);
  QWidget *cw = gtkqt_qw (child);
  if (!a || !cw) return;
  cw->setParent (a->vp);
  for (GList *i = l->children; i; i = i->next) {
    auto *lc = static_cast<GtkLayoutChild *> (i->data);
    if (lc->widget == child) {
      cw->move (lc->x - l->scroll_x, lc->y - l->scroll_y);
      cw->resize (layout_child_size (child, cw));
    }
  }
}

static void layout_unplace (GtkContainer *c, GtkWidget *child)
{
  GtkLayout *l = GTK_LAYOUT (c);
  for (GList *i = l->children; i; i = i->next)
    if (static_cast<GtkLayoutChild *> (i->data)->widget == child) {
      g_free (i->data);
      l->children = g_list_delete_link (l->children, i);
      break;
    }
  if (QWidget *cw = gtkqt_qw (child)) { cw->hide (); cw->setParent (nullptr); }
}

static void layout_add (GtkContainer *c, GtkWidget *child) { gtk_layout_put (GTK_LAYOUT (c), child, 0, 0); }

static void layout_dispose (GObject *o);
static void layout_finalize (GObject *o);

G_DEFINE_TYPE (GtkLayout, gtk_layout, GTK_TYPE_CONTAINER)

static void gtk_layout_init (GtkLayout *l)
{
  l->bin_window = gtkqt_drawable_new (GTKQT_DRAWABLE_WINDOW, GTK_WIDGET (l));
  l->visibility = GDK_VISIBILITY_UNOBSCURED;
}

static void layout_set_scroll_adjustments (GtkLayout *l, GtkAdjustment *h, GtkAdjustment *v)
{
  gtk_layout_set_hadjustment (l, h);
  gtk_layout_set_vadjustment (l, v);
}

static void gtk_layout_class_init (GtkLayoutClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = layout_dispose;
  G_OBJECT_CLASS (klass)->finalize = layout_finalize;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), layout_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), layout_place, layout_unplace);
  GTK_CONTAINER_CLASS (klass)->add = layout_add;
  klass->set_scroll_adjustments = layout_set_scroll_adjustments;
  g_signal_new ("set-scroll-adjustments", G_TYPE_FROM_CLASS (klass), (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                G_STRUCT_OFFSET (GtkLayoutClass, set_scroll_adjustments), nullptr, nullptr, nullptr, G_TYPE_NONE, 2,
                GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
  gtkqt_prop (klass, g_param_spec_object ("hadjustment", nullptr, nullptr, GTK_TYPE_ADJUSTMENT, RW),
              [] (GObject *o, const GValue *v) { gtk_layout_set_hadjustment (GTK_LAYOUT (o), (GtkAdjustment *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, GTK_LAYOUT (o)->hadjustment); });
  gtkqt_prop (klass, g_param_spec_object ("vadjustment", nullptr, nullptr, GTK_TYPE_ADJUSTMENT, RW),
              [] (GObject *o, const GValue *v) { gtk_layout_set_vadjustment (GTK_LAYOUT (o), (GtkAdjustment *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, GTK_LAYOUT (o)->vadjustment); });
  gtkqt_prop (klass, g_param_spec_uint ("width", nullptr, nullptr, 0, G_MAXINT, 100, RW),
              [] (GObject *o, const GValue *v) { gtk_layout_set_size (GTK_LAYOUT (o), g_value_get_uint (v), GTK_LAYOUT (o)->height); },
              [] (GObject *o, GValue *v) { g_value_set_uint (v, GTK_LAYOUT (o)->width); });
  gtkqt_prop (klass, g_param_spec_uint ("height", nullptr, nullptr, 0, G_MAXINT, 100, RW),
              [] (GObject *o, const GValue *v) { gtk_layout_set_size (GTK_LAYOUT (o), GTK_LAYOUT (o)->width, g_value_get_uint (v)); },
              [] (GObject *o, GValue *v) { g_value_set_uint (v, GTK_LAYOUT (o)->height); });
}

static void layout_dispose (GObject *o)
{
  auto *l = GTK_LAYOUT (o);
  for (GtkAdjustment **slot : { &l->hadjustment, &l->vadjustment })
    if (*slot) {
      g_signal_handlers_disconnect_by_data (*slot, l);
      g_object_unref (*slot);
      *slot = nullptr;
    }
  G_OBJECT_CLASS (gtk_layout_parent_class)->dispose (o);
}

static void layout_finalize (GObject *o)
{
  auto *l = GTK_LAYOUT (o);
  g_list_free_full (l->children, g_free);
  if (l->bin_window) {
    l->bin_window->qw = nullptr;
    l->bin_window->widget = nullptr;
    g_object_unref (l->bin_window);
  }
  G_OBJECT_CLASS (gtk_layout_parent_class)->finalize (o);
}

extern "C" {

GtkWidget *gtk_layout_new (GtkAdjustment *hadjustment, GtkAdjustment *vadjustment)
{
  auto *l = (GtkLayout *) g_object_new (GTK_TYPE_LAYOUT, nullptr);
  gtk_layout_set_hadjustment (l, hadjustment);
  gtk_layout_set_vadjustment (l, vadjustment);
  if (!l->width) gtk_layout_set_size (l, 100, 100);
  return GTK_WIDGET (l);
}

GdkWindow *gtk_layout_get_bin_window (GtkLayout *l) { return l->bin_window; }

void gtk_layout_put (GtkLayout *l, GtkWidget *child, gint x, gint y)
{
  if (!child || child->parent) return;
  auto *lc = g_new0 (GtkLayoutChild, 1);
  lc->widget = child;
  lc->x = x;
  lc->y = y;
  l->children = g_list_append (l->children, lc);
  gtkqt_container_adopt (GTK_CONTAINER (l), child);
}

void gtk_layout_move (GtkLayout *l, GtkWidget *child, gint x, gint y)
{
  for (GList *i = l->children; i; i = i->next) {
    auto *lc = static_cast<GtkLayoutChild *> (i->data);
    if (lc->widget != child) continue;
    lc->x = x;
    lc->y = y;
    if (QWidget *cw = gtkqt_qw (child)) cw->move (x - l->scroll_x, y - l->scroll_y);
  }
}

/* the bin_window's size; the adjustments' upper follow it */
void gtk_layout_set_size (GtkLayout *l, guint width, guint height)
{
  l->width = width;
  l->height = height;
  l->bin_window->width = width;
  l->bin_window->height = height;
  for (int i = 0; i < 2; i++) {
    GtkAdjustment *adj = i == 0 ? l->hadjustment : l->vadjustment;
    if (!adj) continue;
    adj->upper = qMax ((double) (i == 0 ? width : height), adj->page_size);
    g_signal_emit_by_name (adj, "changed");
    if (adj->value > adj->upper - adj->page_size) gtk_adjustment_set_value (adj, qMax (0.0, adj->upper - adj->page_size));
  }
  g_object_notify (G_OBJECT (l), "width");
  g_object_notify (G_OBJECT (l), "height");
}

void gtk_layout_get_size (GtkLayout *l, guint *width, guint *height)
{
  if (width) *width = l->width;
  if (height) *height = l->height;
}

GtkAdjustment *gtk_layout_get_hadjustment (GtkLayout *l) { return l->hadjustment; }
GtkAdjustment *gtk_layout_get_vadjustment (GtkLayout *l) { return l->vadjustment; }
void gtk_layout_set_hadjustment (GtkLayout *l, GtkAdjustment *adj) { layout_set_adjustment (l, &l->hadjustment, adj, "hadjustment"); }
void gtk_layout_set_vadjustment (GtkLayout *l, GtkAdjustment *adj) { layout_set_adjustment (l, &l->vadjustment, adj, "vadjustment"); }

void gtk_layout_freeze (GtkLayout *l)
{
  if (l->freeze_count++ == 0)
    if (GtkQtLayoutArea *a = layout_area (l)) a->viewport ()->setUpdatesEnabled (false);
}

void gtk_layout_thaw (GtkLayout *l)
{
  if (l->freeze_count > 0 && --l->freeze_count == 0)
    if (GtkQtLayoutArea *a = layout_area (l)) a->viewport ()->setUpdatesEnabled (true);
}

} /* extern "C" */
