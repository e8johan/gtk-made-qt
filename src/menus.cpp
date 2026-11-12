/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Menus (GtkMenuShell/GtkMenuItem, GMenuModel), popovers, toolbars.
 *
 * GTK menu items are widgets; in Qt they are QActions. A GtkMenuItem here
 * therefore owns a QAction instead of a QWidget, and menu shells add those
 * actions to their QMenu / QMenuBar.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

/* ======================================================================
 * Action lookup ("app.quit", "win.save", custom prefixes)
 * ====================================================================== */
GActionGroup *gtkqt_find_action_group (GtkWidget *widget, const QByteArray &prefix)
{
  GtkWidget *top = nullptr;
  for (GtkWidget *w = widget; w; w = w->parent) {
    if (GActionGroup *g = w->priv->action_groups.value (prefix)) return g;
    if (prefix == "win" && GTK_IS_APPLICATION_WINDOW (w)) return G_ACTION_GROUP (w);
    top = w;
  }
  if (prefix == "app") {
    GtkApplication *app = top && GTK_IS_WINDOW (top) ? gtk_window_get_application (GTK_WINDOW (top)) : nullptr;
    GApplication *ga = app ? G_APPLICATION (app) : g_application_get_default ();
    if (ga) return G_ACTION_GROUP (ga);
  }
  return nullptr;
}

gboolean gtkqt_activate_action (GtkWidget *widget, const gchar *detailed_name, GVariant *target)
{
  if (!detailed_name) return FALSE;
  gchar *name = nullptr;
  GVariant *parsed_target = nullptr;
  if (!g_action_parse_detailed_name (detailed_name, &name, &parsed_target, nullptr)) return FALSE;
  QByteArray full = name, prefix, action = full;
  int dot = full.indexOf ('.');
  if (dot > 0) { prefix = full.left (dot); action = full.mid (dot + 1); }
  GActionGroup *g = gtkqt_find_action_group (widget, prefix);
  gboolean ok = FALSE;
  if (g && g_action_group_has_action (g, action.constData ())) {
    if (g_action_group_get_action_enabled (g, action.constData ())) {
      const GVariantType *pt = g_action_group_get_action_parameter_type (g, action.constData ());
      GVariant *param = target ? target : parsed_target;
      const GVariantType *state_type = g_action_group_get_action_state_type (g, action.constData ());
      if (!pt && !param && state_type && g_variant_type_equal (state_type, G_VARIANT_TYPE_BOOLEAN)) {
        /* stateless activation of a boolean toggle action */
        GVariant *st = g_action_group_get_action_state (g, action.constData ());
        g_action_group_change_action_state (g, action.constData (), g_variant_new_boolean (!g_variant_get_boolean (st)));
        g_variant_unref (st);
      } else {
        g_action_group_activate_action (g, action.constData (), pt ? param : nullptr);
      }
    }
    ok = TRUE;
  } else {
    g_warning ("gtk-made-qt: action '%s' not found", detailed_name);
  }
  if (parsed_target) g_variant_unref (parsed_target);
  g_free (name);
  return ok;
}

/* Refresh enabled/checked state of model-built actions right before a menu shows. */
static void refresh_model_actions (QWidget *menu)
{
  for (QAction *a : menu->actions ()) {
    QByteArray name = a->property ("_gtkqt_action").toByteArray ();
    if (name.isEmpty ()) continue;
    GtkWidget *ctx = gtkqt_from_qobject (menu);
    if (!ctx) {
      QVariant v = menu->property ("_gtkqt_context");
      ctx = v.isValid () ? static_cast<GtkWidget *> (v.value<void *> ()) : nullptr;
    }
    QByteArray prefix, action = name;
    int dot = name.indexOf ('.');
    if (dot > 0) { prefix = name.left (dot); action = name.mid (dot + 1); }
    GActionGroup *g = ctx ? gtkqt_find_action_group (ctx, prefix) : nullptr;
    if (!g && prefix == "app" && g_application_get_default ()) g = G_ACTION_GROUP (g_application_get_default ());
    if (!g || !g_action_group_has_action (g, action.constData ())) continue;
    a->setEnabled (g_action_group_get_action_enabled (g, action.constData ()));
    if (GVariant *st = g_action_group_get_action_state (g, action.constData ())) {
      if (g_variant_is_of_type (st, G_VARIANT_TYPE_BOOLEAN)) {
        a->setCheckable (true);
        a->setChecked (g_variant_get_boolean (st));
      } else if (a->property ("_gtkqt_target").isValid ()) {
        GVariant *t = g_variant_parse (nullptr, a->property ("_gtkqt_target").toByteArray ().constData (), nullptr, nullptr, nullptr);
        a->setCheckable (true);
        a->setChecked (t && g_variant_equal (t, st));
        if (t) g_variant_unref (t);
      }
      g_variant_unref (st);
    }
  }
}

