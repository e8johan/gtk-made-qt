/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GTK 1.2 widgets (most remained in GTK 2 as deprecated widgets).
 * The public fields programs read are kept current.
 */
#ifndef GTKQT_GTK1WIDGETS_H
#define GTKQT_GTK1WIDGETS_H

G_BEGIN_DECLS

/* ---------- GtkPixmap: shows a GdkPixmap ---------- */
typedef struct _GtkPixmap { GtkWidget parent_instance; GdkPixmap *pixmap; GdkBitmap *mask; gboolean build_insensitive; } GtkPixmap;
typedef struct { GtkWidgetClass parent_class; } GtkPixmapClass;
#define GTK_TYPE_PIXMAP     (gtk_pixmap_get_type ())
#define GTK_PIXMAP(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_PIXMAP, GtkPixmap))
#define GTK_IS_PIXMAP(o)    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_PIXMAP))
GType      gtk_pixmap_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_pixmap_new (GdkPixmap *pixmap, GdkBitmap *mask);
void       gtk_pixmap_set (GtkPixmap *pixmap, GdkPixmap *val, GdkBitmap *mask);
void       gtk_pixmap_get (GtkPixmap *pixmap, GdkPixmap **val, GdkBitmap **mask);
void       gtk_pixmap_set_build_insensitive (GtkPixmap *pixmap, gboolean build);

/* ---------- GtkAspectFrame ---------- */
typedef struct _GtkAspectFrame { GtkFrame parent_instance; gfloat xalign, yalign, ratio; gboolean obey_child; } GtkAspectFrame;
typedef struct { GtkFrameClass parent_class; } GtkAspectFrameClass;
#define GTK_TYPE_ASPECT_FRAME (gtk_aspect_frame_get_type ())
#define GTK_ASPECT_FRAME(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ASPECT_FRAME, GtkAspectFrame))
GType      gtk_aspect_frame_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_aspect_frame_new (const gchar *label, gfloat xalign, gfloat yalign, gfloat ratio, gboolean obey_child);
void       gtk_aspect_frame_set (GtkAspectFrame *aspect_frame, gfloat xalign, gfloat yalign, gfloat ratio, gboolean obey_child);

/* ---------- GtkCList (a QTreeWidget) ---------- */
typedef enum { GTK_VISIBILITY_NONE, GTK_VISIBILITY_PARTIAL, GTK_VISIBILITY_FULL } GtkVisibility;
typedef enum { GTK_CELL_EMPTY, GTK_CELL_TEXT, GTK_CELL_PIXMAP, GTK_CELL_PIXTEXT, GTK_CELL_WIDGET } GtkCellType;
typedef enum { GTK_BUTTON_IGNORED = 0, GTK_BUTTON_SELECTS = 1 << 0, GTK_BUTTON_DRAGS = 1 << 1, GTK_BUTTON_EXPANDS = 1 << 2 } GtkButtonAction;
typedef struct _GtkCList GtkCList;
typedef gint (*GtkCListCompareFunc) (GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2);
typedef struct { gchar *title; GdkRectangle area; GtkWidget *button; gint width; GtkJustification justification; gboolean visible; } GtkCListColumn;
/* GTK 1's cell structures: row->cell[column] */
typedef struct { GtkCellType type; gint16 vertical, horizontal; GtkStyle *style;
                 union { gchar *text; struct { GdkPixmap *pixmap; GdkBitmap *mask; } pm;
                         struct { gchar *text; guint8 spacing; GdkPixmap *pixmap; GdkBitmap *mask; } pt; GtkWidget *widget; } u; } GtkCell;
typedef struct { GtkCellType type; gint16 vertical, horizontal; GtkStyle *style; gchar *text; } GtkCellText;
typedef struct { GtkCellType type; gint16 vertical, horizontal; GtkStyle *style; GdkPixmap *pixmap; GdkBitmap *mask; } GtkCellPixmap;
typedef struct { GtkCellType type; gint16 vertical, horizontal; GtkStyle *style; gchar *text; guint8 spacing;
                 GdkPixmap *pixmap; GdkBitmap *mask; } GtkCellPixText;
