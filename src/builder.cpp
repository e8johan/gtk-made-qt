/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkBuilder: loads GtkBuilder / Glade XML (GTK 2.12+, 3 and 4 dialects).
 * Objects are created with g_object_new() from their GType name, so any
 * class with real GObject properties works, including the application's
 * own subclasses. Signal handlers are looked up by symbol name in the
 * executable (link with -rdynamic, which the pkg-config files arrange).
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QXmlStreamReader>
#include <QFile>
#include <gmodule.h>

/* ---------- a tiny element tree ---------- */
struct Node {
  QString tag, text;
  QMap<QString, QString> attrs;
  QList<Node *> kids;
  ~Node () { qDeleteAll (kids); }
  QString attr (const char *n) const { return attrs.value (QString::fromLatin1 (n)); }
};

static Node *parse_xml (const QByteArray &data, GError **error)
{
  QXmlStreamReader r (data);
  auto *root = new Node;
  QList<Node *> stack { root };
  while (!r.atEnd ()) {
    r.readNext ();
    if (r.isStartElement ()) {
      auto *n = new Node;
      n->tag = r.name ().toString ();
      for (const QXmlStreamAttribute &a : r.attributes ()) n->attrs.insert (a.name ().toString (), a.value ().toString ());
      stack.last ()->kids << n;
      stack << n;
    } else if (r.isEndElement ()) {
      stack.removeLast ();
    } else if (r.isCharacters () && stack.size () > 1) {
      stack.last ()->text += r.text ();
    }
  }
  if (r.hasError ()) {
    g_set_error (error, GTK_BUILDER_ERROR, GTK_BUILDER_ERROR_INVALID_TAG, "%s at line %lld",
                 r.errorString ().toUtf8 ().constData (), (long long) r.lineNumber ());
    delete root;
    return nullptr;
  }
  return root;
}

/* ---------- GtkBuilder object ---------- */
struct SignalInfo { GObject *object; QByteArray signal, handler, connect_object; bool after, swapped; };
struct DeferredProp { GObject *object; QByteArray name; QString id; };
struct BuilderData {
  QMap<QString, GObject *> objects;
  QList<GObject *> order;
  QList<SignalInfo> signals_;
  QList<DeferredProp> deferred;
  QString domain;
};

struct _GtkBuilder { GObject parent_instance; gpointer priv; };
typedef struct { GObjectClass parent_class; } GtkBuilderClass;
static BuilderData *bd (GtkBuilder *b) { return static_cast<BuilderData *> (b->priv); }

G_DEFINE_TYPE (GtkBuilder, gtk_builder, G_TYPE_OBJECT)
static void gtk_builder_init (GtkBuilder *b) { b->priv = new BuilderData; }
static void builder_finalize (GObject *o)
{
  BuilderData *d = bd ((GtkBuilder *) o);
  for (GObject *obj : d->order) g_object_unref (obj);
  delete d;
  G_OBJECT_CLASS (gtk_builder_parent_class)->finalize (o);
}
static void gtk_builder_class_init (GtkBuilderClass *klass) { G_OBJECT_CLASS (klass)->finalize = builder_finalize; }

extern "C" GQuark gtk_builder_error_quark (void) { return g_quark_from_static_string ("gtk-builder-error-quark"); }

/* the library's GType functions (some are declared only for one API level) */
#define TYPE_FN(f) extern "C" GType f (void);
#include "types.inc"
#undef TYPE_FN

static GType resolve_type (const QString &name)
{
  QByteArray n = name.toUtf8 ();
  if (GType t = g_type_from_name (n.constData ())) return t;
  /* Not registered yet: call its get_type function (GTK's own mangling rule). */
  QByteArray sym;
  for (int i = 0; i < n.size (); i++) {
    char c = n[i];
    bool up = g_ascii_isupper (c);
    if ((up && i > 0 && !g_ascii_isupper (n[i - 1])) ||
        (i > 2 && up && g_ascii_isupper (n[i - 1]) && g_ascii_isupper (n[i - 2]) && i + 1 < n.size () && !g_ascii_isupper (n[i + 1])))
      sym += '_';
    sym += g_ascii_tolower (c);
  }
  sym += "_get_type";
  /* our own types: from the table (the library's symbols are not exported) */
  static const QHash<QByteArray, GType (*) (void)> own = [] {
    QHash<QByteArray, GType (*) (void)> h;
#define TYPE_FN(f) h.insert (#f, &f);
#include "types.inc"
#undef TYPE_FN
    return h;
  } ();
  if (auto *f = own.value (sym)) return f ();
  /* the program's own types */
  static GModule *self = g_module_open (nullptr, (GModuleFlags) 0);
  gpointer fn = nullptr;
  if (self && g_module_symbol (self, sym.constData (), &fn) && fn) return ((GType (*) (void)) fn) ();
  return G_TYPE_INVALID;
}

