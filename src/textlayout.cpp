/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkTextLayout: a buffer's text laid out at a width, in a default style,
 * for text items outside GtkTextView (GnomeCanvasRichText). The layout is
 * a copy of the buffer's QTextDocument (which carries the tags' formats)
 * with the default style applied: Qt's text layout answers positions and
 * lines, and draws on GdkDrawables through their cairo surfaces.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <gtk/gtkqt-textlayout.h>
#include <QtWidgets>
#include <QTextDocument>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>

struct LayoutPriv {
  QTextDocument *doc = nullptr;     /* the laid out copy */
  bool valid = false;
  QMetaObject::Connection watch;
};
static LayoutPriv *lp (GtkTextLayout *l) { return static_cast<LayoutPriv *> (l->gtkqt_priv); }

G_DEFINE_TYPE (GtkTextLayout, gtk_text_layout, G_TYPE_OBJECT)

static void gtk_text_layout_init (GtkTextLayout *l)
{
  l->gtkqt_priv = new LayoutPriv;
  l->default_style = gtk_text_attributes_new ();
  l->cursor_visible = TRUE;
}

static void text_layout_finalize (GObject *o)
{
  auto *l = GTK_TEXT_LAYOUT (o);
  LayoutPriv *p = lp (l);
  QObject::disconnect (p->watch);
  delete p->doc;
  delete p;
  if (l->buffer) g_object_unref (l->buffer);
  if (l->default_style) gtk_text_attributes_unref (l->default_style);
  if (l->ltr_context) g_object_unref (l->ltr_context);
  if (l->rtl_context) g_object_unref (l->rtl_context);
  g_free (l->preedit_string);
  if (l->preedit_attrs) pango_attr_list_unref (l->preedit_attrs);
  G_OBJECT_CLASS (gtk_text_layout_parent_class)->finalize (o);
}

static void gtk_text_layout_class_init (GtkTextLayoutClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = text_layout_finalize;
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("invalidated", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextLayoutClass, invalidated), nullptr, nullptr,
                nullptr, G_TYPE_NONE, 0);
  g_signal_new ("changed", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextLayoutClass, changed), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
  g_signal_new ("allocate-child", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextLayoutClass, allocate_child), nullptr,
                nullptr, nullptr, G_TYPE_NONE, 3, GTK_TYPE_OBJECT, G_TYPE_INT, G_TYPE_INT);
}

/* the default style on the copy: what tags leave unset */
static void apply_default_style (GtkTextLayout *l, QTextDocument *doc)
{
  GtkTextAttributes *a = l->default_style;
  QFont font = QApplication::font ();
  if (a->font) font = gtkqt_font_from_pango (a->font);
  if (a->font_scale > 0 && a->font_scale != 1.0) font.setPointSizeF (font.pointSizeF () * a->font_scale);
  doc->setDefaultFont (font);
  doc->setDocumentMargin (0);
  QTextOption opt = doc->defaultTextOption ();
  opt.setWrapMode (a->wrap_mode == GTK_WRAP_NONE ? QTextOption::NoWrap
                   : a->wrap_mode == GTK_WRAP_CHAR ? QTextOption::WrapAnywhere
                   : a->wrap_mode == GTK_WRAP_WORD ? QTextOption::WordWrap : QTextOption::WrapAtWordBoundaryOrAnywhere);
  opt.setAlignment (a->justification == GTK_JUSTIFY_RIGHT ? Qt::AlignRight
                    : a->justification == GTK_JUSTIFY_CENTER ? Qt::AlignHCenter
                    : a->justification == GTK_JUSTIFY_FILL ? Qt::AlignJustify : Qt::AlignLeft);
  if (a->direction != GTK_TEXT_DIR_NONE) opt.setTextDirection (a->direction == GTK_TEXT_DIR_RTL ? Qt::RightToLeft : Qt::LeftToRight);
  doc->setDefaultTextOption (opt);
  QTextFrameFormat ff = doc->rootFrame ()->frameFormat ();
  ff.setLeftMargin (a->left_margin);
  ff.setRightMargin (a->right_margin);
  doc->rootFrame ()->setFrameFormat (ff);
  /* paragraph spacing and indent, where a tag did not set them */
  QTextCursor c (doc);
  for (QTextBlock b = doc->begin (); b.isValid (); b = b.next ()) {
    QTextBlockFormat bf = b.blockFormat ();
    if (!bf.hasProperty (QTextFormat::BlockTopMargin)) bf.setTopMargin (a->pixels_above_lines);
    if (!bf.hasProperty (QTextFormat::BlockBottomMargin)) bf.setBottomMargin (a->pixels_below_lines);
    if (!bf.hasProperty (QTextFormat::TextIndent)) bf.setTextIndent (a->indent);
    if (a->pixels_inside_wrap > 0 && !bf.hasProperty (QTextFormat::LineHeight))
      bf.setLineHeight (a->pixels_inside_wrap, QTextBlockFormat::LineDistanceHeight);
    QTextCursor (b).setBlockFormat (bf);
  }
  if (a->appearance.fg_color.red || a->appearance.fg_color.green || a->appearance.fg_color.blue) {
    QTextCharFormat cf;
    cf.setForeground (QColor (a->appearance.fg_color.red >> 8, a->appearance.fg_color.green >> 8, a->appearance.fg_color.blue >> 8));
    c.select (QTextCursor::Document);
    /* only where tags did not colour the text */
    for (QTextBlock b = doc->begin (); b.isValid (); b = b.next ())
      for (auto it = b.begin (); !it.atEnd (); ++it)
        if (!it.fragment ().charFormat ().hasProperty (QTextFormat::ForegroundBrush)) {
          QTextCursor fc (doc);
          fc.setPosition (it.fragment ().position ());
          fc.setPosition (it.fragment ().position () + it.fragment ().length (), QTextCursor::KeepAnchor);
          fc.mergeCharFormat (cf);
        }
  }
}

