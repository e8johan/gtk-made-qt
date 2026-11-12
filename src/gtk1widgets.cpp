/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GTK 1.2 widgets: GtkPixmap, GtkAspectFrame, GtkCList (QTreeWidget),
 * GtkList/GtkListItem, GtkText (QTextEdit), the GtkProgress API, GtkRuler,
 * GtkCalendar (QCalendarWidget), GtkPacker (Tk's packer as a QLayout),
 * GtkTooltips, GtkColorSelection (QColorDialog).
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QBitmap>

#include <algorithm>
#include <cmath>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

/* A GdkPixmap (+ mask) as a QPixmap */
QPixmap gtkqt_qpixmap (GdkPixmap *pixmap, GdkBitmap *mask)
{
  if (!pixmap || !pixmap->surface) return QPixmap ();
  cairo_surface_t *s = pixmap->surface;
  cairo_surface_flush (s);
  int w = cairo_image_surface_get_width (s), h = cairo_image_surface_get_height (s), stride = cairo_image_surface_get_stride (s);
  QImage img (w, h, QImage::Format_ARGB32);
  const uchar *data = cairo_image_surface_get_data (s);
  const uchar *mdata = nullptr;
  int mstride = 0;
  if (mask && mask->surface) {
    cairo_surface_flush (mask->surface);
    mdata = cairo_image_surface_get_data (mask->surface);
    mstride = cairo_image_surface_get_stride (mask->surface);
  }
  for (int y = 0; y < h; y++) {
    const quint32 *in = reinterpret_cast<const quint32 *> (data + y * stride);
    quint32 *out = reinterpret_cast<quint32 *> (img.scanLine (y));
    for (int x = 0; x < w; x++) {
      bool opaque = !mdata || (y < mask->height && x < mask->width && mdata[y * mstride + x]);
      out[x] = opaque ? (0xff000000u | (in[x] & 0xffffff)) : 0;
    }
  }
  return QPixmap::fromImage (img);
}

/* ======================================================================
 * GtkPixmap
 * ====================================================================== */
static QObject *pixmap_create (GtkWidget *)
{
  auto *l = new QLabel;
  l->setAlignment (Qt::AlignCenter);
  return l;
}
G_DEFINE_TYPE (GtkPixmap, gtk_pixmap, GTK_TYPE_WIDGET)
static void gtk_pixmap_init (GtkPixmap *) {}
static void pixmap_dispose (GObject *o)
{
  auto *p = GTK_PIXMAP (o);
  if (p->pixmap) { g_object_unref (p->pixmap); p->pixmap = nullptr; }
  if (p->mask) { g_object_unref (p->mask); p->mask = nullptr; }
  G_OBJECT_CLASS (gtk_pixmap_parent_class)->dispose (o);
}
static void gtk_pixmap_class_init (GtkPixmapClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = pixmap_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), pixmap_create);
}

extern "C" {

GtkWidget *gtk_pixmap_new (GdkPixmap *pixmap, GdkBitmap *mask)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_PIXMAP, nullptr);
  gtk_pixmap_set (GTK_PIXMAP (w), pixmap, mask);
  return w;
}

void gtk_pixmap_set (GtkPixmap *p, GdkPixmap *val, GdkBitmap *mask)
{
  if (val) g_object_ref (val);
  if (mask) g_object_ref (mask);
  if (p->pixmap) g_object_unref (p->pixmap);
  if (p->mask) g_object_unref (p->mask);
  p->pixmap = val;
  p->mask = mask;
  if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (p))) l->setPixmap (gtkqt_qpixmap (val, mask));
}

void gtk_pixmap_get (GtkPixmap *p, GdkPixmap **val, GdkBitmap **mask)
{
  if (val) *val = p->pixmap;
  if (mask) *mask = p->mask;
}
void gtk_pixmap_set_build_insensitive (GtkPixmap *p, gboolean build) { p->build_insensitive = build; }

} /* extern "C" */

/* ======================================================================
 * GtkAspectFrame (a frame; the child keeps its natural shape)
 * ====================================================================== */
G_DEFINE_TYPE (GtkAspectFrame, gtk_aspect_frame, GTK_TYPE_FRAME)
static void gtk_aspect_frame_init (GtkAspectFrame *f) { f->xalign = f->yalign = 0.5; f->ratio = 1; f->obey_child = TRUE; }
static void gtk_aspect_frame_class_init (GtkAspectFrameClass *) {}

extern "C" {
GtkWidget *gtk_aspect_frame_new (const gchar *label, gfloat xalign, gfloat yalign, gfloat ratio, gboolean obey_child)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_ASPECT_FRAME, nullptr);
  if (label) gtk_frame_set_label (GTK_FRAME (w), label);
  gtk_aspect_frame_set (GTK_ASPECT_FRAME (w), xalign, yalign, ratio, obey_child);
  return w;
}
void gtk_aspect_frame_set (GtkAspectFrame *f, gfloat xalign, gfloat yalign, gfloat ratio, gboolean obey_child)
{
  f->xalign = xalign; f->yalign = yalign; f->ratio = ratio; f->obey_child = obey_child;
}
}

/* ======================================================================
 * GtkCList: a QTreeWidget without tree decoration. Rows are kept in a
 * vector of GtkCListRow (public: row_list, selection, rows ...).
 * ====================================================================== */
struct CListCell { QString text; GdkPixmap *pixmap = nullptr; GdkBitmap *mask = nullptr; };
struct CListRowData {
  GtkCListRow row = {};          /* must be first: GTK_CLIST_ROW (list) points here */
  QVector<CListCell> cells;
  QVector<GtkCell> gcells;       /* GTK 1's public view: row->cell[column] */
  QTreeWidgetItem *item = nullptr;
  GtkCTreeRow *tree = nullptr;   /* GtkCTree rows */
};
struct CListData {
  QList<CListRowData *> rows;
  int frozen = 0;
  bool syncing = false;
  bool auto_sort = false;
  bool in_press = false;   /* selection changes now come from this button press */
  GdkEventButton press = {};
};
static CListData *cld (GtkCList *c) { return static_cast<CListData *> (c->priv); }
static QTreeWidget *ctw (GtkCList *c) { return gtkqt_q<QTreeWidget> (GTK_WIDGET (c)); }

static void clist_sync_public (GtkCList *c)
{
  CListData *d = cld (c);
  g_list_free (c->row_list);
  c->row_list = nullptr;
  for (CListRowData *r : d->rows) c->row_list = g_list_prepend (c->row_list, &r->row);
  c->row_list = g_list_reverse (c->row_list);
  c->row_list_end = g_list_last (c->row_list);
  c->rows = d->rows.size ();
}

static void cell_free (CListCell &cell)
{
  if (cell.pixmap) g_object_unref (cell.pixmap);
  if (cell.mask) g_object_unref (cell.mask);
  cell.pixmap = cell.mask = nullptr;
}

static void row_sync_cells (CListRowData *r)
{
  if (r->gcells.size () != r->cells.size ()) {
    for (GtkCell &g : r->gcells) if (g.type != GTK_CELL_PIXMAP) g_free (g.u.text);
    r->gcells = QVector<GtkCell> (r->cells.size ());
    for (GtkCell &g : r->gcells) memset (&g, 0, sizeof g);
  }
  for (int i = 0; i < r->cells.size (); i++) {
    GtkCell &g = r->gcells[i];
    const CListCell &c = r->cells[i];
    if (g.type != GTK_CELL_PIXMAP) g_free (g.u.text);
    memset (&g.u, 0, sizeof g.u);
    if (c.pixmap && c.text.isEmpty ()) { g.type = GTK_CELL_PIXMAP; g.u.pm.pixmap = c.pixmap; g.u.pm.mask = c.mask; }
    else if (c.pixmap) { g.type = GTK_CELL_PIXTEXT; g.u.pt.text = g_strdup (c.text.toUtf8 ().constData ()); g.u.pt.pixmap = c.pixmap; g.u.pt.mask = c.mask; }
    else { g.type = GTK_CELL_TEXT; g.u.text = g_strdup (c.text.toUtf8 ().constData ()); }
  }
  r->row.cell = r->gcells.data ();
}

static void row_update_item (GtkCList *c, CListRowData *r)
{
  row_sync_cells (r);
  for (int col = 0; col < r->cells.size (); col++) {
    r->item->setText (col, r->cells[col].text);
    r->item->setIcon (col, r->cells[col].pixmap ? QIcon (gtkqt_qpixmap (r->cells[col].pixmap, r->cells[col].mask)) : QIcon ());
    if (c->column && c->column[col].justification != GTK_JUSTIFY_LEFT)
      r->item->setTextAlignment (col, c->column[col].justification == GTK_JUSTIFY_RIGHT ? Qt::AlignRight | Qt::AlignVCenter : Qt::AlignCenter);
  }
}

static int clist_row_of_item (GtkCList *c, QTreeWidgetItem *it)
{
  CListData *d = cld (c);
  for (int i = 0; i < d->rows.size (); i++) if (d->rows[i]->item == it) return i;
  return -1;
}

static void clist_rebuild_selection (GtkCList *c)
{
  g_list_free (c->selection);
  c->selection = nullptr;
  CListData *d = cld (c);
  for (int i = 0; i < d->rows.size (); i++)
    if (d->rows[i]->row.state == GTK_STATE_SELECTED) c->selection = g_list_append (c->selection, GINT_TO_POINTER (i));
  c->selection_end = g_list_last (c->selection);
}

/* user selection in Qt -> select_row / unselect_row signals */
static void clist_qt_selection_changed (GtkWidget *w)
{
  auto *c = GTK_CLIST (w);
  CListData *d = cld (c);
  if (d->syncing) return;
  QList<int> sel, unsel;
  for (int i = 0; i < d->rows.size (); i++) {
    bool now = d->rows[i]->item->isSelected ();
    bool was = d->rows[i]->row.state == GTK_STATE_SELECTED;
    if (now && !was) sel << i;
    if (!now && was) unsel << i;
  }
  QTreeWidget *tw = ctw (c);
  int col = tw ? qMax (0, tw->currentColumn ()) : 0;
  /* as GTK 1, a selection made with the mouse comes with its button event */
  GdkEventButton *ev = d->in_press ? &d->press : nullptr;
  for (int i : unsel) g_signal_emit_by_name (c, "unselect-row", i, col, ev);
  for (int i : sel) g_signal_emit_by_name (c, "select-row", i, col, ev);
}

/* records the button event behind a selection; a double click selects the
   row again with a GDK_2BUTTON_PRESS event, which GTK 1 programs (and file
   selections) take as "open" */
class CListMouse : public QObject {
public:
  CListMouse (GtkWidget *w, QObject *parent) : QObject (parent), w (w) {}
protected:
  bool eventFilter (QObject *o, QEvent *e) override
  {
    auto t = e->type ();
    if (t != QEvent::MouseButtonPress && t != QEvent::MouseButtonDblClick && t != QEvent::MouseButtonRelease) return false;
    CListData *d = GTK_IS_CLIST (w) ? cld (GTK_CLIST (w)) : nullptr;
    if (!d) return false;
    if (t == QEvent::MouseButtonRelease) { d->in_press = false; return false; }
    auto *me = static_cast<QMouseEvent *> (e);
    auto *vp = static_cast<QWidget *> (o);
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
    QPointF lp = me->position (), gp = me->globalPosition ();
#else
    QPointF lp = me->localPos (), gp = me->screenPos ();
#endif
    d->press = {};
    d->press.type = t == QEvent::MouseButtonDblClick ? GDK_2BUTTON_PRESS : GDK_BUTTON_PRESS;
    d->press.window = w->window;
    d->press.time = me->timestamp ();
    d->press.x = lp.x ();
    d->press.y = lp.y ();
    d->press.x_root = gp.x ();
    d->press.y_root = gp.y ();
    d->press.button = me->button () == Qt::RightButton ? 3 : me->button () == Qt::MiddleButton ? 2 : 1;
    d->press.state = gtkqt_qt_mods_to_gdk (me->modifiers (), me->buttons ());
    d->in_press = true;
    if (t == QEvent::MouseButtonDblClick) {
      QTreeWidget *tw = ctw (GTK_CLIST (w));
      QTreeWidgetItem *it = tw ? tw->itemAt (lp.toPoint ()) : nullptr;
      int row = -1;
      for (int i = 0; it && i < d->rows.size (); i++) if (d->rows[i]->item == it) row = i;
      if (row >= 0) {
        int col = qMax (0, tw->columnAt (lp.toPoint ().x ()));
        GdkEventButton ev = d->press;
        GtkWidget *cw = w;
        g_object_ref (cw);
        QTimer::singleShot (0, vp, [cw, row, col, ev] () mutable {   /* after Qt has handled the click */
          if (cw->priv && !cw->priv->destroyed && row < GTK_CLIST (cw)->rows)
            g_signal_emit_by_name (cw, "select-row", row, col, &ev);
          g_object_unref (cw);
        });
      }
    }
    return false;
  }
private:
  GtkWidget *w;
};

