/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkTextBuffer (a QTextDocument), GtkTextIter, GtkTextMark, GtkTextTag and
 * GtkTextView (a QTextEdit). Like GTK, one buffer can be shown by several
 * views.
 *
 * Note: offsets are QString (UTF-16) positions, which equal GTK character
 * offsets except for characters outside the Basic Multilingual Plane.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

/* ======================================================================
 * GtkTextMark / GtkTextTag
 * ====================================================================== */
struct _GtkTextMark { GObject parent_instance; GtkTextBuffer *buffer; gchar *name; gint offset; gboolean left_gravity; };
typedef struct { GObjectClass parent_class; } GtkTextMarkClass;
G_DEFINE_TYPE (GtkTextMark, gtk_text_mark, G_TYPE_OBJECT)
static void gtk_text_mark_init (GtkTextMark *) {}
static void gtk_text_mark_finalize (GObject *o)
{
  g_free (((GtkTextMark *) o)->name);
  G_OBJECT_CLASS (gtk_text_mark_parent_class)->finalize (o);
}
static void gtk_text_mark_class_init (GtkTextMarkClass *klass) { G_OBJECT_CLASS (klass)->finalize = gtk_text_mark_finalize; }

struct _GtkTextTag { GObject parent_instance; gchar *name; gpointer format; };
typedef struct { GObjectClass parent_class; } GtkTextTagClass;
G_DEFINE_TYPE (GtkTextTag, gtk_text_tag, G_TYPE_OBJECT)

static QTextCharFormat &tag_fmt (GtkTextTag *t)
{
  if (!t->format) t->format = new QTextCharFormat;
  return *static_cast<QTextCharFormat *> (t->format);
}

static void gtk_text_tag_init (GtkTextTag *) {}
static void gtk_text_tag_finalize (GObject *o)
{
  auto *t = (GtkTextTag *) o;
  g_free (t->name);
  delete static_cast<QTextCharFormat *> (t->format);
  G_OBJECT_CLASS (gtk_text_tag_parent_class)->finalize (o);
}

#define TAG_SET(body) [] (GObject *o, const GValue *v) { QTextCharFormat &f = tag_fmt ((GtkTextTag *) o); body; }

static void gtk_text_tag_class_init (GtkTextTagClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = gtk_text_tag_finalize;
  gtkqt_prop (klass, g_param_spec_string ("name", nullptr, nullptr, nullptr, (GParamFlags) (RW | G_PARAM_CONSTRUCT_ONLY)),
              [] (GObject *o, const GValue *v) { auto *t = (GtkTextTag *) o; g_free (t->name); t->name = g_value_dup_string (v); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, ((GtkTextTag *) o)->name); });
  gtkqt_prop (klass, g_param_spec_string ("foreground", nullptr, nullptr, nullptr, G_PARAM_WRITABLE),
              TAG_SET (f.setForeground (QColor (gtkqt_str (g_value_get_string (v))))), nullptr);
  gtkqt_prop (klass, g_param_spec_string ("background", nullptr, nullptr, nullptr, G_PARAM_WRITABLE),
              TAG_SET (f.setBackground (QColor (gtkqt_str (g_value_get_string (v))))), nullptr);
  gtkqt_prop (klass, g_param_spec_boxed ("foreground-rgba", nullptr, nullptr, GDK_TYPE_RGBA, G_PARAM_WRITABLE),
              TAG_SET (if (auto *c = (GdkRGBA *) g_value_get_boxed (v)) f.setForeground (QColor::fromRgbF (c->red, c->green, c->blue, c->alpha))), nullptr);
  gtkqt_prop (klass, g_param_spec_boxed ("background-rgba", nullptr, nullptr, GDK_TYPE_RGBA, G_PARAM_WRITABLE),
              TAG_SET (if (auto *c = (GdkRGBA *) g_value_get_boxed (v)) f.setBackground (QColor::fromRgbF (c->red, c->green, c->blue, c->alpha))), nullptr);
  gtkqt_prop (klass, g_param_spec_int ("weight", nullptr, nullptr, 0, 1000, 400, G_PARAM_WRITABLE),
              TAG_SET (f.setFontWeight (g_value_get_int (v) >= 600 ? QFont::Bold : QFont::Normal)), nullptr);
  gtkqt_prop (klass, g_param_spec_int ("style", nullptr, nullptr, 0, 2, 0, G_PARAM_WRITABLE),
              TAG_SET (f.setFontItalic (g_value_get_int (v) != 0)), nullptr);
  gtkqt_prop (klass, g_param_spec_int ("underline", nullptr, nullptr, 0, 4, 0, G_PARAM_WRITABLE),
              TAG_SET (f.setFontUnderline (g_value_get_int (v) != 0)), nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("strikethrough", nullptr, nullptr, FALSE, G_PARAM_WRITABLE),
              TAG_SET (f.setFontStrikeOut (g_value_get_boolean (v))), nullptr);
  gtkqt_prop (klass, g_param_spec_double ("size-points", nullptr, nullptr, 0, G_MAXDOUBLE, 0, G_PARAM_WRITABLE),
              TAG_SET (f.setFontPointSize (g_value_get_double (v))), nullptr);
  gtkqt_prop (klass, g_param_spec_double ("scale", nullptr, nullptr, 0, G_MAXDOUBLE, 1, G_PARAM_WRITABLE),
              TAG_SET (f.setFontPointSize (QApplication::font ().pointSizeF () * g_value_get_double (v))), nullptr);
  gtkqt_prop (klass, g_param_spec_string ("family", nullptr, nullptr, nullptr, G_PARAM_WRITABLE),
              TAG_SET (f.setFontFamilies ({ gtkqt_str (g_value_get_string (v)) })), nullptr);
  gtkqt_prop (klass, g_param_spec_string ("font", nullptr, nullptr, nullptr, G_PARAM_WRITABLE),
              TAG_SET ({
                QStringList parts = gtkqt_str (g_value_get_string (v)).split (' ', Qt::SkipEmptyParts);
                bool ok = false;
                double size = parts.isEmpty () ? 0 : parts.last ().toDouble (&ok);
                if (ok) { f.setFontPointSize (size); parts.removeLast (); }
                if (parts.removeAll ("Bold")) f.setFontWeight (QFont::Bold);
                if (parts.removeAll ("Italic")) f.setFontItalic (true);
                if (!parts.isEmpty ()) {
                  QString fam = parts.join (' ');
                  f.setFontFamilies ({ fam.compare ("monospace", Qt::CaseInsensitive) == 0 ? QFontDatabase::systemFont (QFontDatabase::FixedFont).family () : fam });
                }
              }), nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("editable", nullptr, nullptr, TRUE, G_PARAM_WRITABLE), [] (GObject *, const GValue *) {}, nullptr);
}

/* ======================================================================
 * GtkTextBuffer
 * ====================================================================== */
struct BufferData {
  QTextDocument *doc = nullptr;
  int insert = 0, bound = 0;
  GList *marks = nullptr;              /* GtkTextMark* (owned) */
  GtkTextMark *insert_mark = nullptr, *bound_mark = nullptr;
  QMap<QString, GtkTextTag *> tags;
  QList<QPointer<QTextEdit>> views;
  int user_action = 0;
  bool edit_block = false;
};

static BufferData *bd (GtkTextBuffer *b) { return static_cast<BufferData *> (b->priv); }
static QTextDocument *bdoc (GtkTextBuffer *b) { return bd (b)->doc; }
static int end_offset (GtkTextBuffer *b) { return qMax (0, bdoc (b)->characterCount () - 1); }

G_DEFINE_TYPE (GtkTextBuffer, gtk_text_buffer, G_TYPE_OBJECT)

static GtkTextMark *mark_new (GtkTextBuffer *b, const gchar *name, int offset, gboolean left)
{
  auto *m = (GtkTextMark *) g_object_new (gtk_text_mark_get_type (), nullptr);
  m->buffer = b;
  m->name = g_strdup (name);
  m->offset = offset;
  m->left_gravity = left;
  bd (b)->marks = g_list_append (bd (b)->marks, m);
  return m;
}

static void gtk_text_buffer_init (GtkTextBuffer *b)
{
  gtkqt_ensure_app ();
  auto *d = new BufferData;
  b->priv = d;
  d->doc = new QTextDocument;
  d->doc->setDocumentMargin (gtkqt_api <= 2 ? 1 : 4);   /* GTK 2 text starts at the edge */
  d->doc->setUndoRedoEnabled (gtkqt_api >= 4);
  d->insert_mark = mark_new (b, "insert", 0, FALSE);
  d->bound_mark = mark_new (b, "selection_bound", 0, TRUE);
  GtkTextBuffer *self = b;
  QObject::connect (d->doc, &QTextDocument::contentsChange, d->doc, [self] (int pos, int removed, int added) {
    BufferData *bdd = bd (self);
    for (GList *l = bdd->marks; l; l = l->next) {
      auto *m = (GtkTextMark *) l->data;
      if (m == bdd->insert_mark || m == bdd->bound_mark) continue;
      if (m->offset > pos + removed) m->offset += added - removed;
      else if (m->offset > pos) m->offset = pos + (m->left_gravity ? 0 : added);
      else if (m->offset == pos && !m->left_gravity && added && !removed) m->offset += added;
    }
  });
  QObject::connect (d->doc, &QTextDocument::contentsChanged, d->doc, [self] {
    GtkQtEmitGuard guard;
    g_signal_emit_by_name (self, "changed");
  });
  QObject::connect (d->doc, &QTextDocument::modificationChanged, d->doc, [self] (bool) {
    GtkQtEmitGuard guard;
    g_signal_emit_by_name (self, "modified-changed");
  });
}

static void gtk_text_buffer_finalize (GObject *o)
{
  auto *b = GTK_TEXT_BUFFER (o);
  BufferData *d = bd (b);
  g_list_free_full (d->marks, g_object_unref);
  for (GtkTextTag *t : d->tags) g_object_unref (t);
  delete d->doc;
  delete d;
  G_OBJECT_CLASS (gtk_text_buffer_parent_class)->finalize (o);
}

extern "C" {
static void real_insert_text (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len);
static void real_delete_range (GtkTextBuffer *b, GtkTextIter *start, GtkTextIter *end);
static void real_apply_tag (GtkTextBuffer *b, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end);
static void real_remove_tag (GtkTextBuffer *b, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end);
}

