/* gtkrc and CSS: what of them a program on Qt can use.
 *
 * The look of widgets is Qt's (themes, engines and pixmaps are ignored),
 * but programs and users also put things into gtkrc files and CSS that are
 * about the program, not the theme:
 *
 *  - key bindings: gtkrc "binding" blocks attached with class / widget_class
 *    / widget, CSS @binding-set with -gtk-key-bindings, and the key theme
 *    named by the gtk-key-theme-name setting (Emacs)
 *  - colours and fonts: gtkrc "style" blocks (fg, bg, text, base [STATE],
 *    font_name) and CSS color, background-color and font properties on
 *    type, node, #name and .class selectors, with state pseudo-classes
 *  - settings: gtk-xxx = value in gtkrc, settings.ini for GTK 3 and 4
 *
 * A style here is weaker than the program's own gtk_widget_modify_* /
 * override_* calls, as in GTK. */
#include "gtkqt-private.h"
#include <QtWidgets>
#include <algorithm>
#include <cmath>

namespace {

/* ---------------------------------------------------------------------
 * Style properties and rules
 * --------------------------------------------------------------------- */
struct StyleProps {
  QMap<int, GdkColor> colors;        /* kind * 5 + state; kind 0 fg, 1 bg, 2 text, 3 base */
  QByteArray font;                   /* Pango font description string, merged */
  QList<QByteArray> bindings;        /* CSS -gtk-key-bindings: binding set names */
  bool has_bindings = false;
  bool operator== (const StyleProps &o) const
  {
    if (font != o.font || bindings != o.bindings || colors.keys () != o.colors.keys ()) return false;
    for (auto it = colors.begin (); it != colors.end (); ++it) {
      const GdkColor &a = it.value (), &b = o.colors[it.key ()];
      if (a.red != b.red || a.green != b.green || a.blue != b.blue) return false;
    }
    return true;
  }
};

void merge_font (QByteArray &into, const QByteArray &from)
{
  if (from.isEmpty ()) return;
  if (into.isEmpty ()) { into = from; return; }
  PangoFontDescription *a = pango_font_description_from_string (into.constData ());
  PangoFontDescription *b = pango_font_description_from_string (from.constData ());
  pango_font_description_merge (a, b, TRUE);
  char *s = pango_font_description_to_string (a);
  into = s;
  g_free (s);
  pango_font_description_free (a);
  pango_font_description_free (b);
}

void merge_props (StyleProps &into, const StyleProps &from)
{
  for (auto it = from.colors.begin (); it != from.colors.end (); ++it) into.colors[it.key ()] = it.value ();
  merge_font (into.font, from.font);
  if (from.has_bindings) { into.bindings = from.bindings; into.has_bindings = true; }
}

/* rc: named styles, and the paths they are attached to */
struct RcStyle { StyleProps props; };
QHash<QByteArray, RcStyle> &rc_styles () { static QHash<QByteArray, RcStyle> h; return h; }
struct RcRule { GtkPathType type; QByteArray pattern; QByteArray style; int priority; int order; };
QList<RcRule> &rc_rules () { static QList<RcRule> l; return l; }
int rc_order = 0;

/* CSS */
struct Compound {
  QByteArray element;                /* empty or "*": any */
  QByteArray id;
  QList<QByteArray> classes;
  int state = -1;                    /* GtkStateType from a pseudo-class */
  bool bad = false;                  /* something we cannot match (:not, :focus ...) */
};
struct Selector {
  QList<Compound> parts;             /* left to right */
  QList<char> combinators;           /* between parts: ' ' or '>' */
  int state = GTK_STATE_NORMAL;
  int specificity = 0;
};
struct CssRule { Selector sel; StyleProps props; int order; };
struct CssData {
  QList<CssRule> rules;
  QHash<QByteArray, GdkRGBA> colors;   /* @define-color */
  QByteArray source;                   /* gtk_css_provider_to_string */
};
int css_order = 0;

struct ProviderRef { GtkCssProvider *provider; guint priority; };
QList<ProviderRef> &global_providers () { static QList<ProviderRef> l; return l; }

bool any_rules ();

/* ---------------------------------------------------------------------
 * Colours: #rgb, names, rgb()/rgba(), @name, shade/lighter/darker/mix/alpha
 * --------------------------------------------------------------------- */
QHash<QByteArray, GdkRGBA> &named_colors () { static QHash<QByteArray, GdkRGBA> h; return h; }

GdkRGBA shade (GdkRGBA c, double f)
{
  QColor q = QColor::fromRgbF (c.red, c.green, c.blue);
  decltype (q.hslHueF ()) h, s, l;
  q.getHslF (&h, &s, &l);
  l = qBound (0.0, (double) l * f, 1.0);
  s = qBound (0.0, (double) s * f, 1.0);
  q = QColor::fromHslF (h < 0 ? 0 : h, s, l);
  return GdkRGBA { q.redF (), q.greenF (), q.blueF (), c.alpha };
}

GdkRGBA mix (GdkRGBA a, GdkRGBA b, double f)
{
  return GdkRGBA { a.red + (b.red - a.red) * f, a.green + (b.green - a.green) * f,
                   a.blue + (b.blue - a.blue) * f, a.alpha + (b.alpha - a.alpha) * f };
}

/* split "a, b(c, d), e" at top-level commas */
QList<QByteArray> split_args (const QByteArray &s)
{
  QList<QByteArray> out;
  int depth = 0, start = 0;
  for (int i = 0; i < s.size (); i++) {
    char c = s[i];
    if (c == '(') depth++;
    else if (c == ')') depth--;
    else if (c == ',' && depth == 0) { out << s.mid (start, i - start).trimmed (); start = i + 1; }
  }
  out << s.mid (start).trimmed ();
  return out;
}

double parse_number (QByteArray s, double percent_of = 1.0)
{
  s = s.trimmed ();
  if (s.endsWith ('%')) return s.chopped (1).toDouble () / 100.0 * percent_of;
  return s.toDouble ();
}

/* rc_order: rc's mix (factor, a, b) puts the factor first */
bool parse_color (QByteArray s, GdkRGBA *out, const QHash<QByteArray, GdkRGBA> *local, bool rc_order = false)
{
  s = s.trimmed ();
  if (s.startsWith ('"') && s.endsWith ('"') && s.size () >= 2) s = s.mid (1, s.size () - 2);
  if (s.isEmpty ()) return false;
  if (s[0] == '@') {
    QByteArray n = s.mid (1);
    if (local && local->contains (n)) { *out = local->value (n); return true; }
    if (named_colors ().contains (n)) { *out = named_colors ().value (n); return true; }
    for (const ProviderRef &p : global_providers ()) {
      auto *d = static_cast<CssData *> (g_object_get_data (G_OBJECT (p.provider), "gtkqt-css"));
      if (d && d->colors.contains (n)) { *out = d->colors.value (n); return true; }
    }
    return false;
  }
  int paren = s.indexOf ('(');
  if (paren > 0 && s.endsWith (')')) {
    QByteArray fn = s.left (paren).trimmed ().toLower ();
    QList<QByteArray> a = split_args (s.mid (paren + 1, s.size () - paren - 2));
    GdkRGBA c1, c2;
    if ((fn == "rgb" || fn == "rgba") && a.size () >= 3) {
      auto ch = [] (const QByteArray &v) { return v.trimmed ().endsWith ('%') ? parse_number (v) : parse_number (v) / 255.0; };
      *out = GdkRGBA { ch (a[0]), ch (a[1]), ch (a[2]), a.size () > 3 ? parse_number (a[3]) : 1.0 };
      return true;
    }
    if (fn == "shade" && a.size () == 2) {
      if (rc_order) { if (!parse_color (a[1], &c1, local, rc_order)) return false; *out = shade (c1, parse_number (a[0])); }
      else { if (!parse_color (a[0], &c1, local, rc_order)) return false; *out = shade (c1, parse_number (a[1])); }
      return true;
    }
    if ((fn == "lighter" || fn == "darker") && a.size () == 1 && parse_color (a[0], &c1, local, rc_order)) {
      *out = shade (c1, fn == "lighter" ? 1.3 : 0.7);
      return true;
    }
    if (fn == "mix" && a.size () == 3) {
      if (rc_order) {
        if (!parse_color (a[1], &c1, local, rc_order) || !parse_color (a[2], &c2, local, rc_order)) return false;
        *out = mix (c1, c2, parse_number (a[0]));
      } else {
        if (!parse_color (a[0], &c1, local, rc_order) || !parse_color (a[1], &c2, local, rc_order)) return false;
        *out = mix (c1, c2, parse_number (a[2]));
      }
      return true;
    }
    if (fn == "alpha" && a.size () == 2 && parse_color (a[0], &c1, local, rc_order)) {
      c1.alpha *= parse_number (a[1]);
      *out = c1;
      return true;
    }
    return false;
  }
  if (s == "transparent") { *out = GdkRGBA { 0, 0, 0, 0 }; return true; }
  if (s[0] == '#' && (s.size () == 9 || s.size () == 5)) {     /* #rrggbbaa / #rgba */
    int n = s.size () == 9 ? 2 : 1;
    auto hx = [&] (int i) { bool ok; int v = s.mid (1 + i * n, n).toInt (&ok, 16); return n == 1 ? v * 17 / 255.0 : v / 255.0; };
    *out = GdkRGBA { hx (0), hx (1), hx (2), hx (3) };
    return true;
  }
  QColor q (QString::fromLatin1 (s));
  if (!q.isValid ()) return false;
  *out = GdkRGBA { q.redF (), q.greenF (), q.blueF (), q.alphaF () };
  return true;
}

GdkColor to_gdk (const GdkRGBA &c)
{
  GdkColor g {};
  g.red = (guint16) qBound (0.0, c.red * 65535.0 + 0.5, 65535.0);
  g.green = (guint16) qBound (0.0, c.green * 65535.0 + 0.5, 65535.0);
  g.blue = (guint16) qBound (0.0, c.blue * 65535.0 + 0.5, 65535.0);
  g.pixel = 0;
  return g;
}

/* ---------------------------------------------------------------------
 * The rc scanner (also reads the bodies of CSS @binding-set blocks)
 * --------------------------------------------------------------------- */
struct Tok {
  enum Kind { End, Ident, String, Number, Punct } kind = End;
  QByteArray text;
  double number = 0;
  bool integer = false;
};

class Scanner {
public:
  explicit Scanner (const QByteArray &d) : s (d) {}
  Tok next ()
  {
    if (have_peek) { have_peek = false; return peeked; }
    return scan ();
  }
  Tok peek ()
  {
    if (!have_peek) { peeked = scan (); have_peek = true; }
    return peeked;
  }
  bool accept (char c)
  {
    Tok t = peek ();
    if (t.kind == Tok::Punct && t.text.size () == 1 && t.text[0] == c) { next (); return true; }
    return false;
  }
  /* skip a { ... } block whose '{' is next */
  void skip_block ()
  {
    if (!accept ('{')) return;
    int depth = 1;
    while (depth > 0) {
      Tok t = next ();
      if (t.kind == Tok::End) return;
      if (t.kind == Tok::Punct && t.text == "{") depth++;
      if (t.kind == Tok::Punct && t.text == "}") depth--;
    }
  }
  int line () const { return ln; }

private:
  Tok scan ()
  {
    Tok t;
    for (;;) {
      while (i < s.size () && isspace ((unsigned char) s[i])) { if (s[i] == '\n') ln++; i++; }
      if (i >= s.size ()) return t;
      if (s[i] == '#') { while (i < s.size () && s[i] != '\n') i++; continue; }
      if (s[i] == '/' && i + 1 < s.size () && s[i + 1] == '*') {
        int e = s.indexOf ("*/", i + 2);
        for (int k = i; k < (e < 0 ? s.size () : e); k++) if (s[k] == '\n') ln++;
        i = e < 0 ? s.size () : e + 2;
        continue;
      }
      if (s[i] == '/' && i + 1 < s.size () && s[i + 1] == '/') { while (i < s.size () && s[i] != '\n') i++; continue; }
      break;
    }
    char c = s[i];
    if (c == '"') {
      t.kind = Tok::String;
      i++;
      while (i < s.size () && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size ()) {
          i++;
          char e = s[i];
          t.text += e == 'n' ? '\n' : e == 't' ? '\t' : e;
        } else {
          if (s[i] == '\n') ln++;
          t.text += s[i];
        }
        i++;
      }
      i++;
      return t;
    }
    if (isdigit ((unsigned char) c) || ((c == '-' || c == '+' || c == '.') && i + 1 < s.size () &&
                                        (isdigit ((unsigned char) s[i + 1]) || s[i + 1] == '.'))) {
      int st = i++;
      while (i < s.size () && (isalnum ((unsigned char) s[i]) || s[i] == '.')) i++;
      t.kind = Tok::Number;
      t.text = s.mid (st, i - st);
      bool ok;
      if (t.text.startsWith ("0x") || t.text.startsWith ("0X")) { t.number = t.text.mid (2).toLongLong (&ok, 16); t.integer = true; }
      else { t.number = t.text.toDouble (); t.integer = !t.text.contains ('.'); }
      return t;
    }
    if (isalpha ((unsigned char) c) || c == '_' || c == '@') {
      int st = i++;
      while (i < s.size () && (isalnum ((unsigned char) s[i]) || s[i] == '_' || s[i] == '-' || s[i] == ':')) {
        if (s[i] == ':' && !(i + 1 < s.size () && s[i + 1] == ':') && !(i > 0 && s[i - 1] == ':')) break;
        i++;
      }
      t.kind = Tok::Ident;
      t.text = s.mid (st, i - st);
      return t;
    }
    t.kind = Tok::Punct;
    t.text = QByteArray (1, c);
    i++;
    return t;
  }

