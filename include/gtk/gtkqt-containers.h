/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Layout containers.
 */
#ifndef GTKQT_CONTAINERS_H
#define GTKQT_CONTAINERS_H

#include <gtk/gtkqt-widget.h>

G_BEGIN_DECLS

/* Declares instance/class structs that add no public fields. */
#define GTKQT_SIMPLE_TYPE(TN, PN) \
  typedef struct _##TN { PN parent_instance; } TN; \
  typedef struct _##TN##Class { PN##Class parent_class; } TN##Class;


/* GtkBox (+ GTK 2/3 GtkHBox / GtkVBox) */
GTKQT_SIMPLE_TYPE (GtkBox, GtkContainer)
GTKQT_SIMPLE_TYPE (GtkHBox, GtkBox)
GTKQT_SIMPLE_TYPE (GtkVBox, GtkBox)
#define GTK_TYPE_BOX    (gtk_box_get_type ())
#define GTK_BOX(o)      (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_BOX, GtkBox))
#define GTK_IS_BOX(o)   (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_BOX))
#define GTK_TYPE_HBOX   (gtk_hbox_get_type ())
#define GTK_HBOX(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_HBOX, GtkHBox))
#define GTK_TYPE_VBOX   (gtk_vbox_get_type ())
#define GTK_VBOX(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_VBOX, GtkVBox))
#define GTK_IS_VBOX(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_VBOX))
#define GTK_IS_HBOX(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_HBOX))
GType      gtk_box_get_type (void) G_GNUC_CONST;
GType      gtk_hbox_get_type (void) G_GNUC_CONST;
GType      gtk_vbox_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_box_new (GtkOrientation orientation, gint spacing);
GtkWidget *gtk_hbox_new (gboolean homogeneous, gint spacing);
GtkWidget *gtk_vbox_new (gboolean homogeneous, gint spacing);
void       gtk_box_pack_start (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding);
void       gtk_box_pack_end (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding);
void       gtk_box_pack_start_defaults (GtkBox *box, GtkWidget *widget);
void       gtk_box_pack_end_defaults (GtkBox *box, GtkWidget *widget);
void       gtk_box_append (GtkBox *box, GtkWidget *child);
void       gtk_box_prepend (GtkBox *box, GtkWidget *child);
void       gtk_box_remove (GtkBox *box, GtkWidget *child);
void       gtk_box_insert_child_after (GtkBox *box, GtkWidget *child, GtkWidget *sibling);
void       gtk_box_reorder_child (GtkBox *box, GtkWidget *child, gint position);
void       gtk_box_set_homogeneous (GtkBox *box, gboolean homogeneous);
gboolean   gtk_box_get_homogeneous (GtkBox *box);
void       gtk_box_set_spacing (GtkBox *box, gint spacing);
gint       gtk_box_get_spacing (GtkBox *box);
void       gtk_box_set_child_packing (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding, GtkPackType pack_type);
void       gtk_box_query_child_packing (GtkBox *box, GtkWidget *child, gboolean *expand, gboolean *fill, guint *padding, GtkPackType *pack_type);

/* GtkOrientable */
typedef struct _GtkOrientable GtkOrientable;
#define GTK_ORIENTABLE(o) ((GtkOrientable *) (o))
void           gtk_orientable_set_orientation (GtkOrientable *orientable, GtkOrientation orientation);
GtkOrientation gtk_orientable_get_orientation (GtkOrientable *orientable);

/* GtkButtonBox (GTK 2/3) */
GTKQT_SIMPLE_TYPE (GtkButtonBox, GtkBox)
#define GTK_TYPE_BUTTON_BOX (gtk_button_box_get_type ())
#define GTK_BUTTON_BOX(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_BUTTON_BOX, GtkButtonBox))
#define GTK_IS_BUTTON_BOX(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_BUTTON_BOX))
typedef enum { GTK_BUTTONBOX_SPREAD = 1, GTK_BUTTONBOX_EDGE, GTK_BUTTONBOX_START, GTK_BUTTONBOX_END,
               GTK_BUTTONBOX_CENTER, GTK_BUTTONBOX_EXPAND } GtkButtonBoxStyle;
