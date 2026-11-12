/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkAccelGroup, GtkAction/GtkToggleAction/GtkRadioAction, GtkActionGroup and
 * GtkUIManager (the GTK 2.4 - 3.x way of building menus from XML).
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QXmlStreamReader>
#include <QFile>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

/* ======================================================================
 * GtkAccelGroup: a bag of QActions that windows adopt
 * ====================================================================== */
struct _GtkAccelGroup { GObject parent_instance; gpointer priv; };
typedef struct { GObjectClass parent_class; } GtkAccelGroupClass;
struct AccelData { QList<QPointer<QAction>> actions; QList<QPointer<QWidget>> windows; };
static AccelData *accel (GtkAccelGroup *g) { return static_cast<AccelData *> (g->priv); }

G_DEFINE_TYPE (GtkAccelGroup, gtk_accel_group, G_TYPE_OBJECT)
static void gtk_accel_group_init (GtkAccelGroup *g) { g->priv = new AccelData; }
static void accel_finalize (GObject *o)
{
  delete accel ((GtkAccelGroup *) o);
  G_OBJECT_CLASS (gtk_accel_group_parent_class)->finalize (o);
}
static void gtk_accel_group_class_init (GtkAccelGroupClass *klass) { G_OBJECT_CLASS (klass)->finalize = accel_finalize; }

void gtkqt_accel_group_attach (GtkAccelGroup *group, QWidget *window)
{
  if (!window) return;
  accel (group)->windows << window;
  for (const QPointer<QAction> &a : accel (group)->actions) if (a) window->addAction (a);
}

void gtkqt_accel_group_detach (GtkAccelGroup *group, QWidget *window)
{
  if (!window) return;
  accel (group)->windows.removeAll (window);
  for (const QPointer<QAction> &a : accel (group)->actions) if (a) window->removeAction (a);
}

void gtkqt_accel_group_add_action (GtkAccelGroup *group, QAction *action)
{
  if (accel (group)->actions.contains (action)) return;
  accel (group)->actions << action;
  for (const QPointer<QWidget> &w : accel (group)->windows) if (w) w->addAction (action);
}

extern "C" {

GtkAccelGroup *gtk_accel_group_new (void) { return (GtkAccelGroup *) g_object_new (gtk_accel_group_get_type (), nullptr); }

void gtk_accelerator_parse (const gchar *accelerator, guint *key, GdkModifierType *mods)
{
  guint k = 0, m = 0;
  QString s = gtkqt_str (accelerator);
  static const QRegularExpression mod (QStringLiteral ("^<([A-Za-z0-9]+)>"));
  QRegularExpressionMatch match;
  while ((match = mod.match (s)).hasMatch ()) {
    QString n = match.captured (1).toLower ();
    if (n == "control" || n == "ctrl" || n == "ctl" || n == "primary") m |= GDK_CONTROL_MASK;
    else if (n == "shift" || n == "shft") m |= GDK_SHIFT_MASK;
    else if (n == "alt" || n == "mod1") m |= GDK_MOD1_MASK;
    else if (n == "super") m |= GDK_SUPER_MASK;
    s = s.mid (match.capturedLength ());
  }
  if (!s.isEmpty ()) {
    k = gdk_keyval_from_name (s.toUtf8 ().constData ());
    if (k == GDK_KEY_VoidSymbol) k = 0;
    k = gdk_keyval_to_lower (k);
  }
  if (key) *key = k;
  if (mods) *mods = (GdkModifierType) m;
}

gchar *gtk_accelerator_name (guint key, GdkModifierType mods)
{
  QString s;
  if (mods & GDK_CONTROL_MASK) s += "<Control>";
  if (mods & GDK_SHIFT_MASK) s += "<Shift>";
  if (mods & GDK_MOD1_MASK) s += "<Alt>";
  if (mods & GDK_SUPER_MASK) s += "<Super>";
  const char *n = gdk_keyval_name (key);
  s += n ? QString::fromUtf8 (n) : QString ();
  return g_strdup (s.toUtf8 ().constData ());
}

} /* extern "C" */