  QByteArray s;
  int i = 0, ln = 1;
  Tok peeked;
  bool have_peek = false;
};

/* bind "<ctrl>w" { "signal" (args) ... }  |  unbind "<ctrl>w"
 * the next token is "bind" or "unbind" */
void parse_bind (Scanner &sc, GtkBindingSet *set)
{
  Tok kw = sc.next ();
  Tok key = sc.next ();
  if (key.kind != Tok::String) return;
  guint keyval = 0;
  GdkModifierType mods = (GdkModifierType) 0;
  gtk_accelerator_parse (key.text.constData (), &keyval, &mods);
  if (kw.text == "unbind") {
    if (keyval) gtk_binding_entry_skip (set, keyval, mods);
    return;
  }
  if (!sc.accept ('{')) return;
  if (keyval) gtk_binding_entry_clear (set, keyval, mods);
  for (;;) {
    Tok sig = sc.next ();
    if (sig.kind == Tok::End) return;
    if (sig.kind == Tok::Punct && sig.text == "}") break;
    if (sig.kind != Tok::String) continue;
    GSList *args = nullptr;
    if (sc.accept ('(')) {
      for (;;) {
        Tok a = sc.next ();
        if (a.kind == Tok::End || (a.kind == Tok::Punct && a.text == ")")) break;
        if (a.kind == Tok::Punct) continue;            /* , */
        auto *arg = g_new0 (GtkBindingArg, 1);
        if (a.kind == Tok::Number) {
          if (a.integer) { arg->arg_type = G_TYPE_LONG; arg->d.long_data = (glong) a.number; }
          else { arg->arg_type = G_TYPE_DOUBLE; arg->d.double_data = a.number; }
        } else if (a.kind == Tok::String) {
          arg->arg_type = G_TYPE_STRING;
          arg->d.string_data = g_strdup (a.text.constData ());
        } else {
          arg->arg_type = GTK_TYPE_IDENTIFIER;
          arg->d.string_data = g_strdup (a.text.constData ());
        }
        args = g_slist_append (args, arg);
      }
    }
    if (keyval) gtk_binding_entry_add_signall (set, keyval, mods, sig.text.constData (), args);
    for (GSList *l = args; l; l = l->next) {
      auto *arg = (GtkBindingArg *) l->data;
      if (arg->arg_type == G_TYPE_STRING || arg->arg_type == GTK_TYPE_IDENTIFIER) g_free (arg->d.string_data);
      g_free (arg);
    }
    g_slist_free (args);
  }
}

/* bind / unbind statements up to the closing '}' of a binding block */
void parse_binding_body (Scanner &sc, GtkBindingSet *set)
{
  if (!sc.accept ('{')) return;
  for (;;) {
    Tok t = sc.peek ();
    if (t.kind == Tok::End) return;
    if (t.kind == Tok::Punct && t.text == "}") { sc.next (); return; }
    if (t.kind == Tok::Ident && (t.text == "bind" || t.text == "unbind")) { parse_bind (sc, set); continue; }
    sc.next ();                                        /* ; and anything else */
  }
}

GtkBindingSet *binding_set_named (const QByteArray &name)
{
  GtkBindingSet *s = gtk_binding_set_find (name.constData ());
  return s ? s : gtk_binding_set_new (name.constData ());
}

/* ---------------------------------------------------------------------
 * gtkrc
 * --------------------------------------------------------------------- */
QStringList &rc_default_files () { static QStringList l; return l; }
QHash<QString, QDateTime> rc_parsed_files;      /* file -> modification time when read */
bool rc_initialized = false;

