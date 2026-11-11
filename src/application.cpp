/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkApplication (a real GApplication: single instance, actions, command
 * line all come from GIO) and GtkApplicationWindow (a GtkWindow that is
 * also a GActionMap for "win." actions).
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

struct AppData {
  GList *windows = nullptr;
  GMenuModel *menubar = nullptr;
  QMap<QString, QStringList> accels;   /* detailed action name -> accelerators */
  guint next_window_id = 1;
};

static AppData *app_data (GtkApplication *app) { return static_cast<AppData *> (app->priv); }

G_DEFINE_TYPE (GtkApplication, gtk_application, G_TYPE_APPLICATION)

static void gtk_application_init (GtkApplication *app) { app->priv = new AppData; }

static void app_startup (GApplication *app)
{
  G_APPLICATION_CLASS (gtk_application_parent_class)->startup (app);
  gtkqt_ensure_app ();
  if (const gchar *id = g_application_get_application_id (app))
    QCoreApplication::setApplicationName (QString::fromUtf8 (id));
}

static void app_finalize (GObject *o)
{
  AppData *d = app_data (GTK_APPLICATION (o));
  g_list_free (d->windows);
  g_clear_object (&d->menubar);
  delete d;
  G_OBJECT_CLASS (gtk_application_parent_class)->finalize (o);
}

