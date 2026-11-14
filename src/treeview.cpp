/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkListStore / GtkTreeStore (rows of GValues, as in GTK), GtkTreePath,
 * cell renderers, GtkTreeViewColumn, GtkTreeView and GtkTreeSelection.
 *
 * Each view gets a QAbstractItemModel adapter. For every cell the adapter
 * does what GTK does: copy the mapped model columns into the renderer's
 * properties ("text", "active", ...), run the cell data function, then read
 * the renderer's properties back as Qt item roles.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <gobject/gvaluecollector.h>
#include <QtWidgets>
#include <algorithm>
#include <vector>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

class GtkQtItemModel;

/* ======================================================================
 * Rows and stores
 * ====================================================================== */
struct Row {
  std::vector<GValue> values;
  Row *parent = nullptr;
  QList<Row *> children;
  ~Row () { for (GValue &v : values) if (G_IS_VALUE (&v)) g_value_unset (&v); qDeleteAll (children); }
  int index () const { return parent ? parent->children.indexOf (const_cast<Row *> (this)) : 0; }
};

struct SortFunc { GtkTreeIterCompareFunc func; gpointer data; GDestroyNotify destroy; };

struct StoreData {
  QVector<GType> types;
  Row root;
  gint stamp = g_random_int ();
  QList<QPointer<GtkQtItemModel>> adapters;
  int sort_column = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
  GtkSortType sort_order = GTK_SORT_ASCENDING;
  QMap<int, SortFunc> sort_funcs;
  bool is_list = true;
};

struct StoreObj { GObject parent_instance; gpointer priv; };
static StoreData *store (gpointer model) { return static_cast<StoreData *> (((StoreObj *) model)->priv); }

/* GtkTreeModel as a real (empty) interface so it can be used as a property type */
extern "C" GType gtk_tree_model_get_type (void)
{
  static GType t = 0;
  if (!t) {
    t = g_type_register_static_simple (G_TYPE_INTERFACE, "GtkTreeModel", sizeof (GTypeInterface), nullptr, 0, nullptr, (GTypeFlags) 0);
    g_type_interface_add_prerequisite (t, G_TYPE_OBJECT);
  }
  return t;
}

static void model_iface_init (gpointer, gpointer) {}

struct _GtkListStore { GObject parent_instance; gpointer priv; };
struct _GtkTreeStore { GObject parent_instance; gpointer priv; };
typedef struct { GObjectClass parent_class; } GtkListStoreClass;
typedef struct { GObjectClass parent_class; } GtkTreeStoreClass;

static void store_finalize_impl (GObject *o)
{
  StoreData *d = store (o);
  for (SortFunc &f : d->sort_funcs) if (f.destroy) f.destroy (f.data);
  delete d;
}

G_DEFINE_TYPE_WITH_CODE (GtkListStore, gtk_list_store, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (gtk_tree_model_get_type (), model_iface_init))
G_DEFINE_TYPE_WITH_CODE (GtkTreeStore, gtk_tree_store, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (gtk_tree_model_get_type (), model_iface_init))

static void gtk_list_store_init (GtkListStore *s) { s->priv = new StoreData; }
static void gtk_tree_store_init (GtkTreeStore *s) { s->priv = new StoreData; store (s)->is_list = false; }
static void list_finalize (GObject *o) { store_finalize_impl (o); G_OBJECT_CLASS (gtk_list_store_parent_class)->finalize (o); }
static void tree_finalize (GObject *o) { store_finalize_impl (o); G_OBJECT_CLASS (gtk_tree_store_parent_class)->finalize (o); }