static void fill_from_model (QWidget *target, GMenuModel *model, GtkWidget *context, bool *need_separator)
{
  int n = g_menu_model_get_n_items (model);
  bool is_menu = qobject_cast<QMenu *> (target) != nullptr;
  for (int i = 0; i < n; i++) {
    gchar *label = nullptr, *action = nullptr, *accel = nullptr, *icon = nullptr;
    g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_LABEL, "s", &label);
    g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_ACTION, "s", &action);
    g_menu_model_get_item_attribute (model, i, "accel", "s", &accel);
    g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_ICON, "s", &icon);
    GVariant *target_v = g_menu_model_get_item_attribute_value (model, i, G_MENU_ATTRIBUTE_TARGET, nullptr);
    GMenuModel *section = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION);
    GMenuModel *submenu = g_menu_model_get_item_link (model, i, G_MENU_LINK_SUBMENU);
    if (section) {
      if (is_menu && *need_separator) static_cast<QMenu *> (target)->addSeparator ();
      bool inner = false;
      fill_from_model (target, section, context, &inner);
      *need_separator = true;
      g_object_unref (section);
    } else if (submenu) {
      auto *m = new QMenu (gtkqt_mnemonic (label), target);
      m->setProperty ("_gtkqt_context", QVariant::fromValue ((void *) context));
      bool inner = false;
      fill_from_model (m, submenu, context, &inner);
      QObject::connect (m, &QMenu::aboutToShow, m, [m] { refresh_model_actions (m); });
      if (auto *mb = qobject_cast<QMenuBar *> (target)) mb->addMenu (m);
      else static_cast<QMenu *> (target)->addMenu (m);
      *need_separator = true;
      g_object_unref (submenu);
    } else {
      auto *a = new QAction (gtkqt_mnemonic (label), target);
      if (icon) a->setIcon (gtkqt_icon (icon));
      if (action) a->setProperty ("_gtkqt_action", QByteArray (action));
      if (target_v) {
        gchar *s = g_variant_print (target_v, TRUE);
        a->setProperty ("_gtkqt_target", QByteArray (s));
        g_free (s);
      }
      QStringList accels;
      if (accel) accels << gtkqt_str (accel);
      if (action) {
        QString detailed = gtkqt_str (action);
        if (target_v) {
          gchar *d = g_action_print_detailed_name (action, target_v);
          detailed = gtkqt_str (d);
          g_free (d);
        }
        GtkApplication *app = nullptr;
        if (GApplication *ga = g_application_get_default ()) if (GTK_IS_APPLICATION (ga)) app = GTK_APPLICATION (ga);
        if (app) accels += gtkqt_application_accels (app, detailed);
      }
      if (!accels.isEmpty ()) a->setShortcut (gtkqt_accel_to_keyseq (accels.first ().toUtf8 ().constData ()));
      QObject::connect (a, &QAction::triggered, a, [a, target] {
        GtkQtEmitGuard guard;
        QByteArray name = a->property ("_gtkqt_action").toByteArray ();
        if (name.isEmpty ()) return;
        QWidget *w = target;
        GtkWidget *c = nullptr;
        while (w && !c) {
          QVariant v = w->property ("_gtkqt_context");
          c = v.isValid () ? static_cast<GtkWidget *> (v.value<void *> ()) : gtkqt_from_qobject (w);
          w = w->parentWidget ();
        }
        GVariant *t = nullptr;
        if (a->property ("_gtkqt_target").isValid ())
          t = g_variant_parse (nullptr, a->property ("_gtkqt_target").toByteArray ().constData (), nullptr, nullptr, nullptr);
        if (c) gtkqt_activate_action (c, name.constData (), t);
        else if (GApplication *app = g_application_get_default ()) {
          int dot = name.indexOf ('.');
          g_action_group_activate_action (G_ACTION_GROUP (app), name.mid (dot + 1).constData (), t);
        }
        if (t) g_variant_unref (t);
      });
      target->addAction (a);
      *need_separator = true;
    }
    if (target_v) g_variant_unref (target_v);
    g_free (label);
    g_free (action);
    g_free (accel);
    g_free (icon);
  }
}

QMenu *gtkqt_menu_from_model (GMenuModel *model, GtkWidget *context, QWidget *parent)
{
  auto *m = new QMenu (parent);
  m->setProperty ("_gtkqt_context", QVariant::fromValue ((void *) context));
  bool sep = false;
  if (model) fill_from_model (m, model, context, &sep);
  QObject::connect (m, &QMenu::aboutToShow, m, [m] { refresh_model_actions (m); });
  return m;
}

void gtkqt_menubar_fill_from_model (QWidget *menubar, GMenuModel *model, GtkWidget *context)
{
  for (QAction *a : menubar->actions ()) { menubar->removeAction (a); if (a->parent () == menubar) delete a; }
  for (QMenu *m : menubar->findChildren<QMenu *> (QString (), Qt::FindDirectChildrenOnly)) delete m;
  menubar->setProperty ("_gtkqt_context", QVariant::fromValue ((void *) context));
  bool sep = false;
  if (model) fill_from_model (menubar, model, context, &sep);
}

/* ======================================================================
 * GtkMenuShell / GtkMenuBar / GtkMenu
 * ====================================================================== */
static QAction *item_action (GtkWidget *child)
{
  QObject *o = gtkqt_obj (child);
  if (auto *a = qobject_cast<QAction *> (o)) return a;
  /* arbitrary widget inside a menu: wrap it */
  auto *wa = static_cast<QWidgetAction *> (g_object_get_data (G_OBJECT (child), "gtkqt-widget-action"));
  if (!wa && qobject_cast<QWidget *> (o)) {
    wa = new QWidgetAction (nullptr);
    wa->setDefaultWidget (static_cast<QWidget *> (o));
    g_object_set_data (G_OBJECT (child), "gtkqt-widget-action", wa);
  }
  return wa;
}

static void shell_place (GtkContainer *c, GtkWidget *child)
{
  QWidget *shell = gtkqt_qw (GTK_WIDGET (c));
  QAction *a = item_action (child);
  if (!shell || !a) return;
  int pos = child->priv->grid_left;
  QList<QAction *> acts = shell->actions ();
  if (pos >= 0 && pos < acts.size ()) shell->insertAction (acts.at (pos), a);
  else shell->addAction (a);
}

static void shell_unplace (GtkContainer *c, GtkWidget *child)
{
  QWidget *shell = gtkqt_qw (GTK_WIDGET (c));
  if (shell) if (QAction *a = item_action (child)) shell->removeAction (a);
}

static void shell_add (GtkContainer *c, GtkWidget *child)
{
  child->priv->grid_left = -1;
  gtkqt_container_adopt (c, child);
}

static QObject *menubar_create (GtkWidget *) { return new QMenuBar; }

static QObject *menu_create (GtkWidget *w)
{
  auto *m = new QMenu;
  gtkqt_connect (w, m, &QMenu::aboutToHide, [] (GtkWidget *mw) { g_signal_emit_by_name (mw, "deactivate"); });
  return m;
}

G_DEFINE_TYPE (GtkMenuShell, gtk_menu_shell, GTK_TYPE_CONTAINER)
G_DEFINE_TYPE (GtkMenuBar, gtk_menu_bar, GTK_TYPE_MENU_SHELL)
G_DEFINE_TYPE (GtkMenu, gtk_menu, GTK_TYPE_MENU_SHELL)
static void gtk_menu_shell_init (GtkMenuShell *) {}
static void gtk_menu_shell_class_init (GtkMenuShellClass *klass)
{
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), shell_place, shell_unplace);
  GTK_CONTAINER_CLASS (klass)->add = shell_add;
  g_signal_new ("deactivate", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("selection-done", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}
static void gtk_menu_bar_init (GtkMenuBar *) {}
static void gtk_menu_bar_class_init (GtkMenuBarClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), menubar_create); }
static void gtk_menu_init (GtkMenu *m) { GTK_WIDGET (m)->priv->visible = false; }
static void gtk_menu_class_init (GtkMenuClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), menu_create); }