/* lay the buffer out again (when it or the style changed) */
static void relayout (GtkTextLayout *l)
{
  LayoutPriv *p = lp (l);
  if (p->valid) return;
  delete p->doc;
  p->doc = nullptr;
  if (QTextDocument *src = gtkqt_text_buffer_document (l->buffer)) {
    p->doc = src->clone ();
    apply_default_style (l, p->doc);
    p->doc->setTextWidth (l->screen_width > 0 ? l->screen_width : -1);
  }
  p->valid = true;
  int old_height = l->height;
  QSizeF s = p->doc ? p->doc->size () : QSizeF ();
  l->width = (int) std::ceil (l->screen_width > 0 ? qMax ((qreal) l->screen_width, p->doc ? p->doc->idealWidth () : 0) : s.width ());
  l->height = (int) std::ceil (s.height ());
  g_signal_emit_by_name (l, "changed", 0, old_height, l->height);
}

static void invalidate (GtkTextLayout *l)
{
  if (!lp (l)->valid) return;
  lp (l)->valid = false;
  g_signal_emit_by_name (l, "invalidated");
}

static QTextDocument *ldoc (GtkTextLayout *l)
{
  relayout (l);
  return lp (l)->doc;
}

/* the line an offset is on, with its block */
static QTextLine line_at (QTextDocument *doc, int pos, QTextBlock *block)
{
  QTextBlock b = doc->findBlock (pos);
  if (block) *block = b;
  if (!b.isValid () || !b.layout ()) return QTextLine ();
  return b.layout ()->lineForTextPosition (pos - b.position ());
}

static QPointF block_origin (QTextDocument *doc, const QTextBlock &b)
{
  return doc->documentLayout ()->blockBoundingRect (b).topLeft ();
}

static void set_iter (GtkTextLayout *l, GtkTextIter *iter, int pos)
{
  int n = gtk_text_buffer_get_char_count (l->buffer);
  gtk_text_buffer_get_iter_at_offset (l->buffer, iter, qBound (0, pos, n));
}