static void model_signals (GType t)
{
  g_signal_new ("row-changed", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
  g_signal_new ("row-inserted", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
  g_signal_new ("row-deleted", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_POINTER);
  g_signal_new ("sort-column-changed", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}

static void gtk_list_store_class_init (GtkListStoreClass *klass) { G_OBJECT_CLASS (klass)->finalize = list_finalize; model_signals (G_TYPE_FROM_CLASS (klass)); }
static void gtk_tree_store_class_init (GtkTreeStoreClass *klass) { G_OBJECT_CLASS (klass)->finalize = tree_finalize; model_signals (G_TYPE_FROM_CLASS (klass)); }

static Row *iter_row (GtkTreeModel *m, GtkTreeIter *it) { return it && it->stamp == store (m)->stamp ? static_cast<Row *> (it->user_data) : nullptr; }
static void set_iter (GtkTreeModel *m, GtkTreeIter *it, Row *r)
{
  if (!it) return;
  it->stamp = r ? store (m)->stamp : 0;
  it->user_data = r;
  it->user_data2 = it->user_data3 = nullptr;
}

static Row *new_row (StoreData *d)
{
  auto *r = new Row;
  r->values.resize (d->types.size ());
  for (int i = 0; i < d->types.size (); i++) {
    r->values[i] = G_VALUE_INIT;
    g_value_init (&r->values[i], d->types[i]);
  }
  return r;
}

/* ======================================================================
 * GtkTreePath
 * ====================================================================== */
struct _GtkTreePath { std::vector<gint> idx; };

extern "C" {
GType gtk_tree_path_get_type (void)
{
  static GType t = g_boxed_type_register_static ("GtkTreePath", (GBoxedCopyFunc) gtk_tree_path_copy, (GBoxedFreeFunc) gtk_tree_path_free);
  return t;
}
GType gtk_tree_iter_get_type (void)
{
  static GType t = g_boxed_type_register_static ("GtkTreeIter", (GBoxedCopyFunc) gtk_tree_iter_copy, (GBoxedFreeFunc) gtk_tree_iter_free);
  return t;
}
GtkTreePath *gtk_tree_path_new (void) { return new GtkTreePath; }
GtkTreePath *gtk_tree_path_new_first (void) { auto *p = new GtkTreePath; p->idx.push_back (0); return p; }

GtkTreePath *gtk_tree_path_new_from_string (const gchar *path)
{
  auto *p = new GtkTreePath;
  for (const QString &s : gtkqt_str (path).split (':', Qt::SkipEmptyParts)) p->idx.push_back (s.toInt ());
  if (p->idx.empty ()) { delete p; return nullptr; }
  return p;
}

GtkTreePath *gtk_tree_path_new_from_indices (gint first_index, ...)
{
  auto *p = new GtkTreePath;
  va_list ap;
  va_start (ap, first_index);
  for (gint i = first_index; i != -1; i = va_arg (ap, gint)) p->idx.push_back (i);
  va_end (ap);
  return p;
}

GtkTreePath *gtk_tree_path_copy (const GtkTreePath *path) { return path ? new GtkTreePath (*path) : nullptr; }
void gtk_tree_path_free (GtkTreePath *path) { delete path; }

gchar *gtk_tree_path_to_string (GtkTreePath *path)
{
  QStringList l;
  for (int i : path->idx) l << QString::number (i);
  return g_strdup (l.join (':').toUtf8 ().constData ());
}

void gtk_tree_path_append_index (GtkTreePath *path, gint i) { path->idx.push_back (i); }
void gtk_tree_path_prepend_index (GtkTreePath *path, gint i) { path->idx.insert (path->idx.begin (), i); }
gint gtk_tree_path_get_depth (GtkTreePath *path) { return path->idx.size (); }
gint *gtk_tree_path_get_indices (GtkTreePath *path) { return path->idx.empty () ? nullptr : path->idx.data (); }
gint *gtk_tree_path_get_indices_with_depth (GtkTreePath *path, gint *depth) { if (depth) *depth = path->idx.size (); return gtk_tree_path_get_indices (path); }

gint gtk_tree_path_compare (const GtkTreePath *a, const GtkTreePath *b)
{
  return a->idx < b->idx ? -1 : a->idx == b->idx ? 0 : 1;
}

void gtk_tree_path_next (GtkTreePath *path) { if (!path->idx.empty ()) path->idx.back ()++; }
gboolean gtk_tree_path_prev (GtkTreePath *path) { if (path->idx.empty () || path->idx.back () == 0) return FALSE; path->idx.back ()--; return TRUE; }
gboolean gtk_tree_path_up (GtkTreePath *path) { if (path->idx.empty ()) return FALSE; path->idx.pop_back (); return !path->idx.empty (); }
void gtk_tree_path_down (GtkTreePath *path) { path->idx.push_back (0); }

GtkTreeIter *gtk_tree_iter_copy (GtkTreeIter *iter) { return (GtkTreeIter *) g_memdup2 (iter, sizeof (GtkTreeIter)); }
void gtk_tree_iter_free (GtkTreeIter *iter) { g_free (iter); }
} /* extern "C" */

static GtkTreePath *row_path (Row *r)
{
  auto *p = new GtkTreePath;
  for (Row *x = r; x && x->parent; x = x->parent) p->idx.insert (p->idx.begin (), x->index ());
  return p;
}

static Row *path_row (StoreData *d, GtkTreePath *p)
{
  Row *r = &d->root;
  for (int i : p->idx) {
    if (i < 0 || i >= r->children.size ()) return nullptr;
    r = r->children[i];
  }
  return r == &d->root ? nullptr : r;
}

/* ======================================================================
 * Cell renderers (properties hold the current cell's values)
 * ====================================================================== */
enum RendererKind { R_TEXT, R_TOGGLE, R_PIXBUF, R_PROGRESS, R_SPIN, R_COMBO };
struct RendererData {
  RendererKind kind;
  QString text, markup, foreground, background, icon_name;
  GdkPixbuf *pixbuf = nullptr;
  int weight = 400, value = 0;
  bool active = false, activatable = true, visible = true, editable = false, radio = false, sensitive = true;
  float xalign = 0;
  ~RendererData () { if (pixbuf) g_object_unref (pixbuf); }
};

static RendererData *rd (gpointer c) { return static_cast<RendererData *> (((GtkCellRenderer *) c)->priv); }

G_DEFINE_TYPE (GtkCellRenderer, gtk_cell_renderer, G_TYPE_INITIALLY_UNOWNED)
static void gtk_cell_renderer_init (GtkCellRenderer *c) { c->priv = new RendererData; }
static void renderer_finalize (GObject *o) { delete rd (o); G_OBJECT_CLASS (gtk_cell_renderer_parent_class)->finalize (o); }

#define RSTR(name, field) \
  gtkqt_prop (klass, g_param_spec_string (name, nullptr, nullptr, nullptr, RW), \
              [] (GObject *o, const GValue *v) { rd (o)->field = gtkqt_str (g_value_get_string (v)); }, \
              [] (GObject *o, GValue *v) { g_value_set_string (v, rd (o)->field.toUtf8 ().constData ()); })
#define RBOOL(name, field) \
  gtkqt_prop (klass, g_param_spec_boolean (name, nullptr, nullptr, FALSE, RW), \
              [] (GObject *o, const GValue *v) { rd (o)->field = g_value_get_boolean (v); }, \
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, rd (o)->field); })

static void gtk_cell_renderer_class_init (GtkCellRendererClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = renderer_finalize;
  RSTR ("text", text);
  RSTR ("markup", markup);
  RSTR ("foreground", foreground);
  RSTR ("background", background);
  RSTR ("cell-background", background);
  RSTR ("icon-name", icon_name);
  RSTR ("stock-id", icon_name);
  RBOOL ("active", active);
  RBOOL ("activatable", activatable);
  RBOOL ("visible", visible);
  RBOOL ("editable", editable);
  RBOOL ("radio", radio);
  RBOOL ("sensitive", sensitive);
  gtkqt_prop (klass, g_param_spec_int ("weight", nullptr, nullptr, 0, 1000, 400, RW),
              [] (GObject *o, const GValue *v) { rd (o)->weight = g_value_get_int (v); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("value", nullptr, nullptr, 0, 100, 0, RW),
              [] (GObject *o, const GValue *v) { rd (o)->value = g_value_get_int (v); }, nullptr);
  gtkqt_prop (klass, g_param_spec_float ("xalign", nullptr, nullptr, 0, 1, 0, RW),
              [] (GObject *o, const GValue *v) { rd (o)->xalign = g_value_get_float (v); }, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("pixbuf", nullptr, nullptr, GDK_TYPE_PIXBUF, RW),
              [] (GObject *o, const GValue *v) {
                RendererData *d = rd (o);
                if (d->pixbuf) g_object_unref (d->pixbuf);
                d->pixbuf = (GdkPixbuf *) g_value_dup_object (v);
              }, nullptr);
  gtkqt_prop (klass, g_param_spec_boxed ("foreground-rgba", nullptr, nullptr, GDK_TYPE_RGBA, RW),
              [] (GObject *o, const GValue *v) {
                auto *c = (GdkRGBA *) g_value_get_boxed (v);
                rd (o)->foreground = c ? QColor::fromRgbF (c->red, c->green, c->blue, c->alpha).name (QColor::HexArgb) : QString ();
              }, nullptr);
  g_signal_new ("edited", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
  g_signal_new ("toggled", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static GType renderer_subtype (const char *name)
{
  return g_type_register_static_simple (GTK_TYPE_CELL_RENDERER, g_intern_static_string (name), sizeof (GtkCellRendererClass),
                                        nullptr, sizeof (GtkCellRenderer), nullptr, (GTypeFlags) 0);
}

static GtkCellRenderer *renderer_new (GType t, RendererKind kind)
{
  auto *c = (GtkCellRenderer *) g_object_new (t, nullptr);
  rd (c)->kind = kind;
  if (kind == R_PROGRESS) rd (c)->xalign = 0.5f;
  return c;
}

extern "C" {
GType gtk_cell_renderer_text_get_type (void) { static GType t = renderer_subtype ("GtkCellRendererText"); return t; }
GType gtk_cell_renderer_toggle_get_type (void) { static GType t = renderer_subtype ("GtkCellRendererToggle"); return t; }
GType gtk_cell_renderer_pixbuf_get_type (void) { static GType t = renderer_subtype ("GtkCellRendererPixbuf"); return t; }
GType gtk_cell_renderer_progress_get_type (void) { static GType t = renderer_subtype ("GtkCellRendererProgress"); return t; }
GType gtk_cell_renderer_spin_get_type (void) { static GType t = renderer_subtype ("GtkCellRendererSpin"); return t; }
GType gtk_cell_renderer_combo_get_type (void) { static GType t = renderer_subtype ("GtkCellRendererCombo"); return t; }
GtkCellRenderer *gtk_cell_renderer_text_new (void) { return renderer_new (gtk_cell_renderer_text_get_type (), R_TEXT); }
GtkCellRenderer *gtk_cell_renderer_toggle_new (void) { return renderer_new (gtk_cell_renderer_toggle_get_type (), R_TOGGLE); }
GtkCellRenderer *gtk_cell_renderer_pixbuf_new (void) { return renderer_new (gtk_cell_renderer_pixbuf_get_type (), R_PIXBUF); }
GtkCellRenderer *gtk_cell_renderer_progress_new (void) { return renderer_new (gtk_cell_renderer_progress_get_type (), R_PROGRESS); }
GtkCellRenderer *gtk_cell_renderer_spin_new (void) { return renderer_new (gtk_cell_renderer_spin_get_type (), R_SPIN); }
GtkCellRenderer *gtk_cell_renderer_combo_new (void) { return renderer_new (gtk_cell_renderer_combo_get_type (), R_COMBO); }
void gtk_cell_renderer_toggle_set_radio (GtkCellRendererToggle *t, gboolean radio) { rd (t)->radio = radio; }
void gtk_cell_renderer_toggle_set_active (GtkCellRendererToggle *t, gboolean setting) { rd (t)->active = setting; }
void gtk_cell_renderer_set_alignment (GtkCellRenderer *c, gfloat xalign, gfloat) { rd (c)->xalign = xalign; }
void gtk_cell_renderer_set_padding (GtkCellRenderer *, gint, gint) {}
void gtk_cell_renderer_set_visible (GtkCellRenderer *c, gboolean visible) { rd (c)->visible = visible; }
}

/* A column's list of cells with their attribute mappings (also used by combo boxes). */
struct Cell {
  GtkCellRenderer *renderer;
  QList<QPair<QByteArray, int>> attrs;
  GtkTreeCellDataFunc func = nullptr;
  gpointer func_data = nullptr;
  GDestroyNotify func_destroy = nullptr;
};

struct ColumnData {
  QString title;
  QList<Cell> cells;
  int sort_column_id = -1;
  bool expand = false, resizable = true, visible = true, clickable = false;
  int fixed_width = -1, min_width = -1;
  float xalign = 0;
  GtkWidget *view = nullptr;
};

/* ======================================================================
 * GtkTreeViewColumn
 * ====================================================================== */
struct _GtkTreeViewColumn { GInitiallyUnowned parent_instance; gpointer priv; };
typedef struct { GInitiallyUnownedClass parent_class; } GtkTreeViewColumnClass;
static ColumnData *cd (gpointer c) { return static_cast<ColumnData *> (((GtkTreeViewColumn *) c)->priv); }

static void cells_clear (QList<Cell> &cells)
{
  for (Cell &c : cells) {
    if (c.func_destroy) c.func_destroy (c.func_data);
    g_object_unref (c.renderer);
  }
  cells.clear ();
}

G_DEFINE_TYPE (GtkTreeViewColumn, gtk_tree_view_column, G_TYPE_INITIALLY_UNOWNED)
static void gtk_tree_view_column_init (GtkTreeViewColumn *c) { c->priv = new ColumnData; }
static void column_finalize (GObject *o)
{
  cells_clear (cd (o)->cells);
  delete cd (o);
  G_OBJECT_CLASS (gtk_tree_view_column_parent_class)->finalize (o);
}
static void gtk_tree_view_column_class_init (GtkTreeViewColumnClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = column_finalize;
  g_signal_new ("clicked", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_string ("title", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_tree_view_column_set_title ((GtkTreeViewColumn *) o, g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("expand", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_tree_view_column_set_expand ((GtkTreeViewColumn *) o, g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("resizable", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_tree_view_column_set_resizable ((GtkTreeViewColumn *) o, g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("sort-column-id", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_tree_view_column_set_sort_column_id ((GtkTreeViewColumn *) o, g_value_get_int (v)); }, nullptr);
}

/* ======================================================================
 * The Qt adapter
 * ====================================================================== */
class GtkQtItemModel : public QAbstractItemModel {
public:
  GtkTreeModel *model;
  std::function<QList<GObject *> ()> columns;   /* GtkTreeViewColumn* list, or a single pseudo column */
  std::function<QList<Cell> *(GObject *)> cells_of;

  GtkQtItemModel (GtkTreeModel *m) : model (m) { store (m)->adapters << this; }
  ~GtkQtItemModel () override { if (model) store (model)->adapters.removeAll (this); }

  Row *row (const QModelIndex &i) const { return i.isValid () ? static_cast<Row *> (i.internalPointer ()) : &store (model)->root; }
  QModelIndex index_of (Row *r, int col = 0) const
  {
    if (!r || r == &store (model)->root) return QModelIndex ();
    return createIndex (r->index (), col, r);
  }
  QModelIndex index (int r, int c, const QModelIndex &parent) const override
  {
    Row *p = row (parent);
    if (r < 0 || r >= p->children.size () || c < 0 || c >= columnCount (parent)) return QModelIndex ();
    return createIndex (r, c, p->children[r]);
  }
  QModelIndex parent (const QModelIndex &i) const override
  {
    if (!i.isValid ()) return QModelIndex ();
    Row *r = row (i);
    return index_of (r->parent);
  }
  int rowCount (const QModelIndex &parent) const override
  {
    if (parent.column () > 0) return 0;
    return row (parent)->children.size ();
  }
  int columnCount (const QModelIndex &) const override { return qMax (1, (int) columns ().size ()); }

  /* GTK cell rendering protocol: attributes -> renderer properties -> data func */
  void prepare (GObject *column, Cell &c, Row *r) const
  {
    GtkTreeIter it;
    set_iter (model, &it, r);
    for (auto &a : c.attrs) {
      if (a.second < 0 || a.second >= (int) r->values.size ()) continue;
      GParamSpec *ps = g_object_class_find_property (G_OBJECT_GET_CLASS (c.renderer), a.first.constData ());
      if (!ps) continue;
      GValue v = G_VALUE_INIT;
      g_value_init (&v, ps->value_type);
      if (g_value_type_transformable (G_VALUE_TYPE (&r->values[a.second]), ps->value_type)) {
        g_value_transform (&r->values[a.second], &v);
        g_object_set_property (G_OBJECT (c.renderer), a.first.constData (), &v);
      }
      g_value_unset (&v);
    }
    if (c.func) c.func ((GtkTreeViewColumn *) column, c.renderer, model, &it, c.func_data);
  }

  QVariant data (const QModelIndex &i, int role) const override
  {
    if (!i.isValid ()) return QVariant ();
    QList<GObject *> cols = columns ();
    if (i.column () >= cols.size ()) return QVariant ();
    QList<Cell> *cells = cells_of (cols[i.column ()]);
    if (!cells) return QVariant ();
    Row *r = row (i);
    QVariant out;
    QStringList texts;
    for (Cell &c : *cells) {
      prepare (cols[i.column ()], c, r);
      RendererData *d = rd (c.renderer);
      if (!d->visible) continue;
      switch (d->kind) {
      case R_TOGGLE:
        if (role == Qt::CheckStateRole) out = d->active ? Qt::Checked : Qt::Unchecked;
        break;
      case R_PIXBUF:
        if (role == Qt::DecorationRole) {
          if (d->pixbuf) out = QPixmap::fromImage (gtkqt_pixbuf_image (d->pixbuf));
          else if (!d->icon_name.isEmpty ()) out = gtkqt_icon (d->icon_name.toUtf8 ().constData ());
        }
        break;
      case R_PROGRESS:
        if (role == Qt::UserRole + 1) out = d->value;
        if (role == Qt::DisplayRole) texts << (d->text.isEmpty () ? QStringLiteral ("%1 %").arg (d->value) : d->text);
        break;
      default:
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
          QString t = d->text;
          if (t.isEmpty () && !d->markup.isEmpty ())
            t = QTextDocumentFragment::fromHtml (gtkqt_markup_to_html (d->markup.toUtf8 ().constData ())).toPlainText ();
          texts << t;
        }
        if (role == Qt::ForegroundRole && !d->foreground.isEmpty ()) out = QBrush (QColor (d->foreground));
        if (role == Qt::BackgroundRole && !d->background.isEmpty ()) out = QBrush (QColor (d->background));
        if (role == Qt::FontRole && (d->weight >= 600 || d->markup.contains ("<b>"))) { QFont f; f.setBold (true); out = f; }
        if (role == Qt::TextAlignmentRole) out = int ((d->xalign > 0.75 ? Qt::AlignRight : d->xalign > 0.25 ? Qt::AlignHCenter : Qt::AlignLeft) | Qt::AlignVCenter);
        break;
      }
    }
    if ((role == Qt::DisplayRole || role == Qt::EditRole) && !texts.isEmpty ()) return texts.join (' ');
    return out;
  }

  Qt::ItemFlags flags (const QModelIndex &i) const override
  {
    if (!i.isValid ()) return Qt::NoItemFlags;
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    QList<GObject *> cols = columns ();
    if (i.column () < cols.size ())
      if (QList<Cell> *cells = cells_of (cols[i.column ()]))
        for (Cell &c : *cells) {
          prepare (cols[i.column ()], c, row (i));
          if (rd (c.renderer)->kind == R_TOGGLE && rd (c.renderer)->activatable) f |= Qt::ItemIsUserCheckable;
          if (rd (c.renderer)->kind == R_TEXT && rd (c.renderer)->editable) f |= Qt::ItemIsEditable;
        }
    return f;
  }

  bool setData (const QModelIndex &i, const QVariant &v, int role) override
  {
    /* GTK never edits the model behind your back: it emits "edited"/"toggled"
     * on the renderer and the program updates the store. */
    QList<GObject *> cols = columns ();
    if (!i.isValid () || i.column () >= cols.size ()) return false;
    QList<Cell> *cells = cells_of (cols[i.column ()]);
    if (!cells) return false;
    GtkTreePath *p = row_path (row (i));
    gchar *ps = gtk_tree_path_to_string (p);
    gtk_tree_path_free (p);
    GtkQtEmitGuard guard;
    for (Cell &c : *cells) {
      if (role == Qt::CheckStateRole && rd (c.renderer)->kind == R_TOGGLE) g_signal_emit_by_name (c.renderer, "toggled", ps);
      if (role == Qt::EditRole && rd (c.renderer)->kind == R_TEXT)
        g_signal_emit_by_name (c.renderer, "edited", ps, v.toString ().toUtf8 ().constData ());
    }
    g_free (ps);
    return false;
  }

  QVariant headerData (int section, Qt::Orientation o, int role) const override
  {
    QList<GObject *> cols = columns ();
    if (o == Qt::Horizontal && role == Qt::DisplayRole && section < cols.size () && GTK_IS_TREE_VIEW_COLUMN (cols[section]))
      return cd (cols[section])->title;
    return QVariant ();
  }

  /* change notifications coming from the store */
  void before_insert (Row *parent, int pos) { beginInsertRows (index_of (parent), pos, pos); }
  void after_insert () { endInsertRows (); }
  void before_remove (Row *r) { beginRemoveRows (index_of (r->parent), r->index (), r->index ()); }
  void after_remove () { endRemoveRows (); }
  void changed (Row *r) { Q_EMIT dataChanged (index_of (r, 0), index_of (r, columnCount (QModelIndex ()) - 1)); }
  void relayout_begin () { Q_EMIT layoutAboutToBeChanged (); }
  void relayout_end ()
  {
    for (const QModelIndex &old : persistentIndexList ()) {
      Row *r = static_cast<Row *> (old.internalPointer ());
      changePersistentIndex (old, createIndex (r->index (), old.column (), r));
    }
    Q_EMIT layoutChanged ();
  }
  void reset () { beginResetModel (); endResetModel (); }
  void detach () { beginResetModel (); model = nullptr; endResetModel (); }
};

/* ---------- store operations ---------- */
static void emit_row_signal (GtkTreeModel *m, const char *sig, Row *r)
{
  GtkTreePath *p = row_path (r);
  GtkTreeIter it;
  set_iter (m, &it, r);
  GtkQtEmitGuard guard;
  if (!strcmp (sig, "row-deleted")) g_signal_emit_by_name (m, sig, p);
  else g_signal_emit_by_name (m, sig, p, &it);
  gtk_tree_path_free (p);
}

static int compare_rows (GtkTreeModel *m, Row *a, Row *b)
{
  StoreData *d = store (m);
  int col = d->sort_column;
  if (d->sort_funcs.contains (col)) {
    GtkTreeIter ia, ib;
    set_iter (m, &ia, a);
    set_iter (m, &ib, b);
    return d->sort_funcs[col].func (m, &ia, &ib, d->sort_funcs[col].data);
  }
  if (col < 0 || col >= d->types.size ()) return 0;
  const GValue *va = &a->values[col], *vb = &b->values[col];
  switch (G_TYPE_FUNDAMENTAL (d->types[col])) {
  case G_TYPE_STRING: return g_utf8_collate (g_value_get_string (va) ? g_value_get_string (va) : "", g_value_get_string (vb) ? g_value_get_string (vb) : "");
  case G_TYPE_INT: return (g_value_get_int (va) > g_value_get_int (vb)) - (g_value_get_int (va) < g_value_get_int (vb));
  case G_TYPE_UINT: return (g_value_get_uint (va) > g_value_get_uint (vb)) - (g_value_get_uint (va) < g_value_get_uint (vb));
  case G_TYPE_INT64: return (g_value_get_int64 (va) > g_value_get_int64 (vb)) - (g_value_get_int64 (va) < g_value_get_int64 (vb));
  case G_TYPE_LONG: return (g_value_get_long (va) > g_value_get_long (vb)) - (g_value_get_long (va) < g_value_get_long (vb));
  case G_TYPE_DOUBLE: return (g_value_get_double (va) > g_value_get_double (vb)) - (g_value_get_double (va) < g_value_get_double (vb));
  case G_TYPE_FLOAT: return (g_value_get_float (va) > g_value_get_float (vb)) - (g_value_get_float (va) < g_value_get_float (vb));
  case G_TYPE_BOOLEAN: return g_value_get_boolean (va) - g_value_get_boolean (vb);
  default: return 0;
  }
}

static void sort_children (GtkTreeModel *m, Row *parent)
{
  StoreData *d = store (m);
  std::stable_sort (parent->children.begin (), parent->children.end (), [m, d] (Row *a, Row *b) {
    int c = compare_rows (m, a, b);
    return d->sort_order == GTK_SORT_ASCENDING ? c < 0 : c > 0;
  });
  for (Row *c : parent->children) if (!c->children.isEmpty ()) sort_children (m, c);
}

static void resort (GtkTreeModel *m)
{
  StoreData *d = store (m);
  if (d->sort_column == GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID) return;
  for (auto &a : d->adapters) if (a) a->relayout_begin ();
  sort_children (m, &d->root);
  for (auto &a : d->adapters) if (a) a->relayout_end ();
}

static Row *store_insert (GtkTreeModel *m, Row *parent, int pos, GtkTreeIter *iter)
{
  StoreData *d = store (m);
  if (!parent) parent = &d->root;
  if (pos < 0 || pos > parent->children.size ()) pos = parent->children.size ();
  Row *r = new_row (d);
  r->parent = parent;
  for (auto &a : d->adapters) if (a) a->before_insert (parent, pos);
  parent->children.insert (pos, r);
  for (auto &a : d->adapters) if (a) a->after_insert ();
  set_iter (m, iter, r);
  emit_row_signal (m, "row-inserted", r);
  return r;
}

static void store_set_value (GtkTreeModel *m, Row *r, int col, const GValue *v)
{
  StoreData *d = store (m);
  if (!r || col < 0 || col >= d->types.size ()) return;
  if (G_VALUE_TYPE (v) == d->types[col]) g_value_copy (v, &r->values[col]);
  else if (!g_value_transform (v, &r->values[col])) g_warning ("gtk-made-qt: cannot store %s in column %d", G_VALUE_TYPE_NAME (v), col);
}

static void store_set_valist (GtkTreeModel *m, GtkTreeIter *iter, va_list ap)
{
  StoreData *d = store (m);
  Row *r = iter_row (m, iter);
  if (!r) return;
  for (int col = va_arg (ap, gint); col != -1; col = va_arg (ap, gint)) {
    if (col < 0 || col >= d->types.size ()) { g_warning ("gtk-made-qt: invalid column %d", col); return; }
    GValue v = G_VALUE_INIT;
    gchar *err = nullptr;
    G_VALUE_COLLECT_INIT (&v, d->types[col], ap, 0, &err);
    if (err) { g_warning ("%s", err); g_free (err); return; }
    g_value_copy (&v, &r->values[col]);
    g_value_unset (&v);
  }
  for (auto &a : d->adapters) if (a) a->changed (r);
  emit_row_signal (m, "row-changed", r);
  if (d->sort_column >= 0) resort (m);
}

static gboolean store_remove (GtkTreeModel *m, GtkTreeIter *iter)
{
  StoreData *d = store (m);
  Row *r = iter_row (m, iter);
  if (!r) return FALSE;
  Row *parent = r->parent;
  int pos = r->index ();
  GtkTreePath *p = row_path (r);
  for (auto &a : d->adapters) if (a) a->before_remove (r);
  parent->children.removeAt (pos);
  delete r;
  for (auto &a : d->adapters) if (a) a->after_remove ();
  {
    GtkQtEmitGuard guard;
    g_signal_emit_by_name (m, "row-deleted", p);
  }
  gtk_tree_path_free (p);
  if (pos < parent->children.size ()) { set_iter (m, iter, parent->children[pos]); return TRUE; }
  set_iter (m, iter, nullptr);
  return FALSE;
}

static void store_clear (GtkTreeModel *m)
{
  StoreData *d = store (m);
  while (!d->root.children.isEmpty ()) {
    GtkTreeIter it;
    set_iter (m, &it, d->root.children.last ());
    store_remove (m, &it);
  }
}

static void store_init_types (GtkTreeModel *m, gint n, GType *types)
{
  StoreData *d = store (m);
  d->types.clear ();
  for (int i = 0; i < n; i++) d->types << types[i];
}

extern "C" {

GtkListStore *gtk_list_store_newv (gint n_columns, GType *types)
{
  auto *s = (GtkListStore *) g_object_new (GTK_TYPE_LIST_STORE, nullptr);
  store_init_types (GTK_TREE_MODEL (s), n_columns, types);
  return s;
}

GtkListStore *gtk_list_store_new (gint n_columns, ...)
{
  QVector<GType> t;
  va_list ap;
  va_start (ap, n_columns);
  for (int i = 0; i < n_columns; i++) t << va_arg (ap, GType);
  va_end (ap);
  return gtk_list_store_newv (n_columns, t.data ());
}

void gtk_list_store_set_valist (GtkListStore *s, GtkTreeIter *iter, va_list ap) { store_set_valist (GTK_TREE_MODEL (s), iter, ap); }

void gtk_list_store_set (GtkListStore *s, GtkTreeIter *iter, ...)
{
  va_list ap;
  va_start (ap, iter);
  store_set_valist (GTK_TREE_MODEL (s), iter, ap);
  va_end (ap);
}

void gtk_list_store_set_value (GtkListStore *s, GtkTreeIter *iter, gint column, GValue *value)
{
  Row *r = iter_row (GTK_TREE_MODEL (s), iter);
  store_set_value (GTK_TREE_MODEL (s), r, column, value);
  if (r) { for (auto &a : store (s)->adapters) if (a) a->changed (r); emit_row_signal (GTK_TREE_MODEL (s), "row-changed", r); }
}

gboolean gtk_list_store_remove (GtkListStore *s, GtkTreeIter *iter) { return store_remove (GTK_TREE_MODEL (s), iter); }
void gtk_list_store_insert (GtkListStore *s, GtkTreeIter *iter, gint position) { store_insert (GTK_TREE_MODEL (s), nullptr, position, iter); }

void gtk_list_store_insert_with_values (GtkListStore *s, GtkTreeIter *iter, gint position, ...)
{
  GtkTreeIter local;
  if (!iter) iter = &local;
  store_insert (GTK_TREE_MODEL (s), nullptr, position, iter);
  va_list ap;
  va_start (ap, position);
  store_set_valist (GTK_TREE_MODEL (s), iter, ap);
  va_end (ap);
}

void gtk_list_store_prepend (GtkListStore *s, GtkTreeIter *iter) { store_insert (GTK_TREE_MODEL (s), nullptr, 0, iter); }
void gtk_list_store_append (GtkListStore *s, GtkTreeIter *iter) { store_insert (GTK_TREE_MODEL (s), nullptr, -1, iter); }
void gtk_list_store_clear (GtkListStore *s) { store_clear (GTK_TREE_MODEL (s)); }
gboolean gtk_list_store_iter_is_valid (GtkListStore *s, GtkTreeIter *iter) { return iter_row (GTK_TREE_MODEL (s), iter) != nullptr; }

void gtk_list_store_swap (GtkListStore *s, GtkTreeIter *a, GtkTreeIter *b)
{
  Row *ra = iter_row (GTK_TREE_MODEL (s), a), *rb = iter_row (GTK_TREE_MODEL (s), b);
  if (!ra || !rb) return;
  StoreData *d = store (s);
  for (auto &ad : d->adapters) if (ad) ad->relayout_begin ();
  d->root.children.swapItemsAt (ra->index (), rb->index ());
  for (auto &ad : d->adapters) if (ad) ad->relayout_end ();
}

GtkTreeStore *gtk_tree_store_newv (gint n_columns, GType *types)
{
  auto *s = (GtkTreeStore *) g_object_new (GTK_TYPE_TREE_STORE, nullptr);
  store_init_types (GTK_TREE_MODEL (s), n_columns, types);
  return s;
}

GtkTreeStore *gtk_tree_store_new (gint n_columns, ...)
{
  QVector<GType> t;
  va_list ap;
  va_start (ap, n_columns);
  for (int i = 0; i < n_columns; i++) t << va_arg (ap, GType);
  va_end (ap);
  return gtk_tree_store_newv (n_columns, t.data ());
}

void gtk_tree_store_set_valist (GtkTreeStore *s, GtkTreeIter *iter, va_list ap) { store_set_valist (GTK_TREE_MODEL (s), iter, ap); }

void gtk_tree_store_set (GtkTreeStore *s, GtkTreeIter *iter, ...)
{
  va_list ap;
  va_start (ap, iter);
  store_set_valist (GTK_TREE_MODEL (s), iter, ap);
  va_end (ap);
}

void gtk_tree_store_set_value (GtkTreeStore *s, GtkTreeIter *iter, gint column, GValue *value)
{
  gtk_list_store_set_value ((GtkListStore *) s, iter, column, value);
}

gboolean gtk_tree_store_remove (GtkTreeStore *s, GtkTreeIter *iter) { return store_remove (GTK_TREE_MODEL (s), iter); }

void gtk_tree_store_insert (GtkTreeStore *s, GtkTreeIter *iter, GtkTreeIter *parent, gint position)
{
  store_insert (GTK_TREE_MODEL (s), parent ? iter_row (GTK_TREE_MODEL (s), parent) : nullptr, position, iter);
}

void gtk_tree_store_prepend (GtkTreeStore *s, GtkTreeIter *iter, GtkTreeIter *parent) { gtk_tree_store_insert (s, iter, parent, 0); }
void gtk_tree_store_append (GtkTreeStore *s, GtkTreeIter *iter, GtkTreeIter *parent) { gtk_tree_store_insert (s, iter, parent, -1); }
void gtk_tree_store_clear (GtkTreeStore *s) { store_clear (GTK_TREE_MODEL (s)); }

/* ---------- GtkTreeModel ---------- */
GtkTreeModelFlags gtk_tree_model_get_flags (GtkTreeModel *m)
{
  return store (m)->is_list ? (GtkTreeModelFlags) (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY) : GTK_TREE_MODEL_ITERS_PERSIST;
}
gint gtk_tree_model_get_n_columns (GtkTreeModel *m) { return store (m)->types.size (); }
GType gtk_tree_model_get_column_type (GtkTreeModel *m, gint index_) { return store (m)->types.value (index_, G_TYPE_INVALID); }

gboolean gtk_tree_model_get_iter (GtkTreeModel *m, GtkTreeIter *iter, GtkTreePath *path)
{
  Row *r = path ? path_row (store (m), path) : nullptr;
  set_iter (m, iter, r);
  return r != nullptr;
}

gboolean gtk_tree_model_get_iter_from_string (GtkTreeModel *m, GtkTreeIter *iter, const gchar *path_string)
{
  GtkTreePath *p = gtk_tree_path_new_from_string (path_string);
  gboolean ok = gtk_tree_model_get_iter (m, iter, p);
  gtk_tree_path_free (p);
  return ok;
}

gboolean gtk_tree_model_get_iter_first (GtkTreeModel *m, GtkTreeIter *iter)
{
  StoreData *d = store (m);
  set_iter (m, iter, d->root.children.isEmpty () ? nullptr : d->root.children.first ());
  return !d->root.children.isEmpty ();
}

GtkTreePath *gtk_tree_model_get_path (GtkTreeModel *m, GtkTreeIter *iter)
{
  Row *r = iter_row (m, iter);
  return r ? row_path (r) : nullptr;
}

gchar *gtk_tree_model_get_string_from_iter (GtkTreeModel *m, GtkTreeIter *iter)
{
  GtkTreePath *p = gtk_tree_model_get_path (m, iter);
  gchar *s = p ? gtk_tree_path_to_string (p) : nullptr;
  gtk_tree_path_free (p);
  return s;
}

void gtk_tree_model_get_value (GtkTreeModel *m, GtkTreeIter *iter, gint column, GValue *value)
{
  StoreData *d = store (m);
  Row *r = iter_row (m, iter);
  g_value_init (value, d->types.value (column, G_TYPE_INT));
  if (r && column >= 0 && column < (int) r->values.size ()) g_value_copy (&r->values[column], value);
}

void gtk_tree_model_get_valist (GtkTreeModel *m, GtkTreeIter *iter, va_list ap)
{
  Row *r = iter_row (m, iter);
  for (int col = va_arg (ap, gint); col != -1; col = va_arg (ap, gint)) {
    if (!r || col < 0 || col >= (int) r->values.size ()) { g_warning ("gtk_tree_model_get: invalid column or iter"); return; }
    gchar *err = nullptr;
    G_VALUE_LCOPY (&r->values[col], ap, 0, &err);
    if (err) { g_warning ("%s", err); g_free (err); return; }
  }
}

void gtk_tree_model_get (GtkTreeModel *m, GtkTreeIter *iter, ...)
{
  va_list ap;
  va_start (ap, iter);
  gtk_tree_model_get_valist (m, iter, ap);
  va_end (ap);
}

gboolean gtk_tree_model_iter_next (GtkTreeModel *m, GtkTreeIter *iter)
{
  Row *r = iter_row (m, iter);
  if (!r) return FALSE;
  int i = r->index () + 1;
  Row *n = i < r->parent->children.size () ? r->parent->children[i] : nullptr;
  set_iter (m, iter, n);
  return n != nullptr;
}

gboolean gtk_tree_model_iter_previous (GtkTreeModel *m, GtkTreeIter *iter)
{
  Row *r = iter_row (m, iter);
  if (!r) return FALSE;
  int i = r->index () - 1;
  Row *n = i >= 0 ? r->parent->children[i] : nullptr;
  set_iter (m, iter, n);
  return n != nullptr;
}

gboolean gtk_tree_model_iter_children (GtkTreeModel *m, GtkTreeIter *iter, GtkTreeIter *parent)
{
  Row *p = parent ? iter_row (m, parent) : &store (m)->root;
  Row *c = p && !p->children.isEmpty () ? p->children.first () : nullptr;
  set_iter (m, iter, c);
  return c != nullptr;
}

gboolean gtk_tree_model_iter_has_child (GtkTreeModel *m, GtkTreeIter *iter)
{
  Row *r = iter_row (m, iter);
  return r && !r->children.isEmpty ();
}

gint gtk_tree_model_iter_n_children (GtkTreeModel *m, GtkTreeIter *iter)
{
  Row *r = iter ? iter_row (m, iter) : &store (m)->root;
  return r ? r->children.size () : 0;
}

gboolean gtk_tree_model_iter_nth_child (GtkTreeModel *m, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
  Row *p = parent ? iter_row (m, parent) : &store (m)->root;
  Row *c = p && n >= 0 && n < p->children.size () ? p->children[n] : nullptr;
  set_iter (m, iter, c);
  return c != nullptr;
}

gboolean gtk_tree_model_iter_parent (GtkTreeModel *m, GtkTreeIter *iter, GtkTreeIter *child)
{
  Row *r = iter_row (m, child);
  Row *p = r && r->parent != &store (m)->root ? r->parent : nullptr;
  set_iter (m, iter, p);
  return p != nullptr;
}

static gboolean foreach_rec (GtkTreeModel *m, Row *r, GtkTreeModelForeachFunc func, gpointer data)
{
  for (Row *c : QList<Row *> (r->children)) {
    GtkTreeIter it;
    set_iter (m, &it, c);
    GtkTreePath *p = row_path (c);
    gboolean stop = func (m, p, &it, data);
    gtk_tree_path_free (p);
    if (stop || foreach_rec (m, c, func, data)) return TRUE;
  }
  return FALSE;
}

void gtk_tree_model_foreach (GtkTreeModel *m, GtkTreeModelForeachFunc func, gpointer data) { foreach_rec (m, &store (m)->root, func, data); }

void gtk_tree_sortable_set_sort_column_id (GtkTreeSortable *sortable, gint sort_column_id, GtkSortType order)
{
  StoreData *d = store (sortable);
  d->sort_column = sort_column_id;
  d->sort_order = order;
  resort ((GtkTreeModel *) sortable);
  g_signal_emit_by_name (sortable, "sort-column-changed");
}

gboolean gtk_tree_sortable_get_sort_column_id (GtkTreeSortable *sortable, gint *sort_column_id, GtkSortType *order)
{
  StoreData *d = store (sortable);
  if (sort_column_id) *sort_column_id = d->sort_column;
  if (order) *order = d->sort_order;
  return d->sort_column >= 0;
}

void gtk_tree_sortable_set_sort_func (GtkTreeSortable *sortable, gint id, GtkTreeIterCompareFunc func, gpointer data, GDestroyNotify destroy)
{
  StoreData *d = store (sortable);
  if (d->sort_funcs.contains (id) && d->sort_funcs[id].destroy) d->sort_funcs[id].destroy (d->sort_funcs[id].data);
  d->sort_funcs[id] = { func, data, destroy };
}

} /* extern "C" */

/* ======================================================================
 * Column API
 * ====================================================================== */
static void column_changed (GtkTreeViewColumn *col);

static void cells_add_attributes (QList<Cell> &cells, GtkCellRenderer *cell, va_list ap)
{
  for (Cell &c : cells) {
    if (c.renderer != cell) continue;
    for (const char *attr = va_arg (ap, const char *); attr; attr = va_arg (ap, const char *))
      c.attrs << qMakePair (QByteArray (attr), va_arg (ap, gint));
  }
}

static void cells_pack (QList<Cell> &cells, GtkCellRenderer *cell)
{
  g_object_ref_sink (cell);
  cells << Cell { cell, {} };
}

extern "C" {

GtkTreeViewColumn *gtk_tree_view_column_new (void) { return (GtkTreeViewColumn *) g_object_new (GTK_TYPE_TREE_VIEW_COLUMN, nullptr); }

GtkTreeViewColumn *gtk_tree_view_column_new_with_attributes (const gchar *title, GtkCellRenderer *cell, ...)
{
  GtkTreeViewColumn *c = gtk_tree_view_column_new ();
  cd (c)->title = gtkqt_str (title);
  cells_pack (cd (c)->cells, cell);
  va_list ap;
  va_start (ap, cell);
  cells_add_attributes (cd (c)->cells, cell, ap);
  va_end (ap);
  return c;
}

void gtk_tree_view_column_pack_start (GtkTreeViewColumn *c, GtkCellRenderer *cell, gboolean) { cells_pack (cd (c)->cells, cell); column_changed (c); }
void gtk_tree_view_column_pack_end (GtkTreeViewColumn *c, GtkCellRenderer *cell, gboolean) { cells_pack (cd (c)->cells, cell); column_changed (c); }

void gtk_tree_view_column_add_attribute (GtkTreeViewColumn *c, GtkCellRenderer *cell, const gchar *attribute, gint column)
{
  for (Cell &x : cd (c)->cells) if (x.renderer == cell) x.attrs << qMakePair (QByteArray (attribute), column);
  column_changed (c);
}

void gtk_tree_view_column_set_attributes (GtkTreeViewColumn *c, GtkCellRenderer *cell, ...)
{
  for (Cell &x : cd (c)->cells) if (x.renderer == cell) x.attrs.clear ();
  va_list ap;
  va_start (ap, cell);
  cells_add_attributes (cd (c)->cells, cell, ap);
  va_end (ap);
  column_changed (c);
}

void gtk_tree_view_column_set_cell_data_func (GtkTreeViewColumn *c, GtkCellRenderer *cell, GtkTreeCellDataFunc func,
                                              gpointer func_data, GDestroyNotify destroy)
{
  for (Cell &x : cd (c)->cells)
    if (x.renderer == cell) {
      if (x.func_destroy) x.func_destroy (x.func_data);
      x.func = func; x.func_data = func_data; x.func_destroy = destroy;
    }
  column_changed (c);
}

void gtk_tree_view_column_set_title (GtkTreeViewColumn *c, const gchar *title) { cd (c)->title = gtkqt_str (title); column_changed (c); }
const gchar *gtk_tree_view_column_get_title (GtkTreeViewColumn *c)
{
  QByteArray *b = new QByteArray (cd (c)->title.toUtf8 ());
  g_object_set_data_full (G_OBJECT (c), "gtkqt-title", b, [] (gpointer p) { delete static_cast<QByteArray *> (p); });
  return b->constData ();
}
void gtk_tree_view_column_set_sort_column_id (GtkTreeViewColumn *c, gint id) { cd (c)->sort_column_id = id; cd (c)->clickable = true; column_changed (c); }
void gtk_tree_view_column_set_resizable (GtkTreeViewColumn *c, gboolean v) { cd (c)->resizable = v; column_changed (c); }
void gtk_tree_view_column_set_expand (GtkTreeViewColumn *c, gboolean v) { cd (c)->expand = v; column_changed (c); }
void gtk_tree_view_column_set_fixed_width (GtkTreeViewColumn *c, gint w) { cd (c)->fixed_width = w; column_changed (c); }
void gtk_tree_view_column_set_min_width (GtkTreeViewColumn *c, gint w) { cd (c)->min_width = w; column_changed (c); }
void gtk_tree_view_column_set_sizing (GtkTreeViewColumn *, GtkTreeViewColumnSizing) {}
void gtk_tree_view_column_set_visible (GtkTreeViewColumn *c, gboolean v) { cd (c)->visible = v; column_changed (c); }
void gtk_tree_view_column_set_clickable (GtkTreeViewColumn *c, gboolean v) { cd (c)->clickable = v; column_changed (c); }
void gtk_tree_view_column_set_reorderable (GtkTreeViewColumn *, gboolean) {}
void gtk_tree_view_column_set_alignment (GtkTreeViewColumn *c, gfloat x) { cd (c)->xalign = x; }

} /* extern "C" */

/* ======================================================================
 * GtkTreeView
 * ====================================================================== */
struct ViewData {
  GtkTreeModel *model = nullptr;
  QPointer<GtkQtItemModel> adapter;
  QList<GtkTreeViewColumn *> columns;
  GtkTreeSelection *selection = nullptr;
  bool headers_clickable = false;
};
static ViewData *vd (GtkWidget *w) { return static_cast<ViewData *> (g_object_get_data (G_OBJECT (w), "gtkqt-view")); }

/* Progress cells need painting */
class GtkQtCellDelegate : public QStyledItemDelegate {
public:
  using QStyledItemDelegate::QStyledItemDelegate;
  void paint (QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &i) const override
  {
    QVariant v = i.data (Qt::UserRole + 1);
    if (!v.isValid ()) { QStyledItemDelegate::paint (p, opt, i); return; }
    QStyleOptionProgressBar bar;
    bar.initFrom (opt.widget);
    bar.state = opt.state | QStyle::State_Horizontal;
    bar.rect = opt.rect.adjusted (2, 2, -2, -2);
    bar.minimum = 0;
    bar.maximum = 100;
    bar.progress = v.toInt ();
    bar.text = i.data (Qt::DisplayRole).toString ();
    bar.textVisible = true;
    QApplication::style ()->drawControl (QStyle::CE_ProgressBar, &bar, p);
  }
};

static void view_apply_columns (GtkWidget *w)
{
  auto *tv = gtkqt_q<QTreeView> (w);
  ViewData *d = vd (w);
  if (!tv || !d) return;
  if (d->adapter) d->adapter->reset ();
  QHeaderView *h = tv->header ();
  bool any_expand = false;
  /* before a model is set the header has no sections to configure */
  int n = qMin ((int) d->columns.size (), h->count ());
  for (int i = 0; i < n; i++) {
    ColumnData *c = cd (d->columns[i]);
    tv->setColumnHidden (i, !c->visible);
    if (c->fixed_width > 0) tv->setColumnWidth (i, c->fixed_width);
    if (c->expand) { h->setSectionResizeMode (i, QHeaderView::Stretch); any_expand = true; }
    else h->setSectionResizeMode (i, c->resizable ? QHeaderView::Interactive : QHeaderView::ResizeToContents);
  }
  h->setStretchLastSection (!any_expand);
  bool clickable = d->headers_clickable;
  for (GtkTreeViewColumn *c : d->columns) clickable = clickable || cd (c)->clickable;
  h->setSectionsClickable (clickable);
  h->setSortIndicatorShown (false);
  if (d->model) {
    gint col;
    GtkSortType order;
    if (gtk_tree_sortable_get_sort_column_id ((GtkTreeSortable *) d->model, &col, &order))
      for (int i = 0; i < n; i++)
        if (cd (d->columns[i])->sort_column_id == col) {
          h->setSortIndicatorShown (true);
          h->setSortIndicator (i, order == GTK_SORT_ASCENDING ? Qt::AscendingOrder : Qt::DescendingOrder);
        }
  }
  for (int i = 0; i < n; i++)
    if (cd (d->columns[i])->min_width > 0 && tv->columnWidth (i) < cd (d->columns[i])->min_width)
      tv->setColumnWidth (i, cd (d->columns[i])->min_width);
}

static void column_changed (GtkTreeViewColumn *col)
{
  if (cd (col)->view) view_apply_columns (cd (col)->view);
}

static QObject *tree_view_create (GtkWidget *w)
{
  auto *tv = new QTreeView;
  tv->setItemDelegate (new GtkQtCellDelegate (tv));
  tv->setUniformRowHeights (true);
  tv->setAllColumnsShowFocus (true);
  tv->setEditTriggers (QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
  g_object_set_data_full (G_OBJECT (w), "gtkqt-view", new ViewData, [] (gpointer p) { delete static_cast<ViewData *> (p); });
  gtkqt_connect (w, tv, &QTreeView::activated, [] (GtkWidget *vw, const QModelIndex &i) {
    GtkTreePath *p = row_path (static_cast<Row *> (i.internalPointer ()));
    ViewData *d = vd (vw);
    g_signal_emit_by_name (vw, "row-activated", p, i.column () < d->columns.size () ? d->columns[i.column ()] : nullptr);
    gtk_tree_path_free (p);
  });
  gtkqt_connect (w, tv->header (), &QHeaderView::sectionClicked, [] (GtkWidget *vw, int section) {
    ViewData *d = vd (vw);
    if (section >= d->columns.size ()) return;
    GtkTreeViewColumn *c = d->columns[section];
    g_signal_emit_by_name (c, "clicked");
    int id = cd (c)->sort_column_id;
    if (id >= 0 && d->model) {
      gint cur; GtkSortType order;
      gtk_tree_sortable_get_sort_column_id ((GtkTreeSortable *) d->model, &cur, &order);
      order = cur == id && order == GTK_SORT_ASCENDING ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
      gtk_tree_sortable_set_sort_column_id ((GtkTreeSortable *) d->model, id, order);
      view_apply_columns (vw);
    }
  });
  return tv;
}

static void tree_view_dispose (GObject *o);

G_DEFINE_TYPE (GtkTreeView, gtk_tree_view, GTK_TYPE_CONTAINER)
static void gtk_tree_view_init (GtkTreeView *) {}
static void gtk_tree_view_class_init (GtkTreeViewClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = tree_view_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), tree_view_create);
  g_signal_new ("row-activated", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2,
                G_TYPE_POINTER, GTK_TYPE_TREE_VIEW_COLUMN);
  g_signal_new ("cursor-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_object ("model", nullptr, nullptr, gtk_tree_model_get_type (), RW),
              [] (GObject *o, const GValue *v) { gtk_tree_view_set_model (GTK_TREE_VIEW (o), (GtkTreeModel *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_tree_view_get_model (GTK_TREE_VIEW (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("headers-visible", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("reorderable", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_tree_view_set_reorderable (GTK_TREE_VIEW (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("enable-search", nullptr, nullptr, TRUE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("search-column", nullptr, nullptr, -1, G_MAXINT, -1, RW), [] (GObject *, const GValue *) {}, nullptr);
}

/* GtkTreeSelection */
struct _GtkTreeSelection { GObject parent_instance; GtkTreeView *view; };
typedef struct { GObjectClass parent_class; } GtkTreeSelectionClass;
G_DEFINE_TYPE (GtkTreeSelection, gtk_tree_selection, G_TYPE_OBJECT)
static void gtk_tree_selection_init (GtkTreeSelection *) {}
static void gtk_tree_selection_class_init (GtkTreeSelectionClass *klass)
{
  g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_enum ("mode", nullptr, nullptr, gtk_selection_mode_get_type (), GTK_SELECTION_SINGLE, RW),
              [] (GObject *o, const GValue *v) { gtk_tree_selection_set_mode ((GtkTreeSelection *) o, (GtkSelectionMode) g_value_get_enum (v)); }, nullptr);
}

static void tree_view_dispose (GObject *o)
{
  GtkWidget *w = GTK_WIDGET (o);
  if (ViewData *d = vd (w)) {
    if (auto *tv = gtkqt_q<QTreeView> (w)) tv->setModel (nullptr);
    if (d->adapter) { d->adapter->detach (); delete d->adapter.data (); }
    if (d->model) { g_object_unref (d->model); d->model = nullptr; }
    for (GtkTreeViewColumn *c : d->columns) { cd (c)->view = nullptr; g_object_unref (c); }
    d->columns.clear ();
    if (d->selection) { d->selection->view = nullptr; g_object_unref (d->selection); d->selection = nullptr; }
  }
  G_OBJECT_CLASS (gtk_tree_view_parent_class)->dispose (o);
}

static QItemSelectionModel *sel_model (GtkTreeSelection *s)
{
  auto *tv = s->view ? gtkqt_q<QTreeView> (GTK_WIDGET (s->view)) : nullptr;
  return tv ? tv->selectionModel () : nullptr;
}

static void connect_selection (GtkWidget *w)
{
  auto *tv = gtkqt_q<QTreeView> (w);
  ViewData *d = vd (w);
  if (!tv || !tv->selectionModel () || !d->selection) return;
  gtkqt_connect (w, tv->selectionModel (), &QItemSelectionModel::selectionChanged,
                 [] (GtkWidget *vw, const QItemSelection &, const QItemSelection &) { g_signal_emit_by_name (vd (vw)->selection, "changed"); });
  gtkqt_connect (w, tv->selectionModel (), &QItemSelectionModel::currentChanged,
                 [] (GtkWidget *vw, const QModelIndex &, const QModelIndex &) { g_signal_emit_by_name (vw, "cursor-changed"); });
}

extern "C" {

GtkWidget *gtk_tree_view_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_TREE_VIEW, nullptr); }

GtkWidget *gtk_tree_view_new_with_model (GtkTreeModel *model)
{
  GtkWidget *w = gtk_tree_view_new ();
  gtk_tree_view_set_model (GTK_TREE_VIEW (w), model);
  return w;
}

void gtk_tree_view_set_model (GtkTreeView *view, GtkTreeModel *model)
{
  GtkWidget *w = GTK_WIDGET (view);
  auto *tv = gtkqt_q<QTreeView> (w);
  ViewData *d = vd (w);
  if (!tv || !d || d->model == model) return;
  tv->setModel (nullptr);
  if (d->adapter) delete d->adapter.data ();
  if (d->model) g_object_unref (d->model);
  d->model = model;
  if (!model) return;
  g_object_ref (model);
  auto *a = new GtkQtItemModel (model);
  a->setParent (tv);
  a->columns = [w] { QList<GObject *> l; if (ViewData *vdd = vd (w)) for (auto *c : vdd->columns) l << G_OBJECT (c); return l; };
  a->cells_of = [] (GObject *c) -> QList<Cell> * { return &cd (c)->cells; };
  d->adapter = a;
  tv->setModel (a);
  tv->setRootIsDecorated (!store (model)->is_list);
  gtk_tree_view_get_selection (view);
  connect_selection (w);
  view_apply_columns (w);
}

GtkTreeModel *gtk_tree_view_get_model (GtkTreeView *view) { ViewData *d = vd (GTK_WIDGET (view)); return d ? d->model : nullptr; }

gint gtk_tree_view_insert_column (GtkTreeView *view, GtkTreeViewColumn *column, gint position)
{
  ViewData *d = vd (GTK_WIDGET (view));
  if (!d) return 0;
  g_object_ref_sink (column);
  cd (column)->view = GTK_WIDGET (view);
  if (position < 0 || position > d->columns.size ()) position = d->columns.size ();
  d->columns.insert (position, column);
  view_apply_columns (GTK_WIDGET (view));
  return d->columns.size ();
}

gint gtk_tree_view_append_column (GtkTreeView *view, GtkTreeViewColumn *column) { return gtk_tree_view_insert_column (view, column, -1); }

gint gtk_tree_view_insert_column_with_attributes (GtkTreeView *view, gint position, const gchar *title, GtkCellRenderer *cell, ...)
{
  GtkTreeViewColumn *c = gtk_tree_view_column_new ();
  cd (c)->title = gtkqt_str (title);
  cells_pack (cd (c)->cells, cell);
  va_list ap;
  va_start (ap, cell);
  cells_add_attributes (cd (c)->cells, cell, ap);
  va_end (ap);
  return gtk_tree_view_insert_column (view, c, position);
}

gint gtk_tree_view_remove_column (GtkTreeView *view, GtkTreeViewColumn *column)
{
  ViewData *d = vd (GTK_WIDGET (view));
  if (d && d->columns.removeOne (column)) {
    cd (column)->view = nullptr;
    view_apply_columns (GTK_WIDGET (view));
    g_object_unref (column);
  }
  return d ? d->columns.size () : 0;
}

GtkTreeViewColumn *gtk_tree_view_get_column (GtkTreeView *view, gint n)
{
  ViewData *d = vd (GTK_WIDGET (view));
  return d && n >= 0 && n < d->columns.size () ? d->columns[n] : nullptr;
}

guint gtk_tree_view_get_n_columns (GtkTreeView *view) { ViewData *d = vd (GTK_WIDGET (view)); return d ? d->columns.size () : 0; }

GtkTreeSelection *gtk_tree_view_get_selection (GtkTreeView *view)
{
  ViewData *d = vd (GTK_WIDGET (view));
  if (!d) return nullptr;
  if (!d->selection) {
    d->selection = (GtkTreeSelection *) g_object_new (gtk_tree_selection_get_type (), nullptr);
    d->selection->view = view;
    connect_selection (GTK_WIDGET (view));
  }
  return d->selection;
}

void gtk_tree_view_set_headers_visible (GtkTreeView *view, gboolean v) { if (auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view))) tv->setHeaderHidden (!v); }
void gtk_tree_view_set_headers_clickable (GtkTreeView *view, gboolean v) { if (ViewData *d = vd (GTK_WIDGET (view))) { d->headers_clickable = v; view_apply_columns (GTK_WIDGET (view)); } }
void gtk_tree_view_set_rules_hint (GtkTreeView *view, gboolean v) { if (auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view))) tv->setAlternatingRowColors (v); }
void gtk_tree_view_set_reorderable (GtkTreeView *, gboolean) {}
void gtk_tree_view_set_enable_search (GtkTreeView *, gboolean) {}
void gtk_tree_view_set_search_column (GtkTreeView *, gint) {}
void gtk_tree_view_expand_all (GtkTreeView *view) { if (auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view))) tv->expandAll (); }
void gtk_tree_view_collapse_all (GtkTreeView *view) { if (auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view))) tv->collapseAll (); }

static QModelIndex view_index (GtkTreeView *view, GtkTreePath *path, int col = 0)
{
  ViewData *d = vd (GTK_WIDGET (view));
  if (!d || !d->adapter || !path) return QModelIndex ();
  return d->adapter->index_of (path_row (store (d->model), path), col);
}

gboolean gtk_tree_view_expand_row (GtkTreeView *view, GtkTreePath *path, gboolean open_all)
{
  auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view));
  QModelIndex i = view_index (view, path);
  if (!tv || !i.isValid ()) return FALSE;
  if (open_all) tv->expandRecursively (i); else tv->expand (i);
  return TRUE;
}

void gtk_tree_view_set_cursor (GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *focus_column, gboolean start_editing)
{
  auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view));
  ViewData *d = vd (GTK_WIDGET (view));
  int col = focus_column && d ? qMax (0, (int) d->columns.indexOf (focus_column)) : 0;
  QModelIndex i = view_index (view, path, col);
  if (!tv || !i.isValid ()) return;
  tv->setCurrentIndex (i);
  if (start_editing) tv->edit (i);
}

void gtk_tree_view_get_cursor (GtkTreeView *view, GtkTreePath **path, GtkTreeViewColumn **focus_column)
{
  auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view));
  QModelIndex i = tv ? tv->currentIndex () : QModelIndex ();
  if (path) *path = i.isValid () ? row_path (static_cast<Row *> (i.internalPointer ())) : nullptr;
  if (focus_column) *focus_column = i.isValid () ? gtk_tree_view_get_column (view, i.column ()) : nullptr;
}

void gtk_tree_view_scroll_to_cell (GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *, gboolean use_align, gfloat row_align, gfloat)
{
  auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view));
  QModelIndex i = view_index (view, path);
  if (tv && i.isValid ())
    tv->scrollTo (i, !use_align ? QAbstractItemView::EnsureVisible : row_align < 0.25 ? QAbstractItemView::PositionAtTop
                     : row_align > 0.75 ? QAbstractItemView::PositionAtBottom : QAbstractItemView::PositionAtCenter);
}

void gtk_tree_view_row_activated (GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column)
{
  g_signal_emit_by_name (view, "row-activated", path, column);
}

void gtk_tree_view_columns_autosize (GtkTreeView *view)
{
  if (auto *tv = gtkqt_q<QTreeView> (GTK_WIDGET (view)))
    for (int i = 0; i < tv->header ()->count (); i++) tv->resizeColumnToContents (i);
}

/* ---------- GtkTreeSelection ---------- */
void gtk_tree_selection_set_mode (GtkTreeSelection *s, GtkSelectionMode type)
{
  auto *tv = s->view ? gtkqt_q<QTreeView> (GTK_WIDGET (s->view)) : nullptr;
  if (!tv) return;
  static const QAbstractItemView::SelectionMode map[] = { QAbstractItemView::NoSelection, QAbstractItemView::SingleSelection,
                                                          QAbstractItemView::SingleSelection, QAbstractItemView::ExtendedSelection };
  tv->setSelectionMode (map[type & 3]);
  g_object_set_data (G_OBJECT (s), "gtkqt-mode", GINT_TO_POINTER (type));
}

GtkSelectionMode gtk_tree_selection_get_mode (GtkTreeSelection *s)
{
  gpointer p = g_object_get_data (G_OBJECT (s), "gtkqt-mode");
  return p ? (GtkSelectionMode) GPOINTER_TO_INT (p) : GTK_SELECTION_SINGLE;
}

static QList<Row *> selected_rows (GtkTreeSelection *s)
{
  QList<Row *> rows;
  if (QItemSelectionModel *sm = sel_model (s))
    for (const QModelIndex &i : sm->selectedRows ()) rows << static_cast<Row *> (i.internalPointer ());
  return rows;
}

gboolean gtk_tree_selection_get_selected (GtkTreeSelection *s, GtkTreeModel **model, GtkTreeIter *iter)
{
  GtkTreeModel *m = s->view ? gtk_tree_view_get_model (s->view) : nullptr;
  if (model) *model = m;
  QList<Row *> rows = selected_rows (s);
  if (!m || rows.isEmpty ()) { if (iter) memset (iter, 0, sizeof (*iter)); return FALSE; }
  set_iter (m, iter, rows.first ());
  return TRUE;
}

GList *gtk_tree_selection_get_selected_rows (GtkTreeSelection *s, GtkTreeModel **model)
{
  if (model) *model = s->view ? gtk_tree_view_get_model (s->view) : nullptr;
  GList *l = nullptr;
  for (Row *r : selected_rows (s)) l = g_list_append (l, row_path (r));
  return l;
}

gint gtk_tree_selection_count_selected_rows (GtkTreeSelection *s) { return selected_rows (s).size (); }

void gtk_tree_selection_selected_foreach (GtkTreeSelection *s, GtkTreeSelectionForeachFunc func, gpointer data)
{
  GtkTreeModel *m = s->view ? gtk_tree_view_get_model (s->view) : nullptr;
  if (!m) return;
  for (Row *r : selected_rows (s)) {
    GtkTreeIter it;
    set_iter (m, &it, r);
    GtkTreePath *p = row_path (r);
    func (m, p, &it, data);
    gtk_tree_path_free (p);
  }
}

static void select_row (GtkTreeSelection *s, Row *r, QItemSelectionModel::SelectionFlags f)
{
  ViewData *d = s->view ? vd (GTK_WIDGET (s->view)) : nullptr;
  QItemSelectionModel *sm = sel_model (s);
  if (!d || !d->adapter || !sm || !r) return;
  if (gtk_tree_selection_get_mode (s) != GTK_SELECTION_MULTIPLE && (f & QItemSelectionModel::Select)) f |= QItemSelectionModel::Clear;
  sm->select (d->adapter->index_of (r), f | QItemSelectionModel::Rows);
  if (f & QItemSelectionModel::Select) sm->setCurrentIndex (d->adapter->index_of (r), QItemSelectionModel::NoUpdate);
}

void gtk_tree_selection_select_iter (GtkTreeSelection *s, GtkTreeIter *iter)
{
  if (GtkTreeModel *m = s->view ? gtk_tree_view_get_model (s->view) : nullptr) select_row (s, iter_row (m, iter), QItemSelectionModel::Select);
}

void gtk_tree_selection_unselect_iter (GtkTreeSelection *s, GtkTreeIter *iter)
{
  if (GtkTreeModel *m = s->view ? gtk_tree_view_get_model (s->view) : nullptr) select_row (s, iter_row (m, iter), QItemSelectionModel::Deselect);
}

void gtk_tree_selection_select_path (GtkTreeSelection *s, GtkTreePath *path)
{
  if (GtkTreeModel *m = s->view ? gtk_tree_view_get_model (s->view) : nullptr) select_row (s, path_row (store (m), path), QItemSelectionModel::Select);
}

void gtk_tree_selection_select_all (GtkTreeSelection *s)
{
  if (auto *tv = s->view ? gtkqt_q<QTreeView> (GTK_WIDGET (s->view)) : nullptr) tv->selectAll ();
}

void gtk_tree_selection_unselect_all (GtkTreeSelection *s) { if (QItemSelectionModel *sm = sel_model (s)) sm->clearSelection (); }

gboolean gtk_tree_selection_iter_is_selected (GtkTreeSelection *s, GtkTreeIter *iter)
{
  GtkTreeModel *m = s->view ? gtk_tree_view_get_model (s->view) : nullptr;
  return m && selected_rows (s).contains (iter_row (m, iter));
}

GtkTreeView *gtk_tree_selection_get_tree_view (GtkTreeSelection *s) { return s->view; }

} /* extern "C" */

