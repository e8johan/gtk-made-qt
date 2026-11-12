/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Containers: GtkWindow, boxes, grids, tables, frames, paned, notebook,
 * stack, scrolled window, header bar, separators, status bar...
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

static QBoxLayout *box_layout (GtkWidget *w)
{
  QWidget *c = gtkqt_content (w);
  return c ? qobject_cast<QBoxLayout *> (c->layout ()) : nullptr;
}

/* ======================================================================
 * GtkWindow
 * ====================================================================== */
static GListStore *s_toplevel_model = nullptr;

static GListStore *toplevel_model ()
{
  if (!s_toplevel_model) s_toplevel_model = g_list_store_new (GTK_TYPE_WIDGET);
  return s_toplevel_model;
}

struct WindowData {
  int default_w = -1, default_h = -1;
  GtkWindowPosition position = GTK_WIN_POS_NONE;
  GtkWindow *transient_for = nullptr;
  bool shown_once = false;
  bool modal = false;
  bool user_ref = true;
  GtkApplication *application = nullptr;
  GtkWidget *titlebar = nullptr;
};

static WindowData *window_data (GtkWindow *w)
{
  auto *d = static_cast<WindowData *> (g_object_get_data (G_OBJECT (w), "gtkqt-window"));
  if (!d) {
    d = new WindowData;
    g_object_set_data_full (G_OBJECT (w), "gtkqt-window", d, [] (gpointer p) { delete static_cast<WindowData *> (p); });
  }
  return d;
}

/* QDialog with GTK semantics for Escape (it should behave like closing). */
class GtkQtDialogWidget : public QDialog {
public:
  void reject () override { close (); }
};

static QObject *window_create (GtkWidget *w)
{
  /* plain windows are canvases: programs draw on top-level windows too (XMMS) */
  QWidget *win = GTK_IS_DIALOG (w) ? new GtkQtDialogWidget : qobject_cast<QWidget *> (gtkqt_canvas_create (w));
  if (!GTK_IS_DIALOG (w)) win->setWindowFlags (Qt::Window);
  auto *l = new QVBoxLayout (win);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);
  return win;
}

static void window_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *win = gtkqt_qw (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!win || !cw) return;
  auto *l = qobject_cast<QBoxLayout *> (win->layout ());
  WindowData *d = window_data (GTK_WINDOW (c));
  if (child == d->titlebar) l->insertWidget (0, cw);
  else l->addWidget (cw, 1);
}

G_DEFINE_TYPE (GtkWindow, gtk_window, GTK_TYPE_BIN)

static void gtk_window_real_dispose (GObject *o)
{
  GtkWindow *win = GTK_WINDOW (o);
  WindowData *d = window_data (win);
  bool drop_ref = d->user_ref;
  d->user_ref = false;
  if (d->titlebar) {
    GtkWidget *tb = d->titlebar;
    d->titlebar = nullptr;
    gtk_widget_destroy (tb);
    g_object_unref (tb);
  }
  if (d->application) gtk_application_remove_window (d->application, win);
  guint pos;
  if (s_toplevel_model && g_list_store_find (s_toplevel_model, o, &pos))
    g_list_store_remove (s_toplevel_model, pos);
  G_OBJECT_CLASS (gtk_window_parent_class)->dispose (o);
  /* GTK holds a reference on toplevels until they are destroyed. */
  if (drop_ref) g_object_unref (o);
}

static void gtk_window_finalize (GObject *o)
{
  g_free (GTK_WINDOW (o)->title);
  G_OBJECT_CLASS (gtk_window_parent_class)->finalize (o);
}

static void gtk_window_init (GtkWindow *win)
{
  GTK_WIDGET (win)->priv->visible = false;
  GTK_WIDGET (win)->flags |= GTK_TOPLEVEL;
  g_object_ref_sink (win);
  window_data (win)->user_ref = true;
  g_list_store_append (toplevel_model (), win);
}

static gboolean window_close_request_accumulator (GSignalInvocationHint *, GValue *ret, const GValue *handler_ret, gpointer)
{
  gboolean handled = g_value_get_boolean (handler_ret);
  g_value_set_boolean (ret, handled);
  return !handled;
}

static void window_forall (GtkContainer *c, gboolean include_internals, GtkCallback cb, gpointer data)
{
  GTK_CONTAINER_CLASS (gtk_window_parent_class)->forall (c, include_internals, cb, data);
  if (include_internals)
    if (GtkWidget *tb = window_data (GTK_WINDOW (c))->titlebar) cb (tb, data);   /* show_all reaches the header bar */
}

