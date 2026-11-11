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
  d->doc->setDocumentMargin (4);
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

static void gtk_text_buffer_class_init (GtkTextBufferClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = gtk_text_buffer_finalize;
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("changed", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, changed), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("modified-changed", t, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkTextBufferClass, modified_changed),
                nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("mark-set", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_OBJECT);
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
  QString s = len < 0 ? gtkqt_str (text) : QString::fromUtf8 (text, len);
  bdoc (b)->setPlainText (s);
  BufferData *d = bd (b);
  d->insert = d->bound = 0;
  buffer_push_cursor (b);
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

static void insert_formatted (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len, const QList<GtkTextTag *> &tags)
{
  QString s = len < 0 ? gtkqt_str (text) : QString::fromUtf8 (text, len);
  QTextCursor c (bdoc (b));
  int pos = qBound (0, iter_off (iter), end_offset (b));
  c.setPosition (pos);
  QTextCharFormat f;
  for (GtkTextTag *t : tags) f.merge (tag_fmt (t));
  c.insertText (s, tags.isEmpty () ? QTextCharFormat () : f);
  buffer_pull_cursor (b);
  if (iter) iter_init (iter, b, pos + s.size ());
}

void gtk_text_buffer_insert (GtkTextBuffer *b, GtkTextIter *iter, const gchar *text, gint len) { insert_formatted (b, iter, text, len, {}); }

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
  return mark_new (b, mark_name, iter_off (where), left_gravity);
}

void gtk_text_buffer_move_mark (GtkTextBuffer *b, GtkTextMark *mark, const GtkTextIter *where)
{
  BufferData *d = bd (b);
  if (mark == d->insert_mark) { buffer_pull_cursor (b); d->insert = iter_off (where); buffer_push_cursor (b); }
  else if (mark == d->bound_mark) { buffer_pull_cursor (b); d->bound = iter_off (where); buffer_push_cursor (b); }
  else mark->offset = iter_off (where);
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
  if (bd (b)->user_action++ == 0) QTextCursor (bdoc (b)).beginEditBlock ();
}

void gtk_text_buffer_end_user_action (GtkTextBuffer *b)
{
  if (--bd (b)->user_action == 0) QTextCursor (bdoc (b)).endEditBlock ();
}

void gtk_text_buffer_undo (GtkTextBuffer *b) { bdoc (b)->undo (); }
void gtk_text_buffer_redo (GtkTextBuffer *b) { bdoc (b)->redo (); }

static QTextEdit *first_view (GtkTextBuffer *b)
{
  for (const QPointer<QTextEdit> &v : bd (b)->views) if (v) return v;
  return nullptr;
}

void gtk_text_buffer_cut_clipboard (GtkTextBuffer *b, gpointer, gboolean) { if (QTextEdit *v = first_view (b)) v->cut (); }
void gtk_text_buffer_copy_clipboard (GtkTextBuffer *b, gpointer) { if (QTextEdit *v = first_view (b)) v->copy (); }
void gtk_text_buffer_paste_clipboard (GtkTextBuffer *b, gpointer, GtkTextIter *, gboolean) { if (QTextEdit *v = first_view (b)) v->paste (); }

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

} /* extern "C" */

/* ======================================================================
 * GtkTextView
 * ====================================================================== */
static GtkTextBuffer *view_buffer (GtkTextView *v) { return (GtkTextBuffer *) g_object_get_data (G_OBJECT (v), "gtkqt-buffer"); }

static QObject *text_view_create (GtkWidget *)
{
  auto *e = new QTextEdit;
  e->setAcceptRichText (false);
  return e;
}

static void text_view_dispose (GObject *o);

G_DEFINE_TYPE (GtkTextView, gtk_text_view, GTK_TYPE_CONTAINER)
static void gtk_text_view_init (GtkTextView *) {}

static void text_view_constructed (GObject *o)
{
  G_OBJECT_CLASS (gtk_text_view_parent_class)->constructed (o);
  if (!view_buffer (GTK_TEXT_VIEW (o))) {
    GtkTextBuffer *b = gtk_text_buffer_new (nullptr);
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (o), b);
    g_object_unref (b);
  }
}