/* ======================================================================
 * GtkCellLayout + combo boxes with a model
 * ====================================================================== */
static QList<Cell> *layout_cells (GtkCellLayout *l)
{
  if (GTK_IS_TREE_VIEW_COLUMN (l)) return &cd (l)->cells;
  auto *cells = static_cast<QList<Cell> *> (g_object_get_data (G_OBJECT (l), "gtkqt-cells"));
  if (!cells) {
    cells = new QList<Cell>;
    g_object_set_data_full (G_OBJECT (l), "gtkqt-cells", cells, [] (gpointer p) {
      auto *c = static_cast<QList<Cell> *> (p);
      cells_clear (*c);
      delete c;
    });
  }
  return cells;
}

static void combo_refresh (GtkComboBox *combo)
{
  if (auto *c = gtkqt_q<QComboBox> (GTK_WIDGET (combo))) if (auto *a = dynamic_cast<GtkQtItemModel *> (c->model ())) a->reset ();
}

extern "C" {

void gtk_cell_layout_pack_start (GtkCellLayout *l, GtkCellRenderer *cell, gboolean)
{
  cells_pack (*layout_cells (l), cell);
  if (GTK_IS_COMBO_BOX (l)) combo_refresh (GTK_COMBO_BOX (l));
}

void gtk_cell_layout_pack_end (GtkCellLayout *l, GtkCellRenderer *cell, gboolean expand) { gtk_cell_layout_pack_start (l, cell, expand); }

void gtk_cell_layout_add_attribute (GtkCellLayout *l, GtkCellRenderer *cell, const gchar *attribute, gint column)
{
  for (Cell &c : *layout_cells (l)) if (c.renderer == cell) c.attrs << qMakePair (QByteArray (attribute), column);
  if (GTK_IS_COMBO_BOX (l)) combo_refresh (GTK_COMBO_BOX (l));
}

void gtk_cell_layout_set_attributes (GtkCellLayout *l, GtkCellRenderer *cell, ...)
{
  QList<Cell> *cells = layout_cells (l);
  for (Cell &c : *cells) if (c.renderer == cell) c.attrs.clear ();
  va_list ap;
  va_start (ap, cell);
  cells_add_attributes (*cells, cell, ap);
  va_end (ap);
  if (GTK_IS_COMBO_BOX (l)) combo_refresh (GTK_COMBO_BOX (l));
}

void gtk_cell_layout_clear (GtkCellLayout *l) { cells_clear (*layout_cells (l)); }

GtkWidget *gtk_combo_box_new_with_model (GtkTreeModel *model)
{
  GtkWidget *w = gtk_combo_box_new ();
  gtk_combo_box_set_model (GTK_COMBO_BOX (w), model);
  return w;
}

void gtk_combo_box_set_model (GtkComboBox *combo, GtkTreeModel *model)
{
  auto *c = gtkqt_q<QComboBox> (GTK_WIDGET (combo));
  if (!c || !model) return;
  g_object_ref (model);
  g_object_set_data_full (G_OBJECT (combo), "gtkqt-tree-model", model, g_object_unref);
  auto *a = new GtkQtItemModel (model);
  a->setParent (c);
  GObject *self = G_OBJECT (combo);
  a->columns = [self] { return QList<GObject *> { self }; };
  a->cells_of = [] (GObject *o) -> QList<Cell> * { return layout_cells ((GtkCellLayout *) o); };
  QSignalBlocker block (c);
  c->setModel (a);
  c->setCurrentIndex (-1);
}

GtkTreeModel *gtk_combo_box_get_model (GtkComboBox *combo) { return (GtkTreeModel *) g_object_get_data (G_OBJECT (combo), "gtkqt-tree-model"); }

gboolean gtk_combo_box_get_active_iter (GtkComboBox *combo, GtkTreeIter *iter)
{
  auto *c = gtkqt_q<QComboBox> (GTK_WIDGET (combo));
  GtkTreeModel *m = gtk_combo_box_get_model (combo);
  if (!c || !m || c->currentIndex () < 0) return FALSE;
  return gtk_tree_model_iter_nth_child (m, iter, nullptr, c->currentIndex ());
}

void gtk_combo_box_set_active_iter (GtkComboBox *combo, GtkTreeIter *iter)
{
  GtkTreeModel *m = gtk_combo_box_get_model (combo);
  Row *r = m ? iter_row (m, iter) : nullptr;
  gtk_combo_box_set_active (combo, r ? r->index () : -1);
}

void gtk_combo_box_set_entry_text_column (GtkComboBox *combo, gint text_column)
{
  QList<Cell> *cells = layout_cells ((GtkCellLayout *) combo);
  if (cells->isEmpty ()) {
    GtkCellRenderer *r = gtk_cell_renderer_text_new ();
    cells_pack (*cells, r);
    (*cells)[0].attrs << qMakePair (QByteArray ("text"), text_column);
  }
  combo_refresh (combo);
}

void gtk_combo_box_set_id_column (GtkComboBox *, gint) {}

} /* extern "C" */