#define GTK_CELL_TEXT(cell)    (((GtkCellText *) &(cell)))
#define GTK_CELL_PIXMAP(cell)  (((GtkCellPixmap *) &(cell)))
#define GTK_CELL_PIXTEXT(cell) (((GtkCellPixText *) &(cell)))
typedef struct _GtkCListRow { GtkCell *cell; GtkStateType state; GdkColor foreground, background; GtkStyle *style;
                              gpointer data; GDestroyNotify destroy; guint fg_set : 1, bg_set : 1, selectable : 1; } GtkCListRow;
struct _GtkCList {
  GtkContainer parent_instance;
  guint16 flags;
  gint rows;
  gint row_height;
  GList *row_list, *row_list_end;
  gint columns;
  GtkCListColumn *column;
  gint focus_row;
  GtkSelectionMode selection_mode;
  GList *selection, *selection_end, *undo_selection, *undo_unselection;
  gint sort_column;
  GtkSortType sort_type;
  GtkCListCompareFunc compare;
  GtkAdjustment *hadjustment, *vadjustment;
  /*< private >*/
  gpointer priv;
};
typedef struct {
  GtkContainerClass parent_class;
  void (*select_row) (GtkCList *clist, gint row, gint column, GdkEvent *event);
  void (*unselect_row) (GtkCList *clist, gint row, gint column, GdkEvent *event);
  void (*click_column) (GtkCList *clist, gint column);
} GtkCListClass;
#define GTK_TYPE_CLIST     (gtk_clist_get_type ())
#define GTK_CLIST(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_CLIST, GtkCList))
#define GTK_IS_CLIST(o)    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_CLIST))
#define GTK_CLIST_ROW(glist) ((GtkCListRow *) ((glist)->data))
GType      gtk_clist_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_clist_new (gint columns);
GtkWidget *gtk_clist_new_with_titles (gint columns, gchar *titles[]);
void       gtk_clist_construct (GtkCList *clist, gint columns, gchar *titles[]);
gint       gtk_clist_append (GtkCList *clist, gchar *text[]);
gint       gtk_clist_prepend (GtkCList *clist, gchar *text[]);
gint       gtk_clist_insert (GtkCList *clist, gint row, gchar *text[]);
void       gtk_clist_remove (GtkCList *clist, gint row);
void       gtk_clist_clear (GtkCList *clist);
void       gtk_clist_freeze (GtkCList *clist);
void       gtk_clist_thaw (GtkCList *clist);
void       gtk_clist_set_text (GtkCList *clist, gint row, gint column, const gchar *text);
gint       gtk_clist_get_text (GtkCList *clist, gint row, gint column, gchar **text);
void       gtk_clist_set_pixmap (GtkCList *clist, gint row, gint column, GdkPixmap *pixmap, GdkBitmap *mask);
gint       gtk_clist_get_pixmap (GtkCList *clist, gint row, gint column, GdkPixmap **pixmap, GdkBitmap **mask);
void       gtk_clist_set_pixtext (GtkCList *clist, gint row, gint column, const gchar *text, guint8 spacing, GdkPixmap *pixmap, GdkBitmap *mask);
gint       gtk_clist_get_pixtext (GtkCList *clist, gint row, gint column, gchar **text, guint8 *spacing, GdkPixmap **pixmap, GdkBitmap **mask);
GtkCellType gtk_clist_get_cell_type (GtkCList *clist, gint row, gint column);
void       gtk_clist_set_row_data (GtkCList *clist, gint row, gpointer data);
void       gtk_clist_set_row_data_full (GtkCList *clist, gint row, gpointer data, GDestroyNotify destroy);
gpointer   gtk_clist_get_row_data (GtkCList *clist, gint row);
gint       gtk_clist_find_row_from_data (GtkCList *clist, gpointer data);
void       gtk_clist_select_row (GtkCList *clist, gint row, gint column);
void       gtk_clist_unselect_row (GtkCList *clist, gint row, gint column);
void       gtk_clist_select_all (GtkCList *clist);
void       gtk_clist_unselect_all (GtkCList *clist);
void       gtk_clist_undo_selection (GtkCList *clist);
void       gtk_clist_set_selection_mode (GtkCList *clist, GtkSelectionMode mode);
void       gtk_clist_set_selectable (GtkCList *clist, gint row, gboolean selectable);
gboolean   gtk_clist_get_selectable (GtkCList *clist, gint row);
void       gtk_clist_set_column_title (GtkCList *clist, gint column, const gchar *title);
gchar     *gtk_clist_get_column_title (GtkCList *clist, gint column);
void       gtk_clist_set_column_widget (GtkCList *clist, gint column, GtkWidget *widget);
void       gtk_clist_set_column_width (GtkCList *clist, gint column, gint width);
void       gtk_clist_set_column_min_width (GtkCList *clist, gint column, gint min_width);
void       gtk_clist_set_column_max_width (GtkCList *clist, gint column, gint max_width);
void       gtk_clist_set_column_justification (GtkCList *clist, gint column, GtkJustification justification);
void       gtk_clist_set_column_visibility (GtkCList *clist, gint column, gboolean visible);
void       gtk_clist_set_column_resizeable (GtkCList *clist, gint column, gboolean resizeable);
void       gtk_clist_set_column_auto_resize (GtkCList *clist, gint column, gboolean auto_resize);
gint       gtk_clist_columns_autosize (GtkCList *clist);
gint       gtk_clist_optimal_column_width (GtkCList *clist, gint column);
void       gtk_clist_column_titles_show (GtkCList *clist);
void       gtk_clist_column_titles_hide (GtkCList *clist);
void       gtk_clist_column_titles_active (GtkCList *clist);
void       gtk_clist_column_titles_passive (GtkCList *clist);
void       gtk_clist_column_title_active (GtkCList *clist, gint column);
void       gtk_clist_column_title_passive (GtkCList *clist, gint column);
void       gtk_clist_set_shadow_type (GtkCList *clist, GtkShadowType type);
void       gtk_clist_set_row_height (GtkCList *clist, guint height);
void       gtk_clist_set_foreground (GtkCList *clist, gint row, const GdkColor *color);
void       gtk_clist_set_background (GtkCList *clist, gint row, const GdkColor *color);
void       gtk_clist_moveto (GtkCList *clist, gint row, gint column, gfloat row_align, gfloat col_align);
gint       gtk_clist_get_selection_info (GtkCList *clist, gint x, gint y, gint *row, gint *column);
GtkVisibility gtk_clist_row_is_visible (GtkCList *clist, gint row);
void       gtk_clist_set_compare_func (GtkCList *clist, GtkCListCompareFunc cmp_func);
void       gtk_clist_set_sort_column (GtkCList *clist, gint column);
void       gtk_clist_set_sort_type (GtkCList *clist, GtkSortType sort_type);
void       gtk_clist_set_auto_sort (GtkCList *clist, gboolean auto_sort);
void       gtk_clist_sort (GtkCList *clist);
void       gtk_clist_set_reorderable (GtkCList *clist, gboolean reorderable);
void       gtk_clist_set_use_drag_icons (GtkCList *clist, gboolean use_icons);
void       gtk_clist_set_button_actions (GtkCList *clist, guint button, guint8 button_actions);
void       gtk_clist_swap_rows (GtkCList *clist, gint row1, gint row2);
void       gtk_clist_row_move (GtkCList *clist, gint source_row, gint dest_row);
void       gtk_clist_set_hadjustment (GtkCList *clist, GtkAdjustment *adjustment);
void       gtk_clist_set_vadjustment (GtkCList *clist, GtkAdjustment *adjustment);
GtkAdjustment *gtk_clist_get_hadjustment (GtkCList *clist);
GtkAdjustment *gtk_clist_get_vadjustment (GtkCList *clist);
void       gtk_clist_set_policy (GtkCList *clist, GtkPolicyType vscrollbar_policy, GtkPolicyType hscrollbar_policy);
#define    gtk_clist_set_border gtk_clist_set_shadow_type