static void gtk_text_buffer_class_init (GtkTextBufferClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = gtk_text_buffer_finalize;
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("changed", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, changed), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("modified-changed", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, modified_changed),
                nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("mark-set", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, mark_set), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 2, GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_OBJECT);
  g_signal_new ("mark-deleted", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, mark_deleted), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 1, G_TYPE_OBJECT);
  /* Every change goes through these, as in GTK: handlers run before the
   * default handler makes the change (connect_after: after it). */
  g_signal_new ("insert-text", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, insert_text), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 3, GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_INT);
  g_signal_new ("delete-range", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, delete_range), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 2, GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE, GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_new ("apply-tag", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, apply_tag), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 3, GTK_TYPE_TEXT_TAG, GTK_TYPE_TEXT_ITER, GTK_TYPE_TEXT_ITER);
  g_signal_new ("remove-tag", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, remove_tag), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 3, GTK_TYPE_TEXT_TAG, GTK_TYPE_TEXT_ITER, GTK_TYPE_TEXT_ITER);
  g_signal_new ("begin-user-action", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, begin_user_action), nullptr, nullptr,
                nullptr, G_TYPE_NONE, 0);
  g_signal_new ("end-user-action", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, end_user_action), nullptr, nullptr,
                nullptr, G_TYPE_NONE, 0);
  klass->insert_text = real_insert_text;
  klass->delete_range = real_delete_range;
  klass->apply_tag = real_apply_tag;
  klass->remove_tag = real_remove_tag;
  klass->begin_user_action = [] (GtkTextBuffer *b) {
    if (bd (b)->doc->isUndoRedoEnabled () && !bd (b)->edit_block) { QTextCursor (bdoc (b)).beginEditBlock (); bd (b)->edit_block = true; }
  };
  klass->end_user_action = [] (GtkTextBuffer *b) {
    if (bd (b)->edit_block) { QTextCursor (bdoc (b)).endEditBlock (); bd (b)->edit_block = false; }
  };
  gtkqt_prop (klass, g_param_spec_string ("text", nullptr, nullptr, "", RW),
              [] (GObject *o, const GValue *v) { gtk_text_buffer_set_text (GTK_TEXT_BUFFER (o), g_value_get_string (v), -1); },
              [] (GObject *o, GValue *v) {
                GtkTextIter s, e;
                gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (o), &s, &e);
                g_value_take_string (v, gtk_text_buffer_get_text (GTK_TEXT_BUFFER (o), &s, &e, TRUE));
              });
}

static void iter_init (GtkTextIter *it, GtkTextBuffer *b, int offset)
{
  memset (it, 0, sizeof (*it));
  it->buffer = b;
  it->offset = qBound (0, offset, end_offset (b));
}

static int iter_off (const GtkTextIter *it) { return it ? it->offset : 0; }

/* cursor/selection: a view's QTextEdit owns the real cursor */
static void buffer_pull_cursor (GtkTextBuffer *b)
{
  BufferData *d = bd (b);
  for (const QPointer<QTextEdit> &v : d->views) {
    if (!v) continue;
    QTextCursor c = v->textCursor ();
    d->insert = c.position ();
    d->bound = c.anchor ();
    break;
  }
  d->insert_mark->offset = d->insert;
  d->bound_mark->offset = d->bound;
}

static void buffer_push_cursor (GtkTextBuffer *b)
{
  BufferData *d = bd (b);
  for (const QPointer<QTextEdit> &v : d->views) {
    if (!v) continue;
    QTextCursor c (d->doc);
    c.setPosition (qBound (0, d->bound, end_offset (b)));
    c.setPosition (qBound (0, d->insert, end_offset (b)), QTextCursor::KeepAnchor);
    v->setTextCursor (c);
  }
  d->insert_mark->offset = d->insert;
  d->bound_mark->offset = d->bound;
}

GtkWidget *gtkqt_text_buffer_any_view (GtkTextBuffer *b)
{
  for (const QPointer<QTextEdit> &v : bd (b)->views)
    if (v) return gtkqt_from_qobject (v);
  return nullptr;
}