int state_from_name (const QByteArray &n)
{
  if (n == "NORMAL") return GTK_STATE_NORMAL;
  if (n == "ACTIVE") return GTK_STATE_ACTIVE;
  if (n == "PRELIGHT") return GTK_STATE_PRELIGHT;
  if (n == "SELECTED") return GTK_STATE_SELECTED;
  if (n == "INSENSITIVE") return GTK_STATE_INSENSITIVE;
  return -1;
}

/* an rc colour: "#rrggbb", "name", { r, g, b }, @name, shade (f, c) ... */
bool rc_color (Scanner &sc, GdkRGBA *out, const QHash<QByteArray, GdkRGBA> &local)
{
  Tok t = sc.next ();
  if (t.kind == Tok::String) return parse_color (t.text, out, &local, true);
  if (t.kind == Tok::Punct && t.text == "{") {
    double v[3] = { 0, 0, 0 };
    bool ints = true;
    for (int k = 0; k < 3; k++) {
      Tok n = sc.next ();
      v[k] = n.number;
      if (!n.integer) ints = false;
      sc.accept (',');
    }
    sc.accept ('}');
    double scale = ints && (v[0] > 1 || v[1] > 1 || v[2] > 1) ? 65535.0 : 1.0;
    *out = GdkRGBA { v[0] / scale, v[1] / scale, v[2] / scale, 1.0 };
    return true;
  }
  if (t.kind == Tok::Ident && t.text.startsWith ('@')) return parse_color (t.text, out, &local, true);
  if (t.kind == Tok::Ident) {                          /* shade (1.2, @bg) ... */
    QByteArray expr = t.text;
    if (!sc.accept ('(')) return false;
    expr += '(';
    int depth = 1;
    while (depth > 0) {
      Tok a = sc.next ();
      if (a.kind == Tok::End) return false;
      if (a.kind == Tok::Punct && a.text == "(") depth++;
      if (a.kind == Tok::Punct && a.text == ")") { if (--depth == 0) break; }
      expr += a.kind == Tok::String ? "\"" + a.text + "\"" : a.text;
    }
    expr += ')';
    return parse_color (expr, out, &local, true);
  }
  return false;
}

void rc_parse_style (Scanner &sc, const QByteArray &name)
{
  RcStyle st;
  QHash<QByteArray, GdkRGBA> local;
  if (sc.accept ('=')) {                               /* style "b" = "a": a's contents first */
    Tok parent = sc.next ();
    st = rc_styles ().value (parent.text);
  }
  if (!sc.accept ('{')) { rc_styles ().insert (name, st); return; }
  for (;;) {
    Tok t = sc.next ();
    if (t.kind == Tok::End) break;
    if (t.kind == Tok::Punct && t.text == "}") break;
    if (t.kind != Tok::Ident) continue;
    static const char *kinds[] = { "fg", "bg", "text", "base" };
    int kind = -1;
    for (int k = 0; k < 4; k++) if (t.text == kinds[k]) kind = k;
    if (kind >= 0 && sc.accept ('[')) {
      int state = state_from_name (sc.next ().text);
      sc.accept (']');
      sc.accept ('=');
      GdkRGBA c;
      if (rc_color (sc, &c, local) && state >= 0) st.props.colors[kind * 5 + state] = to_gdk (c);
      continue;
    }
    if (t.text == "color" && sc.accept ('[')) {          /* color["name"] = colour */
      QByteArray n = sc.next ().text;
      sc.accept (']');
      sc.accept ('=');
      GdkRGBA c;
      if (rc_color (sc, &c, local)) local.insert (n, c);
      continue;
    }
    if (t.text == "font_name") {
      sc.accept ('=');
      Tok f = sc.next ();
      if (f.kind == Tok::String) st.props.font = f.text;
      continue;
    }
    if (t.text == "engine") { sc.next (); sc.skip_block (); continue; }
    if (sc.peek ().kind == Tok::Punct && sc.peek ().text == "{") { sc.skip_block (); continue; }
    /* bg_pixmap, font, fontset, xthickness, stock, GtkWidget::style-property ...: skipped token by token */
  }
  rc_styles ().insert (name, st);
}

QByteArray settings_skip[] = { "gtk-theme-name", "gtk-font-name", "gtk-icon-theme-name", "gtk-cursor-theme-name",
                               "gtk-cursor-theme-size", "gtk-color-scheme", "gtk-xft-dpi" };

/* gtk-xxx = value: only settings about behaviour; the look follows Qt */
void apply_setting (const QByteArray &name, const QByteArray &value, bool is_string)
{
  for (const QByteArray &s : settings_skip) if (name == s) return;
  GtkSettings *settings = gtk_settings_get_default ();
  GParamSpec *ps = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), name.constData ());
  if (!ps) return;
  GType t = G_PARAM_SPEC_VALUE_TYPE (ps);
  if (t == G_TYPE_STRING) g_object_set (settings, name.constData (), value.constData (), nullptr);
  else if (t == G_TYPE_BOOLEAN) {
    QByteArray v = value.toLower ();
    g_object_set (settings, name.constData (), (gboolean) (v == "true" || v == "1" || v == "yes"), nullptr);
  } else if (t == G_TYPE_INT && !is_string) g_object_set (settings, name.constData (), value.toInt (), nullptr);
}

QString find_theme_file (const QString &theme, const QString &sub);
void rc_parse_file (const QString &path, int depth);

void rc_parse_data (const QByteArray &data, const QString &dir, int depth)
{
  Scanner sc (data);
  for (;;) {
    Tok t = sc.next ();
    if (t.kind == Tok::End) break;
    if (t.kind != Tok::Ident) continue;
    if (t.text == "include") {
      Tok f = sc.next ();
      if (f.kind != Tok::String) continue;
      QString p = QString::fromUtf8 (f.text);
      if (QDir::isRelativePath (p) && !dir.isEmpty ()) p = dir + "/" + p;
      /* a theme's gtkrc is the theme's look: only key themes are read */
      if (p.contains ("/themes/") && !p.contains ("-key/")) continue;
      rc_parse_file (p, depth + 1);
      continue;
    }
    if (t.text == "binding") {
      Tok n = sc.next ();
      if (sc.accept (':')) sc.next ();                  /* : priority */
      parse_binding_body (sc, binding_set_named (n.text));
      continue;
    }
    if (t.text == "style") {
      Tok n = sc.next ();
      rc_parse_style (sc, n.text);
      continue;
    }
    if (t.text == "class" || t.text == "widget" || t.text == "widget_class") {
      Tok pat = sc.next ();
      Tok what = sc.next ();
      Tok name = sc.next ();
      int prio = GTK_PATH_PRIO_RC;
      if (sc.accept (':')) {
        QByteArray p = sc.next ().text;
        if (p == "lowest") prio = GTK_PATH_PRIO_LOWEST;
        else if (p == "gtk") prio = GTK_PATH_PRIO_GTK;
        else if (p == "application") prio = GTK_PATH_PRIO_APPLICATION;
        else if (p == "theme") prio = GTK_PATH_PRIO_THEME;
        else if (p == "highest") prio = GTK_PATH_PRIO_HIGHEST;
      }
      GtkPathType type = t.text == "class" ? GTK_PATH_CLASS : t.text == "widget" ? GTK_PATH_WIDGET : GTK_PATH_WIDGET_CLASS;
      if (what.text == "binding")
        gtk_binding_set_add_path (binding_set_named (name.text), type, pat.text.constData (), (GtkPathPriorityType) prio);
      else if (what.text == "style")
        rc_rules () << RcRule { type, pat.text, name.text, prio, rc_order++ };
      continue;
    }
    if (t.text == "pixmap_path" || t.text == "module_path" || t.text == "im_module_path" || t.text == "im_module_file") {
      sc.next ();
      continue;
    }
    if (t.text == "engine") { sc.next (); sc.skip_block (); continue; }
    if ((t.text.startsWith ("gtk-") || t.text.startsWith ("gtk_")) && sc.accept ('=')) {
      Tok v = sc.next ();
      QByteArray name = t.text;
      name.replace ('_', '-');
      if (v.kind == Tok::Punct && v.text == "-") { Tok n = sc.next (); v.text = "-" + n.text; v.kind = n.kind; }
      apply_setting (name, v.text, v.kind == Tok::String);
      continue;
    }
  }
}

void rc_parse_file (const QString &path, int depth)
{
  if (depth > 10) return;
  QFile f (path);
  if (!f.open (QIODevice::ReadOnly)) return;
  rc_parsed_files.insert (QFileInfo (path).absoluteFilePath (), QFileInfo (path).lastModified ());
  rc_parse_data (f.readAll (), QFileInfo (path).absolutePath (), depth);
}

/* ---------------------------------------------------------------------
 * Widget paths (rc patterns, binding paths)
 * --------------------------------------------------------------------- */