/* ---------- GtkCTree (a GtkCList with tree rows) ---------- */
typedef struct _GtkCTreeNode GtkCTreeNode;
typedef enum { GTK_CTREE_LINES_NONE, GTK_CTREE_LINES_SOLID, GTK_CTREE_LINES_DOTTED, GTK_CTREE_LINES_TABBED } GtkCTreeLineStyle;
typedef enum { GTK_CTREE_EXPANDER_NONE, GTK_CTREE_EXPANDER_SQUARE, GTK_CTREE_EXPANDER_TRIANGLE, GTK_CTREE_EXPANDER_CIRCULAR } GtkCTreeExpanderStyle;
typedef struct _GtkCTreeRow {
  GtkCListRow row;
  GtkCTreeNode *parent, *sibling, *children;
  GdkPixmap *pixmap_closed; GdkBitmap *mask_closed;
  GdkPixmap *pixmap_opened; GdkBitmap *mask_opened;
  guint16 level;
  guint is_leaf : 1, expanded : 1;
} GtkCTreeRow;
typedef struct _GtkCTree { GtkCList clist; gint tree_indent, tree_spacing, tree_column; GtkCTreeLineStyle line_style; GtkCTreeExpanderStyle expander_style; } GtkCTree;
typedef struct { GtkCListClass parent_class; } GtkCTreeClass;
typedef void (*GtkCTreeFunc) (GtkCTree *ctree, GtkCTreeNode *node, gpointer data);
#define GTK_TYPE_CTREE     (gtk_ctree_get_type ())
#define GTK_CTREE(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_CTREE, GtkCTree))
#define GTK_IS_CTREE(o)    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_CTREE))
#define GTK_CTREE_NODE(n)  ((GtkCTreeNode *) (n))
#define GTK_CTREE_ROW(n)   (gtk_ctree_node_get_row_ (GTK_CTREE_NODE (n)))
#define GTK_CTREE_NODE_NEXT(n) (gtk_ctree_node_next_ (GTK_CTREE_NODE (n)))
GtkCTreeRow *gtk_ctree_node_get_row_ (GtkCTreeNode *node);
GtkCTreeNode *gtk_ctree_node_next_ (GtkCTreeNode *node);
GType      gtk_ctree_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_ctree_new (gint columns, gint tree_column);
GtkWidget *gtk_ctree_new_with_titles (gint columns, gint tree_column, gchar *titles[]);
GtkCTreeNode *gtk_ctree_insert_node (GtkCTree *ctree, GtkCTreeNode *parent, GtkCTreeNode *sibling, gchar *text[], guint8 spacing,
                                     GdkPixmap *pixmap_closed, GdkBitmap *mask_closed, GdkPixmap *pixmap_opened,
                                     GdkBitmap *mask_opened, gboolean is_leaf, gboolean expanded);