extern "C" {

GtkTextBuffer *gtk_text_buffer_new (GtkTextTagTable *) { return (GtkTextBuffer *) g_object_new (GTK_TYPE_TEXT_BUFFER, nullptr); }
gint gtk_text_buffer_get_line_count (GtkTextBuffer *b) { return bdoc (b)->blockCount (); }
gint gtk_text_buffer_get_char_count (GtkTextBuffer *b) { return end_offset (b); }

void gtk_text_buffer_set_text (GtkTextBuffer *b, const gchar *text, gint len)
{
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds (b, &start, &end);
  gtk_text_buffer_delete (b, &start, &end);
  if (text && len != 0) {
    gtk_text_buffer_get_start_iter (b, &start);
    gtk_text_buffer_insert (b, &start, text, len);
  }
}

gchar *gtk_text_buffer_get_text (GtkTextBuffer *b, const GtkTextIter *start, const GtkTextIter *end, gboolean)
{
  int s = iter_off (start), e = end ? iter_off (end) : end_offset (b);
  if (e < s) qSwap (s, e);
  return g_strdup (bdoc (b)->toPlainText ().mid (s, e - s).toUtf8 ().constData ());
}

gchar *gtk_text_buffer_get_slice (GtkTextBuffer *b, const GtkTextIter *start, const GtkTextIter *end, gboolean h)
{
  return gtk_text_buffer_get_text (b, start, end, h);
}

static void real_insert_text (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len)
{
  QString s = len < 0 ? gtkqt_str (text) : QString::fromUtf8 (text, len);
  int pos = qBound (0, iter_off (iter), end_offset (b));
  QTextCursor c (bdoc (b));
  c.setPosition (pos);
  c.insertText (s, QTextCharFormat ());
  buffer_pull_cursor (b);
  iter_init (iter, b, pos + s.size ());
}

static void real_delete_range (GtkTextBuffer *b, GtkTextIter *start, GtkTextIter *end)
{
  int s = iter_off (start), e = iter_off (end);
  if (e < s) qSwap (s, e);
  QTextCursor c (bdoc (b));
  c.setPosition (s);
  c.setPosition (e, QTextCursor::KeepAnchor);
  c.removeSelectedText ();
  buffer_pull_cursor (b);
  iter_init (start, b, s);
  iter_init (end, b, s);
}

static void emit_insert (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len)
{
  if (!text) return;
  if (len < 0) len = strlen (text);
  if (len == 0) return;
  GtkQtEmitGuard guard;
  g_signal_emit_by_name (b, "insert-text", iter, text, len);
}

static void insert_formatted (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len, const QList<GtkTextTag *> &tags)
{
  int start = iter_off (iter);
  emit_insert (b, iter, text, len);
  if (tags.isEmpty ()) return;
  GtkTextIter s;
  iter_init (&s, b, start);
  for (GtkTextTag *t : tags) gtk_text_buffer_apply_tag (b, t, &s, iter);
}

void gtk_text_buffer_insert (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len) { emit_insert (b, iter, text, len); }

void gtk_text_buffer_insert_at_cursor (GtkTextBuffer *b, const gchar *text, gint len)
{
  buffer_pull_cursor (b);
  GtkTextIter it;
  iter_init (&it, b, bd (b)->insert);
  gtk_text_buffer_insert (b, &it, text, len);
}

void gtk_text_buffer_insert_with_tags (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len, GtkTextTag *first_tag, ...)
{
  QList<GtkTextTag *> tags;
  va_list ap;
  va_start (ap, first_tag);
  for (GtkTextTag *t = first_tag; t; t = va_arg (ap, GtkTextTag *)) tags << t;
  va_end (ap);
  insert_formatted (b, iter, text, len, tags);
}

void gtk_text_buffer_insert_with_tags_by_name (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len, const gchar *first_tag_name, ...)
{
  QList<GtkTextTag *> tags;
  va_list ap;
  va_start (ap, first_tag_name);
  for (const gchar *n = first_tag_name; n; n = va_arg (ap, const gchar *))
    if (GtkTextTag *t = bd (b)->tags.value (gtkqt_str (n))) tags << t;
  va_end (ap);
  insert_formatted (b, iter, text, len, tags);
}

void gtk_text_buffer_delete (GtkTextBuffer *b, GtkTextIter *start, GtkTextIter *end)
{
  if (start->offset == end->offset) return;
  if (end->offset < start->offset) qSwap (start->offset, end->offset);
  GtkQtEmitGuard guard;
  g_signal_emit_by_name (b, "delete-range", start, end);
}

gboolean gtk_text_buffer_insert_interactive (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len, gboolean editable)
{
  if (!editable) return FALSE;
  gtk_text_buffer_insert (b, iter, text, len);
  return TRUE;
}

gboolean gtk_text_buffer_insert_interactive_at_cursor (GtkTextBuffer *b, const gchar *text, gint len, gboolean editable)
{
  if (!editable) return FALSE;
  gtk_text_buffer_insert_at_cursor (b, text, len);
  return TRUE;
}

gboolean gtk_text_buffer_delete_interactive (GtkTextBuffer *b, GtkTextIter *start, GtkTextIter *end, gboolean editable)
{
  if (!editable) return FALSE;
  gtk_text_buffer_delete (b, start, end);
  return TRUE;
}

void gtk_text_buffer_get_start_iter (GtkTextBuffer *b, GtkTextIter *iter) { iter_init (iter, b, 0); }
void gtk_text_buffer_get_end_iter (GtkTextBuffer *b, GtkTextIter *iter) { iter_init (iter, b, end_offset (b)); }

void gtk_text_buffer_get_bounds (GtkTextBuffer *b, GtkTextIter *start, GtkTextIter *end)
{
  if (start) gtk_text_buffer_get_start_iter (b, start);
  if (end) gtk_text_buffer_get_end_iter (b, end);
}

void gtk_text_buffer_get_iter_at_offset (GtkTextBuffer *b, GtkTextIter *iter, gint char_offset)
{
  iter_init (iter, b, char_offset < 0 ? end_offset (b) : char_offset);
}

void gtk_text_buffer_get_iter_at_line (GtkTextBuffer *b, GtkTextIter *iter, gint line_number)
{
  QTextBlock blk = bdoc (b)->findBlockByNumber (line_number);
  iter_init (iter, b, blk.isValid () ? blk.position () : end_offset (b));
}

void gtk_text_buffer_get_iter_at_line_offset (GtkTextBuffer *b, GtkTextIter *iter, gint line_number, gint char_offset)
{
  QTextBlock blk = bdoc (b)->findBlockByNumber (line_number);
  iter_init (iter, b, blk.isValid () ? blk.position () + qMin (char_offset, blk.length () - 1) : end_offset (b));
}

void gtk_text_buffer_get_iter_at_mark (GtkTextBuffer *b, GtkTextIter *iter, GtkTextMark *mark)
{
  BufferData *d = bd (b);
  if (mark == d->insert_mark || mark == d->bound_mark) buffer_pull_cursor (b);
  iter_init (iter, b, mark ? mark->offset : 0);
}

GtkTextMark *gtk_text_buffer_get_insert (GtkTextBuffer *b) { buffer_pull_cursor (b); return bd (b)->insert_mark; }
GtkTextMark *gtk_text_buffer_get_selection_bound (GtkTextBuffer *b) { buffer_pull_cursor (b); return bd (b)->bound_mark; }

GtkTextMark *gtk_text_buffer_create_mark (GtkTextBuffer *b, const gchar *mark_name, const GtkTextIter *where, gboolean left_gravity)
{
  if (mark_name)
    for (GList *l = bd (b)->marks; l; l = l->next)
      if (!g_strcmp0 (((GtkTextMark *) l->data)->name, mark_name)) {
        ((GtkTextMark *) l->data)->offset = iter_off (where);
        return (GtkTextMark *) l->data;
      }
  GtkTextMark *m = mark_new (b, mark_name, iter_off (where), left_gravity);
  GtkTextIter it;
  iter_init (&it, b, m->offset);
  g_signal_emit_by_name (b, "mark-set", &it, m);
  return m;
}

void gtk_text_buffer_move_mark (GtkTextBuffer *b, GtkTextMark *mark, const GtkTextIter *where)
{
  BufferData *d = bd (b);
  if (!mark) return;
  if (mark == d->insert_mark) { buffer_pull_cursor (b); d->insert = iter_off (where); buffer_push_cursor (b); }
  else if (mark == d->bound_mark) { buffer_pull_cursor (b); d->bound = iter_off (where); buffer_push_cursor (b); }
  else mark->offset = iter_off (where);
  if (mark != d->insert_mark) {   /* the insert mark reports through the view's cursor */
    GtkTextIter it;
    iter_init (&it, b, mark->offset);
    g_signal_emit_by_name (b, "mark-set", &it, mark);
  }
}

void gtk_text_buffer_place_cursor (GtkTextBuffer *b, const GtkTextIter *where)
{
  bd (b)->insert = bd (b)->bound = iter_off (where);
  buffer_push_cursor (b);
}

void gtk_text_buffer_select_range (GtkTextBuffer *b, const GtkTextIter *ins, const GtkTextIter *bound)
{
  bd (b)->insert = iter_off (ins);
  bd (b)->bound = iter_off (bound);
  buffer_push_cursor (b);
}

gboolean gtk_text_buffer_get_selection_bounds (GtkTextBuffer *b, GtkTextIter *start, GtkTextIter *end)
{
  buffer_pull_cursor (b);
  BufferData *d = bd (b);
  int s = qMin (d->insert, d->bound), e = qMax (d->insert, d->bound);
  if (start) iter_init (start, b, s);
  if (end) iter_init (end, b, e);
  return s != e;
}

gboolean gtk_text_buffer_delete_selection (GtkTextBuffer *b, gboolean, gboolean)
{
  GtkTextIter s, e;
  if (!gtk_text_buffer_get_selection_bounds (b, &s, &e)) return FALSE;
  gtk_text_buffer_delete (b, &s, &e);
  return TRUE;
}

gboolean gtk_text_buffer_get_has_selection (GtkTextBuffer *b) { return gtk_text_buffer_get_selection_bounds (b, nullptr, nullptr); }
gboolean gtk_text_buffer_get_modified (GtkTextBuffer *b) { return bdoc (b)->isModified (); }
void gtk_text_buffer_set_modified (GtkTextBuffer *b, gboolean setting) { bdoc (b)->setModified (setting); }

GtkTextTag *gtk_text_buffer_create_tag (GtkTextBuffer *b, const gchar *tag_name, const gchar *first_property_name, ...)
{
  va_list ap;
  va_start (ap, first_property_name);
  auto *t = (GtkTextTag *) g_object_new_valist (GTK_TYPE_TEXT_TAG, first_property_name, ap);
  va_end (ap);
  g_free (t->name);
  t->name = g_strdup (tag_name);
  if (tag_name) {
    if (GtkTextTag *old = bd (b)->tags.take (gtkqt_str (tag_name))) g_object_unref (old);
    bd (b)->tags.insert (gtkqt_str (tag_name), t);
  } else {
    bd (b)->tags.insert (QStringLiteral ("\x01%1").arg ((quintptr) t), t);
  }
  return t;
}

void gtk_text_buffer_apply_tag (GtkTextBuffer *b, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end)
{
  if (tag) g_signal_emit_by_name (b, "apply-tag", tag, start, end);
}

static void real_remove_tag (GtkTextBuffer *b, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end)
{
  /* Qt formats do not stack like GTK tags: clear the properties the tag sets */
  QTextCursor c (bdoc (b));
  c.setPosition (iter_off (start));
  c.setPosition (iter_off (end), QTextCursor::KeepAnchor);
  QTextCharFormat f;
  const QTextCharFormat &tf = tag_fmt (tag);
  QTextCharFormat clear = c.charFormat ();
  for (int id : tf.properties ().keys ()) clear.clearProperty (id);
  c.setCharFormat (clear);
}

void gtk_text_buffer_remove_tag (GtkTextBuffer *b, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end)
{
  if (tag) g_signal_emit_by_name (b, "remove-tag", tag, start, end);
}

void gtk_text_buffer_remove_tag_by_name (GtkTextBuffer *b, const gchar *name, const GtkTextIter *start, const GtkTextIter *end)
{
  if (GtkTextTag *t = bd (b)->tags.value (gtkqt_str (name))) gtk_text_buffer_remove_tag (b, t, start, end);
}

static void real_apply_tag (GtkTextBuffer *b, GtkTextTag *tag, const GtkTextIter *start, const GtkTextIter *end)
{
  QTextCursor c (bdoc (b));
  c.setPosition (iter_off (start));
  c.setPosition (iter_off (end), QTextCursor::KeepAnchor);
  c.mergeCharFormat (tag_fmt (tag));
}

void gtk_text_buffer_apply_tag_by_name (GtkTextBuffer *b, const gchar *name, const GtkTextIter *start, const GtkTextIter *end)
{
  if (GtkTextTag *t = bd (b)->tags.value (gtkqt_str (name))) gtk_text_buffer_apply_tag (b, t, start, end);
}

void gtk_text_buffer_remove_all_tags (GtkTextBuffer *b, const GtkTextIter *start, const GtkTextIter *end)
{
  QTextCursor c (bdoc (b));
  c.setPosition (iter_off (start));
  c.setPosition (iter_off (end), QTextCursor::KeepAnchor);
  c.setCharFormat (QTextCharFormat ());
}

void gtk_text_buffer_begin_user_action (GtkTextBuffer *b)
{
  if (bd (b)->user_action++ == 0) g_signal_emit_by_name (b, "begin-user-action");
}

void gtk_text_buffer_end_user_action (GtkTextBuffer *b)
{
  if (bd (b)->user_action > 0 && --bd (b)->user_action == 0) g_signal_emit_by_name (b, "end-user-action");
}

void gtk_text_buffer_undo (GtkTextBuffer *b) { bdoc (b)->undo (); }
void gtk_text_buffer_redo (GtkTextBuffer *b) { bdoc (b)->redo (); }

static QTextEdit *first_view (GtkTextBuffer *b)
{
  for (const QPointer<QTextEdit> &v : bd (b)->views) if (v) return v;
  return nullptr;
}

static QClipboard::Mode buffer_clip_mode (gpointer clipboard)
{
  return clipboard && clipboard == gtk_clipboard_get (GDK_SELECTION_PRIMARY) && QGuiApplication::clipboard ()->supportsSelection ()
         ? QClipboard::Selection : QClipboard::Clipboard;
}

void gtk_text_buffer_copy_clipboard (GtkTextBuffer *b, gpointer clipboard)
{
  GtkTextIter s, e;
  if (!gtk_text_buffer_get_selection_bounds (b, &s, &e)) return;
  gchar *t = gtk_text_buffer_get_text (b, &s, &e, TRUE);
  QGuiApplication::clipboard ()->setText (QString::fromUtf8 (t), buffer_clip_mode (clipboard));
  g_free (t);
}

void gtk_text_buffer_cut_clipboard (GtkTextBuffer *b, gpointer clipboard, gboolean editable)
{
  gtk_text_buffer_copy_clipboard (b, clipboard);
  gtk_text_buffer_begin_user_action (b);
  gtk_text_buffer_delete_selection (b, TRUE, editable);
  gtk_text_buffer_end_user_action (b);
}

void gtk_text_buffer_paste_clipboard (GtkTextBuffer *b, gpointer clipboard, GtkTextIter *override_location, gboolean editable)
{
  const QMimeData *m = QGuiApplication::clipboard ()->mimeData (buffer_clip_mode (clipboard));
  if (!m || !m->hasText () || !editable) return;
  QByteArray t = m->text ().toUtf8 ();
  gtk_text_buffer_begin_user_action (b);
  if (override_location) gtk_text_buffer_insert (b, override_location, t.constData (), t.size ());
  else {
    gtk_text_buffer_delete_selection (b, TRUE, editable);
    gtk_text_buffer_insert_at_cursor (b, t.constData (), t.size ());
  }
  gtk_text_buffer_end_user_action (b);
}

void gtk_text_buffer_add_selection_clipboard (GtkTextBuffer *, gpointer) {}
void gtk_text_buffer_remove_selection_clipboard (GtkTextBuffer *, gpointer) {}

void gtk_text_buffer_move_mark_by_name (GtkTextBuffer *b, const gchar *name, const GtkTextIter *where)
{
  if (GtkTextMark *m = gtk_text_buffer_get_mark (b, name)) gtk_text_buffer_move_mark (b, m, where);
}

GtkTextMark *gtk_text_buffer_get_mark (GtkTextBuffer *b, const gchar *name)
{
  if (!name) return nullptr;
  if (!strcmp (name, "insert")) return gtk_text_buffer_get_insert (b);
  if (!strcmp (name, "selection_bound")) return gtk_text_buffer_get_selection_bound (b);
  for (GList *l = bd (b)->marks; l; l = l->next)
    if (!g_strcmp0 (((GtkTextMark *) l->data)->name, name)) return (GtkTextMark *) l->data;
  return nullptr;
}

void gtk_text_buffer_delete_mark (GtkTextBuffer *b, GtkTextMark *mark)
{
  BufferData *d = bd (b);
  if (!mark || mark == d->insert_mark || mark == d->bound_mark || !g_list_find (d->marks, mark)) return;
  d->marks = g_list_remove (d->marks, mark);
  mark->buffer = nullptr;
  g_signal_emit_by_name (b, "mark-deleted", mark);
  g_object_unref (mark);
}

void gtk_text_buffer_delete_mark_by_name (GtkTextBuffer *b, const gchar *name) { gtk_text_buffer_delete_mark (b, gtk_text_buffer_get_mark (b, name)); }

void gtk_text_buffer_get_iter_at_line_index (GtkTextBuffer *b, GtkTextIter *iter, gint line, gint byte_index)
{
  QTextBlock blk = bdoc (b)->findBlockByNumber (qBound (0, line, bdoc (b)->blockCount () - 1));
  QByteArray u = blk.text ().toUtf8 ().left (qMax (0, byte_index));
  iter_init (iter, b, blk.position () + QString::fromUtf8 (u).size ());
}

const gchar *gtk_text_mark_get_name (GtkTextMark *mark) { return mark ? mark->name : nullptr; }
void gtk_text_mark_set_visible (GtkTextMark *mark, gboolean setting) { if (mark) g_object_set_data (G_OBJECT (mark), "gtkqt-visible", GINT_TO_POINTER (setting)); }
gboolean gtk_text_mark_get_visible (GtkTextMark *mark) { return mark && g_object_get_data (G_OBJECT (mark), "gtkqt-visible"); }
gboolean gtk_text_mark_get_deleted (GtkTextMark *mark) { return !mark || !mark->buffer; }
GtkTextBuffer *gtk_text_mark_get_buffer (GtkTextMark *mark) { return mark ? mark->buffer : nullptr; }
gboolean gtk_text_mark_get_left_gravity (GtkTextMark *mark) { return mark && mark->left_gravity; }

/* ---------- GtkTextIter ---------- */
GType gtk_text_iter_get_type (void)
{
  static GType t = 0;
  if (!t) t = g_boxed_type_register_static ("GtkTextIter", (GBoxedCopyFunc) gtk_text_iter_copy, (GBoxedFreeFunc) gtk_text_iter_free);
  return t;
}

static GtkTextBuffer *ib (const GtkTextIter *it) { return (GtkTextBuffer *) it->buffer; }

GtkTextBuffer *gtk_text_iter_get_buffer (const GtkTextIter *iter) { return ib (iter); }
GtkTextIter *gtk_text_iter_copy (const GtkTextIter *iter) { return (GtkTextIter *) g_memdup2 (iter, sizeof (GtkTextIter)); }
void gtk_text_iter_free (GtkTextIter *iter) { g_free (iter); }
gint gtk_text_iter_get_offset (const GtkTextIter *iter) { return iter->offset; }
void gtk_text_iter_set_offset (GtkTextIter *iter, gint char_offset) { iter_init (iter, ib (iter), char_offset); }
gint gtk_text_iter_get_line (const GtkTextIter *iter) { return bdoc (ib (iter))->findBlock (iter->offset).blockNumber (); }

gint gtk_text_iter_get_line_offset (const GtkTextIter *iter)
{
  return iter->offset - bdoc (ib (iter))->findBlock (iter->offset).position ();
}

gunichar gtk_text_iter_get_char (const GtkTextIter *iter)
{
  if (iter->offset >= end_offset (ib (iter))) return 0;
  QChar c = bdoc (ib (iter))->characterAt (iter->offset);
  return c == QChar::ParagraphSeparator ? '\n' : c.unicode ();
}

gchar *gtk_text_iter_get_text (const GtkTextIter *start, const GtkTextIter *end)
{
  return gtk_text_buffer_get_text (ib (start), start, end, TRUE);
}

static gboolean iter_move (GtkTextIter *iter, int to)
{
  int e = end_offset (ib (iter));
  to = qBound (0, to, e);
  bool moved = to != iter->offset;
  iter->offset = to;
  return moved && to != e;
}

gboolean gtk_text_iter_forward_char (GtkTextIter *iter) { return iter_move (iter, iter->offset + 1); }
gboolean gtk_text_iter_backward_char (GtkTextIter *iter) { int o = iter->offset; iter_move (iter, o - 1); return iter->offset != o; }
gboolean gtk_text_iter_forward_chars (GtkTextIter *iter, gint count) { return iter_move (iter, iter->offset + count); }
gboolean gtk_text_iter_backward_chars (GtkTextIter *iter, gint count) { int o = iter->offset; iter_move (iter, o - count); return iter->offset != o; }

gboolean gtk_text_iter_forward_line (GtkTextIter *iter)
{
  QTextBlock next = bdoc (ib (iter))->findBlock (iter->offset).next ();
  if (!next.isValid ()) { iter->offset = end_offset (ib (iter)); return FALSE; }
  iter->offset = next.position ();
  return TRUE;
}

gboolean gtk_text_iter_backward_line (GtkTextIter *iter)
{
  QTextBlock blk = bdoc (ib (iter))->findBlock (iter->offset);
  QTextBlock prev = blk.previous ();
  int o = iter->offset;
  iter->offset = prev.isValid () ? prev.position () : 0;
  return iter->offset != o;
}

gboolean gtk_text_iter_forward_to_line_end (GtkTextIter *iter)
{
  QTextBlock blk = bdoc (ib (iter))->findBlock (iter->offset);
  int o = iter->offset;
  iter->offset = blk.position () + blk.length () - 1;
  if (iter->offset == o) {
    if (!gtk_text_iter_forward_line (iter)) return FALSE;
    blk = bdoc (ib (iter))->findBlock (iter->offset);
    iter->offset = blk.position () + blk.length () - 1;
  }
  return iter->offset != end_offset (ib (iter));
}

static gboolean iter_word (GtkTextIter *iter, QTextCursor::MoveOperation op)
{
  QTextCursor c (bdoc (ib (iter)));
  c.setPosition (iter->offset);
  int o = iter->offset;
  c.movePosition (op);
  iter->offset = c.position ();
  return iter->offset != o && iter->offset != end_offset (ib (iter));
}

gboolean gtk_text_iter_forward_word_end (GtkTextIter *iter)
{
  QTextCursor c (bdoc (ib (iter)));
  c.setPosition (iter->offset);
  c.movePosition (QTextCursor::NextWord);
  c.movePosition (QTextCursor::EndOfWord);
  int o = iter->offset;
  iter->offset = c.position ();
  return iter->offset != o;
}

gboolean gtk_text_iter_backward_word_start (GtkTextIter *iter) { return iter_word (iter, QTextCursor::PreviousWord) || iter->offset == 0; }
void gtk_text_iter_forward_to_end (GtkTextIter *iter) { iter->offset = end_offset (ib (iter)); }
gboolean gtk_text_iter_is_start (const GtkTextIter *iter) { return iter->offset == 0; }
gboolean gtk_text_iter_is_end (const GtkTextIter *iter) { return iter->offset >= end_offset (ib (iter)); }
gboolean gtk_text_iter_starts_line (const GtkTextIter *iter) { return gtk_text_iter_get_line_offset (iter) == 0; }

gboolean gtk_text_iter_ends_line (const GtkTextIter *iter)
{
  gunichar c = gtk_text_iter_get_char (iter);
  return c == '\n' || c == 0;
}

gboolean gtk_text_iter_equal (const GtkTextIter *lhs, const GtkTextIter *rhs) { return lhs->offset == rhs->offset; }
gint gtk_text_iter_compare (const GtkTextIter *lhs, const GtkTextIter *rhs) { return lhs->offset < rhs->offset ? -1 : lhs->offset > rhs->offset; }

gboolean gtk_text_iter_forward_search (const GtkTextIter *iter, const gchar *str, GtkTextSearchFlags flags,
                                       GtkTextIter *match_start, GtkTextIter *match_end, const GtkTextIter *limit)
{
  QString text = bdoc (ib (iter))->toPlainText (), needle = gtkqt_str (str);
  int i = text.indexOf (needle, iter->offset, flags & GTK_TEXT_SEARCH_CASE_INSENSITIVE ? Qt::CaseInsensitive : Qt::CaseSensitive);
  if (i < 0 || (limit && i + needle.size () > limit->offset)) return FALSE;
  if (match_start) iter_init (match_start, ib (iter), i);
  if (match_end) iter_init (match_end, ib (iter), i + needle.size ());
  return TRUE;
}

gboolean gtk_text_iter_backward_search (const GtkTextIter *iter, const gchar *str, GtkTextSearchFlags flags,
                                        GtkTextIter *match_start, GtkTextIter *match_end, const GtkTextIter *limit)
{
  QString text = bdoc (ib (iter))->toPlainText (), needle = gtkqt_str (str);
  int from = iter->offset - needle.size ();
  if (from < 0) return FALSE;
  int i = text.lastIndexOf (needle, from, flags & GTK_TEXT_SEARCH_CASE_INSENSITIVE ? Qt::CaseInsensitive : Qt::CaseSensitive);
  if (i < 0 || (limit && i < limit->offset)) return FALSE;
  if (match_start) iter_init (match_start, ib (iter), i);
  if (match_end) iter_init (match_end, ib (iter), i + needle.size ());
  return TRUE;
}

gchar *gtk_text_iter_get_slice (const GtkTextIter *start, const GtkTextIter *end) { return gtk_text_iter_get_text (start, end); }
gchar *gtk_text_iter_get_visible_slice (const GtkTextIter *start, const GtkTextIter *end) { return gtk_text_iter_get_text (start, end); }
gchar *gtk_text_iter_get_visible_text (const GtkTextIter *start, const GtkTextIter *end) { return gtk_text_iter_get_text (start, end); }

gboolean gtk_text_iter_in_range (const GtkTextIter *iter, const GtkTextIter *start, const GtkTextIter *end)
{
  return iter->offset >= start->offset && iter->offset < end->offset;
}

void gtk_text_iter_order (GtkTextIter *first, GtkTextIter *second)
{
  if (first->offset > second->offset) { GtkTextIter t = *first; *first = *second; *second = t; }
}

void gtk_text_iter_set_line (GtkTextIter *iter, gint line)
{
  QTextDocument *doc = bdoc (ib (iter));
  if (line >= doc->blockCount ()) { iter->offset = end_offset (ib (iter)); return; }
  iter_init (iter, ib (iter), doc->findBlockByNumber (qMax (0, line)).position ());
}

void gtk_text_iter_set_line_offset (GtkTextIter *iter, gint char_on_line)
{
  QTextBlock blk = bdoc (ib (iter))->findBlock (iter->offset);
  iter_init (iter, ib (iter), blk.position () + qBound (0, char_on_line, blk.length () - 1));
}
void gtk_text_iter_set_visible_line_offset (GtkTextIter *iter, gint c) { gtk_text_iter_set_line_offset (iter, c); }
gint gtk_text_iter_get_visible_line_offset (const GtkTextIter *iter) { return gtk_text_iter_get_line_offset (iter); }

gint gtk_text_iter_get_chars_in_line (const GtkTextIter *iter)
{
  QTextBlock blk = bdoc (ib (iter))->findBlock (iter->offset);
  /* includes the paragraph delimiter, except on the last line */
  return blk.next ().isValid () ? blk.length () : blk.length () - 1;
}

gint gtk_text_iter_get_line_index (const GtkTextIter *iter)
{
  QTextBlock blk = bdoc (ib (iter))->findBlock (iter->offset);
  return blk.text ().left (iter->offset - blk.position ()).toUtf8 ().size ();
}

gboolean gtk_text_iter_forward_lines (GtkTextIter *iter, gint count)
{
  if (count < 0) return gtk_text_iter_backward_lines (iter, -count);
  gboolean moved = FALSE;
  for (int i = 0; i < count; i++) if (gtk_text_iter_forward_line (iter)) moved = TRUE; else break;
  return moved && !gtk_text_iter_is_end (iter);
}

gboolean gtk_text_iter_backward_lines (GtkTextIter *iter, gint count)
{
  if (count < 0) return gtk_text_iter_forward_lines (iter, -count);
  gboolean moved = FALSE;
  for (int i = 0; i < count; i++) if (gtk_text_iter_backward_line (iter)) moved = TRUE; else break;
  return moved;
}

gboolean gtk_text_iter_forward_cursor_position (GtkTextIter *iter) { return gtk_text_iter_forward_char (iter); }
gboolean gtk_text_iter_backward_cursor_position (GtkTextIter *iter) { return gtk_text_iter_backward_char (iter); }
gboolean gtk_text_iter_is_cursor_position (const GtkTextIter *) { return TRUE; }

gboolean gtk_text_iter_forward_word_ends (GtkTextIter *iter, gint count)
{
  gboolean r = FALSE;
  for (int i = 0; i < count; i++) r = gtk_text_iter_forward_word_end (iter);
  return r;
}
gboolean gtk_text_iter_backward_word_starts (GtkTextIter *iter, gint count)
{
  gboolean r = FALSE;
  for (int i = 0; i < count; i++) r = gtk_text_iter_backward_word_start (iter);
  return r;
}

static bool word_char (const GtkTextIter *it, int delta)
{
  int o = it->offset + delta;
  if (o < 0 || o >= end_offset (ib (it))) return false;
  QChar c = bdoc (ib (it))->characterAt (o);
  return c.isLetterOrNumber () || c == '_';
}
gboolean gtk_text_iter_starts_word (const GtkTextIter *iter) { return word_char (iter, 0) && !word_char (iter, -1); }
gboolean gtk_text_iter_ends_word (const GtkTextIter *iter) { return !word_char (iter, 0) && word_char (iter, -1); }
gboolean gtk_text_iter_inside_word (const GtkTextIter *iter) { return word_char (iter, 0); }

gboolean gtk_text_iter_forward_find_char (GtkTextIter *iter, GtkTextCharPredicate pred, gpointer data, const GtkTextIter *limit)
{
  int lim = limit ? limit->offset : end_offset (ib (iter));
  while (iter->offset < lim) {
    iter->offset++;
    if (iter->offset >= lim) break;
    if (pred (gtk_text_iter_get_char (iter), data)) return TRUE;
  }
  iter->offset = lim;
  return FALSE;
}

gboolean gtk_text_iter_backward_find_char (GtkTextIter *iter, GtkTextCharPredicate pred, gpointer data, const GtkTextIter *limit)
{
  int lim = limit ? limit->offset : 0;
  while (iter->offset > lim) {
    iter->offset--;
    if (pred (gtk_text_iter_get_char (iter), data)) return TRUE;
  }
  iter->offset = lim;
  return FALSE;
}

gboolean gtk_text_iter_editable (const GtkTextIter *, gboolean default_setting) { return default_setting; }
gboolean gtk_text_iter_can_insert (const GtkTextIter *, gboolean default_editability) { return default_editability; }

} /* extern "C" */