static void clist_real_select (GtkCList *c, gint row, gint, GdkEvent *)
{
  CListData *d = cld (c);
  if (row < 0 || row >= d->rows.size ()) return;
  if (c->selection_mode == GTK_SELECTION_SINGLE || c->selection_mode == GTK_SELECTION_BROWSE)
    for (int i = 0; i < d->rows.size (); i++)
      if (i != row && d->rows[i]->row.state == GTK_STATE_SELECTED) {
        d->rows[i]->row.state = GTK_STATE_NORMAL;
        d->syncing = true;
        d->rows[i]->item->setSelected (false);
        d->syncing = false;
      }
  d->rows[row]->row.state = GTK_STATE_SELECTED;
  d->syncing = true;
  d->rows[row]->item->setSelected (true);
  if (QTreeWidget *tw = ctw (c)) tw->setCurrentItem (d->rows[row]->item, 0, QItemSelectionModel::NoUpdate);
  d->syncing = false;
  c->focus_row = row;
  clist_rebuild_selection (c);
}

static void clist_real_unselect (GtkCList *c, gint row, gint, GdkEvent *)
{
  CListData *d = cld (c);
  if (row < 0 || row >= d->rows.size ()) return;
  d->rows[row]->row.state = GTK_STATE_NORMAL;
  d->syncing = true;
  d->rows[row]->item->setSelected (false);
  d->syncing = false;
  clist_rebuild_selection (c);
}

static QObject *clist_create (GtkWidget *w)
{
  auto *tw = new QTreeWidget;
  tw->setRootIsDecorated (GTK_IS_CTREE (w));
  tw->setUniformRowHeights (true);
  tw->setAllColumnsShowFocus (true);
  tw->header ()->setStretchLastSection (true);
  tw->header ()->setSectionsClickable (true);
  tw->setSelectionMode (QAbstractItemView::SingleSelection);
  gtkqt_connect (w, tw, &QTreeWidget::itemSelectionChanged, [] (GtkWidget *cw) { clist_qt_selection_changed (cw); });
  gtkqt_connect (w, tw->header (), &QHeaderView::sectionClicked, [] (GtkWidget *cw, int col) {
    g_signal_emit_by_name (cw, "click-column", col);
  });
  /* GTK 1 programs watch button presses on the list itself */
  tw->viewport ()->installEventFilter (new CListMouse (w, tw));
  tw->viewport ()->installEventFilter (gtkqt_bridge (w));
  return tw;
}

static void clist_dispose (GObject *o);
G_DEFINE_TYPE (GtkCList, gtk_clist, GTK_TYPE_CONTAINER)
static void gtk_clist_init (GtkCList *c)
{
  c->priv = new CListData;
  c->selection_mode = GTK_SELECTION_SINGLE;
  c->focus_row = -1;
}

static void row_free (CListRowData *r)
{
  if (r->row.destroy) r->row.destroy (r->row.data);
  for (CListCell &cell : r->cells) cell_free (cell);
  for (GtkCell &g : r->gcells) if (g.type != GTK_CELL_PIXMAP) g_free (g.u.text);
  delete r->tree;
  delete r;
}

static void clist_dispose (GObject *o)
{
  auto *c = GTK_CLIST (o);
  if (CListData *d = cld (c)) {
    for (CListRowData *r : d->rows) row_free (r);
    delete d;
    c->priv = nullptr;
    g_list_free (c->row_list); c->row_list = nullptr;
    g_list_free (c->selection); c->selection = nullptr;
    if (c->column) {
      for (int i = 0; i < c->columns; i++) g_free (c->column[i].title);
      g_free (c->column);
      c->column = nullptr;
    }
  }
  G_OBJECT_CLASS (gtk_clist_parent_class)->dispose (o);
}

static void gtk_clist_class_init (GtkCListClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = clist_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), clist_create);
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("select-row", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkCListClass, select_row), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER);
  g_signal_new ("unselect-row", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkCListClass, unselect_row), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER);
  g_signal_new ("click-column", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkCListClass, click_column), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 1, G_TYPE_INT);
  g_signal_new ("resize-column", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  g_signal_new ("row-move", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  klass->select_row = clist_real_select;
  klass->unselect_row = clist_real_unselect;
}

static void clist_resort_if_needed (GtkCList *c) { if (cld (c)->auto_sort) gtk_clist_sort (c); }

static CListRowData *clist_new_row (GtkCList *c, gchar *text[])
{
  auto *r = new CListRowData;
  r->row.state = GTK_STATE_NORMAL;
  r->row.selectable = 1;
  r->cells.resize (c->columns);
  for (int i = 0; i < c->columns; i++) r->cells[i].text = text && text[i] ? QString::fromUtf8 (text[i]) : QString ();
  r->item = new QTreeWidgetItem;
  return r;
}

extern "C" {

void gtk_clist_construct (GtkCList *c, gint columns, gchar *titles[])
{
  c->columns = qMax (1, columns);
  c->column = g_new0 (GtkCListColumn, c->columns);
  QTreeWidget *tw = ctw (c);
  QStringList labels;
  for (int i = 0; i < c->columns; i++) {
    c->column[i].title = g_strdup (titles && titles[i] ? titles[i] : "");
    c->column[i].visible = TRUE;
    labels << QString::fromUtf8 (c->column[i].title);
  }
  if (tw) {
    tw->setColumnCount (c->columns);
    tw->setHeaderLabels (labels);
    tw->setHeaderHidden (!titles);
  }
}

GtkWidget *gtk_clist_new (gint columns)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_CLIST, nullptr);
  gtk_clist_construct (GTK_CLIST (w), columns, nullptr);
  return w;
}

GtkWidget *gtk_clist_new_with_titles (gint columns, gchar *titles[])
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_CLIST, nullptr);
  gtk_clist_construct (GTK_CLIST (w), columns, titles);
  return w;
}

gint gtk_clist_insert (GtkCList *c, gint row, gchar *text[])
{
  CListData *d = cld (c);
  if (row < 0 || row > d->rows.size ()) row = d->rows.size ();
  CListRowData *r = clist_new_row (c, text);
  d->rows.insert (row, r);
  if (QTreeWidget *tw = ctw (c)) tw->insertTopLevelItem (row, r->item);
  row_update_item (c, r);
  clist_sync_public (c);
  clist_rebuild_selection (c);
  if (c->selection_mode == GTK_SELECTION_BROWSE && !c->selection) gtk_clist_select_row (c, row, 0);
  if (d->auto_sort) { gtk_clist_sort (c); return d->rows.indexOf (r); }
  return row;
}
gint gtk_clist_append (GtkCList *c, gchar *text[]) { return gtk_clist_insert (c, -1, text); }
gint gtk_clist_prepend (GtkCList *c, gchar *text[]) { return gtk_clist_insert (c, 0, text); }

void gtk_clist_remove (GtkCList *c, gint row)
{
  CListData *d = cld (c);
  if (row < 0 || row >= d->rows.size ()) return;
  if (d->rows[row]->row.state == GTK_STATE_SELECTED) g_signal_emit_by_name (c, "unselect-row", row, 0, nullptr);
  CListRowData *r = d->rows.takeAt (row);
  d->syncing = true;
  delete r->item;
  d->syncing = false;
  row_free (r);
  clist_sync_public (c);
  clist_rebuild_selection (c);
}

void gtk_clist_clear (GtkCList *c)
{
  CListData *d = cld (c);
  d->syncing = true;
  if (QTreeWidget *tw = ctw (c)) tw->clear ();
  d->syncing = false;
  for (CListRowData *r : d->rows) { r->item = nullptr; row_free (r); }
  d->rows.clear ();
  clist_sync_public (c);
  clist_rebuild_selection (c);
  c->focus_row = -1;
}

void gtk_clist_freeze (GtkCList *c) { if (cld (c)->frozen++ == 0) if (QTreeWidget *tw = ctw (c)) tw->setUpdatesEnabled (false); }
void gtk_clist_thaw (GtkCList *c) { if (cld (c)->frozen > 0 && --cld (c)->frozen == 0) if (QTreeWidget *tw = ctw (c)) tw->setUpdatesEnabled (true); }

static CListRowData *row_at (GtkCList *c, gint row) { CListData *d = cld (c); return row >= 0 && row < d->rows.size () ? d->rows[row] : nullptr; }

void gtk_clist_set_text (GtkCList *c, gint row, gint col, const gchar *text)
{
  CListRowData *r = row_at (c, row);
  if (!r || col < 0 || col >= c->columns) return;
  cell_free (r->cells[col]);
  r->cells[col].text = QString::fromUtf8 (text ? text : "");
  row_update_item (c, r);
}

gint gtk_clist_get_text (GtkCList *c, gint row, gint col, gchar **text)
{
  CListRowData *r = row_at (c, row);
  if (!r || col < 0 || col >= c->columns) return 0;
  /* GTK returns a pointer into the row; keep a UTF-8 copy per cell */
  QByteArray key = QByteArray ("gtkqt-cell-") + QByteArray::number (row) + "-" + QByteArray::number (col);
  if (text) {
    *text = g_strdup (r->cells[col].text.toUtf8 ().constData ());
    g_object_set_data_full (G_OBJECT (c), key.constData (), *text, g_free);
  }
  return 1;
}

void gtk_clist_set_pixmap (GtkCList *c, gint row, gint col, GdkPixmap *pixmap, GdkBitmap *mask)
{
  gtk_clist_set_pixtext (c, row, col, nullptr, 0, pixmap, mask);
}
gint gtk_clist_get_pixmap (GtkCList *c, gint row, gint col, GdkPixmap **pixmap, GdkBitmap **mask)
{
  return gtk_clist_get_pixtext (c, row, col, nullptr, nullptr, pixmap, mask);
}
void gtk_clist_set_pixtext (GtkCList *c, gint row, gint col, const gchar *text, guint8, GdkPixmap *pixmap, GdkBitmap *mask)
{
  CListRowData *r = row_at (c, row);
  if (!r || col < 0 || col >= c->columns) return;
  if (pixmap) g_object_ref (pixmap);
  if (mask) g_object_ref (mask);
  cell_free (r->cells[col]);
  r->cells[col].pixmap = pixmap;
  r->cells[col].mask = mask;
  r->cells[col].text = QString::fromUtf8 (text ? text : "");
  row_update_item (c, r);
}
gint gtk_clist_get_pixtext (GtkCList *c, gint row, gint col, gchar **text, guint8 *spacing, GdkPixmap **pixmap, GdkBitmap **mask)
{
  CListRowData *r = row_at (c, row);
  if (!r || col < 0 || col >= c->columns) return 0;
  if (text) gtk_clist_get_text (c, row, col, text);
  if (spacing) *spacing = 0;
  if (pixmap) *pixmap = r->cells[col].pixmap;
  if (mask) *mask = r->cells[col].mask;
  return 1;
}
GtkCellType gtk_clist_get_cell_type (GtkCList *c, gint row, gint col)
{
  CListRowData *r = row_at (c, row);
  if (!r || col < 0 || col >= c->columns) return GTK_CELL_EMPTY;
  if (r->cells[col].pixmap) return r->cells[col].text.isEmpty () ? GTK_CELL_PIXMAP : GTK_CELL_PIXTEXT;
  return GTK_CELL_TEXT;
}

void gtk_clist_set_row_data_full (GtkCList *c, gint row, gpointer data, GDestroyNotify destroy)
{
  CListRowData *r = row_at (c, row);
  if (!r) return;
  if (r->row.destroy) r->row.destroy (r->row.data);
  r->row.data = data;
  r->row.destroy = destroy;
}
void gtk_clist_set_row_data (GtkCList *c, gint row, gpointer data) { gtk_clist_set_row_data_full (c, row, data, nullptr); }
gpointer gtk_clist_get_row_data (GtkCList *c, gint row) { CListRowData *r = row_at (c, row); return r ? r->row.data : nullptr; }
gint gtk_clist_find_row_from_data (GtkCList *c, gpointer data)
{
  CListData *d = cld (c);
  for (int i = 0; i < d->rows.size (); i++) if (d->rows[i]->row.data == data) return i;
  return -1;
}