static bool string_to_value (GtkBuilder *b, GType t, const QString &s, GValue *v, GError **error)
{
  g_value_init (v, t);
  QByteArray u = s.trimmed ().toUtf8 ();
  auto as_bool = [&] { QString l = s.trimmed ().toLower (); return l == "true" || l == "yes" || l == "1" || l == "t" || l == "y"; };
  switch (G_TYPE_FUNDAMENTAL (t)) {
  case G_TYPE_STRING: g_value_set_string (v, s.toUtf8 ().constData ()); return true;
  case G_TYPE_BOOLEAN: g_value_set_boolean (v, as_bool ()); return true;
  case G_TYPE_INT: g_value_set_int (v, u.toInt ()); return true;
  case G_TYPE_UINT: g_value_set_uint (v, u.toUInt ()); return true;
  case G_TYPE_LONG: g_value_set_long (v, u.toLong ()); return true;
  case G_TYPE_ULONG: g_value_set_ulong (v, u.toULong ()); return true;
  case G_TYPE_INT64: g_value_set_int64 (v, u.toLongLong ()); return true;
  case G_TYPE_UINT64: g_value_set_uint64 (v, u.toULongLong ()); return true;
  case G_TYPE_FLOAT: g_value_set_float (v, u.toFloat ()); return true;
  case G_TYPE_DOUBLE: g_value_set_double (v, u.toDouble ()); return true;
  case G_TYPE_ENUM: {
    GEnumClass *ec = (GEnumClass *) g_type_class_ref (t);
    GEnumValue *ev = g_enum_get_value_by_nick (ec, u.constData ());
    if (!ev) ev = g_enum_get_value_by_name (ec, u.constData ());
    if (!ev) ev = g_enum_get_value_by_nick (ec, QByteArray (u).replace ('_', '-').toLower ().constData ());
    bool ok = false;
    int num = u.toInt (&ok);
    g_value_set_enum (v, ev ? ev->value : num);
    g_type_class_unref (ec);
    if (!ev && !ok) { g_set_error (error, GTK_BUILDER_ERROR, GTK_BUILDER_ERROR_INVALID_VALUE, "Invalid enum value '%s'", u.constData ()); return false; }
    return true;
  }
  case G_TYPE_FLAGS: {
    GFlagsClass *fc = (GFlagsClass *) g_type_class_ref (t);
    guint flags = 0;
    for (const QByteArray &part : u.split ('|')) {
      QByteArray p = part.trimmed ();
      GFlagsValue *fv = g_flags_get_value_by_nick (fc, p.constData ());
      if (!fv) fv = g_flags_get_value_by_name (fc, p.constData ());
      flags |= fv ? fv->value : p.toUInt ();
    }
    g_value_set_flags (v, flags);
    g_type_class_unref (fc);
    return true;
  }
  case G_TYPE_BOXED:
    if (t == GDK_TYPE_RGBA) { GdkRGBA c; gdk_rgba_parse (&c, u.constData ()); g_value_set_boxed (v, &c); return true; }
    if (t == G_TYPE_STRV) {
      QList<QByteArray> parts = u.split ('\n');
      std::vector<gchar *> strs;
      for (QByteArray &p : parts) if (!p.trimmed ().isEmpty ()) strs.push_back (p.trimmed ().data ());
      strs.push_back (nullptr);
      g_value_set_boxed (v, strs.data ());
      return true;
    }
    break;
  case G_TYPE_OBJECT:
  case G_TYPE_INTERFACE:
    if (GObject *o = bd (b)->objects.value (s.trimmed ())) { g_value_set_object (v, o); return true; }
    break;
  case G_TYPE_VARIANT:
    if (GVariant *var = g_variant_parse (nullptr, u.constData (), nullptr, nullptr, nullptr)) { g_value_take_variant (v, var); return true; }
    break;
  default:
    break;
  }
  g_value_unset (v);
  return false;
}