GType      gtk_button_box_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_button_box_new (GtkOrientation orientation);
GtkWidget *gtk_hbutton_box_new (void);
GtkWidget *gtk_vbutton_box_new (void);
void       gtk_button_box_set_layout (GtkButtonBox *widget, GtkButtonBoxStyle layout_style);

/* GtkGrid (GTK 3/4) */
GTKQT_SIMPLE_TYPE (GtkGrid, GtkContainer)
#define GTK_TYPE_GRID  (gtk_grid_get_type ())
#define GTK_GRID(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_GRID, GtkGrid))
#define GTK_IS_GRID(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_GRID))
GType      gtk_grid_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_grid_new (void);
void       gtk_grid_attach (GtkGrid *grid, GtkWidget *child, gint left, gint top, gint width, gint height);
void       gtk_grid_attach_next_to (GtkGrid *grid, GtkWidget *child, GtkWidget *sibling, GtkPositionType side, gint width, gint height);
void       gtk_grid_remove (GtkGrid *grid, GtkWidget *child);
GtkWidget *gtk_grid_get_child_at (GtkGrid *grid, gint left, gint top);
void       gtk_grid_set_row_spacing (GtkGrid *grid, guint spacing);
void       gtk_grid_set_column_spacing (GtkGrid *grid, guint spacing);
void       gtk_grid_set_row_homogeneous (GtkGrid *grid, gboolean homogeneous);
void       gtk_grid_set_column_homogeneous (GtkGrid *grid, gboolean homogeneous);
void       gtk_grid_insert_row (GtkGrid *grid, gint position);
void       gtk_grid_insert_column (GtkGrid *grid, gint position);

/* GtkTable (GTK 2/3) */
GTKQT_SIMPLE_TYPE (GtkTable, GtkContainer)
#define GTK_TYPE_TABLE (gtk_table_get_type ())
#define GTK_TABLE(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TABLE, GtkTable))
#define GTK_IS_TABLE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TABLE))
GType      gtk_table_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_table_new (guint rows, guint columns, gboolean homogeneous);
void       gtk_table_resize (GtkTable *table, guint rows, guint columns);
void       gtk_table_attach (GtkTable *table, GtkWidget *child, guint left_attach, guint right_attach,
                             guint top_attach, guint bottom_attach, GtkAttachOptions xoptions,
                             GtkAttachOptions yoptions, guint xpadding, guint ypadding);
void       gtk_table_attach_defaults (GtkTable *table, GtkWidget *widget, guint left_attach, guint right_attach,
                                      guint top_attach, guint bottom_attach);
void       gtk_table_set_row_spacings (GtkTable *table, guint spacing);
void       gtk_table_set_col_spacings (GtkTable *table, guint spacing);
void       gtk_table_set_row_spacing (GtkTable *table, guint row, guint spacing);
void       gtk_table_set_col_spacing (GtkTable *table, guint column, guint spacing);
void       gtk_table_set_homogeneous (GtkTable *table, gboolean homogeneous);

/* GtkFixed */
GTKQT_SIMPLE_TYPE (GtkFixed, GtkContainer)
#define GTK_TYPE_FIXED (gtk_fixed_get_type ())
#define GTK_FIXED(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FIXED, GtkFixed))
#define GTK_IS_FIXED(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_FIXED))
GType      gtk_fixed_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_fixed_new (void);
void       gtk_fixed_put (GtkFixed *fixed, GtkWidget *widget, gint x, gint y);
void       gtk_fixed_move (GtkFixed *fixed, GtkWidget *widget, gint x, gint y);
void       gtk_fixed_remove (GtkFixed *fixed, GtkWidget *widget);

/* GtkFrame */
GTKQT_SIMPLE_TYPE (GtkFrame, GtkBin)
#define GTK_TYPE_FRAME (gtk_frame_get_type ())
#define GTK_FRAME(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FRAME, GtkFrame))
#define GTK_IS_FRAME(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_FRAME))
GType      gtk_frame_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_frame_new (const gchar *label);
void       gtk_frame_set_label (GtkFrame *frame, const gchar *label);
const gchar *gtk_frame_get_label (GtkFrame *frame);
void       gtk_frame_set_shadow_type (GtkFrame *frame, GtkShadowType type);
void       gtk_frame_set_label_align (GtkFrame *frame, gfloat xalign, gfloat yalign);
void       gtk_frame_set_child (GtkFrame *frame, GtkWidget *child);
GtkWidget *gtk_frame_get_child (GtkFrame *frame);