static void gtk_window_class_init (GtkWindowClass *klass)
{
  GTK_CONTAINER_CLASS (klass)->forall = window_forall;
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  oc->dispose = gtk_window_real_dispose;
  oc->finalize = gtk_window_finalize;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), window_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), window_place, gtkqt_bin_unplace_simple);
  g_signal_new ("close-request", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GtkWindowClass, close_request), window_close_request_accumulator, nullptr, nullptr,
                G_TYPE_BOOLEAN, 0);
  g_signal_new ("activate-default", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GtkWindowClass, activate_default), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("set-focus", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr,
                G_TYPE_NONE, 1, GTK_TYPE_WIDGET);

  gtkqt_prop (klass, g_param_spec_string ("title", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_title (GTK_WINDOW (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, GTK_WINDOW (o)->title); });
  gtkqt_prop (klass, g_param_spec_int ("default-width", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { auto *w = GTK_WINDOW (o); gtk_window_set_default_size (w, g_value_get_int (v), window_data (w)->default_h); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, window_data (GTK_WINDOW (o))->default_w); });
  gtkqt_prop (klass, g_param_spec_int ("default-height", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { auto *w = GTK_WINDOW (o); gtk_window_set_default_size (w, window_data (w)->default_w, g_value_get_int (v)); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, window_data (GTK_WINDOW (o))->default_h); });
  gtkqt_prop (klass, g_param_spec_boolean ("resizable", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_resizable (GTK_WINDOW (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_window_get_resizable (GTK_WINDOW (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("modal", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_modal (GTK_WINDOW (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_window_get_modal (GTK_WINDOW (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("decorated", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_decorated (GTK_WINDOW (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("deletable", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_deletable (GTK_WINDOW (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("hide-on-close", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_hide_on_close (GTK_WINDOW (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_window_get_hide_on_close (GTK_WINDOW (o))); });
  gtkqt_prop (klass, g_param_spec_string ("icon-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_icon_name (GTK_WINDOW (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("window-position", nullptr, nullptr, gtk_window_position_get_type (), GTK_WIN_POS_NONE, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_position (GTK_WINDOW (o), (GtkWindowPosition) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("type", nullptr, nullptr, gtk_window_type_get_type (), GTK_WINDOW_TOPLEVEL,
                                        (GParamFlags) (RW | G_PARAM_CONSTRUCT_ONLY)),
              [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("transient-for", nullptr, nullptr, GTK_TYPE_WINDOW, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_transient_for (GTK_WINDOW (o), (GtkWindow *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_window_get_transient_for (GTK_WINDOW (o))); });
  gtkqt_prop (klass, g_param_spec_object ("application", nullptr, nullptr, GTK_TYPE_APPLICATION, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_application (GTK_WINDOW (o), (GtkApplication *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_window_get_application (GTK_WINDOW (o))); });
  gtkqt_prop (klass, g_param_spec_object ("titlebar", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) { gtk_window_set_titlebar (GTK_WINDOW (o), (GtkWidget *) g_value_get_object (v)); }, nullptr);
}

void gtkqt_window_before_show (GtkWidget *w)
{
  WindowData *d = window_data (GTK_WINDOW (w));
  QWidget *qw = gtkqt_qw (w);
  if (!qw || d->shown_once) return;
  d->shown_once = true;
  if (d->modal) qw->setWindowModality (Qt::ApplicationModal);
  /* GTK focuses the first focusable widget in geometric order (top to
   * bottom, then left to right), Qt the first one created: for dialogs
   * built with their buttons first, that would be a button. */
  QPointer<QWidget> guard (qw);
  QTimer::singleShot (0, qw, [guard] {
    if (!guard || guard->property ("gtkqt-focus-chosen").toBool ()) return;
    QWidget *best = nullptr;
    QPoint bp;
    for (QWidget *c : guard->findChildren<QWidget *> ()) {
      if (!c->isVisibleTo (guard) || !c->isEnabled () || !(c->focusPolicy () & Qt::TabFocus) || c->focusProxy ()) continue;
      if (qobject_cast<QAbstractButton *> (c)) continue;   /* GTK 2 buttons do take focus, but are never first in practice */
      QPoint p = c->mapTo (guard, QPoint (0, 0));
      if (!best || p.y () < bp.y () || (p.y () == bp.y () && p.x () < bp.x ())) { best = c; bp = p; }
    }
    if (best) best->setFocus (Qt::TabFocusReason);
  });
  qw->adjustSize ();
  QSize sz = qw->size ();
  if (d->default_w > 0 || d->default_h > 0) {
    QSize hint = qw->sizeHint ().expandedTo (qw->minimumSizeHint ());
    sz = QSize (d->default_w > 0 ? qMax (d->default_w, qw->minimumWidth ()) : hint.width (),
                d->default_h > 0 ? qMax (d->default_h, qw->minimumHeight ()) : hint.height ());
    qw->resize (sz);
  }
  QWidget *parent = d->transient_for ? gtkqt_qw (GTK_WIDGET (d->transient_for)) : nullptr;
  /* a position the program set (gtk_window_move, gtk_widget_set_uposition) wins, as on X */
  if (g_object_get_data (G_OBJECT (w), "gtkqt-positioned")) return;
  if (parent && (d->position == GTK_WIN_POS_CENTER_ON_PARENT || d->position == GTK_WIN_POS_NONE || GTK_IS_DIALOG (w))) {
    QRect pr = parent->frameGeometry ();
    QPoint p = pr.center () - QPoint (sz.width () / 2, sz.height () / 2);
    /* a big window over a small one (a dialog over XMMS's player) stays on the screen */
    if (QScreen *s = parent->screen ()) {
      QRect a = s->availableGeometry ();
      p.setX (qMax (a.left (), qMin (p.x (), a.right () + 1 - sz.width ())));
      p.setY (qMax (a.top (), qMin (p.y (), a.bottom () + 1 - sz.height ())));
    }
    qw->move (p);
  } else if (d->position == GTK_WIN_POS_CENTER || d->position == GTK_WIN_POS_CENTER_ALWAYS) {
    if (QScreen *s = QGuiApplication::primaryScreen ())
      qw->move (s->availableGeometry ().center () - QPoint (sz.width () / 2, sz.height () / 2));
  } else if (d->position == GTK_WIN_POS_MOUSE) {
    qw->move (QCursor::pos () - QPoint (sz.width () / 2, sz.height () / 2));
  }
}

void gtkqt_window_set_menubar_widget (GtkWindow *window, QWidget *menubar)
{
  QWidget *win = gtkqt_qw (GTK_WIDGET (window));
  if (!win) return;
  if (QLayout *l = win->layout ()) {
    if (QWidget *old = l->menuBar ()) { l->setMenuBar (nullptr); old->deleteLater (); }
    l->setMenuBar (menubar);
  }
}

void gtkqt_window_set_application_internal (GtkWindow *window, GtkApplication *app)
{
  window_data (window)->application = app;
}

extern "C" {

GtkWidget *gtk_window_new (GtkWindowType type)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_WINDOW, nullptr);
  if (type == GTK_WINDOW_POPUP)
    if (QWidget *qw = gtkqt_qw (w)) qw->setWindowFlags (Qt::Popup);
  return w;
}

void gtk_window_set_title (GtkWindow *window, const gchar *title)
{
  g_free (window->title);
  window->title = g_strdup (title);
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->setWindowTitle (gtkqt_str (title));
  g_object_notify (G_OBJECT (window), "title");
}

const gchar *gtk_window_get_title (GtkWindow *window) { return window->title; }

void gtk_window_set_default_size (GtkWindow *window, gint width, gint height)
{
  WindowData *d = window_data (window);
  d->default_w = width;
  d->default_h = height;
  QWidget *qw = gtkqt_qw (GTK_WIDGET (window));
  if (qw && d->shown_once && (width > 0 || height > 0))
    qw->resize (width > 0 ? width : qw->width (), height > 0 ? height : qw->height ());
}

void gtk_window_get_default_size (GtkWindow *window, gint *width, gint *height)
{
  WindowData *d = window_data (window);
  if (width) *width = d->default_w;
  if (height) *height = d->default_h;
}

void gtk_window_resize (GtkWindow *window, gint width, gint height)
{
  WindowData *d = window_data (window);
  if (!d->shown_once) { gtk_window_set_default_size (window, width, height); return; }
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->resize (width, height);
}

void gtk_window_get_size (GtkWindow *window, gint *width, gint *height)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (window));
  WindowData *d = window_data (window);
  QSize s = qw ? qw->size () : QSize ();
  if (qw && !d->shown_once && d->default_w > 0) s.setWidth (d->default_w);
  if (qw && !d->shown_once && d->default_h > 0) s.setHeight (d->default_h);
  if (width) *width = s.width ();
  if (height) *height = s.height ();
}

void gtk_window_move (GtkWindow *window, gint x, gint y)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->move (x, y);
  g_object_set_data (G_OBJECT (window), "gtkqt-positioned", GINT_TO_POINTER (1));
}

void gtk_window_get_position (GtkWindow *window, gint *root_x, gint *root_y)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (window));
  if (root_x) *root_x = qw ? qw->x () : 0;
  if (root_y) *root_y = qw ? qw->y () : 0;
}

void gtk_window_set_position (GtkWindow *window, GtkWindowPosition position) { window_data (window)->position = position; }

void gtk_window_set_resizable (GtkWindow *window, gboolean resizable)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (window));
  if (!qw) return;
  g_object_set_data (G_OBJECT (window), "gtkqt-fixed-size", GINT_TO_POINTER (!resizable));
  gtkqt_apply_size_request (GTK_WIDGET (window));
  if (QLayout *l = qw->layout ()) l->setSizeConstraint (resizable ? QLayout::SetDefaultConstraint : QLayout::SetFixedSize);
}

gboolean gtk_window_get_resizable (GtkWindow *window)
{
  return !g_object_get_data (G_OBJECT (window), "gtkqt-fixed-size");
}

void gtk_window_set_modal (GtkWindow *window, gboolean modal)
{
  WindowData *d = window_data (window);
  d->modal = modal;
  QWidget *qw = gtkqt_qw (GTK_WIDGET (window));
  if (!qw) return;
  bool was_visible = qw->isVisible ();
  if (was_visible) qw->hide ();   /* Qt can't change modality of a visible window */
  qw->setWindowModality (modal ? Qt::ApplicationModal : Qt::NonModal);
  if (was_visible) qw->show ();
}

gboolean gtk_window_get_modal (GtkWindow *window) { return window_data (window)->modal; }

void gtk_window_set_transient_for (GtkWindow *window, GtkWindow *parent)
{
  WindowData *d = window_data (window);
  if (d->transient_for) g_object_remove_weak_pointer (G_OBJECT (d->transient_for), (gpointer *) &d->transient_for);
  d->transient_for = parent;
  if (parent) g_object_add_weak_pointer (G_OBJECT (parent), (gpointer *) &d->transient_for);
}

GtkWindow *gtk_window_get_transient_for (GtkWindow *window) { return window_data (window)->transient_for; }
void gtk_window_set_destroy_with_parent (GtkWindow *, gboolean) {}

static void set_window_flag (GtkWindow *window, Qt::WindowType flag, bool on)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (window));
  if (!qw) return;
  bool vis = qw->isVisible ();
  qw->setWindowFlag (flag, on);
  if (vis) qw->show ();
}

void gtk_window_set_decorated (GtkWindow *window, gboolean setting) { set_window_flag (window, Qt::FramelessWindowHint, !setting); }
void gtk_window_set_deletable (GtkWindow *window, gboolean setting) { set_window_flag (window, Qt::WindowCloseButtonHint, setting); }
void gtk_window_set_keep_above (GtkWindow *window, gboolean setting) { set_window_flag (window, Qt::WindowStaysOnTopHint, setting); }

void gtk_window_set_icon_name (GtkWindow *window, const gchar *name)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->setWindowIcon (gtkqt_icon (name));
}

void gtk_window_set_default_icon_name (const gchar *name)
{
  gtkqt_ensure_app ();
  QApplication::setWindowIcon (gtkqt_icon (name));
}

gboolean gtk_window_set_icon_from_file (GtkWindow *window, const gchar *filename, GError **err)
{
  QIcon icon (gtkqt_str (filename));
  if (icon.isNull ()) {
    g_set_error (err, G_FILE_ERROR, G_FILE_ERROR_NOENT, "Could not load icon %s", filename);
    return FALSE;
  }
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->setWindowIcon (icon);
  return TRUE;
}

void gtk_window_set_type_hint (GtkWindow *, int) {}
void gtk_window_set_wmclass (GtkWindow *, const gchar *, const gchar *) {}
void gtk_window_set_role (GtkWindow *, const gchar *) {}

void gtk_window_present (GtkWindow *window)
{
  gtk_widget_show (GTK_WIDGET (window));
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) { qw->raise (); qw->activateWindow (); }
}

void gtk_window_present_with_time (GtkWindow *window, guint32) { gtk_window_present (window); }

void gtk_window_close (GtkWindow *window)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->close ();
}

void gtk_window_destroy (GtkWindow *window) { gtk_widget_destroy (GTK_WIDGET (window)); }
void gtk_window_maximize (GtkWindow *window) { if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->setWindowState (qw->windowState () | Qt::WindowMaximized); }
void gtk_window_unmaximize (GtkWindow *window) { if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->setWindowState (qw->windowState () & ~Qt::WindowMaximized); }
gboolean gtk_window_is_maximized (GtkWindow *window) { QWidget *qw = gtkqt_qw (GTK_WIDGET (window)); return qw && qw->isMaximized (); }
void gtk_window_fullscreen (GtkWindow *window) { if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->setWindowState (qw->windowState () | Qt::WindowFullScreen); }
void gtk_window_unfullscreen (GtkWindow *window) { if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->setWindowState (qw->windowState () & ~Qt::WindowFullScreen); }
void gtk_window_iconify (GtkWindow *window) { if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window))) qw->showMinimized (); }
void gtk_window_minimize (GtkWindow *window) { gtk_window_iconify (window); }
gboolean gtk_window_is_active (GtkWindow *window) { QWidget *qw = gtkqt_qw (GTK_WIDGET (window)); return qw && qw->isActiveWindow (); }
void gtk_window_set_focus (GtkWindow *, GtkWidget *focus) { if (focus) gtk_widget_grab_focus (focus); }

GtkWidget *gtk_window_get_focus (GtkWindow *window)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (window));
  return qw && qw->focusWidget () ? gtkqt_from_qobject (qw->focusWidget ()) : nullptr;
}

void gtk_window_set_default (GtkWindow *, GtkWidget *default_widget) { if (default_widget) gtk_widget_grab_default (default_widget); }
void gtk_window_set_default_widget (GtkWindow *window, GtkWidget *default_widget) { gtk_window_set_default (window, default_widget); }

void gtk_window_set_child (GtkWindow *window, GtkWidget *child)
{
  if (GtkWidget *old = GTK_BIN (window)->child) gtk_container_remove (GTK_CONTAINER (window), old);
  if (child) gtk_container_add (GTK_CONTAINER (window), child);
}

GtkWidget *gtk_window_get_child (GtkWindow *window) { return GTK_BIN (window)->child; }

void gtk_window_set_titlebar (GtkWindow *window, GtkWidget *titlebar)
{
  WindowData *d = window_data (window);
  if (d->titlebar) {
    GtkWidget *old = d->titlebar;
    d->titlebar = nullptr;
    gtk_widget_destroy (old);
    g_object_unref (old);
  }
  if (!titlebar) return;
  d->titlebar = titlebar;
  g_object_ref_sink (titlebar);
  titlebar->parent = GTK_WIDGET (window);
  window_place (GTK_CONTAINER (window), titlebar);
  gtkqt_sync_visible (titlebar);
}

void gtk_window_set_hide_on_close (GtkWindow *window, gboolean setting)
{
  g_object_set_data (G_OBJECT (window), "gtkqt-hide-on-close", GINT_TO_POINTER (setting));
}

gboolean gtk_window_get_hide_on_close (GtkWindow *window)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window), "gtkqt-hide-on-close"));
}

void gtk_window_set_application (GtkWindow *window, GtkApplication *application)
{
  WindowData *d = window_data (window);
  if (d->application == application) return;
  if (d->application) gtk_application_remove_window (d->application, window);
  if (application) gtk_application_add_window (application, window);
}

GtkApplication *gtk_window_get_application (GtkWindow *window) { return window_data (window)->application; }

void gtk_window_add_accel_group (GtkWindow *window, GtkAccelGroup *accel_group)
{
  if (!accel_group) return;
  gtkqt_accel_group_attach (accel_group, gtkqt_qw (GTK_WIDGET (window)));
  auto *groups = (GSList *) g_object_steal_data (G_OBJECT (window), "gtkqt-accel-groups");
  groups = g_slist_append (groups, g_object_ref (accel_group));
  g_object_set_data_full (G_OBJECT (window), "gtkqt-accel-groups", groups,
                          [] (gpointer l) { g_slist_free_full ((GSList *) l, g_object_unref); });
}

void gtk_window_remove_accel_group (GtkWindow *window, GtkAccelGroup *accel_group)
{
  if (!accel_group) return;
  gtkqt_accel_group_detach (accel_group, gtkqt_qw (GTK_WIDGET (window)));
  auto *groups = (GSList *) g_object_steal_data (G_OBJECT (window), "gtkqt-accel-groups");
  if (g_slist_find (groups, accel_group)) {
    groups = g_slist_remove (groups, accel_group);
    g_object_unref (accel_group);
  }
  g_object_set_data_full (G_OBJECT (window), "gtkqt-accel-groups", groups,
                          [] (gpointer l) { g_slist_free_full ((GSList *) l, g_object_unref); });
}

GSList *gtk_accel_groups_from_object (GObject *object)
{
  return (GSList *) g_object_get_data (object, "gtkqt-accel-groups");
}

GList *gtk_window_list_toplevels (void)
{
  GList *list = nullptr;
  if (!s_toplevel_model) return nullptr;
  guint n = g_list_model_get_n_items (G_LIST_MODEL (s_toplevel_model));
  for (guint i = 0; i < n; i++) {
    gpointer item = g_list_model_get_item (G_LIST_MODEL (s_toplevel_model), i);
    list = g_list_append (list, item);
    g_object_unref (item);
  }
  return list;
}

GListModel *gtk_window_get_toplevels (void) { return G_LIST_MODEL (toplevel_model ()); }

} /* extern "C" */

/* ======================================================================
 * GtkBox (+ GtkHBox/GtkVBox/GtkButtonBox/GtkStatusbar)
 * ====================================================================== */
struct BoxData {
  Qt::Orientation orientation = Qt::Horizontal;
  int spacing = 0;
  bool homogeneous = false;
  int button_box_style = 0;
};

static BoxData *box_data (GtkWidget *w)
{
  auto *d = static_cast<BoxData *> (g_object_get_data (G_OBJECT (w), "gtkqt-box"));
  if (!d) {
    d = new BoxData;
    g_object_set_data_full (G_OBJECT (w), "gtkqt-box", d, [] (gpointer p) { delete static_cast<BoxData *> (p); });
  }
  return d;
}

static QObject *box_create (GtkWidget *w)
{
  auto *qw = new QWidget;
  Qt::Orientation o = GTK_IS_VBOX (w) ? Qt::Vertical : Qt::Horizontal;
  box_data (w)->orientation = o;
  auto *l = new QBoxLayout (o == Qt::Horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom, qw);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);
  return qw;
}

