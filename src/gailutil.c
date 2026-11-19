/*
 * GTK made Qt - GTK 2's gail-util on the real ATK: the text segments and
 * attributes accessible text objects report (libgnomecanvas's GailCanvasText
 * uses them). Built when ATK is available; programs get it with
 * pkg-config gail.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include <libgail-util/gail-util.h>
#include <string.h>

G_DEFINE_TYPE (GailTextUtil, gail_text_util, G_TYPE_OBJECT)

static void gail_text_util_init (GailTextUtil *t) { t->buffer = NULL; }

static void text_util_finalize (GObject *o)
{
  GailTextUtil *t = GAIL_TEXT_UTIL (o);
  if (t->buffer) g_object_unref (t->buffer);
  G_OBJECT_CLASS (gail_text_util_parent_class)->finalize (o);
}

static void gail_text_util_class_init (GailTextUtilClass *klass) { G_OBJECT_CLASS (klass)->finalize = text_util_finalize; }

GailTextUtil *gail_text_util_new (void) { return g_object_new (GAIL_TYPE_TEXT_UTIL, NULL); }

void gail_text_util_text_setup (GailTextUtil *t, const gchar *text)
{
  if (!t->buffer) t->buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_set_text (t->buffer, text ? text : "", -1);
}

void gail_text_util_buffer_setup (GailTextUtil *t, GtkTextBuffer *buffer)
{
  if (buffer) g_object_ref (buffer);
  if (t->buffer) g_object_unref (t->buffer);
  t->buffer = buffer;
}

static gchar *buffer_text (GtkTextBuffer *b)
{
  GtkTextIter s, e;
  gtk_text_buffer_get_bounds (b, &s, &e);
  return gtk_text_buffer_get_slice (b, &s, &e, TRUE);
}

/* Is there a boundary of this kind before character i (0..n)? */
static gboolean is_boundary (AtkTextBoundary type, const PangoLogAttr *attrs, const gchar *text, int n, int i,
                             const int *line_starts, int n_lines)
{
  if (i <= 0 || i >= n) return TRUE;
  switch (type) {
  case ATK_TEXT_BOUNDARY_CHAR: return attrs[i].is_cursor_position;
  case ATK_TEXT_BOUNDARY_WORD_START: return attrs[i].is_word_start;
  case ATK_TEXT_BOUNDARY_WORD_END: return attrs[i].is_word_end;
  case ATK_TEXT_BOUNDARY_SENTENCE_START: return attrs[i].is_sentence_start;
  case ATK_TEXT_BOUNDARY_SENTENCE_END: return attrs[i].is_sentence_end;
  case ATK_TEXT_BOUNDARY_LINE_START:
  case ATK_TEXT_BOUNDARY_LINE_END: {
    for (int l = 0; l < n_lines; l++) {
      int b = line_starts[l];
      /* a line start is after its newline, a line end before it */
      if (type == ATK_TEXT_BOUNDARY_LINE_START && b == i) return TRUE;
      if (type == ATK_TEXT_BOUNDARY_LINE_END && b > 0 && i == b - 1 &&
          g_utf8_get_char (g_utf8_offset_to_pointer (text, b - 1)) == '\n') return TRUE;
      if (type == ATK_TEXT_BOUNDARY_LINE_END && b > 0 && i == b &&
          g_utf8_get_char (g_utf8_offset_to_pointer (text, b - 1)) != '\n') return TRUE;   /* a wrapped line */
    }
    return FALSE;
  }
  default: return FALSE;
  }
}