void gtk_clist_select_row (GtkCList *c, gint row, gint col)
{
  CListRowData *r = row_at (c, row);
  if (!r || !r->row.selectable || c->selection_mode == GTK_SELECTION_NONE) return;
  g_signal_emit_by_name (c, "select-row", row, col, nullptr);
}
void gtk_clist_unselect_row (GtkCList *c, gint row, gint col)
{
  if (row_at (c, row)) g_signal_emit_by_name (c, "unselect-row", row, col, nullptr);
}
void gtk_clist_select_all (GtkCList *c)
{
  if (c->selection_mode != GTK_SELECTION_MULTIPLE && c->selection_mode != (GtkSelectionMode) 4) return;
  for (int i = 0; i < c->rows; i++) if (row_at (c, i)->row.state != GTK_STATE_SELECTED) gtk_clist_select_row (c, i, 0);
}
void gtk_clist_unselect_all (GtkCList *c)
{
  for (int i = 0; i < c->rows; i++) if (row_at (c, i)->row.state == GTK_STATE_SELECTED) gtk_clist_unselect_row (c, i, 0);
}
void gtk_clist_undo_selection (GtkCList *) {}

void gtk_clist_set_selection_mode (GtkCList *c, GtkSelectionMode mode)
{
  c->selection_mode = mode;
  if (QTreeWidget *tw = ctw (c))
    tw->setSelectionMode (mode == GTK_SELECTION_NONE ? QAbstractItemView::NoSelection
                          : mode == GTK_SELECTION_MULTIPLE ? QAbstractItemView::ExtendedSelection   /* = GTK_SELECTION_EXTENDED */
                          : QAbstractItemView::SingleSelection);
}
void gtk_clist_set_selectable (GtkCList *c, gint row, gboolean selectable)
{
  if (CListRowData *r = row_at (c, row)) {
    r->row.selectable = selectable;
    r->item->setFlags (selectable ? r->item->flags () | Qt::ItemIsSelectable : r->item->flags () & ~Qt::ItemIsSelectable);
  }
}
gboolean gtk_clist_get_selectable (GtkCList *c, gint row) { CListRowData *r = row_at (c, row); return r && r->row.selectable; }

void gtk_clist_set_column_title (GtkCList *c, gint col, const gchar *title)
{
  if (col < 0 || col >= c->columns) return;
  g_free (c->column[col].title);
  c->column[col].title = g_strdup (title);
  if (QTreeWidget *tw = ctw (c)) tw->headerItem ()->setText (col, gtkqt_str (title));
}
gchar *gtk_clist_get_column_title (GtkCList *c, gint col) { return col >= 0 && col < c->columns ? c->column[col].title : nullptr; }
void gtk_clist_set_column_widget (GtkCList *, gint, GtkWidget *widget) { if (widget) g_object_ref_sink (widget); }
void gtk_clist_set_column_width (GtkCList *c, gint col, gint width)
{
  if (col < 0 || col >= c->columns) return;
  c->column[col].width = width;
  if (QTreeWidget *tw = ctw (c)) tw->setColumnWidth (col, width + 8);
}
void gtk_clist_set_column_min_width (GtkCList *, gint, gint) {}
void gtk_clist_set_column_max_width (GtkCList *, gint, gint) {}
void gtk_clist_set_column_justification (GtkCList *c, gint col, GtkJustification j)
{
  if (col < 0 || col >= c->columns) return;
  c->column[col].justification = j;
  for (CListRowData *r : cld (c)->rows) row_update_item (c, r);
}
void gtk_clist_set_column_visibility (GtkCList *c, gint col, gboolean visible)
{
  if (col < 0 || col >= c->columns) return;
  c->column[col].visible = visible;
  if (QTreeWidget *tw = ctw (c)) tw->setColumnHidden (col, !visible);
}
void gtk_clist_set_column_resizeable (GtkCList *, gint, gboolean) {}
void gtk_clist_set_column_auto_resize (GtkCList *c, gint col, gboolean auto_resize)
{
  if (QTreeWidget *tw = ctw (c); tw && col >= 0 && col < c->columns)
    tw->header ()->setSectionResizeMode (col, auto_resize ? QHeaderView::ResizeToContents : QHeaderView::Interactive);
}
gint gtk_clist_columns_autosize (GtkCList *c)
{
  QTreeWidget *tw = ctw (c);
  int total = 0;
  for (int i = 0; tw && i < c->columns; i++) { tw->resizeColumnToContents (i); total += tw->columnWidth (i); }
  return total;
}
gint gtk_clist_optimal_column_width (GtkCList *c, gint col)
{
  QTreeWidget *tw = ctw (c);
  if (!tw) return 0;
  int w = tw->columnWidth (col);
  tw->resizeColumnToContents (col);
  int opt = tw->columnWidth (col);
  tw->setColumnWidth (col, w);
  return opt;
}
void gtk_clist_column_titles_show (GtkCList *c) { if (QTreeWidget *tw = ctw (c)) tw->setHeaderHidden (false); }
void gtk_clist_column_titles_hide (GtkCList *c) { if (QTreeWidget *tw = ctw (c)) tw->setHeaderHidden (true); }
void gtk_clist_column_titles_active (GtkCList *c) { if (QTreeWidget *tw = ctw (c)) tw->header ()->setSectionsClickable (true); }
void gtk_clist_column_titles_passive (GtkCList *c) { if (QTreeWidget *tw = ctw (c)) tw->header ()->setSectionsClickable (false); }
void gtk_clist_column_title_active (GtkCList *c, gint) { gtk_clist_column_titles_active (c); }
void gtk_clist_column_title_passive (GtkCList *, gint) {}
void gtk_clist_set_shadow_type (GtkCList *c, GtkShadowType type)
{
  if (QTreeWidget *tw = ctw (c)) tw->setFrameShape (type == GTK_SHADOW_NONE ? QFrame::NoFrame : QFrame::StyledPanel);
}
void gtk_clist_set_row_height (GtkCList *c, guint height) { c->row_height = height; }
void gtk_clist_set_foreground (GtkCList *c, gint row, const GdkColor *color)
{
  if (CListRowData *r = row_at (c, row); r && color)
    for (int i = 0; i < c->columns; i++) r->item->setForeground (i, QColor (color->red >> 8, color->green >> 8, color->blue >> 8));
}
void gtk_clist_set_background (GtkCList *c, gint row, const GdkColor *color)
{
  if (CListRowData *r = row_at (c, row); r && color)
    for (int i = 0; i < c->columns; i++) r->item->setBackground (i, QColor (color->red >> 8, color->green >> 8, color->blue >> 8));
}
void gtk_clist_moveto (GtkCList *c, gint row, gint, gfloat row_align, gfloat)
{
  CListRowData *r = row_at (c, row);
  QTreeWidget *tw = ctw (c);
  if (!r || !tw) return;
  tw->scrollToItem (r->item, row_align < 0 ? QAbstractItemView::EnsureVisible
                             : row_align < 0.25 ? QAbstractItemView::PositionAtTop
                             : row_align > 0.75 ? QAbstractItemView::PositionAtBottom : QAbstractItemView::PositionAtCenter);
}
gint gtk_clist_get_selection_info (GtkCList *c, gint x, gint y, gint *row, gint *column)
{
  QTreeWidget *tw = ctw (c);
  if (!tw) return 0;
  QTreeWidgetItem *it = tw->itemAt (x, y);
  int r = it ? clist_row_of_item (c, it) : -1;
  if (row) *row = r;
  if (column) *column = tw->columnAt (x);
  return r >= 0;
}
GtkVisibility gtk_clist_row_is_visible (GtkCList *c, gint row)
{
  CListRowData *r = row_at (c, row);
  QTreeWidget *tw = ctw (c);
  if (!r || !tw) return GTK_VISIBILITY_NONE;
  QRect ir = tw->visualItemRect (r->item), vr = tw->viewport ()->rect ();
  if (!ir.intersects (vr)) return GTK_VISIBILITY_NONE;
  return vr.contains (ir) ? GTK_VISIBILITY_FULL : GTK_VISIBILITY_PARTIAL;
}

static gint default_compare (GtkCList *c, gconstpointer p1, gconstpointer p2)
{
  auto *r1 = reinterpret_cast<const CListRowData *> (p1), *r2 = reinterpret_cast<const CListRowData *> (p2);
  int col = qBound (0, c->sort_column, c->columns - 1);
  QByteArray a = r1->cells[col].text.toUtf8 (), b = r2->cells[col].text.toUtf8 ();
  return strcmp (a.constData (), b.constData ());
}

void gtk_clist_set_compare_func (GtkCList *c, GtkCListCompareFunc f) { c->compare = f; }
void gtk_clist_set_sort_column (GtkCList *c, gint col) { c->sort_column = col; }
void gtk_clist_set_sort_type (GtkCList *c, GtkSortType t) { c->sort_type = t; }
void gtk_clist_set_auto_sort (GtkCList *c, gboolean a) { cld (c)->auto_sort = a; if (a) gtk_clist_sort (c); }

static void clist_reorder_items (GtkCList *c)
{
  CListData *d = cld (c);
  QTreeWidget *tw = ctw (c);
  if (!tw) return;
  d->syncing = true;
  while (tw->topLevelItemCount ()) tw->takeTopLevelItem (0);
  for (CListRowData *r : d->rows) {
    tw->addTopLevelItem (r->item);
    r->item->setSelected (r->row.state == GTK_STATE_SELECTED);
  }
  d->syncing = false;
  clist_sync_public (c);
  clist_rebuild_selection (c);
}

void gtk_clist_sort (GtkCList *c)
{
  CListData *d = cld (c);
  GtkCListCompareFunc cmp = c->compare ? c->compare : default_compare;
  bool desc = c->sort_type == GTK_SORT_DESCENDING;
  /* GTK passes GtkCListRow pointers; ours start with that struct */
  std::stable_sort (d->rows.begin (), d->rows.end (), [&] (CListRowData *a, CListRowData *b) {
    int r = cmp (c, &a->row, &b->row);
    return desc ? r > 0 : r < 0;
  });
  clist_reorder_items (c);
}

void gtk_clist_set_reorderable (GtkCList *c, gboolean r)
{
  if (QTreeWidget *tw = ctw (c)) tw->setDragDropMode (r ? QAbstractItemView::InternalMove : QAbstractItemView::NoDragDrop);
}
void gtk_clist_set_use_drag_icons (GtkCList *, gboolean) {}
void gtk_clist_set_button_actions (GtkCList *, guint, guint8) {}
void gtk_clist_swap_rows (GtkCList *c, gint r1, gint r2)
{
  CListData *d = cld (c);
  if (r1 < 0 || r2 < 0 || r1 >= d->rows.size () || r2 >= d->rows.size ()) return;
  d->rows.swapItemsAt (r1, r2);
  clist_reorder_items (c);
}
void gtk_clist_row_move (GtkCList *c, gint src, gint dst)
{
  CListData *d = cld (c);
  if (src < 0 || dst < 0 || src >= d->rows.size () || dst >= d->rows.size ()) return;
  d->rows.move (src, dst);
  clist_reorder_items (c);
  g_signal_emit_by_name (c, "row-move", src, dst);
}
void gtk_clist_set_hadjustment (GtkCList *c, GtkAdjustment *a) { c->hadjustment = a; }
void gtk_clist_set_vadjustment (GtkCList *c, GtkAdjustment *a) { c->vadjustment = a; }
GtkAdjustment *gtk_clist_get_hadjustment (GtkCList *c)
{
  if (!c->hadjustment) c->hadjustment = (GtkAdjustment *) g_object_ref_sink (gtk_adjustment_new (0, 0, 0, 1, 10, 0));
  return c->hadjustment;
}
GtkAdjustment *gtk_clist_get_vadjustment (GtkCList *c)
{
  if (!c->vadjustment) c->vadjustment = (GtkAdjustment *) g_object_ref_sink (gtk_adjustment_new (0, 0, 0, 1, 10, 0));
  return c->vadjustment;
}
void gtk_clist_set_policy (GtkCList *, GtkPolicyType, GtkPolicyType) {}

} /* extern "C" */

/* ======================================================================
 * GtkCTree: GtkCList rows with a hierarchy (QTreeWidgetItem children)
 * ====================================================================== */
