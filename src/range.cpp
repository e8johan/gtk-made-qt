/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkAdjustment and the widgets that use one: scales, scrollbars, spin
 * buttons. Also progress and level bars.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <cmath>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

/* ======================================================================
 * GtkAdjustment
 * ====================================================================== */
enum { ADJ_CHANGED, ADJ_VALUE_CHANGED, N_ADJ_SIGNALS };
static guint adj_signals[N_ADJ_SIGNALS];

G_DEFINE_TYPE (GtkAdjustment, gtk_adjustment, G_TYPE_INITIALLY_UNOWNED)
static void gtk_adjustment_init (GtkAdjustment *) {}

#define ADJ_PROP(name, field)                                                                           \
  gtkqt_prop (klass, g_param_spec_double (name, nullptr, nullptr, -G_MAXDOUBLE, G_MAXDOUBLE, 0, RW),    \
              [] (GObject *o, const GValue *v) { GTK_ADJUSTMENT (o)->field = g_value_get_double (v);   \
                                                 gtk_adjustment_changed (GTK_ADJUSTMENT (o)); },        \
              [] (GObject *o, GValue *v) { g_value_set_double (v, GTK_ADJUSTMENT (o)->field); })

static void gtk_adjustment_class_init (GtkAdjustmentClass *klass)
{
  GType t = G_TYPE_FROM_CLASS (klass);
  adj_signals[ADJ_CHANGED] = g_signal_new ("changed", t, (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE),
                                           G_STRUCT_OFFSET (GtkAdjustmentClass, changed), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  adj_signals[ADJ_VALUE_CHANGED] = g_signal_new ("value-changed", t, (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE),
                                                 G_STRUCT_OFFSET (GtkAdjustmentClass, value_changed), nullptr, nullptr, nullptr,
                                                 G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_double ("value", nullptr, nullptr, -G_MAXDOUBLE, G_MAXDOUBLE, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_adjustment_set_value (GTK_ADJUSTMENT (o), g_value_get_double (v)); },
              [] (GObject *o, GValue *v) { g_value_set_double (v, GTK_ADJUSTMENT (o)->value); });
  ADJ_PROP ("lower", lower);
  ADJ_PROP ("upper", upper);
  ADJ_PROP ("step-increment", step_increment);
  ADJ_PROP ("page-increment", page_increment);
  ADJ_PROP ("page-size", page_size);
}

extern "C" {

#undef gtk_adjustment_new
GtkAdjustment *gtk_adjustment_new (gdouble value, gdouble lower, gdouble upper, gdouble step_increment,
                                   gdouble page_increment, gdouble page_size)
{
  auto *a = (GtkAdjustment *) g_object_new (GTK_TYPE_ADJUSTMENT, nullptr);
  a->lower = lower;
  a->upper = upper;
  a->step_increment = step_increment;
  a->page_increment = page_increment;
  a->page_size = page_size;
  a->value = CLAMP (value, lower, MAX (lower, upper - page_size));
  return a;
}

gdouble gtk_adjustment_get_value (GtkAdjustment *a) { return a->value; }

void gtk_adjustment_set_value (GtkAdjustment *a, gdouble value)
{
  value = CLAMP (value, a->lower, MAX (a->lower, a->upper - a->page_size));
  if (value == a->value) return;
  a->value = value;
  g_signal_emit (a, adj_signals[ADJ_VALUE_CHANGED], 0);
  g_object_notify (G_OBJECT (a), "value");
}

void gtk_adjustment_clamp_page (GtkAdjustment *a, gdouble lower, gdouble upper)
{
  if (upper > a->value + a->page_size) gtk_adjustment_set_value (a, upper - a->page_size);
  if (lower < a->value) gtk_adjustment_set_value (a, lower);
}

void gtk_adjustment_changed (GtkAdjustment *a) { g_signal_emit (a, adj_signals[ADJ_CHANGED], 0); }
void gtk_adjustment_value_changed (GtkAdjustment *a) { g_signal_emit (a, adj_signals[ADJ_VALUE_CHANGED], 0); }

void gtk_adjustment_configure (GtkAdjustment *a, gdouble value, gdouble lower, gdouble upper,
                               gdouble step_increment, gdouble page_increment, gdouble page_size)
{
  a->lower = lower;
  a->upper = upper;
  a->step_increment = step_increment;
  a->page_increment = page_increment;
  a->page_size = page_size;
  gtk_adjustment_changed (a);
  gtk_adjustment_set_value (a, value);
}

gdouble gtk_adjustment_get_lower (GtkAdjustment *a) { return a->lower; }
gdouble gtk_adjustment_get_upper (GtkAdjustment *a) { return a->upper; }
gdouble gtk_adjustment_get_step_increment (GtkAdjustment *a) { return a->step_increment; }
gdouble gtk_adjustment_get_page_increment (GtkAdjustment *a) { return a->page_increment; }
gdouble gtk_adjustment_get_page_size (GtkAdjustment *a) { return a->page_size; }
void gtk_adjustment_set_lower (GtkAdjustment *a, gdouble v) { a->lower = v; gtk_adjustment_changed (a); }
void gtk_adjustment_set_upper (GtkAdjustment *a, gdouble v) { a->upper = v; gtk_adjustment_changed (a); }
void gtk_adjustment_set_step_increment (GtkAdjustment *a, gdouble v) { a->step_increment = v; gtk_adjustment_changed (a); }
void gtk_adjustment_set_page_increment (GtkAdjustment *a, gdouble v) { a->page_increment = v; gtk_adjustment_changed (a); }
void gtk_adjustment_set_page_size (GtkAdjustment *a, gdouble v) { a->page_size = v; gtk_adjustment_changed (a); }

} /* extern "C" */

/* ======================================================================
 * GtkRange / GtkScale / GtkScrollbar
 *
 * Qt sliders are integer based, so the adjustment's double range is mapped
 * onto integers with a resolution of 10^digits (at least 1/1000 of a step).
 * ====================================================================== */
struct RangeData {
  GtkAdjustment *adj = nullptr;
  gulong h_changed = 0, h_value = 0;
  int digits = 1;
  bool draw_value = true;
  GtkPositionType value_pos = GTK_POS_TOP;
  bool syncing = false;
  bool is_scale = false;
};

static RangeData *range_data (GtkWidget *w)
{
  auto *d = static_cast<RangeData *> (g_object_get_data (G_OBJECT (w), "gtkqt-range"));
  if (!d) {
    d = new RangeData;
    g_object_set_data_full (G_OBJECT (w), "gtkqt-range", d, [] (gpointer p) {
      auto *rd = static_cast<RangeData *> (p);
      if (rd->adj) g_object_unref (rd->adj);
      delete rd;
    });
  }
  return d;
}

static double range_resolution (RangeData *d)
{
  double r = std::pow (10.0, qMax (0, d->digits));
  if (d->adj && d->adj->step_increment > 0) r = qMax (r, 1.0 / d->adj->step_increment);
  return qMin (r, 1e6);
}

static QString range_format (GtkWidget *w, double value)
{
  RangeData *d = range_data (w);
  gchar *custom = nullptr;
  guint id = g_signal_lookup ("format-value", G_OBJECT_TYPE (w));
  if (id && g_signal_has_handler_pending (w, id, 0, FALSE))
    g_signal_emit (w, id, 0, value, &custom);
  if (custom) {
    QString s = gtkqt_str (custom);
    g_free (custom);
    return s;
  }
  return QString::number (value, 'f', qMax (0, d->digits));
}

static void range_sync_from_adj (GtkWidget *w)
{
  RangeData *d = range_data (w);
  auto *sl = gtkqt_inner<QAbstractSlider> (w);
  if (!sl || !d->adj) return;
  GtkAdjustment *a = d->adj;
  double res = range_resolution (d);
  d->syncing = true;
  sl->setRange (qRound (a->lower * res), qRound ((a->upper - a->page_size) * res));
  sl->setSingleStep (qMax (1, (int) qRound (a->step_increment * res)));
  sl->setPageStep (qMax (1, (int) qRound ((qobject_cast<QScrollBar *> (sl) ? a->page_size : a->page_increment) * res)));
  sl->setValue (qRound (a->value * res));
  d->syncing = false;
  if (auto *label = qobject_cast<QLabel *> (w->priv->content.data ()))
    label->setText (range_format (w, a->value));
}

static void range_value_changed_cb (GtkAdjustment *, gpointer w)
{
  range_sync_from_adj ((GtkWidget *) w);
  g_signal_emit_by_name (w, "value-changed");
}

static void range_changed_cb (GtkAdjustment *, gpointer w) { range_sync_from_adj ((GtkWidget *) w); }

static void range_connect_slider (GtkWidget *w, QAbstractSlider *sl)
{
  w->priv->inner = sl;
  gtkqt_connect (w, sl, &QAbstractSlider::valueChanged, [] (GtkWidget *rw, int v) {
    RangeData *d = range_data (rw);
    if (d->syncing || !d->adj) return;
    double value = v / range_resolution (d);
    if (d->is_scale && d->digits >= 0) value = std::round (value * std::pow (10.0, d->digits)) / std::pow (10.0, d->digits);
    gtk_adjustment_set_value (d->adj, value);
  });
}

static QObject *scale_create (GtkWidget *w)
{
  auto *qw = new QWidget;
  auto *l = new QBoxLayout (QBoxLayout::TopToBottom, qw);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (2);
  auto *label = new QLabel (qw);
  label->setAlignment (Qt::AlignCenter);
  Qt::Orientation o = GTK_IS_VSCALE (w) ? Qt::Vertical : Qt::Horizontal;
  auto *sl = new QSlider (o, qw);
  if (o == Qt::Vertical) sl->setInvertedAppearance (true);   /* GTK puts the minimum at the top */
  l->addWidget (label);
  l->addWidget (sl, 1);
  w->priv->content = label;
  range_data (w)->is_scale = true;
  range_connect_slider (w, sl);
  if (o == Qt::Vertical) qw->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);
  return qw;
}