Qt::Alignment gtkqt_child_alignment (GtkWidget *child, Qt::Orientation box_orientation, gboolean fill)
{
  Qt::Alignment a;
  GtkAlign h = child->priv->halign, v = child->priv->valign;
  auto hflag = [] (GtkAlign al) -> Qt::Alignment {
    switch (al) {
    case GTK_ALIGN_START: return Qt::AlignLeft;
    case GTK_ALIGN_END: return Qt::AlignRight;
    case GTK_ALIGN_CENTER: return Qt::AlignHCenter;
    default: return Qt::Alignment ();
    }
  };
  auto vflag = [] (GtkAlign al) -> Qt::Alignment {
    switch (al) {
    case GTK_ALIGN_START: return Qt::AlignTop;
    case GTK_ALIGN_END: return Qt::AlignBottom;
    case GTK_ALIGN_CENTER: case GTK_ALIGN_BASELINE: return Qt::AlignVCenter;
    default: return Qt::Alignment ();
    }
  };
  a |= hflag (h);
  a |= vflag (v);
  if (!fill) {
    if (box_orientation == Qt::Horizontal && !(a & Qt::AlignHorizontal_Mask)) a |= Qt::AlignHCenter;
    if (box_orientation == Qt::Vertical && !(a & Qt::AlignVertical_Mask)) a |= Qt::AlignVCenter;
  }
  return a;
}

int gtkqt_child_stretch (GtkWidget *child, Qt::Orientation orientation, gboolean expand)
{
  if (expand) return 1;
  bool e = orientation == Qt::Horizontal ? child->priv->hexpand : child->priv->vexpand;
  return e ? 1 : 0;
}

static bool child_wants_space (GtkWidget *child, Qt::Orientation o)
{
  if (child->priv->pack_expand) return true;
  if (o == Qt::Horizontal ? child->priv->hexpand : child->priv->vexpand) return true;
  QWidget *cw = gtkqt_qw (child);
  if (!cw || cw->isHidden ()) return false;
  QSizePolicy sp = cw->sizePolicy ();
  if (o == Qt::Horizontal ? (sp.horizontalPolicy () & QSizePolicy::ExpandFlag) : (sp.verticalPolicy () & QSizePolicy::ExpandFlag))
    return true;
  if (cw->layout ()) return cw->layout ()->expandingDirections () & (o == Qt::Horizontal ? Qt::Horizontal : Qt::Vertical);
  return false;
}

static void box_rebuild (GtkWidget *box)
{
  QBoxLayout *l = box_layout (box);
  if (!l) return;
  BoxData *d = box_data (box);
  while (QLayoutItem *it = l->takeAt (0)) {
    if (!it->widget ()) delete it;
    else delete it;   /* QWidgetItem: the widget itself stays */
  }
  QList<GtkWidget *> start, end;
  for (GList *li = box->priv->children; li; li = li->next) {
    auto *c = (GtkWidget *) li->data;
    if (c->priv->pack_type == GTK_PACK_END) end.prepend (c); else start.append (c);
  }
  bool any_expand = d->homogeneous;
  for (GtkWidget *c : start + end) if (child_wants_space (c, d->orientation)) any_expand = true;
  bool first = true;
  auto add = [&] (GtkWidget *c) {
    QWidget *cw = gtkqt_qw (c);
    if (!cw) return;
    if (!first && d->spacing) l->addSpacing (d->spacing);
    first = false;
    if (c->priv->pack_padding) l->addSpacing (c->priv->pack_padding);
    int stretch = d->homogeneous ? 1 : gtkqt_child_stretch (c, d->orientation, c->priv->pack_expand);
    l->addWidget (cw, stretch, gtkqt_child_alignment (c, d->orientation, c->priv->pack_fill));
    if (c->priv->pack_padding) l->addSpacing (c->priv->pack_padding);
  };
  for (GtkWidget *c : start) add (c);
  if (!any_expand) l->addStretch (1);
  for (GtkWidget *c : end) add (c);
  if (d->button_box_style == GTK_BUTTONBOX_END && !any_expand) {
    /* move the stretch to the front */
    QLayoutItem *s = l->takeAt (l->count () - 1 - end.size ());
    if (s) l->insertItem (0, s);
  }
}

void gtkqt_box_relayout (GtkWidget *w)
{
  if (GTK_IS_BOX (w)) box_rebuild (w);
}

static void box_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *cw = gtkqt_qw (child);
  QWidget *content = gtkqt_content (GTK_WIDGET (c));
  if (cw && content) cw->setParent (content);
  box_rebuild (GTK_WIDGET (c));
}

static void box_unplace (GtkContainer *c, GtkWidget *child)
{
  if (QWidget *cw = gtkqt_qw (child)) {
    if (QBoxLayout *l = box_layout (GTK_WIDGET (c))) l->removeWidget (cw);
    cw->hide ();
    cw->setParent (nullptr);
  }
}

static void box_remove (GtkContainer *c, GtkWidget *child)
{
  gtkqt_container_release (c, child);
  if (!GTK_WIDGET (c)->priv->destroyed) box_rebuild (GTK_WIDGET (c));
}

static void box_add (GtkContainer *c, GtkWidget *child)
{
  /* gtk_container_add on a box == pack_start (expand=TRUE, fill=TRUE) in GTK 2/3 */
  child->priv->pack_expand = gtkqt_api < 4;
  child->priv->pack_fill = TRUE;
  child->priv->pack_padding = 0;
  child->priv->pack_type = GTK_PACK_START;
  gtkqt_container_adopt (c, child);
}

G_DEFINE_TYPE (GtkBox, gtk_box, GTK_TYPE_CONTAINER)
G_DEFINE_TYPE (GtkHBox, gtk_hbox, GTK_TYPE_BOX)
G_DEFINE_TYPE (GtkVBox, gtk_vbox, GTK_TYPE_BOX)
G_DEFINE_TYPE (GtkButtonBox, gtk_button_box, GTK_TYPE_BOX)

static void gtk_box_init (GtkBox *) {}
static void gtk_box_class_init (GtkBoxClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), box_create);
  GTK_CONTAINER_CLASS (klass)->add = box_add;
  GTK_CONTAINER_CLASS (klass)->remove = box_remove;
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), box_place, box_unplace);
  gtkqt_prop (klass, g_param_spec_enum ("orientation", nullptr, nullptr, gtk_orientation_get_type (), GTK_ORIENTATION_HORIZONTAL, RW),
              [] (GObject *o, const GValue *v) { gtk_orientable_set_orientation ((GtkOrientable *) o, (GtkOrientation) g_value_get_enum (v)); },
              [] (GObject *o, GValue *v) { g_value_set_enum (v, gtk_orientable_get_orientation ((GtkOrientable *) o)); });
  gtkqt_prop (klass, g_param_spec_int ("spacing", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_box_set_spacing (GTK_BOX (o), g_value_get_int (v)); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, gtk_box_get_spacing (GTK_BOX (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("homogeneous", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_box_set_homogeneous (GTK_BOX (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_box_get_homogeneous (GTK_BOX (o))); });
}
static void gtk_hbox_init (GtkHBox *) {}
static void gtk_hbox_class_init (GtkHBoxClass *) {}
static void gtk_vbox_init (GtkVBox *) {}
static void gtk_vbox_class_init (GtkVBoxClass *) {}
static void gtk_button_box_init (GtkButtonBox *b) { box_data (GTK_WIDGET (b))->button_box_style = GTK_BUTTONBOX_END; }
static void gtk_button_box_class_init (GtkButtonBoxClass *) {}

extern "C" {

GtkWidget *gtk_box_new (GtkOrientation orientation, gint spacing)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_BOX, nullptr);
  gtk_orientable_set_orientation ((GtkOrientable *) w, orientation);
  gtk_box_set_spacing (GTK_BOX (w), spacing);
  return w;
}

GtkWidget *gtk_hbox_new (gboolean homogeneous, gint spacing)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_HBOX, nullptr);
  gtk_box_set_homogeneous (GTK_BOX (w), homogeneous);
  gtk_box_set_spacing (GTK_BOX (w), spacing);
  return w;
}

GtkWidget *gtk_vbox_new (gboolean homogeneous, gint spacing)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_VBOX, nullptr);
  gtk_box_set_homogeneous (GTK_BOX (w), homogeneous);
  gtk_box_set_spacing (GTK_BOX (w), spacing);
  return w;
}

static void box_pack (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding, GtkPackType type)
{
  g_return_if_fail (GTK_IS_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (child));
  if (child->parent) {
    g_warning ("gtk_box_pack: widget already has a parent");
    return;
  }
  child->priv->pack_expand = expand;
  child->priv->pack_fill = fill;
  child->priv->pack_padding = padding;
  child->priv->pack_type = type;
  gtkqt_container_adopt (GTK_CONTAINER (box), child);
}

void gtk_box_pack_start (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding)
{ box_pack (box, child, expand, fill, padding, GTK_PACK_START); }
void gtk_box_pack_end (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding)
{ box_pack (box, child, expand, fill, padding, GTK_PACK_END); }
void gtk_box_pack_start_defaults (GtkBox *box, GtkWidget *widget) { gtk_box_pack_start (box, widget, TRUE, TRUE, 0); }
void gtk_box_pack_end_defaults (GtkBox *box, GtkWidget *widget) { gtk_box_pack_end (box, widget, TRUE, TRUE, 0); }
void gtk_box_append (GtkBox *box, GtkWidget *child) { box_pack (box, child, FALSE, TRUE, 0, GTK_PACK_START); }

void gtk_box_prepend (GtkBox *box, GtkWidget *child)
{
  box_pack (box, child, FALSE, TRUE, 0, GTK_PACK_START);
  gtk_box_reorder_child (box, child, 0);
}

void gtk_box_remove (GtkBox *box, GtkWidget *child) { gtk_container_remove (GTK_CONTAINER (box), child); }

void gtk_box_insert_child_after (GtkBox *box, GtkWidget *child, GtkWidget *sibling)
{
  if (!child->parent) box_pack (box, child, FALSE, TRUE, 0, GTK_PACK_START);
  int pos = sibling ? g_list_index (GTK_WIDGET (box)->priv->children, sibling) + 1 : 0;
  gtk_box_reorder_child (box, child, pos);
}

void gtk_box_reorder_child (GtkBox *box, GtkWidget *child, gint position)
{
  GtkWidgetPrivate *p = GTK_WIDGET (box)->priv;
  if (!g_list_find (p->children, child)) return;
  p->children = g_list_remove (p->children, child);
  p->children = g_list_insert (p->children, child, position);
  box_rebuild (GTK_WIDGET (box));
}

void gtk_box_set_homogeneous (GtkBox *box, gboolean homogeneous)
{
  box_data (GTK_WIDGET (box))->homogeneous = homogeneous;
  box_rebuild (GTK_WIDGET (box));
}

gboolean gtk_box_get_homogeneous (GtkBox *box) { return box_data (GTK_WIDGET (box))->homogeneous; }

void gtk_box_set_spacing (GtkBox *box, gint spacing)
{
  box_data (GTK_WIDGET (box))->spacing = spacing;
  box_rebuild (GTK_WIDGET (box));
}

gint gtk_box_get_spacing (GtkBox *box) { return box_data (GTK_WIDGET (box))->spacing; }

void gtk_box_set_child_packing (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding, GtkPackType pack_type)
{
  child->priv->pack_expand = expand;
  child->priv->pack_fill = fill;
  child->priv->pack_padding = padding;
  child->priv->pack_type = pack_type;
  box_rebuild (GTK_WIDGET (box));
}

void gtk_box_query_child_packing (GtkBox *, GtkWidget *child, gboolean *expand, gboolean *fill, guint *padding, GtkPackType *pack_type)
{
  if (expand) *expand = child->priv->pack_expand;
  if (fill) *fill = child->priv->pack_fill;
  if (padding) *padding = child->priv->pack_padding;
  if (pack_type) *pack_type = child->priv->pack_type;
}