struct _GtkCTreeNode { int dummy; };   /* a GtkCTreeNode * is a CListRowData * */
static CListRowData *nrow (GtkCTreeNode *n) { return reinterpret_cast<CListRowData *> (n); }
static GtkCTreeNode *rnode (CListRowData *r) { return reinterpret_cast<GtkCTreeNode *> (r); }

G_DEFINE_TYPE (GtkCTree, gtk_ctree, GTK_TYPE_CLIST)
static void gtk_ctree_init (GtkCTree *t) { t->tree_indent = 20; t->tree_spacing = 5; }
static void gtk_ctree_class_init (GtkCTreeClass *klass)
{
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("tree-select-row", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);
  g_signal_new ("tree-unselect-row", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);
  g_signal_new ("tree-expand", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_POINTER);
  g_signal_new ("tree-collapse", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_POINTER);
}

/* the rows in display order (depth first), as GtkCList row indices */
static void ctree_collect (QTreeWidgetItem *it, GtkCList *c, QList<CListRowData *> &out)
{
  for (CListRowData *r : cld (c)->rows) if (r->item == it) { out << r; break; }
  for (int i = 0; i < it->childCount (); i++) ctree_collect (it->child (i), c, out);
}
static void ctree_reindex (GtkCList *c)
{
  QTreeWidget *tw = ctw (c);
  if (!tw) return;
  QList<CListRowData *> order;
  for (int i = 0; i < tw->topLevelItemCount (); i++) ctree_collect (tw->topLevelItem (i), c, order);
  cld (c)->rows = order;
  clist_sync_public (c);
  clist_rebuild_selection (c);
}

static void ctree_update_links (CListRowData *r)
{
  QTreeWidgetItem *it = r->item, *p = it->parent ();
  (void) p;
  r->tree->row = r->row;
}

extern "C" {

GtkCTreeRow *gtk_ctree_node_get_row_ (GtkCTreeNode *node)
{
  CListRowData *r = nrow (node);
  if (!r || !r->tree) return nullptr;
  QTreeWidgetItem *it = r->item;
  auto find = [] (QTreeWidgetItem *i) -> GtkCTreeNode * {
    return i ? reinterpret_cast<GtkCTreeNode *> (i->data (0, Qt::UserRole + 1).value<void *> ()) : nullptr;
  };
  r->tree->parent = find (it->parent ());
  QTreeWidgetItem *sib = nullptr;
  if (QTreeWidgetItem *p = it->parent ()) { int i = p->indexOfChild (it); sib = i + 1 < p->childCount () ? p->child (i + 1) : nullptr; }
  else if (QTreeWidget *tw = it->treeWidget ()) { int i = tw->indexOfTopLevelItem (it); sib = i + 1 < tw->topLevelItemCount () ? tw->topLevelItem (i + 1) : nullptr; }
  r->tree->sibling = find (sib);
  r->tree->children = find (it->childCount () ? it->child (0) : nullptr);
  r->tree->expanded = it->isExpanded ();
  ctree_update_links (r);
  return r->tree;
}

GtkCTreeNode *gtk_ctree_node_next_ (GtkCTreeNode *node)
{
  CListRowData *r = nrow (node);
  if (!r) return nullptr;
  QTreeWidgetItem *it = r->item;
  QTreeWidget *tw = it->treeWidget ();
  QTreeWidgetItem *next = tw ? tw->itemBelow (it) : nullptr;
  return next ? reinterpret_cast<GtkCTreeNode *> (next->data (0, Qt::UserRole + 1).value<void *> ()) : nullptr;
}

GtkWidget *gtk_ctree_new_with_titles (gint columns, gint tree_column, gchar *titles[])
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_CTREE, nullptr);
  GTK_CTREE (w)->tree_column = tree_column;
  gtk_clist_construct (GTK_CLIST (w), columns, titles);
  return w;
}
GtkWidget *gtk_ctree_new (gint columns, gint tree_column) { return gtk_ctree_new_with_titles (columns, tree_column, nullptr); }

GtkCTreeNode *gtk_ctree_insert_node (GtkCTree *ctree, GtkCTreeNode *parent, GtkCTreeNode *sibling, gchar *text[], guint8,
                                     GdkPixmap *pixmap_closed, GdkBitmap *mask_closed, GdkPixmap *pixmap_opened,
                                     GdkBitmap *mask_opened, gboolean is_leaf, gboolean expanded)
{
  GtkCList *c = GTK_CLIST (ctree);
  CListRowData *r = clist_new_row (c, text);
  r->tree = new GtkCTreeRow ();
  r->tree->is_leaf = is_leaf;
  r->tree->pixmap_closed = pixmap_closed; r->tree->mask_closed = mask_closed;
  r->tree->pixmap_opened = pixmap_opened; r->tree->mask_opened = mask_opened;
  r->item->setData (0, Qt::UserRole + 1, QVariant::fromValue ((void *) r));
  cld (c)->rows << r;
  QTreeWidget *tw = ctw (c);
  QTreeWidgetItem *pi = parent ? nrow (parent)->item : nullptr;
  QTreeWidgetItem *si = sibling ? nrow (sibling)->item : nullptr;
  if (pi) {
    int idx = si ? pi->indexOfChild (si) : pi->childCount ();
    pi->insertChild (idx < 0 ? pi->childCount () : idx, r->item);
    r->tree->level = gtk_ctree_node_get_row_ (parent)->level + 1;
  } else if (tw) {
    int idx = si ? tw->indexOfTopLevelItem (si) : tw->topLevelItemCount ();
    tw->insertTopLevelItem (idx < 0 ? tw->topLevelItemCount () : idx, r->item);
    r->tree->level = 1;
  }
  if (pixmap_closed && ctree->tree_column >= 0 && ctree->tree_column < c->columns) {
    g_object_ref (pixmap_closed);
    if (mask_closed) g_object_ref (mask_closed);
    r->cells[ctree->tree_column].pixmap = pixmap_closed;
    r->cells[ctree->tree_column].mask = mask_closed;
  }
  row_update_item (c, r);
  r->item->setExpanded (expanded);
  ctree_reindex (c);
  return rnode (r);
}

void gtk_ctree_remove_node (GtkCTree *ctree, GtkCTreeNode *node)
{
  GtkCList *c = GTK_CLIST (ctree);
  CListRowData *r = nrow (node);
  if (!r) return;
  /* children first */
  while (r->item->childCount ()) {
    auto *child = reinterpret_cast<GtkCTreeNode *> (r->item->child (0)->data (0, Qt::UserRole + 1).value<void *> ());
    gtk_ctree_remove_node (ctree, child);
  }
  cld (c)->rows.removeAll (r);
  cld (c)->syncing = true;
  delete r->item;
  cld (c)->syncing = false;
  row_free (r);
  ctree_reindex (c);
}

void gtk_ctree_node_set_row_data_full (GtkCTree *, GtkCTreeNode *node, gpointer data, GDestroyNotify destroy)
{
  CListRowData *r = nrow (node);
  if (!r) return;
  if (r->row.destroy) r->row.destroy (r->row.data);
  r->row.data = data;
  r->row.destroy = destroy;
}
void gtk_ctree_node_set_row_data (GtkCTree *t, GtkCTreeNode *node, gpointer data) { gtk_ctree_node_set_row_data_full (t, node, data, nullptr); }
gpointer gtk_ctree_node_get_row_data (GtkCTree *, GtkCTreeNode *node) { return node ? nrow (node)->row.data : nullptr; }

void gtk_ctree_node_set_text (GtkCTree *t, GtkCTreeNode *node, gint col, const gchar *text)
{
  gtk_clist_set_text (GTK_CLIST (t), cld (GTK_CLIST (t))->rows.indexOf (nrow (node)), col, text);
}
gint gtk_ctree_node_get_text (GtkCTree *t, GtkCTreeNode *node, gint col, gchar **text)
{
  return gtk_clist_get_text (GTK_CLIST (t), cld (GTK_CLIST (t))->rows.indexOf (nrow (node)), col, text);
}
gint gtk_ctree_node_get_pixtext (GtkCTree *t, GtkCTreeNode *node, gint col, gchar **text, guint8 *spacing, GdkPixmap **pm, GdkBitmap **mask)
{
  return gtk_clist_get_pixtext (GTK_CLIST (t), cld (GTK_CLIST (t))->rows.indexOf (nrow (node)), col, text, spacing, pm, mask);
}
void gtk_ctree_node_set_pixtext (GtkCTree *t, GtkCTreeNode *node, gint col, const gchar *text, guint8 spacing, GdkPixmap *pm, GdkBitmap *mask)
{
  gtk_clist_set_pixtext (GTK_CLIST (t), cld (GTK_CLIST (t))->rows.indexOf (nrow (node)), col, text, spacing, pm, mask);
}
GtkCTreeNode *gtk_ctree_node_nth (GtkCTree *t, guint row)
{
  CListData *d = cld (GTK_CLIST (t));
  return row < (guint) d->rows.size () ? rnode (d->rows[row]) : nullptr;
}
GtkCTreeNode *gtk_ctree_find_by_row_data (GtkCTree *t, GtkCTreeNode *, gpointer data)
{
  for (CListRowData *r : cld (GTK_CLIST (t))->rows) if (r->row.data == data) return rnode (r);
  return nullptr;
}
void gtk_ctree_expand (GtkCTree *t, GtkCTreeNode *node) { if (node) { nrow (node)->item->setExpanded (true); g_signal_emit_by_name (t, "tree-expand", node); } }
void gtk_ctree_expand_recursive (GtkCTree *t, GtkCTreeNode *node)
{
  if (!node) { if (QTreeWidget *tw = ctw (GTK_CLIST (t))) tw->expandAll (); return; }
  std::function<void (QTreeWidgetItem *)> rec = [&] (QTreeWidgetItem *it) { it->setExpanded (true); for (int i = 0; i < it->childCount (); i++) rec (it->child (i)); };
  rec (nrow (node)->item);
}
void gtk_ctree_collapse (GtkCTree *t, GtkCTreeNode *node) { if (node) { nrow (node)->item->setExpanded (false); g_signal_emit_by_name (t, "tree-collapse", node); } }
void gtk_ctree_select (GtkCTree *t, GtkCTreeNode *node)
{
  int row = cld (GTK_CLIST (t))->rows.indexOf (nrow (node));
  if (row >= 0) gtk_clist_select_row (GTK_CLIST (t), row, 0);
  g_signal_emit_by_name (t, "tree-select-row", node, 0);
}
void gtk_ctree_unselect (GtkCTree *t, GtkCTreeNode *node)
{
  int row = cld (GTK_CLIST (t))->rows.indexOf (nrow (node));
  if (row >= 0) gtk_clist_unselect_row (GTK_CLIST (t), row, 0);
  g_signal_emit_by_name (t, "tree-unselect-row", node, 0);
}
void gtk_ctree_sort_node (GtkCTree *t, GtkCTreeNode *node)
{
  GtkCList *c = GTK_CLIST (t);
  GtkCListCompareFunc cmp = c->compare ? c->compare : default_compare;
  bool desc = c->sort_type == GTK_SORT_DESCENDING;
  QList<QTreeWidgetItem *> kids;
  QTreeWidgetItem *parent = node ? nrow (node)->item : nullptr;
  QTreeWidget *tw = ctw (c);
  if (parent) kids = parent->takeChildren ();
  else if (tw) while (tw->topLevelItemCount ()) kids << tw->takeTopLevelItem (0);
  auto rowof = [] (QTreeWidgetItem *it) { return static_cast<CListRowData *> (it->data (0, Qt::UserRole + 1).value<void *> ()); };
  std::stable_sort (kids.begin (), kids.end (), [&] (QTreeWidgetItem *a, QTreeWidgetItem *b) {
    int r = cmp (c, &rowof (a)->row, &rowof (b)->row);
    return desc ? r > 0 : r < 0;
  });
  if (parent) parent->addChildren (kids); else if (tw) tw->addTopLevelItems (kids);
  ctree_reindex (c);
}
void gtk_ctree_sort_recursive (GtkCTree *t, GtkCTreeNode *node) { gtk_ctree_sort_node (t, node); }
void gtk_ctree_set_line_style (GtkCTree *t, GtkCTreeLineStyle s) { t->line_style = s; }
void gtk_ctree_set_expander_style (GtkCTree *t, GtkCTreeExpanderStyle s) { t->expander_style = s; }
void gtk_ctree_set_indent (GtkCTree *t, gint indent) { t->tree_indent = indent; if (QTreeWidget *tw = ctw (GTK_CLIST (t))) tw->setIndentation (indent); }
void gtk_ctree_node_moveto (GtkCTree *t, GtkCTreeNode *node, gint col, gfloat ra, gfloat ca)
{
  if (QTreeWidget *tw = ctw (GTK_CLIST (t)); tw && node) { (void) col; (void) ca; tw->scrollToItem (nrow (node)->item, ra < 0.5 ? QAbstractItemView::PositionAtTop : QAbstractItemView::PositionAtCenter); }
}
gboolean gtk_ctree_is_viewable (GtkCTree *, GtkCTreeNode *node)
{
  for (QTreeWidgetItem *p = node ? nrow (node)->item->parent () : nullptr; p; p = p->parent ()) if (!p->isExpanded ()) return FALSE;
  return node != nullptr;
}
static void ctree_walk (GtkCTree *t, QTreeWidgetItem *it, GtkCTreeFunc func, gpointer data, bool post)
{
  auto *node = reinterpret_cast<GtkCTreeNode *> (it->data (0, Qt::UserRole + 1).value<void *> ());
  if (!post) func (t, node, data);
  for (int i = 0; i < it->childCount (); i++) ctree_walk (t, it->child (i), func, data, post);
  if (post) func (t, node, data);
}
void gtk_ctree_post_recursive (GtkCTree *t, GtkCTreeNode *node, GtkCTreeFunc func, gpointer data)
{
  if (node) { ctree_walk (t, nrow (node)->item, func, data, true); return; }
  if (QTreeWidget *tw = ctw (GTK_CLIST (t)))
    for (int i = 0; i < tw->topLevelItemCount (); i++) ctree_walk (t, tw->topLevelItem (i), func, data, true);
}
void gtk_ctree_pre_recursive (GtkCTree *t, GtkCTreeNode *node, GtkCTreeFunc func, gpointer data)
{
  if (node) { ctree_walk (t, nrow (node)->item, func, data, false); return; }
  if (QTreeWidget *tw = ctw (GTK_CLIST (t)))
    for (int i = 0; i < tw->topLevelItemCount (); i++) ctree_walk (t, tw->topLevelItem (i), func, data, false);
}

} /* extern "C" */