static void set_props (GtkBuilder *b, GObject *obj, const QList<Node *> &props)
{
  for (Node *p : props) {
    QByteArray name = p->attr ("name").toUtf8 ().replace ('_', '-');
    GParamSpec *ps = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), name.constData ());
    if (!ps) {
      if (name != "can-focus" && name != "receives-default" && name != "visible")
        g_message ("GtkBuilder: %s has no property '%s' (ignored)", G_OBJECT_TYPE_NAME (obj), name.constData ());
      continue;
    }
    if (!(ps->flags & G_PARAM_WRITABLE)) continue;
    /* an inline <object> as property value */
    GValue v = G_VALUE_INIT;
    GError *err = nullptr;
    if ((G_TYPE_FUNDAMENTAL (ps->value_type) == G_TYPE_OBJECT || G_TYPE_FUNDAMENTAL (ps->value_type) == G_TYPE_INTERFACE) &&
        !bd (b)->objects.contains (p->text.trimmed ())) {
      bd (b)->deferred << DeferredProp { obj, name, p->text.trimmed () };
      continue;
    }
    if (string_to_value (b, ps->value_type, p->text, &v, &err)) {
      g_object_set_property (obj, name.constData (), &v);
      g_value_unset (&v);
    } else if (err) {
      g_warning ("GtkBuilder: %s", err->message);
      g_error_free (err);
    }
  }
}

static GObject *build_object (GtkBuilder *b, Node *n, GObject *parent, GError **error);

static GtkWidget *internal_child (GObject *parent, const QString &name)
{
  if (GTK_IS_DIALOG (parent)) {
    if (name == "vbox" || name == "content_area") return GTK_DIALOG (parent)->vbox;
    if (name == "action_area") return GTK_DIALOG (parent)->action_area;
  }
  if (GTK_IS_COMBO_BOX (parent) && name == "entry") return gtk_bin_get_child (GTK_BIN (parent));
  return nullptr;
}

static void apply_packing (GtkWidget *parent, GtkWidget *child, Node *packing, Node *layout)
{
  GtkWidgetPrivate *p = child->priv;
  for (Node *src : { packing, layout }) {
    if (!src) continue;
    for (Node *prop : src->kids) {
      if (prop->tag != "property") continue;
      QString name = prop->attr ("name").replace ('_', '-'), v = prop->text.trimmed ();
      bool b = v.toLower () == "true" || v == "1" || v.toLower () == "yes";
      if (name == "expand") p->pack_expand = b;
      else if (name == "fill") p->pack_fill = b;
      else if (name == "padding") p->pack_padding = v.toUInt ();
      else if (name == "pack-type") p->pack_type = v.contains ("end", Qt::CaseInsensitive) ? GTK_PACK_END : GTK_PACK_START;
      else if (name == "left-attach" || name == "column") p->grid_left = v.toInt ();
      else if (name == "top-attach" || name == "row") p->grid_top = v.toInt ();
      else if (name == "width" || name == "column-span") p->grid_width = qMax (1, v.toInt ());
      else if (name == "height" || name == "row-span") p->grid_height = qMax (1, v.toInt ());
      else if (name == "right-attach") p->grid_width = qMax (1, v.toInt () - p->grid_left);
      else if (name == "bottom-attach") p->grid_height = qMax (1, v.toInt () - p->grid_top);
      else if (name == "tab-label" || name == "title") p->tab_label = v;
      else if (name == "name") p->stack_name = v;
      else if (name == "x-options" || name == "y-options") {
        int f = (v.contains ("expand") ? GTK_EXPAND : 0) | (v.contains ("fill") ? GTK_FILL : 0) | (v.contains ("shrink") ? GTK_SHRINK : 0);
        (name == "x-options" ? p->xopts : p->yopts) = (GtkAttachOptions) f;
      }
    }
  }
  (void) parent;
}