static QObject *scrollbar_create (GtkWidget *w)
{
  auto *sb = new QScrollBar (GTK_IS_VSCROLLBAR (w) ? Qt::Vertical : Qt::Horizontal);
  range_data (w)->digits = 0;
  range_connect_slider (w, sb);
  return sb;
}

static void scale_layout (GtkWidget *w)
{
  RangeData *d = range_data (w);
  QWidget *qw = gtkqt_qw (w);
  auto *label = qobject_cast<QLabel *> (w->priv->content.data ());
  auto *sl = gtkqt_inner<QAbstractSlider> (w);
  if (!qw || !label || !sl) return;
  auto *l = static_cast<QBoxLayout *> (qw->layout ());
  l->removeWidget (label);
  l->removeWidget (sl);
  bool before = d->value_pos == GTK_POS_TOP || d->value_pos == GTK_POS_LEFT;
  bool vertical_stack = d->value_pos == GTK_POS_TOP || d->value_pos == GTK_POS_BOTTOM;
  l->setDirection (vertical_stack ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
  if (before) { l->addWidget (label); l->addWidget (sl, 1); }
  else { l->addWidget (sl, 1); l->addWidget (label); }
  label->setVisible (d->draw_value);
}

G_DEFINE_TYPE (GtkRange, gtk_range, GTK_TYPE_WIDGET)
G_DEFINE_TYPE (GtkScale, gtk_scale, GTK_TYPE_RANGE)
G_DEFINE_TYPE (GtkHScale, gtk_hscale, GTK_TYPE_SCALE)
G_DEFINE_TYPE (GtkVScale, gtk_vscale, GTK_TYPE_SCALE)
G_DEFINE_TYPE (GtkScrollbar, gtk_scrollbar, GTK_TYPE_RANGE)
G_DEFINE_TYPE (GtkHScrollbar, gtk_hscrollbar, GTK_TYPE_SCROLLBAR)
G_DEFINE_TYPE (GtkVScrollbar, gtk_vscrollbar, GTK_TYPE_SCROLLBAR)

static void gtk_range_init (GtkRange *) {}
static void gtk_range_constructed (GObject *o)
{
  G_OBJECT_CLASS (gtk_range_parent_class)->constructed (o);
  if (!range_data (GTK_WIDGET (o))->adj)
    gtk_range_set_adjustment (GTK_RANGE (o), gtk_adjustment_new (0, 0, 100, 1, 10, 0));
}
static void gtk_range_dispose (GObject *o)
{
  RangeData *d = range_data (GTK_WIDGET (o));
  if (d->adj) {
    g_signal_handler_disconnect (d->adj, d->h_changed);
    g_signal_handler_disconnect (d->adj, d->h_value);
    g_object_unref (d->adj);
    d->adj = nullptr;
  }
  G_OBJECT_CLASS (gtk_range_parent_class)->dispose (o);
}
static void gtk_range_class_init (GtkRangeClass *klass)
{
  G_OBJECT_CLASS (klass)->constructed = gtk_range_constructed;
  G_OBJECT_CLASS (klass)->dispose = gtk_range_dispose;
  g_signal_new ("value-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_object ("adjustment", nullptr, nullptr, GTK_TYPE_ADJUSTMENT, RW),
              [] (GObject *o, const GValue *v) { if (auto *a = (GtkAdjustment *) g_value_get_object (v)) gtk_range_set_adjustment (GTK_RANGE (o), a); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_range_get_adjustment (GTK_RANGE (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("inverted", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_range_set_inverted (GTK_RANGE (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("orientation", nullptr, nullptr, gtk_orientation_get_type (), GTK_ORIENTATION_HORIZONTAL, RW),
              [] (GObject *o, const GValue *v) { gtk_orientable_set_orientation ((GtkOrientable *) o, (GtkOrientation) g_value_get_enum (v)); }, nullptr);
}
static void gtk_scale_init (GtkScale *) {}
static void gtk_scale_class_init (GtkScaleClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), scale_create);
  g_signal_new ("format-value", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, g_signal_accumulator_first_wins, nullptr,
                nullptr, G_TYPE_STRING, 1, G_TYPE_DOUBLE);
  gtkqt_prop (klass, g_param_spec_int ("digits", nullptr, nullptr, -1, 64, 1, RW),
              [] (GObject *o, const GValue *v) { gtk_scale_set_digits (GTK_SCALE (o), g_value_get_int (v)); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, gtk_scale_get_digits (GTK_SCALE (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("draw-value", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_scale_set_draw_value (GTK_SCALE (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_scale_get_draw_value (GTK_SCALE (o))); });
  gtkqt_prop (klass, g_param_spec_enum ("value-pos", nullptr, nullptr, gtk_position_type_get_type (), GTK_POS_TOP, RW),
              [] (GObject *o, const GValue *v) { gtk_scale_set_value_pos (GTK_SCALE (o), (GtkPositionType) g_value_get_enum (v)); }, nullptr);
}
static void gtk_hscale_init (GtkHScale *) {}
static void gtk_hscale_class_init (GtkHScaleClass *) {}
static void gtk_vscale_init (GtkVScale *) {}
static void gtk_vscale_class_init (GtkVScaleClass *) {}
static void gtk_scrollbar_init (GtkScrollbar *) {}
static void gtk_scrollbar_class_init (GtkScrollbarClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), scrollbar_create); }
static void gtk_hscrollbar_init (GtkHScrollbar *) {}
static void gtk_hscrollbar_class_init (GtkHScrollbarClass *) {}
static void gtk_vscrollbar_init (GtkVScrollbar *) {}
static void gtk_vscrollbar_class_init (GtkVScrollbarClass *) {}

extern "C" {

GtkAdjustment *gtk_range_get_adjustment (GtkRange *range) { return range_data (GTK_WIDGET (range))->adj; }

void gtk_range_set_adjustment (GtkRange *range, GtkAdjustment *adjustment)
{
  RangeData *d = range_data (GTK_WIDGET (range));
  if (d->adj == adjustment) return;
  if (d->adj) {
    g_signal_handler_disconnect (d->adj, d->h_changed);
    g_signal_handler_disconnect (d->adj, d->h_value);
    g_object_unref (d->adj);
  }
  d->adj = adjustment;
  if (!adjustment) return;
  g_object_ref_sink (adjustment);
  d->h_changed = g_signal_connect (adjustment, "changed", G_CALLBACK (range_changed_cb), range);
  d->h_value = g_signal_connect (adjustment, "value-changed", G_CALLBACK (range_value_changed_cb), range);
  range_sync_from_adj (GTK_WIDGET (range));
}

gdouble gtk_range_get_value (GtkRange *range)
{
  RangeData *d = range_data (GTK_WIDGET (range));
  return d->adj ? d->adj->value : 0;
}

void gtk_range_set_value (GtkRange *range, gdouble value)
{
  if (GtkAdjustment *a = range_data (GTK_WIDGET (range))->adj) gtk_adjustment_set_value (a, value);
}

void gtk_range_set_range (GtkRange *range, gdouble min, gdouble max)
{
  GtkAdjustment *a = range_data (GTK_WIDGET (range))->adj;
  if (!a) return;
  gtk_adjustment_configure (a, CLAMP (a->value, min, max), min, max, a->step_increment, a->page_increment, a->page_size);
}

void gtk_range_set_increments (GtkRange *range, gdouble step, gdouble page)
{
  GtkAdjustment *a = range_data (GTK_WIDGET (range))->adj;
  if (!a) return;
  a->step_increment = step;
  a->page_increment = page;
  gtk_adjustment_changed (a);
}

void gtk_range_set_inverted (GtkRange *range, gboolean setting)
{
  if (auto *sl = gtkqt_inner<QAbstractSlider> (GTK_WIDGET (range)))
    sl->setInvertedAppearance (qobject_cast<QSlider *> (sl) && sl->orientation () == Qt::Vertical ? !setting : setting);
}

void gtk_range_set_update_policy (GtkRange *range, GtkUpdateType policy)
{
  if (auto *sl = gtkqt_inner<QAbstractSlider> (GTK_WIDGET (range))) sl->setTracking (policy == GTK_UPDATE_CONTINUOUS);
}

GtkWidget *gtk_scale_new (GtkOrientation orientation, GtkAdjustment *adjustment)
{
  GtkWidget *w = (GtkWidget *) g_object_new (orientation == GTK_ORIENTATION_VERTICAL ? GTK_TYPE_VSCALE : GTK_TYPE_HSCALE,
                                             "adjustment", adjustment, nullptr);
  return w;
}

GtkWidget *gtk_scale_new_with_range (GtkOrientation orientation, gdouble min, gdouble max, gdouble step)
{
  GtkWidget *w = gtk_scale_new (orientation, gtk_adjustment_new (min, min, max, step, 10 * step, 0));
  int digits = 0;
  for (double s = std::fabs (step); s > 0 && s < 1 && digits < 5; s *= 10) digits++;
  gtk_scale_set_digits (GTK_SCALE (w), digits);
  return w;
}

GtkWidget *gtk_hscale_new (GtkAdjustment *adjustment) { return gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment); }
GtkWidget *gtk_vscale_new (GtkAdjustment *adjustment) { return gtk_scale_new (GTK_ORIENTATION_VERTICAL, adjustment); }
GtkWidget *gtk_hscale_new_with_range (gdouble min, gdouble max, gdouble step) { return gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, min, max, step); }
GtkWidget *gtk_vscale_new_with_range (gdouble min, gdouble max, gdouble step) { return gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL, min, max, step); }

void gtk_scale_set_digits (GtkScale *scale, gint digits)
{
  range_data (GTK_WIDGET (scale))->digits = digits;
  range_sync_from_adj (GTK_WIDGET (scale));
}

gint gtk_scale_get_digits (GtkScale *scale) { return range_data (GTK_WIDGET (scale))->digits; }

void gtk_scale_set_draw_value (GtkScale *scale, gboolean draw_value)
{
  range_data (GTK_WIDGET (scale))->draw_value = draw_value;
  scale_layout (GTK_WIDGET (scale));
}

gboolean gtk_scale_get_draw_value (GtkScale *scale) { return range_data (GTK_WIDGET (scale))->draw_value; }

void gtk_scale_set_value_pos (GtkScale *scale, GtkPositionType pos)
{
  range_data (GTK_WIDGET (scale))->value_pos = pos;
  scale_layout (GTK_WIDGET (scale));
}

void gtk_scale_set_has_origin (GtkScale *, gboolean) {}

void gtk_scale_add_mark (GtkScale *scale, gdouble, GtkPositionType, const gchar *)
{
  if (auto *s = gtkqt_inner<QSlider> (GTK_WIDGET (scale))) s->setTickPosition (QSlider::TicksBelow);
}

void gtk_scale_clear_marks (GtkScale *scale)
{
  if (auto *s = gtkqt_inner<QSlider> (GTK_WIDGET (scale))) s->setTickPosition (QSlider::NoTicks);
}

GtkWidget *gtk_scrollbar_new (GtkOrientation orientation, GtkAdjustment *adjustment)
{
  return (GtkWidget *) g_object_new (orientation == GTK_ORIENTATION_VERTICAL ? GTK_TYPE_VSCROLLBAR : GTK_TYPE_HSCROLLBAR,
                                     "adjustment", adjustment, nullptr);
}

GtkWidget *gtk_hscrollbar_new (GtkAdjustment *adjustment) { return gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, adjustment); }
GtkWidget *gtk_vscrollbar_new (GtkAdjustment *adjustment) { return gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adjustment); }

} /* extern "C" */

/* ======================================================================
 * GtkSpinButton
 * ====================================================================== */
static void spin_sync (GtkWidget *w)
{
  RangeData *d = range_data (w);
  auto *s = gtkqt_q<QDoubleSpinBox> (w);
  if (!s || !d->adj) return;
  d->syncing = true;
  s->setDecimals (qMax (0, d->digits));
  s->setRange (d->adj->lower, d->adj->upper - d->adj->page_size);
  s->setSingleStep (d->adj->step_increment > 0 ? d->adj->step_increment : 1);
  s->setValue (d->adj->value);
  d->syncing = false;
}

static void spin_value_cb (GtkAdjustment *, gpointer w)
{
  spin_sync ((GtkWidget *) w);
  g_signal_emit_by_name (w, "value-changed");
}

static void spin_changed_cb (GtkAdjustment *, gpointer w) { spin_sync ((GtkWidget *) w); }

static QObject *spin_create (GtkWidget *w)
{
  auto *s = new QDoubleSpinBox;
  range_data (w)->digits = 0;
  gtkqt_connect (w, s, QOverload<double>::of (&QDoubleSpinBox::valueChanged), [] (GtkWidget *sw, double v) {
    RangeData *d = range_data (sw);
    if (!d->syncing && d->adj) gtk_adjustment_set_value (d->adj, v);
  });
  gtkqt_connect (w, s->findChild<QLineEdit *> (), &QLineEdit::textChanged, [] (GtkWidget *sw, const QString &) {
    g_signal_emit_by_name (sw, "changed");
  });
  gtkqt_connect (w, s, &QAbstractSpinBox::editingFinished, [] (GtkWidget *sw) { g_signal_emit_by_name (sw, "activate"); });
  return s;
}

G_DEFINE_TYPE (GtkSpinButton, gtk_spin_button, GTK_TYPE_ENTRY)
static void gtk_spin_button_init (GtkSpinButton *) {}
static void gtk_spin_button_class_init (GtkSpinButtonClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), spin_create);
  g_signal_new ("value-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_object ("adjustment", nullptr, nullptr, GTK_TYPE_ADJUSTMENT, RW),
              [] (GObject *o, const GValue *v) { if (auto *a = (GtkAdjustment *) g_value_get_object (v)) gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (o), a); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (o))); });
  gtkqt_prop (klass, g_param_spec_uint ("digits", nullptr, nullptr, 0, 20, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_spin_button_set_digits (GTK_SPIN_BUTTON (o), g_value_get_uint (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_double ("value", nullptr, nullptr, -G_MAXDOUBLE, G_MAXDOUBLE, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_spin_button_set_value (GTK_SPIN_BUTTON (o), g_value_get_double (v)); },
              [] (GObject *o, GValue *v) { g_value_set_double (v, gtk_spin_button_get_value (GTK_SPIN_BUTTON (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("numeric", nullptr, nullptr, FALSE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("wrap", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_double ("climb-rate", nullptr, nullptr, 0, G_MAXDOUBLE, 0, RW), [] (GObject *, const GValue *) {}, nullptr);
}

extern "C" {

GtkWidget *gtk_spin_button_new (GtkAdjustment *adjustment, gdouble, guint digits)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_SPIN_BUTTON, nullptr);
  range_data (w)->digits = digits;
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (w), adjustment ? adjustment : gtk_adjustment_new (0, 0, 0, 0, 0, 0));
  return w;
}

GtkWidget *gtk_spin_button_new_with_range (gdouble min, gdouble max, gdouble step)
{
  int digits = 0;
  for (double s = std::fabs (step); s > 0 && s < 1 && digits < 10; s *= 10) digits++;
  return gtk_spin_button_new (gtk_adjustment_new (min, min, max, step, 10 * step, 0), step, digits);
}

void gtk_spin_button_set_adjustment (GtkSpinButton *spin, GtkAdjustment *adjustment)
{
  RangeData *d = range_data (GTK_WIDGET (spin));
  if (d->adj == adjustment) return;
  if (d->adj) {
    g_signal_handler_disconnect (d->adj, d->h_changed);
    g_signal_handler_disconnect (d->adj, d->h_value);
    g_object_unref (d->adj);
  }
  d->adj = adjustment;
  if (!adjustment) return;
  g_object_ref_sink (adjustment);
  d->h_changed = g_signal_connect (adjustment, "changed", G_CALLBACK (spin_changed_cb), spin);
  d->h_value = g_signal_connect (adjustment, "value-changed", G_CALLBACK (spin_value_cb), spin);
  spin_sync (GTK_WIDGET (spin));
}

GtkAdjustment *gtk_spin_button_get_adjustment (GtkSpinButton *spin) { return range_data (GTK_WIDGET (spin))->adj; }

void gtk_spin_button_set_digits (GtkSpinButton *spin, guint digits)
{
  range_data (GTK_WIDGET (spin))->digits = digits;
  spin_sync (GTK_WIDGET (spin));
}

void gtk_spin_button_set_increments (GtkSpinButton *spin, gdouble step, gdouble page)
{
  if (GtkAdjustment *a = range_data (GTK_WIDGET (spin))->adj) {
    a->step_increment = step;
    a->page_increment = page;
    gtk_adjustment_changed (a);
  }
}

void gtk_spin_button_set_range (GtkSpinButton *spin, gdouble min, gdouble max)
{
  if (GtkAdjustment *a = range_data (GTK_WIDGET (spin))->adj)
    gtk_adjustment_configure (a, CLAMP (a->value, min, max), min, max, a->step_increment, a->page_increment, a->page_size);
}

gdouble gtk_spin_button_get_value (GtkSpinButton *spin)
{
  /* text typed but not yet committed (no Return or focus change yet) */
  if (auto *sb = gtkqt_inner<QAbstractSpinBox> (GTK_WIDGET (spin))) {
    if (sb->hasAcceptableInput ()) sb->interpretText ();
  } else if (auto *sb2 = qobject_cast<QAbstractSpinBox *> (gtkqt_qw (GTK_WIDGET (spin)))) {
    if (sb2->hasAcceptableInput ()) sb2->interpretText ();
  }
  GtkAdjustment *a = range_data (GTK_WIDGET (spin))->adj;
  return a ? a->value : 0;
}

gint gtk_spin_button_get_value_as_int (GtkSpinButton *spin) { return (gint) std::lround (gtk_spin_button_get_value (spin)); }

void gtk_spin_button_set_value (GtkSpinButton *spin, gdouble value)
{
  if (GtkAdjustment *a = range_data (GTK_WIDGET (spin))->adj) gtk_adjustment_set_value (a, value);
}

void gtk_spin_button_set_numeric (GtkSpinButton *, gboolean) {}
void gtk_spin_button_set_wrap (GtkSpinButton *spin, gboolean wrap) { if (auto *s = gtkqt_q<QDoubleSpinBox> (GTK_WIDGET (spin))) s->setWrapping (wrap); }
void gtk_spin_button_set_update_policy (GtkSpinButton *, GtkSpinButtonUpdatePolicy) {}
void gtk_spin_button_update (GtkSpinButton *spin) { if (auto *s = gtkqt_q<QDoubleSpinBox> (GTK_WIDGET (spin))) s->interpretText (); }

} /* extern "C" */

/* ======================================================================
 * GtkProgressBar / GtkLevelBar
 * ====================================================================== */
static QObject *progress_create (GtkWidget *)
{
  auto *p = new QProgressBar;
  p->setRange (0, 10000);
  p->setValue (0);
  p->setTextVisible (false);
  return p;
}

G_DEFINE_TYPE (GtkProgressBar, gtk_progress_bar, GTK_TYPE_WIDGET)
static void gtk_progress_bar_init (GtkProgressBar *) {}
static void gtk_progress_bar_class_init (GtkProgressBarClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), progress_create);
  gtkqt_prop (klass, g_param_spec_double ("fraction", nullptr, nullptr, 0, 1, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (o), g_value_get_double (v)); },
              [] (GObject *o, GValue *v) { g_value_set_double (v, gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (o))); });
  gtkqt_prop (klass, g_param_spec_string ("text", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_progress_bar_set_text (GTK_PROGRESS_BAR (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("show-text", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("inverted", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_double ("pulse-step", nullptr, nullptr, 0, 1, 0.1, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("orientation", nullptr, nullptr, gtk_orientation_get_type (), GTK_ORIENTATION_HORIZONTAL, RW),
              [] (GObject *o, const GValue *v) { gtk_orientable_set_orientation ((GtkOrientable *) o, (GtkOrientation) g_value_get_enum (v)); }, nullptr);
}

extern "C" {
GtkWidget *gtk_progress_bar_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_PROGRESS_BAR, nullptr); }

void gtk_progress_bar_set_fraction (GtkProgressBar *pbar, gdouble fraction)
{
  if (auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (pbar))) {
    if (p->maximum () == 0) p->setRange (0, 10000);
    p->setValue (qRound (CLAMP (fraction, 0.0, 1.0) * 10000));
  }
  g_object_notify (G_OBJECT (pbar), "fraction");
}

gdouble gtk_progress_bar_get_fraction (GtkProgressBar *pbar)
{
  auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (pbar));
  return p && p->maximum () > 0 ? p->value () / 10000.0 : 0;
}

void gtk_progress_bar_pulse (GtkProgressBar *pbar)
{
  if (auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (pbar))) p->setRange (0, 0);   /* busy indicator */
}

void gtk_progress_bar_set_pulse_step (GtkProgressBar *, gdouble) {}

void gtk_progress_bar_set_text (GtkProgressBar *pbar, const gchar *text)
{
  auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (pbar));
  if (!p) return;
  p->setFormat (text ? gtkqt_str (text).replace ('%', QStringLiteral ("%%")) : QStringLiteral ("%p%"));
  if (gtkqt_api < 3 && text) p->setTextVisible (true);
}

void gtk_progress_bar_set_show_text (GtkProgressBar *pbar, gboolean show_text)
{
  if (auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (pbar))) p->setTextVisible (show_text);
}

void gtk_progress_bar_set_inverted (GtkProgressBar *pbar, gboolean inverted)
{
  if (auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (pbar))) p->setInvertedAppearance (inverted);
}