/* GtkAlignment (GTK 2/3) */
GTKQT_SIMPLE_TYPE (GtkAlignment, GtkBin)
#define GTK_TYPE_ALIGNMENT (gtk_alignment_get_type ())
#define GTK_ALIGNMENT(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ALIGNMENT, GtkAlignment))
#define GTK_IS_ALIGNMENT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ALIGNMENT))
GType      gtk_alignment_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_alignment_new (gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale);
void       gtk_alignment_set (GtkAlignment *alignment, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale);
void       gtk_alignment_set_padding (GtkAlignment *alignment, guint top, guint bottom, guint left, guint right);

/* GtkEventBox (GTK 2/3) - just a bin */
GTKQT_SIMPLE_TYPE (GtkEventBox, GtkBin)
#define GTK_TYPE_EVENT_BOX (gtk_event_box_get_type ())
#define GTK_EVENT_BOX(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_EVENT_BOX, GtkEventBox))
#define GTK_IS_EVENT_BOX(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_EVENT_BOX))
GType      gtk_event_box_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_event_box_new (void);

/* GtkPaned */
GTKQT_SIMPLE_TYPE (GtkPaned, GtkContainer)
#define GTK_TYPE_PANED (gtk_paned_get_type ())
#define GTK_PANED(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_PANED, GtkPaned))
#define GTK_IS_PANED(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_PANED))
GType      gtk_paned_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_paned_new (GtkOrientation orientation);
GtkWidget *gtk_hpaned_new (void);
GtkWidget *gtk_vpaned_new (void);
void       gtk_paned_add1 (GtkPaned *paned, GtkWidget *child);
void       gtk_paned_add2 (GtkPaned *paned, GtkWidget *child);
void       gtk_paned_pack1 (GtkPaned *paned, GtkWidget *child, gboolean resize, gboolean shrink);
void       gtk_paned_pack2 (GtkPaned *paned, GtkWidget *child, gboolean resize, gboolean shrink);
void       gtk_paned_set_start_child (GtkPaned *paned, GtkWidget *child);
void       gtk_paned_set_end_child (GtkPaned *paned, GtkWidget *child);
GtkWidget *gtk_paned_get_start_child (GtkPaned *paned);
GtkWidget *gtk_paned_get_end_child (GtkPaned *paned);
void       gtk_paned_set_position (GtkPaned *paned, gint position);
gint       gtk_paned_get_position (GtkPaned *paned);

/* GtkNotebook */
GTKQT_SIMPLE_TYPE (GtkNotebook, GtkContainer)
#define GTK_TYPE_NOTEBOOK (gtk_notebook_get_type ())
#define GTK_NOTEBOOK(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_NOTEBOOK, GtkNotebook))
#define GTK_IS_NOTEBOOK(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_NOTEBOOK))
GType      gtk_notebook_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_notebook_new (void);
gint       gtk_notebook_append_page (GtkNotebook *notebook, GtkWidget *child, GtkWidget *tab_label);
gint       gtk_notebook_prepend_page (GtkNotebook *notebook, GtkWidget *child, GtkWidget *tab_label);
gint       gtk_notebook_insert_page (GtkNotebook *notebook, GtkWidget *child, GtkWidget *tab_label, gint position);
void       gtk_notebook_remove_page (GtkNotebook *notebook, gint page_num);
gint       gtk_notebook_get_current_page (GtkNotebook *notebook);
void       gtk_notebook_set_current_page (GtkNotebook *notebook, gint page_num);
GtkWidget *gtk_notebook_get_nth_page (GtkNotebook *notebook, gint page_num);
gint       gtk_notebook_get_n_pages (GtkNotebook *notebook);
gint       gtk_notebook_page_num (GtkNotebook *notebook, GtkWidget *child);
void       gtk_notebook_next_page (GtkNotebook *notebook);
void       gtk_notebook_prev_page (GtkNotebook *notebook);
void       gtk_notebook_set_tab_pos (GtkNotebook *notebook, GtkPositionType pos);
void       gtk_notebook_set_show_tabs (GtkNotebook *notebook, gboolean show_tabs);
void       gtk_notebook_set_show_border (GtkNotebook *notebook, gboolean show_border);
void       gtk_notebook_set_scrollable (GtkNotebook *notebook, gboolean scrollable);
void       gtk_notebook_set_tab_label_text (GtkNotebook *notebook, GtkWidget *child, const gchar *tab_text);
const gchar *gtk_notebook_get_tab_label_text (GtkNotebook *notebook, GtkWidget *child);
void       gtk_notebook_set_tab_reorderable (GtkNotebook *notebook, GtkWidget *child, gboolean reorderable);