struct PathComp { QByteArray text; GType type; };

QList<PathComp> widget_path (GtkWidget *w, bool by_class)
{
  QList<PathComp> l;
  for (; w; w = w->parent) {
    QByteArray n;
    if (!by_class) {
      QObject *o = gtkqt_obj (w);
      if (o && !o->objectName ().isEmpty ()) n = o->objectName ().toUtf8 ();
    }
    if (n.isEmpty ()) n = G_OBJECT_TYPE_NAME (w);
    l.prepend (PathComp { n, G_OBJECT_TYPE (w) });
  }
  return l;
}

/* glob match of a joined path; "<GtkButton>" matches one component whose
 * type is GtkButton or derived from it */
bool path_glob (const char *p, const QByteArray &str, const QVector<int> &comp_of, const QList<PathComp> &comps, int i)
{
  for (;;) {
    if (!*p) return i == str.size ();
    if (*p == '*') {
      while (*p == '*') p++;
      if (!*p) return true;
      for (int k = i; k <= str.size (); k++)
        if (path_glob (p, str, comp_of, comps, k)) return true;
      return false;
    }
    if (*p == '<') {
      const char *e = strchr (p, '>');
      if (!e) return false;
      if (i >= str.size () || (i > 0 && str[i - 1] != '.')) return false;
      int c = comp_of[i];
      GType want = g_type_from_name (QByteArray (p + 1, e - p - 1).constData ());
      if (!want || !g_type_is_a (comps[c].type, want)) return false;
      i += comps[c].text.size ();
      p = e + 1;
      continue;
    }
    if (i >= str.size ()) return false;
    if (*p != '?' && *p != str[i]) return false;
    p++;
    i++;
  }
}

bool path_match (const QList<PathComp> &comps, const char *pattern)
{
  QByteArray str;
  QVector<int> comp_of;
  for (int c = 0; c < comps.size (); c++) {
    if (c) { str += '.'; comp_of << c; }
    str += comps[c].text;
    for (int k = 0; k < comps[c].text.size (); k++) comp_of << c;
  }
  comp_of << comps.size () - 1;
  return path_glob (pattern, str, comp_of, comps, 0);
}

/* class "GtkEntry": the type or one it derives from; returns the depth of the
 * matched type below the widget's own (0 = the widget's own type), or -1 */
int class_match_depth (GtkWidget *w, const char *pattern)
{
  int depth = 0;
  for (GType t = G_OBJECT_TYPE (w); t; t = g_type_parent (t), depth++)
    if (g_pattern_match_simple (pattern, g_type_name (t))) return depth;
  return -1;
}

/* ---------------------------------------------------------------------
 * CSS
 * --------------------------------------------------------------------- */
/* GTK 3.20+ / 4 node names; older CSS uses the type names, which match too */
const char *node_name (GType t)
{
  static const struct { const char *type, *node; } table[] = {
    { "GtkWindow", "window" }, { "GtkButton", "button" }, { "GtkToggleButton", "button" },
    { "GtkCheckButton", "checkbutton" }, { "GtkRadioButton", "radiobutton" }, { "GtkLabel", "label" },
    { "GtkEntry", "entry" }, { "GtkSpinButton", "spinbutton" }, { "GtkTextView", "textview" },
    { "GtkTreeView", "treeview" }, { "GtkIconView", "iconview" }, { "GtkBox", "box" }, { "GtkGrid", "grid" },
    { "GtkFrame", "frame" }, { "GtkNotebook", "notebook" }, { "GtkScrolledWindow", "scrolledwindow" },
    { "GtkMenuBar", "menubar" }, { "GtkMenu", "menu" }, { "GtkMenuItem", "menuitem" }, { "GtkComboBox", "combobox" },
    { "GtkScale", "scale" }, { "GtkScrollbar", "scrollbar" }, { "GtkProgressBar", "progressbar" },
    { "GtkStatusbar", "statusbar" }, { "GtkToolbar", "toolbar" }, { "GtkPaned", "paned" }, { "GtkImage", "image" },
    { "GtkSeparator", "separator" }, { "GtkSwitch", "switch" }, { "GtkHeaderBar", "headerbar" },
    { "GtkListBox", "list" }, { "GtkListBoxRow", "row" }, { "GtkDrawingArea", "drawingarea" },
    { "GtkViewport", "viewport" }, { "GtkExpander", "expander" }, { "GtkLevelBar", "levelbar" },
    { "GtkCalendar", "calendar" }, { "GtkSpinner", "spinner" }, { "GtkStack", "stack" },
    { "GtkFlowBox", "flowbox" }, { "GtkFlowBoxChild", "flowboxchild" }, { "GtkSearchBar", "searchbar" },
    { "GtkActionBar", "actionbar" }, { "GtkInfoBar", "infobar" }, { "GtkPopover", "popover" },
    { "GtkColorButton", "colorbutton" }, { "GtkFontButton", "fontbutton" }, { "GtkAccelLabel", "accellabel" },
    { "GtkLayout", "layout" }, { "GtkFixed", "fixed" }, { "GtkEventBox", "eventbox" },
  };
  static QHash<GType, const char *> cache;
  if (cache.contains (t)) return cache.value (t);
  const char *found = nullptr;
  for (GType p = t; p && !found; p = g_type_parent (p))
    for (const auto &e : table)
      if (!strcmp (g_type_name (p), e.type)) { found = e.node; break; }
  cache.insert (t, found ? found : "widget");
  return cache.value (t);
}

bool compound_matches (const Compound &c, GtkWidget *w)
{
  if (c.bad) return false;
  if (!c.element.isEmpty () && c.element != "*") {
    bool ok = c.element == node_name (G_OBJECT_TYPE (w));
    for (GType t = G_OBJECT_TYPE (w); t && !ok; t = g_type_parent (t))
      if (c.element == g_type_name (t)) ok = true;
    if (!ok) return false;
  }
  if (!c.id.isEmpty ()) {
    QObject *o = gtkqt_obj (w);
    if (!o || o->objectName ().toUtf8 () != c.id) return false;
  }
  for (const QByteArray &cls : c.classes) {
    if (w->priv->css_classes.contains (QString::fromUtf8 (cls))) continue;
    if (cls == "view" && (GTK_IS_TEXT_VIEW (w) || GTK_IS_TREE_VIEW (w))) continue;   /* implicit */
    return false;
  }
  return true;
}

bool selector_matches (const Selector &s, GtkWidget *w)
{
  if (s.parts.isEmpty ()) return true;
  int i = s.parts.size () - 1;
  if (!compound_matches (s.parts[i], w)) return false;
  /* ancestors, right to left: descendant ' ' or child '>' */
  std::function<bool (int, GtkWidget *)> up = [&] (int k, GtkWidget *from) -> bool {
    if (k < 0) return true;
    char comb = s.combinators[k];
    for (GtkWidget *a = from->parent; a; a = a->parent) {
      if (compound_matches (s.parts[k], a) && up (k - 1, a)) return true;
      if (comb == '>') return false;
    }
    return false;
  };
  return up (i - 1, w);
}

int pseudo_state (const QByteArray &p)
{
  if (p == "disabled" || p == "insensitive") return GTK_STATE_INSENSITIVE;
  if (p == "selected") return GTK_STATE_SELECTED;
  if (p == "hover" || p == "prelight") return GTK_STATE_PRELIGHT;
  if (p == "active" || p == "checked") return GTK_STATE_ACTIVE;
  return -1;
}