extern "C" {

void gtk_menu_shell_insert (GtkMenuShell *menu_shell, GtkWidget *child, gint position)
{
  g_return_if_fail (GTK_IS_MENU_SHELL (menu_shell));
  if (child->parent) return;
  child->priv->grid_left = position;
  gtkqt_container_adopt (GTK_CONTAINER (menu_shell), child);
}

void gtk_menu_shell_append (GtkMenuShell *menu_shell, GtkWidget *child) { gtk_menu_shell_insert (menu_shell, child, -1); }
void gtk_menu_shell_prepend (GtkMenuShell *menu_shell, GtkWidget *child) { gtk_menu_shell_insert (menu_shell, child, 0); }
void gtk_menu_append (GtkMenu *menu, GtkWidget *child) { gtk_menu_shell_append (GTK_MENU_SHELL (menu), child); }
void gtk_menu_prepend (GtkMenu *menu, GtkWidget *child) { gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), child); }
void gtk_menu_bar_append (GtkMenuBar *menu, GtkWidget *child) { gtk_menu_shell_append (GTK_MENU_SHELL (menu), child); }

GtkWidget *gtk_menu_bar_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_MENU_BAR, nullptr); }

GtkWidget *gtk_menu_bar_new_from_model (GMenuModel *model)
{
  GtkWidget *w = gtk_menu_bar_new ();
  if (QWidget *mb = gtkqt_qw (w)) gtkqt_menubar_fill_from_model (mb, model, w);
  return w;
}

GtkWidget *gtk_menu_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_MENU, nullptr); }

GtkWidget *gtk_menu_new_from_model (GMenuModel *model)
{
  GtkWidget *w = gtk_menu_new ();
  if (auto *m = gtkqt_q<QMenu> (w)) {
    bool sep = false;
    m->setProperty ("_gtkqt_context", QVariant::fromValue ((void *) w));
    fill_from_model (m, model, w, &sep);
    QObject::connect (m, &QMenu::aboutToShow, m, [m] { refresh_model_actions (m); });
  }
  return w;
}

void gtk_menu_popup (GtkMenu *menu, GtkWidget *, GtkWidget *, GtkMenuPositionFunc func, gpointer data, guint, guint32)
{
  auto *m = gtkqt_q<QMenu> (GTK_WIDGET (menu));
  if (!m) return;
  QPoint pos = QCursor::pos ();
  if (func) {
    gint x = pos.x (), y = pos.y ();
    gboolean push_in = FALSE;
    if (gtkqt_api == 1)   /* GTK 1.2's position function has no push_in */
      reinterpret_cast<void (*) (GtkMenu *, gint *, gint *, gpointer)> (func) (menu, &x, &y, data);
    else
      reinterpret_cast<void (*) (GtkMenu *, gint *, gint *, gboolean *, gpointer)> (func) (menu, &x, &y, &push_in, data);
    pos = QPoint (x, y);
  }
  m->popup (pos);
}

void gtk_menu_popup_at_pointer (GtkMenu *menu, const GdkEvent *) { gtk_menu_popup (menu, nullptr, nullptr, nullptr, nullptr, 0, 0); }

void gtk_menu_popup_at_widget (GtkMenu *menu, GtkWidget *widget, int, int, const GdkEvent *)
{
  auto *m = gtkqt_q<QMenu> (GTK_WIDGET (menu));
  QWidget *w = gtkqt_qw (widget);
  if (m) m->popup (w ? w->mapToGlobal (QPoint (0, w->height ())) : QCursor::pos ());
}

void gtk_menu_popdown (GtkMenu *menu) { if (auto *m = gtkqt_q<QMenu> (GTK_WIDGET (menu))) m->hide (); }
void gtk_menu_attach_to_widget (GtkMenu *, GtkWidget *, gpointer) {}

void gtk_menu_set_accel_group (GtkMenu *menu, GtkAccelGroup *accel_group)
{
  g_object_set_data (G_OBJECT (menu), "gtkqt-accel-group", accel_group);
}

void gtk_menu_set_title (GtkMenu *menu, const gchar *title) { if (auto *m = gtkqt_q<QMenu> (GTK_WIDGET (menu))) m->setTitle (gtkqt_str (title)); }

} /* extern "C" */

/* ======================================================================
 * GtkMenuItem family
 * ====================================================================== */
static QObject *menu_item_create (GtkWidget *w)
{
  auto *a = new QAction (nullptr);
  gtkqt_connect (w, a, &QAction::triggered, [] (GtkWidget *mw, bool) {
    g_signal_emit_by_name (mw, "activate");
    if (mw->priv->destroyed) return;
    if (const char *action = (const char *) g_object_get_data (G_OBJECT (mw), "gtkqt-action-name"))
      gtkqt_activate_action (mw, action, (GVariant *) g_object_get_data (G_OBJECT (mw), "gtkqt-action-target"));
  });
  return a;
}

static void menu_item_place (GtkContainer *c, GtkWidget *child)
{
  /* GTK lets you put any widget into a menu item; we take the text of the
   * first label inside it. */
  auto *a = gtkqt_q<QAction> (GTK_WIDGET (c));
  if (!a) return;
  std::function<GtkWidget *(GtkWidget *)> find_label = [&] (GtkWidget *w) -> GtkWidget * {
    if (GTK_IS_LABEL (w)) return w;
    for (GList *l = w->priv->children; l; l = l->next)
      if (GtkWidget *f = find_label ((GtkWidget *) l->data)) return f;
    return nullptr;
  };
  if (GtkWidget *label = find_label (child))
    a->setText (gtkqt_str (gtk_label_get_text (GTK_LABEL (label))).replace ('&', QStringLiteral ("&&")));
}

static void menu_item_unplace (GtkContainer *, GtkWidget *) {}

static void menu_item_dispose (GObject *o);