static void gtk_application_class_init (GtkApplicationClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = app_finalize;
  G_APPLICATION_CLASS (klass)->startup = app_startup;
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("window-added", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkApplicationClass, window_added), nullptr,
                nullptr, nullptr, G_TYPE_NONE, 1, GTK_TYPE_WINDOW);
  g_signal_new ("window-removed", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkApplicationClass, window_removed), nullptr,
                nullptr, nullptr, G_TYPE_NONE, 1, GTK_TYPE_WINDOW);
  g_signal_new ("query-end", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_object ("menubar", nullptr, nullptr, G_TYPE_MENU_MODEL, RW),
              [] (GObject *o, const GValue *v) { gtk_application_set_menubar (GTK_APPLICATION (o), (GMenuModel *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, app_data (GTK_APPLICATION (o))->menubar); });
  gtkqt_prop (klass, g_param_spec_object ("active-window", nullptr, nullptr, GTK_TYPE_WINDOW, G_PARAM_READABLE),
              nullptr, [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_application_get_active_window (GTK_APPLICATION (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("register-session", nullptr, nullptr, FALSE, RW), [] (GObject *, const GValue *) {}, nullptr);
}

QStringList gtkqt_application_accels (GtkApplication *app, const QString &detailed_action)
{
  return app_data (app)->accels.value (detailed_action);
}

/* Window-level shortcuts for every accelerator registered on the app. */
static void install_accels (GtkApplication *app, GtkWindow *window)
{
  QWidget *qw = gtkqt_qw (GTK_WIDGET (window));
  if (!qw) return;
  for (QAction *a : qw->actions ())
    if (a->property ("_gtkqt_app_accel").toBool ()) { qw->removeAction (a); delete a; }
  AppData *d = app_data (app);
  for (auto it = d->accels.begin (); it != d->accels.end (); ++it) {
    if (it.value ().isEmpty ()) continue;
    auto *a = new QAction (qw);
    a->setProperty ("_gtkqt_app_accel", true);
    QList<QKeySequence> keys;
    for (const QString &s : it.value ()) keys << gtkqt_accel_to_keyseq (s.toUtf8 ().constData ());
    a->setShortcuts (keys);
    a->setShortcutContext (Qt::WindowShortcut);
    QByteArray detailed = it.key ().toUtf8 ();
    GtkWidget *w = GTK_WIDGET (window);
    QObject::connect (a, &QAction::triggered, a, [w, detailed] {
      GtkQtEmitGuard guard;
      gtkqt_activate_action (w, detailed.constData (), nullptr);
    });
    qw->addAction (a);
  }
}

static void install_menubar (GtkApplication *app, GtkWindow *window)
{
  if (!GTK_IS_APPLICATION_WINDOW (window)) return;
  AppData *d = app_data (app);
  if (!d->menubar || !gtk_application_window_get_show_menubar (GTK_APPLICATION_WINDOW (window))) {
    gtkqt_window_set_menubar_widget (window, nullptr);
    return;
  }
  auto *mb = new QMenuBar;
  gtkqt_menubar_fill_from_model (mb, d->menubar, GTK_WIDGET (window));
  /* the window-level accel actions do the work; menu entries only display the keys */
  for (QAction *a : mb->findChildren<QAction *> ()) a->setShortcutContext (Qt::WidgetShortcut);
  gtkqt_window_set_menubar_widget (window, mb);
}

extern "C" {

GtkApplication *gtkqt_application_new (const gchar *application_id, GApplicationFlags flags, int api_version)
{
  if (api_version >= 2 && api_version <= 4) gtkqt_api = api_version;
  if (application_id && !g_application_id_is_valid (application_id)) {
    g_critical ("gtk_application_new: invalid application id '%s'", application_id);
    application_id = nullptr;
  }
  return (GtkApplication *) g_object_new (GTK_TYPE_APPLICATION, "application-id", application_id, "flags", flags, nullptr);
}

GtkApplication *gtk_application_new (const gchar *application_id, GApplicationFlags flags)
{
  return gtkqt_application_new (application_id, flags, gtkqt_api);
}

void gtk_application_add_window (GtkApplication *app, GtkWindow *window)
{
  AppData *d = app_data (app);
  if (g_list_find (d->windows, window)) return;
  d->windows = g_list_prepend (d->windows, window);
  gtkqt_window_set_application_internal (window, app);
  g_application_hold (G_APPLICATION (app));
  if (GTK_IS_APPLICATION_WINDOW (window))
    g_object_set_data (G_OBJECT (window), "gtkqt-window-id", GUINT_TO_POINTER (d->next_window_id++));
  install_menubar (app, window);
  install_accels (app, window);
  g_signal_emit_by_name (app, "window-added", window);
  g_object_notify (G_OBJECT (window), "application");
}

void gtk_application_remove_window (GtkApplication *app, GtkWindow *window)
{
  AppData *d = app_data (app);
  if (!g_list_find (d->windows, window)) return;
  d->windows = g_list_remove (d->windows, window);
  gtkqt_window_set_application_internal (window, nullptr);
  g_signal_emit_by_name (app, "window-removed", window);
  g_application_release (G_APPLICATION (app));
}

GList *gtk_application_get_windows (GtkApplication *app) { return app_data (app)->windows; }

GtkWindow *gtk_application_get_active_window (GtkApplication *app)
{
  for (GList *l = app_data (app)->windows; l; l = l->next)
    if (gtk_window_is_active ((GtkWindow *) l->data)) return (GtkWindow *) l->data;
  return app_data (app)->windows ? (GtkWindow *) app_data (app)->windows->data : nullptr;
}

GtkWindow *gtk_application_get_window_by_id (GtkApplication *app, guint id)
{
  for (GList *l = app_data (app)->windows; l; l = l->next)
    if (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (l->data), "gtkqt-window-id")) == id) return (GtkWindow *) l->data;
  return nullptr;
}

void gtk_application_set_menubar (GtkApplication *app, GMenuModel *menubar)
{
  AppData *d = app_data (app);
  g_set_object (&d->menubar, menubar);
  for (GList *l = d->windows; l; l = l->next) install_menubar (app, (GtkWindow *) l->data);
}

GMenuModel *gtk_application_get_menubar (GtkApplication *app) { return app_data (app)->menubar; }
void gtk_application_set_app_menu (GtkApplication *app, GMenuModel *app_menu)
{
  /* There is no app menu on Qt platforms; show it as a menubar if none is set. */
  if (!app_data (app)->menubar && app_menu) {
    GMenu *m = g_menu_new ();
    g_menu_append_submenu (m, g_get_application_name () ? g_get_application_name () : "Application", app_menu);
    gtk_application_set_menubar (app, G_MENU_MODEL (m));
    g_object_unref (m);
  }
}

GMenuModel *gtk_application_get_menu_by_id (GtkApplication *, const gchar *) { return nullptr; }

void gtk_application_set_accels_for_action (GtkApplication *app, const gchar *detailed_action_name, const gchar * const *accels)
{
  QStringList l;
  for (int i = 0; accels && accels[i]; i++) l << QString::fromUtf8 (accels[i]);
  app_data (app)->accels.insert (QString::fromUtf8 (detailed_action_name), l);
  for (GList *w = app_data (app)->windows; w; w = w->next) {
    install_accels (app, (GtkWindow *) w->data);
    install_menubar (app, (GtkWindow *) w->data);
  }
}

void gtk_application_add_accelerator (GtkApplication *app, const gchar *accelerator, const gchar *action_name, GVariant *parameter)
{
  gchar *detailed = g_action_print_detailed_name (action_name, parameter);
  const gchar *accels[] = { accelerator, nullptr };
  gtk_application_set_accels_for_action (app, detailed, accels);
  g_free (detailed);
}

guint gtk_application_inhibit (GtkApplication *, GtkWindow *, int, const gchar *) { return 0; }
void gtk_application_uninhibit (GtkApplication *, guint) {}

} /* extern "C" */

/* ======================================================================
 * GtkApplicationWindow: GActionGroup + GActionMap via an inner
 * GSimpleActionGroup
 * ====================================================================== */
static GSimpleActionGroup *win_group (gpointer w) { return G_SIMPLE_ACTION_GROUP (g_object_get_data (G_OBJECT (w), "gtkqt-win-actions")); }

static gchar **aw_list_actions (GActionGroup *g) { return g_action_group_list_actions (G_ACTION_GROUP (win_group (g))); }

static gboolean aw_query_action (GActionGroup *g, const gchar *name, gboolean *enabled, const GVariantType **ptype,
                                 const GVariantType **stype, GVariant **hint, GVariant **state)
{
  return g_action_group_query_action (G_ACTION_GROUP (win_group (g)), name, enabled, ptype, stype, hint, state);
}

static void aw_activate (GActionGroup *g, const gchar *name, GVariant *param)
{
  g_action_group_activate_action (G_ACTION_GROUP (win_group (g)), name, param);
}

static void aw_change_state (GActionGroup *g, const gchar *name, GVariant *value)
{
  g_action_group_change_action_state (G_ACTION_GROUP (win_group (g)), name, value);
}

static GAction *aw_lookup (GActionMap *m, const gchar *name) { return g_action_map_lookup_action (G_ACTION_MAP (win_group (m)), name); }
static void aw_add (GActionMap *m, GAction *a) { g_action_map_add_action (G_ACTION_MAP (win_group (m)), a); }
static void aw_remove (GActionMap *m, const gchar *name) { g_action_map_remove_action (G_ACTION_MAP (win_group (m)), name); }

static void aw_group_iface_init (GActionGroupInterface *iface)
{
  iface->list_actions = aw_list_actions;
  iface->query_action = aw_query_action;
  iface->activate_action = aw_activate;
  iface->change_action_state = aw_change_state;
}

static void aw_map_iface_init (GActionMapInterface *iface)
{
  iface->lookup_action = aw_lookup;
  iface->add_action = aw_add;
  iface->remove_action = aw_remove;
}

G_DEFINE_TYPE_WITH_CODE (GtkApplicationWindow, gtk_application_window, GTK_TYPE_WINDOW,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION_GROUP, aw_group_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION_MAP, aw_map_iface_init))