static void add_child (GtkBuilder *b, GObject *parent, GObject *child, const QString &type, Node *packing, Node *layout, Node *child_node)
{
  if (GTK_IS_WIDGET (child)) apply_packing (GTK_IS_WIDGET (parent) ? GTK_WIDGET (parent) : nullptr, GTK_WIDGET (child), packing, layout);
  GtkWidget *cw = GTK_IS_WIDGET (child) ? GTK_WIDGET (child) : nullptr;

  if (GTK_IS_TREE_VIEW (parent) && GTK_IS_TREE_VIEW_COLUMN (child)) { gtk_tree_view_append_column (GTK_TREE_VIEW (parent), GTK_TREE_VIEW_COLUMN (child)); return; }
  if ((GTK_IS_TREE_VIEW_COLUMN (parent) || GTK_IS_COMBO_BOX (parent)) && GTK_IS_CELL_RENDERER (child)) {
    gtk_cell_layout_pack_start ((GtkCellLayout *) parent, GTK_CELL_RENDERER (child), TRUE);
    for (Node *k : child_node->kids)
      if (k->tag == "attributes")
        for (Node *a : k->kids)
          gtk_cell_layout_add_attribute ((GtkCellLayout *) parent, GTK_CELL_RENDERER (child), a->attr ("name").toUtf8 ().constData (), a->text.trimmed ().toInt ());
    return;
  }
  if (!cw) return;
  if (GTK_IS_WINDOW (parent) && type == "titlebar") { gtk_window_set_titlebar (GTK_WINDOW (parent), cw); return; }
  if (GTK_IS_HEADER_BAR (parent)) {
    if (type == "title") gtk_header_bar_set_title_widget (GTK_HEADER_BAR (parent), cw);
    else if (type == "end" || cw->priv->pack_type == GTK_PACK_END) gtk_header_bar_pack_end (GTK_HEADER_BAR (parent), cw);
    else gtk_header_bar_pack_start (GTK_HEADER_BAR (parent), cw);
    return;
  }
  if (GTK_IS_NOTEBOOK (parent)) {
    if (type == "tab") {
      /* the tab label for the most recently added page */
      GList *last = g_list_last (GTK_WIDGET (parent)->priv->children);
      if (last && GTK_IS_LABEL (cw)) gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (parent), (GtkWidget *) last->data, gtk_label_get_text (GTK_LABEL (cw)));
      g_object_ref_sink (cw);
      return;
    }
    gtk_notebook_append_page (GTK_NOTEBOOK (parent), cw, nullptr);
    if (!cw->priv->tab_label.isEmpty ()) gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (parent), cw, cw->priv->tab_label.toUtf8 ().constData ());
    return;
  }
  if (GTK_IS_MENU_ITEM (parent) && (type == "submenu" || GTK_IS_MENU (cw))) { gtk_menu_item_set_submenu (GTK_MENU_ITEM (parent), cw); return; }
  if (GTK_IS_MENU_SHELL (parent)) { gtk_menu_shell_append (GTK_MENU_SHELL (parent), cw); return; }
  if (GTK_IS_TOOLBAR (parent)) { gtk_toolbar_insert (GTK_TOOLBAR (parent), GTK_TOOL_ITEM (cw), -1); return; }
  if (GTK_IS_STACK (parent)) {
    QByteArray name = cw->priv->stack_name.toUtf8 (), title = cw->priv->tab_label.toUtf8 ();
    gtk_stack_add_titled (GTK_STACK (parent), cw, name.isEmpty () ? nullptr : name.constData (), title.isEmpty () ? nullptr : title.constData ());
    return;
  }
  if (GTK_IS_PANED (parent)) {
    if (!gtk_paned_get_start_child (GTK_PANED (parent))) gtk_paned_pack1 (GTK_PANED (parent), cw, cw->priv->pack_expand, TRUE);
    else gtk_paned_pack2 (GTK_PANED (parent), cw, TRUE, TRUE);
    return;
  }
  if (GTK_IS_GRID (parent) || GTK_IS_TABLE (parent)) { gtkqt_container_adopt (GTK_CONTAINER (parent), cw); return; }
  if (GTK_IS_BOX (parent)) {
    if (gtkqt_api >= 4 && !packing) gtk_box_append (GTK_BOX (parent), cw);
    else gtk_box_pack_start (GTK_BOX (parent), cw, packing ? cw->priv->pack_expand : TRUE, cw->priv->pack_fill, cw->priv->pack_padding);
    return;
  }
  if (GTK_IS_CONTAINER (parent)) { gtk_container_add (GTK_CONTAINER (parent), cw); return; }
  if (GTK_IS_WIDGET (parent)) { gtk_widget_set_parent (cw, GTK_WIDGET (parent)); return; }
  (void) b;
}

static GObject *build_store (GtkBuilder *b, Node *n, GType t)
{
  QVector<GType> types;
  for (Node *k : n->kids)
    if (k->tag == "columns")
      for (Node *c : k->kids) types << resolve_type (c->attr ("type"));
  GtkTreeModel *m = t == GTK_TYPE_TREE_STORE ? (GtkTreeModel *) gtk_tree_store_newv (types.size (), types.data ())
                                            : (GtkTreeModel *) gtk_list_store_newv (types.size (), types.data ());
  for (Node *k : n->kids)
    if (k->tag == "data")
      for (Node *row : k->kids) {
        GtkTreeIter it;
        if (t == GTK_TYPE_TREE_STORE) gtk_tree_store_append ((GtkTreeStore *) m, &it, nullptr);
        else gtk_list_store_append ((GtkListStore *) m, &it);
        for (Node *col : row->kids) {
          int id = col->attr ("id").toInt ();
          if (id < 0 || id >= types.size ()) continue;
          GValue v = G_VALUE_INIT;
          if (string_to_value (b, types[id], col->text, &v, nullptr)) {
            gtk_list_store_set_value ((GtkListStore *) m, &it, id, &v);
            g_value_unset (&v);
          }
        }
      }
  return G_OBJECT (m);
}