void gtk_progress_bar_set_orientation (GtkProgressBar *pbar, GtkProgressBarOrientation orientation)
{
  if (auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (pbar))) {
    p->setOrientation (orientation >= GTK_PROGRESS_BOTTOM_TO_TOP ? Qt::Vertical : Qt::Horizontal);
    p->setInvertedAppearance (orientation == GTK_PROGRESS_RIGHT_TO_LEFT || orientation == GTK_PROGRESS_TOP_TO_BOTTOM);
  }
}
}

struct LevelData { double min = 0, max = 1, value = 0; };

static QObject *level_create (GtkWidget *)
{
  auto *p = new QProgressBar;
  p->setRange (0, 1000);
  p->setTextVisible (false);
  return p;
}

G_DEFINE_TYPE (GtkLevelBar, gtk_level_bar, GTK_TYPE_WIDGET)
static void gtk_level_bar_init (GtkLevelBar *) {}
static void gtk_level_bar_class_init (GtkLevelBarClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), level_create);
  gtkqt_prop (klass, g_param_spec_double ("value", nullptr, nullptr, 0, G_MAXDOUBLE, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_level_bar_set_value (GTK_LEVEL_BAR (o), g_value_get_double (v)); },
              [] (GObject *o, GValue *v) { g_value_set_double (v, gtk_level_bar_get_value (GTK_LEVEL_BAR (o))); });
}

static LevelData *level_data (GtkLevelBar *b)
{
  auto *d = static_cast<LevelData *> (g_object_get_data (G_OBJECT (b), "gtkqt-level"));
  if (!d) {
    d = new LevelData;
    g_object_set_data_full (G_OBJECT (b), "gtkqt-level", d, [] (gpointer p) { delete static_cast<LevelData *> (p); });
  }
  return d;
}

extern "C" {
GtkWidget *gtk_level_bar_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_LEVEL_BAR, nullptr); }

GtkWidget *gtk_level_bar_new_for_interval (gdouble min_value, gdouble max_value)
{
  GtkWidget *w = gtk_level_bar_new ();
  LevelData *d = level_data (GTK_LEVEL_BAR (w));
  d->min = min_value;
  d->max = max_value;
  return w;
}

void gtk_level_bar_set_value (GtkLevelBar *self, gdouble value)
{
  LevelData *d = level_data (self);
  d->value = value;
  if (auto *p = gtkqt_q<QProgressBar> (GTK_WIDGET (self)))
    p->setValue (d->max > d->min ? qRound ((value - d->min) / (d->max - d->min) * 1000) : 0);
}

gdouble gtk_level_bar_get_value (GtkLevelBar *self) { return level_data (self)->value; }
}