bool parse_selector (QByteArray text, Selector &out)
{
  text = text.trimmed ();
  if (text.isEmpty ()) return false;
  QList<QByteArray> words;
  QList<char> combs;
  QByteArray cur;
  char pending = ' ';
  int depth = 0;
  for (int i = 0; i <= text.size (); i++) {
    char c = i < text.size () ? text[i] : ' ';
    if (c == '(') depth++;
    if (c == ')') depth--;
    if (depth == 0 && (isspace ((unsigned char) c) || c == '>' || c == '+' || c == '~')) {
      if (!cur.isEmpty ()) {
        if (!words.isEmpty ()) combs << pending;
        words << cur;
        cur.clear ();
        pending = ' ';
      }
      if (c == '>') pending = '>';
      if (c == '+' || c == '~') return false;           /* sibling combinators: not supported */
      continue;
    }
    cur += c;
  }
  for (int k = 0; k < words.size (); k++) {
    const QByteArray &wd = words[k];
    Compound comp;
    int i = 0;
    while (i < wd.size () && wd[i] != '#' && wd[i] != '.' && wd[i] != ':') i++;
    comp.element = wd.left (i);
    while (i < wd.size ()) {
      char kind = wd[i++];
      if (kind == ':' && i < wd.size () && wd[i] == ':') { comp.bad = true; break; }   /* ::pseudo-element */
      int st = i;
      int d = 0;
      while (i < wd.size () && (d > 0 || (wd[i] != '#' && wd[i] != '.' && wd[i] != ':'))) {
        if (wd[i] == '(') d++;
        if (wd[i] == ')') d--;
        i++;
      }
      QByteArray n = wd.mid (st, i - st);
      if (kind == '#') { comp.id = n; out.specificity += 10000; }
      else if (kind == '.') { comp.classes << n; out.specificity += 100; }
      else {
        int ps = pseudo_state (n);
        if (ps < 0) comp.bad = true;                   /* :backdrop, :focus, :not() ...: not tracked */
        else comp.state = ps;
        out.specificity += 100;
      }
    }
    if (!comp.element.isEmpty () && comp.element != "*") out.specificity += 1;
    out.parts << comp;
  }
  out.combinators = combs;
  /* subnodes that are part of the widget here: "textview text", "entry selection" */
  while (!out.parts.isEmpty ()) {
    const Compound &last = out.parts.last ();
    if (last.element == "selection") out.state = GTK_STATE_SELECTED;
    else if (last.element == "text" || last.element == "view" || last.element == "contents" || last.element == "viewport") {
      if (last.state >= 0 && out.state == GTK_STATE_NORMAL) out.state = last.state;
    } else break;
    out.parts.removeLast ();
    if (!out.combinators.isEmpty ()) out.combinators.removeLast ();
  }
  if (!out.parts.isEmpty () && out.parts.last ().state >= 0 && out.state == GTK_STATE_NORMAL) out.state = out.parts.last ().state;
  for (int k = 0; k < out.parts.size () - 1; k++)
    if (out.parts[k].state >= 0) out.parts[k].bad = true;   /* states of ancestors: not tracked */
  if (!out.parts.isEmpty ()) out.parts.last ().state = -1;
  return true;
}

/* font-size: 12pt, 16px, 1.2em, 120%, larger, small ... */
void css_font_size (const QByteArray &v, PangoFontDescription *d)
{
  QByteArray s = v.trimmed ().toLower ();
  double base_pt = QApplication::font ().pointSizeF () > 0 ? QApplication::font ().pointSizeF () : 10;
  static const struct { const char *n; double f; } named[] = {
    { "xx-small", 0.6 }, { "x-small", 0.75 }, { "small", 0.89 }, { "medium", 1.0 }, { "large", 1.2 },
    { "x-large", 1.5 }, { "xx-large", 2.0 }, { "smaller", 0.83 }, { "larger", 1.2 } };
  for (const auto &n : named) if (s == n.n) { pango_font_description_set_size (d, (int) (base_pt * n.f * PANGO_SCALE)); return; }
  if (s.endsWith ("px")) { pango_font_description_set_absolute_size (d, s.chopped (2).toDouble () * PANGO_SCALE); return; }
  if (s.endsWith ("pt")) { pango_font_description_set_size (d, (int) (s.chopped (2).toDouble () * PANGO_SCALE)); return; }
  if (s.endsWith ("em") && !s.endsWith ("rem")) { pango_font_description_set_size (d, (int) (s.chopped (2).toDouble () * base_pt * PANGO_SCALE)); return; }
  if (s.endsWith ("rem")) { pango_font_description_set_size (d, (int) (s.chopped (3).toDouble () * base_pt * PANGO_SCALE)); return; }
  if (s.endsWith ('%')) { pango_font_description_set_size (d, (int) (s.chopped (1).toDouble () / 100 * base_pt * PANGO_SCALE)); return; }
  bool ok;
  double n = s.toDouble (&ok);
  if (ok) pango_font_description_set_absolute_size (d, n * PANGO_SCALE);   /* plain numbers are px */
}

bool css_font_weight (const QByteArray &v, PangoFontDescription *d)
{
  QByteArray s = v.trimmed ().toLower ();
  bool ok;
  int n = s.toInt (&ok);
  if (ok) { pango_font_description_set_weight (d, (PangoWeight) n); return true; }
  if (s == "bold" || s == "bolder") { pango_font_description_set_weight (d, PANGO_WEIGHT_BOLD); return true; }
  if (s == "normal") { pango_font_description_set_weight (d, PANGO_WEIGHT_NORMAL); return true; }
  if (s == "lighter") { pango_font_description_set_weight (d, PANGO_WEIGHT_LIGHT); return true; }
  return false;
}

bool css_font_style (const QByteArray &v, PangoFontDescription *d)
{
  QByteArray s = v.trimmed ().toLower ();
  if (s == "italic") { pango_font_description_set_style (d, PANGO_STYLE_ITALIC); return true; }
  if (s == "oblique") { pango_font_description_set_style (d, PANGO_STYLE_OBLIQUE); return true; }
  if (s == "normal") { pango_font_description_set_style (d, PANGO_STYLE_NORMAL); return true; }
  return false;
}

QByteArray first_family (const QByteArray &v)
{
  QByteArray f = split_args (v).value (0).trimmed ();
  if ((f.startsWith ('"') && f.endsWith ('"')) || (f.startsWith ('\'') && f.endsWith ('\''))) f = f.mid (1, f.size () - 2);
  if (f == "monospace") return "Monospace";
  if (f == "sans-serif") return "Sans";
  if (f == "serif") return "Serif";
  return f;
}

/* one declaration into the rule's properties */
void css_declaration (const QByteArray &prop, const QByteArray &value, int state, StyleProps &p, const CssData &d,
                      PangoFontDescription *font, bool &font_set)
{
  GdkRGBA c;
  if (prop == "color") {
    if (parse_color (value, &c, &d.colors) && c.alpha > 0) {
      p.colors[0 * 5 + state] = to_gdk (c);
      p.colors[2 * 5 + state] = to_gdk (c);
    }
  } else if (prop == "background-color" || prop == "background") {
    QByteArray v = value.trimmed ();
    bool ok = parse_color (v, &c, &d.colors);
    if (!ok && prop == "background") {                /* background: none #fff ... */
      for (const QByteArray &part : v.simplified ().split (' '))
        if (parse_color (part, &c, &d.colors)) { ok = true; break; }
    }
    if (ok && c.alpha > 0) {
      p.colors[1 * 5 + state] = to_gdk (c);
      p.colors[3 * 5 + state] = to_gdk (c);
    }
  } else if (prop == "font-family") {
    pango_font_description_set_family (font, first_family (value).constData ());
    font_set = true;
  } else if (prop == "font-size") {
    css_font_size (value, font);
    font_set = true;
  } else if (prop == "font-weight") {
    font_set |= css_font_weight (value, font);
  } else if (prop == "font-style") {
    font_set |= css_font_style (value, font);
  } else if (prop == "font") {
    /* [style] [weight] size[/line-height] family[, family] */
    QList<QByteArray> words = value.simplified ().split (' ');
    int k = 0;
    for (; k < words.size (); k++) {
      if (css_font_style (words[k], font) || css_font_weight (words[k], font)) continue;
      if (words[k] == "normal" || words[k] == "small-caps") continue;
      break;
    }
    if (k < words.size ()) {
      QByteArray size = words[k++];
      if (size.contains ('/')) size = size.left (size.indexOf ('/'));
      css_font_size (size, font);
    }
    if (k < words.size ()) pango_font_description_set_family (font, first_family (words.mid (k).join (' ')).constData ());
    font_set = true;
  } else if (prop == "-gtk-key-bindings" || prop == "gtk-key-bindings") {
    p.bindings.clear ();
    for (const QByteArray &n : split_args (value))
      if (!n.isEmpty () && n != "none") p.bindings << n;
    p.has_bindings = true;
  }
  /* everything else (borders, padding, images, transitions ...) is the theme's: ignored */
}

QByteArray strip_comments (const QByteArray &s)
{
  QByteArray out;
  out.reserve (s.size ());
  for (int i = 0; i < s.size (); i++) {
    if (s[i] == '/' && i + 1 < s.size () && s[i + 1] == '*') {
      int e = s.indexOf ("*/", i + 2);
      if (e < 0) break;
      for (int k = i; k < e; k++) if (s[k] == '\n') out += '\n';   /* keep line numbers */
      i = e + 1;
      continue;
    }
    if (s[i] == '"' || s[i] == '\'') {                  /* strings stay as they are */
      char q = s[i];
      int e = i + 1;
      while (e < s.size () && s[e] != q) { if (s[e] == '\\') e++; e++; }
      out += s.mid (i, e - i + 1);
      i = e;
      continue;
    }
    out += s[i];
  }
  return out;
}