static GMenu *build_menu (Node *n);

static void menu_item_attrs (GMenuItem *item, Node *n)
{
  for (Node *a : n->kids) {
    if (a->tag == "attribute") {
      QByteArray name = a->attr ("name").toUtf8 (), type = a->attr ("type").toUtf8 ();
      GVariant *v = type.isEmpty () ? g_variant_new_string (a->text.toUtf8 ().constData ())
                                    : g_variant_parse (G_VARIANT_TYPE (type.constData ()), a->text.toUtf8 ().constData (), nullptr, nullptr, nullptr);
      if (v) g_menu_item_set_attribute_value (item, name.constData (), v);
    } else if (a->tag == "link") {
      GMenu *sub = build_menu (a);
      g_menu_item_set_link (item, a->attr ("name").toUtf8 ().constData (), G_MENU_MODEL (sub));
      g_object_unref (sub);
    }
  }
}

static GMenu *build_menu (Node *n)
{
  GMenu *menu = g_menu_new ();
  for (Node *k : n->kids) {
    if (k->tag == "item") {
      GMenuItem *item = g_menu_item_new (nullptr, nullptr);
      menu_item_attrs (item, k);
      g_menu_append_item (menu, item);
      g_object_unref (item);
    } else if (k->tag == "section" || k->tag == "submenu") {
      GMenu *sub = build_menu (k);
      GMenuItem *item = k->tag == "section" ? g_menu_item_new_section (nullptr, G_MENU_MODEL (sub))
                                            : g_menu_item_new_submenu (nullptr, G_MENU_MODEL (sub));
      menu_item_attrs (item, k);   /* label etc. are attributes of the section/submenu node */
      g_menu_append_item (menu, item);
      g_object_unref (item);
      g_object_unref (sub);
    }
  }
  return menu;
}

static void register_object (GtkBuilder *b, const QString &id, GObject *o)
{
  /* the builder keeps one reference to everything it hands out */
  if (G_IS_INITIALLY_UNOWNED (o)) g_object_ref_sink (o);
  bd (b)->order << o;
  if (!id.isEmpty ()) {
    bd (b)->objects.insert (id, o);
    gtk_buildable_set_name ((GtkBuildable *) o, id.toUtf8 ().constData ());
  }
}