void gtk_orientable_set_orientation (GtkOrientable *orientable, GtkOrientation orientation)
{
  auto *w = (GtkWidget *) orientable;
  Qt::Orientation o = orientation == GTK_ORIENTATION_HORIZONTAL ? Qt::Horizontal : Qt::Vertical;
  if (GTK_IS_BOX (w)) {
    box_data (w)->orientation = o;
    if (QBoxLayout *l = box_layout (w))
      l->setDirection (o == Qt::Horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    box_rebuild (w);
  } else if (auto *s = qobject_cast<QSplitter *> (gtkqt_obj (w))) {
    s->setOrientation (o);
  } else if (auto *sl = gtkqt_inner<QAbstractSlider> (w)) {
    sl->setOrientation (o);
  } else if (auto *f = qobject_cast<QFrame *> (gtkqt_obj (w))) {
    f->setFrameShape (o == Qt::Horizontal ? QFrame::HLine : QFrame::VLine);
  } else if (auto *pb = qobject_cast<QProgressBar *> (gtkqt_obj (w))) {
    pb->setOrientation (o);
  }
  g_object_set_data (G_OBJECT (w), "gtkqt-orientation", GINT_TO_POINTER (orientation));
}

GtkOrientation gtk_orientable_get_orientation (GtkOrientable *orientable)
{
  auto *w = (GtkWidget *) orientable;
  if (GTK_IS_BOX (w)) return box_data (w)->orientation == Qt::Horizontal ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
  return (GtkOrientation) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "gtkqt-orientation"));
}

GtkWidget *gtk_button_box_new (GtkOrientation orientation)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_BUTTON_BOX, nullptr);
  gtk_orientable_set_orientation ((GtkOrientable *) w, orientation);
  gtk_box_set_spacing (GTK_BOX (w), 6);
  return w;
}

GtkWidget *gtk_hbutton_box_new (void) { return gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL); }
GtkWidget *gtk_vbutton_box_new (void) { return gtk_button_box_new (GTK_ORIENTATION_VERTICAL); }

void gtk_button_box_set_layout (GtkButtonBox *widget, GtkButtonBoxStyle layout_style)
{
  box_data (GTK_WIDGET (widget))->button_box_style = layout_style;
  box_data (GTK_WIDGET (widget))->homogeneous = layout_style == GTK_BUTTONBOX_EXPAND || layout_style == GTK_BUTTONBOX_SPREAD;
  box_rebuild (GTK_WIDGET (widget));
}

} /* extern "C" */

/* ======================================================================
 * GtkGrid / GtkTable
 * ====================================================================== */
static QObject *grid_create (GtkWidget *)
{
  auto *qw = new QWidget;
  auto *l = new QGridLayout (qw);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);
  return qw;
}

static QGridLayout *grid_layout (GtkWidget *w)
{
  QWidget *qw = gtkqt_qw (w);
  return qw ? qobject_cast<QGridLayout *> (qw->layout ()) : nullptr;
}

static void grid_place (GtkContainer *c, GtkWidget *child)
{
  QGridLayout *l = grid_layout (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!l || !cw) return;
  GtkWidgetPrivate *p = child->priv;
  Qt::Alignment a = gtkqt_child_alignment (child, Qt::Horizontal, TRUE);
  if (GTK_IS_TABLE (c)) {
    if (!(p->xopts & GTK_FILL) && !(a & Qt::AlignHorizontal_Mask)) a |= Qt::AlignHCenter;
    if (!(p->yopts & GTK_FILL) && !(a & Qt::AlignVertical_Mask)) a |= Qt::AlignVCenter;
    if (p->xopts & GTK_EXPAND) for (int i = 0; i < p->grid_width; i++) l->setColumnStretch (p->grid_left + i, 1);
    if (p->yopts & GTK_EXPAND) for (int i = 0; i < p->grid_height; i++) l->setRowStretch (p->grid_top + i, 1);
  }
  l->addWidget (cw, p->grid_top, p->grid_left, p->grid_height, p->grid_width, a);
}

static void grid_unplace (GtkContainer *c, GtkWidget *child)
{
  if (QWidget *cw = gtkqt_qw (child)) {
    if (QGridLayout *l = grid_layout (GTK_WIDGET (c))) l->removeWidget (cw);
    cw->hide ();
    cw->setParent (nullptr);
  }
}

static void grid_add (GtkContainer *c, GtkWidget *child)
{
  /* plain gtk_container_add on a grid: next free column on row 0 */
  int col = 0;
  for (GList *l = GTK_WIDGET (c)->priv->children; l; l = l->next) {
    auto *o = (GtkWidget *) l->data;
    if (o->priv->grid_top == 0) col = qMax (col, o->priv->grid_left + o->priv->grid_width);
  }
  child->priv->grid_left = col;
  child->priv->grid_top = 0;
  child->priv->grid_width = child->priv->grid_height = 1;
  gtkqt_container_adopt (c, child);
}