/* the matching '}' for the '{' at open, or -1 */
int block_end (const QByteArray &s, int open)
{
  int depth = 0;
  for (int i = open; i < s.size (); i++) {
    if (s[i] == '"' || s[i] == '\'') {
      char q = s[i];
      for (i++; i < s.size () && s[i] != q; i++) if (s[i] == '\\') i++;
      continue;
    }
    if (s[i] == '{') depth++;
    else if (s[i] == '}' && --depth == 0) return i;
  }
  return -1;
}

void css_parse (CssData &d, const QByteArray &input, const QString &dir, int depth);

void css_parse_file (CssData &d, const QString &path, int depth)
{
  if (depth > 10) return;
  QFile f (path);
  if (f.open (QIODevice::ReadOnly)) css_parse (d, f.readAll (), QFileInfo (path).absolutePath (), depth + 1);
}

void css_parse (CssData &d, const QByteArray &input, const QString &dir, int depth)
{
  QByteArray s = strip_comments (input);
  int i = 0;
  while (i < s.size ()) {
    while (i < s.size () && isspace ((unsigned char) s[i])) i++;
    if (i >= s.size ()) break;
    if (s[i] == '@') {
      int st = i + 1;
      while (i < s.size () && !isspace ((unsigned char) s[i]) && s[i] != '{' && s[i] != ';') i++;
      QByteArray kw = s.mid (st, i - st);
      int semi = s.indexOf (';', i), brace = s.indexOf ('{', i);
      if (kw == "binding-set" && brace >= 0 && (semi < 0 || brace < semi)) {
        QByteArray name = s.mid (i, brace - i).trimmed ();
        int e = block_end (s, brace);
        if (e < 0) break;
        Scanner sc (s.mid (brace, e - brace + 1));
        parse_binding_body (sc, binding_set_named (name));
        i = e + 1;
        continue;
      }
      if (brace >= 0 && (semi < 0 || brace < semi)) {   /* @keyframes, @media ...: skipped */
        int e = block_end (s, brace);
        i = e < 0 ? s.size () : e + 1;
        continue;
      }
      QByteArray rest = s.mid (i, (semi < 0 ? s.size () : semi) - i).trimmed ();
      i = semi < 0 ? s.size () : semi + 1;
      if (kw == "define-color") {
        int sp = rest.indexOf (' ');
        if (sp > 0) {
          GdkRGBA c;
          if (parse_color (rest.mid (sp + 1), &c, &d.colors)) d.colors.insert (rest.left (sp), c);
        }
      } else if (kw == "import") {
        QByteArray u = rest;
        if (u.startsWith ("url(")) u = u.mid (4, u.lastIndexOf (')') - 4);
        u = u.trimmed ();
        if (u.size () >= 2 && (u[0] == '"' || u[0] == '\'')) u = u.mid (1, u.size () - 2);
        QString p = QString::fromUtf8 (u);
        if (p.startsWith ("file://")) p = QUrl (p).toLocalFile ();
        if (QDir::isRelativePath (p)) p = dir + "/" + p;
        if (!p.startsWith ("resource:")) css_parse_file (d, p, depth);
      }
      continue;
    }
    int brace = s.indexOf ('{', i);
    if (brace < 0) break;
    QByteArray selectors = s.mid (i, brace - i);
    int e = block_end (s, brace);
    if (e < 0) break;
    QByteArray body = s.mid (brace + 1, e - brace - 1);
    i = e + 1;
    /* declarations: split at ';' outside parentheses */
    QList<QPair<QByteArray, QByteArray>> decls;
    int pd = 0, ds = 0;
    for (int k = 0; k <= body.size (); k++) {
      char c = k < body.size () ? body[k] : ';';
      if (c == '(') pd++;
      if (c == ')') pd--;
      if (c == ';' && pd == 0) {
        QByteArray one = body.mid (ds, k - ds).trimmed ();
        ds = k + 1;
        int colon = one.indexOf (':');
        if (colon > 0) {
          QByteArray v = one.mid (colon + 1).trimmed ();
          if (v.endsWith ("!important")) v.chop (10);
          decls << qMakePair (one.left (colon).trimmed ().toLower (), v.trimmed ());
        }
      }
    }
    for (const QByteArray &one : split_args (selectors)) {
      CssRule r;
      if (!parse_selector (one, r.sel)) continue;
      PangoFontDescription *font = pango_font_description_new ();
      bool font_set = false;
      for (const auto &dl : decls) css_declaration (dl.first, dl.second, r.sel.state, r.props, d, font, font_set);
      if (font_set) {
        char *fs = pango_font_description_to_string (font);
        r.props.font = fs;
        g_free (fs);
      }
      pango_font_description_free (font);
      if (r.props.colors.isEmpty () && r.props.font.isEmpty () && !r.props.has_bindings) continue;
      /* fonts and bindings only for the plain state */
      if (r.sel.state != GTK_STATE_NORMAL) { r.props.font.clear (); r.props.has_bindings = false; }
      r.order = css_order++;
      d.rules << r;
    }
  }
}

CssData *css_data (GtkCssProvider *p)
{
  auto *d = static_cast<CssData *> (g_object_get_data (G_OBJECT (p), "gtkqt-css"));
  if (!d) {
    d = new CssData;
    g_object_set_data_full (G_OBJECT (p), "gtkqt-css", d, [] (gpointer x) { delete static_cast<CssData *> (x); });
  }
  return d;
}

QList<ProviderRef> *widget_providers (GtkWidget *w)
{
  return static_cast<QList<ProviderRef> *> (g_object_get_data (G_OBJECT (w), "gtkqt-css-providers"));
}

bool any_rules ()
{
  if (!rc_rules ().isEmpty ()) return true;
  for (const ProviderRef &p : global_providers ()) {
    CssData *d = static_cast<CssData *> (g_object_get_data (G_OBJECT (p.provider), "gtkqt-css"));
    if (d && !d->rules.isEmpty ()) return true;
  }
  return false;
}
bool widget_providers_exist = false;

/* ---------------------------------------------------------------------
 * Applying
 * --------------------------------------------------------------------- */
StyleProps compute_style (GtkWidget *w)
{
  StyleProps out;
  /* rc: class < widget_class < widget, a derived class over its parent,
   * then later rules; all under the path priority */
  if (!rc_rules ().isEmpty ()) {
    struct Hit { int prio, cat, depth, order; const RcRule *r; };
    QList<Hit> hits;
    QList<PathComp> names, classes;
    bool have_paths = false;
    for (const RcRule &r : rc_rules ()) {
      if (r.type == GTK_PATH_CLASS) {
        int d = class_match_depth (w, r.pattern.constData ());
        if (d >= 0) hits << Hit { r.priority, 0, -d, r.order, &r };
        continue;
      }
      if (!have_paths) { names = widget_path (w, false); classes = widget_path (w, true); have_paths = true; }
      if (path_match (r.type == GTK_PATH_WIDGET ? names : classes, r.pattern.constData ()))
        hits << Hit { r.priority, r.type == GTK_PATH_WIDGET ? 2 : 1, 0, r.order, &r };
    }
    std::sort (hits.begin (), hits.end (), [] (const Hit &a, const Hit &b) {
      if (a.prio != b.prio) return a.prio < b.prio;
      if (a.cat != b.cat) return a.cat < b.cat;
      if (a.depth != b.depth) return a.depth < b.depth;
      return a.order < b.order;
    });
    for (const Hit &h : hits) {
      auto it = rc_styles ().constFind (h.r->style);
      if (it != rc_styles ().constEnd ()) merge_props (out, it->props);
    }
  }
  /* CSS: provider priority, then specificity, then order */
  struct CHit { guint prio; int spec, order; const CssRule *r; };
  QList<CHit> chits;
  auto collect = [&] (const QList<ProviderRef> &list) {
    for (const ProviderRef &p : list) {
      CssData *d = static_cast<CssData *> (g_object_get_data (G_OBJECT (p.provider), "gtkqt-css"));
      if (!d) continue;
      for (const CssRule &r : d->rules)
        if (selector_matches (r.sel, w)) chits << CHit { p.priority, r.sel.specificity, r.order, &r };
    }
  };
  collect (global_providers ());
  if (QList<ProviderRef> *own = widget_providers (w)) collect (*own);
  std::sort (chits.begin (), chits.end (), [] (const CHit &a, const CHit &b) {
    if (a.prio != b.prio) return a.prio < b.prio;
    if (a.spec != b.spec) return a.spec < b.spec;
    return a.order < b.order;
  });
  for (const CHit &h : chits) merge_props (out, h.r->props);
  return out;
}

QHash<GtkWidget *, StyleProps> &applied () { static QHash<GtkWidget *, StyleProps> h; return h; }

void forget_widget (gpointer, GObject *dead) { applied ().remove ((GtkWidget *) dead); }