G_DEFINE_TYPE (GtkMenuItem, gtk_menu_item, GTK_TYPE_BIN)
static void gtk_menu_item_init (GtkMenuItem *) {}
static void gtk_menu_item_class_init (GtkMenuItemClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = menu_item_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), menu_item_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), menu_item_place, menu_item_unplace);
  g_signal_new ("activate", G_TYPE_FROM_CLASS (klass), (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION), 0, nullptr,
                nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("activate-item", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_string ("label", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_menu_item_set_label (GTK_MENU_ITEM (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, gtk_menu_item_get_label (GTK_MENU_ITEM (o))); });
  gtkqt_prop (klass, g_param_spec_boolean ("use-underline", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_menu_item_set_use_underline (GTK_MENU_ITEM (o), g_value_get_boolean (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("submenu", nullptr, nullptr, GTK_TYPE_MENU, RW),
              [] (GObject *o, const GValue *v) { gtk_menu_item_set_submenu (GTK_MENU_ITEM (o), (GtkWidget *) g_value_get_object (v)); },
              [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_menu_item_get_submenu (GTK_MENU_ITEM (o))); });
  gtkqt_prop (klass, g_param_spec_string ("action-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_actionable_set_action_name ((GtkActionable *) o, g_value_get_string (v)); }, nullptr);
}

static void menu_item_dispose (GObject *o)
{
  if (GtkWidget *sub = (GtkWidget *) g_object_get_data (o, "gtkqt-submenu")) {
    g_object_set_data (o, "gtkqt-submenu", nullptr);
    gtk_widget_destroy (sub);
    g_object_unref (sub);
  }
  if (auto *wa = static_cast<QWidgetAction *> (g_object_get_data (o, "gtkqt-widget-action"))) {
    g_object_set_data (o, "gtkqt-widget-action", nullptr);
    gtkqt_defer_delete (wa);
  }
  G_OBJECT_CLASS (gtk_menu_item_parent_class)->dispose (o);
}

static QObject *check_item_create (GtkWidget *w)
{
  auto *a = static_cast<QAction *> (menu_item_create (w));
  a->setCheckable (true);
  gtkqt_connect (w, a, &QAction::toggled, [] (GtkWidget *mw, bool on) {
    GTK_CHECK_MENU_ITEM (mw)->active = on;
    g_signal_emit_by_name (mw, "toggled");
    g_object_notify (G_OBJECT (mw), "active");
  });
  return a;
}

G_DEFINE_TYPE (GtkCheckMenuItem, gtk_check_menu_item, GTK_TYPE_MENU_ITEM)
static void gtk_check_menu_item_init (GtkCheckMenuItem *) {}
static void gtk_check_menu_item_class_init (GtkCheckMenuItemClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), check_item_create);
  g_signal_new ("toggled", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_boolean ("active", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (o), g_value_get_boolean (v)); },
              [] (GObject *o, GValue *v) { g_value_set_boolean (v, gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (o))); });
}

G_DEFINE_TYPE (GtkRadioMenuItem, gtk_radio_menu_item, GTK_TYPE_CHECK_MENU_ITEM)
static void gtk_radio_menu_item_init (GtkRadioMenuItem *) {}
static void gtk_radio_menu_item_class_init (GtkRadioMenuItemClass *) {}

static QObject *separator_item_create (GtkWidget *)
{
  auto *a = new QAction (nullptr);
  a->setSeparator (true);
  return a;
}

G_DEFINE_TYPE (GtkSeparatorMenuItem, gtk_separator_menu_item, GTK_TYPE_MENU_ITEM)
static void gtk_separator_menu_item_init (GtkSeparatorMenuItem *) {}
static void gtk_separator_menu_item_class_init (GtkSeparatorMenuItemClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), separator_item_create);
}

G_DEFINE_TYPE (GtkImageMenuItem, gtk_image_menu_item, GTK_TYPE_MENU_ITEM)
static void gtk_image_menu_item_init (GtkImageMenuItem *) {}
static void gtk_image_menu_item_class_init (GtkImageMenuItemClass *) {}

/* Default accelerators of stock items, applied when an accel group is given */
static const char *stock_accel (const gchar *stock_id)
{
  static const struct { const char *id, *accel; } map[] = {
    { "gtk-new", "<Control>n" }, { "gtk-open", "<Control>o" }, { "gtk-save", "<Control>s" },
    { "gtk-save-as", "<Shift><Control>s" }, { "gtk-close", "<Control>w" }, { "gtk-quit", "<Control>q" },
    { "gtk-undo", "<Control>z" }, { "gtk-redo", "<Shift><Control>z" }, { "gtk-cut", "<Control>x" },
    { "gtk-copy", "<Control>c" }, { "gtk-paste", "<Control>v" }, { "gtk-select-all", "<Control>a" },
    { "gtk-find", "<Control>f" }, { "gtk-print", "<Control>p" }, { "gtk-help", "F1" },
  };
  if (stock_id)
    for (auto &m : map) if (!strcmp (m.id, stock_id)) return m.accel;
  return nullptr;
}

/* Radio menu groups */
struct RadioMenuGroup { QActionGroup *qgroup; GSList *members = nullptr; int refs = 0; };

static void radio_menu_leave (gpointer data, GObject *where)
{
  auto *g = static_cast<RadioMenuGroup *> (data);
  g->members = g_slist_remove (g->members, where);
  if (--g->refs == 0) { delete g->qgroup; delete g; }
}

extern "C" {

GtkWidget *gtk_menu_item_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_MENU_ITEM, nullptr); }

GtkWidget *gtk_menu_item_new_with_label (const gchar *label)
{
  GtkWidget *w = gtk_menu_item_new ();
  gtk_menu_item_set_label (GTK_MENU_ITEM (w), label);
  return w;
}

GtkWidget *gtk_menu_item_new_with_mnemonic (const gchar *label)
{
  GtkWidget *w = gtk_menu_item_new ();
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (w), TRUE);
  gtk_menu_item_set_label (GTK_MENU_ITEM (w), label);
  return w;
}

static void menu_item_update_text (GtkMenuItem *item)
{
  auto *a = gtkqt_q<QAction> (GTK_WIDGET (item));
  if (!a) return;
  const char *label = (const char *) g_object_get_data (G_OBJECT (item), "gtkqt-label");
  if (g_object_get_data (G_OBJECT (item), "gtkqt-use-underline")) a->setText (gtkqt_mnemonic (label));
  else a->setText (gtkqt_str (label).replace ('&', QStringLiteral ("&&")));
}

void gtk_menu_item_set_label (GtkMenuItem *item, const gchar *label)
{
  g_object_set_data_full (G_OBJECT (item), "gtkqt-label", g_strdup (label), g_free);
  menu_item_update_text (item);
}

const gchar *gtk_menu_item_get_label (GtkMenuItem *item) { return (const gchar *) g_object_get_data (G_OBJECT (item), "gtkqt-label"); }

void gtk_menu_item_set_use_underline (GtkMenuItem *item, gboolean setting)
{
  g_object_set_data (G_OBJECT (item), "gtkqt-use-underline", GINT_TO_POINTER (setting));
  menu_item_update_text (item);
}

void gtk_menu_item_set_submenu (GtkMenuItem *item, GtkWidget *submenu)
{
  auto *a = gtkqt_q<QAction> (GTK_WIDGET (item));
  if (GtkWidget *old = (GtkWidget *) g_object_get_data (G_OBJECT (item), "gtkqt-submenu")) {
    if (a) a->setMenu (static_cast<QMenu *> (nullptr));
    g_object_set_data (G_OBJECT (item), "gtkqt-submenu", nullptr);
    g_object_unref (old);
  }
  if (!submenu) return;
  g_object_ref_sink (submenu);
  g_object_set_data (G_OBJECT (item), "gtkqt-submenu", submenu);
  if (auto *m = gtkqt_q<QMenu> (submenu)) {
    m->setTitle (a ? a->text () : QString ());
    if (a) a->setMenu (m);
  }
}