static GObject *build_object (GtkBuilder *b, Node *n, GObject *existing, GError **error)
{
  QString id = n->attr ("id");
  GObject *obj = existing;
  QList<Node *> props;
  for (Node *k : n->kids) if (k->tag == "property") props << k;

  if (!obj) {
    GType t = resolve_type (n->attr ("class"));
    if (!t) {
      g_set_error (error, GTK_BUILDER_ERROR, GTK_BUILDER_ERROR_INVALID_TYPE_FUNCTION, "Invalid object type '%s'",
                   n->attr ("class").toUtf8 ().constData ());
      return nullptr;
    }
    if (t == GTK_TYPE_LIST_STORE || t == GTK_TYPE_TREE_STORE) obj = build_store (b, n, t);
    else obj = (GObject *) g_object_new (t, nullptr);
    register_object (b, id, obj);
  } else if (!id.isEmpty ()) {
    register_object (b, id, obj);
  }
  /* list items first, so properties such as "active" can refer to them */
  for (Node *k : n->kids) {
    if (k->tag == "items" && GTK_IS_COMBO_BOX_TEXT (obj)) {
      for (Node *it : k->kids)
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (obj), it->attr ("id").isEmpty () ? nullptr : it->attr ("id").toUtf8 ().constData (),
                                   it->text.toUtf8 ().constData ());
    } else if (k->tag == "items" && GTK_IS_STRING_LIST (obj)) {
      for (Node *it : k->kids) gtk_string_list_append (GTK_STRING_LIST (obj), it->text.toUtf8 ().constData ());
    }
  }
  set_props (b, obj, props);

  for (Node *k : n->kids) {
    if (k->tag == "signal") {
      bd (b)->signals_ << SignalInfo { obj, k->attr ("name").toUtf8 (), k->attr ("handler").toUtf8 (), k->attr ("object").toUtf8 (),
                                       k->attr ("after").toLower () == "yes" || k->attr ("after").toLower () == "true",
                                       k->attr ("swapped").isEmpty () ? !k->attr ("object").isEmpty ()
                                         : (k->attr ("swapped").toLower () == "yes" || k->attr ("swapped").toLower () == "true") };
    } else if (k->tag == "accelerator" && GTK_IS_WIDGET (obj)) {
      /* added to the toplevel window's accel group once the widget has one */
      guint key = gdk_keyval_from_name (k->attr ("key").toUtf8 ().constData ());
      guint mods = 0;
      QString m = k->attr ("modifiers");
      static const struct { const char *name; guint mask; } names[] = {
        { "SHIFT", GDK_SHIFT_MASK }, { "LOCK", GDK_LOCK_MASK }, { "CONTROL", GDK_CONTROL_MASK },
        { "MOD1", GDK_MOD1_MASK }, { "MOD2", GDK_MOD2_MASK }, { "MOD3", GDK_MOD3_MASK },
        { "MOD4", GDK_MOD4_MASK }, { "MOD5", GDK_MOD5_MASK }, { "SUPER", GDK_SUPER_MASK },
        { "META", GDK_META_MASK }, { "ALT", GDK_MOD1_MASK } };
      for (const QString &part : m.split ('|', Qt::SkipEmptyParts))
        for (const auto &nm : names)
          if (part.trimmed ().toUpper ().endsWith (QLatin1String (nm.name) + QLatin1String ("_MASK"))) mods |= nm.mask;
      if (key) gtkqt_builder_pending_accel (GTK_WIDGET (obj), k->attr ("signal").toUtf8 (), key, (GdkModifierType) mods);
    } else if (k->tag == "child") {
      Node *child_obj = nullptr, *packing = nullptr, *layout = nullptr;
      for (Node *c : k->kids) {
        if (c->tag == "object" || c->tag == "template") child_obj = c;
        else if (c->tag == "packing") packing = c;
      }
      if (!child_obj) continue;
      for (Node *c : child_obj->kids) if (c->tag == "layout") layout = c;
      QString internal = k->attr ("internal-child");
      if (!internal.isEmpty ()) {
        GtkWidget *ic = internal_child (obj, internal);
        if (ic) build_object (b, child_obj, G_OBJECT (ic), error);
        continue;
      }
      GObject *child = build_object (b, child_obj, nullptr, error);
      if (!child) return nullptr;
      add_child (b, obj, child, k->attr ("type"), packing, layout, k);
    } else if (k->tag == "action-widgets" && GTK_IS_DIALOG (obj)) {
      for (Node *aw : k->kids) {
        static const QMap<QString, int> named = { { "ok", GTK_RESPONSE_OK }, { "cancel", GTK_RESPONSE_CANCEL },
          { "close", GTK_RESPONSE_CLOSE }, { "yes", GTK_RESPONSE_YES }, { "no", GTK_RESPONSE_NO },
          { "apply", GTK_RESPONSE_APPLY }, { "help", GTK_RESPONSE_HELP }, { "accept", GTK_RESPONSE_ACCEPT },
          { "reject", GTK_RESPONSE_REJECT } };
        QString r = aw->attr ("response").toLower ().remove ("gtk_response_");
        int resp = named.contains (r) ? named[r] : r.toInt ();
        auto *w = (GtkWidget *) bd (b)->objects.value (aw->text.trimmed ());
        if (!w) continue;
        g_object_set_data (G_OBJECT (w), "gtkqt-response-id", GINT_TO_POINTER (resp));
        g_signal_connect (w, "clicked", G_CALLBACK (+[] (GtkWidget *bw, gpointer dlg) {
          gtk_dialog_response (GTK_DIALOG (dlg), GPOINTER_TO_INT (g_object_get_data (G_OBJECT (bw), "gtkqt-response-id")));
        }), obj);
      }
    }
  }
  return obj;
}

static void connect_one (GtkBuilder *b, const SignalInfo &s, GModule *module, gpointer user_data)
{
  gpointer fn = nullptr;
  /* GTK's own functions named as handlers in .ui files */
  static const QHash<QByteArray, gpointer> gtk_handlers = {
    { "gtk_main_quit", (gpointer) gtk_main_quit },
    { "gtk_widget_destroy", (gpointer) gtk_widget_destroy },
    { "gtk_widget_hide", (gpointer) gtk_widget_hide },
    { "gtk_widget_show", (gpointer) gtk_widget_show },
    { "gtk_widget_show_all", (gpointer) gtk_widget_show_all },
    { "gtk_widget_hide_on_delete", (gpointer) gtk_widget_hide_on_delete },
    { "gtk_widget_grab_focus", (gpointer) gtk_widget_grab_focus },
    { "gtk_window_close", (gpointer) gtk_window_close },
    { "gtk_window_present", (gpointer) gtk_window_present },
  };
  if (module && g_module_symbol (module, s.handler.constData (), &fn) && fn) { /* the program's handler */ }
  else fn = gtk_handlers.value (s.handler);
  if (!fn) {
    g_warning ("Could not find signal handler '%s'. Did you compile with -rdynamic?", s.handler.constData ());
    return;
  }
  GObject *target = s.connect_object.isEmpty () ? nullptr : bd (b)->objects.value (QString::fromUtf8 (s.connect_object));
  GConnectFlags flags = (GConnectFlags) ((s.after ? G_CONNECT_AFTER : 0) | (s.swapped ? G_CONNECT_SWAPPED : 0));
  if (target) g_signal_connect_object (s.object, s.signal.constData (), G_CALLBACK (fn), target, flags);
  else g_signal_connect_data (s.object, s.signal.constData (), G_CALLBACK (fn), user_data, nullptr, flags);
}