static void gtk_text_view_class_init (GtkTextViewClass *klass)
{
  G_OBJECT_CLASS (klass)->constructed = text_view_constructed;
  G_OBJECT_CLASS (klass)->dispose = text_view_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), text_view_create);
  gtkqt_prop (klass, g_param_spec_object ("buffer", nullptr, nullptr, GTK_TYPE_TEXT_BUFFER, RW),
              [] (GObject *o, const GValue *v) { if (auto *b = (GtkTextBuffer *) g_value_get_object (v)) gtk_text_view_set_buffer (GTK_TEXT_VIEW (o), b); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, view_buffer (GTK_TEXT_VIEW (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("editable", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_editable (GTK_TEXT_VIEW (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_text_view_get_editable (GTK_TEXT_VIEW (o))); });
  gtkqt_prop (klass, g_param_spec_enum ("wrap-mode", nullptr, nullptr, gtk_wrap_mode_get_type (), GTK_WRAP_NONE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (o), (GtkWrapMode) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("cursor-visible", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (o), g_value_get_boolean (v)); }, nullptr);
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
      /* the QTextEdit is deleted later; it must not keep pointing at our document */
      e->setDocument (new QTextDocument (e));
    }
    g_object_set_data (o, "gtkqt-buffer", nullptr);
    g_object_unref (b);
  }
  G_OBJECT_CLASS (gtk_text_view_parent_class)->dispose (o);
}

static void text_margins (GtkTextView *v, int which, int px)
{
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (v));
  if (!e) return;
  QTextFrameFormat f = e->document ()->rootFrame ()->frameFormat ();
  switch (which) {
  case 0: f.setLeftMargin (px); break;
  case 1: f.setRightMargin (px); break;
  case 2: f.setTopMargin (px); break;
  default: f.setBottomMargin (px); break;
  }
  e->document ()->rootFrame ()->setFrameFormat (f);
}

extern "C" {

GtkWidget *gtk_text_view_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_TEXT_VIEW, nullptr); }
GtkWidget *gtk_text_view_new_with_buffer (GtkTextBuffer *buffer) { return (GtkWidget *) g_object_new (GTK_TYPE_TEXT_VIEW, "buffer", buffer, nullptr); }

void gtk_text_view_set_buffer (GtkTextView *view, GtkTextBuffer *buffer)
{
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view));
  if (GtkTextBuffer *old = view_buffer (view)) {
    if (old == buffer) return;
    if (e) bd (old)->views.removeAll (e);
    g_object_set_data (G_OBJECT (view), "gtkqt-buffer", nullptr);
    g_object_unref (old);
  }
  if (!buffer) return;
  g_object_ref (buffer);
  g_object_set_data (G_OBJECT (view), "gtkqt-buffer", buffer);
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
  }
}

GtkTextBuffer *gtk_text_view_get_buffer (GtkTextView *view) { return view_buffer (view); }
void gtk_text_view_set_editable (GtkTextView *view, gboolean setting) { if (auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view))) e->setReadOnly (!setting); }
gboolean gtk_text_view_get_editable (GtkTextView *view) { auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view)); return e && !e->isReadOnly (); }

void gtk_text_view_set_wrap_mode (GtkTextView *view, GtkWrapMode wrap_mode)
{
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view));
  if (!e) return;
  if (wrap_mode == GTK_WRAP_NONE) { e->setLineWrapMode (QTextEdit::NoWrap); return; }
  e->setLineWrapMode (QTextEdit::WidgetWidth);
  e->setWordWrapMode (wrap_mode == GTK_WRAP_CHAR ? QTextOption::WrapAnywhere
                      : wrap_mode == GTK_WRAP_WORD ? QTextOption::WordWrap : QTextOption::WrapAtWordBoundaryOrAnywhere);
}

void gtk_text_view_set_cursor_visible (GtkTextView *view, gboolean setting)
{
  if (auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view))) e->setCursorWidth (setting ? 1 : 0);
}

void gtk_text_view_set_monospace (GtkTextView *view, gboolean monospace)
{
  if (auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view)))
    e->setFont (monospace ? QFontDatabase::systemFont (QFontDatabase::FixedFont) : QApplication::font ());
}

void gtk_text_view_set_justification (GtkTextView *view, GtkJustification j)
{
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
void gtk_text_view_set_pixels_above_lines (GtkTextView *, gint) {}
void gtk_text_view_set_pixels_below_lines (GtkTextView *, gint) {}
void gtk_text_view_set_accepts_tab (GtkTextView *view, gboolean accepts_tab) { if (auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view))) e->setTabChangesFocus (!accepts_tab); }

static void scroll_to_offset (GtkTextView *view, int offset)
{
  auto *e = gtkqt_q<QTextEdit> (GTK_WIDGET (view));
  if (!e) return;
  QTextCursor c (e->document ());
  c.setPosition (qBound (0, offset, e->document ()->characterCount () - 1));
  QRect r = e->cursorRect (c);
  QRect vp = e->viewport ()->rect ();
  if (r.bottom () > vp.bottom ()) e->verticalScrollBar ()->setValue (e->verticalScrollBar ()->value () + r.bottom () - vp.bottom ());
  else if (r.top () < vp.top ()) e->verticalScrollBar ()->setValue (e->verticalScrollBar ()->value () + r.top () - vp.top ());
}

void gtk_text_view_scroll_to_mark (GtkTextView *view, GtkTextMark *mark, gdouble, gboolean, gdouble, gdouble)
{
  if (!mark) return;
  if (GtkTextBuffer *b = view_buffer (view)) {
    GtkTextIter it;
    gtk_text_buffer_get_iter_at_mark (b, &it, mark);
    scroll_to_offset (view, it.offset);
  }
}

gboolean gtk_text_view_scroll_to_iter (GtkTextView *view, GtkTextIter *iter, gdouble, gboolean, gdouble, gdouble)
{
  scroll_to_offset (view, iter->offset);
  return TRUE;
}

} /* extern "C" */