/* ======================================================================
 * GtkEntryCompletion
 *
 * GTK's matching rules (minimum key length, case-insensitive prefix or the
 * program's match function) pick rows of any GtkTreeModel; the matches are
 * shown by a QCompleter popup in unfiltered mode, so Qt only displays and
 * navigates them. Choosing one emits match-selected, whose default handler
 * puts the row's text in the entry, so programs can take over as in GTK.
 * ====================================================================== */
struct _GtkEntryCompletion {
  GObject parent;
  GtkTreeModel *model;
  GtkWidget *entry;                      /* weak */
  gint text_column, minimum_key_length;
  gboolean inline_completion, inline_selection, popup_completion, popup_set_width, popup_single_match;
  GtkEntryCompletionMatchFunc match_func;
  gpointer match_data;
  GDestroyNotify match_notify;
  gchar *prefix;                         /* the key of the last completion */
  QPointer<QCompleter> *completer;
  QStringList *texts;                    /* the matches shown, and their rows */
  QStringList *paths;
};
typedef struct { GObjectClass parent_class; } GtkEntryCompletionClass;

G_DEFINE_TYPE (GtkEntryCompletion, gtk_entry_completion, G_TYPE_OBJECT)

static void completion_init_fields (GtkEntryCompletion *c)
{
  c->text_column = -1;
  c->minimum_key_length = 1;
  c->popup_completion = c->popup_set_width = c->popup_single_match = TRUE;
  c->completer = new QPointer<QCompleter>;
  c->texts = new QStringList;
  c->paths = new QStringList;
}
static void gtk_entry_completion_init (GtkEntryCompletion *c) { completion_init_fields (c); }