GtkWidget *gtk_menu_item_get_submenu (GtkMenuItem *item) { return (GtkWidget *) g_object_get_data (G_OBJECT (item), "gtkqt-submenu"); }
void gtk_menu_item_remove_submenu (GtkMenuItem *item) { gtk_menu_item_set_submenu (item, nullptr); }
void gtk_menu_item_set_right_justified (GtkMenuItem *, gboolean) {}
void gtk_menu_item_activate (GtkMenuItem *item) { if (auto *a = gtkqt_q<QAction> (GTK_WIDGET (item))) a->trigger (); }
void gtk_menu_item_set_accel_path (GtkMenuItem *, const gchar *) {}

GtkWidget *gtk_check_menu_item_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_CHECK_MENU_ITEM, nullptr); }

GtkWidget *gtk_check_menu_item_new_with_label (const gchar *label)
{
  GtkWidget *w = gtk_check_menu_item_new ();
  gtk_menu_item_set_label (GTK_MENU_ITEM (w), label);
  return w;
}

GtkWidget *gtk_check_menu_item_new_with_mnemonic (const gchar *label)
{
  GtkWidget *w = gtk_check_menu_item_new ();
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (w), TRUE);
  gtk_menu_item_set_label (GTK_MENU_ITEM (w), label);
  return w;
}

/* as GTK: a change activates the item, so "activate" handlers (item factory
   callbacks) run too, after the state has changed */
void gtk_check_menu_item_set_active (GtkCheckMenuItem *item, gboolean is_active)
{
  auto *a = gtkqt_q<QAction> (GTK_WIDGET (item));
  if (!a || a->isChecked () == !!is_active) return;
  a->setChecked (is_active);
  g_signal_emit_by_name (item, "activate");
}

gboolean gtk_check_menu_item_get_active (GtkCheckMenuItem *item)
{
  auto *a = gtkqt_q<QAction> (GTK_WIDGET (item));
  return a && a->isChecked ();
}

void gtk_check_menu_item_set_draw_as_radio (GtkCheckMenuItem *, gboolean) {}

static GtkWidget *radio_menu_item_new (GSList *group)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_RADIO_MENU_ITEM, nullptr);
  RadioMenuGroup *g = group ? static_cast<RadioMenuGroup *> (g_object_get_data (G_OBJECT (group->data), "gtkqt-radio-menu-group")) : nullptr;
  if (!g) {
    g = new RadioMenuGroup;
    g->qgroup = new QActionGroup (nullptr);
    g->qgroup->setExclusive (true);
  }
  bool first = !g->members;
  g->members = g_slist_prepend (g->members, w);
  g->refs++;
  g_object_set_data (G_OBJECT (w), "gtkqt-radio-menu-group", g);
  g_object_weak_ref (G_OBJECT (w), radio_menu_leave, g);
  if (auto *a = gtkqt_q<QAction> (w)) {
    g->qgroup->addAction (a);
    if (first) a->setChecked (true);
  }
  return w;
}

GtkWidget *gtk_radio_menu_item_new_with_label (GSList *group, const gchar *label)
{
  GtkWidget *w = radio_menu_item_new (group);
  gtk_menu_item_set_label (GTK_MENU_ITEM (w), label);
  return w;
}

GtkWidget *gtk_radio_menu_item_new_with_mnemonic (GSList *group, const gchar *label)
{
  GtkWidget *w = radio_menu_item_new (group);
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (w), TRUE);
  gtk_menu_item_set_label (GTK_MENU_ITEM (w), label);
  return w;
}

GSList *gtk_radio_menu_item_get_group (GtkRadioMenuItem *item)
{
  auto *g = static_cast<RadioMenuGroup *> (g_object_get_data (G_OBJECT (item), "gtkqt-radio-menu-group"));
  return g ? g->members : nullptr;
}

GtkWidget *gtk_separator_menu_item_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_SEPARATOR_MENU_ITEM, nullptr); }

GtkWidget *gtk_image_menu_item_new_from_stock (const gchar *stock_id, GtkAccelGroup *accel_group)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_IMAGE_MENU_ITEM, nullptr);
  QString label = gtkqt_stock_label (stock_id);
  if (auto *a = gtkqt_q<QAction> (w)) {
    a->setText (label.isEmpty () ? gtkqt_str (stock_id) : label);
    a->setIcon (gtkqt_icon (stock_id));
    if (accel_group) {
      if (const char *accel = stock_accel (stock_id)) a->setShortcut (gtkqt_accel_to_keyseq (accel));
      gtkqt_accel_group_add_action (accel_group, a);
    }
  }
  return w;
}

GtkWidget *gtk_image_menu_item_new_with_label (const gchar *label)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_IMAGE_MENU_ITEM, nullptr);
  gtk_menu_item_set_label (GTK_MENU_ITEM (w), label);
  return w;
}

GtkWidget *gtk_image_menu_item_new_with_mnemonic (const gchar *label)
{
  GtkWidget *w = gtk_image_menu_item_new_with_label (nullptr);
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (w), TRUE);
  gtk_menu_item_set_label (GTK_MENU_ITEM (w), label);
  return w;
}

void gtk_image_menu_item_set_image (GtkImageMenuItem *item, GtkWidget *image)
{
  auto *a = gtkqt_q<QAction> (GTK_WIDGET (item));
  auto *l = image ? gtkqt_q<QLabel> (image) : nullptr;
  if (a && l) a->setIcon (QIcon (l->pixmap (Qt::ReturnByValue)));
}

} /* extern "C" */

/* ======================================================================
 * GtkOptionMenu (GTK 2): a button showing the chosen item of a GtkMenu
 * ====================================================================== */
static QObject *option_menu_create (GtkWidget *w)
{
  auto *b = new QPushButton;
  (void) w;
  return b;
}

G_DEFINE_TYPE (GtkOptionMenu, gtk_option_menu, GTK_TYPE_BUTTON)
static void gtk_option_menu_init (GtkOptionMenu *) {}
static void gtk_option_menu_class_init (GtkOptionMenuClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), option_menu_create);
  g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}

/* GTK shows the chosen item's label inside the option menu, so programs read
 * GTK_BIN (option_menu)->child as a GtkLabel; that label is the button text */