extern "C" {

GtkTextLayout *gtk_text_layout_new (void) { return (GtkTextLayout *) g_object_new (GTK_TYPE_TEXT_LAYOUT, nullptr); }

void gtk_text_layout_set_buffer (GtkTextLayout *l, GtkTextBuffer *buffer)
{
  LayoutPriv *p = lp (l);
  QObject::disconnect (p->watch);
  if (buffer) g_object_ref (buffer);
  if (l->buffer) g_object_unref (l->buffer);
  l->buffer = buffer;
  if (QTextDocument *doc = gtkqt_text_buffer_document (buffer))
    p->watch = QObject::connect (doc, &QTextDocument::contentsChanged, doc, [l] { invalidate (l); });
  p->valid = false;
  g_signal_emit_by_name (l, "invalidated");
}

GtkTextBuffer *gtk_text_layout_get_buffer (GtkTextLayout *l) { return l->buffer; }

void gtk_text_layout_set_default_style (GtkTextLayout *l, GtkTextAttributes *values)
{
  if (!values || values == l->default_style) return;
  gtk_text_attributes_ref (values);
  gtk_text_attributes_unref (l->default_style);
  l->default_style = values;
  gtk_text_layout_default_style_changed (l);
}

void gtk_text_layout_default_style_changed (GtkTextLayout *l)
{
  lp (l)->valid = true;   /* so invalidate signals */
  invalidate (l);
}

void gtk_text_layout_set_contexts (GtkTextLayout *l, PangoContext *ltr, PangoContext *rtl)
{
  if (ltr) g_object_ref (ltr);
  if (rtl) g_object_ref (rtl);
  if (l->ltr_context) g_object_unref (l->ltr_context);
  if (l->rtl_context) g_object_unref (l->rtl_context);
  l->ltr_context = ltr;
  l->rtl_context = rtl;
  gtk_text_layout_default_style_changed (l);
}

void gtk_text_layout_set_cursor_direction (GtkTextLayout *l, GtkTextDirection d) { l->cursor_direction = d; }
void gtk_text_layout_set_overwrite_mode (GtkTextLayout *l, gboolean overwrite) { l->overwrite_mode = overwrite != FALSE; }
void gtk_text_layout_set_keyboard_direction (GtkTextLayout *l, GtkTextDirection d) { l->keyboard_direction = d; }

void gtk_text_layout_set_screen_width (GtkTextLayout *l, gint width)
{
  if (width == l->screen_width) return;
  l->screen_width = width;
  gtk_text_layout_default_style_changed (l);
}

/* text an input method is composing shows at the cursor (Qt's input
   methods handle that in Qt's own widgets) */
void gtk_text_layout_set_preedit_string (GtkTextLayout *l, const gchar *s, PangoAttrList *attrs, gint cursor_pos)
{
  g_free (l->preedit_string);
  l->preedit_string = g_strdup (s);
  if (attrs) pango_attr_list_ref (attrs);
  if (l->preedit_attrs) pango_attr_list_unref (l->preedit_attrs);
  l->preedit_attrs = attrs;
  l->preedit_len = s ? strlen (s) : 0;
  l->preedit_cursor = cursor_pos;
}

void gtk_text_layout_set_cursor_visible (GtkTextLayout *l, gboolean visible) { l->cursor_visible = visible != FALSE; }
gboolean gtk_text_layout_get_cursor_visible (GtkTextLayout *l) { return l->cursor_visible; }

void gtk_text_layout_get_size (GtkTextLayout *l, gint *width, gint *height)
{
  relayout (l);
  if (width) *width = l->width;
  if (height) *height = l->height;
}

void gtk_text_layout_wrap_loop_start (GtkTextLayout *l) { l->wrap_loop_count++; }
void gtk_text_layout_wrap_loop_end (GtkTextLayout *l) { if (l->wrap_loop_count > 0) l->wrap_loop_count--; }
gboolean gtk_text_layout_is_valid (GtkTextLayout *l) { return lp (l)->valid; }
/* the whole text is laid out at once */
void gtk_text_layout_validate_yrange (GtkTextLayout *l, GtkTextIter *, gint, gint) { relayout (l); }
void gtk_text_layout_validate (GtkTextLayout *l, gint) { relayout (l); }
void gtk_text_layout_invalidate (GtkTextLayout *l, const GtkTextIter *, const GtkTextIter *) { invalidate (l); }
void gtk_text_layout_invalidate_cursors (GtkTextLayout *, const GtkTextIter *, const GtkTextIter *) {}

void gtk_text_layout_get_iter_at_position (GtkTextLayout *l, GtkTextIter *iter, gint *trailing, gint x, gint y)
{
  QTextDocument *doc = ldoc (l);
  int pos = doc ? doc->documentLayout ()->hitTest (QPointF (x, y), Qt::FuzzyHit) : 0;
  if (trailing) *trailing = 0;
  set_iter (l, iter, qMax (0, pos));
}

void gtk_text_layout_get_iter_at_pixel (GtkTextLayout *l, GtkTextIter *iter, gint x, gint y)
{
  gtk_text_layout_get_iter_at_position (l, iter, nullptr, x, y);
}

void gtk_text_layout_get_iter_at_line (GtkTextLayout *l, GtkTextIter *iter, gpointer, gint) { set_iter (l, iter, 0); }

void gtk_text_layout_get_iter_location (GtkTextLayout *l, const GtkTextIter *iter, GdkRectangle *rect)
{
  QTextDocument *doc = ldoc (l);
  *rect = { 0, 0, 0, 0 };
  if (!doc) return;
  int pos = gtk_text_iter_get_offset (iter);
  QTextBlock b;
  QTextLine line = line_at (doc, pos, &b);
  if (!line.isValid ()) return;
  QPointF o = block_origin (doc, b);
  int rel = pos - b.position ();
  qreal x0 = line.cursorToX (rel), x1 = line.cursorToX (qMin (rel + 1, line.textStart () + line.textLength ()));
  *rect = { (int) (o.x () + x0), (int) (o.y () + line.y ()), qMax (1, (int) (x1 - x0)), (int) std::ceil (line.height ()) };
}

void gtk_text_layout_get_line_yrange (GtkTextLayout *l, const GtkTextIter *iter, gint *y, gint *height)
{
  GdkRectangle r;
  gtk_text_layout_get_iter_location (l, iter, &r);
  if (y) *y = r.y;
  if (height) *height = r.height;
}

void gtk_text_layout_get_cursor_locations (GtkTextLayout *l, GtkTextIter *iter, GdkRectangle *strong, GdkRectangle *weak)
{
  GdkRectangle r;
  gtk_text_layout_get_iter_location (l, iter, &r);
  r.width = 0;
  if (strong) *strong = r;
  if (weak) *weak = r;
}

gboolean gtk_text_layout_clamp_iter_to_vrange (GtkTextLayout *l, GtkTextIter *iter, gint top, gint bottom)
{
  GdkRectangle r;
  gtk_text_layout_get_iter_location (l, iter, &r);
  if (r.y >= top && r.y + r.height <= bottom) return FALSE;
  gtk_text_layout_get_iter_at_pixel (l, iter, r.x, r.y < top ? top : bottom - 1);
  return TRUE;
}

gboolean gtk_text_layout_iter_starts_line (GtkTextLayout *l, const GtkTextIter *iter)
{
  QTextDocument *doc = ldoc (l);
  int pos = gtk_text_iter_get_offset (iter);
  QTextBlock b;
  QTextLine line = doc ? line_at (doc, pos, &b) : QTextLine ();
  return line.isValid () && line.textStart () == pos - b.position ();
}

/* direction < 0: the start of the display line, else its end */
gboolean gtk_text_layout_move_iter_to_line_end (GtkTextLayout *l, GtkTextIter *iter, gint direction)
{
  QTextDocument *doc = ldoc (l);
  int pos = gtk_text_iter_get_offset (iter);
  QTextBlock b;
  QTextLine line = doc ? line_at (doc, pos, &b) : QTextLine ();
  if (!line.isValid ()) return FALSE;
  int to = b.position () + line.textStart ();
  if (direction >= 0) {
    to += line.textLength ();
    /* before the line's last character when it is a wrap point's space or the paragraph end */
    if (line.lineNumber () < b.layout ()->lineCount () - 1) to--;
  }
  set_iter (l, iter, to);
  return to != pos;
}

static gboolean move_lines (GtkTextLayout *l, GtkTextIter *iter, int delta)
{
  QTextDocument *doc = ldoc (l);
  int pos = gtk_text_iter_get_offset (iter);
  QTextBlock b;
  QTextLine line = doc ? line_at (doc, pos, &b) : QTextLine ();
  if (!line.isValid ()) return FALSE;
  QPointF o = block_origin (doc, b);
  qreal x = line.cursorToX (pos - b.position ()) + o.x ();
  qreal y = delta > 0 ? o.y () + line.y () + line.height () + 1 : o.y () + line.y () - 1;
  if (y < 0 || y >= doc->size ().height ()) return FALSE;
  int to = doc->documentLayout ()->hitTest (QPointF (x, y), Qt::FuzzyHit);
  if (to < 0 || to == pos) return FALSE;
  set_iter (l, iter, to);
  return TRUE;
}

gboolean gtk_text_layout_move_iter_to_previous_line (GtkTextLayout *l, GtkTextIter *iter) { return move_lines (l, iter, -1); }
gboolean gtk_text_layout_move_iter_to_next_line (GtkTextLayout *l, GtkTextIter *iter) { return move_lines (l, iter, 1); }

void gtk_text_layout_move_iter_to_x (GtkTextLayout *l, GtkTextIter *iter, gint x)
{
  QTextDocument *doc = ldoc (l);
  int pos = gtk_text_iter_get_offset (iter);
  QTextBlock b;
  QTextLine line = doc ? line_at (doc, pos, &b) : QTextLine ();
  if (!line.isValid ()) return;
  QPointF o = block_origin (doc, b);
  set_iter (l, iter, b.position () + line.xToCursor (x - o.x ()));
}

/* by characters in display order (left to right for left-to-right text) */
gboolean gtk_text_layout_move_iter_visually (GtkTextLayout *l, GtkTextIter *iter, gint count)
{
  QTextDocument *doc = ldoc (l);
  if (!doc) return FALSE;
  QTextCursor c (doc);
  c.setPosition (gtk_text_iter_get_offset (iter));
  bool ok = c.movePosition (count > 0 ? QTextCursor::Right : QTextCursor::Left, QTextCursor::MoveAnchor, qAbs (count));
  set_iter (l, iter, c.position ());
  return ok && !gtk_text_iter_is_end (iter);
}

void gtk_text_layout_spew (GtkTextLayout *l)
{
  relayout (l);
  g_print ("GtkTextLayout %p: %d x %d at width %d\n", (void *) l, l->width, l->height, l->screen_width);
}

void gtk_text_layout_draw (GtkTextLayout *l, GtkWidget *, GdkDrawable *drawable, GdkGC *, gint x_offset, gint y_offset, gint x,
                           gint y, gint width, gint height, GList **widgets)
{
  if (widgets) *widgets = nullptr;
  QTextDocument *doc = ldoc (l);
  cairo_surface_t *target = gtkqt_drawable_target (drawable);
  if (!doc || !target || width <= 0 || height <= 0) return;
  QImage img (width, height, QImage::Format_ARGB32_Premultiplied);
  img.fill (Qt::transparent);
  {
    QPainter p (&img);
    p.setRenderHint (QPainter::TextAntialiasing);
    p.translate (-x, -y);
    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.clip = QRectF (x, y, width, height);
    if (l->cursor_visible && l->buffer) {
      GtkTextIter it;
      gtk_text_buffer_get_iter_at_mark (l->buffer, &it, gtk_text_buffer_get_insert (l->buffer));
      ctx.cursorPosition = gtk_text_iter_get_offset (&it);
    }
    /* the selection, as the buffer has it */
    GtkTextIter s, e;
    if (l->buffer && gtk_text_buffer_get_selection_bounds (l->buffer, &s, &e)) {
      QAbstractTextDocumentLayout::Selection sel;
      sel.cursor = QTextCursor (doc);
      sel.cursor.setPosition (gtk_text_iter_get_offset (&s));
      sel.cursor.setPosition (gtk_text_iter_get_offset (&e), QTextCursor::KeepAnchor);
      sel.format.setBackground (QApplication::palette ().highlight ());
      sel.format.setForeground (QApplication::palette ().highlightedText ());
      ctx.selections << sel;
    }
    doc->documentLayout ()->draw (&p, ctx);
  }
  cairo_surface_t *src = cairo_image_surface_create_for_data (img.bits (), CAIRO_FORMAT_ARGB32, img.width (), img.height (),
                                                              img.bytesPerLine ());
  cairo_t *cr = cairo_create (target);
  cairo_set_source_surface (cr, src, x + x_offset, y + y_offset);
  cairo_rectangle (cr, x + x_offset, y + y_offset, width, height);
  cairo_fill (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (src);
}

} /* extern "C" */