G_DEFINE_TYPE (GtkGrid, gtk_grid, GTK_TYPE_CONTAINER)
G_DEFINE_TYPE (GtkTable, gtk_table, GTK_TYPE_CONTAINER)
static void gtk_grid_init (GtkGrid *) {}
static void gtk_grid_class_init (GtkGridClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), grid_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), grid_place, grid_unplace);
  GTK_CONTAINER_CLASS (klass)->add = grid_add;
  gtkqt_prop (klass, g_param_spec_uint ("row-spacing", nullptr, nullptr, 0, 32767, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_grid_set_row_spacing (GTK_GRID (o), g_value_get_uint (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_uint ("column-spacing", nullptr, nullptr, 0, 32767, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_grid_set_column_spacing (GTK_GRID (o), g_value_get_uint (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("row-homogeneous", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_grid_set_row_homogeneous (GTK_GRID (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("column-homogeneous", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_grid_set_column_homogeneous (GTK_GRID (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("orientation", nullptr, nullptr, gtk_orientation_get_type (), GTK_ORIENTATION_HORIZONTAL, RW),
              [] (GObject *, const GValue *) {}, nullptr);
}
static void gtk_table_init (GtkTable *) {}
static void gtk_table_class_init (GtkTableClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), grid_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), grid_place, grid_unplace);
  GTK_CONTAINER_CLASS (klass)->add = grid_add;
  gtkqt_prop (klass, g_param_spec_uint ("n-rows", nullptr, nullptr, 0, 65535, 1, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_uint ("n-columns", nullptr, nullptr, 0, 65535, 1, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_uint ("row-spacing", nullptr, nullptr, 0, 65535, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_table_set_row_spacings (GTK_TABLE (o), g_value_get_uint (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_uint ("column-spacing", nullptr, nullptr, 0, 65535, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_table_set_col_spacings (GTK_TABLE (o), g_value_get_uint (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("homogeneous", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_table_set_homogeneous (GTK_TABLE (o), g_value_get_boolean (v)); }, nullptr);
}

extern "C" {

GtkWidget *gtk_grid_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_GRID, nullptr); }

void gtk_grid_attach (GtkGrid *grid, GtkWidget *child, gint left, gint top, gint width, gint height)
{
  g_return_if_fail (GTK_IS_GRID (grid));
  if (child->parent) return;
  child->priv->grid_left = left;
  child->priv->grid_top = top;
  child->priv->grid_width = qMax (1, width);
  child->priv->grid_height = qMax (1, height);
  gtkqt_container_adopt (GTK_CONTAINER (grid), child);
}

void gtk_grid_attach_next_to (GtkGrid *grid, GtkWidget *child, GtkWidget *sibling, GtkPositionType side, gint width, gint height)
{
  int left = 0, top = 0;
  if (sibling) {
    GtkWidgetPrivate *s = sibling->priv;
    switch (side) {
    case GTK_POS_LEFT: left = s->grid_left - width; top = s->grid_top; break;
    case GTK_POS_RIGHT: left = s->grid_left + s->grid_width; top = s->grid_top; break;
    case GTK_POS_TOP: left = s->grid_left; top = s->grid_top - height; break;
    case GTK_POS_BOTTOM: left = s->grid_left; top = s->grid_top + s->grid_height; break;
    }
  } else {
    for (GList *l = GTK_WIDGET (grid)->priv->children; l; l = l->next) {
      auto *o = (GtkWidget *) l->data;
      if (side == GTK_POS_RIGHT) left = qMax (left, o->priv->grid_left + o->priv->grid_width);
      if (side == GTK_POS_BOTTOM) top = qMax (top, o->priv->grid_top + o->priv->grid_height);
    }
  }
  gtk_grid_attach (grid, child, qMax (0, left), qMax (0, top), width, height);
}

void gtk_grid_remove (GtkGrid *grid, GtkWidget *child) { gtk_container_remove (GTK_CONTAINER (grid), child); }

GtkWidget *gtk_grid_get_child_at (GtkGrid *grid, gint left, gint top)
{
  for (GList *l = GTK_WIDGET (grid)->priv->children; l; l = l->next) {
    auto *c = (GtkWidget *) l->data;
    GtkWidgetPrivate *p = c->priv;
    if (left >= p->grid_left && left < p->grid_left + p->grid_width && top >= p->grid_top && top < p->grid_top + p->grid_height)
      return c;
  }
  return nullptr;
}

void gtk_grid_set_row_spacing (GtkGrid *grid, guint spacing) { if (auto *l = grid_layout (GTK_WIDGET (grid))) l->setVerticalSpacing (spacing); }
void gtk_grid_set_column_spacing (GtkGrid *grid, guint spacing) { if (auto *l = grid_layout (GTK_WIDGET (grid))) l->setHorizontalSpacing (spacing); }

void gtk_grid_set_row_homogeneous (GtkGrid *grid, gboolean homogeneous)
{
  if (auto *l = grid_layout (GTK_WIDGET (grid)))
    for (int r = 0; r < qMax (1, l->rowCount ()); r++) l->setRowStretch (r, homogeneous ? 1 : 0);
}

void gtk_grid_set_column_homogeneous (GtkGrid *grid, gboolean homogeneous)
{
  g_object_set_data (G_OBJECT (grid), "gtkqt-col-homog", GINT_TO_POINTER (homogeneous));
  if (auto *l = grid_layout (GTK_WIDGET (grid)))
    for (int c = 0; c < qMax (1, l->columnCount ()); c++) l->setColumnStretch (c, homogeneous ? 1 : 0);
}

static void grid_shift (GtkGrid *grid, bool rows, gint position)
{
  QList<GtkWidget *> kids;
  for (GList *l = GTK_WIDGET (grid)->priv->children; l; l = l->next) kids << (GtkWidget *) l->data;
  for (GtkWidget *c : kids) {
    int &v = rows ? c->priv->grid_top : c->priv->grid_left;
    if (v >= position) v++;
    grid_unplace (GTK_CONTAINER (grid), c);
    grid_place (GTK_CONTAINER (grid), c);
    gtkqt_sync_visible (c);
  }
}

void gtk_grid_insert_row (GtkGrid *grid, gint position) { grid_shift (grid, true, position); }
void gtk_grid_insert_column (GtkGrid *grid, gint position) { grid_shift (grid, false, position); }

GtkWidget *gtk_table_new (guint, guint, gboolean homogeneous)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_TABLE, nullptr);
  gtk_table_set_homogeneous (GTK_TABLE (w), homogeneous);
  return w;
}

void gtk_table_resize (GtkTable *, guint, guint) {}

void gtk_table_attach (GtkTable *table, GtkWidget *child, guint left_attach, guint right_attach,
                       guint top_attach, guint bottom_attach, GtkAttachOptions xoptions,
                       GtkAttachOptions yoptions, guint, guint)
{
  g_return_if_fail (GTK_IS_TABLE (table));
  if (child->parent) return;
  GtkWidgetPrivate *p = child->priv;
  p->grid_left = left_attach;
  p->grid_top = top_attach;
  p->grid_width = qMax (1, (int) right_attach - (int) left_attach);
  p->grid_height = qMax (1, (int) bottom_attach - (int) top_attach);
  p->xopts = xoptions;
  p->yopts = yoptions;
  gtkqt_container_adopt (GTK_CONTAINER (table), child);
}

void gtk_table_attach_defaults (GtkTable *table, GtkWidget *widget, guint left_attach, guint right_attach,
                                guint top_attach, guint bottom_attach)
{
  gtk_table_attach (table, widget, left_attach, right_attach, top_attach, bottom_attach,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
}

void gtk_table_set_row_spacings (GtkTable *table, guint spacing) { if (auto *l = grid_layout (GTK_WIDGET (table))) l->setVerticalSpacing (spacing); }
void gtk_table_set_col_spacings (GtkTable *table, guint spacing) { if (auto *l = grid_layout (GTK_WIDGET (table))) l->setHorizontalSpacing (spacing); }
void gtk_table_set_row_spacing (GtkTable *table, guint, guint spacing) { gtk_table_set_row_spacings (table, spacing); }
void gtk_table_set_col_spacing (GtkTable *table, guint, guint spacing) { gtk_table_set_col_spacings (table, spacing); }

void gtk_table_set_homogeneous (GtkTable *table, gboolean homogeneous)
{
  if (auto *l = grid_layout (GTK_WIDGET (table))) {
    for (int r = 0; r < 64; r++) l->setRowStretch (r, homogeneous ? 1 : 0);
    for (int c = 0; c < 64; c++) l->setColumnStretch (c, homogeneous ? 1 : 0);
  }
}

} /* extern "C" */

/* ======================================================================
 * GtkFixed
 * ====================================================================== */
class GtkQtFixedWidget : public QWidget {
public:
  QSize sizeHint () const override
  {
    QRect r;
    for (QObject *o : children ())
      if (auto *w = qobject_cast<QWidget *> (o))
        if (!w->isHidden ()) r |= QRect (w->pos (), w->sizeHint ());
    return QSize (r.right () + 1, r.bottom () + 1).expandedTo (QSize (1, 1));
  }
  QSize minimumSizeHint () const override { return sizeHint (); }
};

static QObject *fixed_create (GtkWidget *) { return new GtkQtFixedWidget; }

static void fixed_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *pw = gtkqt_qw (GTK_WIDGET (c)), *cw = gtkqt_qw (child);
  if (!pw || !cw) return;
  cw->setParent (pw);
  cw->move (child->priv->grid_left, child->priv->grid_top);
  cw->resize (cw->sizeHint ().expandedTo (cw->minimumSize ()));
  pw->updateGeometry ();
}

G_DEFINE_TYPE (GtkFixed, gtk_fixed, GTK_TYPE_CONTAINER)
static void gtk_fixed_init (GtkFixed *) {}
static void gtk_fixed_class_init (GtkFixedClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), fixed_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), fixed_place, gtkqt_bin_unplace_simple);
}

extern "C" {
GtkWidget *gtk_fixed_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_FIXED, nullptr); }

void gtk_fixed_put (GtkFixed *fixed, GtkWidget *widget, gint x, gint y)
{
  widget->priv->grid_left = x;
  widget->priv->grid_top = y;
  gtkqt_container_adopt (GTK_CONTAINER (fixed), widget);
}

void gtk_fixed_move (GtkFixed *fixed, GtkWidget *widget, gint x, gint y)
{
  widget->priv->grid_left = x;
  widget->priv->grid_top = y;
  if (QWidget *cw = gtkqt_qw (widget)) cw->move (x, y);
  if (QWidget *pw = gtkqt_qw (GTK_WIDGET (fixed))) pw->updateGeometry ();
}

void gtk_fixed_remove (GtkFixed *fixed, GtkWidget *widget) { gtk_container_remove (GTK_CONTAINER (fixed), widget); }
}

/* ======================================================================
 * GtkFrame, GtkAlignment, GtkEventBox, GtkViewport
 * ====================================================================== */
static QObject *frame_create (GtkWidget *)
{
  auto *g = new QGroupBox;
  auto *l = new QVBoxLayout (g);
  l->setContentsMargins (6, 6, 6, 6);
  return g;
}

G_DEFINE_TYPE (GtkFrame, gtk_frame, GTK_TYPE_BIN)
static void gtk_frame_init (GtkFrame *) {}
static void gtk_frame_class_init (GtkFrameClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), frame_create);
  gtkqt_prop (klass, g_param_spec_string ("label", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_frame_set_label (GTK_FRAME (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, gtk_frame_get_label (GTK_FRAME (o))); });
  gtkqt_prop (klass, g_param_spec_enum ("shadow-type", nullptr, nullptr, gtk_shadow_type_get_type (), GTK_SHADOW_ETCHED_IN, RW),
              [] (GObject *o, const GValue *v) { gtk_frame_set_shadow_type (GTK_FRAME (o), (GtkShadowType) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_float ("label-xalign", nullptr, nullptr, 0, 1, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_frame_set_label_align (GTK_FRAME (o), g_value_get_float (v), 0.5); }, nullptr);
}

static QObject *alignment_create (GtkWidget *)
{
  auto *qw = new QWidget;
  auto *l = new QGridLayout (qw);
  l->setContentsMargins (0, 0, 0, 0);
  return qw;
}

struct AlignData { float xalign = 0.5f, yalign = 0.5f, xscale = 1, yscale = 1; };

static void alignment_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *pw = gtkqt_qw (GTK_WIDGET (c)), *cw = gtkqt_qw (child);
  auto *d = static_cast<AlignData *> (g_object_get_data (G_OBJECT (c), "gtkqt-align"));
  if (!pw || !cw) return;
  Qt::Alignment a;
  if (d && d->xscale < 1) a |= d->xalign < 0.25 ? Qt::AlignLeft : d->xalign > 0.75 ? Qt::AlignRight : Qt::AlignHCenter;
  if (d && d->yscale < 1) a |= d->yalign < 0.25 ? Qt::AlignTop : d->yalign > 0.75 ? Qt::AlignBottom : Qt::AlignVCenter;
  static_cast<QGridLayout *> (pw->layout ())->addWidget (cw, 0, 0, a);
}

G_DEFINE_TYPE (GtkAlignment, gtk_alignment, GTK_TYPE_BIN)
static void gtk_alignment_init (GtkAlignment *) {}
static void gtk_alignment_class_init (GtkAlignmentClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), alignment_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), alignment_place, gtkqt_bin_unplace_simple);
}

G_DEFINE_TYPE (GtkEventBox, gtk_event_box, GTK_TYPE_BIN)
static void gtk_event_box_init (GtkEventBox *) {}
static void gtk_event_box_class_init (GtkEventBoxClass *) {}

G_DEFINE_TYPE (GtkViewport, gtk_viewport, GTK_TYPE_BIN)
static void gtk_viewport_init (GtkViewport *) {}
static void gtk_viewport_class_init (GtkViewportClass *) {}

extern "C" {

GtkWidget *gtk_frame_new (const gchar *label)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_FRAME, nullptr);
  gtk_frame_set_label (GTK_FRAME (w), label);
  return w;
}

void gtk_frame_set_label (GtkFrame *frame, const gchar *label)
{
  if (auto *g = gtkqt_q<QGroupBox> (GTK_WIDGET (frame))) g->setTitle (gtkqt_str (label));
}

const gchar *gtk_frame_get_label (GtkFrame *frame)
{
  auto *g = gtkqt_q<QGroupBox> (GTK_WIDGET (frame));
  return g && !g->title ().isEmpty () ? gtkqt_ret (GTK_WIDGET (frame), g->title ()) : nullptr;
}

void gtk_frame_set_shadow_type (GtkFrame *frame, GtkShadowType type)
{
  if (auto *g = gtkqt_q<QGroupBox> (GTK_WIDGET (frame))) g->setFlat (type == GTK_SHADOW_NONE);
}

void gtk_frame_set_label_align (GtkFrame *frame, gfloat xalign, gfloat)
{
  if (auto *g = gtkqt_q<QGroupBox> (GTK_WIDGET (frame)))
    g->setAlignment (xalign < 0.25 ? Qt::AlignLeft : xalign > 0.75 ? Qt::AlignRight : Qt::AlignHCenter);
}

void gtk_frame_set_child (GtkFrame *frame, GtkWidget *child)
{
  if (GtkWidget *old = GTK_BIN (frame)->child) gtk_container_remove (GTK_CONTAINER (frame), old);
  if (child) gtk_container_add (GTK_CONTAINER (frame), child);
}

GtkWidget *gtk_frame_get_child (GtkFrame *frame) { return GTK_BIN (frame)->child; }

GtkWidget *gtk_alignment_new (gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_ALIGNMENT, nullptr);
  gtk_alignment_set (GTK_ALIGNMENT (w), xalign, yalign, xscale, yscale);
  return w;
}

void gtk_alignment_set (GtkAlignment *alignment, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale)
{
  auto *d = new AlignData { xalign, yalign, xscale, yscale };
  g_object_set_data_full (G_OBJECT (alignment), "gtkqt-align", d, [] (gpointer p) { delete static_cast<AlignData *> (p); });
  if (GtkWidget *child = GTK_BIN (alignment)->child) {
    gtkqt_bin_unplace_simple (GTK_CONTAINER (alignment), child);
    alignment_place (GTK_CONTAINER (alignment), child);
    gtkqt_sync_visible (child);
  }
}

void gtk_alignment_set_padding (GtkAlignment *alignment, guint top, guint bottom, guint left, guint right)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (alignment))) qw->layout ()->setContentsMargins (left, top, right, bottom);
}

GtkWidget *gtk_event_box_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_EVENT_BOX, nullptr); }
GtkWidget *gtk_viewport_new (GtkAdjustment *, GtkAdjustment *) { return (GtkWidget *) g_object_new (GTK_TYPE_VIEWPORT, nullptr); }

} /* extern "C" */

/* ======================================================================
 * GtkPaned
 * ====================================================================== */
static QObject *paned_create (GtkWidget *w)
{
  auto *s = new QSplitter (Qt::Horizontal);
  s->setChildrenCollapsible (false);
  (void) w;
  return s;
}

static void paned_place (GtkContainer *c, GtkWidget *child)
{
  auto *s = gtkqt_q<QSplitter> (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!s || !cw) return;
  int slot = child->priv->grid_left;   /* 0 = first pane, 1 = second */
  s->insertWidget (slot == 0 ? 0 : s->count (), cw);
  s->setStretchFactor (s->indexOf (cw), child->priv->pack_expand ? 1 : 0);
}

G_DEFINE_TYPE (GtkPaned, gtk_paned, GTK_TYPE_CONTAINER)
static void gtk_paned_init (GtkPaned *) {}
static void gtk_paned_class_init (GtkPanedClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), paned_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), paned_place, gtkqt_bin_unplace_simple);
  gtkqt_prop (klass, g_param_spec_int ("position", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_paned_set_position (GTK_PANED (o), g_value_get_int (v)); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, gtk_paned_get_position (GTK_PANED (o))); });
  gtkqt_prop (klass, g_param_spec_enum ("orientation", nullptr, nullptr, gtk_orientation_get_type (), GTK_ORIENTATION_HORIZONTAL, RW),
              [] (GObject *o, const GValue *v) { gtk_orientable_set_orientation ((GtkOrientable *) o, (GtkOrientation) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("start-child", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) { gtk_paned_set_start_child (GTK_PANED (o), (GtkWidget *) g_value_get_object (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("end-child", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) { gtk_paned_set_end_child (GTK_PANED (o), (GtkWidget *) g_value_get_object (v)); }, nullptr);
}

static GtkWidget *paned_child (GtkPaned *paned, int slot)
{
  for (GList *l = GTK_WIDGET (paned)->priv->children; l; l = l->next)
    if (((GtkWidget *) l->data)->priv->grid_left == slot) return (GtkWidget *) l->data;
  return nullptr;
}

static void paned_set (GtkPaned *paned, GtkWidget *child, int slot, gboolean resize)
{
  if (GtkWidget *old = paned_child (paned, slot)) gtk_container_remove (GTK_CONTAINER (paned), old);
  if (!child) return;
  child->priv->grid_left = slot;
  child->priv->pack_expand = resize;
  gtkqt_container_adopt (GTK_CONTAINER (paned), child);
}

extern "C" {
GtkWidget *gtk_paned_new (GtkOrientation orientation)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_PANED, nullptr);
  gtk_orientable_set_orientation ((GtkOrientable *) w, orientation);
  return w;
}
GtkWidget *gtk_hpaned_new (void) { return gtk_paned_new (GTK_ORIENTATION_HORIZONTAL); }
GtkWidget *gtk_vpaned_new (void) { return gtk_paned_new (GTK_ORIENTATION_VERTICAL); }
void gtk_paned_add1 (GtkPaned *paned, GtkWidget *child) { paned_set (paned, child, 0, FALSE); }
void gtk_paned_add2 (GtkPaned *paned, GtkWidget *child) { paned_set (paned, child, 1, TRUE); }
void gtk_paned_pack1 (GtkPaned *paned, GtkWidget *child, gboolean resize, gboolean) { paned_set (paned, child, 0, resize); }
void gtk_paned_pack2 (GtkPaned *paned, GtkWidget *child, gboolean resize, gboolean) { paned_set (paned, child, 1, resize); }
void gtk_paned_set_start_child (GtkPaned *paned, GtkWidget *child) { paned_set (paned, child, 0, TRUE); }
void gtk_paned_set_end_child (GtkPaned *paned, GtkWidget *child) { paned_set (paned, child, 1, TRUE); }
GtkWidget *gtk_paned_get_start_child (GtkPaned *paned) { return paned_child (paned, 0); }
GtkWidget *gtk_paned_get_end_child (GtkPaned *paned) { return paned_child (paned, 1); }

void gtk_paned_set_position (GtkPaned *paned, gint position)
{
  auto *s = gtkqt_q<QSplitter> (GTK_WIDGET (paned));
  if (!s) return;
  int total = s->orientation () == Qt::Horizontal ? qMax (s->width (), s->sizeHint ().width ())
                                                  : qMax (s->height (), s->sizeHint ().height ());
  s->setSizes ({ position, qMax (1, total - position) });
  g_object_set_data (G_OBJECT (paned), "gtkqt-pos", GINT_TO_POINTER (position));
}

gint gtk_paned_get_position (GtkPaned *paned)
{
  auto *s = gtkqt_q<QSplitter> (GTK_WIDGET (paned));
  if (!s || s->sizes ().isEmpty () || !s->isVisible ()) return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (paned), "gtkqt-pos"));
  return s->sizes ().value (0);
}
}

