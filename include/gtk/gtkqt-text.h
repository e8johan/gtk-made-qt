/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Multi-line text: GtkTextBuffer (a QTextDocument) and GtkTextView.
 */
#ifndef GTKQT_TEXT_H
#define GTKQT_TEXT_H

#include <gtk/gtkqt-containers.h>

G_BEGIN_DECLS

typedef struct _GtkTextBuffer GtkTextBuffer;
typedef struct _GtkTextBufferClass GtkTextBufferClass;
typedef struct _GtkTextTag GtkTextTag;
typedef struct _GtkTextTagTable GtkTextTagTable;
typedef struct _GtkTextMark GtkTextMark;
typedef struct _GtkIMContext GtkIMContext;

/* GtkTextIter lives on the stack in user code, so its size is public. */
typedef struct _GtkTextIter {
  /*< private >*/
  gpointer buffer;
  gint offset;
  gint dummy[10];
  gpointer dummy_ptr[4];
} GtkTextIter;

struct _GtkTextBuffer { GObject parent_instance; gpointer priv; };
struct _GtkTextBufferClass {
  GObjectClass parent_class;
  void (*insert_text) (GtkTextBuffer *buffer, GtkTextIter *pos, const gchar *text, gint length);
  void (*insert_pixbuf) (GtkTextBuffer *buffer, GtkTextIter *pos, gpointer pixbuf);
  void (*insert_child_anchor) (GtkTextBuffer *buffer, GtkTextIter *pos, gpointer anchor);
  void (*delete_range) (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end);
  void (*changed) (GtkTextBuffer *buffer);
  void (*modified_changed) (GtkTextBuffer *buffer);
  void (*mark_set) (GtkTextBuffer *buffer, const GtkTextIter *location, GtkTextMark *mark);
  void (*mark_deleted) (GtkTextBuffer *buffer, GtkTextMark *mark);
  void (*apply_tag) (GtkTextBuffer *buffer, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end);
  void (*remove_tag) (GtkTextBuffer *buffer, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end);
  void (*begin_user_action) (GtkTextBuffer *buffer);
  void (*end_user_action) (GtkTextBuffer *buffer);
};

#define GTK_TYPE_TEXT_BUFFER  (gtk_text_buffer_get_type ())
#define GTK_TEXT_BUFFER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TEXT_BUFFER, GtkTextBuffer))
#define GTK_IS_TEXT_BUFFER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TEXT_BUFFER))
#define GTK_TYPE_TEXT_TAG     (gtk_text_tag_get_type ())
#define GTK_TEXT_TAG(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TEXT_TAG, GtkTextTag))
#define GTK_IS_TEXT_TAG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TEXT_TAG))
#define GTK_TYPE_TEXT_ITER    (gtk_text_iter_get_type ())
GType          gtk_text_buffer_get_type (void) G_GNUC_CONST;
GType          gtk_text_tag_get_type (void) G_GNUC_CONST;
GType          gtk_text_mark_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_TEXT_MARK    (gtk_text_mark_get_type ())
GType          gtk_text_iter_get_type (void) G_GNUC_CONST;