/* GtkStack + GtkStackSwitcher (GTK 3/4) */
GTKQT_SIMPLE_TYPE (GtkStack, GtkContainer)
GTKQT_SIMPLE_TYPE (GtkStackSwitcher, GtkBox)
#define GTK_TYPE_STACK            (gtk_stack_get_type ())
#define GTK_STACK(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_STACK, GtkStack))
#define GTK_IS_STACK(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_STACK))
#define GTK_TYPE_STACK_SWITCHER   (gtk_stack_switcher_get_type ())
#define GTK_STACK_SWITCHER(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_STACK_SWITCHER, GtkStackSwitcher))
#define GTK_IS_STACK_SWITCHER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_STACK_SWITCHER))
GType      gtk_stack_get_type (void) G_GNUC_CONST;
GType      gtk_stack_switcher_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_stack_new (void);
void       gtk_stack_add_named (GtkStack *stack, GtkWidget *child, const gchar *name);
void       gtk_stack_add_titled (GtkStack *stack, GtkWidget *child, const gchar *name, const gchar *title);
void       gtk_stack_add_child (GtkStack *stack, GtkWidget *child);
void       gtk_stack_remove (GtkStack *stack, GtkWidget *child);
void       gtk_stack_set_visible_child (GtkStack *stack, GtkWidget *child);
GtkWidget *gtk_stack_get_visible_child (GtkStack *stack);
void       gtk_stack_set_visible_child_name (GtkStack *stack, const gchar *name);
const gchar *gtk_stack_get_visible_child_name (GtkStack *stack);
GtkWidget *gtk_stack_get_child_by_name (GtkStack *stack, const gchar *name);
void       gtk_stack_set_transition_type (GtkStack *stack, GtkStackTransitionType transition);
void       gtk_stack_set_transition_duration (GtkStack *stack, guint duration);
GtkWidget *gtk_stack_switcher_new (void);
void       gtk_stack_switcher_set_stack (GtkStackSwitcher *switcher, GtkStack *stack);

/* GtkScrolledWindow + GtkViewport */
GTKQT_SIMPLE_TYPE (GtkScrolledWindow, GtkBin)
GTKQT_SIMPLE_TYPE (GtkViewport, GtkBin)
#define GTK_TYPE_SCROLLED_WINDOW (gtk_scrolled_window_get_type ())
#define GTK_SCROLLED_WINDOW(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_SCROLLED_WINDOW, GtkScrolledWindow))
#define GTK_IS_SCROLLED_WINDOW(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_SCROLLED_WINDOW))
#define GTK_TYPE_VIEWPORT        (gtk_viewport_get_type ())
#define GTK_VIEWPORT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_VIEWPORT, GtkViewport))
#define GTK_IS_VIEWPORT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_VIEWPORT))
GType      gtk_scrolled_window_get_type (void) G_GNUC_CONST;
GType      gtk_viewport_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_scrolled_window_new (GtkAdjustment *hadjustment, GtkAdjustment *vadjustment);  /* GTK 4: no args */
void       gtk_scrolled_window_set_policy (GtkScrolledWindow *scrolled_window, GtkPolicyType hscrollbar_policy, GtkPolicyType vscrollbar_policy);
void       gtk_scrolled_window_set_shadow_type (GtkScrolledWindow *scrolled_window, GtkShadowType type);
void       gtk_scrolled_window_set_has_frame (GtkScrolledWindow *scrolled_window, gboolean has_frame);
void       gtk_scrolled_window_add_with_viewport (GtkScrolledWindow *scrolled_window, GtkWidget *child);
void       gtk_scrolled_window_set_child (GtkScrolledWindow *scrolled_window, GtkWidget *child);
GtkWidget *gtk_scrolled_window_get_child (GtkScrolledWindow *scrolled_window);
void       gtk_scrolled_window_set_min_content_width (GtkScrolledWindow *scrolled_window, gint width);
void       gtk_scrolled_window_set_min_content_height (GtkScrolledWindow *scrolled_window, gint height);
void       gtk_scrolled_window_set_propagate_natural_height (GtkScrolledWindow *scrolled_window, gboolean propagate);
GtkWidget *gtk_viewport_new (GtkAdjustment *hadjustment, GtkAdjustment *vadjustment);