extern "C" {

GtkBuilder *gtk_builder_new (void) { return (GtkBuilder *) g_object_new (GTK_TYPE_BUILDER, nullptr); }

guint gtk_builder_add_from_string (GtkBuilder *b, const gchar *buffer, gsize length, GError **error)
{
  gtkqt_ensure_app ();
  QByteArray data = length == (gsize) -1 ? QByteArray (buffer) : QByteArray (buffer, (int) length);
  Node *root = parse_xml (data, error);
  if (!root) return 0;
  Node *iface = nullptr;
  for (Node *k : root->kids) if (k->tag == "interface") iface = k;
  if (!iface) { delete root; g_set_error (error, GTK_BUILDER_ERROR, GTK_BUILDER_ERROR_UNHANDLED_TAG, "No <interface> element"); return 0; }
  int n_signals_before = bd (b)->signals_.size ();
  bool ok = true;
  for (Node *k : iface->kids) {
    if (k->tag == "object" || k->tag == "template") {
      if (!build_object (b, k, nullptr, error)) { ok = false; break; }
    } else if (k->tag == "menu") {
      GMenu *m = build_menu (k);
      register_object (b, k->attr ("id"), G_OBJECT (m));   /* keeps the creation reference */
    }
  }
  /* object properties that referred to objects defined later */
  for (const DeferredProp &p : bd (b)->deferred)
    if (GObject *o = bd (b)->objects.value (p.id)) g_object_set (p.object, p.name.constData (), o, nullptr);
    else g_warning ("GtkBuilder: unknown object '%s'", p.id.toUtf8 ().constData ());
  bd (b)->deferred.clear ();
  delete root;
  gtkqt_builder_accels_retry ();
  /* GTK 4 connects handlers while building; GTK 2/3 wait for connect_signals() */
  if (ok && gtkqt_api >= 4) {
    static GModule *module = g_module_open (nullptr, (GModuleFlags) 0);
    for (int i = n_signals_before; i < bd (b)->signals_.size (); i++) connect_one (b, bd (b)->signals_[i], module, nullptr);
    bd (b)->signals_.clear ();
  }
  return ok ? 1 : 0;
}

guint gtk_builder_add_from_file (GtkBuilder *b, const gchar *filename, GError **error)
{
  QFile f (QString::fromUtf8 (filename));
  if (!f.open (QIODevice::ReadOnly)) {
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT, "Failed to open file '%s'", filename);
    return 0;
  }
  QByteArray data = f.readAll ();
  return gtk_builder_add_from_string (b, data.constData (), data.size (), error);
}

guint gtk_builder_add_from_resource (GtkBuilder *b, const gchar *resource_path, GError **error)
{
  GBytes *bytes = g_resources_lookup_data (resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE, error);
  if (!bytes) return 0;
  gsize len;
  const char *data = (const char *) g_bytes_get_data (bytes, &len);
  guint r = gtk_builder_add_from_string (b, data, len, error);
  g_bytes_unref (bytes);
  return r;
}

static GtkBuilder *new_with (guint (*add) (GtkBuilder *, const gchar *, GError **), const gchar *arg)
{
  GtkBuilder *b = gtk_builder_new ();
  GError *err = nullptr;
  if (!add (b, arg, &err)) g_error ("GtkBuilder: %s", err ? err->message : "failed");
  return b;
}

GtkBuilder *gtk_builder_new_from_file (const gchar *filename) { return new_with (gtk_builder_add_from_file, filename); }
GtkBuilder *gtk_builder_new_from_resource (const gchar *path) { return new_with (gtk_builder_add_from_resource, path); }

GtkBuilder *gtk_builder_new_from_string (const gchar *string, gssize length)
{
  GtkBuilder *b = gtk_builder_new ();
  GError *err = nullptr;
  if (!gtk_builder_add_from_string (b, string, length < 0 ? (gsize) -1 : (gsize) length, &err)) g_error ("GtkBuilder: %s", err ? err->message : "failed");
  return b;
}