GtkTextBuffer *gtk_text_buffer_new (GtkTextTagTable *table);
gint           gtk_text_buffer_get_line_count (GtkTextBuffer *buffer);
gint           gtk_text_buffer_get_char_count (GtkTextBuffer *buffer);
void           gtk_text_buffer_set_text (GtkTextBuffer *buffer, const gchar *text, gint len);
gchar         *gtk_text_buffer_get_text (GtkTextBuffer *buffer, const GtkTextIter *start, const GtkTextIter *end, gboolean include_hidden_chars);
gchar         *gtk_text_buffer_get_slice (GtkTextBuffer *buffer, const GtkTextIter *start, const GtkTextIter *end, gboolean include_hidden_chars);
void           gtk_text_buffer_insert (GtkTextBuffer *buffer, GtkTextIter *iter, const gchar *text, gint len);
void           gtk_text_buffer_insert_at_cursor (GtkTextBuffer *buffer, const gchar *text, gint len);
void           gtk_text_buffer_insert_with_tags (GtkTextBuffer *buffer, GtkTextIter *iter, const gchar *text, gint len, GtkTextTag *first_tag, ...);
void           gtk_text_buffer_insert_with_tags_by_name (GtkTextBuffer *buffer, GtkTextIter *iter, const gchar *text, gint len, const gchar *first_tag_name, ...);
void           gtk_text_buffer_delete (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end);
void           gtk_text_buffer_get_start_iter (GtkTextBuffer *buffer, GtkTextIter *iter);
void           gtk_text_buffer_get_end_iter (GtkTextBuffer *buffer, GtkTextIter *iter);
void           gtk_text_buffer_get_bounds (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end);
void           gtk_text_buffer_get_iter_at_offset (GtkTextBuffer *buffer, GtkTextIter *iter, gint char_offset);
void           gtk_text_buffer_get_iter_at_line (GtkTextBuffer *buffer, GtkTextIter *iter, gint line_number);
void           gtk_text_buffer_get_iter_at_line_offset (GtkTextBuffer *buffer, GtkTextIter *iter, gint line_number, gint char_offset);
void           gtk_text_buffer_get_iter_at_mark (GtkTextBuffer *buffer, GtkTextIter *iter, GtkTextMark *mark);
GtkTextMark   *gtk_text_buffer_get_insert (GtkTextBuffer *buffer);
GtkTextMark   *gtk_text_buffer_get_selection_bound (GtkTextBuffer *buffer);
GtkTextMark   *gtk_text_buffer_create_mark (GtkTextBuffer *buffer, const gchar *mark_name, const GtkTextIter *where, gboolean left_gravity);
void           gtk_text_buffer_move_mark (GtkTextBuffer *buffer, GtkTextMark *mark, const GtkTextIter *where);
void           gtk_text_buffer_place_cursor (GtkTextBuffer *buffer, const GtkTextIter *where);
void           gtk_text_buffer_select_range (GtkTextBuffer *buffer, const GtkTextIter *ins, const GtkTextIter *bound);
gboolean       gtk_text_buffer_get_selection_bounds (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end);
gboolean       gtk_text_buffer_delete_selection (GtkTextBuffer *buffer, gboolean interactive, gboolean default_editable);
gboolean       gtk_text_buffer_get_has_selection (GtkTextBuffer *buffer);
gboolean       gtk_text_buffer_get_modified (GtkTextBuffer *buffer);
void           gtk_text_buffer_set_modified (GtkTextBuffer *buffer, gboolean setting);
GtkTextTag    *gtk_text_buffer_create_tag (GtkTextBuffer *buffer, const gchar *tag_name, const gchar *first_property_name, ...);
void           gtk_text_buffer_apply_tag (GtkTextBuffer *buffer, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end);
void           gtk_text_buffer_apply_tag_by_name (GtkTextBuffer *buffer, const gchar *name, const GtkTextIter *start, const GtkTextIter *end);
void           gtk_text_buffer_remove_all_tags (GtkTextBuffer *buffer, const GtkTextIter *start, const GtkTextIter *end);
void           gtk_text_buffer_begin_user_action (GtkTextBuffer *buffer);
void           gtk_text_buffer_end_user_action (GtkTextBuffer *buffer);
void           gtk_text_buffer_undo (GtkTextBuffer *buffer);
void           gtk_text_buffer_redo (GtkTextBuffer *buffer);
void           gtk_text_buffer_cut_clipboard (GtkTextBuffer *buffer, gpointer clipboard, gboolean default_editable);
void           gtk_text_buffer_copy_clipboard (GtkTextBuffer *buffer, gpointer clipboard);
void           gtk_text_buffer_move_mark_by_name (GtkTextBuffer *buffer, const gchar *name, const GtkTextIter *where);
GtkTextMark   *gtk_text_buffer_get_mark (GtkTextBuffer *buffer, const gchar *name);
void           gtk_text_buffer_delete_mark (GtkTextBuffer *buffer, GtkTextMark *mark);
void           gtk_text_buffer_delete_mark_by_name (GtkTextBuffer *buffer, const gchar *name);
void           gtk_text_buffer_remove_tag (GtkTextBuffer *buffer, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end);
void           gtk_text_buffer_remove_tag_by_name (GtkTextBuffer *buffer, const gchar *name, const GtkTextIter *start, const GtkTextIter *end);
gboolean       gtk_text_buffer_insert_interactive (GtkTextBuffer *buffer, GtkTextIter *iter, const gchar *text, gint len, gboolean default_editable);
gboolean       gtk_text_buffer_insert_interactive_at_cursor (GtkTextBuffer *buffer, const gchar *text, gint len, gboolean default_editable);
gboolean       gtk_text_buffer_delete_interactive (GtkTextBuffer *buffer, GtkTextIter *start_iter, GtkTextIter *end_iter, gboolean default_editable);
void           gtk_text_buffer_get_iter_at_line_index (GtkTextBuffer *buffer, GtkTextIter *iter, gint line_number, gint byte_index);
void           gtk_text_buffer_add_selection_clipboard (GtkTextBuffer *buffer, gpointer clipboard);
void           gtk_text_buffer_remove_selection_clipboard (GtkTextBuffer *buffer, gpointer clipboard);
const gchar   *gtk_text_mark_get_name (GtkTextMark *mark);
void           gtk_text_mark_set_visible (GtkTextMark *mark, gboolean setting);
gboolean       gtk_text_mark_get_visible (GtkTextMark *mark);
gboolean       gtk_text_mark_get_deleted (GtkTextMark *mark);
GtkTextBuffer *gtk_text_mark_get_buffer (GtkTextMark *mark);
gboolean       gtk_text_mark_get_left_gravity (GtkTextMark *mark);
void           gtk_text_buffer_paste_clipboard (GtkTextBuffer *buffer, gpointer clipboard, GtkTextIter *override_location, gboolean default_editable);