/* ======================================================================
 * GtkNotebook
 * ====================================================================== */
static QObject *notebook_create (GtkWidget *w)
{
  auto *t = new QTabWidget;
  gtkqt_connect (w, t, &QTabWidget::currentChanged, [] (GtkWidget *nb, int index) {
    if (index < 0) return;
    GtkWidget *page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (nb), index);
    g_signal_emit_by_name (nb, "switch-page", page, (guint) index);
    g_object_notify (G_OBJECT (nb), "page");
  });
  return t;
}

static void notebook_place (GtkContainer *c, GtkWidget *child)
{
  auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!t || !cw) return;
  int pos = child->priv->grid_left;
  QString label = child->priv->tab_label;
  if (label.isEmpty ()) label = QStringLiteral ("Page %1").arg (t->count () + 1);
  QSignalBlocker block (t);
  int idx = t->insertTab (pos < 0 || pos > t->count () ? t->count () : pos, cw, label);
  if (auto *tab_widget = (GtkWidget *) g_object_get_data (G_OBJECT (child), "gtkqt-tab-widget")) {
    if (!GTK_IS_LABEL (tab_widget)) {
      t->setTabText (idx, QString ());
      if (QWidget *tw = gtkqt_qw (tab_widget)) {
        t->tabBar ()->setTabButton (idx, QTabBar::LeftSide, tw);
        if (tab_widget->priv->visible) tw->show ();
      }
    }
  }
  if (t->count () == 1) {
    block.unblock ();
    Q_EMIT t->currentChanged (0);
  }
}

static void notebook_unplace (GtkContainer *c, GtkWidget *child)
{
  auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (t && cw) {
    int i = t->indexOf (cw);
    if (i >= 0) t->removeTab (i);
    cw->hide ();
    cw->setParent (nullptr);
  }
  if (auto *tab = (GtkWidget *) g_object_get_data (G_OBJECT (child), "gtkqt-tab-widget")) {
    g_object_set_data (G_OBJECT (child), "gtkqt-tab-widget", nullptr);
    gtk_widget_destroy (tab);
    g_object_unref (tab);
  }
}

static void notebook_add (GtkContainer *c, GtkWidget *child)
{
  child->priv->grid_left = -1;
  gtkqt_container_adopt (c, child);
}

G_DEFINE_TYPE (GtkNotebook, gtk_notebook, GTK_TYPE_CONTAINER)
static void gtk_notebook_init (GtkNotebook *n) { n->tab_pos = GTK_POS_TOP; n->show_tabs = 1; n->show_border = 1; }
static void gtk_notebook_class_init (GtkNotebookClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), notebook_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), notebook_place, notebook_unplace);
  GTK_CONTAINER_CLASS (klass)->add = notebook_add;
  g_signal_new ("switch-page", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr,
                G_TYPE_NONE, 2, GTK_TYPE_WIDGET, G_TYPE_UINT);
  gtkqt_prop (klass, g_param_spec_int ("page", nullptr, nullptr, -1, G_MAXINT, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_notebook_set_current_page (GTK_NOTEBOOK (o), g_value_get_int (v)); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, gtk_notebook_get_current_page (GTK_NOTEBOOK (o))); });
  gtkqt_prop (klass, g_param_spec_enum ("tab-pos", nullptr, nullptr, gtk_position_type_get_type (), GTK_POS_TOP, RW),
              [] (GObject *o, const GValue *v) { gtk_notebook_set_tab_pos (GTK_NOTEBOOK (o), (GtkPositionType) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("show-tabs", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_notebook_set_show_tabs (GTK_NOTEBOOK (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("scrollable", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_notebook_set_scrollable (GTK_NOTEBOOK (o), g_value_get_boolean (v)); }, nullptr);
}

extern "C" {
GtkWidget *gtk_notebook_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_NOTEBOOK, nullptr); }

gint gtk_notebook_insert_page (GtkNotebook *notebook, GtkWidget *child, GtkWidget *tab_label, gint position)
{
  g_return_val_if_fail (!child->parent, -1);
  child->priv->grid_left = position;
  if (tab_label) {
    g_object_ref_sink (tab_label);
    g_object_set_data (G_OBJECT (child), "gtkqt-tab-widget", tab_label);
    if (GTK_IS_LABEL (tab_label)) child->priv->tab_label = gtkqt_str (gtk_label_get_text (GTK_LABEL (tab_label)));
  }
  gtkqt_container_adopt (GTK_CONTAINER (notebook), child);
  auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook));
  return t ? t->indexOf (gtkqt_qw (child)) : -1;
}

gint gtk_notebook_append_page (GtkNotebook *notebook, GtkWidget *child, GtkWidget *tab_label)
{ return gtk_notebook_insert_page (notebook, child, tab_label, -1); }
gint gtk_notebook_prepend_page (GtkNotebook *notebook, GtkWidget *child, GtkWidget *tab_label)
{ return gtk_notebook_insert_page (notebook, child, tab_label, 0); }

void gtk_notebook_remove_page (GtkNotebook *notebook, gint page_num)
{
  if (GtkWidget *c = gtk_notebook_get_nth_page (notebook, page_num < 0 ? gtk_notebook_get_n_pages (notebook) - 1 : page_num))
    gtk_container_remove (GTK_CONTAINER (notebook), c);
}

gint gtk_notebook_get_current_page (GtkNotebook *notebook)
{
  auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook));
  return t ? t->currentIndex () : -1;
}

void gtk_notebook_set_current_page (GtkNotebook *notebook, gint page_num)
{
  if (auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook))) t->setCurrentIndex (page_num < 0 ? t->count () - 1 : page_num);
}

GtkWidget *gtk_notebook_get_nth_page (GtkNotebook *notebook, gint page_num)
{
  auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook));
  return t && t->widget (page_num) ? gtkqt_from_qobject (t->widget (page_num)) : nullptr;
}

gint gtk_notebook_get_n_pages (GtkNotebook *notebook)
{
  auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook));
  return t ? t->count () : 0;
}

gint gtk_notebook_page_num (GtkNotebook *notebook, GtkWidget *child)
{
  auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook));
  return t ? t->indexOf (gtkqt_qw (child)) : -1;
}

void gtk_notebook_next_page (GtkNotebook *notebook)
{
  int i = gtk_notebook_get_current_page (notebook);
  if (i + 1 < gtk_notebook_get_n_pages (notebook)) gtk_notebook_set_current_page (notebook, i + 1);
}

void gtk_notebook_prev_page (GtkNotebook *notebook)
{
  int i = gtk_notebook_get_current_page (notebook);
  if (i > 0) gtk_notebook_set_current_page (notebook, i - 1);
}

void gtk_notebook_set_tab_pos (GtkNotebook *notebook, GtkPositionType pos)
{
  notebook->tab_pos = pos;
  static const QTabWidget::TabPosition map[] = { QTabWidget::West, QTabWidget::East, QTabWidget::North, QTabWidget::South };
  if (auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook))) t->setTabPosition (map[pos & 3]);
}

void gtk_notebook_set_show_tabs (GtkNotebook *notebook, gboolean show_tabs)
{
  notebook->show_tabs = show_tabs;
  if (auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook))) t->tabBar ()->setVisible (show_tabs);
}

void gtk_notebook_set_show_border (GtkNotebook *notebook, gboolean show_border)
{
  notebook->show_border = show_border;
  if (auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook))) t->setDocumentMode (!show_border);
}

void gtk_notebook_set_scrollable (GtkNotebook *notebook, gboolean scrollable)
{
  if (auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook))) t->setUsesScrollButtons (scrollable);
}

void gtk_notebook_set_tab_label_text (GtkNotebook *notebook, GtkWidget *child, const gchar *tab_text)
{
  child->priv->tab_label = gtkqt_str (tab_text);
  if (auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook))) {
    int i = t->indexOf (gtkqt_qw (child));
    if (i >= 0) t->setTabText (i, child->priv->tab_label);
  }
}

const gchar *gtk_notebook_get_tab_label_text (GtkNotebook *, GtkWidget *child)
{
  return gtkqt_ret (child, child->priv->tab_label);
}

void gtk_notebook_set_tab_reorderable (GtkNotebook *notebook, GtkWidget *, gboolean reorderable)
{
  if (auto *t = gtkqt_q<QTabWidget> (GTK_WIDGET (notebook))) t->setMovable (reorderable);
}
}

/* ======================================================================
 * GtkStack / GtkStackSwitcher
 * ====================================================================== */
static QObject *stack_create (GtkWidget *w)
{
  auto *s = new QStackedWidget;
  gtkqt_connect (w, s, &QStackedWidget::currentChanged, [] (GtkWidget *st, int) {
    g_object_notify (G_OBJECT (st), "visible-child");
    g_object_notify (G_OBJECT (st), "visible-child-name");
  });
  return s;
}

static void stack_place (GtkContainer *c, GtkWidget *child)
{
  auto *s = gtkqt_q<QStackedWidget> (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!s || !cw) return;
  s->addWidget (cw);
  g_signal_emit_by_name (c, "gtkqt-pages-changed");
}

static void stack_unplace (GtkContainer *c, GtkWidget *child)
{
  auto *s = gtkqt_q<QStackedWidget> (GTK_WIDGET (c));
  if (QWidget *cw = gtkqt_qw (child)) {
    if (s) s->removeWidget (cw);
    cw->hide ();
    cw->setParent (nullptr);
  }
  g_signal_emit_by_name (c, "gtkqt-pages-changed");
}

G_DEFINE_TYPE (GtkStack, gtk_stack, GTK_TYPE_CONTAINER)
static void gtk_stack_init (GtkStack *) {}
static void gtk_stack_class_init (GtkStackClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), stack_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), stack_place, stack_unplace);
  g_signal_new ("gtkqt-pages-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_object ("visible-child", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) { gtk_stack_set_visible_child (GTK_STACK (o), (GtkWidget *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_stack_get_visible_child (GTK_STACK (o))); });
  gtkqt_prop (klass, g_param_spec_string ("visible-child-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_stack_set_visible_child_name (GTK_STACK (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, gtk_stack_get_visible_child_name (GTK_STACK (o))); });
  gtkqt_prop (klass, g_param_spec_enum ("transition-type", nullptr, nullptr, gtk_stack_transition_type_get_type (), 0, RW),
              [] (GObject *, const GValue *) {}, nullptr);
}

static void switcher_rebuild (GtkWidget *sw);

G_DEFINE_TYPE (GtkStackSwitcher, gtk_stack_switcher, GTK_TYPE_BOX)
static void gtk_stack_switcher_init (GtkStackSwitcher *) {}
static QObject *switcher_create (GtkWidget *w)
{
  auto *qw = new QWidget;
  auto *l = new QHBoxLayout (qw);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);
  auto *content = new QWidget (qw);   /* GTK children (unusual for a switcher) live here */
  new QBoxLayout (QBoxLayout::LeftToRight, content);
  content->hide ();
  w->priv->content = content;
  return qw;
}

static void gtk_stack_switcher_class_init (GtkStackSwitcherClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), switcher_create);
  gtkqt_prop (klass, g_param_spec_object ("stack", nullptr, nullptr, GTK_TYPE_STACK, RW),
              [] (GObject *o, const GValue *v) { gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (o), (GtkStack *) g_value_get_object (v)); },
              nullptr);
}