/* ======================================================================
 * GtkList / GtkListItem: a column of selectable items with any content
 * ====================================================================== */
class GtkQtListItem : public QWidget {
public:
  bool selected = false;
protected:
  void paintEvent (QPaintEvent *) override
  {
    if (!selected) return;
    QPainter p (this);
    p.fillRect (rect (), palette ().color (QPalette::Highlight));
  }
  void mousePressEvent (QMouseEvent *e) override
  {
    GtkWidget *w = gtkqt_from_qobject (this);
    if (w && e->button () == Qt::LeftButton && w->parent && GTK_IS_LIST (w->parent)) {
      auto *list = GTK_LIST (w->parent);
      if (list->selection_mode == GTK_SELECTION_MULTIPLE && selected) gtk_list_unselect_child (list, w);
      else gtk_list_select_child (list, w);
    }
    QWidget::mousePressEvent (e);
  }
};

static void list_item_paint (GtkWidget *item, bool sel)
{
  auto *q = dynamic_cast<GtkQtListItem *> (gtkqt_qw (item));
  if (!q) return;
  q->selected = sel;
  QPalette p = q->palette ();
  p.setColor (QPalette::WindowText, QApplication::palette ().color (sel ? QPalette::HighlightedText : QPalette::WindowText));
  for (QLabel *l : q->findChildren<QLabel *> ()) l->setPalette (p);
  q->update ();
}

static QObject *list_item_create (GtkWidget *)
{
  auto *q = new GtkQtListItem;
  q->setAutoFillBackground (false);
  return q;
}

/* GtkItem: menu items and list items; select/deselect/toggle signals */
G_DEFINE_TYPE (GtkItem, gtk_item, GTK_TYPE_BIN)
static void gtk_item_init (GtkItem *) {}
static void gtk_item_class_init (GtkItemClass *klass)
{
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("select", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("deselect", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("toggle", t, (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION), 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}
extern "C" {
void gtk_item_select (GtkItem *item) { g_signal_emit_by_name (item, "select"); }
void gtk_item_deselect (GtkItem *item) { g_signal_emit_by_name (item, "deselect"); }
void gtk_item_toggle (GtkItem *item) { g_signal_emit_by_name (item, "toggle"); }
}

G_DEFINE_TYPE (GtkListItem, gtk_list_item, GTK_TYPE_ITEM)
static void gtk_list_item_init (GtkListItem *) {}
static void gtk_list_item_class_init (GtkListItemClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), list_item_create);
}

static QObject *list_create (GtkWidget *)
{
  auto *w = new QWidget;
  auto *l = new QVBoxLayout (w);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);
  l->addStretch (1);
  return w;
}

static void list_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (c)), *cw = gtkqt_qw (child);
  auto *l = qw ? qobject_cast<QVBoxLayout *> (qw->layout ()) : nullptr;
  if (!l || !cw) return;
  int idx = g_list_index (GTK_LIST (c)->children, child);
  l->insertWidget (idx < 0 ? l->count () - 1 : idx, cw);
}

static void list_unplace (GtkContainer *c, GtkWidget *child)
{
  auto *list = GTK_LIST (c);
  list->children = g_list_remove (list->children, child);
  if (g_list_find (list->selection, child)) list->selection = g_list_remove (list->selection, child);
  if (QWidget *cw = gtkqt_qw (child)) { cw->hide (); cw->setParent (nullptr); }
}

static void list_add (GtkContainer *c, GtkWidget *child)
{
  auto *list = GTK_LIST (c);
  if (!g_list_find (list->children, child)) list->children = g_list_append (list->children, child);
  GTK_CONTAINER_CLASS (g_type_class_peek (GTK_TYPE_CONTAINER))->add (c, child);
}

G_DEFINE_TYPE (GtkList, gtk_list, GTK_TYPE_CONTAINER)
static void gtk_list_init (GtkList *l) { l->selection_mode = GTK_SELECTION_SINGLE; }
static void gtk_list_class_init (GtkListClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), list_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), list_place, list_unplace);
  GTK_CONTAINER_CLASS (klass)->add = list_add;
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("selection-changed", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkListClass, selection_changed), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("select-child", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkListClass, select_child), nullptr, nullptr, nullptr, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
  g_signal_new ("unselect-child", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkListClass, unselect_child), nullptr, nullptr, nullptr, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
  klass->select_child = [] (GtkList *list, GtkWidget *child) {
    if (list->selection_mode != GTK_SELECTION_MULTIPLE)
      while (list->selection) {
        auto *old = (GtkWidget *) list->selection->data;
        list->selection = g_list_remove (list->selection, old);
        list_item_paint (old, false);
        g_signal_emit_by_name (old, "deselect");
      }
    if (!g_list_find (list->selection, child)) list->selection = g_list_append (list->selection, child);
    list_item_paint (child, true);
    g_signal_emit_by_name (child, "select");
    g_signal_emit_by_name (list, "selection-changed");
  };
  klass->unselect_child = [] (GtkList *list, GtkWidget *child) {
    if (!g_list_find (list->selection, child)) return;
    list->selection = g_list_remove (list->selection, child);
    list_item_paint (child, false);
    g_signal_emit_by_name (child, "deselect");
    g_signal_emit_by_name (list, "selection-changed");
  };
}

extern "C" {

GtkWidget *gtk_list_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_LIST, nullptr); }
GtkWidget *gtk_list_item_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_LIST_ITEM, nullptr); }
GtkWidget *gtk_list_item_new_with_label (const gchar *label)
{
  GtkWidget *item = gtk_list_item_new ();
  GtkWidget *l = gtk_label_new (label);
  gtk_misc_set_alignment (GTK_MISC (l), 0, 0.5);
  gtk_container_add (GTK_CONTAINER (item), l);
  gtk_widget_show (l);
  return item;
}

void gtk_list_insert_items (GtkList *list, GList *items, gint position)
{
  int pos = position < 0 ? g_list_length (list->children) : position;
  for (GList *l = items; l; l = l->next, pos++) {
    auto *child = (GtkWidget *) l->data;
    list->children = g_list_insert (list->children, child, pos);
    gtk_container_add (GTK_CONTAINER (list), child);
  }
  g_list_free (items);   /* GTK takes ownership of the list */
}
void gtk_list_append_items (GtkList *list, GList *items) { gtk_list_insert_items (list, items, -1); }
void gtk_list_prepend_items (GtkList *list, GList *items) { gtk_list_insert_items (list, items, 0); }
void gtk_list_remove_items (GtkList *list, GList *items)
{
  for (GList *l = items; l; l = l->next) gtk_container_remove (GTK_CONTAINER (list), (GtkWidget *) l->data);
}
void gtk_list_clear_items (GtkList *list, gint start, gint end)
{
  GList *victims = nullptr;
  int i = 0;
  for (GList *l = list->children; l; l = l->next, i++)
    if (i >= start && (end < 0 || i < end)) victims = g_list_append (victims, l->data);
  for (GList *l = victims; l; l = l->next) gtk_widget_destroy ((GtkWidget *) l->data);
  g_list_free (victims);
}
void gtk_list_select_child (GtkList *list, GtkWidget *child) { g_signal_emit_by_name (list, "select-child", child); }
void gtk_list_unselect_child (GtkList *list, GtkWidget *child) { g_signal_emit_by_name (list, "unselect-child", child); }
void gtk_list_select_item (GtkList *list, gint item) { if (auto *w = (GtkWidget *) g_list_nth_data (list->children, item)) gtk_list_select_child (list, w); }
void gtk_list_unselect_item (GtkList *list, gint item) { if (auto *w = (GtkWidget *) g_list_nth_data (list->children, item)) gtk_list_unselect_child (list, w); }
gint gtk_list_child_position (GtkList *list, GtkWidget *child) { return g_list_index (list->children, child); }
void gtk_list_set_selection_mode (GtkList *list, GtkSelectionMode mode) { list->selection_mode = mode; }
void gtk_list_item_select (GtkListItem *item)
{
  GtkWidget *w = GTK_WIDGET (item);
  if (w->parent && GTK_IS_LIST (w->parent)) gtk_list_select_child (GTK_LIST (w->parent), w);
}
void gtk_list_item_deselect (GtkListItem *item)
{
  GtkWidget *w = GTK_WIDGET (item);
  if (w->parent && GTK_IS_LIST (w->parent)) gtk_list_unselect_child (GTK_LIST (w->parent), w);
}

} /* extern "C" */

/* ======================================================================
 * GtkText (GTK 1): a QTextEdit with an insertion point
 * ====================================================================== */
static QTextEdit *text_edit1 (GtkText *t) { return gtkqt_q<QTextEdit> (GTK_WIDGET (t)); }

static void text_bind_adjustment (GtkText *t, GtkAdjustment *adj, bool vertical)
{
  QTextEdit *e = text_edit1 (t);
  if (!e || !adj) return;
  QScrollBar *sb = vertical ? e->verticalScrollBar () : e->horizontalScrollBar ();
  if (vertical) e->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  else e->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  auto sync = [adj, sb] {
    g_object_set_data (G_OBJECT (adj), "gtkqt-syncing", GINT_TO_POINTER (1));
    gtk_adjustment_configure (adj, sb->value (), sb->minimum (), sb->maximum () + sb->pageStep (), sb->singleStep (),
                              sb->pageStep (), sb->pageStep ());
    g_object_set_data (G_OBJECT (adj), "gtkqt-syncing", nullptr);
  };
  QObject::connect (sb, &QScrollBar::rangeChanged, sb, sync);
  QObject::connect (sb, &QScrollBar::valueChanged, sb, sync);
  sync ();
  auto *guard = new QPointer<QScrollBar> (sb);
  g_signal_connect_data (adj, "value-changed", G_CALLBACK (+[] (GtkAdjustment *a, gpointer p) {
    if (g_object_get_data (G_OBJECT (a), "gtkqt-syncing")) return;
    auto *g = static_cast<QPointer<QScrollBar> *> (p);
    if (*g) (*g)->setValue ((int) std::lround (gtk_adjustment_get_value (a)));
  }), guard, [] (gpointer p, GClosure *) { delete static_cast<QPointer<QScrollBar> *> (p); }, (GConnectFlags) 0);
}