/* ======================================================================
 * GtkTextView: a QTextEdit. Typing, deleting, cut and paste go through the
 * buffer (insert-text / delete-range signals, user actions) as in GTK, and
 * the GTK 2 border windows (line numbers ...) are QWidgets in the
 * scroll area's viewport margins whose expose-event the program draws.
 * ====================================================================== */
static GtkTextBuffer *view_buffer (GtkTextView *v) { return v ? v->buffer : nullptr; }

static int border_index (GtkTextWindowType t)
{
  switch (t) {
  case GTK_TEXT_WINDOW_LEFT: return 0;
  case GTK_TEXT_WINDOW_RIGHT: return 1;
  case GTK_TEXT_WINDOW_TOP: return 2;
  case GTK_TEXT_WINDOW_BOTTOM: return 3;
  default: return -1;
  }
}

class GtkQtTextEdit;

class GtkQtTextBorder : public QWidget {
public:
  GtkQtTextBorder (GtkQtTextEdit *edit, GtkTextWindowType type);
  ~GtkQtTextBorder () override;
  GtkQtTextEdit *edit;
  GtkTextWindowType type;
  GdkWindow *gdkwin;
protected:
  void paintEvent (QPaintEvent *e) override;
};

class GtkQtTextEdit : public QTextEdit {
public:
  GtkWidget *gw;
  int sizes[4] = { 0, 0, 0, 0 };
  GtkQtTextBorder *borders[4] = { nullptr, nullptr, nullptr, nullptr };
  GdkWindow *text_window = nullptr;
  bool overwrite = false;

