/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkTreeModel / GtkListStore / GtkTreeStore / GtkTreeView.
 *
 * Stores hold GValues in GTK fashion; GtkTreeView renders them through a
 * QTreeView + QAbstractItemModel adapter. Text, numeric, toggle, pixbuf and
 * progress cell renderers are supported with the usual attribute mapping.
 */
#ifndef GTKQT_TREE_H
#define GTKQT_TREE_H

#include <gtk/gtkqt-containers.h>

G_BEGIN_DECLS

typedef struct _GtkTreeModel GtkTreeModel;      /* "interface"; stores are the only models */
typedef struct _GtkTreePath GtkTreePath;
typedef struct _GtkTreeRowReference GtkTreeRowReference;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeStore GtkTreeStore;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkTreeViewColumn GtkTreeViewColumn;
typedef struct _GtkCellRenderer GtkCellRenderer;

typedef struct _GtkTreeIter {
  gint stamp;
  gpointer user_data;
  gpointer user_data2;
  gpointer user_data3;
} GtkTreeIter;

typedef gboolean (*GtkTreeModelForeachFunc) (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);
typedef void (*GtkTreeSelectionForeachFunc) (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);
typedef void (*GtkTreeCellDataFunc) (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model,
                                     GtkTreeIter *iter, gpointer data);
typedef gint (*GtkTreeIterCompareFunc) (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);

#define GTK_TYPE_TREE_MODEL   (gtk_tree_model_get_type ())
#define GTK_TREE_MODEL(o)     ((GtkTreeModel *) (o))
#define GTK_IS_TREE_MODEL(o)  (GTK_IS_LIST_STORE (o) || GTK_IS_TREE_STORE (o))
#define GTK_TYPE_LIST_STORE   (gtk_list_store_get_type ())
#define GTK_LIST_STORE(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_LIST_STORE, GtkListStore))
#define GTK_IS_LIST_STORE(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_LIST_STORE))
#define GTK_TYPE_TREE_STORE   (gtk_tree_store_get_type ())
#define GTK_TREE_STORE(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TREE_STORE, GtkTreeStore))
#define GTK_IS_TREE_STORE(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TREE_STORE))
#define GTK_TYPE_TREE_ITER    (gtk_tree_iter_get_type ())
#define GTK_TYPE_TREE_PATH    (gtk_tree_path_get_type ())
#define GTK_TREE_SORTABLE(o)  ((GtkTreeSortable *) (o))
typedef struct _GtkTreeSortable GtkTreeSortable;
GType gtk_tree_model_get_type (void) G_GNUC_CONST;
GType gtk_list_store_get_type (void) G_GNUC_CONST;
GType gtk_tree_store_get_type (void) G_GNUC_CONST;
GType gtk_tree_iter_get_type (void) G_GNUC_CONST;
GType gtk_tree_path_get_type (void) G_GNUC_CONST;

/* GtkTreePath */
GtkTreePath *gtk_tree_path_new (void);
GtkTreePath *gtk_tree_path_new_first (void);
GtkTreePath *gtk_tree_path_new_from_string (const gchar *path);
GtkTreePath *gtk_tree_path_new_from_indices (gint first_index, ...);
GtkTreePath *gtk_tree_path_copy (const GtkTreePath *path);
void         gtk_tree_path_free (GtkTreePath *path);
gchar       *gtk_tree_path_to_string (GtkTreePath *path);
void         gtk_tree_path_append_index (GtkTreePath *path, gint index_);
void         gtk_tree_path_prepend_index (GtkTreePath *path, gint index_);
gint         gtk_tree_path_get_depth (GtkTreePath *path);
gint        *gtk_tree_path_get_indices (GtkTreePath *path);
gint        *gtk_tree_path_get_indices_with_depth (GtkTreePath *path, gint *depth);
gint         gtk_tree_path_compare (const GtkTreePath *a, const GtkTreePath *b);
void         gtk_tree_path_next (GtkTreePath *path);
gboolean     gtk_tree_path_prev (GtkTreePath *path);
gboolean     gtk_tree_path_up (GtkTreePath *path);
void         gtk_tree_path_down (GtkTreePath *path);
gboolean     gtk_tree_path_is_ancestor (GtkTreePath *path, GtkTreePath *descendant);
gboolean     gtk_tree_path_is_descendant (GtkTreePath *path, GtkTreePath *ancestor);

GtkTreeIter *gtk_tree_iter_copy (GtkTreeIter *iter);
void         gtk_tree_iter_free (GtkTreeIter *iter);