static QObject *text1_create (GtkWidget *w)
{
  auto *e = new QTextEdit;
  e->setAcceptRichText (false);
  e->setReadOnly (true);
  e->setLineWrapMode (QTextEdit::NoWrap);
  gtkqt_connect (w, e, &QTextEdit::textChanged, [] (GtkWidget *tw) {
    auto *t = GTK_TEXT (tw);
    if (QTextEdit *te = text_edit1 (t)) t->text_len = te->document ()->characterCount () - 1;
    g_signal_emit_by_name (tw, "changed");
  });
  return e;
}

static void text1_dispose (GObject *o)
{
  auto *t = GTK_TEXT (o);
  if (t->hadj) { g_object_unref (t->hadj); t->hadj = nullptr; }
  if (t->vadj) { g_object_unref (t->vadj); t->vadj = nullptr; }
  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (o)))->dispose (o);
}

G_DEFINE_TYPE (GtkText, gtk_text, GTK_TYPE_WIDGET)
static void gtk_text_init (GtkText *) {}
static void gtk_text_class_init (GtkTextClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = text1_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), text1_create);
  g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}

extern "C" {

GtkWidget *gtk_text_new (GtkAdjustment *hadj, GtkAdjustment *vadj)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_TEXT, nullptr);
  gtk_text_set_adjustments (GTK_TEXT (w), hadj, vadj);
  return w;
}

void gtk_text_set_adjustments (GtkText *t, GtkAdjustment *hadj, GtkAdjustment *vadj)
{
  if (!hadj) hadj = gtk_adjustment_new (0, 0, 1, 0.1, 0.1, 0.1);
  if (!vadj) vadj = gtk_adjustment_new (0, 0, 1, 0.1, 0.1, 0.1);
  g_object_ref_sink (hadj);
  g_object_ref_sink (vadj);
  if (t->hadj) g_object_unref (t->hadj);
  if (t->vadj) g_object_unref (t->vadj);
  t->hadj = hadj;
  t->vadj = vadj;
  text_bind_adjustment (t, hadj, false);
  text_bind_adjustment (t, vadj, true);
}

void gtk_text_set_editable (GtkText *t, gboolean editable) { t->editable = editable; if (QTextEdit *e = text_edit1 (t)) e->setReadOnly (!editable); }
void gtk_text_set_word_wrap (GtkText *t, gint wrap) { if (QTextEdit *e = text_edit1 (t)) { e->setLineWrapMode (QTextEdit::WidgetWidth); e->setWordWrapMode (wrap ? QTextOption::WordWrap : QTextOption::WrapAnywhere); } }
void gtk_text_set_line_wrap (GtkText *t, gint wrap) { if (QTextEdit *e = text_edit1 (t)) e->setLineWrapMode (wrap ? QTextEdit::WidgetWidth : QTextEdit::NoWrap); }
void gtk_text_set_point (GtkText *t, guint index) { t->point = qMin (index, gtk_text_get_length (t)); }
guint gtk_text_get_point (GtkText *t) { return t->point; }
guint gtk_text_get_length (GtkText *t) { QTextEdit *e = text_edit1 (t); return e ? (guint) e->document ()->characterCount () - 1 : 0; }
void gtk_text_freeze (GtkText *t) { if (QTextEdit *e = text_edit1 (t)) e->setUpdatesEnabled (false); }
void gtk_text_thaw (GtkText *t) { if (QTextEdit *e = text_edit1 (t)) e->setUpdatesEnabled (true); }

void gtk_text_insert (GtkText *t, GdkFont *font, const GdkColor *fore, const GdkColor *back, const char *chars, gint length)
{
  QTextEdit *e = text_edit1 (t);
  if (!e || !chars) return;
  QString s = length < 0 ? QString::fromUtf8 (chars) : QString::fromUtf8 (chars, length);
  if (s.contains (QChar::ReplacementCharacter)) s = length < 0 ? QString::fromLatin1 (chars) : QString::fromLatin1 (chars, length);
  QTextCursor c (e->document ());
  c.setPosition (qMin ((int) t->point, e->document ()->characterCount () - 1));
  QTextCharFormat f;
  if (fore) f.setForeground (QColor (fore->red >> 8, fore->green >> 8, fore->blue >> 8));
  if (back) f.setBackground (QColor (back->red >> 8, back->green >> 8, back->blue >> 8));
  (void) font;
  c.insertText (s, f);
  t->point = c.position ();
}

gint gtk_text_backward_delete (GtkText *t, guint n)
{
  QTextEdit *e = text_edit1 (t);
  if (!e || n > t->point) return FALSE;
  QTextCursor c (e->document ());
  c.setPosition (t->point);
  c.setPosition (t->point - n, QTextCursor::KeepAnchor);
  c.removeSelectedText ();
  t->point -= n;
  return TRUE;
}
gint gtk_text_forward_delete (GtkText *t, guint n)
{
  QTextEdit *e = text_edit1 (t);
  if (!e || t->point + n > gtk_text_get_length (t)) return FALSE;
  QTextCursor c (e->document ());
  c.setPosition (t->point);
  c.setPosition (t->point + n, QTextCursor::KeepAnchor);
  c.removeSelectedText ();
  return TRUE;
}

/* ======================================================================
 * GtkProgress API (GTK 1) on GtkProgressBar
 * ====================================================================== */
static void progress_sync (GtkProgressBar *p)
{
  GtkAdjustment *a = p->adjustment;
  if (!a) return;
  double range = a->upper - a->lower;
  double frac = range > 0 ? (a->value - a->lower) / range : 0;
  gtk_progress_bar_set_fraction (p, qBound (0.0, frac, 1.0));
  if (p->show_text) {
    QString f = gtkqt_str (p->format ? p->format : "%P %%");
    f.replace ("%p", QString::number (frac * 100, 'f', 0)).replace ("%P", QString::number (frac * 100, 'f', 0))
     .replace ("%v", QString::number (a->value, 'f', 0)).replace ("%l", QString::number (a->lower, 'f', 0))
     .replace ("%u", QString::number (a->upper, 'f', 0)).replace ("%%", "%");
    gtk_progress_bar_set_text (p, f.toUtf8 ().constData ());
  }
}

GtkAdjustment *gtk_progress_get_adjustment_ (GtkProgress *p)
{
  if (!p->adjustment) {
    p->adjustment = (GtkAdjustment *) g_object_ref_sink (gtk_adjustment_new (0, 0, 100, 0, 0, 0));
    g_signal_connect_swapped (p->adjustment, "value-changed", G_CALLBACK (progress_sync), p);
    g_signal_connect_swapped (p->adjustment, "changed", G_CALLBACK (progress_sync), p);
  }
  return p->adjustment;
}

void gtk_progress_set_adjustment (GtkProgress *p, GtkAdjustment *adj)
{
  if (p->adjustment) { g_signal_handlers_disconnect_by_data (p->adjustment, p); g_object_unref (p->adjustment); p->adjustment = nullptr; }
  if (adj) {
    p->adjustment = (GtkAdjustment *) g_object_ref_sink (adj);
    g_signal_connect_swapped (adj, "value-changed", G_CALLBACK (progress_sync), p);
    g_signal_connect_swapped (adj, "changed", G_CALLBACK (progress_sync), p);
    progress_sync (p);
  }
}
GtkWidget *gtk_progress_bar_new_with_adjustment (GtkAdjustment *adj)
{
  GtkWidget *w = gtk_progress_bar_new ();
  gtk_progress_set_adjustment (GTK_PROGRESS (w), adj);
  return w;
}
void gtk_progress_bar_update (GtkProgressBar *p, gfloat pct) { gtk_progress_set_percentage (p, pct); }
void gtk_progress_set_percentage (GtkProgress *p, gfloat pct)
{
  GtkAdjustment *a = gtk_progress_get_adjustment_ (p);
  gtk_adjustment_set_value (a, a->lower + pct * (a->upper - a->lower));
  progress_sync (p);
}
void gtk_progress_set_value (GtkProgress *p, gfloat v) { gtk_adjustment_set_value (gtk_progress_get_adjustment_ (p), v); progress_sync (p); }
gfloat gtk_progress_get_value (GtkProgress *p) { return gtk_progress_get_adjustment_ (p)->value; }
gfloat gtk_progress_get_current_percentage (GtkProgress *p)
{
  GtkAdjustment *a = gtk_progress_get_adjustment_ (p);
  return a->upper > a->lower ? (a->value - a->lower) / (a->upper - a->lower) : 0;
}
void gtk_progress_configure (GtkProgress *p, gfloat value, gfloat min, gfloat max)
{
  GtkAdjustment *a = gtk_progress_get_adjustment_ (p);
  gtk_adjustment_configure (a, value, min, max, a->step_increment, a->page_increment, a->page_size);
  progress_sync (p);
}
void gtk_progress_set_show_text (GtkProgress *p, gboolean show)
{
  p->show_text = show;
  gtk_progress_bar_set_show_text (p, show);
  progress_sync (p);
}
void gtk_progress_set_text_alignment (GtkProgress *p, gfloat x, gfloat y) { p->x_align = x; p->y_align = y; }
void gtk_progress_set_format_string (GtkProgress *p, const gchar *format) { g_free (p->format); p->format = g_strdup (format); progress_sync (p); }
void gtk_progress_set_activity_mode (GtkProgress *p, guint mode)
{
  p->activity_mode = mode != 0;
  if (auto *pb = gtkqt_q<QProgressBar> (GTK_WIDGET (p))) {
    if (mode) pb->setRange (0, 0); else { pb->setRange (0, 1000); progress_sync (p); }
  }
}
void gtk_progress_bar_set_bar_style (GtkProgressBar *, GtkProgressBarStyle) {}
void gtk_progress_bar_set_discrete_blocks (GtkProgressBar *, guint) {}
void gtk_progress_bar_set_activity_step (GtkProgressBar *, guint) {}
void gtk_progress_bar_set_activity_blocks (GtkProgressBar *, guint) {}

} /* extern "C" */

/* ======================================================================
 * GtkRuler
 * ====================================================================== */
class GtkQtRuler : public QWidget {
public:
  bool vertical = false;
  QSize sizeHint () const override { return vertical ? QSize (18, 40) : QSize (40, 18); }
  QSize minimumSizeHint () const override { return vertical ? QSize (18, 1) : QSize (1, 18); }
protected:
  void paintEvent (QPaintEvent *) override
  {
    GtkWidget *w = gtkqt_from_qobject (this);
    if (!w) return;
    auto *r = GTK_RULER (w);
    QPainter p (this);
    p.fillRect (rect (), palette ().color (QPalette::Window));
    p.setPen (palette ().color (QPalette::WindowText));
    int len = vertical ? height () : width (), thick = vertical ? width () : height ();
    p.drawLine (vertical ? QLine (thick - 1, 0, thick - 1, len) : QLine (0, thick - 1, len, thick - 1));
    double range = r->upper - r->lower;
    if (range == 0 || len <= 0) return;
    double per_unit = len / range;
    static const double steps[] = { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000 };
    double step = 1;
    for (double s : steps) { step = s; if (std::fabs (s * per_unit) >= 50) break; }
    QFont f = font ();
    f.setPixelSize (qMax (7, thick / 2));
    p.setFont (f);
    double start = std::floor (qMin (r->lower, r->upper) / step) * step;
    for (int k = 0;; k++) {
      double v = start + k * step / 5;
      if (v > qMax (r->lower, r->upper)) break;
      int pos = (int) std::lround ((v - r->lower) * per_unit);
      bool major = k % 5 == 0;
      int tick = major ? thick - 1 : thick / 4;
      if (vertical) p.drawLine (thick - tick, pos, thick, pos); else p.drawLine (pos, thick - tick, pos, thick);
      if (major) {
        if (vertical) { p.save (); p.translate (2, pos + 2); p.rotate (90); p.drawText (0, 0, QString::number (v)); p.restore (); }
        else p.drawText (pos + 2, thick / 2 + 2, QString::number (v));
      }
    }
    int pos = (int) std::lround ((r->position - r->lower) * per_unit);
    QPolygon tri;
    if (vertical) tri << QPoint (0, pos - 4) << QPoint (thick / 2, pos) << QPoint (0, pos + 4);
    else tri << QPoint (pos - 4, 0) << QPoint (pos, thick / 2) << QPoint (pos + 4, 0);
    p.setBrush (palette ().color (QPalette::WindowText));
    p.drawPolygon (tri);
  }
};