static void completion_detach (GtkEntryCompletion *c)
{
  if (*c->completer) {
    QCompleter *q = *c->completer;
    if (QAbstractItemView *p = q->popup ()) p->hide ();
    q->deleteLater ();
  }
  *c->completer = nullptr;
  if (c->entry) g_object_remove_weak_pointer (G_OBJECT (c->entry), (gpointer *) &c->entry);
  c->entry = nullptr;
}

static void completion_finalize (GObject *o)
{
  GtkEntryCompletion *c = GTK_ENTRY_COMPLETION (o);
  completion_detach (c);
  if (c->model) g_object_unref (c->model);
  if (c->match_notify) c->match_notify (c->match_data);
  g_free (c->prefix);
  delete c->completer;
  delete c->texts;
  delete c->paths;
  G_OBJECT_CLASS (gtk_entry_completion_parent_class)->finalize (o);
}

static gchar *completion_row_text (GtkEntryCompletion *c, GtkTreeIter *it)
{
  if (c->text_column < 0) return nullptr;
  gchar *s = nullptr;
  GValue v = G_VALUE_INIT;
  gtk_tree_model_get_value (c->model, it, c->text_column, &v);
  if (G_VALUE_HOLDS_STRING (&v)) s = g_value_dup_string (&v);
  g_value_unset (&v);
  return s;
}