GtkTextBuffer *gtk_text_iter_get_buffer (const GtkTextIter *iter);
GtkTextIter   *gtk_text_iter_copy (const GtkTextIter *iter);
void           gtk_text_iter_free (GtkTextIter *iter);
gint           gtk_text_iter_get_offset (const GtkTextIter *iter);
void           gtk_text_iter_set_offset (GtkTextIter *iter, gint char_offset);
gint           gtk_text_iter_get_line (const GtkTextIter *iter);
gint           gtk_text_iter_get_line_offset (const GtkTextIter *iter);
gunichar       gtk_text_iter_get_char (const GtkTextIter *iter);
gchar         *gtk_text_iter_get_text (const GtkTextIter *start, const GtkTextIter *end);
gboolean       gtk_text_iter_forward_char (GtkTextIter *iter);
gboolean       gtk_text_iter_backward_char (GtkTextIter *iter);
gboolean       gtk_text_iter_forward_chars (GtkTextIter *iter, gint count);
gboolean       gtk_text_iter_backward_chars (GtkTextIter *iter, gint count);
gboolean       gtk_text_iter_forward_line (GtkTextIter *iter);
gboolean       gtk_text_iter_backward_line (GtkTextIter *iter);
gboolean       gtk_text_iter_forward_to_line_end (GtkTextIter *iter);
gboolean       gtk_text_iter_forward_word_end (GtkTextIter *iter);
gboolean       gtk_text_iter_backward_word_start (GtkTextIter *iter);
void           gtk_text_iter_forward_to_end (GtkTextIter *iter);
gboolean       gtk_text_iter_is_start (const GtkTextIter *iter);
gboolean       gtk_text_iter_is_end (const GtkTextIter *iter);
gboolean       gtk_text_iter_starts_line (const GtkTextIter *iter);
gboolean       gtk_text_iter_ends_line (const GtkTextIter *iter);
gchar         *gtk_text_iter_get_slice (const GtkTextIter *start, const GtkTextIter *end);
gchar         *gtk_text_iter_get_visible_slice (const GtkTextIter *start, const GtkTextIter *end);
gchar         *gtk_text_iter_get_visible_text (const GtkTextIter *start, const GtkTextIter *end);
gboolean       gtk_text_iter_in_range (const GtkTextIter *iter, const GtkTextIter *start, const GtkTextIter *end);
void           gtk_text_iter_order (GtkTextIter *first, GtkTextIter *second);
void           gtk_text_iter_set_line (GtkTextIter *iter, gint line_number);
void           gtk_text_iter_set_line_offset (GtkTextIter *iter, gint char_on_line);
void           gtk_text_iter_set_visible_line_offset (GtkTextIter *iter, gint char_on_line);
gint           gtk_text_iter_get_visible_line_offset (const GtkTextIter *iter);
gint           gtk_text_iter_get_chars_in_line (const GtkTextIter *iter);
gint           gtk_text_iter_get_line_index (const GtkTextIter *iter);
gboolean       gtk_text_iter_forward_lines (GtkTextIter *iter, gint count);
gboolean       gtk_text_iter_backward_lines (GtkTextIter *iter, gint count);
gboolean       gtk_text_iter_forward_cursor_position (GtkTextIter *iter);
gboolean       gtk_text_iter_backward_cursor_position (GtkTextIter *iter);
gboolean       gtk_text_iter_forward_word_ends (GtkTextIter *iter, gint count);
gboolean       gtk_text_iter_backward_word_starts (GtkTextIter *iter, gint count);
gboolean       gtk_text_iter_starts_word (const GtkTextIter *iter);
gboolean       gtk_text_iter_ends_word (const GtkTextIter *iter);
gboolean       gtk_text_iter_inside_word (const GtkTextIter *iter);
gboolean       gtk_text_iter_is_cursor_position (const GtkTextIter *iter);
typedef gboolean (*GtkTextCharPredicate) (gunichar ch, gpointer user_data);
gboolean       gtk_text_iter_forward_find_char (GtkTextIter *iter, GtkTextCharPredicate pred, gpointer user_data, const GtkTextIter *limit);
gboolean       gtk_text_iter_backward_find_char (GtkTextIter *iter, GtkTextCharPredicate pred, gpointer user_data, const GtkTextIter *limit);
gboolean       gtk_text_iter_editable (const GtkTextIter *iter, gboolean default_setting);
gboolean       gtk_text_iter_can_insert (const GtkTextIter *iter, gboolean default_editability);
gboolean       gtk_text_iter_equal (const GtkTextIter *lhs, const GtkTextIter *rhs);
gint           gtk_text_iter_compare (const GtkTextIter *lhs, const GtkTextIter *rhs);
typedef enum { GTK_TEXT_SEARCH_VISIBLE_ONLY = 1 << 0, GTK_TEXT_SEARCH_TEXT_ONLY = 1 << 1,
               GTK_TEXT_SEARCH_CASE_INSENSITIVE = 1 << 2 } GtkTextSearchFlags;