/* GtkTreeModel */
GtkTreeModelFlags gtk_tree_model_get_flags (GtkTreeModel *tree_model);
gint         gtk_tree_model_get_n_columns (GtkTreeModel *tree_model);
GType        gtk_tree_model_get_column_type (GtkTreeModel *tree_model, gint index_);
gboolean     gtk_tree_model_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path);
gboolean     gtk_tree_model_get_iter_from_string (GtkTreeModel *tree_model, GtkTreeIter *iter, const gchar *path_string);
gboolean     gtk_tree_model_get_iter_first (GtkTreeModel *tree_model, GtkTreeIter *iter);
GtkTreePath *gtk_tree_model_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter);
gchar       *gtk_tree_model_get_string_from_iter (GtkTreeModel *tree_model, GtkTreeIter *iter);
void         gtk_tree_model_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value);
void         gtk_tree_model_get (GtkTreeModel *tree_model, GtkTreeIter *iter, ...);
void         gtk_tree_model_get_valist (GtkTreeModel *tree_model, GtkTreeIter *iter, va_list var_args);
gboolean     gtk_tree_model_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter);
gboolean     gtk_tree_model_iter_previous (GtkTreeModel *tree_model, GtkTreeIter *iter);
gboolean     gtk_tree_model_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent);
gboolean     gtk_tree_model_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter);
gint         gtk_tree_model_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter);
gboolean     gtk_tree_model_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n);
gboolean     gtk_tree_model_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child);
void         gtk_tree_model_foreach (GtkTreeModel *model, GtkTreeModelForeachFunc func, gpointer user_data);

/* GtkListStore / GtkTreeStore */
GtkListStore *gtk_list_store_new (gint n_columns, ...);
GtkListStore *gtk_list_store_newv (gint n_columns, GType *types);
void          gtk_list_store_set (GtkListStore *list_store, GtkTreeIter *iter, ...);
void          gtk_list_store_set_valist (GtkListStore *list_store, GtkTreeIter *iter, va_list var_args);
void          gtk_list_store_set_value (GtkListStore *list_store, GtkTreeIter *iter, gint column, GValue *value);
gboolean      gtk_list_store_remove (GtkListStore *list_store, GtkTreeIter *iter);
void          gtk_list_store_insert (GtkListStore *list_store, GtkTreeIter *iter, gint position);
void          gtk_list_store_insert_with_values (GtkListStore *list_store, GtkTreeIter *iter, gint position, ...);
void          gtk_list_store_prepend (GtkListStore *list_store, GtkTreeIter *iter);
void          gtk_list_store_append (GtkListStore *list_store, GtkTreeIter *iter);
void          gtk_list_store_clear (GtkListStore *list_store);
gboolean      gtk_list_store_iter_is_valid (GtkListStore *list_store, GtkTreeIter *iter);
void          gtk_list_store_swap (GtkListStore *store, GtkTreeIter *a, GtkTreeIter *b);
void          gtk_list_store_insert_before (GtkListStore *list_store, GtkTreeIter *iter, GtkTreeIter *sibling);
void          gtk_list_store_insert_after (GtkListStore *list_store, GtkTreeIter *iter, GtkTreeIter *sibling);
void          gtk_list_store_reorder (GtkListStore *store, gint *new_order);
void          gtk_list_store_move_before (GtkListStore *store, GtkTreeIter *iter, GtkTreeIter *position);
void          gtk_list_store_move_after (GtkListStore *store, GtkTreeIter *iter, GtkTreeIter *position);

GtkTreeStore *gtk_tree_store_new (gint n_columns, ...);
GtkTreeStore *gtk_tree_store_newv (gint n_columns, GType *types);
void          gtk_tree_store_set (GtkTreeStore *tree_store, GtkTreeIter *iter, ...);
void          gtk_tree_store_set_valist (GtkTreeStore *tree_store, GtkTreeIter *iter, va_list var_args);
void          gtk_tree_store_set_value (GtkTreeStore *tree_store, GtkTreeIter *iter, gint column, GValue *value);
gboolean      gtk_tree_store_remove (GtkTreeStore *tree_store, GtkTreeIter *iter);
void          gtk_tree_store_insert (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *parent, gint position);
void          gtk_tree_store_prepend (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *parent);
void          gtk_tree_store_append (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *parent);
void          gtk_tree_store_clear (GtkTreeStore *tree_store);
void          gtk_tree_store_insert_before (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *parent, GtkTreeIter *sibling);
void          gtk_tree_store_insert_after (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *parent, GtkTreeIter *sibling);
void          gtk_tree_store_insert_with_values (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *parent, gint position, ...);
gboolean      gtk_tree_store_is_ancestor (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *descendant);
gint          gtk_tree_store_iter_depth (GtkTreeStore *tree_store, GtkTreeIter *iter);
gboolean      gtk_tree_store_iter_is_valid (GtkTreeStore *tree_store, GtkTreeIter *iter);
void          gtk_tree_store_reorder (GtkTreeStore *tree_store, GtkTreeIter *parent, gint *new_order);
void          gtk_tree_store_swap (GtkTreeStore *tree_store, GtkTreeIter *a, GtkTreeIter *b);
void          gtk_tree_store_move_before (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *position);
void          gtk_tree_store_move_after (GtkTreeStore *tree_store, GtkTreeIter *iter, GtkTreeIter *position);