GObject *gtk_builder_get_object (GtkBuilder *b, const gchar *name) { return bd (b)->objects.value (QString::fromUtf8 (name)); }

GSList *gtk_builder_get_objects (GtkBuilder *b)
{
  GSList *l = nullptr;
  for (GObject *o : bd (b)->objects) l = g_slist_prepend (l, o);
  return l;
}

void gtk_builder_connect_signals (GtkBuilder *b, gpointer user_data)
{
  static GModule *module = g_module_open (nullptr, (GModuleFlags) 0);
  for (const SignalInfo &s : bd (b)->signals_) connect_one (b, s, module, user_data);
  bd (b)->signals_.clear ();
}

void gtk_builder_connect_signals_full (GtkBuilder *b, GtkBuilderConnectFunc func, gpointer user_data)
{
  for (const SignalInfo &s : bd (b)->signals_) {
    GObject *target = s.connect_object.isEmpty () ? nullptr : bd (b)->objects.value (QString::fromUtf8 (s.connect_object));
    func (b, s.object, s.signal.constData (), s.handler.constData (), target,
          (GConnectFlags) ((s.after ? G_CONNECT_AFTER : 0) | (s.swapped ? G_CONNECT_SWAPPED : 0)), user_data);
  }
  bd (b)->signals_.clear ();
}

void gtk_builder_set_translation_domain (GtkBuilder *b, const gchar *domain) { bd (b)->domain = QString::fromUtf8 (domain); }

void gtk_builder_expose_object (GtkBuilder *b, const gchar *name, GObject *object)
{
  g_object_ref (object);
  bd (b)->order << object;
  bd (b)->objects.insert (QString::fromUtf8 (name), object);
}

void gtk_builder_set_current_object (GtkBuilder *, GObject *) {}
GType gtk_builder_get_type_from_name (GtkBuilder *, const char *type_name) { return resolve_type (QString::fromUtf8 (type_name)); }

} /* extern "C" */

/* ---------- <accelerator> ----------
 * GTK adds a builder accelerator to the accel group of the widget's
 * toplevel window (making one if the window has none). A widget built in a
 * file of its own (button panels) gets its window when it is packed: the
 * accelerators wait until then. */
struct PendingAccel { QByteArray signal; guint key; GdkModifierType mods; };
static QList<GtkWidget *> s_accel_widgets;                    /* weak */
bool gtkqt_builder_accels_waiting = false;

static void pending_gone (gpointer, GObject *w)
{
  s_accel_widgets.removeAll ((GtkWidget *) w);
  gtkqt_builder_accels_waiting = !s_accel_widgets.isEmpty ();
}

void gtkqt_builder_pending_accel (GtkWidget *w, const QByteArray &signal, guint key, GdkModifierType mods)
{
  auto *list = static_cast<QList<PendingAccel> *> (g_object_get_data (G_OBJECT (w), "gtkqt-pending-accels"));
  if (!list) {
    list = new QList<PendingAccel>;
    g_object_set_data_full (G_OBJECT (w), "gtkqt-pending-accels", list, [] (gpointer p) { delete static_cast<QList<PendingAccel> *> (p); });
    s_accel_widgets << w;
    g_object_weak_ref (G_OBJECT (w), pending_gone, nullptr);
  }
  *list << PendingAccel { signal, key, mods };
  gtkqt_builder_accels_waiting = true;
}

void gtkqt_builder_accels_retry (void)
{
  for (GtkWidget *w : QList<GtkWidget *> (s_accel_widgets)) {
    GtkWidget *top = gtk_widget_get_toplevel (w);
    if (!GTK_IS_WINDOW (top)) continue;
    GSList *groups = gtk_accel_groups_from_object (G_OBJECT (top));
    GtkAccelGroup *group = groups ? (GtkAccelGroup *) groups->data : nullptr;
    if (!group) {
      group = gtk_accel_group_new ();
      gtk_window_add_accel_group (GTK_WINDOW (top), group);
      g_object_unref (group);
    }
    auto *list = static_cast<QList<PendingAccel> *> (g_object_get_data (G_OBJECT (w), "gtkqt-pending-accels"));
    for (const PendingAccel &a : *list)
      gtk_widget_add_accelerator (w, a.signal.constData (), group, a.key, a.mods, GTK_ACCEL_VISIBLE);
    g_object_weak_unref (G_OBJECT (w), pending_gone, nullptr);
    s_accel_widgets.removeAll (w);
    g_object_set_data (G_OBJECT (w), "gtkqt-pending-accels", nullptr);
  }
  gtkqt_builder_accels_waiting = !s_accel_widgets.isEmpty ();
}