gboolean       gtk_text_iter_forward_search (const GtkTextIter *iter, const gchar *str, GtkTextSearchFlags flags,
                                             GtkTextIter *match_start, GtkTextIter *match_end, const GtkTextIter *limit);
gboolean       gtk_text_iter_backward_search (const GtkTextIter *iter, const gchar *str, GtkTextSearchFlags flags,
                                              GtkTextIter *match_start, GtkTextIter *match_end, const GtkTextIter *limit);

/* GtkTextView. The GTK 2 public fields programs read are kept current. */
#define GTK_TEXT_UNKNOWN_CHAR 0xFFFC
typedef enum {
  GTK_TEXT_WINDOW_PRIVATE, GTK_TEXT_WINDOW_WIDGET, GTK_TEXT_WINDOW_TEXT, GTK_TEXT_WINDOW_LEFT,
  GTK_TEXT_WINDOW_RIGHT, GTK_TEXT_WINDOW_TOP, GTK_TEXT_WINDOW_BOTTOM
} GtkTextWindowType;
typedef struct _GtkTextView {
  GtkContainer parent_instance;
  GtkTextBuffer *buffer;
  GtkIMContext *im_context;
  gboolean editable, overwrite_mode, cursor_visible, accepts_tab;
  GtkWrapMode wrap_mode;
  GtkJustification justify;
  gint left_margin, right_margin, indent, pixels_above_lines, pixels_below_lines, pixels_inside_wrap;
  gint xoffset, yoffset, width, height;
  /*< private >*/
  gpointer priv;
} GtkTextView;
typedef struct _GtkTextViewClass {
  GtkContainerClass parent_class;
  void (*set_scroll_adjustments) (GtkTextView *text_view, GtkAdjustment *hadjustment, GtkAdjustment *vadjustment);
  void (*populate_popup) (GtkTextView *text_view, GtkWidget *menu);
  void (*move_cursor) (GtkTextView *text_view, gint step, gint count, gboolean extend_selection);
  void (*page_horizontally) (GtkTextView *text_view, gint count, gboolean extend_selection);
  void (*set_anchor) (GtkTextView *text_view);
  void (*insert_at_cursor) (GtkTextView *text_view, const gchar *str);
  void (*delete_from_cursor) (GtkTextView *text_view, gint type, gint count);
  void (*backspace) (GtkTextView *text_view);
  void (*cut_clipboard) (GtkTextView *text_view);
  void (*copy_clipboard) (GtkTextView *text_view);
  void (*paste_clipboard) (GtkTextView *text_view);
  void (*toggle_overwrite) (GtkTextView *text_view);
} GtkTextViewClass;
#define GTK_TYPE_TEXT_VIEW  (gtk_text_view_get_type ())
#define GTK_TEXT_VIEW(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TEXT_VIEW, GtkTextView))
#define GTK_IS_TEXT_VIEW(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TEXT_VIEW))
GType          gtk_text_view_get_type (void) G_GNUC_CONST;
GtkWidget     *gtk_text_view_new (void);
GtkWidget     *gtk_text_view_new_with_buffer (GtkTextBuffer *buffer);
void           gtk_text_view_set_buffer (GtkTextView *text_view, GtkTextBuffer *buffer);
GtkTextBuffer *gtk_text_view_get_buffer (GtkTextView *text_view);
void           gtk_text_view_set_editable (GtkTextView *text_view, gboolean setting);
gboolean       gtk_text_view_get_editable (GtkTextView *text_view);
void           gtk_text_view_set_wrap_mode (GtkTextView *text_view, GtkWrapMode wrap_mode);
void           gtk_text_view_set_cursor_visible (GtkTextView *text_view, gboolean setting);
void           gtk_text_view_set_monospace (GtkTextView *text_view, gboolean monospace);
void           gtk_text_view_set_justification (GtkTextView *text_view, GtkJustification justification);
void           gtk_text_view_set_left_margin (GtkTextView *text_view, gint left_margin);
void           gtk_text_view_set_right_margin (GtkTextView *text_view, gint right_margin);
void           gtk_text_view_set_top_margin (GtkTextView *text_view, gint top_margin);
void           gtk_text_view_set_bottom_margin (GtkTextView *text_view, gint bottom_margin);
void           gtk_text_view_set_pixels_above_lines (GtkTextView *text_view, gint pixels);
void           gtk_text_view_set_pixels_below_lines (GtkTextView *text_view, gint pixels);
void           gtk_text_view_set_accepts_tab (GtkTextView *text_view, gboolean accepts_tab);
void           gtk_text_view_scroll_to_mark (GtkTextView *text_view, GtkTextMark *mark, gdouble within_margin,
                                             gboolean use_align, gdouble xalign, gdouble yalign);
