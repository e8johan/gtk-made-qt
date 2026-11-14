/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Controls: labels, buttons, switches, entries, combo boxes, images...
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QDesktopServices>
#include <QTextDocumentFragment>
#include <QUrl>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

/* ======================================================================
 * GtkLabel
 * ====================================================================== */
class GtkQtLabel : public QLabel {
public:
  Qt::TextElideMode elide = Qt::ElideNone;
  QSize minimumSizeHint () const override
  {
    QSize s = QLabel::minimumSizeHint ();
    if (elide != Qt::ElideNone) s.setWidth (fontMetrics ().horizontalAdvance (QStringLiteral ("...")) + 4);
    return s;
  }
protected:
  void paintEvent (QPaintEvent *e) override
  {
    if (elide == Qt::ElideNone || textFormat () == Qt::RichText || wordWrap ()) { QLabel::paintEvent (e); return; }
    QPainter p (this);
    QRect r = contentsRect ();
    QString t = fontMetrics ().elidedText (text (), elide, r.width ());
    style ()->drawItemText (&p, r, alignment (), palette (), isEnabled (), t, foregroundRole ());
  }
};

struct LabelData {
  QString label;        /* as given (may contain markup / mnemonics) */
  bool markup = false, underline = false;
  GtkWidget *mnemonic_widget = nullptr;
};

static LabelData *label_data (GtkWidget *w)
{
  auto *d = static_cast<LabelData *> (g_object_get_data (G_OBJECT (w), "gtkqt-label"));
  if (!d) {
    d = new LabelData;
    g_object_set_data_full (G_OBJECT (w), "gtkqt-label", d, [] (gpointer p) { delete static_cast<LabelData *> (p); });
  }
  return d;
}

static QObject *label_create (GtkWidget *)
{
  auto *l = new GtkQtLabel;
  l->setAlignment (Qt::AlignCenter);
  l->setTextFormat (Qt::PlainText);
  return l;
}

static void label_mirror_to_button (GtkWidget *label);
static void label_refresh (GtkWidget *w)
{
  auto *l = gtkqt_q<QLabel> (w);
  LabelData *d = label_data (w);
  if (!l) return;
  if (g_object_get_data (G_OBJECT (w), "gtkqt-button-text")) label_mirror_to_button (w);
  QByteArray raw = d->label.toUtf8 ();
  if (d->markup) {
    QString html = gtkqt_markup_to_html (raw.constData ());
    if (d->underline) html.replace (QRegularExpression (QStringLiteral ("_(\\w)")), QStringLiteral ("<u>\\1</u>"));
    l->setTextFormat (Qt::RichText);
    l->setText (html);
  } else if (d->underline && d->mnemonic_widget) {
    l->setTextFormat (Qt::PlainText);
    l->setText (gtkqt_mnemonic (raw.constData ()));
    if (QWidget *buddy = gtkqt_qw (d->mnemonic_widget)) l->setBuddy (buddy);
  } else {
    l->setTextFormat (Qt::PlainText);
    QString t = d->label;
    if (d->underline) t = gtkqt_mnemonic (raw.constData ()).remove (QRegularExpression (QStringLiteral ("&(?!&)"))).replace (QStringLiteral ("&&"), QStringLiteral ("&"));
    l->setText (t);
  }
}

