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
  void (*changed) (GtkTextBuffer *buffer);
  void (*modified_changed) (GtkTextBuffer *buffer);
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
gboolean       gtk_text_iter_equal (const GtkTextIter *lhs, const GtkTextIter *rhs);
gint           gtk_text_iter_compare (const GtkTextIter *lhs, const GtkTextIter *rhs);
typedef enum { GTK_TEXT_SEARCH_VISIBLE_ONLY = 1 << 0, GTK_TEXT_SEARCH_TEXT_ONLY = 1 << 1,
               GTK_TEXT_SEARCH_CASE_INSENSITIVE = 1 << 2 } GtkTextSearchFlags;
gboolean       gtk_text_iter_forward_search (const GtkTextIter *iter, const gchar *str, GtkTextSearchFlags flags,
                                             GtkTextIter *match_start, GtkTextIter *match_end, const GtkTextIter *limit);
gboolean       gtk_text_iter_backward_search (const GtkTextIter *iter, const gchar *str, GtkTextSearchFlags flags,
                                              GtkTextIter *match_start, GtkTextIter *match_end, const GtkTextIter *limit);

/* GtkTextView */
GTKQT_SIMPLE_TYPE (GtkTextView, GtkContainer)
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

G_END_DECLS

#endif