  explicit GtkQtTextEdit (GtkWidget *w) : gw (w)
  {
    setAcceptRichText (false);
    QObject::connect (verticalScrollBar (), &QScrollBar::valueChanged, this, [this] { update_borders (true, false); });
    QObject::connect (horizontalScrollBar (), &QScrollBar::valueChanged, this, [this] { update_borders (false, true); });
  }
  ~GtkQtTextEdit () override
  {
    if (text_window) { text_window->qw = nullptr; text_window->widget = nullptr; g_object_unref (text_window); }
  }

  GtkTextView *view () const { return gw && gw->priv && !gw->priv->destroyed ? GTK_TEXT_VIEW (gw) : nullptr; }

  void update_borders (bool vertical, bool horizontal)
  {
    for (int i = 0; i < 4; i++)
      if (borders[i] && ((i < 2 && vertical) || (i >= 2 && horizontal))) borders[i]->update ();
  }

  void set_border (int i, int size)
  {
    if (size < 0) size = 0;
    if (sizes[i] == size && (size == 0 || borders[i])) return;
    sizes[i] = size;
    if (size > 0 && !borders[i]) {
      static const GtkTextWindowType types[4] = { GTK_TEXT_WINDOW_LEFT, GTK_TEXT_WINDOW_RIGHT, GTK_TEXT_WINDOW_TOP, GTK_TEXT_WINDOW_BOTTOM };
      borders[i] = new GtkQtTextBorder (this, types[i]);
    }
    setViewportMargins (sizes[0], sizes[2], sizes[1], sizes[3]);
    layout_borders ();
  }

  void layout_borders ()
  {
    QRect vp = viewport ()->geometry ();
    for (int i = 0; i < 4; i++) {
      if (!borders[i]) continue;
      QRect r;
      if (i == 0) r = QRect (vp.left () - sizes[0], vp.top (), sizes[0], vp.height ());
      else if (i == 1) r = QRect (vp.right () + 1, vp.top (), sizes[1], vp.height ());
      else if (i == 2) r = QRect (vp.left (), vp.top () - sizes[2], vp.width (), sizes[2]);
      else r = QRect (vp.left (), vp.bottom () + 1, vp.width (), sizes[3]);
      borders[i]->setGeometry (r);
      borders[i]->setVisible (sizes[i] > 0);
    }
  }

  /* typed text: replaces the selection, like GTK's insert-at-cursor */
  void commit (const QString &text)
  {
    GtkTextView *v = view ();
    if (!v || isReadOnly ()) return;
    GtkTextBuffer *b = view_buffer (v);
    QByteArray u = text.toUtf8 ();
    gtk_text_buffer_begin_user_action (b);
    gboolean had_selection = gtk_text_buffer_delete_selection (b, TRUE, TRUE);
    if (!had_selection && overwrite) {
      GtkTextIter s, e;
      gtk_text_buffer_get_iter_at_mark (b, &s, gtk_text_buffer_get_insert (b));
      e = s;
      if (!gtk_text_iter_ends_line (&e)) {
        gtk_text_iter_forward_char (&e);
        gtk_text_buffer_delete_interactive (b, &s, &e, TRUE);
      }
    }
    gtk_text_buffer_insert_interactive_at_cursor (b, u.constData (), u.size (), TRUE);
    gtk_text_buffer_end_user_action (b);
    ensureCursorVisible ();
  }

  /* Backspace / Delete, optionally by word */
  void delete_from_cursor (bool forward, bool word)
  {
    GtkTextView *v = view ();
    if (!v || isReadOnly ()) return;
    GtkTextBuffer *b = view_buffer (v);
    gtk_text_buffer_begin_user_action (b);
    if (!gtk_text_buffer_delete_selection (b, TRUE, TRUE)) {
      QTextCursor c = textCursor ();
      int pos = c.position ();
      c.movePosition (forward ? (word ? QTextCursor::NextWord : QTextCursor::NextCharacter)
                              : (word ? QTextCursor::PreviousWord : QTextCursor::PreviousCharacter));
      int other = c.position ();
      if (other != pos) {
        GtkTextIter s, e;
        gtk_text_buffer_get_iter_at_offset (b, &s, qMin (pos, other));
        gtk_text_buffer_get_iter_at_offset (b, &e, qMax (pos, other));
        gtk_text_buffer_delete_interactive (b, &s, &e, TRUE);
      }
    }
    gtk_text_buffer_end_user_action (b);
    ensureCursorVisible ();
  }

protected:
  bool event (QEvent *e) override
  {
    if (e->type () == QEvent::ShortcutOverride) {
      /* GTK: window accelerators win over the text view, except for typing */
      auto *ke = static_cast<QKeyEvent *> (e);
      Qt::KeyboardModifiers m = ke->modifiers () & ~(Qt::ShiftModifier | Qt::KeypadModifier);
      if (!ke->text ().isEmpty () && ke->text ().at (0).isPrint () && m == Qt::NoModifier) { e->accept (); return true; }
      e->ignore ();
      return false;
    }
    return QTextEdit::event (e);
  }