gboolean       gtk_text_view_scroll_to_iter (GtkTextView *text_view, GtkTextIter *iter, gdouble within_margin,
                                             gboolean use_align, gdouble xalign, gdouble yalign);

GdkWindow     *gtk_text_view_get_window (GtkTextView *text_view, GtkTextWindowType win);
GtkTextWindowType gtk_text_view_get_window_type (GtkTextView *text_view, GdkWindow *window);
void           gtk_text_view_set_border_window_size (GtkTextView *text_view, GtkTextWindowType type, gint size);
gint           gtk_text_view_get_border_window_size (GtkTextView *text_view, GtkTextWindowType type);
void           gtk_text_view_buffer_to_window_coords (GtkTextView *text_view, GtkTextWindowType win, gint buffer_x, gint buffer_y,
                                                      gint *window_x, gint *window_y);
void           gtk_text_view_window_to_buffer_coords (GtkTextView *text_view, GtkTextWindowType win, gint window_x, gint window_y,
                                                      gint *buffer_x, gint *buffer_y);
void           gtk_text_view_get_visible_rect (GtkTextView *text_view, GdkRectangle *visible_rect);
void           gtk_text_view_get_iter_location (GtkTextView *text_view, const GtkTextIter *iter, GdkRectangle *location);
void           gtk_text_view_get_iter_at_location (GtkTextView *text_view, GtkTextIter *iter, gint x, gint y);
void           gtk_text_view_get_line_at_y (GtkTextView *text_view, GtkTextIter *target_iter, gint y, gint *line_top);
void           gtk_text_view_get_line_yrange (GtkTextView *text_view, const GtkTextIter *iter, gint *y, gint *height);
gboolean       gtk_text_view_move_mark_onscreen (GtkTextView *text_view, GtkTextMark *mark);
gboolean       gtk_text_view_place_cursor_onscreen (GtkTextView *text_view);
void           gtk_text_view_scroll_mark_onscreen (GtkTextView *text_view, GtkTextMark *mark);
void           gtk_text_view_set_tabs (GtkTextView *text_view, PangoTabArray *tabs);
PangoTabArray *gtk_text_view_get_tabs (GtkTextView *text_view);
void           gtk_text_view_set_overwrite (GtkTextView *text_view, gboolean overwrite);
gboolean       gtk_text_view_get_overwrite (GtkTextView *text_view);
GtkWrapMode    gtk_text_view_get_wrap_mode (GtkTextView *text_view);
gboolean       gtk_text_view_get_cursor_visible (GtkTextView *text_view);
gint           gtk_text_view_get_left_margin (GtkTextView *text_view);
gint           gtk_text_view_get_right_margin (GtkTextView *text_view);
void           gtk_text_view_set_indent (GtkTextView *text_view, gint indent);

G_END_DECLS

#endif