/* Sorting */
#define GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID (-1)
#define GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID (-2)
void     gtk_tree_sortable_set_sort_column_id (GtkTreeSortable *sortable, gint sort_column_id, GtkSortType order);
gboolean gtk_tree_sortable_get_sort_column_id (GtkTreeSortable *sortable, gint *sort_column_id, GtkSortType *order);
void     gtk_tree_sortable_sort_column_changed (GtkTreeSortable *sortable);
void     gtk_tree_sortable_set_sort_func (GtkTreeSortable *sortable, gint sort_column_id, GtkTreeIterCompareFunc sort_func,
                                          gpointer user_data, GDestroyNotify destroy);

/* GtkCellRenderer */
struct _GtkCellRenderer { GInitiallyUnowned parent_instance; gpointer priv; };
typedef struct _GtkCellRendererClass { GInitiallyUnownedClass parent_class; } GtkCellRendererClass;
typedef GtkCellRenderer GtkCellRendererText;
typedef GtkCellRenderer GtkCellRendererToggle;
typedef GtkCellRenderer GtkCellRendererPixbuf;
typedef GtkCellRenderer GtkCellRendererProgress;
typedef GtkCellRenderer GtkCellRendererSpin;
typedef GtkCellRenderer GtkCellRendererCombo;
#define GTK_TYPE_CELL_RENDERER          (gtk_cell_renderer_get_type ())
#define GTK_CELL_RENDERER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_CELL_RENDERER, GtkCellRenderer))
#define GTK_IS_CELL_RENDERER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_CELL_RENDERER))
#define GTK_TYPE_CELL_RENDERER_TEXT     (gtk_cell_renderer_text_get_type ())
#define GTK_CELL_RENDERER_TEXT(o)       ((GtkCellRendererText *) (o))
#define GTK_TYPE_CELL_RENDERER_TOGGLE   (gtk_cell_renderer_toggle_get_type ())
#define GTK_CELL_RENDERER_TOGGLE(o)     ((GtkCellRendererToggle *) (o))
#define GTK_TYPE_CELL_RENDERER_PIXBUF   (gtk_cell_renderer_pixbuf_get_type ())
#define GTK_TYPE_CELL_RENDERER_PROGRESS (gtk_cell_renderer_progress_get_type ())
#define GTK_TYPE_CELL_RENDERER_SPIN     (gtk_cell_renderer_spin_get_type ())
#define GTK_TYPE_CELL_RENDERER_COMBO    (gtk_cell_renderer_combo_get_type ())
GType            gtk_cell_renderer_get_type (void) G_GNUC_CONST;
GType            gtk_cell_renderer_text_get_type (void) G_GNUC_CONST;
GType            gtk_cell_renderer_toggle_get_type (void) G_GNUC_CONST;
GType            gtk_cell_renderer_pixbuf_get_type (void) G_GNUC_CONST;
GType            gtk_cell_renderer_progress_get_type (void) G_GNUC_CONST;
GType            gtk_cell_renderer_spin_get_type (void) G_GNUC_CONST;
GType            gtk_cell_renderer_combo_get_type (void) G_GNUC_CONST;
GtkCellRenderer *gtk_cell_renderer_text_new (void);
GtkCellRenderer *gtk_cell_renderer_toggle_new (void);
GtkCellRenderer *gtk_cell_renderer_pixbuf_new (void);
GtkCellRenderer *gtk_cell_renderer_progress_new (void);
GtkCellRenderer *gtk_cell_renderer_spin_new (void);
GtkCellRenderer *gtk_cell_renderer_combo_new (void);
void             gtk_cell_renderer_toggle_set_radio (GtkCellRendererToggle *toggle, gboolean radio);
void             gtk_cell_renderer_toggle_set_active (GtkCellRendererToggle *toggle, gboolean setting);
void             gtk_cell_renderer_set_alignment (GtkCellRenderer *cell, gfloat xalign, gfloat yalign);
void             gtk_cell_renderer_set_padding (GtkCellRenderer *cell, gint xpad, gint ypad);
void             gtk_cell_renderer_set_visible (GtkCellRenderer *cell, gboolean visible);

