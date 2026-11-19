/*
 * GTK made Qt - GtkTextLayout (gtk/gtktextlayout.h, gtk/gtktextdisplay.h):
 * GTK 2's semi-public text layout, which text items outside GtkTextView
 * use (libgnomecanvas's GnomeCanvasRichText). A layout lays out a
 * buffer's text at a width, in its default style, answers positions and
 * draws on a GdkDrawable. It is laid out by Qt's text layout.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#ifndef GTKQT_TEXTLAYOUT_H
#define GTKQT_TEXTLAYOUT_H

#include <gtk/gtk.h>

#if !defined (GTKQT_COMPILATION) || defined (GTKQT_EXPORT_API)
#pragma GCC visibility push(default)
#endif
G_BEGIN_DECLS

#define GTK_TYPE_TEXT_LAYOUT          (gtk_text_layout_get_type ())
#define GTK_TEXT_LAYOUT(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TEXT_LAYOUT, GtkTextLayout))
#define GTK_IS_TEXT_LAYOUT(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TEXT_LAYOUT))
#define GTK_TEXT_LAYOUT_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), GTK_TYPE_TEXT_LAYOUT, GtkTextLayoutClass))

typedef struct _GtkTextLayout {
  GObject parent_instance;
  gint screen_width;          /* the width to wrap at */
  gint width, height;         /* the laid out text's size */
  GtkTextBuffer *buffer;
  GtkTextAttributes *default_style;
  PangoContext *ltr_context, *rtl_context;
  GtkTextAttributes *one_style_cache;
  gpointer one_display_cache;
  gint wrap_loop_count;
  guint cursor_visible : 1;
  guint cursor_direction : 2;
  guint keyboard_direction : 2;
  gchar *preedit_string;
  PangoAttrList *preedit_attrs;
  gint preedit_len;
  gint preedit_cursor;
  guint overwrite_mode : 1;
  /*< private >*/
  gpointer gtkqt_priv;
} GtkTextLayout;

typedef struct _GtkTextLayoutClass {
  GObjectClass parent_class;
  void (*invalidated) (GtkTextLayout *layout);
  void (*changed) (GtkTextLayout *layout, gint y, gint old_height, gint new_height);
  gpointer (*wrap) (void);
  void (*get_log_attrs) (void);
  void (*invalidate) (GtkTextLayout *layout, const GtkTextIter *start, const GtkTextIter *end);
  void (*free_line_data) (void);
  void (*allocate_child) (GtkTextLayout *layout, GtkWidget *child, gint x, gint y);
  void (*invalidate_cursors) (GtkTextLayout *layout, const GtkTextIter *start, const GtkTextIter *end);
} GtkTextLayoutClass;

GType          gtk_text_layout_get_type (void) G_GNUC_CONST;
GtkTextLayout *gtk_text_layout_new (void);
void           gtk_text_layout_set_buffer (GtkTextLayout *layout, GtkTextBuffer *buffer);
GtkTextBuffer *gtk_text_layout_get_buffer (GtkTextLayout *layout);
void           gtk_text_layout_set_default_style (GtkTextLayout *layout, GtkTextAttributes *values);
void           gtk_text_layout_set_contexts (GtkTextLayout *layout, PangoContext *ltr_context, PangoContext *rtl_context);
void           gtk_text_layout_set_cursor_direction (GtkTextLayout *layout, GtkTextDirection direction);
void           gtk_text_layout_set_overwrite_mode (GtkTextLayout *layout, gboolean overwrite);
void           gtk_text_layout_set_keyboard_direction (GtkTextLayout *layout, GtkTextDirection keyboard_dir);
void           gtk_text_layout_default_style_changed (GtkTextLayout *layout);
void           gtk_text_layout_set_screen_width (GtkTextLayout *layout, gint width);
void           gtk_text_layout_set_preedit_string (GtkTextLayout *layout, const gchar *preedit_string,
                                                   PangoAttrList *preedit_attrs, gint cursor_pos);
void           gtk_text_layout_set_cursor_visible (GtkTextLayout *layout, gboolean cursor_visible);
gboolean       gtk_text_layout_get_cursor_visible (GtkTextLayout *layout);
void           gtk_text_layout_get_size (GtkTextLayout *layout, gint *width, gint *height);
void           gtk_text_layout_wrap_loop_start (GtkTextLayout *layout);
void           gtk_text_layout_wrap_loop_end (GtkTextLayout *layout);
gboolean       gtk_text_layout_is_valid (GtkTextLayout *layout);
void           gtk_text_layout_validate_yrange (GtkTextLayout *layout, GtkTextIter *anchor_line, gint y0_, gint y1_);
void           gtk_text_layout_validate (GtkTextLayout *layout, gint max_pixels);
void           gtk_text_layout_invalidate (GtkTextLayout *layout, const GtkTextIter *start, const GtkTextIter *end);
void           gtk_text_layout_invalidate_cursors (GtkTextLayout *layout, const GtkTextIter *start, const GtkTextIter *end);
void           gtk_text_layout_get_iter_at_pixel (GtkTextLayout *layout, GtkTextIter *iter, gint x, gint y);
void           gtk_text_layout_get_iter_at_position (GtkTextLayout *layout, GtkTextIter *iter, gint *trailing, gint x, gint y);
void           gtk_text_layout_get_iter_at_line (GtkTextLayout *layout, GtkTextIter *iter, gpointer line, gint byte_offset);
void           gtk_text_layout_get_iter_location (GtkTextLayout *layout, const GtkTextIter *iter, GdkRectangle *rect);
void           gtk_text_layout_get_line_yrange (GtkTextLayout *layout, const GtkTextIter *iter, gint *y, gint *height);
void           gtk_text_layout_get_cursor_locations (GtkTextLayout *layout, GtkTextIter *iter, GdkRectangle *strong_pos,
                                                     GdkRectangle *weak_pos);
gboolean       gtk_text_layout_clamp_iter_to_vrange (GtkTextLayout *layout, GtkTextIter *iter, gint top, gint bottom);
gboolean       gtk_text_layout_move_iter_to_line_end (GtkTextLayout *layout, GtkTextIter *iter, gint direction);
gboolean       gtk_text_layout_move_iter_to_previous_line (GtkTextLayout *layout, GtkTextIter *iter);
gboolean       gtk_text_layout_move_iter_to_next_line (GtkTextLayout *layout, GtkTextIter *iter);
void           gtk_text_layout_move_iter_to_x (GtkTextLayout *layout, GtkTextIter *iter, gint x);
gboolean       gtk_text_layout_move_iter_visually (GtkTextLayout *layout, GtkTextIter *iter, gint count);
gboolean       gtk_text_layout_iter_starts_line (GtkTextLayout *layout, const GtkTextIter *iter);
void           gtk_text_layout_spew (GtkTextLayout *layout);

/* gtk/gtktextdisplay.h: the layout's area (x, y, width, height, in layout
 * coordinates) on a drawable, the layout's origin at x_offset, y_offset */
void           gtk_text_layout_draw (GtkTextLayout *layout, GtkWidget *widget, GdkDrawable *drawable, GdkGC *cursor_gc,
                                     gint x_offset, gint y_offset, gint x, gint y, gint width, gint height, GList **widgets);

G_END_DECLS
#if !defined (GTKQT_COMPILATION) || defined (GTKQT_EXPORT_API)
#pragma GCC visibility pop
#endif

#endif