static void switcher_rebuild (GtkWidget *sw)
{
  QWidget *qw = gtkqt_qw (sw);
  auto *stack = (GtkStack *) g_object_get_data (G_OBJECT (sw), "gtkqt-stack");
  if (!qw || !qw->layout ()) return;
  auto *l = qobject_cast<QBoxLayout *> (qw->layout ());
  /* buttons are internal: owned by the switcher QWidget, not GTK children */
  for (QAbstractButton *b : qw->findChildren<QAbstractButton *> (QString (), Qt::FindDirectChildrenOnly)) delete b;
  while (QLayoutItem *it = l->takeAt (0)) delete it;
  if (!stack) return;
  auto *group = qw->findChild<QButtonGroup *> ();
  if (!group) { group = new QButtonGroup (qw); group->setExclusive (true); }
  l->addStretch (1);
  for (GList *c = GTK_WIDGET (stack)->priv->children; c; c = c->next) {
    auto *page = (GtkWidget *) c->data;
    QString title = page->priv->tab_label.isEmpty () ? page->priv->stack_name : page->priv->tab_label;
    auto *b = new QPushButton (title, qw);
    b->setCheckable (true);
    group->addButton (b);
    l->addWidget (b);
    b->setChecked (gtk_stack_get_visible_child (stack) == page);
    GtkStack *st = stack;
    QObject::connect (b, &QPushButton::clicked, b, [st, page] { gtk_stack_set_visible_child (st, page); });
  }
  l->addStretch (1);
}

extern "C" {
GtkWidget *gtk_stack_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_STACK, nullptr); }

void gtk_stack_add_titled (GtkStack *stack, GtkWidget *child, const gchar *name, const gchar *title)
{
  child->priv->stack_name = gtkqt_str (name);
  child->priv->tab_label = gtkqt_str (title);
  gtkqt_container_adopt (GTK_CONTAINER (stack), child);
}

void gtk_stack_add_named (GtkStack *stack, GtkWidget *child, const gchar *name) { gtk_stack_add_titled (stack, child, name, nullptr); }
void gtk_stack_add_child (GtkStack *stack, GtkWidget *child) { gtk_stack_add_titled (stack, child, nullptr, nullptr); }
void gtk_stack_remove (GtkStack *stack, GtkWidget *child) { gtk_container_remove (GTK_CONTAINER (stack), child); }

void gtk_stack_set_visible_child (GtkStack *stack, GtkWidget *child)
{
  if (auto *s = gtkqt_q<QStackedWidget> (GTK_WIDGET (stack)))
    if (QWidget *cw = gtkqt_qw (child)) s->setCurrentWidget (cw);
}

GtkWidget *gtk_stack_get_visible_child (GtkStack *stack)
{
  auto *s = gtkqt_q<QStackedWidget> (GTK_WIDGET (stack));
  return s && s->currentWidget () ? gtkqt_from_qobject (s->currentWidget ()) : nullptr;
}

GtkWidget *gtk_stack_get_child_by_name (GtkStack *stack, const gchar *name)
{
  for (GList *c = GTK_WIDGET (stack)->priv->children; c; c = c->next)
    if (((GtkWidget *) c->data)->priv->stack_name == gtkqt_str (name)) return (GtkWidget *) c->data;
  return nullptr;
}

void gtk_stack_set_visible_child_name (GtkStack *stack, const gchar *name)
{
  if (GtkWidget *c = gtk_stack_get_child_by_name (stack, name)) gtk_stack_set_visible_child (stack, c);
}

const gchar *gtk_stack_get_visible_child_name (GtkStack *stack)
{
  GtkWidget *c = gtk_stack_get_visible_child (stack);
  return c && !c->priv->stack_name.isEmpty () ? gtkqt_ret (GTK_WIDGET (stack), c->priv->stack_name) : nullptr;
}

void gtk_stack_set_transition_type (GtkStack *, GtkStackTransitionType) {}
void gtk_stack_set_transition_duration (GtkStack *, guint) {}

GtkWidget *gtk_stack_switcher_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_STACK_SWITCHER, nullptr); }

void gtk_stack_switcher_set_stack (GtkStackSwitcher *switcher, GtkStack *stack)
{
  GtkWidget *sw = GTK_WIDGET (switcher);
  g_object_set_data (G_OBJECT (sw), "gtkqt-stack", stack);
  if (!stack) { switcher_rebuild (sw); return; }
  auto cb = G_CALLBACK (+[] (GObject *, gpointer sw) { switcher_rebuild ((GtkWidget *) sw); });
  auto cb2 = G_CALLBACK (+[] (GObject *, GParamSpec *, gpointer sw) { switcher_rebuild ((GtkWidget *) sw); });
  g_signal_connect_object (stack, "gtkqt-pages-changed", cb, sw, (GConnectFlags) 0);
  g_signal_connect_object (stack, "notify::visible-child", cb2, sw, (GConnectFlags) 0);
  switcher_rebuild (sw);
}
}

/* ======================================================================
 * GtkScrolledWindow
 * ====================================================================== */
struct ScrollData { Qt::ScrollBarPolicy h = Qt::ScrollBarAsNeeded, v = Qt::ScrollBarAsNeeded; bool frame = true; };

static ScrollData *scroll_data (GtkWidget *w)
{
  auto *d = static_cast<ScrollData *> (g_object_get_data (G_OBJECT (w), "gtkqt-scroll"));
  if (!d) {
    d = new ScrollData;
    g_object_set_data_full (G_OBJECT (w), "gtkqt-scroll", d, [] (gpointer p) { delete static_cast<ScrollData *> (p); });
  }
  return d;
}

static QObject *scrolled_create (GtkWidget *)
{
  auto *qw = new QWidget;
  auto *l = new QVBoxLayout (qw);
  l->setContentsMargins (0, 0, 0, 0);
  return qw;
}

static QAbstractScrollArea *scrolled_area (GtkWidget *sw)
{
  QWidget *qw = gtkqt_qw (sw);
  return qw ? qw->findChild<QAbstractScrollArea *> (QString (), Qt::FindDirectChildrenOnly) : nullptr;
}

static void scrolled_apply (GtkWidget *sw)
{
  ScrollData *d = scroll_data (sw);
  if (QAbstractScrollArea *a = scrolled_area (sw)) {
    a->setHorizontalScrollBarPolicy (d->h);
    a->setVerticalScrollBarPolicy (d->v);
    a->setFrameShape (d->frame ? QFrame::StyledPanel : QFrame::NoFrame);
  }
}

static void scrolled_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (c)), *cw = gtkqt_qw (child);
  if (!qw || !cw) return;
  if (qobject_cast<QAbstractScrollArea *> (cw)) {
    qw->layout ()->addWidget (cw);   /* already scrollable (text view, tree view) */
  } else {
    auto *area = new QScrollArea (qw);
    area->setWidgetResizable (true);
    area->setWidget (cw);
    qw->layout ()->addWidget (area);
  }
  scrolled_apply (GTK_WIDGET (c));
}

static void scrolled_unplace (GtkContainer *c, GtkWidget *child)
{
  QWidget *cw = gtkqt_qw (child);
  QAbstractScrollArea *a = scrolled_area (GTK_WIDGET (c));
  if (auto *sa = qobject_cast<QScrollArea *> (a)) {
    if (sa->widget () == cw) sa->takeWidget ();
    delete sa;
  }
  if (cw) { cw->hide (); cw->setParent (nullptr); }
}

G_DEFINE_TYPE (GtkScrolledWindow, gtk_scrolled_window, GTK_TYPE_BIN)
static void gtk_scrolled_window_init (GtkScrolledWindow *) {}
static void gtk_scrolled_window_class_init (GtkScrolledWindowClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), scrolled_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), scrolled_place, scrolled_unplace);
  gtkqt_prop (klass, g_param_spec_enum ("hscrollbar-policy", nullptr, nullptr, gtk_policy_type_get_type (), GTK_POLICY_AUTOMATIC, RW),
              [] (GObject *o, const GValue *v) { auto *d = scroll_data (GTK_WIDGET (o));
                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (o), (GtkPolicyType) g_value_get_enum (v),
                  d->v == Qt::ScrollBarAlwaysOn ? GTK_POLICY_ALWAYS : d->v == Qt::ScrollBarAlwaysOff ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("vscrollbar-policy", nullptr, nullptr, gtk_policy_type_get_type (), GTK_POLICY_AUTOMATIC, RW),
              [] (GObject *o, const GValue *v) { auto *d = scroll_data (GTK_WIDGET (o));
                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (o),
                  d->h == Qt::ScrollBarAlwaysOn ? GTK_POLICY_ALWAYS : d->h == Qt::ScrollBarAlwaysOff ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC,
                  (GtkPolicyType) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("shadow-type", nullptr, nullptr, gtk_shadow_type_get_type (), GTK_SHADOW_NONE, RW),
              [] (GObject *o, const GValue *v) { gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (o), (GtkShadowType) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("has-frame", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_scrolled_window_set_has_frame (GTK_SCROLLED_WINDOW (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("min-content-height", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (o), g_value_get_int (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_int ("min-content-width", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (o), g_value_get_int (v)); }, nullptr);
}

extern "C" {
GtkWidget *gtk_scrolled_window_new (GtkAdjustment *, GtkAdjustment *)
{
  return (GtkWidget *) g_object_new (GTK_TYPE_SCROLLED_WINDOW, nullptr);
}

void gtk_scrolled_window_set_policy (GtkScrolledWindow *sw, GtkPolicyType h, GtkPolicyType v)
{
  auto conv = [] (GtkPolicyType p) {
    return p == GTK_POLICY_ALWAYS ? Qt::ScrollBarAlwaysOn : p == GTK_POLICY_NEVER || p == GTK_POLICY_EXTERNAL
           ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded;
  };
  ScrollData *d = scroll_data (GTK_WIDGET (sw));
  d->h = conv (h);
  d->v = conv (v);
  scrolled_apply (GTK_WIDGET (sw));
}

void gtk_scrolled_window_set_shadow_type (GtkScrolledWindow *sw, GtkShadowType type)
{
  scroll_data (GTK_WIDGET (sw))->frame = type != GTK_SHADOW_NONE;
  scrolled_apply (GTK_WIDGET (sw));
}

void gtk_scrolled_window_set_has_frame (GtkScrolledWindow *sw, gboolean has_frame)
{
  scroll_data (GTK_WIDGET (sw))->frame = has_frame;
  scrolled_apply (GTK_WIDGET (sw));
}

void gtk_scrolled_window_add_with_viewport (GtkScrolledWindow *sw, GtkWidget *child)
{
  gtk_container_add (GTK_CONTAINER (sw), child);
}

void gtk_scrolled_window_set_child (GtkScrolledWindow *sw, GtkWidget *child)
{
  if (GtkWidget *old = GTK_BIN (sw)->child) gtk_container_remove (GTK_CONTAINER (sw), old);
  if (child) gtk_container_add (GTK_CONTAINER (sw), child);
}

GtkWidget *gtk_scrolled_window_get_child (GtkScrolledWindow *sw) { return GTK_BIN (sw)->child; }
void gtk_scrolled_window_set_min_content_width (GtkScrolledWindow *sw, gint width)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (sw))) qw->setMinimumWidth (qMax (0, width));
}
void gtk_scrolled_window_set_min_content_height (GtkScrolledWindow *sw, gint height)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (sw))) qw->setMinimumHeight (qMax (0, height));
}
void gtk_scrolled_window_set_propagate_natural_height (GtkScrolledWindow *, gboolean) {}
}