gchar *gail_text_util_get_text (GailTextUtil *t, gpointer layout, GailOffsetType function, AtkTextBoundary type,
                                gint offset, gint *start_offset, gint *end_offset)
{
  if (start_offset) *start_offset = 0;
  if (end_offset) *end_offset = 0;
  if (!t || !t->buffer) return g_strdup ("");
  gchar *text = buffer_text (t->buffer);
  int n = g_utf8_strlen (text, -1);
  PangoLogAttr *attrs = g_new0 (PangoLogAttr, n + 1);
  pango_get_log_attrs (text, -1, -1, pango_language_get_default (), attrs, n + 1);

  /* line starts: the layout's lines (wrapped text), else after each newline */
  GArray *starts = g_array_new (FALSE, FALSE, sizeof (int));
  int zero = 0;
  g_array_append_val (starts, zero);
  if (layout && PANGO_IS_LAYOUT (layout)) {
    const char *ltext = pango_layout_get_text (layout);
    for (GSList *l = pango_layout_get_lines_readonly (layout); l; l = l->next) {
      PangoLayoutLine *line = l->data;
      int c = g_utf8_pointer_to_offset (ltext, ltext + line->start_index);
      if (c > 0) g_array_append_val (starts, c);
    }
  } else {
    for (int i = 0; i < n; i++)
      if (g_utf8_get_char (g_utf8_offset_to_pointer (text, i)) == '\n') { int c = i + 1; g_array_append_val (starts, c); }
  }

  offset = CLAMP (offset, 0, n);
  int s = offset, e = offset;
#define BOUNDARY(i) is_boundary (type, attrs, text, n, (i), (const int *) starts->data, starts->len)
  /* the segment at the offset: from the boundary at or before it to the next one */
  while (s > 0 && !BOUNDARY (s)) s--;
  e = offset + 1;
  while (e < n && !BOUNDARY (e)) e++;
  if (offset >= n) { s = n; e = n; }
  if (function == GAIL_BEFORE_OFFSET) {
    e = s;
    s = MAX (0, s - 1);
    while (s > 0 && !BOUNDARY (s)) s--;
  } else if (function == GAIL_AFTER_OFFSET) {
    s = MIN (e, n);
    e = MIN (s + 1, n);
    while (e < n && !BOUNDARY (e)) e++;
  }
#undef BOUNDARY
  e = MIN (e, n);
  if (start_offset) *start_offset = s;
  if (end_offset) *end_offset = e;
  gchar *out = g_strndup (g_utf8_offset_to_pointer (text, s), g_utf8_offset_to_pointer (text, e) - g_utf8_offset_to_pointer (text, s));
  g_array_free (starts, TRUE);
  g_free (attrs);
  g_free (text);
  return out;
}

gchar *gail_text_util_get_substring (GailTextUtil *t, gint start_pos, gint end_pos)
{
  if (!t || !t->buffer) return g_strdup ("");
  GtkTextIter s, e;
  gtk_text_buffer_get_iter_at_offset (t->buffer, &s, start_pos);
  if (end_pos < 0) gtk_text_buffer_get_end_iter (t->buffer, &e);
  else gtk_text_buffer_get_iter_at_offset (t->buffer, &e, end_pos);
  return gtk_text_buffer_get_text (t->buffer, &s, &e, FALSE);
}

/* ---------- attributes ---------- */
AtkAttributeSet *gail_misc_add_attribute (AtkAttributeSet *set, AtkTextAttribute attr, gchar *value)
{
  AtkAttribute *at = g_new (AtkAttribute, 1);
  at->name = g_strdup (atk_text_attribute_get_name (attr));
  at->value = value;
  return g_slist_prepend (set, at);
}

static AtkAttributeSet *add_font (AtkAttributeSet *set, const PangoFontDescription *f)
{
  if (!f) return set;
  PangoFontMask m = pango_font_description_get_set_fields (f);
  if (m & PANGO_FONT_MASK_FAMILY) set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_FAMILY_NAME, g_strdup (pango_font_description_get_family (f)));
  if (m & PANGO_FONT_MASK_SIZE)
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_SIZE, g_strdup_printf ("%i", pango_font_description_get_size (f) / PANGO_SCALE));
  if (m & PANGO_FONT_MASK_WEIGHT)
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_WEIGHT, g_strdup_printf ("%d", pango_font_description_get_weight (f)));
  if (m & PANGO_FONT_MASK_STYLE)
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_STYLE,
                                   g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_STYLE, pango_font_description_get_style (f))));
  return set;
}