static void option_menu_show (GtkWidget *om, const QString &action_text)
{
  QString plain = action_text;
  plain.remove (QRegularExpression (QStringLiteral ("&(?!&)"))).replace (QStringLiteral ("&&"), QStringLiteral ("&"));
  QByteArray t = plain.toUtf8 ();
  GtkWidget *child = GTK_BIN (om)->child;
  if (!child) {
    child = gtk_label_new (t.constData ());
    gtk_container_add (GTK_CONTAINER (om), child);
    gtk_widget_show (child);
  } else if (GTK_IS_LABEL (child)) {
    gtk_label_set_text (GTK_LABEL (child), t.constData ());
  } else if (auto *b = gtkqt_q<QPushButton> (om)) {
    b->setText (action_text);
  }
}

extern "C" {
GtkWidget *gtk_option_menu_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_OPTION_MENU, nullptr); }

void gtk_option_menu_set_menu (GtkOptionMenu *option_menu, GtkWidget *menu)
{
  auto *b = gtkqt_q<QPushButton> (GTK_WIDGET (option_menu));
  auto *m = gtkqt_q<QMenu> (menu);
  if (!b || !m) return;
  g_object_ref_sink (menu);
  g_object_set_data_full (G_OBJECT (option_menu), "gtkqt-menu", menu, [] (gpointer p) {
    gtk_widget_destroy ((GtkWidget *) p);
    g_object_unref (p);
  });
  b->setMenu (m);
  GtkWidget *om = GTK_WIDGET (option_menu);
  auto update = [om, b, m] (QAction *chosen) {
    if (chosen) option_menu_show (om, chosen->text ());
    b->setProperty ("_gtkqt_history", m->actions ().indexOf (chosen));
  };
  QList<QAction *> acts = m->actions ();
  for (QAction *a : acts) if (a->isVisible () && !a->isSeparator ()) { update (a); break; }
  gtkqt_connect (GTK_WIDGET (option_menu), m, &QMenu::triggered, [update] (GtkWidget *om, QAction *a) {
    update (a);
    g_signal_emit_by_name (om, "changed");
  });
}

GtkWidget *gtk_option_menu_get_menu (GtkOptionMenu *option_menu) { return (GtkWidget *) g_object_get_data (G_OBJECT (option_menu), "gtkqt-menu"); }

void gtk_option_menu_set_history (GtkOptionMenu *option_menu, guint index_)
{
  auto *b = gtkqt_q<QPushButton> (GTK_WIDGET (option_menu));
  if (b && b->menu () && (int) index_ < b->menu ()->actions ().size ()) {
    option_menu_show (GTK_WIDGET (option_menu), b->menu ()->actions ().at (index_)->text ());
    b->setProperty ("_gtkqt_history", (int) index_);
  }
}

gint gtk_option_menu_get_history (GtkOptionMenu *option_menu)
{
  auto *b = gtkqt_q<QPushButton> (GTK_WIDGET (option_menu));
  return b ? b->property ("_gtkqt_history").toInt () : -1;
}
}

/* ======================================================================
 * GtkPopover / GtkPopoverMenu / GtkPopoverMenuBar / GtkMenuButton
 * ====================================================================== */
static QObject *popover_create (GtkWidget *)
{
  auto *qw = new QFrame (nullptr, Qt::Popup);
  qw->setFrameShape (QFrame::StyledPanel);
  auto *l = new QVBoxLayout (qw);
  l->setContentsMargins (6, 6, 6, 6);
  return qw;
}

static QObject *popover_menu_create (GtkWidget *) { return new QMenu; }

G_DEFINE_TYPE (GtkPopover, gtk_popover, GTK_TYPE_BIN)
G_DEFINE_TYPE (GtkPopoverMenu, gtk_popover_menu, GTK_TYPE_POPOVER)
static void gtk_popover_init (GtkPopover *p) { GTK_WIDGET (p)->priv->visible = false; }
static void gtk_popover_class_init (GtkPopoverClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), popover_create);
  g_signal_new ("closed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}
static void gtk_popover_menu_init (GtkPopoverMenu *) {}
static void gtk_popover_menu_class_init (GtkPopoverMenuClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), popover_menu_create); }

static QObject *popover_menubar_create (GtkWidget *) { return new QMenuBar; }
G_DEFINE_TYPE (GtkPopoverMenuBar, gtk_popover_menu_bar, GTK_TYPE_WIDGET)
static void gtk_popover_menu_bar_init (GtkPopoverMenuBar *) {}
static void gtk_popover_menu_bar_class_init (GtkPopoverMenuBarClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), popover_menubar_create); }

static QObject *menu_button_create (GtkWidget *w)
{
  auto *b = new QToolButton;
  b->setPopupMode (QToolButton::InstantPopup);
  b->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);
  b->setText (QStringLiteral ("▾"));
  gtkqt_connect (w, b, &QToolButton::clicked, [] (GtkWidget *mb, bool) {
    if (auto *pop = (GtkWidget *) g_object_get_data (G_OBJECT (mb), "gtkqt-popover")) gtk_popover_popup (GTK_POPOVER (pop));
  });
  return b;
}

G_DEFINE_TYPE (GtkMenuButton, gtk_menu_button, GTK_TYPE_WIDGET)
static void gtk_menu_button_init (GtkMenuButton *) {}
static void gtk_menu_button_class_init (GtkMenuButtonClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), menu_button_create);
  gtkqt_prop (klass, g_param_spec_object ("menu-model", nullptr, nullptr, G_TYPE_MENU_MODEL, RW),
              [] (GObject *o, const GValue *v) { gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (o), (GMenuModel *) g_value_get_object (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("icon-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("label", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_menu_button_set_label (GTK_MENU_BUTTON (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("popover", nullptr, nullptr, GTK_TYPE_POPOVER, RW),
              [] (GObject *o, const GValue *v) { gtk_menu_button_set_popover (GTK_MENU_BUTTON (o), (GtkWidget *) g_value_get_object (v)); }, nullptr);
}

extern "C" {

GtkWidget *gtk_popover_new (GtkWidget *relative_to)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_POPOVER, nullptr);
  gtk_popover_set_relative_to (GTK_POPOVER (w), relative_to);
  return w;
}

void gtk_popover_set_relative_to (GtkPopover *popover, GtkWidget *relative_to)
{
  g_object_set_data (G_OBJECT (popover), "gtkqt-relative-to", relative_to);
}

void gtk_popover_set_child (GtkPopover *popover, GtkWidget *child)
{
  if (GtkWidget *old = GTK_BIN (popover)->child) gtk_container_remove (GTK_CONTAINER (popover), old);
  if (child) gtk_container_add (GTK_CONTAINER (popover), child);
}

void gtk_popover_popup (GtkPopover *popover)
{
  GtkWidget *w = GTK_WIDGET (popover);
  auto *rel = (GtkWidget *) g_object_get_data (G_OBJECT (popover), "gtkqt-relative-to");
  if (!rel) rel = w->parent;
  QWidget *anchor = rel ? gtkqt_qw (rel) : nullptr;
  QPoint pos = anchor ? anchor->mapToGlobal (QPoint (0, anchor->height ())) : QCursor::pos ();
  QObject *o = gtkqt_obj (w);
  if (auto *m = qobject_cast<QMenu *> (o)) {
    m->popup (pos);
  } else if (QWidget *qw = qobject_cast<QWidget *> (o)) {
    w->priv->visible = true;
    qw->adjustSize ();
    qw->move (pos);
    qw->show ();
  }
}

void gtk_popover_popdown (GtkPopover *popover)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (popover))) qw->hide ();
  GTK_WIDGET (popover)->priv->visible = false;
  g_signal_emit_by_name (popover, "closed");
}