static void gtk_application_window_init (GtkApplicationWindow *w)
{
  GSimpleActionGroup *g = g_simple_action_group_new ();
  g_object_set_data_full (G_OBJECT (w), "gtkqt-win-actions", g, g_object_unref);
  g_object_set_data (G_OBJECT (w), "gtkqt-show-menubar", GINT_TO_POINTER (1));
  /* forward the group's change notifications as our own */
  g_signal_connect_object (g, "action-added", G_CALLBACK (+[] (GActionGroup *, const gchar *n, gpointer self) {
    g_action_group_action_added (G_ACTION_GROUP (self), n); }), w, (GConnectFlags) 0);
  g_signal_connect_object (g, "action-removed", G_CALLBACK (+[] (GActionGroup *, const gchar *n, gpointer self) {
    g_action_group_action_removed (G_ACTION_GROUP (self), n); }), w, (GConnectFlags) 0);
  g_signal_connect_object (g, "action-enabled-changed", G_CALLBACK (+[] (GActionGroup *, const gchar *n, gboolean e, gpointer self) {
    g_action_group_action_enabled_changed (G_ACTION_GROUP (self), n, e); }), w, (GConnectFlags) 0);
  g_signal_connect_object (g, "action-state-changed", G_CALLBACK (+[] (GActionGroup *, const gchar *n, GVariant *s, gpointer self) {
    g_action_group_action_state_changed (G_ACTION_GROUP (self), n, s); }), w, (GConnectFlags) 0);
}

static void gtk_application_window_class_init (GtkApplicationWindowClass *klass)
{
  gtkqt_prop (klass, g_param_spec_boolean ("show-menubar", nullptr, nullptr, TRUE, RW),
              [] (GObject *o, const GValue *v) { gtk_application_window_set_show_menubar (GTK_APPLICATION_WINDOW (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_application_window_get_show_menubar (GTK_APPLICATION_WINDOW (o))); });
}

extern "C" {

GtkWidget *gtk_application_window_new (GtkApplication *application)
{
  return (GtkWidget *) g_object_new (GTK_TYPE_APPLICATION_WINDOW, "application", application, nullptr);
}

void gtk_application_window_set_show_menubar (GtkApplicationWindow *window, gboolean show_menubar)
{
  g_object_set_data (G_OBJECT (window), "gtkqt-show-menubar", GINT_TO_POINTER (show_menubar));
  if (GtkApplication *app = gtk_window_get_application (GTK_WINDOW (window))) install_menubar (app, GTK_WINDOW (window));
}

gboolean gtk_application_window_get_show_menubar (GtkApplicationWindow *window)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window), "gtkqt-show-menubar"));
}

guint gtk_application_window_get_id (GtkApplicationWindow *window)
{
  return GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (window), "gtkqt-window-id"));
}

} /* extern "C" */