AtkAttributeSet *gail_misc_layout_get_run_attributes (AtkAttributeSet *set, PangoLayout *layout, gchar *text, gint offset,
                                                     gint *start_offset, gint *end_offset)
{
  if (start_offset) *start_offset = 0;
  if (end_offset) *end_offset = text ? g_utf8_strlen (text, -1) : 0;
  PangoAttrList *list = layout ? pango_layout_get_attributes (layout) : NULL;
  if (!list || !text) return set;
  int index = g_utf8_offset_to_pointer (text, offset) - text;
  PangoAttrIterator *it = pango_attr_list_get_iterator (list);
  int s = 0, e = 0;
  do {
    pango_attr_iterator_range (it, &s, &e);
    if (index >= s && index < e) break;
  } while (pango_attr_iterator_next (it));
  if (start_offset) *start_offset = g_utf8_pointer_to_offset (text, text + MIN (s, (int) strlen (text)));
  if (end_offset) *end_offset = g_utf8_pointer_to_offset (text, text + MIN (e, (int) strlen (text)));
  PangoFontDescription *f = pango_font_description_new ();
  pango_attr_iterator_get_font (it, f, NULL, NULL);
  set = add_font (set, f);
  pango_font_description_free (f);
  PangoAttribute *a;
  if ((a = pango_attr_iterator_get (it, PANGO_ATTR_UNDERLINE)))
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_UNDERLINE,
                                   g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_UNDERLINE, ((PangoAttrInt *) a)->value)));
  if ((a = pango_attr_iterator_get (it, PANGO_ATTR_STRIKETHROUGH)))
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_STRIKETHROUGH,
                                   g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_STRIKETHROUGH, ((PangoAttrInt *) a)->value)));
  if ((a = pango_attr_iterator_get (it, PANGO_ATTR_FOREGROUND))) {
    PangoColor c = ((PangoAttrColor *) a)->color;
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_FG_COLOR, g_strdup_printf ("%u,%u,%u", c.red, c.green, c.blue));
  }
  if ((a = pango_attr_iterator_get (it, PANGO_ATTR_BACKGROUND))) {
    PangoColor c = ((PangoAttrColor *) a)->color;
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_BG_COLOR, g_strdup_printf ("%u,%u,%u", c.red, c.green, c.blue));
  }
  pango_attr_iterator_destroy (it);
  return set;
}

AtkAttributeSet *gail_misc_get_default_attributes (AtkAttributeSet *set, PangoLayout *layout, GtkWidget *widget)
{
  if (layout) {
    PangoContext *ctx = pango_layout_get_context (layout);
    set = add_font (set, pango_context_get_font_description (ctx));
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_DIRECTION,
                                   g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_DIRECTION,
                                                                           widget ? gtk_widget_get_direction (widget) : 0)));
    set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_WRAP_MODE,
                                   g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_WRAP_MODE,
                                                                           pango_layout_get_wrap (layout) == PANGO_WRAP_WORD ? 2 : 1)));
  }
  set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_UNDERLINE, g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_UNDERLINE, 0)));
  set = gail_misc_add_attribute (set, ATK_TEXT_ATTR_STRIKETHROUGH, g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_STRIKETHROUGH, 0)));
  return set;
}

/* ---------- coordinates ---------- */
void gail_misc_get_origins (GtkWidget *widget, gint *x_window, gint *y_window, gint *x_toplevel, gint *y_toplevel)
{
  gint wx = 0, wy = 0, tx = 0, ty = 0;
  if (widget && widget->window) gdk_window_get_origin (widget->window, &wx, &wy);
  GtkWidget *top = widget ? gtk_widget_get_toplevel (widget) : NULL;
  if (top && top->window) gdk_window_get_origin (top->window, &tx, &ty);
  if (x_window) *x_window = wx;
  if (y_window) *y_window = wy;
  if (x_toplevel) *x_toplevel = tx;
  if (y_toplevel) *y_toplevel = ty;
}