GtkWidget *gtk_popover_menu_new_from_model (GMenuModel *model)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_POPOVER_MENU, nullptr);
  if (auto *m = gtkqt_q<QMenu> (w)) {
    m->setProperty ("_gtkqt_context", QVariant::fromValue ((void *) w));
    bool sep = false;
    if (model) fill_from_model (m, model, w, &sep);
    QObject::connect (m, &QMenu::aboutToShow, m, [m] { refresh_model_actions (m); });
  }
  return w;
}

GtkWidget *gtk_popover_menu_bar_new_from_model (GMenuModel *model)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_POPOVER_MENU_BAR, nullptr);
  if (QWidget *mb = gtkqt_qw (w)) gtkqt_menubar_fill_from_model (mb, model, w);
  return w;
}

GtkWidget *gtk_menu_button_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_MENU_BUTTON, nullptr); }

void gtk_menu_button_set_menu_model (GtkMenuButton *menu_button, GMenuModel *menu_model)
{
  auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (menu_button));
  if (!b) return;
  if (QMenu *old = b->menu ()) { b->setMenu (nullptr); old->deleteLater (); }
  if (menu_model) b->setMenu (gtkqt_menu_from_model (menu_model, GTK_WIDGET (menu_button), b));
}

void gtk_menu_button_set_popup (GtkMenuButton *menu_button, GtkWidget *menu)
{
  auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (menu_button));
  if (!b || !menu) return;
  g_object_ref_sink (menu);
  g_object_set_data_full (G_OBJECT (menu_button), "gtkqt-popup-menu", menu, g_object_unref);
  b->setMenu (gtkqt_q<QMenu> (menu));
}

void gtk_menu_button_set_popover (GtkMenuButton *menu_button, GtkWidget *popover)
{
  auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (menu_button));
  if (!b || !popover) return;
  g_object_ref_sink (popover);
  g_object_set_data_full (G_OBJECT (menu_button), "gtkqt-popover", popover, g_object_unref);
  gtk_popover_set_relative_to (GTK_POPOVER (popover), GTK_WIDGET (menu_button));
  if (auto *m = gtkqt_q<QMenu> (popover)) b->setMenu (m);
  else b->setPopupMode (QToolButton::DelayedPopup);
}

void gtk_menu_button_set_icon_name (GtkMenuButton *menu_button, const char *icon_name)
{
  if (auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (menu_button))) {
    QIcon icon = gtkqt_icon (icon_name);
    if (!icon.isNull ()) { b->setIcon (icon); b->setToolButtonStyle (Qt::ToolButtonIconOnly); }
    else b->setText (QStringLiteral ("☰"));
  }
}

void gtk_menu_button_set_label (GtkMenuButton *menu_button, const char *label)
{
  if (auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (menu_button))) b->setText (gtkqt_mnemonic (label) + QStringLiteral (" ▾"));
}

} /* extern "C" */

/* ======================================================================
 * GtkToolbar and tool items (GTK 2/3)
 * ====================================================================== */
static QObject *toolbar_create (GtkWidget *)
{
  auto *t = new QToolBar;
  t->setToolButtonStyle (Qt::ToolButtonTextUnderIcon);
  t->setMovable (false);
  return t;
}

static void toolbar_place (GtkContainer *c, GtkWidget *child)
{
  auto *t = gtkqt_q<QToolBar> (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!t || !cw) return;
  if (auto *tb = qobject_cast<QToolButton *> (cw)) tb->setToolButtonStyle (t->toolButtonStyle ());
  QList<QAction *> acts = t->actions ();
  int pos = child->priv->grid_left;
  QAction *a = pos >= 0 && pos < acts.size () ? t->insertWidget (acts.at (pos), cw) : t->addWidget (cw);
  cw->setProperty ("_gtkqt_vis_action", QVariant::fromValue ((QObject *) a));
  cw->show ();
  a->setVisible (child->priv->visible);
}

static void toolbar_unplace (GtkContainer *c, GtkWidget *child)
{
  auto *t = gtkqt_q<QToolBar> (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!t || !cw) return;
  if (auto *a = qobject_cast<QAction *> (cw->property ("_gtkqt_vis_action").value<QObject *> ())) t->removeAction (a);
  cw->setProperty ("_gtkqt_vis_action", QVariant ());
  cw->hide ();
  cw->setParent (nullptr);
}

static QObject *tool_item_create (GtkWidget *)
{
  auto *qw = new QWidget;
  auto *l = new QHBoxLayout (qw);
  l->setContentsMargins (0, 0, 0, 0);
  return qw;
}

static QObject *tool_button_create (GtkWidget *w)
{
  auto *b = new QToolButton;
  b->setAutoRaise (true);
  gtkqt_connect (w, b, &QToolButton::clicked, [] (GtkWidget *tw, bool) {
    g_signal_emit_by_name (tw, "clicked");
    if (const char *action = (const char *) g_object_get_data (G_OBJECT (tw), "gtkqt-action-name"))
      gtkqt_activate_action (tw, action, (GVariant *) g_object_get_data (G_OBJECT (tw), "gtkqt-action-target"));
  });
  return b;
}

static QObject *toggle_tool_button_create (GtkWidget *w)
{
  auto *b = static_cast<QToolButton *> (tool_button_create (w));
  b->setCheckable (true);
  gtkqt_connect (w, b, &QToolButton::toggled, [] (GtkWidget *tw, bool) { g_signal_emit_by_name (tw, "toggled"); });
  return b;
}

static QObject *separator_tool_create (GtkWidget *)
{
  auto *f = new QFrame;
  f->setFrameShape (QFrame::VLine);
  f->setFrameShadow (QFrame::Sunken);
  return f;
}