GtkAccelGroup *gtkqt_accel_group_dummy (void) { return gtk_accel_group_new (); }

/* ======================================================================
 * GtkAction
 * ====================================================================== */
struct _GtkAction { GObject parent_instance; gpointer priv; };
typedef struct { GObjectClass parent_class; } GtkActionClass;
struct ActionData {
  QString name, label, tooltip, stock, accel;
  bool sensitive = true, visible = true, toggle = false, active = false;
  int radio_value = 0;
  GSList *radio_group = nullptr;   /* shared by all members (first member owns it) */
  QList<QPointer<QAction>> proxies;
};
static ActionData *act (gpointer a) { return static_cast<ActionData *> (((GtkAction *) a)->priv); }

G_DEFINE_TYPE (GtkAction, gtk_action, G_TYPE_OBJECT)
static void gtk_action_init (GtkAction *a) { a->priv = new ActionData; }
static void action_finalize (GObject *o)
{
  delete act (o);
  G_OBJECT_CLASS (gtk_action_parent_class)->finalize (o);
}
static void gtk_action_class_init (GtkActionClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = action_finalize;
  g_signal_new ("activate", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("toggled", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_OBJECT);
  gtkqt_prop (klass, g_param_spec_string ("name", nullptr, nullptr, nullptr, (GParamFlags) (RW | G_PARAM_CONSTRUCT_ONLY)),
              [] (GObject *o, const GValue *v) { act (o)->name = gtkqt_str (g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, act (o)->name.toUtf8 ().constData ()); });
  gtkqt_prop (klass, g_param_spec_string ("label", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { act (o)->label = gtkqt_str (g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("tooltip", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { act (o)->tooltip = gtkqt_str (g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("stock-id", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { act (o)->stock = gtkqt_str (g_value_get_string (v)); }, nullptr);
}

GType gtk_toggle_action_get_type (void) { return gtk_action_get_type (); }

static void action_set_active (GtkAction *a, bool on, bool emit)
{
  ActionData *d = act (a);
  if (d->active == on) return;
  d->active = on;
  for (const QPointer<QAction> &p : d->proxies) if (p) { QSignalBlocker b (p); p->setChecked (on); }
  if (on && d->radio_group) {
    for (GSList *l = d->radio_group; l; l = l->next)
      if (l->data != a) action_set_active ((GtkAction *) l->data, false, true);
    for (GSList *l = d->radio_group; l; l = l->next) g_signal_emit_by_name (l->data, "changed", a);
  }
  if (emit) g_signal_emit_by_name (a, "toggled");
}

extern "C" {

GtkAction *gtk_action_new (const gchar *name, const gchar *label, const gchar *tooltip, const gchar *stock_id)
{
  return (GtkAction *) g_object_new (GTK_TYPE_ACTION, "name", name, "label", label, "tooltip", tooltip, "stock-id", stock_id, nullptr);
}

const gchar *gtk_action_get_name (GtkAction *action)
{
  QByteArray *b = static_cast<QByteArray *> (g_object_get_data (G_OBJECT (action), "gtkqt-name-utf8"));
  if (!b) {
    b = new QByteArray (act (action)->name.toUtf8 ());
    g_object_set_data_full (G_OBJECT (action), "gtkqt-name-utf8", b, [] (gpointer p) { delete static_cast<QByteArray *> (p); });
  }
  return b->constData ();
}

void gtk_action_activate (GtkAction *action)
{
  ActionData *d = act (action);
  if (!d->sensitive) return;
  if (d->toggle) action_set_active (action, d->radio_group ? true : !d->active, true);
  g_signal_emit_by_name (action, "activate");
}

void gtk_action_set_sensitive (GtkAction *action, gboolean sensitive)
{
  act (action)->sensitive = sensitive;
  for (const QPointer<QAction> &p : act (action)->proxies) if (p) p->setEnabled (sensitive);
}

void gtk_action_set_visible (GtkAction *action, gboolean visible)
{
  act (action)->visible = visible;
  for (const QPointer<QAction> &p : act (action)->proxies) if (p) p->setVisible (visible);
}

gboolean gtk_toggle_action_get_active (GtkToggleAction *action) { return act (action)->active; }
void gtk_toggle_action_set_active (GtkToggleAction *action, gboolean is_active) { action_set_active (action, is_active, true); }

} /* extern "C" */

/* ======================================================================
 * GtkActionGroup
 * ====================================================================== */
struct _GtkActionGroup { GObject parent_instance; gpointer priv; };
typedef struct { GObjectClass parent_class; } GtkActionGroupClass;
struct GroupData { QString name; QList<GtkAction *> actions; };
static GroupData *grp (GtkActionGroup *g) { return static_cast<GroupData *> (g->priv); }

G_DEFINE_TYPE (GtkActionGroup, gtk_action_group, G_TYPE_OBJECT)
static void gtk_action_group_init (GtkActionGroup *g) { g->priv = new GroupData; }
static void group_finalize (GObject *o)
{
  for (GtkAction *a : grp ((GtkActionGroup *) o)->actions) g_object_unref (a);
  delete grp ((GtkActionGroup *) o);
  G_OBJECT_CLASS (gtk_action_group_parent_class)->finalize (o);
}
static void gtk_action_group_class_init (GtkActionGroupClass *klass) { G_OBJECT_CLASS (klass)->finalize = group_finalize; }

static GtkAction *action_from_entry (const char *name, const char *stock, const char *label, const char *accel, const char *tooltip)
{
  GtkAction *a = gtk_action_new (name, label, tooltip, stock);
  ActionData *d = act (a);
  if (!label && stock) d->label = gtkqt_stock_label (stock).replace ('&', '_');
  d->accel = gtkqt_str (accel);
  return a;
}

extern "C" {

GtkActionGroup *gtk_action_group_new (const gchar *name)
{
  auto *g = (GtkActionGroup *) g_object_new (GTK_TYPE_ACTION_GROUP, nullptr);
  grp (g)->name = gtkqt_str (name);
  return g;
}

void gtk_action_group_add_action (GtkActionGroup *g, GtkAction *action) { grp (g)->actions << GTK_ACTION (g_object_ref (action)); }

void gtk_action_group_add_action_with_accel (GtkActionGroup *g, GtkAction *action, const gchar *accelerator)
{
  if (accelerator) act (action)->accel = gtkqt_str (accelerator);
  gtk_action_group_add_action (g, action);
}

void gtk_action_group_add_actions (GtkActionGroup *g, const GtkActionEntry *entries, guint n, gpointer user_data)
{
  for (guint i = 0; i < n; i++) {
    const GtkActionEntry &e = entries[i];
    GtkAction *a = action_from_entry (e.name, e.stock_id, e.label, e.accelerator, e.tooltip);
    if (e.callback) g_signal_connect (a, "activate", e.callback, user_data);
    gtk_action_group_add_action (g, a);
    g_object_unref (a);
  }
}

void gtk_action_group_add_toggle_actions (GtkActionGroup *g, const GtkToggleActionEntry *entries, guint n, gpointer user_data)
{
  for (guint i = 0; i < n; i++) {
    const GtkToggleActionEntry &e = entries[i];
    GtkAction *a = action_from_entry (e.name, e.stock_id, e.label, e.accelerator, e.tooltip);
    act (a)->toggle = true;
    act (a)->active = e.is_active;
    if (e.callback) g_signal_connect (a, "activate", e.callback, user_data);
    gtk_action_group_add_action (g, a);
    g_object_unref (a);
  }
}

void gtk_action_group_add_radio_actions (GtkActionGroup *g, const GtkRadioActionEntry *entries, guint n, gint value,
                                         GCallback on_change, gpointer user_data)
{
  GSList *group = nullptr;
  QList<GtkAction *> made;
  for (guint i = 0; i < n; i++) {
    const GtkRadioActionEntry &e = entries[i];
    GtkAction *a = action_from_entry (e.name, e.stock_id, e.label, e.accelerator, e.tooltip);
    act (a)->toggle = true;
    act (a)->radio_value = e.value;
    act (a)->active = e.value == value;
    group = g_slist_append (group, a);
    made << a;
    gtk_action_group_add_action (g, a);
  }
  for (GtkAction *a : made) act (a)->radio_group = group;
  if (on_change && !made.isEmpty ()) g_signal_connect (made.first (), "changed", on_change, user_data);
  for (GtkAction *a : made) g_object_unref (a);
}

GtkAction *gtk_action_group_get_action (GtkActionGroup *g, const gchar *action_name)
{
  for (GtkAction *a : grp (g)->actions) if (act (a)->name == gtkqt_str (action_name)) return a;
  return nullptr;
}

void gtk_action_group_set_translation_domain (GtkActionGroup *, const gchar *) {}

} /* extern "C" */

/* ======================================================================
 * GtkUIManager
 * ====================================================================== */
struct _GtkUIManager { GObject parent_instance; gpointer priv; };
typedef struct { GObjectClass parent_class; } GtkUIManagerClass;
struct UIData {
  QList<GtkActionGroup *> groups;
  QMap<QString, GtkWidget *> widgets;   /* path -> widget (ref held) */
  QMap<QString, GtkAction *> path_actions;
  GtkAccelGroup *accel = nullptr;
  guint merge_id = 1;
};
static UIData *ui (GtkUIManager *m) { return static_cast<UIData *> (m->priv); }

G_DEFINE_TYPE (GtkUIManager, gtk_ui_manager, G_TYPE_OBJECT)
static void gtk_ui_manager_init (GtkUIManager *m) { m->priv = new UIData; }
static void ui_finalize (GObject *o)
{
  UIData *d = ui ((GtkUIManager *) o);
  for (GtkActionGroup *g : d->groups) g_object_unref (g);
  for (GtkWidget *w : d->widgets) g_object_unref (w);
  if (d->accel) g_object_unref (d->accel);
  delete d;
  G_OBJECT_CLASS (gtk_ui_manager_parent_class)->finalize (o);
}
static void gtk_ui_manager_class_init (GtkUIManagerClass *klass) { G_OBJECT_CLASS (klass)->finalize = ui_finalize; }

static GtkAction *ui_find_action (GtkUIManager *m, const QString &name)
{
  QByteArray n = name.toUtf8 ();
  for (GtkActionGroup *g : ui (m)->groups)
    if (GtkAction *a = gtk_action_group_get_action (g, n.constData ())) return a;
  return nullptr;
}

/* Hook a GTK menu item / tool button to its GtkAction. */
static void proxy_bind (GtkUIManager *m, GtkWidget *w, GtkAction *a)
{
  ActionData *d = act (a);
  QAction *qa = gtkqt_q<QAction> (w);
  if (qa) {
    d->proxies << qa;
    qa->setEnabled (d->sensitive);
    qa->setVisible (d->visible);
    if (!d->tooltip.isEmpty ()) qa->setToolTip (d->tooltip);
    if (!d->stock.isEmpty ()) qa->setIcon (gtkqt_icon (d->stock.toUtf8 ().constData ()));
    if (!d->accel.isEmpty ()) {
      qa->setShortcut (gtkqt_accel_to_keyseq (d->accel.toUtf8 ().constData ()));
      gtkqt_accel_group_add_action (gtk_ui_manager_get_accel_group (m), qa);
    }
    if (d->toggle) { qa->setCheckable (true); qa->setChecked (d->active); }
  }
  g_signal_connect_object (w, GTK_IS_MENU_ITEM (w) ? "activate" : "clicked",
                           G_CALLBACK (+[] (GtkWidget *, gpointer action) { gtk_action_activate (GTK_ACTION (action)); }),
                           a, (GConnectFlags) 0);
}

static void ui_register (GtkUIManager *m, const QString &path, GtkWidget *w)
{
  UIData *d = ui (m);
  if (GtkWidget *old = d->widgets.value (path)) g_object_unref (old);
  d->widgets.insert (path, GTK_WIDGET (g_object_ref_sink (w)));
}

static bool ui_parse (GtkUIManager *m, const QByteArray &xml, GError **error)
{
  QXmlStreamReader r (xml);
  struct Frame { QString path; GtkWidget *shell; };   /* shell: menubar/menu/toolbar to append into */
  QList<Frame> stack;
  while (!r.atEnd ()) {
    r.readNext ();
    if (r.isEndElement ()) {
      QString tag = r.name ().toString ();
      if ((tag == "menubar" || tag == "menu" || tag == "popup" || tag == "toolbar") && !stack.isEmpty ()) stack.removeLast ();
      continue;
    }
    if (!r.isStartElement ()) continue;
    QString tag = r.name ().toString ();
    QString action = r.attributes ().value ("action").toString ();
    QString name = r.attributes ().value ("name").toString ();
    if (name.isEmpty ()) name = action;
    QString parent_path = stack.isEmpty () ? QString () : stack.last ().path;
    QString path = parent_path + "/" + name;
    GtkWidget *shell = stack.isEmpty () ? nullptr : stack.last ().shell;
    GtkAction *a = action.isEmpty () ? nullptr : ui_find_action (m, action);
    if (!action.isEmpty () && !a && tag != "ui")
      g_warning ("GtkUIManager: no action '%s' for <%s>", action.toUtf8 ().constData (), tag.toUtf8 ().constData ());
    if (a) ui (m)->path_actions.insert (path, a);

    if (tag == "ui") continue;
    if (tag == "menubar" || tag == "popup" || tag == "toolbar") {
      GtkWidget *w = tag == "menubar" ? gtk_menu_bar_new () : tag == "popup" ? gtk_menu_new () : gtk_toolbar_new ();
      if (tag != "popup") gtk_widget_show (w);
      ui_register (m, path, w);
      stack << Frame { path, w };
    } else if (tag == "menu") {
      GtkWidget *item = gtk_menu_item_new_with_mnemonic (a ? act (a)->label.toUtf8 ().constData () : name.toUtf8 ().constData ());
      GtkWidget *menu = gtk_menu_new ();
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
      gtk_widget_show (item);
      if (shell && GTK_IS_MENU_SHELL (shell)) gtk_menu_shell_append (GTK_MENU_SHELL (shell), item);
      ui_register (m, path, item);
      stack << Frame { path, menu };
    } else if (tag == "menuitem" && shell && GTK_IS_MENU_SHELL (shell)) {
      QByteArray label = a ? act (a)->label.toUtf8 () : name.toUtf8 ();
      GtkWidget *item = a && act (a)->toggle ? gtk_check_menu_item_new_with_mnemonic (label.constData ())
                                             : gtk_menu_item_new_with_mnemonic (label.constData ());
      if (a) proxy_bind (m, item, a);
      gtk_widget_show (item);
      gtk_menu_shell_append (GTK_MENU_SHELL (shell), item);
      ui_register (m, path, item);
    } else if (tag == "toolitem" && shell && GTK_IS_TOOLBAR (shell)) {
      GtkToolItem *item = a && act (a)->toggle ? gtk_toggle_tool_button_new () : gtk_tool_button_new (nullptr, nullptr);
      if (a) {
        QByteArray label = act (a)->label.toUtf8 ();
        gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), label.constData ());
        if (!act (a)->stock.isEmpty ()) gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item), act (a)->stock.toUtf8 ().constData ());
        if (!act (a)->tooltip.isEmpty ()) gtk_tool_item_set_tooltip_text (item, act (a)->tooltip.toUtf8 ().constData ());
        proxy_bind (m, GTK_WIDGET (item), a);
      }
      gtk_widget_show (GTK_WIDGET (item));
      gtk_toolbar_insert (GTK_TOOLBAR (shell), item, -1);
      ui_register (m, path, GTK_WIDGET (item));
    } else if (tag == "separator" && shell) {
      if (GTK_IS_MENU_SHELL (shell)) {
        GtkWidget *sep = gtk_separator_menu_item_new ();
        gtk_widget_show (sep);
        gtk_menu_shell_append (GTK_MENU_SHELL (shell), sep);
      } else if (GTK_IS_TOOLBAR (shell)) {
        GtkToolItem *sep = gtk_separator_tool_item_new ();
        gtk_widget_show (GTK_WIDGET (sep));
        gtk_toolbar_insert (GTK_TOOLBAR (shell), sep, -1);
      }
    } else if (tag == "placeholder" || tag == "accelerator") {
      /* placeholders are transparent; accelerators are taken from the actions */
    }
  }
  if (r.hasError ()) {
    g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, "UI description: %s (line %lld)",
                 r.errorString ().toUtf8 ().constData (), (long long) r.lineNumber ());
    return false;
  }
  return true;
}

extern "C" {

GtkUIManager *gtk_ui_manager_new (void) { return (GtkUIManager *) g_object_new (GTK_TYPE_UI_MANAGER, nullptr); }

void gtk_ui_manager_insert_action_group (GtkUIManager *m, GtkActionGroup *g, gint pos)
{
  g_object_ref (g);
  if (pos < 0 || pos > ui (m)->groups.size ()) ui (m)->groups.append (g);
  else ui (m)->groups.insert (pos, g);
}

guint gtk_ui_manager_add_ui_from_string (GtkUIManager *m, const gchar *buffer, gssize length, GError **error)
{
  gtkqt_ensure_app ();
  QByteArray xml = length < 0 ? QByteArray (buffer) : QByteArray (buffer, length);
  return ui_parse (m, xml, error) ? ui (m)->merge_id++ : 0;
}

guint gtk_ui_manager_add_ui_from_file (GtkUIManager *m, const gchar *filename, GError **error)
{
  QFile f (gtkqt_str (filename));
  if (!f.open (QIODevice::ReadOnly)) {
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT, "Cannot open %s", filename);
    return 0;
  }
  QByteArray data = f.readAll ();
  return gtk_ui_manager_add_ui_from_string (m, data.constData (), data.size (), error);
}

GtkWidget *gtk_ui_manager_get_widget (GtkUIManager *m, const gchar *path)
{
  QString p = gtkqt_str (path);
  if (!p.startsWith ('/')) p.prepend ('/');
  if (GtkWidget *w = ui (m)->widgets.value (p)) return w;
  /* GTK also accepts paths without the toplevel's name for unique leaves */
  for (auto it = ui (m)->widgets.begin (); it != ui (m)->widgets.end (); ++it)
    if (it.key ().endsWith (p)) return it.value ();
  return nullptr;
}

GtkAction *gtk_ui_manager_get_action (GtkUIManager *m, const gchar *path)
{
  QString p = gtkqt_str (path);
  if (!p.startsWith ('/')) p.prepend ('/');
  return ui (m)->path_actions.value (p);
}

GtkAccelGroup *gtk_ui_manager_get_accel_group (GtkUIManager *m)
{
  if (!ui (m)->accel) ui (m)->accel = gtk_accel_group_new ();
  return ui (m)->accel;
}

void gtk_ui_manager_ensure_update (GtkUIManager *) {}

} /* extern "C" */