/* GtkExpander */
GTKQT_SIMPLE_TYPE (GtkExpander, GtkBin)
#define GTK_TYPE_EXPANDER (gtk_expander_get_type ())
#define GTK_EXPANDER(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_EXPANDER, GtkExpander))
#define GTK_IS_EXPANDER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_EXPANDER))
GType      gtk_expander_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_expander_new (const gchar *label);
void       gtk_expander_set_expanded (GtkExpander *expander, gboolean expanded);
gboolean   gtk_expander_get_expanded (GtkExpander *expander);
void       gtk_expander_set_child (GtkExpander *expander, GtkWidget *child);

/* GtkHeaderBar (GTK 3/4): shown as a bar along the top of the window */
GTKQT_SIMPLE_TYPE (GtkHeaderBar, GtkContainer)
#define GTK_TYPE_HEADER_BAR (gtk_header_bar_get_type ())
#define GTK_HEADER_BAR(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_HEADER_BAR, GtkHeaderBar))
#define GTK_IS_HEADER_BAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_HEADER_BAR))
GType      gtk_header_bar_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_header_bar_new (void);
void       gtk_header_bar_pack_start (GtkHeaderBar *bar, GtkWidget *child);
void       gtk_header_bar_pack_end (GtkHeaderBar *bar, GtkWidget *child);
void       gtk_header_bar_set_title (GtkHeaderBar *bar, const gchar *title);
void       gtk_header_bar_set_subtitle (GtkHeaderBar *bar, const gchar *subtitle);
void       gtk_header_bar_set_title_widget (GtkHeaderBar *bar, GtkWidget *title_widget);
void       gtk_header_bar_set_show_close_button (GtkHeaderBar *bar, gboolean setting);
void       gtk_header_bar_set_show_title_buttons (GtkHeaderBar *bar, gboolean setting);

/* GtkSeparator */
GTKQT_SIMPLE_TYPE (GtkSeparator, GtkWidget)
#define GTK_TYPE_SEPARATOR (gtk_separator_get_type ())
#define GTK_SEPARATOR(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_SEPARATOR, GtkSeparator))
#define GTK_IS_SEPARATOR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_SEPARATOR))
GType      gtk_separator_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_separator_new (GtkOrientation orientation);
GtkWidget *gtk_hseparator_new (void);
GtkWidget *gtk_vseparator_new (void);

/* GtkStatusbar */
GTKQT_SIMPLE_TYPE (GtkStatusbar, GtkBox)
#define GTK_TYPE_STATUSBAR (gtk_statusbar_get_type ())
#define GTK_STATUSBAR(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_STATUSBAR, GtkStatusbar))
#define GTK_IS_STATUSBAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_STATUSBAR))
GType      gtk_statusbar_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_statusbar_new (void);
guint      gtk_statusbar_get_context_id (GtkStatusbar *statusbar, const gchar *context_description);
guint      gtk_statusbar_push (GtkStatusbar *statusbar, guint context_id, const gchar *text);
void       gtk_statusbar_pop (GtkStatusbar *statusbar, guint context_id);
void       gtk_statusbar_remove (GtkStatusbar *statusbar, guint context_id, guint message_id);
void       gtk_statusbar_remove_all (GtkStatusbar *statusbar, guint context_id);
void       gtk_statusbar_set_has_resize_grip (GtkStatusbar *statusbar, gboolean setting);

G_END_DECLS

#endif