void apply_props (GtkWidget *w, const StyleProps &now)
{
  auto it = applied ().find (w);
  bool had = it != applied ().end ();
  if (!had && now.colors.isEmpty () && now.font.isEmpty () && !now.has_bindings) return;
  StyleProps old = had ? it.value () : StyleProps ();
  if (had && old == now) return;
  if (!had) g_object_weak_ref (G_OBJECT (w), forget_widget, nullptr);
  applied ().insert (w, now);
  for (auto c = old.colors.begin (); c != old.colors.end (); ++c)
    if (!now.colors.contains (c.key ())) gtkqt_style_rc_color (w, c.key () / 5, (GtkStateType) (c.key () % 5), nullptr);
  for (auto c = now.colors.begin (); c != now.colors.end (); ++c) {
    auto o = old.colors.constFind (c.key ());
    if (o != old.colors.constEnd () && o->red == c->red && o->green == c->green && o->blue == c->blue) continue;
    gtkqt_style_rc_color (w, c.key () / 5, (GtkStateType) (c.key () % 5), &c.value ());
  }
  if (now.font != old.font) {
    if (now.font.isEmpty ()) gtkqt_style_rc_font (w, nullptr);
    else {
      PangoFontDescription *d = pango_font_description_from_string (now.font.constData ());
      PangoFontDescription *base = gtkqt_pango_from_font (QApplication::font ());
      pango_font_description_merge (d, base, FALSE);
      gtkqt_style_rc_font (w, d);
      pango_font_description_free (base);
      pango_font_description_free (d);
    }
  }
}

void style_tree (GtkWidget *w, bool recurse)
{
  if (!w || !w->priv || w->priv->destroyed) return;
  apply_props (w, compute_style (w));
  if (!recurse || !GTK_IS_CONTAINER (w)) return;
  GList *kids = gtk_container_get_children (GTK_CONTAINER (w));
  for (GList *l = kids; l; l = l->next) style_tree ((GtkWidget *) l->data, true);
  g_list_free (kids);
}

QString find_theme_file (const QString &theme, const QString &sub)
{
  QStringList dirs;
  dirs << QDir::homePath () + "/.themes" << QString::fromUtf8 (g_get_user_data_dir ()) + "/themes";
  for (const gchar *const *d = g_get_system_data_dirs (); *d; d++) dirs << QString::fromUtf8 (*d) + "/themes";
  for (const QString &d : dirs) {
    QString p = d + "/" + theme + "/" + sub;
    if (QFile::exists (p)) return p;
  }
  return QString ();
}

GtkCssProvider *key_theme_provider = nullptr;
QString loaded_key_theme;

/* gtk-key-theme-name: GTK 2 reads <theme>/gtk-2.0-key/gtkrc, GTK 3
 * <theme>/gtk-3.0/gtk-keys.css (GTK 4 has no key themes) */
void load_key_theme ()
{
  gchar *name = nullptr;
  g_object_get (gtk_settings_get_default (), "gtk-key-theme-name", &name, nullptr);
  QString theme = QString::fromUtf8 (name ? name : "");
  g_free (name);
  if (theme == loaded_key_theme) return;
  loaded_key_theme = theme;
  if (gtkqt_api <= 2) {
    if (theme.isEmpty () || theme == "Default") return;
    QString f = find_theme_file (theme, "gtk-2.0-key/gtkrc");
    if (!f.isEmpty ()) { rc_parse_file (f, 0); gtkqt_rc_restyle_all (); }
    return;
  }
  if (gtkqt_api >= 4) return;
  if (!key_theme_provider) {
    key_theme_provider = gtk_css_provider_new ();
    gtk_style_context_add_provider_for_screen (nullptr, key_theme_provider, GTK_STYLE_PROVIDER_PRIORITY_SETTINGS);
  }
  QString f = theme.isEmpty () || theme == "Default" ? QString () : find_theme_file (theme, "gtk-3.0/gtk-keys.css");
  if (f.isEmpty ()) gtk_css_provider_load_from_data (key_theme_provider, "", 0, nullptr);
  else gtk_css_provider_load_from_path (key_theme_provider, f.toUtf8 ().constData (), nullptr);
}

/* GTK 3 / 4: [Settings] in settings.ini */
void read_settings_ini (const QString &path)
{
  GKeyFile *kf = g_key_file_new ();
  if (g_key_file_load_from_file (kf, path.toUtf8 ().constData (), G_KEY_FILE_NONE, nullptr)) {
    gchar **keys = g_key_file_get_keys (kf, "Settings", nullptr, nullptr);
    for (int i = 0; keys && keys[i]; i++) {
      gchar *v = g_key_file_get_value (kf, "Settings", keys[i], nullptr);
      QByteArray val = QByteArray (v ? v : "").trimmed ();
      bool quoted = val.size () >= 2 && val.startsWith ('"') && val.endsWith ('"');
      if (quoted) val = val.mid (1, val.size () - 2);
      apply_setting (keys[i], val, quoted);
      g_free (v);
    }
    g_strfreev (keys);
  }
  g_key_file_free (kf);
}

} /* namespace */

/* ---------------------------------------------------------------------
 * Internal entry points
 * --------------------------------------------------------------------- */