void gail_misc_get_extents_from_pango_rectangle (GtkWidget *widget, PangoRectangle *r, gint x_layout, gint y_layout,
                                                 gint *x, gint *y, gint *width, gint *height, AtkCoordType coords)
{
  gint wx, wy, tx, ty;
  gail_misc_get_origins (widget, &wx, &wy, &tx, &ty);
  gint px = PANGO_PIXELS (r->x) + x_layout + wx, py = PANGO_PIXELS (r->y) + y_layout + wy;
  if (coords == ATK_XY_WINDOW) { px -= tx; py -= ty; }
  if (x) *x = px;
  if (y) *y = py;
  if (width) *width = PANGO_PIXELS (r->width);
  if (height) *height = PANGO_PIXELS (r->height);
}

gint gail_misc_get_index_at_point_in_layout (GtkWidget *widget, PangoLayout *layout, gint x_layout, gint y_layout,
                                             gint x, gint y, AtkCoordType coords)
{
  gint wx, wy, tx, ty;
  gail_misc_get_origins (widget, &wx, &wy, &tx, &ty);
  gint lx = x - x_layout - wx, ly = y - y_layout - wy;
  if (coords == ATK_XY_WINDOW) { lx += tx; ly += ty; }
  int index = -1, trailing = 0;
  if (!pango_layout_xy_to_index (layout, lx * PANGO_SCALE, ly * PANGO_SCALE, &index, &trailing)) return -1;
  return index + trailing;
}

/* the run of the same tags around the offset; its attributes are the
   tags' (the text view's formatting is Qt's) */
AtkAttributeSet *gail_misc_buffer_get_run_attributes (GtkTextBuffer *buffer, gint offset, gint *start_offset, gint *end_offset)
{
  GtkTextIter it, s, e;
  gtk_text_buffer_get_iter_at_offset (buffer, &it, offset);
  s = e = it;
  if (!gtk_text_iter_begins_tag (&s, NULL)) gtk_text_iter_backward_to_tag_toggle (&s, NULL);
  gtk_text_iter_forward_to_tag_toggle (&e, NULL);
  if (start_offset) *start_offset = gtk_text_iter_get_offset (&s);
  if (end_offset) *end_offset = gtk_text_iter_get_offset (&e);
  AtkAttributeSet *set = NULL;
  GSList *tags = gtk_text_iter_get_tags (&it);
  for (GSList *l = tags; l; l = l->next) {
    gchar *name = NULL;
    g_object_get (l->data, "name", &name, NULL);
    if (name) {
      AtkAttribute *at = g_new (AtkAttribute, 1);
      at->name = g_strdup ("tag");
      at->value = name;
      set = g_slist_prepend (set, at);
    }
  }
  g_slist_free (tags);
  return set;
}

/* ---------- GtkAccessible: an accessible object for a widget ---------- */
G_DEFINE_TYPE (GtkAccessible, gtk_accessible, ATK_TYPE_OBJECT)

static void gtk_accessible_init (GtkAccessible *a) { a->widget = NULL; }

static void accessible_widget_destroyed (GtkWidget *widget, GtkAccessible *a)
{
  a->widget = NULL;
  atk_object_notify_state_change (ATK_OBJECT (a), ATK_STATE_DEFUNCT, TRUE);
}

static void accessible_real_connect_widget_destroyed (GtkAccessible *a)
{
  if (a->widget) g_signal_connect (a->widget, "destroy", G_CALLBACK (accessible_widget_destroyed), a);
}

static void gtk_accessible_class_init (GtkAccessibleClass *klass) { klass->connect_widget_destroyed = accessible_real_connect_widget_destroyed; }

void gtk_accessible_connect_widget_destroyed (GtkAccessible *a)
{
  GtkAccessibleClass *k = GTK_ACCESSIBLE_GET_CLASS (a);
  if (k->connect_widget_destroyed) k->connect_widget_destroyed (a);
}

GtkWidget *gtk_accessible_get_widget (GtkAccessible *a) { return a->widget; }
void gtk_accessible_set_widget (GtkAccessible *a, GtkWidget *widget) { a->widget = widget; }