static gboolean ruler_motion (GtkWidget *w, GdkEventMotion *ev)
{
  auto *r = GTK_RULER (w);
  QWidget *qw = gtkqt_qw (w);
  if (!qw) return FALSE;
  bool vertical = G_TYPE_CHECK_INSTANCE_TYPE (w, gtk_vruler_get_type ());
  int len = vertical ? qw->height () : qw->width ();
  double coord = vertical ? ev->y : ev->x;
  if (len > 0) r->position = r->lower + (r->upper - r->lower) * coord / len;
  qw->update ();
  return FALSE;
}

static QObject *hruler_create (GtkWidget *) { return new GtkQtRuler; }
static QObject *vruler_create (GtkWidget *) { auto *r = new GtkQtRuler; r->vertical = true; return r; }

G_DEFINE_TYPE (GtkRuler, gtk_ruler, GTK_TYPE_WIDGET)
static void gtk_ruler_init (GtkRuler *r) { r->upper = 100; r->max_size = 100; }
static void gtk_ruler_class_init (GtkRulerClass *klass) { GTK_WIDGET_CLASS (klass)->motion_notify_event = ruler_motion; }

typedef GtkRuler GtkHRuler, GtkVRuler;
typedef GtkRulerClass GtkHRulerClass, GtkVRulerClass;
G_DEFINE_TYPE (GtkHRuler, gtk_hruler, GTK_TYPE_RULER)
static void gtk_hruler_init (GtkHRuler *) {}
static void gtk_hruler_class_init (GtkHRulerClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), hruler_create); }
G_DEFINE_TYPE (GtkVRuler, gtk_vruler, GTK_TYPE_RULER)
static void gtk_vruler_init (GtkVRuler *) {}
static void gtk_vruler_class_init (GtkVRulerClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), vruler_create); }

extern "C" {
GtkWidget *gtk_hruler_new (void) { return (GtkWidget *) g_object_new (gtk_hruler_get_type (), nullptr); }
GtkWidget *gtk_vruler_new (void) { return (GtkWidget *) g_object_new (gtk_vruler_get_type (), nullptr); }
void gtk_ruler_set_metric (GtkRuler *r, GtkMetricType m) { r->metric = m; gtk_widget_queue_draw (GTK_WIDGET (r)); }
void gtk_ruler_set_range (GtkRuler *r, gdouble lower, gdouble upper, gdouble position, gdouble max_size)
{
  r->lower = lower; r->upper = upper; r->position = position; r->max_size = max_size;
  gtk_widget_queue_draw (GTK_WIDGET (r));
}
}

/* ======================================================================
 * GtkCalendar: a QCalendarWidget
 * ====================================================================== */
static void calendar_sync (GtkWidget *w)
{
  auto *c = GTK_CALENDAR (w);
  auto *cal = gtkqt_q<QCalendarWidget> (w);
  if (!cal) return;
  QDate d = cal->selectedDate ();
  c->year = cal->yearShown ();
  c->month = cal->monthShown () - 1;
  c->selected_day = d.month () == cal->monthShown () && d.year () == cal->yearShown () ? d.day () : 0;
}

static void calendar_paint_marks (GtkCalendar *c)
{
  auto *cal = gtkqt_q<QCalendarWidget> (GTK_WIDGET (c));
  if (!cal) return;
  QDate first (c->year, c->month + 1, 1);
  QTextCharFormat bold, plain;
  bold.setFontWeight (QFont::Bold);
  for (int d = 1; d <= first.daysInMonth () && d <= 31; d++)
    cal->setDateTextFormat (QDate (c->year, c->month + 1, d), c->marked_date[d - 1] ? bold : plain);
}

static QObject *calendar_create (GtkWidget *w)
{
  auto *cal = new QCalendarWidget;
  cal->setGridVisible (false);
  gtkqt_connect (w, cal, &QCalendarWidget::selectionChanged, [] (GtkWidget *cw) {
    calendar_sync (cw);
    g_signal_emit_by_name (cw, "day-selected");
  });
  gtkqt_connect (w, cal, &QCalendarWidget::activated, [] (GtkWidget *cw, QDate) {
    g_signal_emit_by_name (cw, "day-selected-double-click");
  });
  gtkqt_connect (w, cal, &QCalendarWidget::currentPageChanged, [] (GtkWidget *cw, int, int) {
    calendar_sync (cw);
    calendar_paint_marks (GTK_CALENDAR (cw));
    g_signal_emit_by_name (cw, "month-changed");
  });
  return cal;
}

G_DEFINE_TYPE (GtkCalendar, gtk_calendar, GTK_TYPE_WIDGET)
static void gtk_calendar_init (GtkCalendar *c)
{
  QDate d = QDate::currentDate ();
  c->year = d.year (); c->month = d.month () - 1; c->selected_day = d.day ();
}
static void gtk_calendar_class_init (GtkCalendarClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), calendar_create);
  GType t = G_TYPE_FROM_CLASS (klass);
  for (const char *s : { "day-selected", "day-selected-double-click", "month-changed", "prev-month", "next-month", "prev-year", "next-year" })
    g_signal_new (s, t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}

extern "C" {
GtkWidget *gtk_calendar_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_CALENDAR, nullptr); }
gint gtk_calendar_select_month (GtkCalendar *c, guint month, guint year)
{
  if (auto *cal = gtkqt_q<QCalendarWidget> (GTK_WIDGET (c))) cal->setCurrentPage (year, month + 1);
  c->month = month; c->year = year;
  calendar_paint_marks (c);
  return TRUE;
}
void gtk_calendar_select_day (GtkCalendar *c, guint day)
{
  if (auto *cal = gtkqt_q<QCalendarWidget> (GTK_WIDGET (c)); cal && day > 0) cal->setSelectedDate (QDate (c->year, c->month + 1, day));
  c->selected_day = day;
}
gint gtk_calendar_mark_day (GtkCalendar *c, guint day)
{
  if (day < 1 || day > 31) return FALSE;
  if (!c->marked_date[day - 1]) { c->marked_date[day - 1] = 1; c->num_marked_dates++; }
  calendar_paint_marks (c);
  return TRUE;
}
gint gtk_calendar_unmark_day (GtkCalendar *c, guint day)
{
  if (day < 1 || day > 31) return FALSE;
  if (c->marked_date[day - 1]) { c->marked_date[day - 1] = 0; c->num_marked_dates--; }
  calendar_paint_marks (c);
  return TRUE;
}
void gtk_calendar_clear_marks (GtkCalendar *c)
{
  memset (c->marked_date, 0, sizeof c->marked_date);
  c->num_marked_dates = 0;
  calendar_paint_marks (c);
}
void gtk_calendar_display_options (GtkCalendar *c, GtkCalendarDisplayOptions flags)
{
  auto *cal = gtkqt_q<QCalendarWidget> (GTK_WIDGET (c));
  if (!cal) return;
  cal->setNavigationBarVisible (flags & GTK_CALENDAR_SHOW_HEADING);
  cal->setHorizontalHeaderFormat (flags & GTK_CALENDAR_SHOW_DAY_NAMES ? QCalendarWidget::ShortDayNames : QCalendarWidget::NoHorizontalHeader);
  cal->setVerticalHeaderFormat (flags & GTK_CALENDAR_SHOW_WEEK_NUMBERS ? QCalendarWidget::ISOWeekNumbers : QCalendarWidget::NoVerticalHeader);
  cal->setFirstDayOfWeek (flags & GTK_CALENDAR_WEEK_START_MONDAY ? Qt::Monday : Qt::Sunday);
}
void gtk_calendar_get_date (GtkCalendar *c, guint *year, guint *month, guint *day)
{
  calendar_sync (GTK_WIDGET (c));
  if (year) *year = c->year;
  if (month) *month = c->month;
  if (day) *day = c->selected_day;
}
void gtk_calendar_freeze (GtkCalendar *) {}
void gtk_calendar_thaw (GtkCalendar *) {}
}

/* ======================================================================
 * GtkPacker: Tk's packing algorithm as a QLayout
 * ====================================================================== */
class GtkQtPackerLayout : public QLayout {
public:
  GtkPacker *packer = nullptr;
  QList<QLayoutItem *> items;
  ~GtkQtPackerLayout () override { while (!items.isEmpty ()) delete items.takeFirst (); }
  void addItem (QLayoutItem *item) override { items << item; }
  int count () const override { return items.size (); }
  QLayoutItem *itemAt (int i) const override { return items.value (i); }
  QLayoutItem *takeAt (int i) override { return i >= 0 && i < items.size () ? items.takeAt (i) : nullptr; }

  const GtkPackerChild *child_for (QLayoutItem *it) const
  {
    for (GList *l = packer ? packer->children : nullptr; l; l = l->next) {
      auto *pc = (GtkPackerChild *) l->data;
      if (gtkqt_qw (pc->widget) == it->widget ()) return pc;
    }
    return nullptr;
  }
  QSize child_req (const GtkPackerChild *pc, QLayoutItem *it) const
  {
    QSize s = it->sizeHint ().expandedTo (it->minimumSize ());
    return QSize (s.width () + 2 * (pc->pad_x + pc->i_pad_x + pc->border_width), s.height () + 2 * (pc->pad_y + pc->i_pad_y + pc->border_width));
  }
  QList<QPair<const GtkPackerChild *, QLayoutItem *>> ordered () const
  {
    QList<QPair<const GtkPackerChild *, QLayoutItem *>> out;
    for (GList *l = packer ? packer->children : nullptr; l; l = l->next) {
      auto *pc = (GtkPackerChild *) l->data;
      QWidget *cw = gtkqt_qw (pc->widget);
      for (QLayoutItem *it : items) if (it->widget () == cw && cw && !cw->isHidden ()) out << qMakePair ((const GtkPackerChild *) pc, it);
    }
    return out;
  }
  QSize sizeHint () const override
  {
    auto kids = ordered ();
    int w = 0, h = 0;
    for (int i = kids.size () - 1; i >= 0; i--) {
      QSize r = child_req (kids[i].first, kids[i].second);
      if (kids[i].first->side == GTK_SIDE_TOP || kids[i].first->side == GTK_SIDE_BOTTOM) { h += r.height (); w = qMax (w, r.width ()); }
      else { w += r.width (); h = qMax (h, r.height ()); }
    }
    QMargins m = contentsMargins ();
    return QSize (w + m.left () + m.right (), h + m.top () + m.bottom ());
  }
  QSize minimumSize () const override { return sizeHint (); }
  void setGeometry (const QRect &r) override
  {
    QLayout::setGeometry (r);
    QRect cav = contentsRect ();
    auto kids = ordered ();
    for (int i = 0; i < kids.size (); i++) {
      const GtkPackerChild *pc = kids[i].first;
      QSize req = child_req (pc, kids[i].second);
      bool vert = pc->side == GTK_SIDE_TOP || pc->side == GTK_SIDE_BOTTOM;
      /* share the space left over by the remaining children among the expanding ones */
      int rest = 0, nexp = 0;
      for (int j = i; j < kids.size (); j++) {
        bool v2 = kids[j].first->side == GTK_SIDE_TOP || kids[j].first->side == GTK_SIDE_BOTTOM;
        if (v2 != vert) continue;
        QSize rj = child_req (kids[j].first, kids[j].second);
        rest += vert ? rj.height () : rj.width ();
        if (kids[j].first->options & GTK_PACK_EXPAND) nexp++;
      }
      int avail = vert ? cav.height () : cav.width ();
      int extent = vert ? req.height () : req.width ();
      if ((pc->options & GTK_PACK_EXPAND) && nexp > 0) extent += qMax (0, avail - rest) / nexp;
      extent = qMin (extent, avail);
      QRect frame;
      switch (pc->side) {
      case GTK_SIDE_TOP: frame = QRect (cav.left (), cav.top (), cav.width (), extent); cav.setTop (cav.top () + extent); break;
      case GTK_SIDE_BOTTOM: frame = QRect (cav.left (), cav.bottom () - extent + 1, cav.width (), extent); cav.setBottom (cav.bottom () - extent); break;
      case GTK_SIDE_LEFT: frame = QRect (cav.left (), cav.top (), extent, cav.height ()); cav.setLeft (cav.left () + extent); break;
      default: frame = QRect (cav.right () - extent + 1, cav.top (), extent, cav.height ()); cav.setRight (cav.right () - extent); break;
      }
      int bx = pc->pad_x + pc->border_width, by = pc->pad_y + pc->border_width;
      QRect inner = frame.adjusted (bx, by, -bx, -by);
      QSize nat (req.width () - 2 * (pc->pad_x + pc->border_width), req.height () - 2 * (pc->pad_y + pc->border_width));
      int cw = (pc->options & GTK_FILL_X) ? inner.width () : qMin (nat.width (), inner.width ());
      int ch = (pc->options & GTK_FILL_Y) ? inner.height () : qMin (nat.height (), inner.height ());
      int x = inner.left () + (inner.width () - cw) / 2, y = inner.top () + (inner.height () - ch) / 2;
      switch (pc->anchor) {
      case GTK_ANCHOR_NORTH: case GTK_ANCHOR_NORTH_WEST: case GTK_ANCHOR_NORTH_EAST: y = inner.top (); break;
      case GTK_ANCHOR_SOUTH: case GTK_ANCHOR_SOUTH_WEST: case GTK_ANCHOR_SOUTH_EAST: y = inner.bottom () - ch + 1; break;
      default: break;
      }
      switch (pc->anchor) {
      case GTK_ANCHOR_WEST: case GTK_ANCHOR_NORTH_WEST: case GTK_ANCHOR_SOUTH_WEST: x = inner.left (); break;
      case GTK_ANCHOR_EAST: case GTK_ANCHOR_NORTH_EAST: case GTK_ANCHOR_SOUTH_EAST: x = inner.right () - cw + 1; break;
      default: break;
      }
      kids[i].second->setGeometry (QRect (x, y, qMax (0, cw), qMax (0, ch)));
    }
  }
};