  void keyPressEvent (QKeyEvent *e) override
  {
    GtkTextView *v = view ();
    if (!v) { QTextEdit::keyPressEvent (e); return; }
    Qt::KeyboardModifiers m = e->modifiers () & ~Qt::KeypadModifier;
    bool ctrl = m & Qt::ControlModifier;
    if (e->matches (QKeySequence::Cut)) { g_signal_emit_by_name (v, "cut-clipboard"); return; }
    if (e->matches (QKeySequence::Copy)) { g_signal_emit_by_name (v, "copy-clipboard"); return; }
    if (e->matches (QKeySequence::Paste)) { g_signal_emit_by_name (v, "paste-clipboard"); return; }
    if ((e->matches (QKeySequence::Undo) || e->matches (QKeySequence::Redo)) && gtkqt_api < 4) return;
    if (isReadOnly ()) { QTextEdit::keyPressEvent (e); return; }
    switch (e->key ()) {
    case Qt::Key_Backspace:
      if (!(m & ~(Qt::ControlModifier | Qt::ShiftModifier))) { delete_from_cursor (false, ctrl); return; }
      break;
    case Qt::Key_Delete:
      if (!(m & ~Qt::ControlModifier)) { delete_from_cursor (true, ctrl); return; }
      break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
      if (!(m & ~Qt::ShiftModifier)) { commit (QStringLiteral ("\n")); return; }
      break;
    case Qt::Key_Tab:
      if (m == Qt::NoModifier && !tabChangesFocus ()) { commit (QStringLiteral ("\t")); return; }
      break;
    case Qt::Key_Insert:
      if (m == Qt::NoModifier) { g_signal_emit_by_name (v, "toggle-overwrite"); return; }
      break;
    default:
      break;
    }
    QString t = e->text ();
    if (!t.isEmpty () && t.at (0).isPrint () && !(m & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
      commit (t);
      return;
    }
    QTextEdit::keyPressEvent (e);
  }

  void inputMethodEvent (QInputMethodEvent *e) override
  {
    if (!e->commitString ().isEmpty () && e->preeditString ().isEmpty () && e->replacementLength () == 0 && view ()) {
      commit (e->commitString ());
      e->accept ();
      return;
    }
    QTextEdit::inputMethodEvent (e);
  }

  void insertFromMimeData (const QMimeData *source) override
  {
    if (source && source->hasText () && view ()) commit (source->text ());
  }

  void contextMenuEvent (QContextMenuEvent *e) override
  {
    GtkTextView *v = view ();
    if (!v) return;
    GtkTextBuffer *b = view_buffer (v);
    bool sel = gtk_text_buffer_get_has_selection (b), ro = isReadOnly ();
    QMenu menu (this);
    QPointer<GtkQtTextEdit> self (this);
    GtkWidget *gwid = gw;
    g_object_ref (gwid);
    menu.addAction (QIcon::fromTheme ("edit-cut"), QStringLiteral ("Cu&t"), [gwid] { g_signal_emit_by_name (gwid, "cut-clipboard"); })->setEnabled (sel && !ro);
    menu.addAction (QIcon::fromTheme ("edit-copy"), QStringLiteral ("&Copy"), [gwid] { g_signal_emit_by_name (gwid, "copy-clipboard"); })->setEnabled (sel);
    menu.addAction (QIcon::fromTheme ("edit-paste"), QStringLiteral ("&Paste"), [gwid] { g_signal_emit_by_name (gwid, "paste-clipboard"); })
        ->setEnabled (!ro && QGuiApplication::clipboard ()->mimeData () && QGuiApplication::clipboard ()->mimeData ()->hasText ());
    menu.addAction (QIcon::fromTheme ("edit-delete"), QStringLiteral ("&Delete"), [self] { if (self) self->delete_from_cursor (true, false); })
        ->setEnabled (sel && !ro);
    menu.addSeparator ();
    menu.addAction (QIcon::fromTheme ("edit-select-all"), QStringLiteral ("Select &All"), [self] { if (self) self->selectAll (); });
    menu.exec (e->globalPos ());
    g_object_unref (gwid);
  }

  void resizeEvent (QResizeEvent *e) override
  {
    QTextEdit::resizeEvent (e);
    layout_borders ();
  }
};

GtkQtTextBorder::GtkQtTextBorder (GtkQtTextEdit *e, GtkTextWindowType t) : QWidget (e), edit (e), type (t)
{
  gdkwin = gtkqt_drawable_new (GTKQT_DRAWABLE_TEXT_BORDER, e->gw);
  gdkwin->text_window_type = t;
  gdkwin->qw = this;
  setAutoFillBackground (false);
}

GtkQtTextBorder::~GtkQtTextBorder ()
{
  gdkwin->qw = nullptr;
  gdkwin->widget = nullptr;
  g_object_unref (gdkwin);
}

void GtkQtTextBorder::paintEvent (QPaintEvent *e)
{
  qreal dpr = devicePixelRatioF ();
  QImage img (size () * dpr, QImage::Format_ARGB32_Premultiplied);
  img.setDevicePixelRatio (dpr);
  img.fill (palette ().color (QPalette::Window));
  GtkTextView *v = edit->view ();
  if (v) {
    cairo_surface_t *s = cairo_image_surface_create_for_data (img.bits (), CAIRO_FORMAT_ARGB32, img.width (), img.height (), img.bytesPerLine ());
    cairo_surface_set_device_scale (s, dpr, dpr);
    gdkwin->surface = s;
    GtkWidget *w = GTK_WIDGET (v);
    {
      GtkQtEmitGuard guard;
      g_object_ref (w);
      GdkEvent ev = {};
      ev.expose.type = GDK_EXPOSE;
      ev.expose.window = gdkwin;
      ev.expose.area = { e->rect ().x (), e->rect ().y (), e->rect ().width (), e->rect ().height () };
      ev.expose.count = 0;
      gtkqt_emit_event (w, &ev, "expose-event");
      g_object_unref (w);
    }
    gdkwin->surface = nullptr;
    cairo_surface_flush (s);
    cairo_surface_destroy (s);
  }
  QPainter p (this);
  p.drawImage (0, 0, img);
}

static GtkQtTextEdit *text_edit (GtkTextView *v) { return v ? dynamic_cast<GtkQtTextEdit *> (gtkqt_q<QTextEdit> (GTK_WIDGET (v))) : nullptr; }

static QObject *text_view_create (GtkWidget *w)
{
  auto *e = new GtkQtTextEdit (w);
  return e;
}

static void text_view_dispose (GObject *o);

G_DEFINE_TYPE (GtkTextView, gtk_text_view, GTK_TYPE_CONTAINER)
static void gtk_text_view_init (GtkTextView *v)
{
  v->im_context = (GtkIMContext *) g_object_new (GTK_TYPE_IM_CONTEXT, nullptr);
  v->editable = TRUE;
  v->cursor_visible = TRUE;
  v->accepts_tab = TRUE;
}

static void text_view_constructed (GObject *o)
{
  G_OBJECT_CLASS (gtk_text_view_parent_class)->constructed (o);
  if (!view_buffer (GTK_TEXT_VIEW (o))) {
    GtkTextBuffer *b = gtk_text_buffer_new (nullptr);
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (o), b);
    g_object_unref (b);
  }
}

static void view_cut (GtkTextView *v)
{
  gtk_text_buffer_cut_clipboard (view_buffer (v), gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), gtk_text_view_get_editable (v));
  if (auto *e = text_edit (v)) e->ensureCursorVisible ();
}
static void view_copy (GtkTextView *v) { gtk_text_buffer_copy_clipboard (view_buffer (v), gtk_clipboard_get (GDK_SELECTION_CLIPBOARD)); }
static void view_paste (GtkTextView *v)
{
  gtk_text_buffer_paste_clipboard (view_buffer (v), gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), nullptr, gtk_text_view_get_editable (v));
  if (auto *e = text_edit (v)) e->ensureCursorVisible ();
}