static gboolean completion_default_match (GtkEntryCompletion *c, const gchar *key, GtkTreeIter *it)
{
  gchar *text = completion_row_text (c, it);
  if (!text) return FALSE;
  QString t = QString::fromUtf8 (text).normalized (QString::NormalizationForm_D);
  g_free (text);
  return t.startsWith (QString::fromUtf8 (key).normalized (QString::NormalizationForm_D), Qt::CaseInsensitive);
}

/* fills texts/paths with the rows that match key */
static void completion_collect (GtkEntryCompletion *c, const gchar *key)
{
  c->texts->clear ();
  c->paths->clear ();
  if (!c->model) return;
  GtkTreeIter it;
  if (!gtk_tree_model_get_iter_first (c->model, &it)) return;
  do {
    gboolean ok = c->match_func ? c->match_func (c, key, &it, c->match_data) : completion_default_match (c, key, &it);
    if (!ok) continue;
    gchar *text = completion_row_text (c, &it);
    gchar *path = gtk_tree_model_get_string_from_iter (c->model, &it);
    *c->texts << QString::fromUtf8 (text ? text : "");
    *c->paths << QString::fromUtf8 (path);
    g_free (text);
    g_free (path);
  } while (gtk_tree_model_iter_next (c->model, &it));
}

static QLineEdit *completion_line (GtkEntryCompletion *c)
{
  return c->entry ? gtkqt_q<QLineEdit> (c->entry) : nullptr;
}