static QObject *packer_create (GtkWidget *w)
{
  auto *qw = new QWidget;
  auto *l = new GtkQtPackerLayout;
  l->packer = GTK_PACKER (w);
  l->setContentsMargins (0, 0, 0, 0);
  qw->setLayout (l);
  return qw;
}

static void packer_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (c)), *cw = gtkqt_qw (child);
  if (qw && cw && qw->layout ()) qw->layout ()->addWidget (cw);
}

static void packer_unplace (GtkContainer *c, GtkWidget *child)
{
  auto *p = GTK_PACKER (c);
  for (GList *l = p->children; l; l = l->next)
    if (((GtkPackerChild *) l->data)->widget == child) { g_free (l->data); p->children = g_list_delete_link (p->children, l); break; }
  if (QWidget *cw = gtkqt_qw (child)) {
    if (QWidget *pw = cw->parentWidget (); pw && pw->layout ()) pw->layout ()->removeWidget (cw);
    cw->hide ();
    cw->setParent (nullptr);
  }
}

static void packer_add (GtkContainer *c, GtkWidget *child)
{
  gtk_packer_add_defaults (GTK_PACKER (c), child, GTK_SIDE_TOP, GTK_ANCHOR_CENTER, (GtkPackerOptions) 0);
}

G_DEFINE_TYPE (GtkPacker, gtk_packer, GTK_TYPE_CONTAINER)
static void gtk_packer_init (GtkPacker *) {}
static void gtk_packer_class_init (GtkPackerClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), packer_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), packer_place, packer_unplace);
  GTK_CONTAINER_CLASS (klass)->add = packer_add;
}

extern "C" {

GtkWidget *gtk_packer_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_PACKER, nullptr); }

void gtk_packer_add (GtkPacker *p, GtkWidget *child, GtkSideType side, GtkAnchorType anchor, GtkPackerOptions options,
                     guint border_width, guint pad_x, guint pad_y, guint i_pad_x, guint i_pad_y)
{
  auto *pc = g_new0 (GtkPackerChild, 1);
  pc->widget = child;
  pc->side = side; pc->anchor = anchor; pc->options = options;
  pc->border_width = border_width; pc->pad_x = pad_x; pc->pad_y = pad_y; pc->i_pad_x = i_pad_x; pc->i_pad_y = i_pad_y;
  p->children = g_list_append (p->children, pc);
  GTK_CONTAINER_CLASS (g_type_class_peek (GTK_TYPE_CONTAINER))->add (GTK_CONTAINER (p), child);
}

void gtk_packer_add_defaults (GtkPacker *p, GtkWidget *child, GtkSideType side, GtkAnchorType anchor, GtkPackerOptions options)
{
  gtk_packer_add (p, child, side, anchor, options, p->default_border_width, p->default_pad_x, p->default_pad_y,
                  p->default_i_pad_x, p->default_i_pad_y);
  ((GtkPackerChild *) g_list_last (p->children)->data)->use_default = 1;
}

void gtk_packer_set_child_packing (GtkPacker *p, GtkWidget *child, GtkSideType side, GtkAnchorType anchor, GtkPackerOptions options,
                                   guint border_width, guint pad_x, guint pad_y, guint i_pad_x, guint i_pad_y)
{
  for (GList *l = p->children; l; l = l->next) {
    auto *pc = (GtkPackerChild *) l->data;
    if (pc->widget != child) continue;
    pc->side = side; pc->anchor = anchor; pc->options = options; pc->use_default = 0;
    pc->border_width = border_width; pc->pad_x = pad_x; pc->pad_y = pad_y; pc->i_pad_x = i_pad_x; pc->i_pad_y = i_pad_y;
  }
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (p)); qw && qw->layout ()) qw->layout ()->invalidate ();
}

void gtk_packer_reorder_child (GtkPacker *p, GtkWidget *child, gint position)
{
  for (GList *l = p->children; l; l = l->next)
    if (((GtkPackerChild *) l->data)->widget == child) {
      gpointer pc = l->data;
      p->children = g_list_delete_link (p->children, l);
      p->children = g_list_insert (p->children, pc, position);
      break;
    }
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (p)); qw && qw->layout ()) qw->layout ()->invalidate ();
}

void gtk_packer_set_spacing (GtkPacker *p, guint spacing) { p->spacing = spacing; }
void gtk_packer_set_default_border_width (GtkPacker *p, guint b) { p->default_border_width = b; }
void gtk_packer_set_default_pad (GtkPacker *p, guint x, guint y) { p->default_pad_x = x; p->default_pad_y = y; }
void gtk_packer_set_default_ipad (GtkPacker *p, guint x, guint y) { p->default_i_pad_x = x; p->default_i_pad_y = y; }

} /* extern "C" */

/* ======================================================================
 * GtkTooltips
 * ====================================================================== */
struct _GtkTooltips { GObject parent_instance; gboolean enabled; GSList *widgets; };
typedef struct { GObjectClass parent_class; } GtkTooltipsClass;
G_DEFINE_TYPE (GtkTooltips, gtk_tooltips, G_TYPE_INITIALLY_UNOWNED)
static void gtk_tooltips_init (GtkTooltips *t) { t->enabled = TRUE; }
static void gtk_tooltips_class_init (GtkTooltipsClass *) {}

extern "C" {
GtkTooltips *gtk_tooltips_new (void) { return (GtkTooltips *) g_object_new (gtk_tooltips_get_type (), nullptr); }
static void tooltips_apply (GtkTooltips *t)
{
  for (GSList *l = t->widgets; l; l = l->next) {
    auto *w = (GtkWidget *) l->data;
    const char *tip = (const char *) g_object_get_data (G_OBJECT (w), "gtkqt-tooltips-tip");
    gtk_widget_set_tooltip_text (w, t->enabled ? tip : nullptr);
  }
}
void gtk_tooltips_enable (GtkTooltips *t) { t->enabled = TRUE; tooltips_apply (t); }
void gtk_tooltips_disable (GtkTooltips *t) { t->enabled = FALSE; tooltips_apply (t); }
void gtk_tooltips_set_delay (GtkTooltips *, guint) {}
void gtk_tooltips_set_tip (GtkTooltips *t, GtkWidget *widget, const gchar *tip_text, const gchar *)
{
  if (!widget) return;
  g_object_set_data_full (G_OBJECT (widget), "gtkqt-tooltips-tip", g_strdup (tip_text), g_free);
  if (!g_slist_find (t->widgets, widget)) {
    t->widgets = g_slist_prepend (t->widgets, widget);
    g_object_weak_ref (G_OBJECT (widget), [] (gpointer tp, GObject *dead) {
      auto *tt = (GtkTooltips *) tp;
      tt->widgets = g_slist_remove (tt->widgets, dead);
    }, t);
  }
  gtk_widget_set_tooltip_text (widget, t->enabled ? tip_text : nullptr);
}
void gtk_tooltips_set_colors (GtkTooltips *, GdkColor *, GdkColor *) {}
void gtk_tooltips_force_window (GtkTooltips *) {}
}

/* ======================================================================
 * GtkColorSelection (+ dialog): QColorDialog without its buttons
 * ====================================================================== */
static QObject *colorsel_create (GtkWidget *)
{
  auto *d = new QColorDialog;
  d->setOptions (QColorDialog::NoButtons | QColorDialog::DontUseNativeDialog);
  d->setWindowFlags (Qt::Widget);
  return d;
}

G_DEFINE_TYPE (GtkColorSelection, gtk_color_selection, GTK_TYPE_BOX)
static void gtk_color_selection_init (GtkColorSelection *) {}
static void gtk_color_selection_class_init (GtkColorSelectionClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), colorsel_create);
  g_signal_new ("color-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}

G_DEFINE_TYPE (GtkColorSelectionDialog, gtk_color_selection_dialog, GTK_TYPE_DIALOG)
static void gtk_color_selection_dialog_init (GtkColorSelectionDialog *) {}
static void gtk_color_selection_dialog_class_init (GtkColorSelectionDialogClass *) {}

extern "C" {

GtkWidget *gtk_color_selection_new (void)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_COLOR_SELECTION, nullptr);
  if (auto *d = gtkqt_q<QColorDialog> (w))
    gtkqt_connect (w, d, &QColorDialog::currentColorChanged, [] (GtkWidget *cw, const QColor &) { g_signal_emit_by_name (cw, "color-changed"); });
  return w;
}

void gtk_color_selection_set_color (GtkColorSelection *cs, gdouble *color)
{
  if (auto *d = gtkqt_q<QColorDialog> (GTK_WIDGET (cs)))
    d->setCurrentColor (QColor::fromRgbF (qBound (0.0, color[0], 1.0), qBound (0.0, color[1], 1.0), qBound (0.0, color[2], 1.0)));
}
void gtk_color_selection_get_color (GtkColorSelection *cs, gdouble *color)
{
  QColor c = gtkqt_q<QColorDialog> (GTK_WIDGET (cs)) ? gtkqt_q<QColorDialog> (GTK_WIDGET (cs))->currentColor () : QColor (Qt::black);
  color[0] = c.redF (); color[1] = c.greenF (); color[2] = c.blueF ();
  color[3] = c.alphaF ();
}
void gtk_color_selection_set_current_color (GtkColorSelection *cs, const GdkColor *c)
{
  gdouble v[4] = { c->red / 65535.0, c->green / 65535.0, c->blue / 65535.0, 1 };
  gtk_color_selection_set_color (cs, v);
}
void gtk_color_selection_get_current_color (GtkColorSelection *cs, GdkColor *c)
{
  gdouble v[4];
  gtk_color_selection_get_color (cs, v);
  c->red = (guint16) (v[0] * 65535); c->green = (guint16) (v[1] * 65535); c->blue = (guint16) (v[2] * 65535);
  c->pixel = ((c->red >> 8) << 16) | ((c->green >> 8) << 8) | (c->blue >> 8);
}
void gtk_color_selection_set_opacity (GtkColorSelection *cs, gboolean use_opacity)
{
  if (auto *d = gtkqt_q<QColorDialog> (GTK_WIDGET (cs))) d->setOption (QColorDialog::ShowAlphaChannel, use_opacity);
}
void gtk_color_selection_set_update_policy (GtkColorSelection *, GtkUpdateType) {}

GtkWidget *gtk_color_selection_dialog_new (const gchar *title)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_COLOR_SELECTION_DIALOG, nullptr);
  auto *d = GTK_COLOR_SELECTION_DIALOG (w);
  if (title) gtk_window_set_title (GTK_WINDOW (w), title);
  d->main_vbox = GTK_DIALOG (w)->vbox;
  d->colorsel = gtk_color_selection_new ();
  gtk_box_pack_start (GTK_BOX (d->main_vbox), d->colorsel, TRUE, TRUE, 0);
  gtk_widget_show (d->colorsel);
  d->cancel_button = gtk_dialog_add_button (GTK_DIALOG (w), "gtk-cancel", GTK_RESPONSE_CANCEL);
  d->ok_button = gtk_dialog_add_button (GTK_DIALOG (w), "gtk-ok", GTK_RESPONSE_OK);
  d->help_button = gtk_button_new_from_stock ("gtk-help");
  g_object_ref_sink (d->help_button);
  return w;
}

} /* extern "C" */