G_DEFINE_TYPE (GtkLabel, gtk_label, GTK_TYPE_WIDGET)
static void gtk_label_init (GtkLabel *) {}
static void gtk_label_class_init (GtkLabelClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), label_create);
  g_signal_new ("activate-link", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, g_signal_accumulator_true_handled,
                nullptr, nullptr, G_TYPE_BOOLEAN, 1, G_TYPE_STRING);
  gtkqt_prop (klass, g_param_spec_string ("label", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_label (GTK_LABEL (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, gtk_label_get_label (GTK_LABEL (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("use-markup", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_use_markup (GTK_LABEL (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("use-underline", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_use_underline (GTK_LABEL (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("wrap", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_wrap (GTK_LABEL (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("selectable", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_selectable (GTK_LABEL (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_float ("xalign", nullptr, nullptr, 0, 1, 0.5, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_xalign (GTK_LABEL (o), g_value_get_float (v)); },
              [] (GObject *o, GValue *v) { g_value_set_float (v, gtk_label_get_xalign (GTK_LABEL (o))); });
  gtkqt_prop (klass, g_param_spec_float ("yalign", nullptr, nullptr, 0, 1, 0.5, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_yalign (GTK_LABEL (o), g_value_get_float (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("justify", nullptr, nullptr, gtk_justification_get_type (), GTK_JUSTIFY_LEFT, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_justify (GTK_LABEL (o), (GtkJustification) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("ellipsize", nullptr, nullptr, pango_ellipsize_mode_get_type (), PANGO_ELLIPSIZE_NONE, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_ellipsize (GTK_LABEL (o), (PangoEllipsizeMode) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("width-chars", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_width_chars (GTK_LABEL (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("max-width-chars", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_max_width_chars (GTK_LABEL (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("mnemonic-widget", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_mnemonic_widget (GTK_LABEL (o), (GtkWidget *) g_value_get_object (v)); }, nullptr);
}

extern "C" {

GtkWidget *gtk_label_new (const gchar *str)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_LABEL, nullptr);
  gtk_label_set_text (GTK_LABEL (w), str);
  return w;
}

GtkWidget *gtk_label_new_with_mnemonic (const gchar *str)
{
  GtkWidget *w = gtk_label_new (nullptr);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (w), str);
  return w;
}

void gtk_label_set_text (GtkLabel *label, const gchar *str)
{
  LabelData *d = label_data (GTK_WIDGET (label));
  d->label = gtkqt_str (str);
  d->markup = false;
  d->underline = false;
  label_refresh (GTK_WIDGET (label));
  g_object_notify (G_OBJECT (label), "label");
}

const gchar *gtk_label_get_text (GtkLabel *label)
{
  LabelData *d = label_data (GTK_WIDGET (label));
  QString t = d->label;
  if (d->markup) t = QTextDocumentFragment::fromHtml (gtkqt_markup_to_html (d->label.toUtf8 ().constData ())).toPlainText ();
  if (d->underline) t.remove (QRegularExpression (QStringLiteral ("_(?!_)"))).replace (QStringLiteral ("__"), QStringLiteral ("_"));
  return gtkqt_ret (GTK_WIDGET (label), t);
}

void gtk_label_set_label (GtkLabel *label, const gchar *str)
{
  label_data (GTK_WIDGET (label))->label = gtkqt_str (str);
  label_refresh (GTK_WIDGET (label));
  g_object_notify (G_OBJECT (label), "label");
}

const gchar *gtk_label_get_label (GtkLabel *label)
{
  return gtkqt_ret (GTK_WIDGET (label), label_data (GTK_WIDGET (label))->label);
}

void gtk_label_set_markup (GtkLabel *label, const gchar *str)
{
  LabelData *d = label_data (GTK_WIDGET (label));
  d->label = gtkqt_str (str);
  d->markup = true;
  d->underline = false;
  label_refresh (GTK_WIDGET (label));
}

void gtk_label_set_markup_with_mnemonic (GtkLabel *label, const gchar *str)
{
  gtk_label_set_markup (label, str);
  label_data (GTK_WIDGET (label))->underline = true;
  label_refresh (GTK_WIDGET (label));
}

void gtk_label_set_text_with_mnemonic (GtkLabel *label, const gchar *str)
{
  gtk_label_set_text (label, str);
  label_data (GTK_WIDGET (label))->underline = true;
  label_refresh (GTK_WIDGET (label));
}

void gtk_label_set_use_markup (GtkLabel *label, gboolean setting)
{
  label_data (GTK_WIDGET (label))->markup = setting;
  label_refresh (GTK_WIDGET (label));
}

void gtk_label_set_use_underline (GtkLabel *label, gboolean setting)
{
  label_data (GTK_WIDGET (label))->underline = setting;
  label_refresh (GTK_WIDGET (label));
}

void gtk_label_set_mnemonic_widget (GtkLabel *label, GtkWidget *widget)
{
  label_data (GTK_WIDGET (label))->mnemonic_widget = widget;
  label_refresh (GTK_WIDGET (label));
}

static void label_set_halign (GtkLabel *label, Qt::Alignment h)
{
  if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (label))) l->setAlignment (h | (l->alignment () & Qt::AlignVertical_Mask));
}

void gtk_label_set_justify (GtkLabel *label, GtkJustification jtype)
{
  if (jtype == GTK_JUSTIFY_FILL) label_set_halign (label, Qt::AlignJustify);
  else if (jtype == GTK_JUSTIFY_RIGHT) label_set_halign (label, Qt::AlignRight);
  else if (jtype == GTK_JUSTIFY_LEFT) label_set_halign (label, Qt::AlignLeft);
  else label_set_halign (label, Qt::AlignHCenter);
}

void gtk_label_set_xalign (GtkLabel *label, gfloat xalign)
{
  g_object_set_data (G_OBJECT (label), "gtkqt-xalign", GINT_TO_POINTER ((int) (xalign * 1000)));
  label_set_halign (label, xalign < 0.25 ? Qt::AlignLeft : xalign > 0.75 ? Qt::AlignRight : Qt::AlignHCenter);
}

gfloat gtk_label_get_xalign (GtkLabel *label)
{
  gpointer p = g_object_get_data (G_OBJECT (label), "gtkqt-xalign");
  return p ? GPOINTER_TO_INT (p) / 1000.0f : 0.5f;
}

void gtk_label_set_yalign (GtkLabel *label, gfloat yalign)
{
  if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (label)))
    l->setAlignment ((l->alignment () & Qt::AlignHorizontal_Mask) |
                     (yalign < 0.25 ? Qt::AlignTop : yalign > 0.75 ? Qt::AlignBottom : Qt::AlignVCenter));
}

void gtk_label_set_line_wrap (GtkLabel *label, gboolean wrap) { gtk_label_set_wrap (label, wrap); }
void gtk_label_set_wrap (GtkLabel *label, gboolean wrap) { if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (label))) l->setWordWrap (wrap); }
void gtk_label_set_wrap_mode (GtkLabel *, PangoWrapMode) {}

void gtk_label_set_selectable (GtkLabel *label, gboolean setting)
{
  if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (label)))
    l->setTextInteractionFlags (setting ? Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard : Qt::LinksAccessibleByMouse);
}

void gtk_label_set_ellipsize (GtkLabel *label, PangoEllipsizeMode mode)
{
  if (auto *l = dynamic_cast<GtkQtLabel *> (gtkqt_obj (GTK_WIDGET (label)))) {
    l->elide = mode == PANGO_ELLIPSIZE_START ? Qt::ElideLeft : mode == PANGO_ELLIPSIZE_MIDDLE ? Qt::ElideMiddle
               : mode == PANGO_ELLIPSIZE_END ? Qt::ElideRight : Qt::ElideNone;
    l->updateGeometry ();
    l->update ();
  }
}

void gtk_label_set_width_chars (GtkLabel *label, gint n_chars)
{
  if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (label)))
    l->setMinimumWidth (n_chars > 0 ? l->fontMetrics ().averageCharWidth () * n_chars : 0);
}

void gtk_label_set_max_width_chars (GtkLabel *label, gint n_chars)
{
  if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (label)))
    l->setMaximumWidth (n_chars > 0 ? l->fontMetrics ().averageCharWidth () * n_chars : QWIDGETSIZE_MAX);
}

void gtk_label_set_pattern (GtkLabel *, const gchar *) {}

void gtk_misc_set_alignment (GtkMisc *misc, gfloat xalign, gfloat yalign)
{
  if (GTK_IS_LABEL (misc)) {
    gtk_label_set_xalign (GTK_LABEL (misc), xalign);
    gtk_label_set_yalign (GTK_LABEL (misc), yalign);
  } else if (auto *l = gtkqt_q<QLabel> (misc)) {
    l->setAlignment ((xalign < 0.25 ? Qt::AlignLeft : xalign > 0.75 ? Qt::AlignRight : Qt::AlignHCenter) |
                     (yalign < 0.25 ? Qt::AlignTop : yalign > 0.75 ? Qt::AlignBottom : Qt::AlignVCenter));
  }
}

void gtk_misc_set_padding (GtkMisc *misc, gint xpad, gint ypad)
{
  if (QWidget *qw = gtkqt_qw (misc)) qw->setContentsMargins (xpad, ypad, xpad, ypad);
}

} /* extern "C" */

/* ======================================================================
 * GtkButton family
 * ====================================================================== */
static void button_clicked_cb (GtkWidget *w)
{
  g_signal_emit_by_name (w, "clicked");
  if (w->priv->destroyed) return;
  if (const char *action = (const char *) g_object_get_data (G_OBJECT (w), "gtkqt-action-name"))
    gtkqt_activate_action (w, action, (GVariant *) g_object_get_data (G_OBJECT (w), "gtkqt-action-target"));
}

/* GTK's order on a click: "released" (whose default handler emits
 * "clicked", which toggles a toggle button) runs the program's released
 * handlers last. Qt emits released before clicked, so ours waits for the
 * clicked that follows (or for the event loop, when the pointer left). */
static void release_now (GtkWidget *w)
{
  if (!g_object_get_data (G_OBJECT (w), "gtkqt-release-pending")) return;
  g_object_set_data (G_OBJECT (w), "gtkqt-release-pending", nullptr);
  g_signal_emit_by_name (w, "released");
}

static void button_connect (GtkWidget *w, QAbstractButton *b)
{
  gtkqt_connect (w, b, &QAbstractButton::clicked, [] (GtkWidget *bw, bool) {
    button_clicked_cb (bw);
    if (!bw->priv->destroyed) release_now (bw);
  });
  gtkqt_connect (w, b, &QAbstractButton::pressed, [] (GtkWidget *bw) { g_signal_emit_by_name (bw, "pressed"); });
  gtkqt_connect (w, b, &QAbstractButton::released, [b] (GtkWidget *bw) {
    g_object_set_data (G_OBJECT (bw), "gtkqt-release-pending", GINT_TO_POINTER (1));
    GtkWidget *keep = GTK_WIDGET (g_object_ref (bw));
    QTimer::singleShot (0, b, [keep] {
      GtkQtEmitGuard guard;
      if (!keep->priv->destroyed) release_now (keep);
      g_object_unref (keep);
    });
  });
}

/* Toggle, check and radio buttons change state in GtkToggleButton's
 * "clicked" default handler, as in GTK, not by themselves: handlers of
 * "clicked" (and accelerators emitting it) then see the new state. */
template <class Base> class GtkQtToggle : public Base {
protected:
  void nextCheckState () override {}
};

static void toggle_real_clicked (GtkButton *button)
{
  auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (button));
  if (!b) return;
  if (GTK_IS_RADIO_BUTTON (button)) { if (!b->isChecked ()) b->setChecked (true); }
  else b->setChecked (!b->isChecked ());
}

static void toggle_connect (GtkWidget *w, QAbstractButton *b)
{
  b->setCheckable (true);
  gtkqt_connect (w, b, &QAbstractButton::toggled, [] (GtkWidget *bw, bool on) {
    GTK_TOGGLE_BUTTON (bw)->active = on;
    g_signal_emit_by_name (bw, "toggled");
    g_object_notify (G_OBJECT (bw), "active");
  });
}

static QObject *button_create (GtkWidget *w)
{
  auto *b = new QPushButton;
  button_connect (w, b);
  return b;
}

/* A label inside a button (gtk_button_new_with_label makes one, as GTK does;
 * GTK 1/2 programs reach it with GTK_BIN (button)->child) is shown as the
 * Qt button's own text: its QLabel stays out of the button. */
static bool syncing_button_label = false;
static void label_mirror_to_button (GtkWidget *label)
{
  if (syncing_button_label) return;
  auto *b = label->parent ? gtkqt_q<QAbstractButton> (label->parent) : nullptr;
  if (!b) return;
  LabelData *d = label_data (label);
  QByteArray raw = d->label.toUtf8 ();
  QString text = d->markup ? gtkqt_str (gtk_label_get_text (GTK_LABEL (label))) : d->label;
  if (d->underline && !d->markup) b->setText (gtkqt_mnemonic (raw.constData ()));
  else b->setText (text.replace ('&', QStringLiteral ("&&")));
}

static void button_place (GtkContainer *c, GtkWidget *child)
{
  auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!b || !cw) return;
  if (GTK_IS_LABEL (child)) {
    g_object_set_data (G_OBJECT (child), "gtkqt-button-text", GINT_TO_POINTER (1));
    cw->hide ();
    label_mirror_to_button (child);
    return;
  }
  if (!b->layout ()) {
    auto *l = new QHBoxLayout (b);
    l->setContentsMargins (8, 4, 8, 4);
    /* the button grows with what is packed into the child later */
    l->setSizeConstraint (QLayout::SetMinimumSize);
  }
  b->layout ()->addWidget (cw);
  cw->setAttribute (Qt::WA_TransparentForMouseEvents);
}

G_DEFINE_TYPE (GtkButton, gtk_button, GTK_TYPE_BIN)
static void gtk_button_init (GtkButton *) {}
static void gtk_button_class_init (GtkButtonClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), button_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), button_place, [] (GtkContainer *c, GtkWidget *child) {
    if (g_object_get_data (G_OBJECT (child), "gtkqt-button-text")) {
      g_object_set_data (G_OBJECT (child), "gtkqt-button-text", nullptr);
      if (auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (c))) b->setText (QString ());
      return;
    }
    gtkqt_bin_unplace_simple (c, child);
  });
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("clicked", t, (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION), G_STRUCT_OFFSET (GtkButtonClass, clicked),
                nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("pressed", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkButtonClass, pressed), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("released", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkButtonClass, released), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("enter", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkButtonClass, enter), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("leave", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkButtonClass, leave), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("activate", t, (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION), G_STRUCT_OFFSET (GtkButtonClass, activate),
                nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  klass->activate = [] (GtkButton *b) { gtk_button_clicked (b); };
  gtkqt_prop (klass, g_param_spec_string ("label", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_button_set_label (GTK_BUTTON (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, gtk_button_get_label (GTK_BUTTON (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("use-underline", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_button_set_use_underline (GTK_BUTTON (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("use-stock", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_button_set_use_stock (GTK_BUTTON (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("icon-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_button_set_icon_name (GTK_BUTTON (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("relief", nullptr, nullptr, gtk_relief_style_get_type (), GTK_RELIEF_NORMAL, RW),
              [] (GObject *o, const GValue *v) { gtk_button_set_relief (GTK_BUTTON (o), (GtkReliefStyle) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("has-frame", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_button_set_has_frame (GTK_BUTTON (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("image", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) { gtk_button_set_image (GTK_BUTTON (o), (GtkWidget *) g_value_get_object (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("action-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_actionable_set_action_name ((GtkActionable *) o, g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, gtk_actionable_get_action_name ((GtkActionable *) o)); });
  /* GtkActivatable (GTK 2.16-3): Glade writes these; the GtkAction link is not kept */
  gtkqt_prop (klass, g_param_spec_boolean ("use-action-appearance", nullptr, nullptr, TRUE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("related-action", nullptr, nullptr, G_TYPE_OBJECT, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_variant ("action-target", nullptr, nullptr, G_VARIANT_TYPE_ANY, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_actionable_set_action_target_value ((GtkActionable *) o, g_value_get_variant (v)); }, nullptr);
}

/* GtkToggleButton */
static QObject *toggle_create (GtkWidget *w)
{
  auto *b = new GtkQtToggle<QPushButton>;
  button_connect (w, b);
  toggle_connect (w, b);
  return b;
}

G_DEFINE_TYPE (GtkToggleButton, gtk_toggle_button, GTK_TYPE_BUTTON)
static void gtk_toggle_button_init (GtkToggleButton *) {}
static void gtk_toggle_button_class_init (GtkToggleButtonClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), toggle_create);
  GTK_BUTTON_CLASS (klass)->clicked = toggle_real_clicked;
  g_signal_new ("toggled", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkToggleButtonClass, toggled),
                nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_boolean ("active", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("draw-indicator", nullptr, nullptr, FALSE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("inconsistent", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("group", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) {
                auto *g = (GtkWidget *) g_value_get_object (v);
                if (GTK_IS_CHECK_BUTTON (o)) gtk_check_button_set_group (GTK_CHECK_BUTTON (o), g ? GTK_CHECK_BUTTON (g) : nullptr);
                else gtk_toggle_button_set_group (GTK_TOGGLE_BUTTON (o), g ? GTK_TOGGLE_BUTTON (g) : nullptr);
              }, nullptr);
}

/* GtkCheckButton */
static QObject *check_create (GtkWidget *w)
{
  auto *b = new GtkQtToggle<QCheckBox>;
  button_connect (w, b);
  toggle_connect (w, b);
  return b;
}

G_DEFINE_TYPE (GtkCheckButton, gtk_check_button, GTK_TYPE_TOGGLE_BUTTON)
static void gtk_check_button_init (GtkCheckButton *) {}
static void gtk_check_button_class_init (GtkCheckButtonClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), check_create); }

/* Radio groups (GTK 2/3 GSList groups and GTK 4 set_group) */
struct RadioGroup {
  QPointer<QButtonGroup> qgroup;
  GSList *members = nullptr;     /* newest first, like GTK */
  int refs = 0;
};

static RadioGroup *radio_group_of (GtkWidget *w) { return static_cast<RadioGroup *> (g_object_get_data (G_OBJECT (w), "gtkqt-radio-group")); }

static void radio_leave (gpointer data, GObject *where)
{
  auto *g = static_cast<RadioGroup *> (data);
  g->members = g_slist_remove (g->members, where);
  if (--g->refs == 0) { delete g->qgroup.data (); delete g; }
}

static void radio_join (GtkWidget *w, RadioGroup *g)
{
  if (radio_group_of (w) == g) return;
  if (RadioGroup *old = radio_group_of (w)) {
    if (auto *b = gtkqt_q<QAbstractButton> (w)) if (old->qgroup) old->qgroup->removeButton (b);
    g_object_weak_unref (G_OBJECT (w), radio_leave, old);
    radio_leave (old, G_OBJECT (w));
  }
  if (!g) {
    g = new RadioGroup;
    gtkqt_ensure_app ();
    g->qgroup = new QButtonGroup;
    g->qgroup->setExclusive (true);
  }
  bool first = g->members == nullptr;
  g->members = g_slist_prepend (g->members, w);
  g->refs++;
  g_object_set_data (G_OBJECT (w), "gtkqt-radio-group", g);
  g_object_weak_ref (G_OBJECT (w), radio_leave, g);
  if (auto *b = gtkqt_q<QAbstractButton> (w)) {
    b->setAutoExclusive (false);
    g->qgroup->addButton (b);
    if (first && GTK_IS_RADIO_BUTTON (w)) b->setChecked (true);
  }
}

static QObject *radio_create (GtkWidget *w)
{
  auto *b = new GtkQtToggle<QRadioButton>;
  b->setAutoExclusive (false);
  button_connect (w, b);
  toggle_connect (w, b);
  return b;
}

G_DEFINE_TYPE (GtkRadioButton, gtk_radio_button, GTK_TYPE_CHECK_BUTTON)
static void gtk_radio_button_init (GtkRadioButton *) {}
static void gtk_radio_button_class_init (GtkRadioButtonClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), radio_create); }

/* GtkLinkButton */
static QObject *link_create (GtkWidget *w)
{
  auto *b = new QPushButton;
  b->setFlat (true);
  b->setCursor (Qt::PointingHandCursor);
  QPalette pal = b->palette ();
  pal.setColor (QPalette::ButtonText, pal.color (QPalette::Link));
  b->setPalette (pal);
  QFont f = b->font ();
  f.setUnderline (true);
  b->setFont (f);
  gtkqt_connect (w, b, &QAbstractButton::clicked, [] (GtkWidget *bw, bool) {
    g_signal_emit_by_name (bw, "clicked");
    gboolean handled = FALSE;
    g_signal_emit_by_name (bw, "activate-link", &handled);
    if (!handled)
      QDesktopServices::openUrl (QUrl (gtkqt_str ((const char *) g_object_get_data (G_OBJECT (bw), "gtkqt-uri"))));
  });
  return b;
}

G_DEFINE_TYPE (GtkLinkButton, gtk_link_button, GTK_TYPE_BUTTON)
static void gtk_link_button_init (GtkLinkButton *) {}
static void gtk_link_button_class_init (GtkLinkButtonClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), link_create);
  g_signal_new ("activate-link", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, g_signal_accumulator_true_handled,
                nullptr, nullptr, G_TYPE_BOOLEAN, 0);
  gtkqt_prop (klass, g_param_spec_string ("uri", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { g_object_set_data_full (o, "gtkqt-uri", g_value_dup_string (v), g_free); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, (const char *) g_object_get_data (o, "gtkqt-uri")); });
}

extern "C" {

GtkWidget *gtk_button_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_BUTTON, nullptr); }

GtkWidget *gtk_button_new_with_label (const gchar *label)
{
  GtkWidget *w = gtk_button_new ();
  gtk_button_set_label (GTK_BUTTON (w), label);
  return w;
}

GtkWidget *gtk_button_new_with_mnemonic (const gchar *label)
{
  GtkWidget *w = gtk_button_new ();
  gtk_button_set_use_underline (GTK_BUTTON (w), TRUE);
  gtk_button_set_label (GTK_BUTTON (w), label);
  return w;
}

GtkWidget *gtk_button_new_from_stock (const gchar *stock_id)
{
  GtkWidget *w = gtk_button_new ();
  gtk_button_set_use_stock (GTK_BUTTON (w), TRUE);
  gtk_button_set_label (GTK_BUTTON (w), stock_id);
  return w;
}

GtkWidget *gtk_button_new_from_icon_name (const gchar *icon_name, GtkIconSize size)
{
  GtkWidget *w = gtk_button_new ();
  gtk_button_set_icon_name (GTK_BUTTON (w), icon_name);
  if (auto *b = gtkqt_q<QAbstractButton> (w)) {
    int px = gtkqt_icon_size_px (size);
    b->setIconSize (QSize (px, px));
  }
  return w;
}

static void button_update_text (GtkButton *button)
{
  auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (button));
  if (!b) return;
  const char *label = (const char *) g_object_get_data (G_OBJECT (button), "gtkqt-label");
  bool stock = g_object_get_data (G_OBJECT (button), "gtkqt-use-stock");
  bool underline = g_object_get_data (G_OBJECT (button), "gtkqt-use-underline");
  QString s = stock ? gtkqt_stock_label (label) : QString ();
  if (stock && !s.isEmpty ()) {
    b->setText (s);
    if (b->icon ().isNull () && gtkqt_api < 3) b->setIcon (gtkqt_icon (label));
  } else if (underline) {
    b->setText (gtkqt_mnemonic (label));
  } else {
    b->setText (gtkqt_str (label).replace ('&', QStringLiteral ("&&")));
  }
  /* the label child, as GTK has one: made here, or kept in step */
  GtkWidget *child = GTK_BIN (button)->child;
  if (!label || (child && !GTK_IS_LABEL (child))) return;
  QString shown = b->text ();
  shown.remove (QRegularExpression (QStringLiteral ("&(?!&)"))).replace (QStringLiteral ("&&"), QStringLiteral ("&"));
  QByteArray t = shown.toUtf8 ();
  syncing_button_label = true;
  if (!child) {
    child = gtk_label_new (t.constData ());
    gtk_container_add (GTK_CONTAINER (button), child);
    gtk_widget_show (child);
  } else {
    gtk_label_set_text (GTK_LABEL (child), t.constData ());
  }
  syncing_button_label = false;
}

void gtk_button_set_label (GtkButton *button, const gchar *label)
{
  g_object_set_data_full (G_OBJECT (button), "gtkqt-label", g_strdup (label), g_free);
  button_update_text (button);
  g_object_notify (G_OBJECT (button), "label");
}

const gchar *gtk_button_get_label (GtkButton *button)
{
  return (const gchar *) g_object_get_data (G_OBJECT (button), "gtkqt-label");
}

void gtk_button_set_use_underline (GtkButton *button, gboolean use_underline)
{
  g_object_set_data (G_OBJECT (button), "gtkqt-use-underline", GINT_TO_POINTER (use_underline));
  button_update_text (button);
}

void gtk_button_set_use_stock (GtkButton *button, gboolean use_stock)
{
  g_object_set_data (G_OBJECT (button), "gtkqt-use-stock", GINT_TO_POINTER (use_stock));
  button_update_text (button);
}

void gtk_button_set_icon_name (GtkButton *button, const gchar *icon_name)
{
  if (auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (button))) {
    QIcon icon = gtkqt_icon (icon_name);
    b->setIcon (icon);
    if (icon.isNull () && b->text ().isEmpty ()) b->setText (gtkqt_str (icon_name));   /* missing theme: show the name */
  }
}

void gtk_button_set_image (GtkButton *button, GtkWidget *image)
{
  auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (button));
  if (!b || !image) return;
  auto *l = gtkqt_q<QLabel> (image);
  if (l && !l->pixmap (Qt::ReturnByValue).isNull ()) {
    b->setIcon (QIcon (l->pixmap (Qt::ReturnByValue)));
    b->setIconSize (l->pixmap (Qt::ReturnByValue).size ());
  }
  g_object_ref_sink (image);
  g_object_set_data_full (G_OBJECT (button), "gtkqt-image", image, g_object_unref);
}

void gtk_button_set_always_show_image (GtkButton *, gboolean) {}

void gtk_button_set_relief (GtkButton *button, GtkReliefStyle relief)
{
  if (auto *b = gtkqt_q<QPushButton> (GTK_WIDGET (button))) b->setFlat (relief == GTK_RELIEF_NONE);
}

void gtk_button_set_has_frame (GtkButton *button, gboolean has_frame)
{
  if (auto *b = gtkqt_q<QPushButton> (GTK_WIDGET (button))) b->setFlat (!has_frame);
}

void gtk_button_set_child (GtkButton *button, GtkWidget *child)
{
  if (GtkWidget *old = GTK_BIN (button)->child) gtk_container_remove (GTK_CONTAINER (button), old);
  if (child) gtk_container_add (GTK_CONTAINER (button), child);
}

GtkWidget *gtk_button_get_child (GtkButton *button) { return GTK_BIN (button)->child; }

/* as in GTK: only "clicked" (whose default handler toggles a toggle
 * button), no press or release */
void gtk_button_clicked (GtkButton *button)
{
  if (gtk_widget_is_sensitive (GTK_WIDGET (button))) button_clicked_cb (GTK_WIDGET (button));
}

void gtk_button_pressed (GtkButton *button) { g_signal_emit_by_name (button, "pressed"); }
void gtk_button_released (GtkButton *button) { g_signal_emit_by_name (button, "released"); }

void gtk_actionable_set_action_name (GtkActionable *actionable, const gchar *action_name)
{
  g_object_set_data_full (G_OBJECT (actionable), "gtkqt-action-name", g_strdup (action_name), g_free);
}

const gchar *gtk_actionable_get_action_name (GtkActionable *actionable)
{
  return (const gchar *) g_object_get_data (G_OBJECT (actionable), "gtkqt-action-name");
}

void gtk_actionable_set_action_target_value (GtkActionable *actionable, GVariant *target_value)
{
  if (target_value) g_variant_ref_sink (target_value);
  g_object_set_data_full (G_OBJECT (actionable), "gtkqt-action-target", target_value,
                          (GDestroyNotify) g_variant_unref);
}

void gtk_actionable_set_detailed_action_name (GtkActionable *actionable, const gchar *detailed_action_name)
{
  gchar *name = nullptr;
  GVariant *target = nullptr;
  GError *err = nullptr;
  if (g_action_parse_detailed_name (detailed_action_name, &name, &target, &err)) {
    gtk_actionable_set_action_name (actionable, name);
    gtk_actionable_set_action_target_value (actionable, target);
    if (target) g_variant_unref (target);
  } else {
    g_clear_error (&err);
  }
  g_free (name);
}

GtkWidget *gtk_toggle_button_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_TOGGLE_BUTTON, nullptr); }

GtkWidget *gtk_toggle_button_new_with_label (const gchar *label)
{
  GtkWidget *w = gtk_toggle_button_new ();
  gtk_button_set_label (GTK_BUTTON (w), label);
  return w;
}

GtkWidget *gtk_toggle_button_new_with_mnemonic (const gchar *label)
{
  GtkWidget *w = gtk_toggle_button_new ();
  gtk_button_set_use_underline (GTK_BUTTON (w), TRUE);
  gtk_button_set_label (GTK_BUTTON (w), label);
  return w;
}

void gtk_toggle_button_set_active (GtkToggleButton *toggle_button, gboolean is_active)
{
  if (auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (toggle_button))) b->setChecked (is_active);
}

gboolean gtk_toggle_button_get_active (GtkToggleButton *toggle_button)
{
  auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (toggle_button));
  return b && b->isChecked ();
}

void gtk_toggle_button_toggled (GtkToggleButton *toggle_button) { g_signal_emit_by_name (toggle_button, "toggled"); }
void gtk_toggle_button_set_mode (GtkToggleButton *, gboolean) {}

void gtk_toggle_button_set_inconsistent (GtkToggleButton *toggle_button, gboolean setting)
{
  if (auto *c = gtkqt_q<QCheckBox> (GTK_WIDGET (toggle_button))) {
    c->setTristate (setting);
    if (setting) c->setCheckState (Qt::PartiallyChecked);
  }
}

void gtk_toggle_button_set_group (GtkToggleButton *toggle_button, GtkToggleButton *group)
{
  radio_join (GTK_WIDGET (toggle_button), group ? radio_group_of (GTK_WIDGET (group)) : nullptr);
  if (group && !radio_group_of (GTK_WIDGET (group))) {
    radio_join (GTK_WIDGET (group), nullptr);
    radio_join (GTK_WIDGET (toggle_button), radio_group_of (GTK_WIDGET (group)));
  }
}

GtkWidget *gtk_check_button_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_CHECK_BUTTON, nullptr); }

GtkWidget *gtk_check_button_new_with_label (const gchar *label)
{
  GtkWidget *w = gtk_check_button_new ();
  gtk_button_set_label (GTK_BUTTON (w), label);
  return w;
}

GtkWidget *gtk_check_button_new_with_mnemonic (const gchar *label)
{
  GtkWidget *w = gtk_check_button_new ();
  gtk_button_set_use_underline (GTK_BUTTON (w), TRUE);
  gtk_button_set_label (GTK_BUTTON (w), label);
  return w;
}

void gtk_check_button_set_active (GtkCheckButton *self, gboolean setting) { gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self), setting); }
gboolean gtk_check_button_get_active (GtkCheckButton *self) { return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self)); }
void gtk_check_button_set_label (GtkCheckButton *self, const char *label) { gtk_button_set_label (GTK_BUTTON (self), label); }
void gtk_check_button_set_group (GtkCheckButton *self, GtkCheckButton *group) { gtk_toggle_button_set_group (GTK_TOGGLE_BUTTON (self), GTK_TOGGLE_BUTTON (group)); }
void gtk_check_button_set_inconsistent (GtkCheckButton *self, gboolean inconsistent) { gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self), inconsistent); }

GtkWidget *gtk_radio_button_new (GSList *group)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_RADIO_BUTTON, nullptr);
  radio_join (w, group ? radio_group_of ((GtkWidget *) group->data) : nullptr);
  return w;
}

GtkWidget *gtk_radio_button_new_with_label (GSList *group, const gchar *label)
{
  GtkWidget *w = gtk_radio_button_new (group);
  gtk_button_set_label (GTK_BUTTON (w), label);
  return w;
}

GtkWidget *gtk_radio_button_new_with_mnemonic (GSList *group, const gchar *label)
{
  GtkWidget *w = gtk_radio_button_new (group);
  gtk_button_set_use_underline (GTK_BUTTON (w), TRUE);
  gtk_button_set_label (GTK_BUTTON (w), label);
  return w;
}

GtkWidget *gtk_radio_button_new_from_widget (GtkRadioButton *member)
{
  return gtk_radio_button_new (member ? gtk_radio_button_get_group (member) : nullptr);
}

GtkWidget *gtk_radio_button_new_with_label_from_widget (GtkRadioButton *member, const gchar *label)
{
  return gtk_radio_button_new_with_label (member ? gtk_radio_button_get_group (member) : nullptr, label);
}

GtkWidget *gtk_radio_button_new_with_mnemonic_from_widget (GtkRadioButton *member, const gchar *label)
{
  return gtk_radio_button_new_with_mnemonic (member ? gtk_radio_button_get_group (member) : nullptr, label);
}

GSList *gtk_radio_button_get_group (GtkRadioButton *radio_button)
{
  RadioGroup *g = radio_group_of (GTK_WIDGET (radio_button));
  return g ? g->members : nullptr;
}

void gtk_radio_button_set_group (GtkRadioButton *radio_button, GSList *group)
{
  radio_join (GTK_WIDGET (radio_button), group ? radio_group_of ((GtkWidget *) group->data) : nullptr);
}

void gtk_radio_button_join_group (GtkRadioButton *radio_button, GtkRadioButton *group_source)
{
  radio_join (GTK_WIDGET (radio_button), group_source ? radio_group_of (GTK_WIDGET (group_source)) : nullptr);
}

GtkWidget *gtk_link_button_new (const gchar *uri) { return gtk_link_button_new_with_label (uri, uri); }

GtkWidget *gtk_link_button_new_with_label (const gchar *uri, const gchar *label)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_LINK_BUTTON, "uri", uri, nullptr);
  gtk_button_set_label (GTK_BUTTON (w), label ? label : uri);
  if (QWidget *qw = gtkqt_qw (w)) qw->setToolTip (gtkqt_str (uri));
  return w;
}

} /* extern "C" */

/* ======================================================================
 * GtkSwitch
 * ====================================================================== */
class GtkQtSwitch : public QAbstractButton {
public:
  GtkQtSwitch () { setCheckable (true); setCursor (Qt::PointingHandCursor); setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed); }
  QSize sizeHint () const override { int h = fontMetrics ().height () + 8; return QSize (h * 2, h); }
protected:
  void paintEvent (QPaintEvent *) override
  {
    QPainter p (this);
    p.setRenderHint (QPainter::Antialiasing);
    QRectF r = QRectF (rect ()).adjusted (1, 1, -1, -1);
    QColor on = palette ().color (QPalette::Highlight), off = palette ().color (QPalette::Mid);
    p.setPen (Qt::NoPen);
    p.setBrush (isChecked () ? on : off);
    if (!isEnabled ()) p.setOpacity (0.5);
    p.drawRoundedRect (r, r.height () / 2, r.height () / 2);
    qreal d = r.height () - 4;
    QRectF knob (isChecked () ? r.right () - d - 2 : r.left () + 2, r.top () + 2, d, d);
    p.setBrush (Qt::white);
    p.drawEllipse (knob);
  }
};

static QObject *switch_create (GtkWidget *w)
{
  auto *s = new GtkQtSwitch;
  gtkqt_connect (w, s, &QAbstractButton::toggled, [] (GtkWidget *sw, bool on) {
    gboolean handled = FALSE;
    g_signal_emit_by_name (sw, "state-set", (gboolean) on, &handled);
    g_object_notify (G_OBJECT (sw), "active");
    g_object_notify (G_OBJECT (sw), "state");
    if (const char *action = (const char *) g_object_get_data (G_OBJECT (sw), "gtkqt-action-name"))
      gtkqt_activate_action (sw, action, (GVariant *) g_object_get_data (G_OBJECT (sw), "gtkqt-action-target"));
  });
  return s;
}

G_DEFINE_TYPE (GtkSwitch, gtk_switch, GTK_TYPE_WIDGET)
static void gtk_switch_init (GtkSwitch *) {}
static void gtk_switch_class_init (GtkSwitchClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), switch_create);
  g_signal_new ("state-set", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, g_signal_accumulator_true_handled, nullptr,
                nullptr, G_TYPE_BOOLEAN, 1, G_TYPE_BOOLEAN);
  g_signal_new ("activate", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_boolean ("active", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_switch_set_active (GTK_SWITCH (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_switch_get_active (GTK_SWITCH (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("state", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_switch_set_state (GTK_SWITCH (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_switch_get_state (GTK_SWITCH (o))); });
  gtkqt_prop (klass, g_param_spec_string ("action-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_actionable_set_action_name ((GtkActionable *) o, g_value_get_string (v)); }, nullptr);
}

extern "C" {
GtkWidget *gtk_switch_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_SWITCH, nullptr); }
void gtk_switch_set_active (GtkSwitch *self, gboolean is_active) { if (auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (self))) b->setChecked (is_active); }
gboolean gtk_switch_get_active (GtkSwitch *self) { auto *b = gtkqt_q<QAbstractButton> (GTK_WIDGET (self)); return b && b->isChecked (); }
void gtk_switch_set_state (GtkSwitch *self, gboolean state) { gtk_switch_set_active (self, state); }
gboolean gtk_switch_get_state (GtkSwitch *self) { return gtk_switch_get_active (self); }
}

/* ======================================================================
 * GtkEntry / GtkEditable
 * ====================================================================== */
static QLineEdit *editable_line (GtkEditable *e)
{
  QObject *o = gtkqt_obj ((GtkWidget *) e);
  if (auto *l = qobject_cast<QLineEdit *> (o)) return l;
  if (auto *sb = qobject_cast<QAbstractSpinBox *> (o)) return sb->findChild<QLineEdit *> ();
  if (auto *cb = qobject_cast<QComboBox *> (o)) return cb->lineEdit ();
  return nullptr;
}

static QObject *s_adopt = nullptr;   /* lets a GtkEntry wrap an existing QLineEdit */

/* GTK 1's public GtkEntry fields: the text as wide characters and UTF-8 */
static void entry_sync_fields (GtkEntry *entry, const QString &t)
{
  QVector<uint> u = t.toUcs4 ();
  g_free (entry->text);
  entry->text = g_new0 (guint32, u.size () + 1);
  for (int i = 0; i < u.size (); i++) entry->text[i] = u[i];
  entry->text_length = entry->text_size = u.size ();
  g_free (entry->text_mb);
  entry->text_mb = g_strdup (t.toUtf8 ().constData ());
  entry->use_wchar = 1;
}

/* A QLineEdit whose typing, deleting, cutting and pasting go through the
 * GtkEditable signals "insert-text" and "delete-text", so handlers can
 * filter or veto the change as in GTK. */
class GtkQtLineEdit : public QLineEdit {
public:
  GtkWidget *gw;
  explicit GtkQtLineEdit (GtkWidget *w) : gw (w) {}
  bool alive () const { return gw && gw->priv && !gw->priv->destroyed; }
  void delete_range (int start, int end)
  {
    if (start == end) return;
    gtk_editable_delete_text (GTK_EDITABLE (gw), qMin (start, end), qMax (start, end));
  }
  void replace_selection (const QString &text)
  {
    if (isReadOnly ()) return;
    g_object_ref (gw);
    if (hasSelectedText ()) delete_range (selectionStart (), selectionStart () + selectedText ().size ());
    if (alive () && !text.isEmpty ()) {
      QByteArray u = text.toUtf8 ();
      gint pos = cursorPosition ();
      gtk_editable_insert_text (GTK_EDITABLE (gw), u.constData (), u.size (), &pos);
      if (alive ()) setCursorPosition (pos);
    }
    g_object_unref (gw);
  }
protected:
  void keyPressEvent (QKeyEvent *e) override
  {
    if (!alive () || isReadOnly ()) { QLineEdit::keyPressEvent (e); return; }
    Qt::KeyboardModifiers m = e->modifiers () & ~Qt::KeypadModifier;
    if (e->matches (QKeySequence::Paste)) {
      QString t = QGuiApplication::clipboard ()->text ();
      t.replace ('\n', ' ').remove ('\r');
      replace_selection (t);
      return;
    }
    if (e->matches (QKeySequence::Cut)) {
      if (hasSelectedText ()) {
        QGuiApplication::clipboard ()->setText (selectedText ());
        replace_selection (QString ());
      }
      return;
    }
    if ((e->key () == Qt::Key_Backspace || e->key () == Qt::Key_Delete) && !(m & ~Qt::ControlModifier)) {
      if (hasSelectedText ()) { replace_selection (QString ()); return; }
      int pos = cursorPosition (), other = pos;
      QString t = text ();
      bool fwd = e->key () == Qt::Key_Delete;
      if (m & Qt::ControlModifier) {
        int saved = pos;
        if (fwd) cursorWordForward (false); else cursorWordBackward (false);
        other = cursorPosition ();
        setCursorPosition (saved);
      } else {
        other = fwd ? qMin (pos + 1, (int) t.size ()) : qMax (pos - 1, 0);
      }
      delete_range (pos, other);
      return;
    }
    QString t = e->text ();
    if (!t.isEmpty () && t.at (0).isPrint () && !(m & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
      replace_selection (t);
      return;
    }
    QLineEdit::keyPressEvent (e);
  }
  void inputMethodEvent (QInputMethodEvent *e) override
  {
    if (alive () && !e->commitString ().isEmpty () && e->preeditString ().isEmpty () && e->replacementLength () == 0) {
      replace_selection (e->commitString ());
      e->accept ();
      return;
    }
    QLineEdit::inputMethodEvent (e);
  }
};

static QObject *entry_create (GtkWidget *w)
{
  QLineEdit *e;
  if (s_adopt) { e = qobject_cast<QLineEdit *> (s_adopt); s_adopt = nullptr; g_object_set_data (G_OBJECT (w), "gtkqt-borrowed", GINT_TO_POINTER (1)); }
  else e = new GtkQtLineEdit (w);
  gtkqt_connect (w, e, &QLineEdit::textChanged, [] (GtkWidget *ew, const QString &t) {
    if (GTK_IS_ENTRY (ew)) entry_sync_fields (GTK_ENTRY (ew), t);
    g_signal_emit_by_name (ew, "changed");
    g_object_notify (G_OBJECT (ew), "text");
  });
  gtkqt_connect (w, e, &QLineEdit::returnPressed, [] (GtkWidget *ew) { g_signal_emit_by_name (ew, "activate"); });
  auto cursor = [] (GtkWidget *ew) {
    auto *l = qobject_cast<QLineEdit *> (ew->priv->obj.data ());
    if (!l || !GTK_IS_ENTRY (ew)) return;
    GtkEditable *ed = GTK_EDITABLE (ew);
    ed->current_pos = l->cursorPosition ();
    ed->has_selection = l->hasSelectedText ();
    ed->selection_start_pos = l->hasSelectedText () ? l->selectionStart () : ed->current_pos;
    ed->selection_end_pos = l->hasSelectedText () ? l->selectionEnd () : ed->current_pos;
  };
  gtkqt_connect (w, e, &QLineEdit::cursorPositionChanged, [cursor] (GtkWidget *ew, int, int) { cursor (ew); });
  gtkqt_connect (w, e, &QLineEdit::selectionChanged, cursor);
  return e;
}


G_DEFINE_TYPE (GtkEntry, gtk_entry, GTK_TYPE_WIDGET)

static void entry_dispose (GObject *o)
{
  if (GTK_IS_ENTRY (o)) {
    g_free (GTK_ENTRY (o)->text); GTK_ENTRY (o)->text = nullptr;
    g_free (GTK_ENTRY (o)->text_mb); GTK_ENTRY (o)->text_mb = nullptr;
  }
  /* A borrowed QLineEdit (combo box entry) belongs to its QComboBox. */
  GtkWidget *w = GTK_WIDGET (o);
  if (g_object_get_data (o, "gtkqt-borrowed") && w->priv->obj) {
    w->priv->obj->removeEventFilter (w->priv->bridge);
    w->priv->obj->setProperty ("_gtkqt_widget", QVariant ());
    if (w->priv->bridge) { w->priv->bridge->widget = nullptr; delete w->priv->bridge; w->priv->bridge = nullptr; }
    w->priv->obj = nullptr;
    w->priv->destroyed = true;
  }
  G_OBJECT_CLASS (gtk_entry_parent_class)->dispose (o);
}
static void gtk_entry_init (GtkEntry *e)
{
  e->editable.editable = 1;
  e->editable.visible = 1;
  e->visible = 1;
  e->text = g_new0 (guint32, 1);
  e->text_mb = g_strdup ("");
  e->use_wchar = 1;
}

static void entry_real_insert_text (GtkEditable *editable, const gchar *new_text, gint len, gint *position)
{
  QLineEdit *l = editable_line (editable);
  if (!l) return;
  QString ins = len < 0 ? gtkqt_str (new_text) : QString::fromUtf8 (new_text, len);
  int pos = position ? *position : l->cursorPosition ();
  if (pos < 0) pos = l->text ().size ();
  pos = qBound (0, pos, (int) l->text ().size ());
  QString t = l->text ();
  if (l->maxLength () > 0 && t.size () + ins.size () > l->maxLength ()) ins.truncate (qMax (0, l->maxLength () - (int) t.size ()));
  t.insert (pos, ins);
  l->setText (t);
  l->setCursorPosition (pos + ins.size ());
  if (position) *position = pos + ins.size ();
}

static void entry_real_delete_text (GtkEditable *editable, gint start_pos, gint end_pos)
{
  QLineEdit *l = editable_line (editable);
  if (!l) return;
  QString t = l->text ();
  if (end_pos < 0 || end_pos > t.size ()) end_pos = t.size ();
  if (start_pos >= end_pos) return;
  t.remove (start_pos, end_pos - start_pos);
  l->setText (t);
  l->setCursorPosition (start_pos);
}

static void gtk_entry_class_init (GtkEntryClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = entry_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), entry_create);
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("changed", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("activate", t, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("icon-press", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_POINTER);
  /* GtkEditable: the default handlers make the change */
  g_signal_new_class_handler ("insert-text", t, G_SIGNAL_RUN_LAST, G_CALLBACK (entry_real_insert_text), nullptr, nullptr, nullptr,
                              G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER);
  g_signal_new_class_handler ("delete-text", t, G_SIGNAL_RUN_LAST, G_CALLBACK (entry_real_delete_text), nullptr, nullptr, nullptr,
                              G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  gtkqt_prop (klass, g_param_spec_string ("text", nullptr, nullptr, "", RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_text (GTK_ENTRY (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, gtk_entry_get_text (GTK_ENTRY (o))); });
  gtkqt_prop (klass, g_param_spec_string ("placeholder-text", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_placeholder_text (GTK_ENTRY (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("editable", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_editable (GTK_ENTRY (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("visibility", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_visibility (GTK_ENTRY (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("max-length", nullptr, nullptr, 0, 65535, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_max_length (GTK_ENTRY (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("width-chars", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_width_chars (GTK_ENTRY (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_float ("xalign", nullptr, nullptr, 0, 1, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_alignment (GTK_ENTRY (o), g_value_get_float (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("has-frame", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_has_frame (GTK_ENTRY (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("activates-default", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_activates_default (GTK_ENTRY (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("input-purpose", nullptr, nullptr, gtk_input_purpose_get_type (), 0, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_input_purpose (GTK_ENTRY (o), (GtkInputPurpose) g_value_get_enum (v)); }, nullptr);
  for (const char *name : { "primary-icon-activatable", "secondary-icon-activatable", "primary-icon-sensitive", "secondary-icon-sensitive" })
    gtkqt_prop (klass, g_param_spec_boolean (name, nullptr, nullptr, TRUE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("primary-icon-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_icon_from_icon_name (GTK_ENTRY (o), GTK_ENTRY_ICON_PRIMARY, g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("secondary-icon-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_entry_set_icon_from_icon_name (GTK_ENTRY (o), GTK_ENTRY_ICON_SECONDARY, g_value_get_string (v)); }, nullptr);
}

QLineEdit *gtkqt_entry_adopt_begin (QLineEdit *line) { s_adopt = line; return line; }

extern "C" {

GtkWidget *gtk_entry_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_ENTRY, nullptr); }

GtkWidget *gtk_entry_new_with_max_length (gint max)
{
  GtkWidget *w = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (w), max);
  return w;
}

void gtk_entry_set_text (GtkEntry *entry, const gchar *text) { gtk_editable_set_text (GTK_EDITABLE (entry), text); }
const gchar *gtk_entry_get_text (GtkEntry *entry) { return gtk_editable_get_text (GTK_EDITABLE (entry)); }

guint16 gtk_entry_get_text_length (GtkEntry *entry)
{
  QLineEdit *l = editable_line (GTK_EDITABLE (entry));
  return l ? l->text ().size () : 0;
}

void gtk_entry_append_text (GtkEntry *entry, const gchar *text)
{
  if (QLineEdit *l = editable_line (GTK_EDITABLE (entry))) l->setText (l->text () + gtkqt_str (text));
}

void gtk_entry_prepend_text (GtkEntry *entry, const gchar *text)
{
  if (QLineEdit *l = editable_line (GTK_EDITABLE (entry))) l->setText (gtkqt_str (text) + l->text ());
}

void gtk_entry_set_editable (GtkEntry *entry, gboolean editable) { gtk_editable_set_editable (GTK_EDITABLE (entry), editable); }

void gtk_entry_set_visibility (GtkEntry *entry, gboolean visible)
{
  entry->editable.visible = visible;
  entry->visible = visible;
  if (QLineEdit *l = editable_line (GTK_EDITABLE (entry))) l->setEchoMode (visible ? QLineEdit::Normal : QLineEdit::Password);
}

gboolean gtk_entry_get_visibility (GtkEntry *entry)
{
  QLineEdit *l = editable_line (GTK_EDITABLE (entry));
  return !l || l->echoMode () == QLineEdit::Normal;
}

void gtk_entry_set_invisible_char (GtkEntry *, gunichar) {}

void gtk_entry_set_max_length (GtkEntry *entry, gint max)
{
  if (QLineEdit *l = editable_line (GTK_EDITABLE (entry))) l->setMaxLength (max > 0 ? max : 32767);
}

gint gtk_entry_get_max_length (GtkEntry *entry)
{
  QLineEdit *l = editable_line (GTK_EDITABLE (entry));
  return l && l->maxLength () != 32767 ? l->maxLength () : 0;
}

void gtk_entry_set_width_chars (GtkEntry *entry, gint n_chars) { gtk_editable_set_width_chars (GTK_EDITABLE (entry), n_chars); }
void gtk_entry_set_activates_default (GtkEntry *, gboolean) {}

void gtk_entry_set_placeholder_text (GtkEntry *entry, const gchar *text)
{
  if (QLineEdit *l = editable_line (GTK_EDITABLE (entry))) l->setPlaceholderText (gtkqt_str (text));
}

void gtk_entry_set_alignment (GtkEntry *entry, gfloat xalign) { gtk_editable_set_alignment (GTK_EDITABLE (entry), xalign); }
void gtk_entry_set_has_frame (GtkEntry *entry, gboolean setting) { if (QLineEdit *l = editable_line (GTK_EDITABLE (entry))) l->setFrame (setting); }

void gtk_entry_set_input_purpose (GtkEntry *entry, GtkInputPurpose purpose)
{
  if (purpose == GTK_INPUT_PURPOSE_PASSWORD || purpose == GTK_INPUT_PURPOSE_PIN) gtk_entry_set_visibility (entry, FALSE);
}

void gtk_entry_set_icon_from_icon_name (GtkEntry *entry, GtkEntryIconPosition icon_pos, const gchar *icon_name)
{
  QLineEdit *l = editable_line (GTK_EDITABLE (entry));
  if (!l || !icon_name) return;
  QAction *a = l->addAction (gtkqt_icon (icon_name), icon_pos == GTK_ENTRY_ICON_PRIMARY ? QLineEdit::LeadingPosition : QLineEdit::TrailingPosition);
  gtkqt_connect ((GtkWidget *) entry, a, &QAction::triggered, [icon_pos] (GtkWidget *e, bool) {
    GdkEvent ev = {};
    ev.button.type = GDK_BUTTON_PRESS;
    ev.button.button = 1;
    g_signal_emit_by_name (e, "icon-press", (int) icon_pos, &ev);
  });
}

void gtk_entry_select_region (GtkEntry *entry, gint start, gint end) { gtk_editable_select_region (GTK_EDITABLE (entry), start, end); }
void gtk_entry_set_position (GtkEntry *entry, gint position) { gtk_editable_set_position (GTK_EDITABLE (entry), position); }
GtkEntryBuffer *gtk_entry_get_buffer (GtkEntry *entry) { return (GtkEntryBuffer *) entry; }
const char *gtk_entry_buffer_get_text (GtkEntryBuffer *buffer) { return gtk_editable_get_text ((GtkEditable *) buffer); }

void gtk_entry_buffer_set_text (GtkEntryBuffer *buffer, const char *chars, int n_chars)
{
  QByteArray b = n_chars < 0 ? QByteArray (chars) : QByteArray (chars, n_chars);
  gtk_editable_set_text ((GtkEditable *) buffer, b.constData ());
}

guint gtk_entry_buffer_get_length (GtkEntryBuffer *buffer) { return gtk_entry_get_text_length ((GtkEntry *) buffer); }

const char *gtk_editable_get_text (GtkEditable *editable)
{
  QLineEdit *l = editable_line (editable);
  return gtkqt_ret ((GtkWidget *) editable, l ? l->text () : QString ());
}

void gtk_editable_set_text (GtkEditable *editable, const char *text)
{
  QLineEdit *l = editable_line (editable);
  if (!l) return;
  if (l->text () == gtkqt_str (text)) return;
  if (!g_signal_lookup ("insert-text", G_OBJECT_TYPE (editable))) { l->setText (gtkqt_str (text)); return; }
  gtk_editable_delete_text (editable, 0, -1);
  gint pos = 0;
  if (text && *text) gtk_editable_insert_text (editable, text, -1, &pos);
}

gchar *gtk_editable_get_chars (GtkEditable *editable, gint start_pos, gint end_pos)
{
  QLineEdit *l = editable_line (editable);
  QString t = l ? l->text () : QString ();
  if (end_pos < 0 || end_pos > t.size ()) end_pos = t.size ();
  return g_strdup (t.mid (start_pos, end_pos - start_pos).toUtf8 ().constData ());
}

void gtk_editable_insert_text (GtkEditable *editable, const gchar *new_text, gint len, gint *position)
{
  if (!new_text) return;
  if (len < 0) len = strlen (new_text);
  if (len == 0) return;
  gint pos = position ? *position : -1;
  if (g_signal_lookup ("insert-text", G_OBJECT_TYPE (editable))) {
    QByteArray copy (new_text, len);   /* the signal takes a NUL-terminated copy */
    g_signal_emit_by_name (editable, "insert-text", copy.constData (), len, &pos);
  } else {
    entry_real_insert_text (editable, new_text, len, &pos);
  }
  if (position) *position = pos;
}

void gtk_editable_delete_text (GtkEditable *editable, gint start_pos, gint end_pos)
{
  QLineEdit *l = editable_line (editable);
  if (!l) return;
  int n = l->text ().size ();
  if (end_pos < 0 || end_pos > n) end_pos = n;
  if (start_pos < 0) start_pos = 0;
  if (start_pos >= end_pos) return;
  if (g_signal_lookup ("delete-text", G_OBJECT_TYPE (editable))) g_signal_emit_by_name (editable, "delete-text", start_pos, end_pos);
  else entry_real_delete_text (editable, start_pos, end_pos);
}

void gtk_editable_select_region (GtkEditable *editable, gint start_pos, gint end_pos)
{
  if (QLineEdit *l = editable_line (editable)) {
    if (end_pos < 0) end_pos = l->text ().size ();
    l->setSelection (start_pos, end_pos - start_pos);
  }
}

gboolean gtk_editable_get_selection_bounds (GtkEditable *editable, gint *start_pos, gint *end_pos)
{
  QLineEdit *l = editable_line (editable);
  if (!l || !l->hasSelectedText ()) {
    if (start_pos) *start_pos = l ? l->cursorPosition () : 0;
    if (end_pos) *end_pos = l ? l->cursorPosition () : 0;
    return FALSE;
  }
  if (start_pos) *start_pos = l->selectionStart ();
  if (end_pos) *end_pos = l->selectionEnd ();
  return TRUE;
}

void gtk_editable_set_position (GtkEditable *editable, gint position)
{
  if (QLineEdit *l = editable_line (editable)) l->setCursorPosition (position < 0 ? l->text ().size () : position);
}

gint gtk_editable_get_position (GtkEditable *editable) { QLineEdit *l = editable_line (editable); return l ? l->cursorPosition () : 0; }
void gtk_editable_set_editable (GtkEditable *editable, gboolean is_editable)
{
  if (GTK_IS_ENTRY (editable)) editable->editable = is_editable;
  if (QLineEdit *l = editable_line (editable)) l->setReadOnly (!is_editable);
}
gboolean gtk_editable_get_editable (GtkEditable *editable) { QLineEdit *l = editable_line (editable); return l && !l->isReadOnly (); }
void gtk_editable_cut_clipboard (GtkEditable *editable) { if (QLineEdit *l = editable_line (editable)) l->cut (); }
void gtk_editable_copy_clipboard (GtkEditable *editable) { if (QLineEdit *l = editable_line (editable)) l->copy (); }
void gtk_editable_paste_clipboard (GtkEditable *editable) { if (QLineEdit *l = editable_line (editable)) l->paste (); }
void gtk_editable_delete_selection (GtkEditable *editable) { if (QLineEdit *l = editable_line (editable)) l->del (); }

void gtk_editable_set_width_chars (GtkEditable *editable, gint n_chars)
{
  if (QLineEdit *l = editable_line (editable)) {
    QWidget *qw = gtkqt_qw ((GtkWidget *) editable);
    int w = n_chars > 0 ? l->fontMetrics ().averageCharWidth () * n_chars + 12 : 0;
    if (qw) qw->setMinimumWidth (w);
    if (n_chars > 0 && qw) qw->setMaximumWidth (qMax (w, qw->maximumWidth () == QWIDGETSIZE_MAX ? QWIDGETSIZE_MAX : w));
  }
}

void gtk_editable_set_alignment (GtkEditable *editable, gfloat xalign)
{
  if (QLineEdit *l = editable_line (editable))
    l->setAlignment ((xalign < 0.25 ? Qt::AlignLeft : xalign > 0.75 ? Qt::AlignRight : Qt::AlignHCenter) | Qt::AlignVCenter);
}

} /* extern "C" */

/* ======================================================================
 * GtkComboBox / GtkComboBoxText / GtkCombo
 * ====================================================================== */
static QObject *combo_create (GtkWidget *w)
{
  auto *c = new QComboBox;
  if (g_object_get_data (G_OBJECT (w), "gtkqt-has-entry") || GTK_IS_COMBO (w)) c->setEditable (true);
  if (GTK_IS_COMBO_BOX (w))
    gtkqt_connect (w, c, QOverload<int>::of (&QComboBox::currentIndexChanged), [] (GtkWidget *cw, int) {
      g_signal_emit_by_name (cw, "changed");
      g_object_notify (G_OBJECT (cw), "active");
    });
  return c;
}

static void combo_make_entry (GtkWidget *w)
{
  auto *c = gtkqt_q<QComboBox> (w);
  if (!c) return;
  c->setEditable (true);
  gtkqt_entry_adopt_begin (c->lineEdit ());
  GtkWidget *entry = gtk_entry_new ();
  g_object_ref_sink (entry);
  entry->parent = w;
  entry->priv->visible = true;
  if (GTK_IS_BIN (w)) GTK_BIN (w)->child = entry;
  if (GTK_IS_COMBO (w)) GTK_COMBO (w)->entry = entry;
  g_object_set_data_full (G_OBJECT (w), "gtkqt-entry", entry, [] (gpointer p) {
    auto *e = (GtkWidget *) p;
    e->parent = nullptr;
    g_object_run_dispose (G_OBJECT (e));
    g_object_unref (e);
  });
}


G_DEFINE_TYPE (GtkComboBox, gtk_combo_box, GTK_TYPE_BIN)
G_DEFINE_TYPE (GtkComboBoxText, gtk_combo_box_text, GTK_TYPE_COMBO_BOX)

static void combo_dispose (GObject *o)
{
  g_object_set_data (o, "gtkqt-entry", nullptr);
  if (GTK_IS_BIN (o)) GTK_BIN (o)->child = nullptr;
  G_OBJECT_CLASS (gtk_combo_box_parent_class)->dispose (o);
}
static void gtk_combo_box_init (GtkComboBox *) {}
static void gtk_combo_box_class_init (GtkComboBoxClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = combo_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), combo_create);
  g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_int ("active", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_combo_box_set_active (GTK_COMBO_BOX (o), g_value_get_int (v)); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, gtk_combo_box_get_active (GTK_COMBO_BOX (o))); });
  gtkqt_prop (klass, g_param_spec_string ("active-id", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_combo_box_set_active_id (GTK_COMBO_BOX (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, gtk_combo_box_get_active_id (GTK_COMBO_BOX (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("has-entry", nullptr, nullptr, FALSE, (GParamFlags) (RW | G_PARAM_CONSTRUCT_ONLY)),
              [] (GObject *o, const GValue *v) { if (g_value_get_boolean (v)) g_object_set_data (o, "gtkqt-has-entry", GINT_TO_POINTER (1)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("entry-text-column", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("id-column", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_combo_box_set_id_column (GTK_COMBO_BOX (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_pointer ("model", nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_combo_box_set_model (GTK_COMBO_BOX (o), (GtkTreeModel *) g_value_get_pointer (v)); }, nullptr);
}

static void gtk_combo_box_text_init (GtkComboBoxText *) {}
static void gtk_combo_box_text_class_init (GtkComboBoxTextClass *) {}

static void gtk_combo_init (GtkCombo *) {}
static void gtk_combo_class_init (GtkComboClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), combo_create); }
G_DEFINE_TYPE (GtkCombo, gtk_combo, GTK_TYPE_BOX)

extern "C" {

GtkWidget *gtk_combo_box_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_COMBO_BOX, nullptr); }
GtkWidget *gtk_combo_box_new_text (void) { return (GtkWidget *) g_object_new (GTK_TYPE_COMBO_BOX_TEXT, nullptr); }
GtkWidget *gtk_combo_box_text_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_COMBO_BOX_TEXT, nullptr); }

GtkWidget *gtk_combo_box_new_with_entry (void)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_COMBO_BOX, "has-entry", TRUE, nullptr);
  combo_make_entry (w);
  return w;
}

GtkWidget *gtk_combo_box_text_new_with_entry (void)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_COMBO_BOX_TEXT, "has-entry", TRUE, nullptr);
  combo_make_entry (w);
  return w;
}

static QComboBox *qcombo (gpointer w) { return gtkqt_q<QComboBox> ((GtkWidget *) w); }

void gtk_combo_box_text_insert (GtkComboBoxText *combo, gint position, const gchar *id, const gchar *text)
{
  if (QComboBox *c = qcombo (combo)) {
    if (position < 0 || position > c->count ()) position = c->count ();
    QSignalBlocker b (c);
    bool first = c->count () == 0;
    c->insertItem (position, gtkqt_str (text), id ? QVariant (gtkqt_str (id)) : QVariant ());
    if (first && !c->isEditable ()) c->setCurrentIndex (-1);   /* GTK: no active item until set */
  }
}

void gtk_combo_box_text_append (GtkComboBoxText *combo, const gchar *id, const gchar *text) { gtk_combo_box_text_insert (combo, -1, id, text); }
void gtk_combo_box_text_prepend (GtkComboBoxText *combo, const gchar *id, const gchar *text) { gtk_combo_box_text_insert (combo, 0, id, text); }
void gtk_combo_box_text_append_text (GtkComboBoxText *combo, const gchar *text) { gtk_combo_box_text_insert (combo, -1, nullptr, text); }
void gtk_combo_box_text_prepend_text (GtkComboBoxText *combo, const gchar *text) { gtk_combo_box_text_insert (combo, 0, nullptr, text); }
void gtk_combo_box_text_insert_text (GtkComboBoxText *combo, gint position, const gchar *text) { gtk_combo_box_text_insert (combo, position, nullptr, text); }
void gtk_combo_box_text_remove (GtkComboBoxText *combo, gint position) { if (QComboBox *c = qcombo (combo)) c->removeItem (position); }
void gtk_combo_box_text_remove_all (GtkComboBoxText *combo) { if (QComboBox *c = qcombo (combo)) c->clear (); }

gchar *gtk_combo_box_text_get_active_text (GtkComboBoxText *combo)
{
  QComboBox *c = qcombo (combo);
  if (!c) return nullptr;
  if (c->isEditable ()) return g_strdup (c->currentText ().toUtf8 ().constData ());
  return c->currentIndex () >= 0 ? g_strdup (c->currentText ().toUtf8 ().constData ()) : nullptr;
}

void gtk_combo_box_append_text (GtkComboBox *combo, const gchar *text) { gtk_combo_box_text_append_text ((GtkComboBoxText *) combo, text); }
void gtk_combo_box_prepend_text (GtkComboBox *combo, const gchar *text) { gtk_combo_box_text_prepend_text ((GtkComboBoxText *) combo, text); }
void gtk_combo_box_insert_text (GtkComboBox *combo, gint position, const gchar *text) { gtk_combo_box_text_insert_text ((GtkComboBoxText *) combo, position, text); }
void gtk_combo_box_remove_text (GtkComboBox *combo, gint position) { gtk_combo_box_text_remove ((GtkComboBoxText *) combo, position); }
gchar *gtk_combo_box_get_active_text (GtkComboBox *combo) { return gtk_combo_box_text_get_active_text ((GtkComboBoxText *) combo); }

void gtk_combo_box_set_active (GtkComboBox *combo, gint index_) { if (QComboBox *c = qcombo (combo)) c->setCurrentIndex (index_); }
gint gtk_combo_box_get_active (GtkComboBox *combo) { QComboBox *c = qcombo (combo); return c ? c->currentIndex () : -1; }

const gchar *gtk_combo_box_get_active_id (GtkComboBox *combo)
{
  QComboBox *c = qcombo (combo);
  if (!c || c->currentIndex () < 0) return nullptr;
  QVariant v = c->currentData ();
  return v.isValid () ? gtkqt_ret (GTK_WIDGET (combo), v.toString ()) : nullptr;
}

gboolean gtk_combo_box_set_active_id (GtkComboBox *combo, const gchar *active_id)
{
  QComboBox *c = qcombo (combo);
  if (!c) return FALSE;
  int i = active_id ? c->findData (gtkqt_str (active_id)) : -1;
  c->setCurrentIndex (i);
  return i >= 0 || !active_id;
}

void gtk_combo_box_popup (GtkComboBox *combo) { if (QComboBox *c = qcombo (combo)) c->showPopup (); }

/* GtkCombo: as in GTK, combo->list is a GtkList of GtkListItems (programs
 * fill it themselves); the Qt combo box shows those items. Choosing one
 * selects it in the list, and the list's selection puts the item's string
 * (gtk_combo_set_item_string, else its label) into the entry. */
static const char *combo_item_string (GtkWidget *item)
{
  if (const char *s = (const char *) g_object_get_data (G_OBJECT (item), "gtkqt-combo-string")) return s;
  GtkWidget *child = GTK_BIN (item)->child;
  return child && GTK_IS_LABEL (child) ? gtk_label_get_text (GTK_LABEL (child)) : "";
}

static void combo_rebuild (GtkCombo *combo)
{
  QComboBox *c = qcombo (combo);
  if (!c || !combo->list) return;
  GtkList *list = GTK_LIST (combo->list);
  QLineEdit *le = c->lineEdit ();
  QString text = le ? le->text () : QString ();
  {
    QSignalBlocker b1 (c), b2 (le);
    c->clear ();
    for (GList *l = list->children; l; l = l->next) {
      auto *item = (GtkWidget *) l->data;
      GtkWidget *child = GTK_BIN (item)->child;
      c->addItem (gtkqt_str (child && GTK_IS_LABEL (child) ? gtk_label_get_text (GTK_LABEL (child)) : combo_item_string (item)));
    }
    int sel = list->selection ? g_list_index (list->children, list->selection->data) : -1;
    c->setCurrentIndex (sel);
    if (le) le->setText (text);
  }
  /* GTK's combo list is in browse mode: with items, one is always selected */
  if (!list->selection && list->children) gtk_list_select_child (list, (GtkWidget *) list->children->data);
}

static void combo_schedule_rebuild (GtkCombo *combo)
{
  if (g_object_get_data (G_OBJECT (combo), "gtkqt-combo-rebuild")) return;
  g_object_set_data (G_OBJECT (combo), "gtkqt-combo-rebuild", GINT_TO_POINTER (1));
  g_object_ref (combo);
  QTimer::singleShot (0, qApp, [combo] {
    g_object_set_data (G_OBJECT (combo), "gtkqt-combo-rebuild", nullptr);
    if (GTK_WIDGET (combo)->priv && !GTK_WIDGET (combo)->priv->destroyed) combo_rebuild (combo);
    g_object_unref (combo);
  });
}

static void combo_list_changed (GtkContainer *, GtkWidget *, gpointer combo) { combo_schedule_rebuild (GTK_COMBO (combo)); }

static void combo_selection_changed (GtkList *list, gpointer cp)
{
  auto *combo = GTK_COMBO (cp);
  if (!list->selection) return;
  auto *item = (GtkWidget *) list->selection->data;
  if (QComboBox *c = qcombo (combo)) {
    QSignalBlocker b (c);
    c->setCurrentIndex (g_list_index (list->children, item));
  }
  if (combo->entry) gtk_entry_set_text (GTK_ENTRY (combo->entry), combo_item_string (item));
}

GtkWidget *gtk_combo_new (void)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_COMBO, nullptr);
  combo_make_entry (w);
  auto *combo = GTK_COMBO (w);
  combo->list = (GtkWidget *) g_object_ref_sink (gtk_list_new ());
  gtk_list_set_selection_mode (GTK_LIST (combo->list), GTK_SELECTION_BROWSE);
  g_signal_connect_after (combo->list, "add", G_CALLBACK (combo_list_changed), w);
  g_signal_connect_after (combo->list, "remove", G_CALLBACK (combo_list_changed), w);
  g_signal_connect (combo->list, "selection-changed", G_CALLBACK (combo_selection_changed), w);
  g_object_set_data_full (G_OBJECT (w), "gtkqt-combo-list", combo->list, [] (gpointer p) {
    g_signal_handlers_disconnect_matched (p, G_SIGNAL_MATCH_FUNC, 0, 0, nullptr, (gpointer) combo_list_changed, nullptr);
    g_signal_handlers_disconnect_matched (p, G_SIGNAL_MATCH_FUNC, 0, 0, nullptr, (gpointer) combo_selection_changed, nullptr);
    gtk_widget_destroy ((GtkWidget *) p);
    g_object_unref (p);
  });
  if (QComboBox *c = qcombo (w))
    gtkqt_connect (w, c, QOverload<int>::of (&QComboBox::activated), [] (GtkWidget *cw, int i) {
      GtkList *list = GTK_LIST (GTK_COMBO (cw)->list);
      if (auto *item = (GtkWidget *) g_list_nth_data (list->children, i)) gtk_list_select_child (list, item);
    });
  return w;
}

void gtk_combo_set_item_string (GtkCombo *combo, GtkItem *item, const gchar *item_value)
{
  g_object_set_data_full (G_OBJECT (item), "gtkqt-combo-string", g_strdup (item_value), g_free);
  if (combo) combo_schedule_rebuild (combo);
}

void gtk_combo_set_popdown_strings (GtkCombo *combo, GList *strings)
{
  if (!combo->list) return;
  GtkList *list = GTK_LIST (combo->list);
  gtk_list_clear_items (list, 0, -1);
  for (GList *l = strings; l; l = l->next) {
    GtkWidget *item = gtk_list_item_new_with_label ((const char *) l->data);
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (list), item);
  }
  combo_rebuild (combo);   /* now: the first string shows in the entry, as in GTK */
}

void gtk_combo_set_value_in_list (GtkCombo *combo, gboolean val, gboolean)
{
  if (QComboBox *c = qcombo (combo)) c->setEditable (!val);
}

} /* extern "C" */

/* ======================================================================
 * GtkStringList / GtkStringObject / GtkDropDown (GTK 4)
 * ====================================================================== */
struct _GtkStringObject { GObject parent_instance; gchar *string; };
typedef struct { GObjectClass parent_class; } GtkStringObjectClass;
G_DEFINE_TYPE (GtkStringObject, gtk_string_object, G_TYPE_OBJECT)
static void gtk_string_object_init (GtkStringObject *) {}
static void gtk_string_object_finalize (GObject *o)
{
  g_free (GTK_STRING_OBJECT (o)->string);
  G_OBJECT_CLASS (gtk_string_object_parent_class)->finalize (o);
}
static void gtk_string_object_class_init (GtkStringObjectClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = gtk_string_object_finalize;
  gtkqt_prop (klass, g_param_spec_string ("string", nullptr, nullptr, nullptr, G_PARAM_READABLE),
              nullptr, [] (GObject *o, GValue *v) { g_value_set_string (v, GTK_STRING_OBJECT (o)->string); });
}

struct _GtkStringList { GObject parent_instance; GPtrArray *items; };
typedef struct { GObjectClass parent_class; } GtkStringListClass;

static GType string_list_item_type (GListModel *) { return GTK_TYPE_STRING_OBJECT; }
static guint string_list_n_items (GListModel *m) { return GTK_STRING_LIST (m)->items->len; }
static gpointer string_list_get_item (GListModel *m, guint i)
{
  GPtrArray *a = GTK_STRING_LIST (m)->items;
  return i < a->len ? g_object_ref (g_ptr_array_index (a, i)) : nullptr;
}
static void string_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = string_list_item_type;
  iface->get_n_items = string_list_n_items;
  iface->get_item = string_list_get_item;
}

G_DEFINE_TYPE_WITH_CODE (GtkStringList, gtk_string_list, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, string_list_model_init))
static void gtk_string_list_init (GtkStringList *l) { l->items = g_ptr_array_new_with_free_func (g_object_unref); }
static void gtk_string_list_finalize (GObject *o)
{
  g_ptr_array_unref (GTK_STRING_LIST (o)->items);
  G_OBJECT_CLASS (gtk_string_list_parent_class)->finalize (o);
}
static void gtk_string_list_class_init (GtkStringListClass *klass) { G_OBJECT_CLASS (klass)->finalize = gtk_string_list_finalize; }

static void dropdown_rebuild (GtkWidget *w)
{
  auto *c = gtkqt_q<QComboBox> (w);
  auto *model = (GListModel *) g_object_get_data (G_OBJECT (w), "gtkqt-model");
  if (!c) return;
  QSignalBlocker block (c);
  int cur = c->currentIndex ();
  c->clear ();
  guint n = model ? g_list_model_get_n_items (model) : 0;
  for (guint i = 0; i < n; i++) {
    GObject *item = (GObject *) g_list_model_get_item (model, i);
    gchar *s = nullptr;
    if (GTK_IS_STRING_OBJECT (item)) s = g_strdup (GTK_STRING_OBJECT (item)->string);
    else if (g_object_class_find_property (G_OBJECT_GET_CLASS (item), "string")) g_object_get (item, "string", &s, nullptr);
    c->addItem (gtkqt_str (s));
    g_free (s);
    g_object_unref (item);
  }
  c->setCurrentIndex (n ? qBound (0, cur, (int) n - 1) : -1);
}

static QObject *dropdown_create (GtkWidget *w)
{
  auto *c = new QComboBox;
  gtkqt_connect (w, c, QOverload<int>::of (&QComboBox::currentIndexChanged), [] (GtkWidget *dw, int) {
    g_object_notify (G_OBJECT (dw), "selected");
    g_object_notify (G_OBJECT (dw), "selected-item");
  });
  return c;
}

G_DEFINE_TYPE (GtkDropDown, gtk_drop_down, GTK_TYPE_WIDGET)
static void gtk_drop_down_init (GtkDropDown *) {}
static void gtk_drop_down_class_init (GtkDropDownClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), dropdown_create);
  gtkqt_prop (klass, g_param_spec_uint ("selected", nullptr, nullptr, 0, G_MAXUINT, GTK_INVALID_LIST_POSITION, RW),
              [] (GObject *o, const GValue *v) { gtk_drop_down_set_selected (GTK_DROP_DOWN (o), g_value_get_uint (v)); },
              [] (GObject *o, GValue *v) { g_value_set_uint (v, gtk_drop_down_get_selected (GTK_DROP_DOWN (o))); });
  gtkqt_prop (klass, g_param_spec_object ("selected-item", nullptr, nullptr, G_TYPE_OBJECT, G_PARAM_READABLE),
              nullptr, [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_drop_down_get_selected_item (GTK_DROP_DOWN (o))); });
  gtkqt_prop (klass, g_param_spec_object ("model", nullptr, nullptr, G_TYPE_LIST_MODEL, RW),
              [] (GObject *o, const GValue *v) {
                auto *m = (GListModel *) g_value_get_object (v);
                if (m) g_object_ref (m);
                g_object_set_data_full (o, "gtkqt-model", m, m ? g_object_unref : nullptr);
                if (m) g_signal_connect_object (m, "items-changed", G_CALLBACK (+[] (GListModel *, guint, guint, guint, gpointer w) { dropdown_rebuild ((GtkWidget *) w); }), o, (GConnectFlags) 0);
                dropdown_rebuild (GTK_WIDGET (o));
              },
              [] (GObject *o, GValue *v) { g_value_set_object (v, g_object_get_data (o, "gtkqt-model")); });
}

extern "C" {

GtkStringList *gtk_string_list_new (const char * const *strings)
{
  auto *l = (GtkStringList *) g_object_new (GTK_TYPE_STRING_LIST, nullptr);
  for (int i = 0; strings && strings[i]; i++) gtk_string_list_append (l, strings[i]);
  return l;
}

void gtk_string_list_append (GtkStringList *self, const char *string)
{
  auto *o = (GtkStringObject *) g_object_new (GTK_TYPE_STRING_OBJECT, nullptr);
  o->string = g_strdup (string);
  g_ptr_array_add (self->items, o);
  g_list_model_items_changed (G_LIST_MODEL (self), self->items->len - 1, 0, 1);
}

void gtk_string_list_remove (GtkStringList *self, guint position)
{
  if (position >= self->items->len) return;
  g_ptr_array_remove_index (self->items, position);
  g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);
}

const char *gtk_string_list_get_string (GtkStringList *self, guint position)
{
  return position < self->items->len ? GTK_STRING_OBJECT (g_ptr_array_index (self->items, position))->string : nullptr;
}

const char *gtk_string_object_get_string (GtkStringObject *self) { return self->string; }

GtkWidget *gtk_drop_down_new (GListModel *model, gpointer)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_DROP_DOWN, "model", model, nullptr);
  if (model) g_object_unref (model);   /* gtk_drop_down_new takes ownership */
  return w;
}

GtkWidget *gtk_drop_down_new_from_strings (const char * const *strings)
{
  return gtk_drop_down_new (G_LIST_MODEL (gtk_string_list_new (strings)), nullptr);
}

guint gtk_drop_down_get_selected (GtkDropDown *self)
{
  auto *c = gtkqt_q<QComboBox> (GTK_WIDGET (self));
  return c && c->currentIndex () >= 0 ? (guint) c->currentIndex () : GTK_INVALID_LIST_POSITION;
}

void gtk_drop_down_set_selected (GtkDropDown *self, guint position)
{
  if (auto *c = gtkqt_q<QComboBox> (GTK_WIDGET (self))) c->setCurrentIndex (position == GTK_INVALID_LIST_POSITION ? -1 : (int) position);
}

gpointer gtk_drop_down_get_selected_item (GtkDropDown *self)
{
  auto *model = (GListModel *) g_object_get_data (G_OBJECT (self), "gtkqt-model");
  guint i = gtk_drop_down_get_selected (self);
  if (!model || i == GTK_INVALID_LIST_POSITION) return nullptr;
  gpointer item = g_list_model_get_item (model, i);
  g_object_unref (item);   /* transfer none, as in GTK: the model keeps it alive */
  return item;
}

GListModel *gtk_drop_down_get_model (GtkDropDown *self) { return (GListModel *) g_object_get_data (G_OBJECT (self), "gtkqt-model"); }

} /* extern "C" */

/* ======================================================================
 * GtkImage, GtkSpinner
 * ====================================================================== */
static QObject *image_create (GtkWidget *)
{
  auto *l = new QLabel;
  l->setAlignment (Qt::AlignCenter);
  return l;
}

G_DEFINE_TYPE (GtkImage, gtk_image, GTK_TYPE_WIDGET)
static void gtk_image_init (GtkImage *) {}
static void gtk_image_class_init (GtkImageClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), image_create);
  gtkqt_prop (klass, g_param_spec_string ("icon-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_image_set_from_icon_name (GTK_IMAGE (o), g_value_get_string (v), GTK_ICON_SIZE_BUTTON); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("file", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_image_set_from_file (GTK_IMAGE (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("stock", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_image_set_from_icon_name (GTK_IMAGE (o), g_value_get_string (v), GTK_ICON_SIZE_BUTTON); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("pixel-size", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_image_set_pixel_size (GTK_IMAGE (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("icon-size", nullptr, nullptr, gtk_icon_size_get_type (), GTK_ICON_SIZE_BUTTON, RW),
              [] (GObject *o, const GValue *v) {
                int px = gtkqt_icon_size_px ((GtkIconSize) g_value_get_enum (v));
                if (!g_object_get_data (o, "gtkqt-pixel-size")) gtk_image_set_pixel_size (GTK_IMAGE (o), px);
              }, nullptr);
}

static void image_show_icon (GtkImage *image)
{
  auto *l = gtkqt_q<QLabel> (GTK_WIDGET (image));
  if (!l) return;
  const char *name = (const char *) g_object_get_data (G_OBJECT (image), "gtkqt-icon-name");
  int px = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (image), "gtkqt-pixel-size"));
  if (px <= 0) px = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (image), "gtkqt-icon-px"));
  if (px <= 0) px = 16;
  if (!name) return;
  QIcon icon = gtkqt_icon (name);
  /* an icon the theme lacks keeps its space, as GTK's missing-image does */
  if (icon.isNull ()) { l->clear (); l->setMinimumSize (px, px); }
  else l->setPixmap (icon.pixmap (px, px));
}

extern "C" {
GtkWidget *gtk_image_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_IMAGE, nullptr); }

GtkWidget *gtk_image_new_from_file (const gchar *filename)
{
  GtkWidget *w = gtk_image_new ();
  gtk_image_set_from_file (GTK_IMAGE (w), filename);
  return w;
}

GtkWidget *gtk_image_new_from_icon_name (const gchar *icon_name, GtkIconSize size)
{
  GtkWidget *w = gtk_image_new ();
  gtk_image_set_from_icon_name (GTK_IMAGE (w), icon_name, size);
  return w;
}

GtkWidget *gtk_image_new_from_stock (const gchar *stock_id, GtkIconSize size) { return gtk_image_new_from_icon_name (stock_id, size); }

GtkWidget *gtk_image_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  GtkWidget *w = gtk_image_new ();
  gtk_image_set_from_pixbuf (GTK_IMAGE (w), pixbuf);
  return w;
}

void gtk_image_set_from_file (GtkImage *image, const gchar *filename)
{
  auto *l = gtkqt_q<QLabel> (GTK_WIDGET (image));
  if (!l) return;
  QPixmap pm (gtkqt_str (filename));
  if (pm.isNull ()) l->setPixmap (QApplication::style ()->standardIcon (QStyle::SP_MessageBoxWarning).pixmap (16, 16));
  else l->setPixmap (pm);
}

void gtk_image_set_from_icon_name (GtkImage *image, const gchar *icon_name, GtkIconSize size)
{
  g_object_set_data_full (G_OBJECT (image), "gtkqt-icon-name", g_strdup (icon_name), g_free);
  g_object_set_data (G_OBJECT (image), "gtkqt-icon-px", GINT_TO_POINTER (gtkqt_icon_size_px (size)));
  image_show_icon (image);
}

void gtk_image_set_from_pixbuf (GtkImage *image, GdkPixbuf *pixbuf)
{
  if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (image))) l->setPixmap (QPixmap::fromImage (gtkqt_pixbuf_image (pixbuf)));
}

void gtk_image_set_pixel_size (GtkImage *image, gint pixel_size)
{
  g_object_set_data (G_OBJECT (image), "gtkqt-pixel-size", GINT_TO_POINTER (pixel_size));
  image_show_icon (image);
}

void gtk_image_clear (GtkImage *image) { if (auto *l = gtkqt_q<QLabel> (GTK_WIDGET (image))) l->clear (); }
GtkWidget *gtk_picture_new_for_filename (const char *filename) { return gtk_image_new_from_file (filename); }
}

static QObject *spinner_create (GtkWidget *)
{
  auto *p = new QProgressBar;
  p->setTextVisible (false);
  p->setRange (0, 1);
  p->setValue (0);
  p->setFixedSize (32, 12);
  return p;
}

G_DEFINE_TYPE (GtkSpinner, gtk_spinner, GTK_TYPE_WIDGET)
static void gtk_spinner_init (GtkSpinner *) {}
static void gtk_spinner_class_init (GtkSpinnerClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), spinner_create);
  gtkqt_prop (klass, g_param_spec_boolean ("spinning", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_spinner_set_spinning (GTK_SPINNER (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("active", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_spinner_set_spinning (GTK_SPINNER (o), g_value_get_boolean (v)); }, nullptr);
}

extern "C" {
GtkWidget *gtk_spinner_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_SPINNER, nullptr); }
void gtk_spinner_set_spinning (GtkSpinner *spinner, gboolean spinning)
{
  if (auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (spinner))) { p->setRange (0, spinning ? 0 : 1); p->setValue (0); }
}
void gtk_spinner_start (GtkSpinner *spinner) { gtk_spinner_set_spinning (spinner, TRUE); }
void gtk_spinner_stop (GtkSpinner *spinner) { gtk_spinner_set_spinning (spinner, FALSE); }
}

/* ======================================================================
 * GtkArrow
 * ====================================================================== */
namespace {
class GtkQtArrow : public QWidget {
public:
  GtkArrowType type = GTK_ARROW_RIGHT;
  QSize sizeHint () const override { int s = style ()->pixelMetric (QStyle::PM_MenuButtonIndicator, nullptr, this); return QSize (s + 4, s + 4); }
  QSize minimumSizeHint () const override { return QSize (8, 8); }
protected:
  void paintEvent (QPaintEvent *) override
  {
    if (type == GTK_ARROW_NONE) return;
    static const QStyle::PrimitiveElement pe[] = { QStyle::PE_IndicatorArrowUp, QStyle::PE_IndicatorArrowDown,
                                                   QStyle::PE_IndicatorArrowLeft, QStyle::PE_IndicatorArrowRight };
    QStyleOption opt;
    opt.initFrom (this);
    int s = qMin (width (), height ());
    opt.rect = QRect ((width () - s) / 2, (height () - s) / 2, s, s);
    QPainter p (this);
    style ()->drawPrimitive (pe[type], &opt, &p, this);
  }
};
}

static QObject *arrow_create (GtkWidget *) { return new GtkQtArrow; }

G_DEFINE_TYPE (GtkArrow, gtk_arrow, GTK_TYPE_WIDGET)
static void gtk_arrow_init (GtkArrow *a) { a->arrow_type = GTK_ARROW_RIGHT; a->shadow_type = GTK_SHADOW_OUT; }
static void gtk_arrow_class_init (GtkArrowClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), arrow_create);
  gtkqt_prop (klass, g_param_spec_enum ("arrow-type", nullptr, nullptr, gtk_arrow_type_get_type (), GTK_ARROW_RIGHT, RW),
              [] (GObject *o, const GValue *v) { gtk_arrow_set (GTK_ARROW (o), (GtkArrowType) g_value_get_enum (v), GTK_ARROW (o)->shadow_type); },
              [] (GObject *o, GValue *v) { g_value_set_enum (v, GTK_ARROW (o)->arrow_type); });
  gtkqt_prop (klass, g_param_spec_enum ("shadow-type", nullptr, nullptr, gtk_shadow_type_get_type (), GTK_SHADOW_OUT, RW),
              [] (GObject *o, const GValue *v) { GTK_ARROW (o)->shadow_type = (GtkShadowType) g_value_get_enum (v); }, nullptr);
}

extern "C" {
GtkWidget *gtk_arrow_new (GtkArrowType arrow_type, GtkShadowType shadow_type)
{
  auto *w = (GtkWidget *) g_object_new (GTK_TYPE_ARROW, nullptr);
  gtk_arrow_set (GTK_ARROW (w), arrow_type, shadow_type);
  return w;
}
void gtk_arrow_set (GtkArrow *arrow, GtkArrowType arrow_type, GtkShadowType shadow_type)
{
  arrow->arrow_type = arrow_type;
  arrow->shadow_type = shadow_type;
  if (auto *q = dynamic_cast<GtkQtArrow *> (gtkqt_qw (GTK_WIDGET (arrow)))) { q->type = arrow_type; q->update (); }
}
} /* extern "C" */