void gtkqt_rc_init (void)
{
  if (rc_initialized) return;
  rc_initialized = true;
  if (gtkqt_api <= 2) {
    QStringList files;
    QByteArray env = qgetenv ("GTK2_RC_FILES");
    if (env.isEmpty ()) env = qgetenv ("GTK_RC_FILES");
    if (!env.isEmpty ()) files = QString::fromLocal8Bit (env).split (':', Qt::SkipEmptyParts);
    else files << "/etc/gtk-2.0/gtkrc" << QDir::homePath () + "/.gtkrc-2.0";
    files << rc_default_files ();
    for (const QString &f : files) rc_parse_file (f, 0);
  } else {
    QString sub = gtkqt_api >= 4 ? "gtk-4.0" : "gtk-3.0";
    QStringList dirs;
    for (const gchar *const *d = g_get_system_config_dirs (); *d; d++) dirs.prepend (QString::fromUtf8 (*d));
    dirs.prepend ("/etc");
    dirs << QString::fromUtf8 (g_get_user_config_dir ());
    dirs.removeDuplicates ();
    for (const QString &d : dirs) read_settings_ini (d + "/" + sub + "/settings.ini");
    QString user_css = QString::fromUtf8 (g_get_user_config_dir ()) + "/" + sub + "/gtk.css";
    if (QFile::exists (user_css)) {
      GtkCssProvider *p = gtk_css_provider_new ();
      gtk_css_provider_load_from_path (p, user_css.toUtf8 ().constData (), nullptr);
      gtk_style_context_add_provider_for_screen (nullptr, p, GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
  }
  g_signal_connect (gtk_settings_get_default (), "notify::gtk-key-theme-name", G_CALLBACK (+[] (GObject *, GParamSpec *, gpointer) {
    load_key_theme ();
  }), nullptr);
  load_key_theme ();
}

void gtkqt_rc_style_widget (GtkWidget *widget, bool recurse)
{
  if (!widget) return;
  if (!any_rules () && !widget_providers_exist && applied ().isEmpty ()) return;
  style_tree (widget, recurse);
}

/* the program gave up its own colour or font: the rc / CSS one applies again */
void gtkqt_rc_program_reset (GtkWidget *widget)
{
  auto it = applied ().find (widget);
  if (it == applied ().end ()) return;
  it->colors.clear ();
  it->font.clear ();
  style_tree (widget, false);
}

void gtkqt_rc_restyle_all (void)
{
  if (!QCoreApplication::instance ()) return;
  if (!any_rules () && !widget_providers_exist && applied ().isEmpty ()) return;
  const auto all = QApplication::allWidgets ();
  QSet<GtkWidget *> seen;
  for (QWidget *qw : all) {
    auto *w = (GtkWidget *) qw->property ("_gtkqt_widget").value<void *> ();
    if (!w || seen.contains (w) || !w->priv || w->priv->destroyed || !qw->isVisible ()) continue;
    seen.insert (w);
    style_tree (w, false);
  }
}

bool gtkqt_widget_path_match (GtkWidget *widget, int path_type, const char *pattern)
{
  if (path_type == GTK_PATH_CLASS) return class_match_depth (widget, pattern) >= 0;
  return path_match (widget_path (widget, path_type == GTK_PATH_WIDGET_CLASS), pattern);
}

QList<GtkBindingSet *> gtkqt_css_binding_sets (GtkWidget *widget)
{
  QList<GtkBindingSet *> out;
  auto it = applied ().constFind (widget);
  if (it == applied ().constEnd ()) return out;
  for (const QByteArray &n : it->bindings)
    if (GtkBindingSet *s = gtk_binding_set_find (n.constData ())) out << s;
  return out;
}

/* ---------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */
struct _GtkCssProvider { GObject parent_instance; };
typedef struct { GObjectClass parent_class; } GtkCssProviderClass;
G_DEFINE_TYPE (GtkCssProvider, gtk_css_provider, G_TYPE_OBJECT)
static void gtk_css_provider_init (GtkCssProvider *) {}
static void gtk_css_provider_class_init (GtkCssProviderClass *klass)
{
  g_signal_new ("parsing-error", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr,
                G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_ERROR);
}

static void provider_reload (GtkCssProvider *p, const QByteArray &data, const QString &dir)
{
  CssData *d = css_data (p);
  d->rules.clear ();
  d->colors.clear ();
  d->source = data;
  css_parse (*d, data, dir, 0);
  gtkqt_rc_restyle_all ();
}

extern "C" {

GtkCssProvider *gtk_css_provider_new (void) { return (GtkCssProvider *) g_object_new (gtk_css_provider_get_type (), nullptr); }

gboolean gtk_css_provider_load_from_data (GtkCssProvider *provider, const gchar *data, gssize length, GError **)
{
  g_return_val_if_fail (provider, FALSE);
  provider_reload (provider, data ? (length < 0 ? QByteArray (data) : QByteArray (data, (int) length)) : QByteArray (),
                   QDir::currentPath ());
  return TRUE;
}

void gtk_css_provider_load_from_string (GtkCssProvider *provider, const char *string)
{
  gtk_css_provider_load_from_data (provider, string, -1, nullptr);
}

void gtk_css_provider_load_from_bytes (GtkCssProvider *provider, GBytes *data)
{
  gsize n = 0;
  const char *p = data ? (const char *) g_bytes_get_data (data, &n) : nullptr;
  gtk_css_provider_load_from_data (provider, p, (gssize) n, nullptr);
}

gboolean gtk_css_provider_load_from_path (GtkCssProvider *provider, const char *path, GError **error)
{
  g_return_val_if_fail (provider && path, FALSE);
  QFile f (QString::fromUtf8 (path));
  if (!f.open (QIODevice::ReadOnly)) {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Failed to open %s", path);
    return FALSE;
  }
  provider_reload (provider, f.readAll (), QFileInfo (f).absolutePath ());
  return TRUE;
}

gboolean gtk_css_provider_load_from_file (GtkCssProvider *provider, GFile *file, GError **error)
{
  gchar *path = file ? g_file_get_path (file) : nullptr;
  gboolean ok = path ? gtk_css_provider_load_from_path (provider, path, error) : FALSE;
  g_free (path);
  return ok;
}

void gtk_css_provider_load_from_resource (GtkCssProvider *provider, const char *resource_path)
{
  GBytes *b = g_resources_lookup_data (resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr);
  if (!b) return;
  gtk_css_provider_load_from_bytes (provider, b);
  g_bytes_unref (b);
}

GtkCssProvider *gtk_css_provider_get_default (void)
{
  static GtkCssProvider *p = gtk_css_provider_new ();
  return p;
}

/* named themes are the look: an empty provider */
GtkCssProvider *gtk_css_provider_get_named (const gchar *, const gchar *) { return gtk_css_provider_get_default (); }

char *gtk_css_provider_to_string (GtkCssProvider *provider) { return g_strdup (css_data (provider)->source.constData ()); }

void gtk_style_context_add_provider_for_screen (GdkScreen *, gpointer provider, guint priority)
{
  if (!provider || !G_IS_OBJECT (provider) || !g_type_is_a (G_OBJECT_TYPE (provider), gtk_css_provider_get_type ())) return;
  auto *p = (GtkCssProvider *) provider;
  for (ProviderRef &r : global_providers ()) if (r.provider == p) { r.priority = priority; gtkqt_rc_restyle_all (); return; }
  g_object_ref (p);
  css_data (p);
  global_providers () << ProviderRef { p, priority };
  gtkqt_rc_restyle_all ();
}
void gtk_style_context_add_provider_for_display (GdkDisplay *, gpointer provider, guint priority)
{
  gtk_style_context_add_provider_for_screen (nullptr, provider, priority);
}

void gtk_style_context_remove_provider_for_screen (GdkScreen *, gpointer provider)
{
  for (int i = 0; i < global_providers ().size (); i++)
    if (global_providers ()[i].provider == provider) {
      global_providers ().removeAt (i);
      gtkqt_rc_restyle_all ();
      g_object_unref (provider);
      return;
    }
}
void gtk_style_context_remove_provider_for_display (GdkDisplay *, gpointer provider)
{
  gtk_style_context_remove_provider_for_screen (nullptr, provider);
}

/* a widget's own provider applies to that widget only, as in GTK 3 */
void gtk_style_context_add_provider (GtkStyleContext *context, gpointer provider, guint priority)
{
  auto *w = (GtkWidget *) context;
  if (!w || !provider || !G_IS_OBJECT (provider) || !g_type_is_a (G_OBJECT_TYPE (provider), gtk_css_provider_get_type ())) return;
  QList<ProviderRef> *l = widget_providers (w);
  if (!l) {
    l = new QList<ProviderRef>;
    g_object_set_data_full (G_OBJECT (w), "gtkqt-css-providers", l, [] (gpointer x) {
      auto *list = static_cast<QList<ProviderRef> *> (x);
      for (const ProviderRef &r : *list) g_object_unref (r.provider);
      delete list;
    });
  }
  for (const ProviderRef &r : *l) if (r.provider == provider) return;
  g_object_ref (provider);
  css_data ((GtkCssProvider *) provider);
  *l << ProviderRef { (GtkCssProvider *) provider, priority };
  widget_providers_exist = true;
  if (QWidget *qw = gtkqt_qw (w); qw && qw->isVisible ()) style_tree (w, false);
}

void gtk_style_context_remove_provider (GtkStyleContext *context, gpointer provider)
{
  auto *w = (GtkWidget *) context;
  QList<ProviderRef> *l = w ? widget_providers (w) : nullptr;
  if (!l) return;
  for (int i = 0; i < l->size (); i++)
    if ((*l)[i].provider == provider) { l->removeAt (i); g_object_unref (provider); break; }
  style_tree (w, false);
}

void gtk_style_context_reset_widgets (GdkScreen *) { gtkqt_rc_restyle_all (); }

gboolean gtk_style_context_lookup_color (GtkStyleContext *context, const gchar *color_name, GdkRGBA *color)
{
  QHash<QByteArray, GdkRGBA> local;
  if (auto *w = (GtkWidget *) context) {
    if (QList<ProviderRef> *l = widget_providers (w))
      for (const ProviderRef &r : *l) {
        CssData *d = css_data (r.provider);
        for (auto it = d->colors.begin (); it != d->colors.end (); ++it) local.insert (it.key (), it.value ());
      }
  }
  GdkRGBA c;
  if (!color_name || !parse_color (QByteArray ("@") + color_name, &c, &local)) return FALSE;
  if (color) *color = c;
  return TRUE;
}

void gtk_rc_parse_string (const gchar *rc_string)
{
  if (!rc_string) return;
  rc_parse_data (QByteArray (rc_string), QDir::currentPath (), 0);
  gtkqt_rc_restyle_all ();
}

void gtk_rc_parse (const gchar *filename)
{
  if (!filename) return;
  rc_parse_file (QString::fromUtf8 (filename), 0);
  gtkqt_rc_restyle_all ();
}

void gtk_rc_add_default_file (const gchar *filename)
{
  if (!filename) return;
  rc_default_files () << QString::fromUtf8 (filename);
  if (rc_initialized) gtk_rc_parse (filename);
}

void gtk_rc_set_default_files (gchar **filenames)
{
  rc_default_files ().clear ();
  for (int i = 0; filenames && filenames[i]; i++) rc_default_files () << QString::fromUtf8 (filenames[i]);
}

gchar **gtk_rc_get_default_files (void)
{
  static GPtrArray *a = nullptr;
  if (a) g_ptr_array_free (a, TRUE);
  a = g_ptr_array_new_with_free_func (g_free);
  for (const QString &f : rc_default_files ()) g_ptr_array_add (a, g_strdup (f.toUtf8 ().constData ()));
  g_ptr_array_add (a, nullptr);
  return (gchar **) a->pdata;
}

/* re-read the files that changed since they were read */
gboolean gtk_rc_reparse_all (void)
{
  QStringList changed;
  for (auto it = rc_parsed_files.begin (); it != rc_parsed_files.end (); ++it)
    if (QFileInfo (it.key ()).lastModified () != it.value ()) changed << it.key ();
  if (changed.isEmpty ()) return FALSE;
  for (const QString &f : changed) rc_parse_file (f, 0);
  gtkqt_rc_restyle_all ();
  return TRUE;
}

void gtk_rc_reset_styles (GtkSettings *) { gtkqt_rc_restyle_all (); }

} /* extern "C" */