G_DEFINE_TYPE (GtkToolbar, gtk_toolbar, GTK_TYPE_CONTAINER)
G_DEFINE_TYPE (GtkToolItem, gtk_tool_item, GTK_TYPE_BIN)
G_DEFINE_TYPE (GtkToolButton, gtk_tool_button, GTK_TYPE_TOOL_ITEM)
G_DEFINE_TYPE (GtkToggleToolButton, gtk_toggle_tool_button, GTK_TYPE_TOOL_BUTTON)
G_DEFINE_TYPE (GtkSeparatorToolItem, gtk_separator_tool_item, GTK_TYPE_TOOL_ITEM)

static void gtk_toolbar_init (GtkToolbar *) {}
static void gtk_toolbar_class_init (GtkToolbarClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), toolbar_create);
  gtkqt_set_place (GTK_CONTAINER_CLASS (klass), toolbar_place, toolbar_unplace);
  GTK_CONTAINER_CLASS (klass)->add = [] (GtkContainer *c, GtkWidget *child) { child->priv->grid_left = -1; gtkqt_container_adopt (c, child); };
}
static void gtk_tool_item_init (GtkToolItem *) {}
static void gtk_tool_item_class_init (GtkToolItemClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), tool_item_create); }
static void gtk_tool_button_init (GtkToolButton *) {}
static void gtk_tool_button_class_init (GtkToolButtonClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), tool_button_create);
  g_signal_new ("clicked", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_string ("label", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_tool_button_set_label (GTK_TOOL_BUTTON (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("icon-name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (o), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("stock-id", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) {
                const char *id = g_value_get_string (v);
                gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (o), id);
                gtk_tool_button_set_label (GTK_TOOL_BUTTON (o), gtkqt_stock_label (id).remove ('&').toUtf8 ().constData ());
              }, nullptr);
}
static void gtk_toggle_tool_button_init (GtkToggleToolButton *) {}
static void gtk_toggle_tool_button_class_init (GtkToggleToolButtonClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), toggle_tool_button_create);
  g_signal_new ("toggled", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}
static void gtk_separator_tool_item_init (GtkSeparatorToolItem *) {}
static void gtk_separator_tool_item_class_init (GtkSeparatorToolItemClass *klass) { gtkqt_set_create (GTK_WIDGET_CLASS (klass), separator_tool_create); }

extern "C" {

GtkWidget *gtk_toolbar_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_TOOLBAR, nullptr); }

void gtk_toolbar_insert (GtkToolbar *toolbar, GtkToolItem *item, gint pos)
{
  GtkWidget *w = GTK_WIDGET (item);
  if (w->parent) return;
  w->priv->grid_left = pos;
  gtkqt_container_adopt (GTK_CONTAINER (toolbar), w);
}

void gtk_toolbar_set_style (GtkToolbar *toolbar, GtkToolbarStyle style)
{
  auto *t = gtkqt_q<QToolBar> (GTK_WIDGET (toolbar));
  if (!t) return;
  static const Qt::ToolButtonStyle map[] = { Qt::ToolButtonIconOnly, Qt::ToolButtonTextOnly, Qt::ToolButtonTextUnderIcon, Qt::ToolButtonTextBesideIcon };
  t->setToolButtonStyle (map[style & 3]);
  for (QToolButton *b : t->findChildren<QToolButton *> ()) b->setToolButtonStyle (map[style & 3]);
}

void gtk_toolbar_set_icon_size (GtkToolbar *toolbar, GtkIconSize icon_size)
{
  int px = gtkqt_icon_size_px (icon_size);
  if (auto *t = gtkqt_q<QToolBar> (GTK_WIDGET (toolbar))) t->setIconSize (QSize (px, px));
}

gint gtk_toolbar_get_n_items (GtkToolbar *toolbar) { return g_list_length (GTK_WIDGET (toolbar)->priv->children); }

GtkToolItem *gtk_tool_item_new (void) { return (GtkToolItem *) g_object_new (GTK_TYPE_TOOL_ITEM, nullptr); }

GtkToolItem *gtk_tool_button_new (GtkWidget *icon_widget, const gchar *label)
{
  auto *item = (GtkToolItem *) g_object_new (GTK_TYPE_TOOL_BUTTON, nullptr);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), label);
  if (icon_widget) {
    if (auto *l = gtkqt_q<QLabel> (icon_widget))
      if (auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (item))) b->setIcon (QIcon (l->pixmap (Qt::ReturnByValue)));
    g_object_ref_sink (icon_widget);
    g_object_set_data_full (G_OBJECT (item), "gtkqt-icon-widget", icon_widget, g_object_unref);
  }
  return item;
}

GtkToolItem *gtk_tool_button_new_from_stock (const gchar *stock_id)
{
  auto *item = (GtkToolItem *) g_object_new (GTK_TYPE_TOOL_BUTTON, "stock-id", stock_id, nullptr);
  return item;
}

void gtk_tool_button_set_icon_name (GtkToolButton *button, const gchar *icon_name)
{
  if (auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (button))) b->setIcon (gtkqt_icon (icon_name));
}

void gtk_tool_button_set_label (GtkToolButton *button, const gchar *label)
{
  if (auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (button))) b->setText (gtkqt_mnemonic (label));
}

GtkToolItem *gtk_toggle_tool_button_new (void) { return (GtkToolItem *) g_object_new (GTK_TYPE_TOGGLE_TOOL_BUTTON, nullptr); }

GtkToolItem *gtk_toggle_tool_button_new_from_stock (const gchar *stock_id)
{
  return (GtkToolItem *) g_object_new (GTK_TYPE_TOGGLE_TOOL_BUTTON, "stock-id", stock_id, nullptr);
}

void gtk_toggle_tool_button_set_active (GtkToggleToolButton *button, gboolean is_active)
{
  if (auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (button))) b->setChecked (is_active);
}

gboolean gtk_toggle_tool_button_get_active (GtkToggleToolButton *button)
{
  auto *b = gtkqt_q<QToolButton> (GTK_WIDGET (button));
  return b && b->isChecked ();
}

GtkToolItem *gtk_separator_tool_item_new (void) { return (GtkToolItem *) g_object_new (GTK_TYPE_SEPARATOR_TOOL_ITEM, nullptr); }
void gtk_tool_item_set_tooltip_text (GtkToolItem *tool_item, const gchar *text) { gtk_widget_set_tooltip_text (GTK_WIDGET (tool_item), text); }

void gtk_tool_item_set_expand (GtkToolItem *tool_item, gboolean expand)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (tool_item)))
    qw->setSizePolicy (expand ? QSizePolicy::Expanding : QSizePolicy::Preferred, qw->sizePolicy ().verticalPolicy ());
}

void gtk_tool_item_set_is_important (GtkToolItem *, gboolean) {}

} /* extern "C" */