/* ======================================================================
 * GtkExpander
 * ====================================================================== */
static QObject *expander_create (GtkWidget *w)
{
  auto *qw = new QWidget;
  auto *l = new QVBoxLayout (qw);
  l->setContentsMargins (0, 0, 0, 0);
  auto *btn = new QToolButton (qw);
  btn->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);
  btn->setArrowType (Qt::RightArrow);
  btn->setCheckable (true);
  btn->setAutoRaise (true);
  l->addWidget (btn, 0, Qt::AlignLeft);
  auto *content = new QWidget (qw);
  auto *cl = new QVBoxLayout (content);
  cl->setContentsMargins (12, 0, 0, 0);
  content->hide ();
  l->addWidget (content, 1);
  w->priv->inner = btn;
  w->priv->content = content;
  gtkqt_connect (w, btn, &QToolButton::toggled, [] (GtkWidget *e, bool on) {
    auto *b = gtkqt_inner<QToolButton> (e);
    b->setArrowType (on ? Qt::DownArrow : Qt::RightArrow);
    gtkqt_content (e)->setVisible (on);
    g_signal_emit_by_name (e, "activate");
    g_object_notify (G_OBJECT (e), "expanded");
  });
  return qw;
}

G_DEFINE_TYPE (GtkExpander, gtk_expander, GTK_TYPE_BIN)
static void gtk_expander_init (GtkExpander *) {}
static void gtk_expander_class_init (GtkExpanderClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), expander_create);
  g_signal_new ("activate", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_boolean ("expanded", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_expander_set_expanded (GTK_EXPANDER (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_expander_get_expanded (GTK_EXPANDER (o))); });
  gtkqt_prop (klass, g_param_spec_string ("label", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { if (auto *b = gtkqt_inner<QToolButton> (GTK_WIDGET (o))) b->setText (gtkqt_mnemonic (g_value_get_string (v))); }, nullptr);
}

extern "C" {
GtkWidget *gtk_expander_new (const gchar *label)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_EXPANDER, nullptr);
  if (auto *b = gtkqt_inner<QToolButton> (w)) b->setText (gtkqt_str (label));
  return w;
}
void gtk_expander_set_expanded (GtkExpander *e, gboolean expanded)
{
  if (auto *b = gtkqt_inner<QToolButton> (GTK_WIDGET (e))) b->setChecked (expanded);
}
gboolean gtk_expander_get_expanded (GtkExpander *e)
{
  auto *b = gtkqt_inner<QToolButton> (GTK_WIDGET (e));
  return b && b->isChecked ();
}
void gtk_expander_set_child (GtkExpander *e, GtkWidget *child)
{
  if (GtkWidget *old = GTK_BIN (e)->child) gtk_container_remove (GTK_CONTAINER (e), old);
  if (child) gtk_container_add (GTK_CONTAINER (e), child);
}
}

/* ======================================================================
 * GtkHeaderBar
 * ====================================================================== */
static QObject *headerbar_create (GtkWidget *w)
{
  auto *qw = new QFrame;
  qw->setFrameShape (QFrame::StyledPanel);
  qw->setAutoFillBackground (true);
  auto *l = new QHBoxLayout (qw);
  l->setContentsMargins (6, 4, 6, 4);
  auto *start = new QWidget (qw), *end = new QWidget (qw);
  auto *sl = new QHBoxLayout (start), *el = new QHBoxLayout (end);
  sl->setContentsMargins (0, 0, 0, 0);
  el->setContentsMargins (0, 0, 0, 0);
  el->setDirection (QBoxLayout::RightToLeft);
  auto *title = new QLabel (qw);
  QFont f = title->font ();
  f.setBold (true);
  title->setFont (f);
  title->setAlignment (Qt::AlignCenter);
  l->addWidget (start);
  l->addWidget (title, 1);
  l->addWidget (end);
  start->setObjectName ("start");
  end->setObjectName ("end");
  w->priv->inner = title;
  return qw;
}

static void headerbar_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (c)), *cw = gtkqt_qw (child);
  if (!qw || !cw) return;
  if (child->priv->stack_name == "title") {
    auto *l = static_cast<QHBoxLayout *> (qw->layout ());
    if (auto *title = gtkqt_inner<QLabel> (GTK_WIDGET (c))) title->hide ();
    l->insertWidget (2, cw, 1, Qt::AlignCenter);
    return;
  }
  QWidget *side = qw->findChild<QWidget *> (child->priv->pack_type == GTK_PACK_END ? "end" : "start");
  side->layout ()->addWidget (cw);
}

G_DEFINE_TYPE (GtkHeaderBar, gtk_header_bar, GTK_TYPE_CONTAINER)
static void gtk_header_bar_init (GtkHeaderBar *) {}
static void gtk_header_bar_class_init (GtkHeaderBarClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), headerbar_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), headerbar_place, gtkqt_bin_unplace_simple);
  gtkqt_prop (klass, g_param_spec_string ("title", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_header_bar_set_title (GTK_HEADER_BAR (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("show-close-button", nullptr, nullptr, FALSE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("show-title-buttons", nullptr, nullptr, TRUE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("title-widget", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) { gtk_header_bar_set_title_widget (GTK_HEADER_BAR (o), (GtkWidget *) g_value_get_object (v)); }, nullptr);
}

extern "C" {
GtkWidget *gtk_header_bar_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_HEADER_BAR, nullptr); }
void gtk_header_bar_pack_start (GtkHeaderBar *bar, GtkWidget *child)
{
  child->priv->pack_type = GTK_PACK_START;
  gtkqt_container_adopt (GTK_CONTAINER (bar), child);
}
void gtk_header_bar_pack_end (GtkHeaderBar *bar, GtkWidget *child)
{
  child->priv->pack_type = GTK_PACK_END;
  gtkqt_container_adopt (GTK_CONTAINER (bar), child);
}
void gtk_header_bar_set_title (GtkHeaderBar *bar, const gchar *title)
{
  if (auto *l = gtkqt_inner<QLabel> (GTK_WIDGET (bar))) l->setText (gtkqt_str (title));
  if (GTK_WIDGET (bar)->parent && GTK_IS_WINDOW (GTK_WIDGET (bar)->parent))
    gtk_window_set_title (GTK_WINDOW (GTK_WIDGET (bar)->parent), title);
}
void gtk_header_bar_set_subtitle (GtkHeaderBar *bar, const gchar *subtitle)
{
  if (auto *l = gtkqt_inner<QLabel> (GTK_WIDGET (bar))) {
    QString t = l->text ().section ('\n', 0, 0);
    l->setText (subtitle ? t + "\n" + gtkqt_str (subtitle) : t);
  }
}
void gtk_header_bar_set_title_widget (GtkHeaderBar *bar, GtkWidget *title_widget)
{
  if (!title_widget) return;
  title_widget->priv->stack_name = "title";
  gtkqt_container_adopt (GTK_CONTAINER (bar), title_widget);
}
void gtk_header_bar_set_show_close_button (GtkHeaderBar *, gboolean) {}
void gtk_header_bar_set_show_title_buttons (GtkHeaderBar *, gboolean) {}
}

/* ======================================================================
 * GtkSeparator
 * ====================================================================== */
static QObject *separator_create (GtkWidget *)
{
  auto *f = new QFrame;
  f->setFrameShape (QFrame::HLine);
  f->setFrameShadow (QFrame::Sunken);
  return f;
}

G_DEFINE_TYPE (GtkSeparator, gtk_separator, GTK_TYPE_WIDGET)
static void gtk_separator_init (GtkSeparator *) {}
static void gtk_separator_class_init (GtkSeparatorClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), separator_create);
  gtkqt_prop (klass, g_param_spec_enum ("orientation", nullptr, nullptr, gtk_orientation_get_type (), GTK_ORIENTATION_HORIZONTAL, RW),
              [] (GObject *o, const GValue *v) { gtk_orientable_set_orientation ((GtkOrientable *) o, (GtkOrientation) g_value_get_enum (v)); }, nullptr);
}

extern "C" {
GtkWidget *gtk_separator_new (GtkOrientation orientation)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_SEPARATOR, nullptr);
  gtk_orientable_set_orientation ((GtkOrientable *) w, orientation);
  return w;
}
GtkWidget *gtk_hseparator_new (void) { return gtk_separator_new (GTK_ORIENTATION_HORIZONTAL); }
GtkWidget *gtk_vseparator_new (void) { return gtk_separator_new (GTK_ORIENTATION_VERTICAL); }
}

/* ======================================================================
 * GtkStatusbar: a horizontal box with an internal message label
 * ====================================================================== */
struct StatusMsg { guint ctx, id; QString text; };
struct StatusData { QList<StatusMsg> stack; QStringList contexts; guint next_id = 1; };

static QObject *statusbar_create (GtkWidget *w)
{
  auto *qw = new QWidget;
  auto *l = new QHBoxLayout (qw);
  l->setContentsMargins (6, 2, 6, 2);
  auto *label = new QLabel (qw);
  l->addWidget (label, 1);
  auto *content = new QWidget (qw);
  auto *cl = new QBoxLayout (QBoxLayout::LeftToRight, content);
  cl->setContentsMargins (0, 0, 0, 0);
  cl->setSpacing (0);
  l->addWidget (content);
  w->priv->inner = label;
  w->priv->content = content;
  return qw;
}

static StatusData *status_data (GtkStatusbar *sb)
{
  auto *d = static_cast<StatusData *> (g_object_get_data (G_OBJECT (sb), "gtkqt-status"));
  if (!d) {
    d = new StatusData;
    g_object_set_data_full (G_OBJECT (sb), "gtkqt-status", d, [] (gpointer p) { delete static_cast<StatusData *> (p); });
  }
  return d;
}

static void status_update (GtkStatusbar *sb)
{
  StatusData *d = status_data (sb);
  if (auto *l = gtkqt_inner<QLabel> (GTK_WIDGET (sb))) l->setText (d->stack.isEmpty () ? QString () : d->stack.last ().text);
}

G_DEFINE_TYPE (GtkStatusbar, gtk_statusbar, GTK_TYPE_BOX)
static void gtk_statusbar_init (GtkStatusbar *) {}
static void gtk_statusbar_class_init (GtkStatusbarClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), statusbar_create);
}

extern "C" {
GtkWidget *gtk_statusbar_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_STATUSBAR, nullptr); }

guint gtk_statusbar_get_context_id (GtkStatusbar *sb, const gchar *context_description)
{
  StatusData *d = status_data (sb);
  QString c = gtkqt_str (context_description);
  int i = d->contexts.indexOf (c);
  if (i < 0) { d->contexts << c; i = d->contexts.size () - 1; }
  return i + 1;
}

guint gtk_statusbar_push (GtkStatusbar *sb, guint context_id, const gchar *text)
{
  StatusData *d = status_data (sb);
  guint id = d->next_id++;
  d->stack.append ({ context_id, id, gtkqt_str (text) });
  status_update (sb);
  return id;
}

void gtk_statusbar_pop (GtkStatusbar *sb, guint context_id)
{
  StatusData *d = status_data (sb);
  for (int i = d->stack.size () - 1; i >= 0; i--)
    if (d->stack[i].ctx == context_id) { d->stack.removeAt (i); break; }
  status_update (sb);
}

void gtk_statusbar_remove (GtkStatusbar *sb, guint context_id, guint message_id)
{
  StatusData *d = status_data (sb);
  for (int i = 0; i < d->stack.size (); i++)
    if (d->stack[i].ctx == context_id && d->stack[i].id == message_id) { d->stack.removeAt (i); break; }
  status_update (sb);
}

void gtk_statusbar_remove_all (GtkStatusbar *sb, guint context_id)
{
  StatusData *d = status_data (sb);
  for (int i = d->stack.size () - 1; i >= 0; i--)
    if (d->stack[i].ctx == context_id) d->stack.removeAt (i);
  status_update (sb);
}

void gtk_statusbar_set_has_resize_grip (GtkStatusbar *, gboolean) {}
}