void       gtk_ctree_remove_node (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_node_set_row_data (GtkCTree *ctree, GtkCTreeNode *node, gpointer data);
void       gtk_ctree_node_set_row_data_full (GtkCTree *ctree, GtkCTreeNode *node, gpointer data, GDestroyNotify destroy);
gpointer   gtk_ctree_node_get_row_data (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_node_set_text (GtkCTree *ctree, GtkCTreeNode *node, gint column, const gchar *text);
gint       gtk_ctree_node_get_text (GtkCTree *ctree, GtkCTreeNode *node, gint column, gchar **text);
gint       gtk_ctree_node_get_pixtext (GtkCTree *ctree, GtkCTreeNode *node, gint column, gchar **text, guint8 *spacing,
                                       GdkPixmap **pixmap, GdkBitmap **mask);
void       gtk_ctree_node_set_pixtext (GtkCTree *ctree, GtkCTreeNode *node, gint column, const gchar *text, guint8 spacing,
                                       GdkPixmap *pixmap, GdkBitmap *mask);
GtkCTreeNode *gtk_ctree_node_nth (GtkCTree *ctree, guint row);
GtkCTreeNode *gtk_ctree_find_by_row_data (GtkCTree *ctree, GtkCTreeNode *node, gpointer data);
void       gtk_ctree_expand (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_expand_recursive (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_collapse (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_select (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_unselect (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_sort_node (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_sort_recursive (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_set_line_style (GtkCTree *ctree, GtkCTreeLineStyle line_style);
void       gtk_ctree_set_expander_style (GtkCTree *ctree, GtkCTreeExpanderStyle expander_style);
void       gtk_ctree_set_indent (GtkCTree *ctree, gint indent);
void       gtk_ctree_node_moveto (GtkCTree *ctree, GtkCTreeNode *node, gint column, gfloat row_align, gfloat col_align);
gboolean   gtk_ctree_is_viewable (GtkCTree *ctree, GtkCTreeNode *node);
void       gtk_ctree_post_recursive (GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeFunc func, gpointer data);
void       gtk_ctree_pre_recursive (GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeFunc func, gpointer data);

/* ---------- GtkList / GtkListItem ---------- */
typedef struct _GtkListItem { GtkBin parent_instance; } GtkListItem;
typedef struct { GtkBinClass parent_class; } GtkListItemClass;
typedef struct _GtkList { GtkContainer parent_instance; GList *children; GList *selection; GtkSelectionMode selection_mode; } GtkList;
typedef struct { GtkContainerClass parent_class; void (*selection_changed) (GtkList *list); void (*select_child) (GtkList *list, GtkWidget *child);
                 void (*unselect_child) (GtkList *list, GtkWidget *child); } GtkListClass;
#define GTK_TYPE_LIST       (gtk_list_get_type ())
#define GTK_LIST(o)         (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_LIST, GtkList))
#define GTK_IS_LIST(o)      (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_LIST))
#define GTK_TYPE_LIST_ITEM  (gtk_list_item_get_type ())
#define GTK_LIST_ITEM(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_LIST_ITEM, GtkListItem))
#define GTK_IS_LIST_ITEM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_LIST_ITEM))
GType      gtk_list_get_type (void) G_GNUC_CONST;
GType      gtk_list_item_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_list_new (void);
void       gtk_list_insert_items (GtkList *list, GList *items, gint position);
void       gtk_list_append_items (GtkList *list, GList *items);
void       gtk_list_prepend_items (GtkList *list, GList *items);
void       gtk_list_remove_items (GtkList *list, GList *items);
void       gtk_list_clear_items (GtkList *list, gint start, gint end);
void       gtk_list_select_item (GtkList *list, gint item);
void       gtk_list_unselect_item (GtkList *list, gint item);
void       gtk_list_select_child (GtkList *list, GtkWidget *child);
void       gtk_list_unselect_child (GtkList *list, GtkWidget *child);
gint       gtk_list_child_position (GtkList *list, GtkWidget *child);
void       gtk_list_set_selection_mode (GtkList *list, GtkSelectionMode mode);
GtkWidget *gtk_list_item_new (void);
GtkWidget *gtk_list_item_new_with_label (const gchar *label);
void       gtk_list_item_select (GtkListItem *list_item);
void       gtk_list_item_deselect (GtkListItem *list_item);

/* ---------- GtkText (GTK 1 multi-line text) ---------- */
typedef struct _GtkText { GtkWidget parent_instance; GtkAdjustment *hadj, *vadj; guint text_len; guint point; gboolean editable; } GtkText;
typedef struct { GtkWidgetClass parent_class; } GtkTextClass;
#define GTK_TYPE_TEXT  (gtk_text_get_type ())
#define GTK_TEXT(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TEXT, GtkText))
#define GTK_IS_TEXT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TEXT))
GType      gtk_text_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_text_new (GtkAdjustment *hadj, GtkAdjustment *vadj);
void       gtk_text_set_editable (GtkText *text, gboolean editable);
void       gtk_text_set_word_wrap (GtkText *text, gint word_wrap);
void       gtk_text_set_line_wrap (GtkText *text, gint line_wrap);
void       gtk_text_set_adjustments (GtkText *text, GtkAdjustment *hadj, GtkAdjustment *vadj);
void       gtk_text_set_point (GtkText *text, guint index);
guint      gtk_text_get_point (GtkText *text);
guint      gtk_text_get_length (GtkText *text);
void       gtk_text_freeze (GtkText *text);
void       gtk_text_thaw (GtkText *text);
void       gtk_text_insert (GtkText *text, GdkFont *font, const GdkColor *fore, const GdkColor *back, const char *chars, gint length);
gint       gtk_text_backward_delete (GtkText *text, guint nchars);
gint       gtk_text_forward_delete (GtkText *text, guint nchars);

/* ---------- GtkProgress (GTK 1 API of GtkProgressBar) ---------- */
typedef enum { GTK_PROGRESS_CONTINUOUS, GTK_PROGRESS_DISCRETE } GtkProgressBarStyle;
typedef GtkProgressBar GtkProgress;
#define GTK_PROGRESS(o)  ((GtkProgress *) (o))
void       gtk_progress_bar_update (GtkProgressBar *pbar, gfloat percentage);
void       gtk_progress_bar_set_bar_style (GtkProgressBar *pbar, GtkProgressBarStyle style);
void       gtk_progress_bar_set_discrete_blocks (GtkProgressBar *pbar, guint blocks);
void       gtk_progress_bar_set_activity_step (GtkProgressBar *pbar, guint step);
void       gtk_progress_bar_set_activity_blocks (GtkProgressBar *pbar, guint blocks);
GtkWidget *gtk_progress_bar_new_with_adjustment (GtkAdjustment *adjustment);
void       gtk_progress_set_show_text (GtkProgress *progress, gboolean show_text);
void       gtk_progress_set_text_alignment (GtkProgress *progress, gfloat x_align, gfloat y_align);
void       gtk_progress_set_format_string (GtkProgress *progress, const gchar *format);
void       gtk_progress_set_adjustment (GtkProgress *progress, GtkAdjustment *adjustment);
void       gtk_progress_configure (GtkProgress *progress, gfloat value, gfloat min, gfloat max);
void       gtk_progress_set_percentage (GtkProgress *progress, gfloat percentage);
void       gtk_progress_set_value (GtkProgress *progress, gfloat value);
gfloat     gtk_progress_get_value (GtkProgress *progress);
gfloat     gtk_progress_get_current_percentage (GtkProgress *progress);
void       gtk_progress_set_activity_mode (GtkProgress *progress, guint activity_mode);
GtkAdjustment *gtk_progress_get_adjustment_ (GtkProgress *progress);

/* ---------- GtkRuler ---------- */
typedef enum { GTK_PIXELS, GTK_INCHES, GTK_CENTIMETERS } GtkMetricType;
typedef struct _GtkRuler { GtkWidget parent_instance; gint xsrc, ysrc, slider_size; gdouble lower, upper, position, max_size; GtkMetricType metric; } GtkRuler;
typedef struct { GtkWidgetClass parent_class; } GtkRulerClass;
#define GTK_TYPE_RULER  (gtk_ruler_get_type ())
#define GTK_RULER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_RULER, GtkRuler))
#define GTK_IS_RULER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_RULER))
#define GTK_HRULER(o)   GTK_RULER (o)
#define GTK_VRULER(o)   GTK_RULER (o)
GType      gtk_ruler_get_type (void) G_GNUC_CONST;
GType      gtk_hruler_get_type (void) G_GNUC_CONST;
GType      gtk_vruler_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_hruler_new (void);
GtkWidget *gtk_vruler_new (void);
void       gtk_ruler_set_metric (GtkRuler *ruler, GtkMetricType metric);
void       gtk_ruler_set_range (GtkRuler *ruler, gdouble lower, gdouble upper, gdouble position, gdouble max_size);

/* ---------- GtkCalendar ---------- */
typedef enum { GTK_CALENDAR_SHOW_HEADING = 1 << 0, GTK_CALENDAR_SHOW_DAY_NAMES = 1 << 1, GTK_CALENDAR_NO_MONTH_CHANGE = 1 << 2,
               GTK_CALENDAR_SHOW_WEEK_NUMBERS = 1 << 3, GTK_CALENDAR_WEEK_START_MONDAY = 1 << 4 } GtkCalendarDisplayOptions;
typedef struct _GtkCalendar { GtkWidget parent_instance; gint month, year, selected_day; gint num_marked_dates; gint marked_date[31]; } GtkCalendar;
typedef struct { GtkWidgetClass parent_class; } GtkCalendarClass;
#define GTK_TYPE_CALENDAR  (gtk_calendar_get_type ())
#define GTK_CALENDAR(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_CALENDAR, GtkCalendar))
#define GTK_IS_CALENDAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_CALENDAR))
GType      gtk_calendar_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_calendar_new (void);
gint       gtk_calendar_select_month (GtkCalendar *calendar, guint month, guint year);
void       gtk_calendar_select_day (GtkCalendar *calendar, guint day);
gint       gtk_calendar_mark_day (GtkCalendar *calendar, guint day);
gint       gtk_calendar_unmark_day (GtkCalendar *calendar, guint day);
void       gtk_calendar_clear_marks (GtkCalendar *calendar);
void       gtk_calendar_display_options (GtkCalendar *calendar, GtkCalendarDisplayOptions flags);
#define    gtk_calendar_set_display_options gtk_calendar_display_options
void       gtk_calendar_get_date (GtkCalendar *calendar, guint *year, guint *month, guint *day);
void       gtk_calendar_freeze (GtkCalendar *calendar);
void       gtk_calendar_thaw (GtkCalendar *calendar);

/* ---------- GtkPacker ---------- */
typedef enum { GTK_PACK_EXPAND = 1 << 0, GTK_FILL_X = 1 << 1, GTK_FILL_Y = 1 << 2 } GtkPackerOptions;
typedef enum { GTK_SIDE_TOP, GTK_SIDE_BOTTOM, GTK_SIDE_LEFT, GTK_SIDE_RIGHT } GtkSideType;
typedef enum { GTK_ANCHOR_CENTER, GTK_ANCHOR_NORTH, GTK_ANCHOR_NORTH_WEST, GTK_ANCHOR_NORTH_EAST, GTK_ANCHOR_SOUTH,
               GTK_ANCHOR_SOUTH_WEST, GTK_ANCHOR_SOUTH_EAST, GTK_ANCHOR_WEST, GTK_ANCHOR_EAST,
               GTK_ANCHOR_N = GTK_ANCHOR_NORTH, GTK_ANCHOR_NW = GTK_ANCHOR_NORTH_WEST, GTK_ANCHOR_NE = GTK_ANCHOR_NORTH_EAST,
               GTK_ANCHOR_S = GTK_ANCHOR_SOUTH, GTK_ANCHOR_SW = GTK_ANCHOR_SOUTH_WEST, GTK_ANCHOR_SE = GTK_ANCHOR_SOUTH_EAST,
               GTK_ANCHOR_W = GTK_ANCHOR_WEST, GTK_ANCHOR_E = GTK_ANCHOR_EAST } GtkAnchorType;
typedef struct _GtkPackerChild {
  GtkWidget *widget;
  GtkAnchorType anchor;
  GtkSideType side;
  GtkPackerOptions options;
  guint use_default : 1;
  guint border_width : 16;
  guint pad_x : 16, pad_y : 16, i_pad_x : 16, i_pad_y : 16;
} GtkPackerChild;
typedef struct _GtkPacker { GtkContainer parent_instance; GList *children; guint spacing; guint default_border_width : 16;
                            guint default_pad_x : 16, default_pad_y : 16, default_i_pad_x : 16, default_i_pad_y : 16; } GtkPacker;
typedef struct { GtkContainerClass parent_class; } GtkPackerClass;
#define GTK_TYPE_PACKER  (gtk_packer_get_type ())
#define GTK_PACKER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_PACKER, GtkPacker))
#define GTK_IS_PACKER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_PACKER))
GType      gtk_packer_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_packer_new (void);
void       gtk_packer_add_defaults (GtkPacker *packer, GtkWidget *child, GtkSideType side, GtkAnchorType anchor, GtkPackerOptions options);
void       gtk_packer_add (GtkPacker *packer, GtkWidget *child, GtkSideType side, GtkAnchorType anchor, GtkPackerOptions options,
                           guint border_width, guint pad_x, guint pad_y, guint i_pad_x, guint i_pad_y);
void       gtk_packer_set_child_packing (GtkPacker *packer, GtkWidget *child, GtkSideType side, GtkAnchorType anchor,
                                         GtkPackerOptions options, guint border_width, guint pad_x, guint pad_y, guint i_pad_x, guint i_pad_y);
void       gtk_packer_reorder_child (GtkPacker *packer, GtkWidget *child, gint position);
void       gtk_packer_set_spacing (GtkPacker *packer, guint spacing);
void       gtk_packer_set_default_border_width (GtkPacker *packer, guint border);
void       gtk_packer_set_default_pad (GtkPacker *packer, guint pad_x, guint pad_y);
void       gtk_packer_set_default_ipad (GtkPacker *packer, guint i_pad_x, guint i_pad_y);

/* ---------- GtkTooltips ---------- */
typedef struct _GtkTooltips GtkTooltips;
#define GTK_TYPE_TOOLTIPS (gtk_tooltips_get_type ())
#define GTK_TOOLTIPS(o)   ((GtkTooltips *) (o))
GType        gtk_tooltips_get_type (void) G_GNUC_CONST;
GtkTooltips *gtk_tooltips_new (void);
void         gtk_tooltips_enable (GtkTooltips *tooltips);
void         gtk_tooltips_disable (GtkTooltips *tooltips);
void         gtk_tooltips_set_delay (GtkTooltips *tooltips, guint delay);
void         gtk_tooltips_set_tip (GtkTooltips *tooltips, GtkWidget *widget, const gchar *tip_text, const gchar *tip_private);
void         gtk_tooltips_set_colors (GtkTooltips *tooltips, GdkColor *background, GdkColor *foreground);
void         gtk_tooltips_force_window (GtkTooltips *tooltips);

/* ---------- GtkColorSelection ---------- */
typedef struct _GtkColorSelection { GtkBox parent_instance; } GtkColorSelection;
typedef struct { GtkBoxClass parent_class; } GtkColorSelectionClass;
typedef struct _GtkColorSelectionDialog { GtkDialog parent_instance; GtkWidget *colorsel, *main_vbox, *ok_button, *reset_button,
                                          *cancel_button, *help_button; } GtkColorSelectionDialog;
typedef struct { GtkDialogClass parent_class; } GtkColorSelectionDialogClass;
#define GTK_TYPE_COLOR_SELECTION         (gtk_color_selection_get_type ())
#define GTK_COLOR_SELECTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_COLOR_SELECTION, GtkColorSelection))
#define GTK_TYPE_COLOR_SELECTION_DIALOG  (gtk_color_selection_dialog_get_type ())
#define GTK_COLOR_SELECTION_DIALOG(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_COLOR_SELECTION_DIALOG, GtkColorSelectionDialog))
GType      gtk_color_selection_get_type (void) G_GNUC_CONST;
GType      gtk_color_selection_dialog_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_color_selection_new (void);
void       gtk_color_selection_set_color (GtkColorSelection *colorsel, gdouble *color);
void       gtk_color_selection_get_color (GtkColorSelection *colorsel, gdouble *color);
void       gtk_color_selection_set_current_color (GtkColorSelection *colorsel, const GdkColor *color);
void       gtk_color_selection_get_current_color (GtkColorSelection *colorsel, GdkColor *color);
void       gtk_color_selection_set_opacity (GtkColorSelection *colorsel, gboolean use_opacity);
void       gtk_color_selection_set_update_policy (GtkColorSelection *colorsel, GtkUpdateType policy);
GtkWidget *gtk_color_selection_dialog_new (const gchar *title);

G_END_DECLS

#endif