static void completion_choose (GtkEntryCompletion *c, int row)
{
  if (!c->model || row < 0 || row >= c->paths->size ()) return;
  GtkTreeIter it;
  if (!gtk_tree_model_get_iter_from_string (c->model, &it, c->paths->at (row).toUtf8 ().constData ())) return;
  gboolean handled = FALSE;
  g_signal_emit_by_name (c, "match-selected", c->model, &it, &handled);
}

static void completion_run (GtkEntryCompletion *c)
{
  QLineEdit *line = completion_line (c);
  if (!line) return;
  QByteArray key = line->text ().toUtf8 ();
  if (g_utf8_strlen (key.constData (), -1) < c->minimum_key_length) {
    if (*c->completer && (*c->completer)->popup ()) (*c->completer)->popup ()->hide ();
    return;
  }
  g_free (c->prefix);
  c->prefix = g_strdup (key.constData ());
  completion_collect (c, key.constData ());
  if (c->texts->isEmpty ()) {
    g_signal_emit_by_name (c, "no-matches");
    if (*c->completer && (*c->completer)->popup ()) (*c->completer)->popup ()->hide ();
    return;
  }
  if (c->inline_completion) gtk_entry_completion_insert_prefix (c);
  if (!c->popup_completion) return;
  if (c->texts->size () == 1 && !c->popup_single_match) return;
  if (!*c->completer) {
    QCompleter *q = new QCompleter (line);
    q->setWidget (line);
    q->setCompletionMode (QCompleter::UnfilteredPopupCompletion);
    q->setModel (new QStringListModel (q));
    QObject::connect (q, QOverload<const QModelIndex &>::of (&QCompleter::activated), q, [c] (const QModelIndex &i) {
      GtkQtEmitGuard guard;
      completion_choose (c, i.row ());
    });
    *c->completer = q;
  }
  QCompleter *q = *c->completer;
  static_cast<QStringListModel *> (q->model ())->setStringList (*c->texts);
  q->complete (c->popup_set_width ? QRect () : QRect (0, 0, q->popup ()->sizeHintForColumn (0) + 24, line->height ()));
}