/* GtkCellLayout (combo boxes with a model) */
typedef struct _GtkCellLayout GtkCellLayout;
#define GTK_CELL_LAYOUT(o) ((GtkCellLayout *) (o))
void gtk_cell_layout_pack_start (GtkCellLayout *cell_layout, GtkCellRenderer *cell, gboolean expand);
void gtk_cell_layout_pack_end (GtkCellLayout *cell_layout, GtkCellRenderer *cell, gboolean expand);
void gtk_cell_layout_add_attribute (GtkCellLayout *cell_layout, GtkCellRenderer *cell, const gchar *attribute, gint column);
void gtk_cell_layout_set_attributes (GtkCellLayout *cell_layout, GtkCellRenderer *cell, ...) G_GNUC_NULL_TERMINATED;
void gtk_cell_layout_clear (GtkCellLayout *cell_layout);
GtkWidget    *gtk_combo_box_new_with_model (GtkTreeModel *model);
void          gtk_combo_box_set_model (GtkComboBox *combo_box, GtkTreeModel *model);
GtkTreeModel *gtk_combo_box_get_model (GtkComboBox *combo_box);
gboolean      gtk_combo_box_get_active_iter (GtkComboBox *combo_box, GtkTreeIter *iter);
void          gtk_combo_box_set_active_iter (GtkComboBox *combo_box, GtkTreeIter *iter);
void          gtk_combo_box_set_entry_text_column (GtkComboBox *combo_box, gint text_column);
void          gtk_combo_box_set_id_column (GtkComboBox *combo_box, gint id_column);

/* GtkTreeViewColumn: GTK 2's public fields (programs read ->button,
 * ->sort_column_id ...); the header button emits "clicked" and
 * "size-allocate" as the column's header section does */
struct _GtkTreeViewColumn {
  GInitiallyUnowned parent_instance;
  GtkWidget *tree_view;
  GtkWidget *button;
  GtkWidget *child;
  GtkWidget *arrow;
  GtkWidget *alignment;
  GdkWindow *window;
  gpointer editable_widget;
  gfloat xalign;
  guint property_changed_signal;
  gint spacing;
  GtkTreeViewColumnSizing column_type;
  gint requested_width;
  gint button_request;
  gint resized_width;
  gint width;
  gint fixed_width;
  gint min_width;
  gint max_width;
  gint drag_x;
  gint drag_y;
  gchar *title;
  GList *cell_list;
  guint sort_clicked_signal;
  guint sort_column_changed_signal;
  gint sort_column_id;
  GtkSortType sort_order;
  guint visible : 1;
  guint resizable : 1;
  guint clickable : 1;
  guint dirty : 1;
  guint show_sort_indicator : 1;
  guint maybe_reordered : 1;
  guint reorderable : 1;
  guint use_resized_width : 1;
  guint expand : 1;
  gpointer gtkqt_priv;
};
typedef enum {
  GTK_TREE_VIEW_DROP_BEFORE,
  GTK_TREE_VIEW_DROP_AFTER,
  GTK_TREE_VIEW_DROP_INTO_OR_BEFORE,
  GTK_TREE_VIEW_DROP_INTO_OR_AFTER
} GtkTreeViewDropPosition;
typedef enum {
  GTK_TREE_VIEW_GRID_LINES_NONE,
  GTK_TREE_VIEW_GRID_LINES_HORIZONTAL,
  GTK_TREE_VIEW_GRID_LINES_VERTICAL,
  GTK_TREE_VIEW_GRID_LINES_BOTH
} GtkTreeViewGridLines;
typedef struct _GtkTreeView GtkTreeView;
typedef gboolean (*GtkTreeViewColumnDropFunc) (GtkTreeView *tree_view, GtkTreeViewColumn *column, GtkTreeViewColumn *prev_column,
                                               GtkTreeViewColumn *next_column, gpointer data);
typedef gboolean (*GtkTreeSelectionFunc) (GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path,
                                          gboolean path_currently_selected, gpointer data);

#define GTK_TYPE_TREE_VIEW_COLUMN (gtk_tree_view_column_get_type ())
#define GTK_TREE_VIEW_COLUMN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TREE_VIEW_COLUMN, GtkTreeViewColumn))
#define GTK_IS_TREE_VIEW_COLUMN(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TREE_VIEW_COLUMN))
GType              gtk_tree_view_column_get_type (void) G_GNUC_CONST;
GtkTreeViewColumn *gtk_tree_view_column_new (void);
GtkTreeViewColumn *gtk_tree_view_column_new_with_attributes (const gchar *title, GtkCellRenderer *cell, ...) G_GNUC_NULL_TERMINATED;
void               gtk_tree_view_column_pack_start (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, gboolean expand);
void               gtk_tree_view_column_pack_end (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, gboolean expand);
void               gtk_tree_view_column_add_attribute (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell_renderer,
                                                       const gchar *attribute, gint column);