static void gtk_text_view_class_init (GtkTextViewClass *klass)
{
  G_OBJECT_CLASS (klass)->constructed = text_view_constructed;
  G_OBJECT_CLASS (klass)->dispose = text_view_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), text_view_create);
  GType t = G_TYPE_FROM_CLASS (klass);
  GSignalFlags action = (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION);
  g_signal_new ("cut-clipboard", t, action, G_STRUCT_OFFSET (GtkTextViewClass, cut_clipboard), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("copy-clipboard", t, action, G_STRUCT_OFFSET (GtkTextViewClass, copy_clipboard), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("paste-clipboard", t, action, G_STRUCT_OFFSET (GtkTextViewClass, paste_clipboard), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("toggle-overwrite", t, action, G_STRUCT_OFFSET (GtkTextViewClass, toggle_overwrite), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("backspace", t, action, G_STRUCT_OFFSET (GtkTextViewClass, backspace), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("insert-at-cursor", t, action, G_STRUCT_OFFSET (GtkTextViewClass, insert_at_cursor), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 1, G_TYPE_STRING);
  g_signal_new ("select-all", t, action, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
  g_signal_new ("set-anchor", t, action, G_STRUCT_OFFSET (GtkTextViewClass, set_anchor), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("populate-popup", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextViewClass, populate_popup), nullptr, nullptr, nullptr,
                G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
  klass->cut_clipboard = view_cut;
  klass->copy_clipboard = view_copy;
  klass->paste_clipboard = view_paste;
  klass->toggle_overwrite = [] (GtkTextView *v) { gtk_text_view_set_overwrite (v, !gtk_text_view_get_overwrite (v)); };
  klass->backspace = [] (GtkTextView *v) { if (auto *e = text_edit (v)) e->delete_from_cursor (false, false); };
  klass->insert_at_cursor = [] (GtkTextView *v, const gchar *str) { if (auto *e = text_edit (v)) e->commit (gtkqt_str (str)); };
  g_signal_override_class_handler ("select-all", t, G_CALLBACK (+[] (GtkTextView *v, gboolean select) {
    if (auto *e = text_edit (v)) { if (select) e->selectAll (); else { QTextCursor c = e->textCursor (); c.clearSelection (); e->setTextCursor (c); } }
  }));
  gtkqt_prop (klass, g_param_spec_object ("buffer", nullptr, nullptr, GTK_TYPE_TEXT_BUFFER, RW),
              [] (GObject *o, const GValue *v) { if (auto *b = (GtkTextBuffer *) g_value_get_object (v)) gtk_text_view_set_buffer (GTK_TEXT_VIEW (o), b); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, view_buffer (GTK_TEXT_VIEW (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("editable", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_editable (GTK_TEXT_VIEW (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_text_view_get_editable (GTK_TEXT_VIEW (o))); });
  gtkqt_prop (klass, g_param_spec_enum ("wrap-mode", nullptr, nullptr, gtk_wrap_mode_get_type (), GTK_WRAP_NONE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (o), (GtkWrapMode) g_value_get_enum (v)); },
              [] (GObject *o, GValue *v) { g_value_set_enum (v, GTK_TEXT_VIEW (o)->wrap_mode); });
  gtkqt_prop (klass, g_param_spec_boolean ("cursor-visible", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, GTK_TEXT_VIEW (o)->cursor_visible); });
  gtkqt_prop (klass, g_param_spec_boolean ("overwrite", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_overwrite (GTK_TEXT_VIEW (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_text_view_get_overwrite (GTK_TEXT_VIEW (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("monospace", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_monospace (GTK_TEXT_VIEW (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("justification", nullptr, nullptr, gtk_justification_get_type (), GTK_JUSTIFY_LEFT, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_justification (GTK_TEXT_VIEW (o), (GtkJustification) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("left-margin", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_left_margin (GTK_TEXT_VIEW (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("right-margin", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_right_margin (GTK_TEXT_VIEW (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("top-margin", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_top_margin (GTK_TEXT_VIEW (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("bottom-margin", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_bottom_margin (GTK_TEXT_VIEW (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("accepts-tab", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (o), g_value_get_boolean (v)); }, nullptr);
}

static void text_view_dispose (GObject *o)
{
  GtkTextView *v = GTK_TEXT_VIEW (o);
  if (GtkTextBuffer *b = view_buffer (v)) {
    auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (v));
    if (e) {
      bd (b)->views.removeAll (e);
      QObject::disconnect (e, &QTextEdit::cursorPositionChanged, nullptr, nullptr);
      /* the QTextEdit is deleted later; it must not keep pointing at our document */
      e->setDocument (new QTextDocument (e));
    }
    v->buffer = nullptr;
    g_object_unref (b);
  }
  if (v->im_context) { g_object_unref (v->im_context); v->im_context = nullptr; }
  g_object_set_data (o, "gtkqt-tabs", nullptr);
  G_OBJECT_CLASS (gtk_text_view_parent_class)->dispose (o);
}

static void text_margins (GtkTextView *v, int which, int px)
{
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (v));
  if (!e) return;
  QTextFrameFormat f = e->document ()->rootFrame ()->frameFormat ();
  switch (which) {
  case 0: f.setLeftMargin (px); v->left_margin = px; break;
  case 1: f.setRightMargin (px); v->right_margin = px; break;
  case 2: f.setTopMargin (px); break;
  default: f.setBottomMargin (px); break;
  }
  e->document ()->rootFrame ()->setFrameFormat (f);
}

/* coordinates: buffer = document coordinates; text window = viewport */
static QPoint scroll_offset (QTextEdit *e) { return QPoint (e->horizontalScrollBar ()->value (), e->verticalScrollBar ()->value ()); }

static QPoint window_origin (GtkQtTextEdit *e, GtkTextWindowType win)
{
  /* the window's top-left, in viewport coordinates */
  QRect vp = e->viewport ()->geometry ();
  switch (win) {
  case GTK_TEXT_WINDOW_WIDGET: return -vp.topLeft ();
  case GTK_TEXT_WINDOW_LEFT: return QPoint (-e->sizes[0], 0);
  case GTK_TEXT_WINDOW_RIGHT: return QPoint (vp.width (), 0);
  case GTK_TEXT_WINDOW_TOP: return QPoint (0, -e->sizes[2]);
  case GTK_TEXT_WINDOW_BOTTOM: return QPoint (0, vp.height ());
  default: return QPoint (0, 0);
  }
}

extern "C" {

GtkWidget *gtk_text_view_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_TEXT_VIEW, nullptr); }
GtkWidget *gtk_text_view_new_with_buffer (GtkTextBuffer *buffer) { return (GtkWidget *) g_object_new (GTK_TYPE_TEXT_VIEW, "buffer", buffer, nullptr); }

void gtk_text_view_set_buffer (GtkTextView *view, GtkTextBuffer *buffer)
{
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view));
  if (GtkTextBuffer *old = view_buffer (view)) {
    if (old == buffer) return;
    if (e) {
      bd (old)->views.removeAll (e);
      QObject::disconnect (e, &QTextEdit::cursorPositionChanged, nullptr, nullptr);
      QObject::disconnect (bdoc (old), nullptr, e, nullptr);
    }
    view->buffer = nullptr;
    g_object_unref (old);
  }
  if (!buffer) return;
  g_object_ref (buffer);
  view->buffer = buffer;
  if (e) {
    bool ro = e->isReadOnly ();
    e->setDocument (bdoc (buffer));
    e->setReadOnly (ro);
    bd (buffer)->views.append (e);
    GtkTextBuffer *b = buffer;
    QObject::connect (e, &QTextEdit::cursorPositionChanged, e, [b, e] {
      GtkQtEmitGuard guard;
      BufferData *d = bd (b);
      d->insert = e->textCursor ().position ();
      d->bound = e->textCursor ().anchor ();
      d->insert_mark->offset = d->insert;
      d->bound_mark->offset = d->bound;
      GtkTextIter it;
      iter_init (&it, b, d->insert);
      g_signal_emit_by_name (b, "mark-set", &it, d->insert_mark);
    });
    if (auto *te = dynamic_cast<GtkQtTextEdit *> (e))
      QObject::connect (bdoc (buffer), &QTextDocument::contentsChanged, e, [te] { te->update_borders (true, true); });
  }
  g_object_notify (G_OBJECT (view), "buffer");
}

GtkTextBuffer *gtk_text_view_get_buffer (GtkTextView *view) { return view_buffer (view); }
void gtk_text_view_set_editable (GtkTextView *view, gboolean setting)
{
  view->editable = setting;
  if (auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view))) e->setReadOnly (!setting);
}
gboolean gtk_text_view_get_editable (GtkTextView *view) { auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view)); return e && !e->isReadOnly (); }

void gtk_text_view_set_wrap_mode (GtkTextView *view, GtkWrapMode wrap_mode)
{
  view->wrap_mode = wrap_mode;
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view));
  if (!e) return;
  if (wrap_mode == GTK_WRAP_NONE) { e->setLineWrapMode (QTextEdit::NoWrap); return; }
  e->setLineWrapMode (QTextEdit::WidgetWidth);
  e->setWordWrapMode (wrap_mode == GTK_WRAP_CHAR ? QTextOption::WrapAnywhere
                      : wrap_mode == GTK_WRAP_WORD ? QTextOption::WordWrap : QTextOption::WrapAtWordBoundaryOrAnywhere);
}
GtkWrapMode gtk_text_view_get_wrap_mode (GtkTextView *view) { return view->wrap_mode; }

void gtk_text_view_set_cursor_visible (GtkTextView *view, gboolean setting)
{
  view->cursor_visible = setting;
  if (auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view))) e->setCursorWidth (setting ? 1 : 0);
}
gboolean gtk_text_view_get_cursor_visible (GtkTextView *view) { return view->cursor_visible; }

void gtk_text_view_set_overwrite (GtkTextView *view, gboolean overwrite)
{
  view->overwrite_mode = overwrite;
  if (auto *e = text_edit (view)) { e->overwrite = overwrite; e->setOverwriteMode (overwrite); }
}
gboolean gtk_text_view_get_overwrite (GtkTextView *view) { return view->overwrite_mode; }

void gtk_text_view_set_monospace (GtkTextView *view, gboolean monospace)
{
  if (auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view)))
    e->setFont (monospace ? QFontDatabase::systemFont (QFontDatabase::FixedFont) : QApplication::font ());
}

void gtk_text_view_set_justification (GtkTextView *view, GtkJustification j)
{
  view->justify = j;
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view));
  if (!e) return;
  QTextOption opt = e->document ()->defaultTextOption ();
  opt.setAlignment (j == GTK_JUSTIFY_RIGHT ? Qt::AlignRight : j == GTK_JUSTIFY_CENTER ? Qt::AlignHCenter
                    : j == GTK_JUSTIFY_FILL ? Qt::AlignJustify : Qt::AlignLeft);
  e->document ()->setDefaultTextOption (opt);
}

void gtk_text_view_set_left_margin (GtkTextView *view, gint m) { text_margins (view, 0, m); }
void gtk_text_view_set_right_margin (GtkTextView *view, gint m) { text_margins (view, 1, m); }
void gtk_text_view_set_top_margin (GtkTextView *view, gint m) { text_margins (view, 2, m); }
void gtk_text_view_set_bottom_margin (GtkTextView *view, gint m) { text_margins (view, 3, m); }
gint gtk_text_view_get_left_margin (GtkTextView *view) { return view->left_margin; }
gint gtk_text_view_get_right_margin (GtkTextView *view) { return view->right_margin; }
void gtk_text_view_set_indent (GtkTextView *view, gint indent) { view->indent = indent; }
void gtk_text_view_set_pixels_above_lines (GtkTextView *view, gint p) { view->pixels_above_lines = p; }
void gtk_text_view_set_pixels_below_lines (GtkTextView *view, gint p) { view->pixels_below_lines = p; }
void gtk_text_view_set_accepts_tab (GtkTextView *view, gboolean accepts_tab)
{
  view->accepts_tab = accepts_tab;
  if (auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view))) e->setTabChangesFocus (!accepts_tab);
}

static void scroll_to_offset (GtkTextView *view, int offset, double margin = 0, bool use_align = false, double yalign = 0.5)
{
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view));
  if (!e) return;
  QTextCursor c (e->document ());
  c.setPosition (qBound (0, offset, e->document ()->characterCount () - 1));
  QRect r = e->cursorRect (c);
  QRect vp = e->viewport ()->rect ();
  int m = (int) (margin * vp.height ());
  QScrollBar *sb = e->verticalScrollBar ();
  if (use_align) sb->setValue (sb->value () + r.top () - (int) ((vp.height () - r.height ()) * yalign));
  else if (r.bottom () > vp.bottom () - m) sb->setValue (sb->value () + r.bottom () - vp.bottom () + m);
  else if (r.top () < vp.top () + m) sb->setValue (sb->value () + r.top () - vp.top () - m);
  QScrollBar *hb = e->horizontalScrollBar ();
  if (r.right () > vp.right ()) hb->setValue (hb->value () + r.right () - vp.right ());
  else if (r.left () < vp.left ()) hb->setValue (hb->value () + r.left () - vp.left ());
}

void gtk_text_view_scroll_to_mark (GtkTextView *view, GtkTextMark *mark, gdouble within_margin, gboolean use_align, gdouble, gdouble yalign)
{
  if (!mark) return;
  if (GtkTextBuffer *b = view_buffer (view)) {
    GtkTextIter it;
    gtk_text_buffer_get_iter_at_mark (b, &it, mark);
    scroll_to_offset (view, it.offset, within_margin, use_align, yalign);
  }
}

gboolean gtk_text_view_scroll_to_iter (GtkTextView *view, GtkTextIter *iter, gdouble within_margin, gboolean use_align, gdouble, gdouble yalign)
{
  scroll_to_offset (view, iter->offset, within_margin, use_align, yalign);
  return TRUE;
}

void gtk_text_view_scroll_mark_onscreen (GtkTextView *view, GtkTextMark *mark) { gtk_text_view_scroll_to_mark (view, mark, 0, FALSE, 0, 0); }

GdkWindow *gtk_text_view_get_window (GtkTextView *view, GtkTextWindowType win)
{
  GtkQtTextEdit *e = text_edit (view);
  if (!e) return nullptr;
  if (win == GTK_TEXT_WINDOW_WIDGET) return gtkqt_gdk_window (GTK_WIDGET (view));
  if (win == GTK_TEXT_WINDOW_TEXT) {
    if (!e->text_window) {
      e->text_window = gtkqt_drawable_new (GTKQT_DRAWABLE_TEXT_BORDER, GTK_WIDGET (view));
      e->text_window->text_window_type = GTK_TEXT_WINDOW_TEXT;
      e->text_window->qw = e->viewport ();
    }
    return e->text_window;
  }
  int i = border_index (win);
  return i >= 0 && e->borders[i] && e->sizes[i] > 0 ? e->borders[i]->gdkwin : nullptr;
}

GtkTextWindowType gtk_text_view_get_window_type (GtkTextView *view, GdkWindow *window)
{
  GtkQtTextEdit *e = text_edit (view);
  if (!e || !window) return GTK_TEXT_WINDOW_PRIVATE;
  if (window == GTK_WIDGET (view)->window) return GTK_TEXT_WINDOW_WIDGET;
  if (window == e->text_window) return GTK_TEXT_WINDOW_TEXT;
  for (GtkQtTextBorder *b : e->borders)
    if (b && b->gdkwin == window) return b->type;
  return GTK_TEXT_WINDOW_PRIVATE;
}

void gtk_text_view_set_border_window_size (GtkTextView *view, GtkTextWindowType type, gint size)
{
  int i = border_index (type);
  if (GtkQtTextEdit *e = text_edit (view); e && i >= 0) e->set_border (i, size);
}

gint gtk_text_view_get_border_window_size (GtkTextView *view, GtkTextWindowType type)
{
  int i = border_index (type);
  GtkQtTextEdit *e = text_edit (view);
  return e && i >= 0 ? e->sizes[i] : 0;
}

void gtk_text_view_buffer_to_window_coords (GtkTextView *view, GtkTextWindowType win, gint bx, gint by, gint *wx, gint *wy)
{
  GtkQtTextEdit *e = text_edit (view);
  QPoint p (bx, by);
  if (e) p = p - scroll_offset (e) - window_origin (e, win);
  if (wx) *wx = p.x ();
  if (wy) *wy = p.y ();
}

void gtk_text_view_window_to_buffer_coords (GtkTextView *view, GtkTextWindowType win, gint wx, gint wy, gint *bx, gint *by)
{
  GtkQtTextEdit *e = text_edit (view);
  QPoint p (wx, wy);
  if (e) p = p + window_origin (e, win) + scroll_offset (e);
  if (bx) *bx = p.x ();
  if (by) *by = p.y ();
}

void gtk_text_view_get_visible_rect (GtkTextView *view, GdkRectangle *r)
{
  GtkQtTextEdit *e = text_edit (view);
  if (!r) return;
  if (!e) { *r = { 0, 0, 0, 0 }; return; }
  QPoint o = scroll_offset (e);
  *r = { o.x (), o.y (), e->viewport ()->width (), e->viewport ()->height () };
}

void gtk_text_view_get_iter_location (GtkTextView *view, const GtkTextIter *iter, GdkRectangle *location)
{
  GtkQtTextEdit *e = text_edit (view);
  if (!e || !location) return;
  QTextCursor c (e->document ());
  c.setPosition (qBound (0, iter->offset, e->document ()->characterCount () - 1));
  QRect r = e->cursorRect (c).translated (scroll_offset (e));
  int w = 0;
  if (iter->offset < e->document ()->characterCount () - 1) {
    QChar ch = e->document ()->characterAt (iter->offset);
    if (ch != QChar::ParagraphSeparator) w = QFontMetrics (e->font ()).horizontalAdvance (ch);
  }
  *location = { r.x (), r.y (), w, r.height () };
}

void gtk_text_view_get_iter_at_location (GtkTextView *view, GtkTextIter *iter, gint x, gint y)
{
  GtkQtTextEdit *e = text_edit (view);
  GtkTextBuffer *b = view_buffer (view);
  int pos = e ? e->document ()->documentLayout ()->hitTest (QPointF (x, y), Qt::FuzzyHit) : 0;
  iter_init (iter, b, qMax (0, pos));
}

void gtk_text_view_get_line_at_y (GtkTextView *view, GtkTextIter *target, gint y, gint *line_top)
{
  GtkQtTextEdit *e = text_edit (view);
  GtkTextBuffer *b = view_buffer (view);
  if (!e) { iter_init (target, b, 0); if (line_top) *line_top = 0; return; }
  QTextDocument *doc = e->document ();
  QAbstractTextDocumentLayout *lay = doc->documentLayout ();
  QTextBlock blk;
  if (y <= 0) blk = doc->firstBlock ();
  else {
    int pos = lay->hitTest (QPointF (doc->documentMargin () + 1, y), Qt::FuzzyHit);
    blk = doc->findBlock (qMax (0, pos));
    if (!blk.isValid ()) blk = doc->lastBlock ();
    /* hitTest snaps to the nearest line; step to the block that contains y */
    while (blk.isValid () && blk.next ().isValid () && lay->blockBoundingRect (blk).bottom () <= y) blk = blk.next ();
    while (blk.isValid () && blk.previous ().isValid () && lay->blockBoundingRect (blk).top () > y) blk = blk.previous ();
  }
  iter_init (target, b, blk.position ());
  if (line_top) *line_top = (int) lay->blockBoundingRect (blk).top ();
}

void gtk_text_view_get_line_yrange (GtkTextView *view, const GtkTextIter *iter, gint *y, gint *height)
{
  GtkQtTextEdit *e = text_edit (view);
  if (!e) { if (y) *y = 0; if (height) *height = 0; return; }
  QTextBlock blk = e->document ()->findBlock (iter->offset);
  if (!blk.isValid ()) blk = e->document ()->lastBlock ();
  QRectF r = e->document ()->documentLayout ()->blockBoundingRect (blk);
  if (y) *y = (int) r.top ();
  if (height) *height = (int) std::ceil (r.height ());
}

gboolean gtk_text_view_move_mark_onscreen (GtkTextView *view, GtkTextMark *mark)
{
  GtkTextBuffer *b = view_buffer (view);
  if (!b || !mark) return FALSE;
  GtkTextIter it;
  gtk_text_buffer_get_iter_at_mark (b, &it, mark);
  GdkRectangle vis, loc;
  gtk_text_view_get_visible_rect (view, &vis);
  gtk_text_view_get_iter_location (view, &it, &loc);
  if (loc.y >= vis.y && loc.y + loc.height <= vis.y + vis.height) return FALSE;
  int y = loc.y < vis.y ? vis.y : vis.y + vis.height - qMax (1, loc.height);
  GtkTextIter n;
  gtk_text_view_get_iter_at_location (view, &n, loc.x, y);
  gtk_text_buffer_move_mark (b, mark, &n);
  return TRUE;
}

gboolean gtk_text_view_place_cursor_onscreen (GtkTextView *view)
{
  GtkTextBuffer *b = view_buffer (view);
  if (!b) return FALSE;
  GtkTextMark *ins = gtk_text_buffer_get_insert (b);
  GtkTextIter it;
  gtk_text_buffer_get_iter_at_mark (b, &it, ins);
  GdkRectangle vis, loc;
  gtk_text_view_get_visible_rect (view, &vis);
  gtk_text_view_get_iter_location (view, &it, &loc);
  if (loc.y >= vis.y && loc.y + loc.height <= vis.y + vis.height) return FALSE;
  GtkTextIter n;
  gtk_text_view_get_iter_at_location (view, &n, loc.x, loc.y < vis.y ? vis.y : vis.y + vis.height - qMax (1, loc.height));
  gtk_text_buffer_place_cursor (b, &n);
  return TRUE;
}

void gtk_text_view_set_tabs (GtkTextView *view, PangoTabArray *tabs)
{
  g_object_set_data_full (G_OBJECT (view), "gtkqt-tabs", tabs ? pango_tab_array_copy (tabs) : nullptr,
                          (GDestroyNotify) pango_tab_array_free);
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view));
  if (!e) return;
  qreal dist = 8 * QFontMetricsF (e->font ()).horizontalAdvance (' ');
  if (tabs && pango_tab_array_get_size (tabs) > 0) {
    gint loc = 0;
    pango_tab_array_get_tab (tabs, 0, nullptr, &loc);
    dist = pango_tab_array_get_positions_in_pixels (tabs) ? loc : loc / (double) PANGO_SCALE;
  }
  if (dist > 0) e->setTabStopDistance (dist);
}

PangoTabArray *gtk_text_view_get_tabs (GtkTextView *view)
{
  auto *t = (PangoTabArray *) g_object_get_data (G_OBJECT (view), "gtkqt-tabs");
  return t ? pango_tab_array_copy (t) : nullptr;
}

} /* extern "C" */