static gboolean completion_real_match_selected (GtkEntryCompletion *c, GtkTreeModel *, GtkTreeIter *it)
{
  if (!c->entry) return FALSE;
  gchar *text = completion_row_text (c, it);
  if (!text) return FALSE;
  gtk_entry_set_text (GTK_ENTRY (c->entry), text);
  gtk_editable_set_position (GTK_EDITABLE (c->entry), -1);
  g_free (text);
  return TRUE;
}

/* inserts the part of prefix past what was typed, selected, as GTK does */
static gboolean completion_real_insert_prefix (GtkEntryCompletion *c, const gchar *prefix)
{
  QLineEdit *line = completion_line (c);
  if (!line || !prefix) return FALSE;
  QString typed = line->text (), full = QString::fromUtf8 (prefix);
  if (full.size () <= typed.size () || !full.startsWith (typed, Qt::CaseInsensitive)) return FALSE;
  int pos = typed.size ();
  QByteArray rest = full.mid (pos).toUtf8 ();
  gint p = pos;
  gtk_editable_insert_text (GTK_EDITABLE (c->entry), rest.constData (), -1, &p);
  gtk_editable_select_region (GTK_EDITABLE (c->entry), pos, p);
  return TRUE;
}

static void gtk_entry_completion_class_init (GtkEntryCompletionClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = completion_finalize;
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new_class_handler ("match-selected", t, G_SIGNAL_RUN_LAST, G_CALLBACK (completion_real_match_selected),
                              g_signal_accumulator_true_handled, nullptr, nullptr,
                              G_TYPE_BOOLEAN, 2, GTK_TYPE_TREE_MODEL, G_TYPE_POINTER);
  g_signal_new_class_handler ("insert-prefix", t, G_SIGNAL_RUN_LAST, G_CALLBACK (completion_real_insert_prefix),
                              g_signal_accumulator_true_handled, nullptr, nullptr, G_TYPE_BOOLEAN, 1, G_TYPE_STRING);
  g_signal_new ("cursor-on-match", t, G_SIGNAL_RUN_LAST, 0, g_signal_accumulator_true_handled, nullptr, nullptr,
                G_TYPE_BOOLEAN, 2, GTK_TYPE_TREE_MODEL, G_TYPE_POINTER);
  g_signal_new ("action-activated", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_INT);
  g_signal_new ("no-matches", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_object ("model", nullptr, nullptr, GTK_TYPE_TREE_MODEL, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_completion_set_model (GTK_ENTRY_COMPLETION (o), (GtkTreeModel *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, GTK_ENTRY_COMPLETION (o)->model); });
  gtkqt_prop (klass, g_param_spec_int ("text-column", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_completion_set_text_column (GTK_ENTRY_COMPLETION (o), g_value_get_int (v)); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, GTK_ENTRY_COMPLETION (o)->text_column); });
  gtkqt_prop (klass, g_param_spec_int ("minimum-key-length", nullptr, nullptr, 0, G_MAXINT, 1, RW),
              [] (GObject *o, const GValue *v) { GTK_ENTRY_COMPLETION (o)->minimum_key_length = g_value_get_int (v); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, GTK_ENTRY_COMPLETION (o)->minimum_key_length); });
  gtkqt_prop (klass, g_param_spec_boolean ("inline-completion", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { GTK_ENTRY_COMPLETION (o)->inline_completion = g_value_get_boolean (v); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, GTK_ENTRY_COMPLETION (o)->inline_completion); });
  gtkqt_prop (klass, g_param_spec_boolean ("popup-completion", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { GTK_ENTRY_COMPLETION (o)->popup_completion = g_value_get_boolean (v); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, GTK_ENTRY_COMPLETION (o)->popup_completion); });
}

extern "C" {

GtkEntryCompletion *gtk_entry_completion_new (void) { return (GtkEntryCompletion *) g_object_new (GTK_TYPE_ENTRY_COMPLETION, nullptr); }
GtkWidget *gtk_entry_completion_get_entry (GtkEntryCompletion *c) { return c->entry; }

void gtk_entry_completion_set_model (GtkEntryCompletion *c, GtkTreeModel *model)
{
  if (model) g_object_ref (model);
  if (c->model) g_object_unref (c->model);
  c->model = model;
  /* GTK picks the first string column once a model is there */
  if (model && c->text_column < 0)
    for (gint i = 0; i < gtk_tree_model_get_n_columns (model); i++)
      if (gtk_tree_model_get_column_type (model, i) == G_TYPE_STRING) { c->text_column = i; break; }
}
GtkTreeModel *gtk_entry_completion_get_model (GtkEntryCompletion *c) { return c->model; }

void gtk_entry_completion_set_match_func (GtkEntryCompletion *c, GtkEntryCompletionMatchFunc func, gpointer data, GDestroyNotify notify)
{
  if (c->match_notify) c->match_notify (c->match_data);
  c->match_func = func;
  c->match_data = data;
  c->match_notify = notify;
}
void gtk_entry_completion_set_minimum_key_length (GtkEntryCompletion *c, gint length) { c->minimum_key_length = MAX (0, length); }
gint gtk_entry_completion_get_minimum_key_length (GtkEntryCompletion *c) { return c->minimum_key_length; }
void gtk_entry_completion_complete (GtkEntryCompletion *c) { completion_run (c); }

void gtk_entry_completion_insert_prefix (GtkEntryCompletion *c)
{
  gchar *prefix = gtk_entry_completion_compute_prefix (c, c->prefix ? c->prefix : "");
  gboolean handled = FALSE;
  if (prefix) g_signal_emit_by_name (c, "insert-prefix", prefix, &handled);
  g_free (prefix);
}

/* the longest common prefix of the matches for key */
gchar *gtk_entry_completion_compute_prefix (GtkEntryCompletion *c, const char *key)
{
  completion_collect (c, key ? key : "");
  if (c->texts->isEmpty ()) return nullptr;
  QString p = c->texts->first ();
  for (const QString &t : *c->texts) {
    int n = 0;
    while (n < p.size () && n < t.size () && p[n] == t[n]) n++;
    p.truncate (n);
  }
  return g_strdup (p.toUtf8 ().constData ());
}

void gtk_entry_completion_set_text_column (GtkEntryCompletion *c, gint column) { c->text_column = column; }
gint gtk_entry_completion_get_text_column (GtkEntryCompletion *c) { return c->text_column; }
void gtk_entry_completion_set_inline_completion (GtkEntryCompletion *c, gboolean v) { c->inline_completion = v; }
gboolean gtk_entry_completion_get_inline_completion (GtkEntryCompletion *c) { return c->inline_completion; }
void gtk_entry_completion_set_inline_selection (GtkEntryCompletion *c, gboolean v) { c->inline_selection = v; }
gboolean gtk_entry_completion_get_inline_selection (GtkEntryCompletion *c) { return c->inline_selection; }
void gtk_entry_completion_set_popup_completion (GtkEntryCompletion *c, gboolean v) { c->popup_completion = v; }
gboolean gtk_entry_completion_get_popup_completion (GtkEntryCompletion *c) { return c->popup_completion; }
void gtk_entry_completion_set_popup_set_width (GtkEntryCompletion *c, gboolean v) { c->popup_set_width = v; }
gboolean gtk_entry_completion_get_popup_set_width (GtkEntryCompletion *c) { return c->popup_set_width; }
void gtk_entry_completion_set_popup_single_match (GtkEntryCompletion *c, gboolean v) { c->popup_single_match = v; }
gboolean gtk_entry_completion_get_popup_single_match (GtkEntryCompletion *c) { return c->popup_single_match; }
const gchar *gtk_entry_completion_get_completion_prefix (GtkEntryCompletion *c) { return c->prefix; }

static void entry_completion_unset (gpointer data)
{
  GtkEntryCompletion *c = (GtkEntryCompletion *) data;
  completion_detach (c);
  g_object_unref (c);
}

void gtk_entry_set_completion (GtkEntry *entry, GtkEntryCompletion *completion)
{
  if (!GTK_IS_ENTRY (entry)) return;
  /* drops (and detaches) the previous one */
  g_object_set_data_full (G_OBJECT (entry), "gtkqt-completion", nullptr, nullptr);
  if (!completion) return;
  g_object_ref (completion);
  completion_detach (completion);
  completion->entry = GTK_WIDGET (entry);
  g_object_add_weak_pointer (G_OBJECT (entry), (gpointer *) &completion->entry);
  g_object_set_data_full (G_OBJECT (entry), "gtkqt-completion", completion, entry_completion_unset);
  /* typing (not set_text from the program) runs the completion */
  QLineEdit *line = gtkqt_q<QLineEdit> (GTK_WIDGET (entry));
  if (line && !line->property ("_gtkqt_completion").toBool ()) {
    line->setProperty ("_gtkqt_completion", true);
    QObject::connect (line, &QLineEdit::textEdited, line, [entry] (const QString &) {
      auto *c = (GtkEntryCompletion *) g_object_get_data (G_OBJECT (entry), "gtkqt-completion");
      if (!c) return;
      GtkQtEmitGuard guard;
      completion_run (c);
    });
  }
}

GtkEntryCompletion *gtk_entry_get_completion (GtkEntry *entry)
{
  return GTK_IS_ENTRY (entry) ? (GtkEntryCompletion *) g_object_get_data (G_OBJECT (entry), "gtkqt-completion") : nullptr;
}

} /* extern "C" */