void               gtk_tree_view_column_set_attributes (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell_renderer, ...) G_GNUC_NULL_TERMINATED;
void               gtk_tree_view_column_set_cell_data_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell_renderer,
                                                            GtkTreeCellDataFunc func, gpointer func_data, GDestroyNotify destroy);
void               gtk_tree_view_column_set_title (GtkTreeViewColumn *tree_column, const gchar *title);
const gchar       *gtk_tree_view_column_get_title (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_set_sort_column_id (GtkTreeViewColumn *tree_column, gint sort_column_id);
void               gtk_tree_view_column_set_resizable (GtkTreeViewColumn *tree_column, gboolean resizable);
void               gtk_tree_view_column_set_expand (GtkTreeViewColumn *tree_column, gboolean expand);
void               gtk_tree_view_column_set_fixed_width (GtkTreeViewColumn *tree_column, gint fixed_width);
void               gtk_tree_view_column_set_min_width (GtkTreeViewColumn *tree_column, gint min_width);
void               gtk_tree_view_column_set_sizing (GtkTreeViewColumn *tree_column, GtkTreeViewColumnSizing type);
void               gtk_tree_view_column_set_visible (GtkTreeViewColumn *tree_column, gboolean visible);
void               gtk_tree_view_column_set_clickable (GtkTreeViewColumn *tree_column, gboolean clickable);
void               gtk_tree_view_column_set_reorderable (GtkTreeViewColumn *tree_column, gboolean reorderable);
void               gtk_tree_view_column_set_alignment (GtkTreeViewColumn *tree_column, gfloat xalign);
gfloat             gtk_tree_view_column_get_alignment (GtkTreeViewColumn *tree_column);
gboolean           gtk_tree_view_column_get_visible (GtkTreeViewColumn *tree_column);
gboolean           gtk_tree_view_column_get_resizable (GtkTreeViewColumn *tree_column);
gboolean           gtk_tree_view_column_get_clickable (GtkTreeViewColumn *tree_column);
gboolean           gtk_tree_view_column_get_expand (GtkTreeViewColumn *tree_column);
gint               gtk_tree_view_column_get_sort_column_id (GtkTreeViewColumn *tree_column);
gint               gtk_tree_view_column_get_width (GtkTreeViewColumn *tree_column);
gint               gtk_tree_view_column_get_fixed_width (GtkTreeViewColumn *tree_column);
gint               gtk_tree_view_column_get_min_width (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_set_max_width (GtkTreeViewColumn *tree_column, gint max_width);
gint               gtk_tree_view_column_get_max_width (GtkTreeViewColumn *tree_column);
GtkTreeViewColumnSizing gtk_tree_view_column_get_sizing (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_set_spacing (GtkTreeViewColumn *tree_column, gint spacing);
gint               gtk_tree_view_column_get_spacing (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_set_sort_indicator (GtkTreeViewColumn *tree_column, gboolean setting);
gboolean           gtk_tree_view_column_get_sort_indicator (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_set_sort_order (GtkTreeViewColumn *tree_column, GtkSortType order);
GtkSortType        gtk_tree_view_column_get_sort_order (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_set_widget (GtkTreeViewColumn *tree_column, GtkWidget *widget);
GtkWidget         *gtk_tree_view_column_get_widget (GtkTreeViewColumn *tree_column);
GtkWidget         *gtk_tree_view_column_get_button (GtkTreeViewColumn *tree_column);
GtkWidget         *gtk_tree_view_column_get_tree_view (GtkTreeViewColumn *tree_column);
gboolean           gtk_tree_view_column_get_reorderable (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_clear (GtkTreeViewColumn *tree_column);
GList             *gtk_tree_view_column_get_cell_renderers (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_clicked (GtkTreeViewColumn *tree_column);
void               gtk_tree_view_column_queue_resize (GtkTreeViewColumn *tree_column);

/* GtkTreeView */
GTKQT_SIMPLE_TYPE (GtkTreeView, GtkContainer)
#define GTK_TYPE_TREE_VIEW  (gtk_tree_view_get_type ())
#define GTK_TREE_VIEW(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TREE_VIEW, GtkTreeView))
#define GTK_IS_TREE_VIEW(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TREE_VIEW))
GType              gtk_tree_view_get_type (void) G_GNUC_CONST;
GtkWidget         *gtk_tree_view_new (void);
GtkWidget         *gtk_tree_view_new_with_model (GtkTreeModel *model);
void               gtk_tree_view_set_model (GtkTreeView *tree_view, GtkTreeModel *model);
GtkTreeModel      *gtk_tree_view_get_model (GtkTreeView *tree_view);
gint               gtk_tree_view_append_column (GtkTreeView *tree_view, GtkTreeViewColumn *column);
gint               gtk_tree_view_insert_column (GtkTreeView *tree_view, GtkTreeViewColumn *column, gint position);
gint               gtk_tree_view_insert_column_with_attributes (GtkTreeView *tree_view, gint position, const gchar *title,
                                                                GtkCellRenderer *cell, ...) G_GNUC_NULL_TERMINATED;
gint               gtk_tree_view_remove_column (GtkTreeView *tree_view, GtkTreeViewColumn *column);
GtkTreeViewColumn *gtk_tree_view_get_column (GtkTreeView *tree_view, gint n);
guint              gtk_tree_view_get_n_columns (GtkTreeView *tree_view);
GtkTreeSelection  *gtk_tree_view_get_selection (GtkTreeView *tree_view);
void               gtk_tree_view_set_headers_visible (GtkTreeView *tree_view, gboolean headers_visible);
void               gtk_tree_view_set_headers_clickable (GtkTreeView *tree_view, gboolean setting);
void               gtk_tree_view_set_rules_hint (GtkTreeView *tree_view, gboolean setting);
void               gtk_tree_view_set_reorderable (GtkTreeView *tree_view, gboolean reorderable);
void               gtk_tree_view_set_enable_search (GtkTreeView *tree_view, gboolean enable_search);
void               gtk_tree_view_set_search_column (GtkTreeView *tree_view, gint column);
void               gtk_tree_view_expand_all (GtkTreeView *tree_view);
void               gtk_tree_view_collapse_all (GtkTreeView *tree_view);
gboolean           gtk_tree_view_expand_row (GtkTreeView *tree_view, GtkTreePath *path, gboolean open_all);
void               gtk_tree_view_set_cursor (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *focus_column, gboolean start_editing);
void               gtk_tree_view_get_cursor (GtkTreeView *tree_view, GtkTreePath **path, GtkTreeViewColumn **focus_column);
void               gtk_tree_view_scroll_to_cell (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column,
                                                 gboolean use_align, gfloat row_align, gfloat col_align);
void               gtk_tree_view_row_activated (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column);
void               gtk_tree_view_columns_autosize (GtkTreeView *tree_view);
GList             *gtk_tree_view_get_columns (GtkTreeView *tree_view);
void               gtk_tree_view_move_column_after (GtkTreeView *tree_view, GtkTreeViewColumn *column, GtkTreeViewColumn *base_column);
void               gtk_tree_view_set_column_drag_function (GtkTreeView *tree_view, GtkTreeViewColumnDropFunc func,
                                                           gpointer user_data, GDestroyNotify destroy);
void               gtk_tree_view_set_expander_column (GtkTreeView *tree_view, GtkTreeViewColumn *column);
GtkTreeViewColumn *gtk_tree_view_get_expander_column (GtkTreeView *tree_view);
void               gtk_tree_view_collapse_row (GtkTreeView *tree_view, GtkTreePath *path);
gboolean           gtk_tree_view_row_expanded (GtkTreeView *tree_view, GtkTreePath *path);
void               gtk_tree_view_expand_to_path (GtkTreeView *tree_view, GtkTreePath *path);
gboolean           gtk_tree_view_get_path_at_pos (GtkTreeView *tree_view, gint x, gint y, GtkTreePath **path,
                                                  GtkTreeViewColumn **column, gint *cell_x, gint *cell_y);
void               gtk_tree_view_get_cell_area (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, GdkRectangle *rect);
void               gtk_tree_view_get_background_area (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, GdkRectangle *rect);
void               gtk_tree_view_get_visible_rect (GtkTreeView *tree_view, GdkRectangle *visible_rect);
gboolean           gtk_tree_view_get_visible_range (GtkTreeView *tree_view, GtkTreePath **start_path, GtkTreePath **end_path);
void               gtk_tree_view_widget_to_tree_coords (GtkTreeView *tree_view, gint wx, gint wy, gint *tx, gint *ty);
void               gtk_tree_view_tree_to_widget_coords (GtkTreeView *tree_view, gint tx, gint ty, gint *wx, gint *wy);
void               gtk_tree_view_convert_widget_to_bin_window_coords (GtkTreeView *tree_view, gint wx, gint wy, gint *bx, gint *by);
void               gtk_tree_view_convert_bin_window_to_widget_coords (GtkTreeView *tree_view, gint bx, gint by, gint *wx, gint *wy);
void               gtk_tree_view_convert_bin_window_to_tree_coords (GtkTreeView *tree_view, gint bx, gint by, gint *tx, gint *ty);
void               gtk_tree_view_convert_tree_to_bin_window_coords (GtkTreeView *tree_view, gint tx, gint ty, gint *bx, gint *by);
void               gtk_tree_view_scroll_to_point (GtkTreeView *tree_view, gint tree_x, gint tree_y);
GdkWindow         *gtk_tree_view_get_bin_window (GtkTreeView *tree_view);
GtkAdjustment     *gtk_tree_view_get_hadjustment (GtkTreeView *tree_view);
GtkAdjustment     *gtk_tree_view_get_vadjustment (GtkTreeView *tree_view);
void               gtk_tree_view_set_grid_lines (GtkTreeView *tree_view, GtkTreeViewGridLines grid_lines);
GtkTreeViewGridLines gtk_tree_view_get_grid_lines (GtkTreeView *tree_view);
gboolean           gtk_tree_view_get_headers_visible (GtkTreeView *tree_view);
gboolean           gtk_tree_view_get_rules_hint (GtkTreeView *tree_view);
gboolean           gtk_tree_view_get_reorderable (GtkTreeView *tree_view);
void               gtk_tree_view_set_fixed_height_mode (GtkTreeView *tree_view, gboolean enable);
void               gtk_tree_view_set_enable_tree_lines (GtkTreeView *tree_view, gboolean enabled);
void               gtk_tree_view_set_show_expanders (GtkTreeView *tree_view, gboolean enabled);
void               gtk_tree_view_set_level_indentation (GtkTreeView *tree_view, gint indentation);
void               gtk_tree_view_set_hover_selection (GtkTreeView *tree_view, gboolean hover);
void               gtk_tree_view_set_rubber_banding (GtkTreeView *tree_view, gboolean enable);
/* drag and drop between rows: the drop position is drawn as GTK does */
typedef struct _GtkTargetEntry GtkTargetEntry;
void               gtk_tree_view_enable_model_drag_source (GtkTreeView *tree_view, GdkModifierType start_button_mask,
                                                           const GtkTargetEntry *targets, gint n_targets, GdkDragAction actions);
void               gtk_tree_view_enable_model_drag_dest (GtkTreeView *tree_view, const GtkTargetEntry *targets, gint n_targets,
                                                         GdkDragAction actions);
void               gtk_tree_view_unset_rows_drag_source (GtkTreeView *tree_view);
void               gtk_tree_view_unset_rows_drag_dest (GtkTreeView *tree_view);
void               gtk_tree_view_set_drag_dest_row (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewDropPosition pos);
void               gtk_tree_view_get_drag_dest_row (GtkTreeView *tree_view, GtkTreePath **path, GtkTreeViewDropPosition *pos);
gboolean           gtk_tree_view_get_dest_row_at_pos (GtkTreeView *tree_view, gint drag_x, gint drag_y, GtkTreePath **path,
                                                      GtkTreeViewDropPosition *pos);

/* GtkTreeSelection */
#define GTK_TYPE_TREE_SELECTION (gtk_tree_selection_get_type ())
#define GTK_TREE_SELECTION(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TREE_SELECTION, GtkTreeSelection))
#define GTK_IS_TREE_SELECTION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TREE_SELECTION))
GType            gtk_tree_selection_get_type (void) G_GNUC_CONST;
void             gtk_tree_selection_set_mode (GtkTreeSelection *selection, GtkSelectionMode type);
GtkSelectionMode gtk_tree_selection_get_mode (GtkTreeSelection *selection);
gboolean         gtk_tree_selection_get_selected (GtkTreeSelection *selection, GtkTreeModel **model, GtkTreeIter *iter);
GList           *gtk_tree_selection_get_selected_rows (GtkTreeSelection *selection, GtkTreeModel **model);
gint             gtk_tree_selection_count_selected_rows (GtkTreeSelection *selection);
void             gtk_tree_selection_selected_foreach (GtkTreeSelection *selection, GtkTreeSelectionForeachFunc func, gpointer data);
void             gtk_tree_selection_select_iter (GtkTreeSelection *selection, GtkTreeIter *iter);
void             gtk_tree_selection_unselect_iter (GtkTreeSelection *selection, GtkTreeIter *iter);
void             gtk_tree_selection_select_path (GtkTreeSelection *selection, GtkTreePath *path);
void             gtk_tree_selection_select_all (GtkTreeSelection *selection);
void             gtk_tree_selection_unselect_all (GtkTreeSelection *selection);
gboolean         gtk_tree_selection_iter_is_selected (GtkTreeSelection *selection, GtkTreeIter *iter);
GtkTreeView     *gtk_tree_selection_get_tree_view (GtkTreeSelection *selection);
void             gtk_tree_selection_set_select_function (GtkTreeSelection *selection, GtkTreeSelectionFunc func,
                                                         gpointer data, GDestroyNotify destroy);
gboolean         gtk_tree_selection_path_is_selected (GtkTreeSelection *selection, GtkTreePath *path);
void             gtk_tree_selection_unselect_path (GtkTreeSelection *selection, GtkTreePath *path);
void             gtk_tree_selection_select_range (GtkTreeSelection *selection, GtkTreePath *start_path, GtkTreePath *end_path);
void             gtk_tree_selection_unselect_range (GtkTreeSelection *selection, GtkTreePath *start_path, GtkTreePath *end_path);

/* GtkTreeRowReference: follows its row through inserts, deletes and sorting */
GtkTreeRowReference *gtk_tree_row_reference_new (GtkTreeModel *model, GtkTreePath *path);
GtkTreeRowReference *gtk_tree_row_reference_copy (GtkTreeRowReference *reference);
void                 gtk_tree_row_reference_free (GtkTreeRowReference *reference);
GtkTreePath         *gtk_tree_row_reference_get_path (GtkTreeRowReference *reference);
GtkTreeModel        *gtk_tree_row_reference_get_model (GtkTreeRowReference *reference);
gboolean             gtk_tree_row_reference_valid (GtkTreeRowReference *reference);

/* GtkEntryCompletion: the matches pop up in a QCompleter under the entry */
typedef struct _GtkEntryCompletion GtkEntryCompletion;
typedef gboolean (*GtkEntryCompletionMatchFunc) (GtkEntryCompletion *completion, const gchar *key,
                                                 GtkTreeIter *iter, gpointer user_data);
#define GTK_TYPE_ENTRY_COMPLETION   (gtk_entry_completion_get_type ())
#define GTK_ENTRY_COMPLETION(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ENTRY_COMPLETION, GtkEntryCompletion))
#define GTK_IS_ENTRY_COMPLETION(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ENTRY_COMPLETION))
GType               gtk_entry_completion_get_type (void) G_GNUC_CONST;
GtkEntryCompletion *gtk_entry_completion_new (void);
GtkWidget          *gtk_entry_completion_get_entry (GtkEntryCompletion *completion);
void                gtk_entry_completion_set_model (GtkEntryCompletion *completion, GtkTreeModel *model);
GtkTreeModel       *gtk_entry_completion_get_model (GtkEntryCompletion *completion);
void                gtk_entry_completion_set_match_func (GtkEntryCompletion *completion, GtkEntryCompletionMatchFunc func,
                                                         gpointer func_data, GDestroyNotify func_notify);
void                gtk_entry_completion_set_minimum_key_length (GtkEntryCompletion *completion, gint length);
gint                gtk_entry_completion_get_minimum_key_length (GtkEntryCompletion *completion);
void                gtk_entry_completion_complete (GtkEntryCompletion *completion);
void                gtk_entry_completion_insert_prefix (GtkEntryCompletion *completion);
void                gtk_entry_completion_set_text_column (GtkEntryCompletion *completion, gint column);
gint                gtk_entry_completion_get_text_column (GtkEntryCompletion *completion);
void                gtk_entry_completion_set_inline_completion (GtkEntryCompletion *completion, gboolean inline_completion);
gboolean            gtk_entry_completion_get_inline_completion (GtkEntryCompletion *completion);
void                gtk_entry_completion_set_inline_selection (GtkEntryCompletion *completion, gboolean inline_selection);
gboolean            gtk_entry_completion_get_inline_selection (GtkEntryCompletion *completion);
void                gtk_entry_completion_set_popup_completion (GtkEntryCompletion *completion, gboolean popup_completion);
gboolean            gtk_entry_completion_get_popup_completion (GtkEntryCompletion *completion);
void                gtk_entry_completion_set_popup_set_width (GtkEntryCompletion *completion, gboolean popup_set_width);
gboolean            gtk_entry_completion_get_popup_set_width (GtkEntryCompletion *completion);
void                gtk_entry_completion_set_popup_single_match (GtkEntryCompletion *completion, gboolean popup_single_match);
gboolean            gtk_entry_completion_get_popup_single_match (GtkEntryCompletion *completion);
const gchar        *gtk_entry_completion_get_completion_prefix (GtkEntryCompletion *completion);
gchar              *gtk_entry_completion_compute_prefix (GtkEntryCompletion *completion, const char *key);
void                gtk_entry_set_completion (GtkEntry *entry, GtkEntryCompletion *completion);
GtkEntryCompletion *gtk_entry_get_completion (GtkEntry *entry);

G_END_DECLS

#endif
