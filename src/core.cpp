/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Copyright (C) 2010-2014 Johan Thelin, Robin Burchell and contributors
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version. See COPYING for details.
 *
 * Core: GtkWidget, GtkContainer, GtkBin, the Qt<->GObject bridge, the main
 * loop, properties, string/icon helpers.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QEventLoop>
#include <QKeySequence>
#include <QScreen>
#include <QWindow>
#include <QRegularExpression>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

int gtkqt_api = 3;

/* ======================================================================
 * Enum GTypes (needed for enum properties and GtkBuilder)
 * ====================================================================== */
#define GTKQT_ENUM(func, Name, ...)                                         \
  extern "C" GType func (void) {                                            \
    static GType t = 0;                                                     \
    if (!t) {                                                               \
      static const GEnumValue v[] = { __VA_ARGS__, { 0, nullptr, nullptr } }; \
      t = g_enum_register_static (Name, v);                                 \
    }                                                                       \
    return t;                                                               \
  }
#define EV(val, nick) { val, #val, nick }

GTKQT_ENUM (gtk_align_get_type, "GtkAlign", EV (GTK_ALIGN_FILL, "fill"), EV (GTK_ALIGN_START, "start"),
            EV (GTK_ALIGN_END, "end"), EV (GTK_ALIGN_CENTER, "center"), EV (GTK_ALIGN_BASELINE, "baseline"))
GTKQT_ENUM (gtk_orientation_get_type, "GtkOrientation", EV (GTK_ORIENTATION_HORIZONTAL, "horizontal"),
            EV (GTK_ORIENTATION_VERTICAL, "vertical"))
GTKQT_ENUM (gtk_wrap_mode_get_type, "GtkWrapMode", EV (GTK_WRAP_NONE, "none"), EV (GTK_WRAP_CHAR, "char"),
            EV (GTK_WRAP_WORD, "word"), EV (GTK_WRAP_WORD_CHAR, "word-char"))
GTKQT_ENUM (gtk_justification_get_type, "GtkJustification", EV (GTK_JUSTIFY_LEFT, "left"),
            EV (GTK_JUSTIFY_RIGHT, "right"), EV (GTK_JUSTIFY_CENTER, "center"), EV (GTK_JUSTIFY_FILL, "fill"))
GTKQT_ENUM (gtk_policy_type_get_type, "GtkPolicyType", EV (GTK_POLICY_ALWAYS, "always"),
            EV (GTK_POLICY_AUTOMATIC, "automatic"), EV (GTK_POLICY_NEVER, "never"), EV (GTK_POLICY_EXTERNAL, "external"))
GTKQT_ENUM (gtk_position_type_get_type, "GtkPositionType", EV (GTK_POS_LEFT, "left"), EV (GTK_POS_RIGHT, "right"),
            EV (GTK_POS_TOP, "top"), EV (GTK_POS_BOTTOM, "bottom"))
GTKQT_ENUM (gtk_selection_mode_get_type, "GtkSelectionMode", EV (GTK_SELECTION_NONE, "none"),
            EV (GTK_SELECTION_SINGLE, "single"), EV (GTK_SELECTION_BROWSE, "browse"), EV (GTK_SELECTION_MULTIPLE, "multiple"))
GTKQT_ENUM (gtk_window_position_get_type, "GtkWindowPosition", EV (GTK_WIN_POS_NONE, "none"),
            EV (GTK_WIN_POS_CENTER, "center"), EV (GTK_WIN_POS_MOUSE, "mouse"),
            EV (GTK_WIN_POS_CENTER_ALWAYS, "center-always"), EV (GTK_WIN_POS_CENTER_ON_PARENT, "center-on-parent"))
GTKQT_ENUM (gtk_shadow_type_get_type, "GtkShadowType", EV (GTK_SHADOW_NONE, "none"), EV (GTK_SHADOW_IN, "in"),
            EV (GTK_SHADOW_OUT, "out"), EV (GTK_SHADOW_ETCHED_IN, "etched-in"), EV (GTK_SHADOW_ETCHED_OUT, "etched-out"))
GTKQT_ENUM (gtk_relief_style_get_type, "GtkReliefStyle", EV (GTK_RELIEF_NORMAL, "normal"),
            EV (GTK_RELIEF_HALF, "half"), EV (GTK_RELIEF_NONE, "none"))
GTKQT_ENUM (gtk_message_type_get_type, "GtkMessageType", EV (GTK_MESSAGE_INFO, "info"),
            EV (GTK_MESSAGE_WARNING, "warning"), EV (GTK_MESSAGE_QUESTION, "question"),
            EV (GTK_MESSAGE_ERROR, "error"), EV (GTK_MESSAGE_OTHER, "other"))
GTKQT_ENUM (gtk_buttons_type_get_type, "GtkButtonsType", EV (GTK_BUTTONS_NONE, "none"), EV (GTK_BUTTONS_OK, "ok"),
            EV (GTK_BUTTONS_CLOSE, "close"), EV (GTK_BUTTONS_CANCEL, "cancel"), EV (GTK_BUTTONS_YES_NO, "yes-no"),
            EV (GTK_BUTTONS_OK_CANCEL, "ok-cancel"))
GTKQT_ENUM (gtk_stack_transition_type_get_type, "GtkStackTransitionType", EV (GTK_STACK_TRANSITION_TYPE_NONE, "none"),
            EV (GTK_STACK_TRANSITION_TYPE_CROSSFADE, "crossfade"), EV (GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT, "slide-right"),
            EV (GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT, "slide-left"), EV (GTK_STACK_TRANSITION_TYPE_SLIDE_UP, "slide-up"),
            EV (GTK_STACK_TRANSITION_TYPE_SLIDE_DOWN, "slide-down"),
            EV (GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT, "slide-left-right"),
            EV (GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN, "slide-up-down"))
GTKQT_ENUM (gtk_license_get_type, "GtkLicense", EV (GTK_LICENSE_UNKNOWN, "unknown"), EV (GTK_LICENSE_CUSTOM, "custom"),
            EV (GTK_LICENSE_GPL_2_0, "gpl-2-0"), EV (GTK_LICENSE_GPL_3_0, "gpl-3-0"), EV (GTK_LICENSE_LGPL_2_1, "lgpl-2-1"),
            EV (GTK_LICENSE_LGPL_3_0, "lgpl-3-0"), EV (GTK_LICENSE_BSD, "bsd"), EV (GTK_LICENSE_MIT_X11, "mit-x11"),
            EV (GTK_LICENSE_ARTISTIC, "artistic"), EV (GTK_LICENSE_GPL_2_0_ONLY, "gpl-2-0-only"),
            EV (GTK_LICENSE_GPL_3_0_ONLY, "gpl-3-0-only"), EV (GTK_LICENSE_LGPL_2_1_ONLY, "lgpl-2-1-only"),
            EV (GTK_LICENSE_LGPL_3_0_ONLY, "lgpl-3-0-only"), EV (GTK_LICENSE_AGPL_3_0, "agpl-3-0"),
            EV (GTK_LICENSE_AGPL_3_0_ONLY, "agpl-3-0-only"), EV (GTK_LICENSE_BSD_3, "bsd-3"),
            EV (GTK_LICENSE_APACHE_2_0, "apache-2-0"), EV (GTK_LICENSE_MPL_2_0, "mpl-2-0"))
GTKQT_ENUM (gtk_input_purpose_get_type, "GtkInputPurpose", EV (GTK_INPUT_PURPOSE_FREE_FORM, "free-form"),
            EV (GTK_INPUT_PURPOSE_ALPHA, "alpha"), EV (GTK_INPUT_PURPOSE_DIGITS, "digits"),
            EV (GTK_INPUT_PURPOSE_NUMBER, "number"), EV (GTK_INPUT_PURPOSE_PHONE, "phone"),
            EV (GTK_INPUT_PURPOSE_URL, "url"), EV (GTK_INPUT_PURPOSE_EMAIL, "email"), EV (GTK_INPUT_PURPOSE_NAME, "name"),
            EV (GTK_INPUT_PURPOSE_PASSWORD, "password"), EV (GTK_INPUT_PURPOSE_PIN, "pin"))
GTKQT_ENUM (gtk_window_type_get_type, "GtkWindowType", EV (GTK_WINDOW_TOPLEVEL, "toplevel"), EV (GTK_WINDOW_POPUP, "popup"))
GTKQT_ENUM (gtk_pack_type_get_type, "GtkPackType", EV (GTK_PACK_START, "start"), EV (GTK_PACK_END, "end"))
GTKQT_ENUM (gtk_icon_size_get_type, "GtkIconSize", EV (GTK_ICON_SIZE_INVALID, "invalid"), EV (GTK_ICON_SIZE_MENU, "menu"),
            EV (GTK_ICON_SIZE_SMALL_TOOLBAR, "small-toolbar"), EV (GTK_ICON_SIZE_LARGE_TOOLBAR, "large-toolbar"),
            EV (GTK_ICON_SIZE_BUTTON, "button"), EV (GTK_ICON_SIZE_DND, "dnd"), EV (GTK_ICON_SIZE_DIALOG, "dialog"))
GTKQT_ENUM (gtk_file_chooser_action_get_type, "GtkFileChooserAction", EV (GTK_FILE_CHOOSER_ACTION_OPEN, "open"),
            EV (GTK_FILE_CHOOSER_ACTION_SAVE, "save"), EV (GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "select-folder"),
            EV (GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, "create-folder"))

/* ======================================================================
 * Deferred deletion of Qt objects
 * ====================================================================== */
static int s_emit_depth = 0;
static QList<QPointer<QObject>> s_pending_deletes;
static guint s_delete_idle = 0;

static gboolean process_deletes (gpointer)
{
  s_delete_idle = 0;
  if (s_emit_depth > 0)
    return G_SOURCE_REMOVE;       /* rescheduled when the outermost callback returns */
  QList<QPointer<QObject>> list;
  list.swap (s_pending_deletes);
  for (const QPointer<QObject> &p : list)
    if (p) delete p.data ();
  return G_SOURCE_REMOVE;
}

static void schedule_deletes ()
{
  if (!s_delete_idle && !s_pending_deletes.isEmpty () && s_emit_depth == 0)
    s_delete_idle = g_idle_add_full (G_PRIORITY_HIGH_IDLE, process_deletes, nullptr, nullptr);
}

void gtkqt_defer_delete (QObject *obj)
{
  if (!obj) return;
  if (QWidget *w = qobject_cast<QWidget *> (obj))
    w->hide ();
  s_pending_deletes.append (obj);
  schedule_deletes ();
}

GtkQtEmitGuard::GtkQtEmitGuard () { s_emit_depth++; }
GtkQtEmitGuard::~GtkQtEmitGuard ()
{
  if (--s_emit_depth == 0)
    schedule_deletes ();
}

/* ======================================================================
 * Initialisation and main loop
 * ====================================================================== */
static QList<QEventLoop *> s_loops;
static bool s_initialized = false;

static gboolean autoquit_cb (gpointer)
{
  /* Test/screenshot helper: GTKQT_SCREENSHOT=file.png GTKQT_AUTOQUIT=ms */
  const char *shot = g_getenv ("GTKQT_SCREENSHOT");
  if (shot && *shot) {
    int n = 0;
    for (QWidget *tl : QApplication::topLevelWidgets ()) {
      if (!tl->isVisible () || qobject_cast<QMenu *> (tl)) continue;
      QString name = QString::fromUtf8 (shot);
      if (n > 0) {
        int dot = name.lastIndexOf ('.');
        name.insert (dot < 0 ? name.size () : dot, QStringLiteral ("-%1").arg (n));
      }
      tl->grab ().save (name);
      n++;
    }
  }
  fflush (stdout);
  fflush (stderr);
  _exit (0);
  return G_SOURCE_REMOVE;
}

void gtkqt_script_start ();

static void setup_app ()
{
  gtkqt_script_start ();
  qApp->setQuitOnLastWindowClosed (false);
  if (g_get_prgname ())
    QCoreApplication::setApplicationName (QString::fromUtf8 (g_get_prgname ()));
  if (const char *ms = g_getenv ("GTKQT_AUTOQUIT"))
    g_timeout_add (atoi (ms), autoquit_cb, nullptr);
  if (!QAbstractEventDispatcher::instance ()->inherits ("QEventDispatcherGlib") &&
      !QAbstractEventDispatcher::instance ()->inherits ("QPAEventDispatcherGlib"))
    g_warning ("gtk-made-qt: Qt is not using the GLib event loop (QT_NO_GLIB set?); "
               "g_timeout_add() and friends will not run");
}

/* Qt's gtk2/gtk3 platform-theme plugins load the real GTK into the process,
 * whose GObject types (GtkWidget ...) would collide with ours. Use Qt's own
 * GNOME theme instead where Qt would pick them: when asked for explicitly, or
 * by default on GNOME-family desktops. */
static void avoid_real_gtk_theme ()
{
  QByteArray theme = qgetenv ("QT_QPA_PLATFORMTHEME").toLower ();
  QByteArray desktop = qgetenv ("XDG_CURRENT_DESKTOP").toLower ();
  bool gtk_desktop = false;
  for (const char *d : { "gnome", "unity", "xfce", "mate", "cinnamon", "budgie", "pantheon", "lxde" })
    if (desktop.contains (d)) gtk_desktop = true;
  if (theme.contains ("gtk") || (theme.isEmpty () && gtk_desktop)) {
    if (theme.contains ("gtk"))
      g_message ("gtk-made-qt: Qt's %s platform theme would load the real GTK; using Qt's gnome theme", theme.constData ());
    qputenv ("QT_QPA_PLATFORMTHEME", "gnome");
  }
}

void gtkqt_ensure_app (void)
{
  if (QCoreApplication::instance ()) {
    if (!s_initialized) { s_initialized = true; setup_app (); }
    return;
  }
  static int argc = 1;
  static char *argv[] = { g_strdup (g_get_prgname () ? g_get_prgname () : "gtk-made-qt"), nullptr };
  avoid_real_gtk_theme ();
  new QApplication (argc, argv);
  s_initialized = true;
  setup_app ();
}

extern "C" {

gboolean gtkqt_init_check (int *argc, char ***argv, int api_version)
{
  if (api_version >= 1 && api_version <= 4)
    gtkqt_api = api_version;
  if (argc && argv && *argv && *argc > 0 && !g_get_prgname ()) {
    gchar *base = g_path_get_basename ((*argv)[0]);
    g_set_prgname (base);
    g_free (base);
  }
  if (!QCoreApplication::instance ()) {
    avoid_real_gtk_theme ();
    if (argc && argv && *argv)
      new QApplication (*argc, *argv);
    else
      gtkqt_ensure_app ();
  }
  gtkqt_ensure_app ();
  return TRUE;
}

void gtkqt_init (int *argc, char ***argv, int api_version) { gtkqt_init_check (argc, argv, api_version); }
void gtk_init (int *argc, char ***argv) { gtkqt_init_check (argc, argv, gtkqt_api); }
gboolean gtk_init_check (int *argc, char ***argv) { return gtkqt_init_check (argc, argv, gtkqt_api); }
gboolean gtk_is_initialized (void) { return QCoreApplication::instance () != nullptr; }
void gtk_disable_setlocale (void) {}

void gtk_main (void)
{
  gtkqt_ensure_app ();
  QEventLoop loop;
  s_loops.append (&loop);
  loop.exec ();
  s_loops.removeOne (&loop);
}

void gtk_main_quit (void)
{
  if (!s_loops.isEmpty ())
    s_loops.last ()->quit ();
  else
    g_warning ("gtk_main_quit: no main loop running");
}

guint gtk_main_level (void) { return s_loops.size (); }
gboolean gtk_events_pending (void) { return g_main_context_pending (nullptr); }
gboolean gtk_main_iteration (void) { g_main_context_iteration (nullptr, TRUE); return FALSE; }
gboolean gtk_main_iteration_do (gboolean blocking) { g_main_context_iteration (nullptr, blocking); return FALSE; }
gboolean gtk_true (void) { return TRUE; }
gboolean gtk_false (void) { return FALSE; }
guint gtk_get_major_version (void) { return gtkqt_api; }
guint gtk_get_minor_version (void) { return gtkqt_api == 4 ? 14 : 24; }
guint gtk_get_micro_version (void) { return 0; }
const gchar *gtk_check_version (guint major, guint, guint)
{
  return major >= 2 && major <= 4 ? nullptr : "gtk-made-qt implements GTK 2, 3 and 4 APIs only";
}

} /* extern "C" */

/* ======================================================================
 * Strings, icons, accelerators
 * ====================================================================== */
QString gtkqt_str (const gchar *s) { return s ? QString::fromUtf8 (s) : QString (); }

const char *gtkqt_ret (GtkWidget *w, const QString &s)
{
  w->priv->scratch = s.toUtf8 ();
  return w->priv->scratch.constData ();
}

QString gtkqt_mnemonic (const gchar *s)
{
  QString in = gtkqt_str (s), out;
  for (int i = 0; i < in.size (); i++) {
    QChar c = in[i];
    if (c == '&') out += QStringLiteral ("&&");
    else if (c == '_' && i + 1 < in.size () && in[i + 1] == '_') { out += '_'; i++; }
    else if (c == '_') out += '&';
    else out += c;
  }
  return out;
}

QString gtkqt_markup_to_html (const gchar *markup)
{
  /* Pango markup is close to HTML. Translate the <span> attributes that
   * differ and wrap in <qt> so QLabel takes it as rich text. */
  QString s = gtkqt_str (markup);
  static const QRegularExpression span (QStringLiteral ("<span([^>]*)>"));
  static const QRegularExpression attr (QStringLiteral ("([a-z_]+)\\s*=\\s*[\"']([^\"']*)[\"']"));
  QString out;
  int last = 0;
  auto it = span.globalMatch (s);
  while (it.hasNext ()) {
    auto m = it.next ();
    out += s.mid (last, m.capturedStart () - last);
    QStringList css;
    auto ai = attr.globalMatch (m.captured (1));
    while (ai.hasNext ()) {
      auto am = ai.next ();
      QString k = am.captured (1), v = am.captured (2);
      if (k == "foreground" || k == "fgcolor" || k == "color") css << "color:" + v;
      else if (k == "background" || k == "bgcolor") css << "background-color:" + v;
      else if (k == "weight") css << "font-weight:" + (v == "bold" || v == "heavy" ? QString ("bold") : v);
      else if (k == "style") css << "font-style:" + v;
      else if (k == "font_family" || k == "face") css << "font-family:" + v;
      else if (k == "underline" && v != "none") css << "text-decoration:underline";
      else if (k == "strikethrough" && v == "true") css << "text-decoration:line-through";
      else if (k == "size" || k == "font_size") {
        if (v == "x-large" || v == "xx-large" || v == "large" || v == "small" || v == "x-small" || v == "medium")
          css << "font-size:" + v;
        else if (v.endsWith ("pt")) css << "font-size:" + v;
        else if (v.toInt () > 0) css << QStringLiteral ("font-size:%1pt").arg (v.toInt () / 1024);
      }
    }
    out += QStringLiteral ("<span style=\"%1\">").arg (css.join (';'));
    last = m.capturedEnd ();
  }
  out += s.mid (last);
  out.replace (QStringLiteral ("<tt>"), QStringLiteral ("<code>")).replace (QStringLiteral ("</tt>"), QStringLiteral ("</code>"));
  out.replace ('\n', QStringLiteral ("<br>"));
  return QStringLiteral ("<qt>") + out + QStringLiteral ("</qt>");
}

struct StockItem { const char *id; const char *label; const char *icon; };
static const StockItem stock_items[] = {
  { "gtk-about", "_About", "help-about" }, { "gtk-add", "_Add", "list-add" },
  { "gtk-apply", "_Apply", "dialog-ok-apply" }, { "gtk-bold", "_Bold", "format-text-bold" },
  { "gtk-cancel", "_Cancel", "dialog-cancel" }, { "gtk-clear", "_Clear", "edit-clear" },
  { "gtk-close", "_Close", "window-close" }, { "gtk-copy", "_Copy", "edit-copy" },
  { "gtk-cut", "Cu_t", "edit-cut" }, { "gtk-delete", "_Delete", "edit-delete" },
  { "gtk-dialog-error", "Error", "dialog-error" }, { "gtk-dialog-info", "Information", "dialog-information" },
  { "gtk-dialog-question", "Question", "dialog-question" }, { "gtk-dialog-warning", "Warning", "dialog-warning" },
  { "gtk-edit", "_Edit", "document-edit" }, { "gtk-execute", "_Execute", "system-run" },
  { "gtk-find", "_Find", "edit-find" }, { "gtk-find-and-replace", "Find and _Replace", "edit-find-replace" },
  { "gtk-go-back", "_Back", "go-previous" }, { "gtk-go-down", "_Down", "go-down" },
  { "gtk-go-forward", "_Forward", "go-next" }, { "gtk-go-up", "_Up", "go-up" },
  { "gtk-help", "_Help", "help-browser" }, { "gtk-home", "_Home", "go-home" },
  { "gtk-info", "_Information", "dialog-information" }, { "gtk-italic", "_Italic", "format-text-italic" },
  { "gtk-new", "_New", "document-new" }, { "gtk-no", "_No", "dialog-cancel" },
  { "gtk-ok", "_OK", "dialog-ok" }, { "gtk-open", "_Open", "document-open" },
  { "gtk-paste", "_Paste", "edit-paste" }, { "gtk-preferences", "_Preferences", "preferences-system" },
  { "gtk-print", "_Print", "document-print" }, { "gtk-properties", "_Properties", "document-properties" },
  { "gtk-quit", "_Quit", "application-exit" }, { "gtk-redo", "_Redo", "edit-redo" },
  { "gtk-refresh", "_Refresh", "view-refresh" }, { "gtk-remove", "_Remove", "list-remove" },
  { "gtk-save", "_Save", "document-save" }, { "gtk-save-as", "Save _As", "document-save-as" },
  { "gtk-select-all", "Select _All", "edit-select-all" }, { "gtk-stop", "_Stop", "process-stop" },
  { "gtk-underline", "_Underline", "format-text-underline" }, { "gtk-undo", "_Undo", "edit-undo" },
  { "gtk-yes", "_Yes", "dialog-ok" }, { "gtk-zoom-in", "Zoom _In", "zoom-in" },
  { "gtk-zoom-out", "Zoom _Out", "zoom-out" },
};

static const StockItem stock_items2[] = {
  { "gtk-print-preview", "Print Pre_view", "document-print-preview" }, { "gtk-jump-to", "_Jump to", "go-jump" },
  { "gtk-select-font", "_Font", "preferences-desktop-font" }, { "gtk-page-setup", "Page Set_up", "document-page-setup" },
  { "gtk-revert-to-saved", "_Revert", "document-revert" }, { "gtk-goto-first", "_First", "go-first" },
  { "gtk-goto-last", "_Last", "go-last" }, { "gtk-goto-top", "_Top", "go-top" }, { "gtk-goto-bottom", "_Bottom", "go-bottom" },
  { "gtk-index", "_Index", "help-contents" }, { "gtk-indent", "Increase Indent", "format-indent-more" },
  { "gtk-unindent", "Decrease Indent", "format-indent-less" }, { "gtk-convert", "_Convert", "view-refresh" },
  { "gtk-spell-check", "_Spell Check", "tools-check-spelling" }, { "gtk-fullscreen", "_Fullscreen", "view-fullscreen" },
  { "gtk-leave-fullscreen", "_Leave Fullscreen", "view-restore" }, { "gtk-media-play", "P_lay", "media-playback-start" },
  { "gtk-media-pause", "P_ause", "media-playback-pause" }, { "gtk-media-stop", "_Stop", "media-playback-stop" },
  { "gtk-media-next", "_Next", "media-skip-forward" }, { "gtk-media-previous", "P_revious", "media-skip-backward" },
  { "gtk-directory", "Directory", "folder" }, { "gtk-file", "File", "text-x-generic" },
  { "gtk-harddisk", "_Harddisk", "drive-harddisk" }, { "gtk-network", "_Network", "network-workgroup" },
  { "gtk-color-picker", "Color Picker", "color-picker" }, { "gtk-sort-ascending", "_Ascending", "view-sort-ascending" },
  { "gtk-sort-descending", "_Descending", "view-sort-descending" }, { "gtk-strikethrough", "_Strikethrough", "format-text-strikethrough" },
  { "gtk-justify-left", "_Left", "format-justify-left" }, { "gtk-justify-right", "_Right", "format-justify-right" },
  { "gtk-justify-center", "_Center", "format-justify-center" }, { "gtk-justify-fill", "_Fill", "format-justify-fill" },
  { "gtk-missing-image", "Missing image", "image-missing" }, { "gtk-discard", "_Discard", "edit-clear" },
  { "gtk-connect", "_Connect", "network-connect" }, { "gtk-disconnect", "_Disconnect", "network-disconnect" },
  { "gtk-dnd", "Drag and drop", "text-x-generic" }, { "gtk-caps-lock-warning", "Caps Lock is on", "dialog-warning" },
};

/* stock items and icons registered by the program (gtk_stock_add, icon factories) */
QHash<QString, QByteArray> gtkqt_user_stock_labels;
QHash<QString, QIcon> gtkqt_user_icons;

static const StockItem *find_stock (const gchar *id)
{
  if (!id) return nullptr;
  if (gtkqt_user_stock_labels.contains (QString::fromUtf8 (id))) {
    static StockItem user;
    static QByteArray keep_id, keep_label;
    keep_id = id;
    keep_label = gtkqt_user_stock_labels.value (QString::fromUtf8 (id));
    const StockItem *builtin = nullptr;
    for (const StockItem &s : stock_items)
      if (!strcmp (s.id, id)) builtin = &s;
    for (const StockItem &s : stock_items2)
      if (!strcmp (s.id, id)) builtin = &s;
    user = { keep_id.constData (), keep_label.constData (), builtin ? builtin->icon : keep_id.constData () };
    return &user;
  }
  for (const StockItem &s : stock_items)
    if (!strcmp (s.id, id)) return &s;
  for (const StockItem &s : stock_items2)
    if (!strcmp (s.id, id)) return &s;
  return nullptr;
}

QString gtkqt_stock_label (const gchar *stock_id)
{
  const StockItem *s = find_stock (stock_id);
  return s ? gtkqt_mnemonic (s->label) : QString ();
}

QIcon gtkqt_icon (const gchar *icon_name)
{
  if (!icon_name || !*icon_name) return QIcon ();
  if (gtkqt_user_icons.contains (QString::fromUtf8 (icon_name))) return gtkqt_user_icons.value (QString::fromUtf8 (icon_name));
  if (const StockItem *s = find_stock (icon_name))
    icon_name = s->icon;
  QString name = QString::fromUtf8 (icon_name);
  if (name.startsWith ('/') && QFile::exists (name))
    return QIcon (name);
  QIcon icon = QIcon::fromTheme (name);
  if (!icon.isNull ()) return icon;
  if (name.endsWith (QLatin1String ("-symbolic"))) {
    icon = QIcon::fromTheme (name.chopped (9));
    if (!icon.isNull ()) return icon;
  }
  /* Fall back on Qt's built-in standard pixmaps for the common ones. */
  static const struct { const char *name; QStyle::StandardPixmap sp; } fallbacks[] = {
    { "dialog-information", QStyle::SP_MessageBoxInformation }, { "dialog-warning", QStyle::SP_MessageBoxWarning },
    { "dialog-error", QStyle::SP_MessageBoxCritical }, { "dialog-question", QStyle::SP_MessageBoxQuestion },
    { "document-open", QStyle::SP_DialogOpenButton }, { "document-save", QStyle::SP_DialogSaveButton },
    { "document-new", QStyle::SP_FileIcon }, { "folder", QStyle::SP_DirIcon },
    { "window-close", QStyle::SP_DialogCloseButton }, { "application-exit", QStyle::SP_DialogCloseButton },
    { "dialog-ok", QStyle::SP_DialogOkButton }, { "dialog-cancel", QStyle::SP_DialogCancelButton },
    { "go-previous", QStyle::SP_ArrowBack }, { "go-next", QStyle::SP_ArrowForward },
    { "go-up", QStyle::SP_ArrowUp }, { "go-down", QStyle::SP_ArrowDown }, { "go-home", QStyle::SP_DirHomeIcon },
    { "view-refresh", QStyle::SP_BrowserReload }, { "process-stop", QStyle::SP_BrowserStop },
    { "help-about", QStyle::SP_MessageBoxInformation }, { "help-browser", QStyle::SP_DialogHelpButton },
    { "edit-delete", QStyle::SP_TrashIcon }, { "user-trash", QStyle::SP_TrashIcon },
    { "media-playback-start", QStyle::SP_MediaPlay }, { "media-playback-pause", QStyle::SP_MediaPause },
    { "media-playback-stop", QStyle::SP_MediaStop }, { "computer", QStyle::SP_ComputerIcon },
  };
  QString base = name;
  if (base.endsWith (QLatin1String ("-symbolic"))) base.chop (9);
  for (const auto &f : fallbacks)
    if (base == QLatin1String (f.name))
      return QApplication::style ()->standardIcon (f.sp);
  return QIcon ();
}

int gtkqt_icon_size_px (GtkIconSize size)
{
  switch (size) {
  case GTK_ICON_SIZE_MENU: case GTK_ICON_SIZE_SMALL_TOOLBAR: case GTK_ICON_SIZE_BUTTON: return 16;
  case GTK_ICON_SIZE_LARGE_TOOLBAR: return 24;
  case GTK_ICON_SIZE_DND: return 32;
  case GTK_ICON_SIZE_DIALOG: return 48;
  default: return 16;
  }
}

QKeySequence gtkqt_accel_to_keyseq (const gchar *accel)
{
  if (!accel || !*accel) return QKeySequence ();
  QString s = QString::fromUtf8 (accel), out;
  static const QRegularExpression mod (QStringLiteral ("^<([A-Za-z0-9]+)>"));
  QRegularExpressionMatch m;
  while ((m = mod.match (s)).hasMatch ()) {
    QString k = m.captured (1).toLower ();
    if (k == "control" || k == "ctrl" || k == "primary" || k == "ctl") out += "Ctrl+";
    else if (k == "shift" || k == "shft") out += "Shift+";
    else if (k == "alt" || k == "mod1") out += "Alt+";
    else if (k == "super" || k == "meta") out += "Meta+";
    s = s.mid (m.capturedLength ());
  }
  static const QMap<QString, QString> names = {
    { "Return", "Return" }, { "KP_Enter", "Enter" }, { "Escape", "Esc" }, { "BackSpace", "Backspace" },
    { "Delete", "Del" }, { "Insert", "Ins" }, { "Page_Up", "PgUp" }, { "Page_Down", "PgDown" },
    { "plus", "+" }, { "minus", "-" }, { "equal", "=" }, { "comma", "," }, { "period", "." },
    { "slash", "/" }, { "question", "?" }, { "space", "Space" }, { "Tab", "Tab" },
  };
  out += names.value (s, s.size () == 1 ? s.toUpper () : s);
  return QKeySequence (out);
}

/* ======================================================================
 * Properties
 * ====================================================================== */
struct PropEntry { GtkQtPropSet set; GtkQtPropGet get; };
static GQuark prop_quark ()
{
  static GQuark q = g_quark_from_static_string ("gtkqt-prop-entry");
  return q;
}

static void generic_set_property (GObject *o, guint id, const GValue *v, GParamSpec *ps)
{
  auto *e = static_cast<PropEntry *> (g_param_spec_get_qdata (ps, prop_quark ()));
  if (e && e->set) e->set (o, v);
  else G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, ps);
}

static void generic_get_property (GObject *o, guint id, GValue *v, GParamSpec *ps)
{
  auto *e = static_cast<PropEntry *> (g_param_spec_get_qdata (ps, prop_quark ()));
  if (e && e->get) e->get (o, v);
  else if (!e) G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, ps);
}

void gtkqt_prop (gpointer klass, GParamSpec *pspec, GtkQtPropSet set, GtkQtPropGet get)
{
  static guint next_id = 1;
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  oc->set_property = generic_set_property;
  oc->get_property = generic_get_property;
  g_param_spec_set_qdata (pspec, prop_quark (), new PropEntry { set, get });
  g_object_class_install_property (oc, next_id++, pspec);
}

/* ======================================================================
 * GtkWidget
 * ====================================================================== */
enum {
  SIG_DESTROY, SIG_SHOW, SIG_HIDE, SIG_MAP, SIG_UNMAP, SIG_REALIZE, SIG_UNREALIZE, SIG_SIZE_ALLOCATE,
  SIG_DRAW, SIG_EVENT, SIG_EVENT_AFTER, SIG_BUTTON_PRESS, SIG_BUTTON_RELEASE, SIG_SCROLL, SIG_MOTION,
  SIG_DELETE, SIG_KEY_PRESS, SIG_KEY_RELEASE, SIG_ENTER, SIG_LEAVE, SIG_CONFIGURE, SIG_FOCUS_IN,
  SIG_FOCUS_OUT, SIG_EXPOSE, SIG_GRAB_FOCUS, SIG_STATE_FLAGS_CHANGED, N_WIDGET_SIGNALS
};
static guint widget_signals[N_WIDGET_SIGNALS];

G_DEFINE_TYPE (GtkWidget, gtk_widget, G_TYPE_INITIALLY_UNOWNED)

GtkWidgetPrivate *gtkqt_priv (GtkWidget *w) { return w->priv; }

GtkQtBridge *gtkqt_bridge (GtkWidget *w)
{
  if (!w->priv->bridge)
    w->priv->bridge = new GtkQtBridge (w);
  return w->priv->bridge;
}

void gtkqt_set_create (GtkWidgetClass *klass, GtkQtCreateFunc create)
{
  klass->gtkqt_create = (gpointer) create;
}

/* Program-defined widget classes that draw themselves (GTK 1/2 custom
 * widgets overriding expose_event, size_request or draw) get a canvas. */
static gboolean widget_class_draws (GtkWidget *w)
{
  auto *k = GTK_WIDGET_GET_CLASS (w);
  auto *base = (GtkWidgetClass *) g_type_class_peek (GTK_TYPE_WIDGET);
  return k->expose_event != base->expose_event || k->size_request != base->size_request || k->draw != base->draw;
}

static QObject *widget_default_create (GtkWidget *w)
{
  if (widget_class_draws (w)) return gtkqt_canvas_create (w);
  return new QWidget;
}

static void widget_default_realize (GtkWidget *w)
{
  w->flags |= GTK_REALIZED;
  gtkqt_gdk_window (w);
}

/* GTK emits realize before a widget is first drawn; GTK 1/2 custom widgets
 * create their GdkWindow there. */
static void ensure_realized (GtkWidget *w)
{
  if (!w || (w->flags & GTK_REALIZED) || !w->priv || w->priv->destroyed) return;
  if (w->parent && !(w->parent->flags & GTK_REALIZED)) ensure_realized (w->parent);
  w->flags |= GTK_REALIZED;
  g_signal_emit (w, widget_signals[SIG_REALIZE], 0);
}

QObject *gtkqt_obj (GtkWidget *w)
{
  if (!w || !w->priv) return nullptr;
  GtkWidgetPrivate *p = w->priv;
  if (p->obj || p->destroyed || p->creating) return p->obj.data ();
  if (p->created) return nullptr;   /* the Qt object was deleted behind our back */
  gtkqt_ensure_app ();
  p->creating = true;
  auto create = (GtkQtCreateFunc) GTK_WIDGET_GET_CLASS (w)->gtkqt_create;
  QObject *o = create ? create (w) : widget_default_create (w);
  p->creating = false;
  p->created = true;
  p->obj = o;
  o->setProperty ("_gtkqt_widget", QVariant::fromValue ((void *) w));
  GtkQtBridge *b = gtkqt_bridge (w);
  b->setParent (o);
  o->installEventFilter (b);
  if (QWidget *qw = qobject_cast<QWidget *> (o)) {
    if (!p->sensitive) qw->setEnabled (false);
    gtkqt_apply_size_request (w);
    if (!p->css_classes.isEmpty ()) {
      QStringList c = p->css_classes;
      p->css_classes.clear ();
      for (const QString &cls : c) gtk_widget_add_css_class (w, cls.toUtf8 ().constData ());
    }
  }
  w->window = gtkqt_gdk_window (w);
  return o;
}

QWidget *gtkqt_qw (GtkWidget *w) { return qobject_cast<QWidget *> (gtkqt_obj (w)); }

GtkWidget *gtkqt_from_qobject (QObject *obj)
{
  while (obj) {
    QVariant v = obj->property ("_gtkqt_widget");
    if (v.isValid ()) return static_cast<GtkWidget *> (v.value<void *> ());
    obj = obj->parent ();
  }
  return nullptr;
}

static gboolean is_toplevel_type (GtkWidget *w)
{
  return GTK_IS_WINDOW (w);
}

void gtkqt_sync_visible (GtkWidget *w)
{
  QObject *o = gtkqt_obj (w);
  if (!o) return;
  bool vis = w->priv->visible;
  if (auto *va = qobject_cast<QAction *> (o->property ("_gtkqt_vis_action").value<QObject *> ())) {
    va->setVisible (vis);   /* widgets inside a QToolBar are shown/hidden through their action */
    return;
  }
  if (QWidget *qw = qobject_cast<QWidget *> (o)) {
    if (!qw->parentWidget () && !is_toplevel_type (w)) {
      if (!vis) qw->hide ();
      return;   /* not placed yet; it gets synced when added to a container */
    }
    /* Pages of a QStackedWidget (GtkStack, GtkNotebook) are shown and hidden
     * by the stack itself; forcing them visible would overlap the pages. */
    if (auto *stack = qobject_cast<QStackedWidget *> (qw->parentWidget ())) {
      if (!vis) qw->hide ();
      else if (stack->currentWidget () == qw) qw->show ();   /* added hidden, shown later (show_all) */
      return;
    }
    if (vis && qw->isWindow () && GTK_IS_WINDOW (w))
      gtkqt_window_before_show (w);
    if (qw->isVisible () != vis || qw->isHidden () == vis)
      qw->setVisible (vis);
  } else if (QAction *a = qobject_cast<QAction *> (o)) {
    a->setVisible (vis);
  }
}

void gtkqt_apply_size_request (GtkWidget *w)
{
  QWidget *qw = qobject_cast<QWidget *> (w->priv->obj.data ());
  if (!qw) return;
  GtkWidgetPrivate *p = w->priv;
  qw->setMinimumSize (QSize (p->width_request > 0 ? p->width_request : 0,
                             p->height_request > 0 ? p->height_request : 0));
  /* a fixed-size window without children (it is drawn on): the request is its size */
  if (qw->isWindow () && g_object_get_data (G_OBJECT (w), "gtkqt-fixed-size") && GTK_IS_CONTAINER (w) &&
      !GTK_BIN (w)->child && p->width_request > 0 && p->height_request > 0) {
    if (qw->layout ()) qw->layout ()->setSizeConstraint (QLayout::SetDefaultConstraint);   /* an empty layout would make it 0x0 */
    qw->setFixedSize (p->width_request, p->height_request);
  }
}

static void gtk_widget_init (GtkWidget *w)
{
  w->priv = new _GtkWidgetPrivate;
  /* GTK 4 widgets are visible by default (toplevels are not). */
  w->priv->visible = gtkqt_api >= 4;
  w->flags = GTK_SENSITIVE | GTK_PARENT_SENSITIVE;
  w->style = gtkqt_default_style_ref ();
}

static void gtk_widget_constructed (GObject *o)
{
  G_OBJECT_CLASS (gtk_widget_parent_class)->constructed (o);
  gtkqt_obj (GTK_WIDGET (o));
}

static void gtk_widget_real_destroy (GtkWidget *)
{
}

static void gtk_widget_dispose (GObject *o)
{
  GtkWidget *w = GTK_WIDGET (o);
  GtkWidgetPrivate *p = w->priv;
  if (!p->in_destruction) {
    p->in_destruction = true;
    g_signal_emit (w, widget_signals[SIG_DESTROY], 0);   /* containers destroy their children here */
    if (w->parent && GTK_IS_CONTAINER (w->parent))
      gtk_container_remove (GTK_CONTAINER (w->parent), w);
    for (GList *l = p->controllers; l; l = l->next)
      g_object_unref (l->data);
    g_list_free (p->controllers);
    p->controllers = nullptr;
    for (GActionGroup *g : p->action_groups) g_object_unref (g);
    p->action_groups.clear ();
    p->destroyed = true;
    if (p->bridge) p->bridge->widget = nullptr;
    if (p->obj) {
      p->obj->setProperty ("_gtkqt_widget", QVariant ());
      p->obj->removeEventFilter (p->bridge);
      gtkqt_defer_delete (p->obj.data ());
    } else if (p->bridge && !p->bridge->parent ()) {
      delete p->bridge;
    }
    p->bridge = nullptr;
    p->visible = false;
  }
  G_OBJECT_CLASS (gtk_widget_parent_class)->dispose (o);
}

static void gtk_widget_finalize (GObject *o)
{
  GtkWidget *w = GTK_WIDGET (o);
  if (w->window) { w->window->widget = nullptr; g_object_unref (w->window); }
  w->window = nullptr;
  if (w->style) g_object_unref (w->style);
  w->style = nullptr;
  delete w->priv;
  w->priv = nullptr;
  G_OBJECT_CLASS (gtk_widget_parent_class)->finalize (o);
}

#define WSET(fn, getter) [] (GObject *o, const GValue *v) { fn (GTK_WIDGET (o), getter (v)); }
#define WGET(setter, fn) [] (GObject *o, GValue *v) { setter (v, fn (GTK_WIDGET (o))); }
#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

static void set_margin_all (GtkWidget *w, gint m)
{
  gtk_widget_set_margin_start (w, m); gtk_widget_set_margin_end (w, m);
  gtk_widget_set_margin_top (w, m); gtk_widget_set_margin_bottom (w, m);
}

static void gtk_widget_class_init (GtkWidgetClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  oc->constructed = gtk_widget_constructed;
  oc->dispose = gtk_widget_dispose;
  oc->finalize = gtk_widget_finalize;
  klass->destroy = gtk_widget_real_destroy;
  klass->realize = widget_default_realize;
  klass->gtkqt_create = (gpointer) widget_default_create;

  GType t = G_TYPE_FROM_CLASS (klass);
  auto sig0 = [t] (const char *name, guint offset, GSignalFlags flags = G_SIGNAL_RUN_LAST) {
    return g_signal_new (name, t, flags, offset, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  };
  auto sigev = [t] (const char *name, guint offset, GType arg = G_TYPE_POINTER) {
    return g_signal_new (name, t, G_SIGNAL_RUN_LAST, offset, g_signal_accumulator_true_handled, nullptr, nullptr,
                         G_TYPE_BOOLEAN, 1, arg);
  };
  widget_signals[SIG_DESTROY] = sig0 ("destroy", G_STRUCT_OFFSET (GtkWidgetClass, destroy),
                                      (GSignalFlags) (G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS));
  widget_signals[SIG_SHOW] = sig0 ("show", G_STRUCT_OFFSET (GtkWidgetClass, show), G_SIGNAL_RUN_FIRST);
  widget_signals[SIG_HIDE] = sig0 ("hide", G_STRUCT_OFFSET (GtkWidgetClass, hide), G_SIGNAL_RUN_FIRST);
  widget_signals[SIG_MAP] = sig0 ("map", G_STRUCT_OFFSET (GtkWidgetClass, map), G_SIGNAL_RUN_FIRST);
  widget_signals[SIG_UNMAP] = sig0 ("unmap", G_STRUCT_OFFSET (GtkWidgetClass, unmap), G_SIGNAL_RUN_FIRST);
  widget_signals[SIG_REALIZE] = sig0 ("realize", G_STRUCT_OFFSET (GtkWidgetClass, realize), G_SIGNAL_RUN_FIRST);
  widget_signals[SIG_UNREALIZE] = sig0 ("unrealize", G_STRUCT_OFFSET (GtkWidgetClass, unrealize));
  widget_signals[SIG_SIZE_ALLOCATE] = g_signal_new ("size-allocate", t, G_SIGNAL_RUN_FIRST,
                                                    G_STRUCT_OFFSET (GtkWidgetClass, size_allocate), nullptr, nullptr,
                                                    nullptr, G_TYPE_NONE, 1, G_TYPE_POINTER);
  widget_signals[SIG_DRAW] = sigev ("draw", G_STRUCT_OFFSET (GtkWidgetClass, draw));
  widget_signals[SIG_EVENT] = sigev ("event", G_STRUCT_OFFSET (GtkWidgetClass, event));
  widget_signals[SIG_EVENT_AFTER] = g_signal_new ("event-after", t, (GSignalFlags) 0, 0, nullptr, nullptr, nullptr,
                                                  G_TYPE_NONE, 1, G_TYPE_POINTER);
  widget_signals[SIG_BUTTON_PRESS] = sigev ("button-press-event", G_STRUCT_OFFSET (GtkWidgetClass, button_press_event));
  widget_signals[SIG_BUTTON_RELEASE] = sigev ("button-release-event", G_STRUCT_OFFSET (GtkWidgetClass, button_release_event));
  widget_signals[SIG_SCROLL] = sigev ("scroll-event", G_STRUCT_OFFSET (GtkWidgetClass, scroll_event));
  widget_signals[SIG_MOTION] = sigev ("motion-notify-event", G_STRUCT_OFFSET (GtkWidgetClass, motion_notify_event));
  widget_signals[SIG_DELETE] = sigev ("delete-event", G_STRUCT_OFFSET (GtkWidgetClass, delete_event));
  widget_signals[SIG_KEY_PRESS] = sigev ("key-press-event", G_STRUCT_OFFSET (GtkWidgetClass, key_press_event));
  widget_signals[SIG_KEY_RELEASE] = sigev ("key-release-event", G_STRUCT_OFFSET (GtkWidgetClass, key_release_event));
  widget_signals[SIG_ENTER] = sigev ("enter-notify-event", G_STRUCT_OFFSET (GtkWidgetClass, enter_notify_event));
  widget_signals[SIG_LEAVE] = sigev ("leave-notify-event", G_STRUCT_OFFSET (GtkWidgetClass, leave_notify_event));
  widget_signals[SIG_CONFIGURE] = sigev ("configure-event", G_STRUCT_OFFSET (GtkWidgetClass, configure_event));
  widget_signals[SIG_FOCUS_IN] = sigev ("focus-in-event", G_STRUCT_OFFSET (GtkWidgetClass, focus_in_event));
  widget_signals[SIG_FOCUS_OUT] = sigev ("focus-out-event", G_STRUCT_OFFSET (GtkWidgetClass, focus_out_event));
  widget_signals[SIG_EXPOSE] = sigev ("expose-event", G_STRUCT_OFFSET (GtkWidgetClass, expose_event));
  sigev ("client-event", 0);   /* X client messages: never delivered without X */
  /* answer to gtk_selection_convert (GtkSelectionData *, guint time) */
  g_signal_new ("selection-received", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2,
                G_TYPE_POINTER, G_TYPE_UINT);
  /* the owner side (gtk_selection_owner_set): (GtkSelectionData *, guint info, guint time) */
  g_signal_new ("selection-get", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 3,
                G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_UINT);
  sigev ("selection-clear-event", 0);
  /* GTK 2 drag-and-drop and style signals (gtk2.cpp emits them) */
  g_signal_new ("style-set", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_OBJECT);
  g_signal_new ("state-changed", t, G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_INT);
  g_signal_new ("drag-motion", t, G_SIGNAL_RUN_LAST, 0, g_signal_accumulator_true_handled, nullptr, nullptr,
                G_TYPE_BOOLEAN, 4, G_TYPE_OBJECT, G_TYPE_INT, G_TYPE_INT, G_TYPE_UINT);
  g_signal_new ("drag-drop", t, G_SIGNAL_RUN_LAST, 0, g_signal_accumulator_true_handled, nullptr, nullptr,
                G_TYPE_BOOLEAN, 4, G_TYPE_OBJECT, G_TYPE_INT, G_TYPE_INT, G_TYPE_UINT);
  g_signal_new ("drag-leave", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_UINT);
  g_signal_new ("drag-data-received", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 6,
                G_TYPE_OBJECT, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_UINT);
  g_signal_new ("drag-begin", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_OBJECT);
  g_signal_new ("drag-end", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_OBJECT);
  g_signal_new ("drag-data-get", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 4,
                G_TYPE_OBJECT, G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_UINT);
  g_signal_new ("drag-data-delete", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_OBJECT);
  widget_signals[SIG_GRAB_FOCUS] = sig0 ("grab-focus", 0);
  widget_signals[SIG_STATE_FLAGS_CHANGED] = g_signal_new ("state-flags-changed", t, G_SIGNAL_RUN_FIRST, 0, nullptr,
                                                          nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_UINT);

  gtkqt_prop (klass, g_param_spec_boolean ("visible", nullptr, nullptr, FALSE, RW),
              WSET (gtk_widget_set_visible, g_value_get_boolean), WGET (g_value_set_boolean, gtk_widget_get_visible));
  gtkqt_prop (klass, g_param_spec_boolean ("sensitive", nullptr, nullptr, TRUE, RW),
              WSET (gtk_widget_set_sensitive, g_value_get_boolean), WGET (g_value_set_boolean, gtk_widget_get_sensitive));
  gtkqt_prop (klass, g_param_spec_string ("name", nullptr, nullptr, nullptr, RW),
              WSET (gtk_widget_set_name, g_value_get_string), WGET (g_value_set_string, gtk_widget_get_name));
  gtkqt_prop (klass, g_param_spec_string ("tooltip-text", nullptr, nullptr, nullptr, RW),
              WSET (gtk_widget_set_tooltip_text, g_value_get_string), WGET (g_value_take_string, gtk_widget_get_tooltip_text));
  gtkqt_prop (klass, g_param_spec_string ("tooltip-markup", nullptr, nullptr, nullptr, RW),
              WSET (gtk_widget_set_tooltip_markup, g_value_get_string), nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("has-tooltip", nullptr, nullptr, FALSE, RW),
              WSET (gtk_widget_set_has_tooltip, g_value_get_boolean), nullptr);
  gtkqt_prop (klass, g_param_spec_int ("width-request", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { GtkWidget *w = GTK_WIDGET (o); gtk_widget_set_size_request (w, g_value_get_int (v), w->priv->height_request); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, GTK_WIDGET (o)->priv->width_request); });
  gtkqt_prop (klass, g_param_spec_int ("height-request", nullptr, nullptr, -1, G_MAXINT, -1, RW),
              [] (GObject *o, const GValue *v) { GtkWidget *w = GTK_WIDGET (o); gtk_widget_set_size_request (w, w->priv->width_request, g_value_get_int (v)); },
              [] (GObject *o, GValue *v) { g_value_set_int (v, GTK_WIDGET (o)->priv->height_request); });
  gtkqt_prop (klass, g_param_spec_boolean ("hexpand", nullptr, nullptr, FALSE, RW),
              WSET (gtk_widget_set_hexpand, g_value_get_boolean), WGET (g_value_set_boolean, gtk_widget_get_hexpand));
  gtkqt_prop (klass, g_param_spec_boolean ("vexpand", nullptr, nullptr, FALSE, RW),
              WSET (gtk_widget_set_vexpand, g_value_get_boolean), WGET (g_value_set_boolean, gtk_widget_get_vexpand));
  gtkqt_prop (klass, g_param_spec_boolean ("hexpand-set", nullptr, nullptr, FALSE, RW),
              WSET (gtk_widget_set_hexpand_set, g_value_get_boolean), nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("vexpand-set", nullptr, nullptr, FALSE, RW),
              WSET (gtk_widget_set_vexpand_set, g_value_get_boolean), nullptr);
  gtkqt_prop (klass, g_param_spec_enum ("halign", nullptr, nullptr, gtk_align_get_type (), GTK_ALIGN_FILL, RW),
              [] (GObject *o, const GValue *v) { gtk_widget_set_halign (GTK_WIDGET (o), (GtkAlign) g_value_get_enum (v)); },
              [] (GObject *o, GValue *v) { g_value_set_enum (v, gtk_widget_get_halign (GTK_WIDGET (o))); });
  gtkqt_prop (klass, g_param_spec_enum ("valign", nullptr, nullptr, gtk_align_get_type (), GTK_ALIGN_FILL, RW),
              [] (GObject *o, const GValue *v) { gtk_widget_set_valign (GTK_WIDGET (o), (GtkAlign) g_value_get_enum (v)); },
              [] (GObject *o, GValue *v) { g_value_set_enum (v, gtk_widget_get_valign (GTK_WIDGET (o))); });
  gtkqt_prop (klass, g_param_spec_int ("margin-start", nullptr, nullptr, 0, G_MAXINT16, 0, RW),
              WSET (gtk_widget_set_margin_start, g_value_get_int), WGET (g_value_set_int, gtk_widget_get_margin_start));
  gtkqt_prop (klass, g_param_spec_int ("margin-end", nullptr, nullptr, 0, G_MAXINT16, 0, RW),
              WSET (gtk_widget_set_margin_end, g_value_get_int), WGET (g_value_set_int, gtk_widget_get_margin_end));
  gtkqt_prop (klass, g_param_spec_int ("margin-left", nullptr, nullptr, 0, G_MAXINT16, 0, RW),
              WSET (gtk_widget_set_margin_start, g_value_get_int), WGET (g_value_set_int, gtk_widget_get_margin_start));
  gtkqt_prop (klass, g_param_spec_int ("margin-right", nullptr, nullptr, 0, G_MAXINT16, 0, RW),
              WSET (gtk_widget_set_margin_end, g_value_get_int), WGET (g_value_set_int, gtk_widget_get_margin_end));
  gtkqt_prop (klass, g_param_spec_int ("margin-top", nullptr, nullptr, 0, G_MAXINT16, 0, RW),
              WSET (gtk_widget_set_margin_top, g_value_get_int), WGET (g_value_set_int, gtk_widget_get_margin_top));
  gtkqt_prop (klass, g_param_spec_int ("margin-bottom", nullptr, nullptr, 0, G_MAXINT16, 0, RW),
              WSET (gtk_widget_set_margin_bottom, g_value_get_int), WGET (g_value_set_int, gtk_widget_get_margin_bottom));
  gtkqt_prop (klass, g_param_spec_int ("margin", nullptr, nullptr, 0, G_MAXINT16, 0, RW),
              WSET (set_margin_all, g_value_get_int), WGET (g_value_set_int, gtk_widget_get_margin_top));
  gtkqt_prop (klass, g_param_spec_boolean ("can-focus", nullptr, nullptr, TRUE, RW),
              WSET (gtk_widget_set_can_focus, g_value_get_boolean), WGET (g_value_set_boolean, gtk_widget_get_can_focus));
  gtkqt_prop (klass, g_param_spec_boolean ("focusable", nullptr, nullptr, TRUE, RW),
              WSET (gtk_widget_set_can_focus, g_value_get_boolean), WGET (g_value_set_boolean, gtk_widget_get_can_focus));
  gtkqt_prop (klass, g_param_spec_boolean ("has-focus", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { if (g_value_get_boolean (v)) gtk_widget_grab_focus (GTK_WIDGET (o)); },
              WGET (g_value_set_boolean, gtk_widget_has_focus));
  gtkqt_prop (klass, g_param_spec_boolean ("is-focus", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { if (g_value_get_boolean (v)) gtk_widget_grab_focus (GTK_WIDGET (o)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("can-default", nullptr, nullptr, FALSE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("has-default", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { if (g_value_get_boolean (v)) gtk_widget_grab_default (GTK_WIDGET (o)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("receives-default", nullptr, nullptr, FALSE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("no-show-all", nullptr, nullptr, FALSE, RW),
              WSET (gtk_widget_set_no_show_all, g_value_get_boolean), nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("app-paintable", nullptr, nullptr, FALSE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_double ("opacity", nullptr, nullptr, 0, 1, 1, RW),
              WSET (gtk_widget_set_opacity, g_value_get_double), nullptr);
  gtkqt_prop (klass, g_param_spec_int ("events", nullptr, nullptr, 0, G_MAXINT, 0, RW),
              WSET (gtk_widget_set_events, g_value_get_int), WGET (g_value_set_int, gtk_widget_get_events));
  gtkqt_prop (klass, g_param_spec_boxed ("css-classes", nullptr, nullptr, G_TYPE_STRV, RW),
              [] (GObject *o, const GValue *v) { gtk_widget_set_css_classes (GTK_WIDGET (o), (const char **) g_value_get_boxed (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("css-name", nullptr, nullptr, nullptr, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("parent", nullptr, nullptr, GTK_TYPE_WIDGET, G_PARAM_READABLE),
              nullptr, [] (GObject *o, GValue *v) { g_value_set_object (v, GTK_WIDGET (o)->parent); });
}

/* ---------- bridge: Qt events -> GDK events -> GTK signals ---------- */
gboolean gtkqt_emit_event (GtkWidget *w, GdkEvent *ev, const char *signal_name)
{
  gboolean handled = FALSE;
  /* handlers may destroy the widget (as GTK, keep it alive meanwhile) */
  g_object_ref (w);
  g_signal_emit (w, widget_signals[SIG_EVENT], 0, ev, &handled);
  if (!handled && signal_name && !w->priv->destroyed) {
    guint id = g_signal_lookup (signal_name, G_OBJECT_TYPE (w));
    if (id) g_signal_emit (w, id, 0, ev, &handled);
  }
  if (!w->priv->destroyed)
    g_signal_emit (w, widget_signals[SIG_EVENT_AFTER], 0, ev);
  g_object_unref (w);
  return handled;
}

guint gtkqt_qt_mods_to_gdk (Qt::KeyboardModifiers mods, Qt::MouseButtons buttons)
{
  guint s = 0;
  if (mods & Qt::ShiftModifier) s |= GDK_SHIFT_MASK;
  if (mods & Qt::ControlModifier) s |= GDK_CONTROL_MASK;
  if (mods & Qt::AltModifier) s |= GDK_MOD1_MASK;
  if (mods & Qt::MetaModifier) s |= GDK_SUPER_MASK;
  if (buttons & Qt::LeftButton) s |= GDK_BUTTON1_MASK;
  if (buttons & Qt::MiddleButton) s |= GDK_BUTTON2_MASK;
  if (buttons & Qt::RightButton) s |= GDK_BUTTON3_MASK;
  return s;
}

guint gtkqt_qt_key_to_keyval (int key, const QString &text, Qt::KeyboardModifiers mods)
{
  static const QMap<int, guint> special = {
    { Qt::Key_Backspace, GDK_KEY_BackSpace }, { Qt::Key_Tab, GDK_KEY_Tab }, { Qt::Key_Backtab, GDK_KEY_ISO_Left_Tab },
    { Qt::Key_Return, GDK_KEY_Return }, { Qt::Key_Enter, GDK_KEY_KP_Enter }, { Qt::Key_Escape, GDK_KEY_Escape },
    { Qt::Key_Delete, GDK_KEY_Delete }, { Qt::Key_Insert, GDK_KEY_Insert }, { Qt::Key_Home, GDK_KEY_Home },
    { Qt::Key_End, GDK_KEY_End }, { Qt::Key_Left, GDK_KEY_Left }, { Qt::Key_Right, GDK_KEY_Right },
    { Qt::Key_Up, GDK_KEY_Up }, { Qt::Key_Down, GDK_KEY_Down }, { Qt::Key_PageUp, GDK_KEY_Page_Up },
    { Qt::Key_PageDown, GDK_KEY_Page_Down }, { Qt::Key_Shift, GDK_KEY_Shift_L }, { Qt::Key_Control, GDK_KEY_Control_L },
    { Qt::Key_Alt, GDK_KEY_Alt_L }, { Qt::Key_Meta, GDK_KEY_Super_L }, { Qt::Key_CapsLock, GDK_KEY_Caps_Lock },
    { Qt::Key_NumLock, GDK_KEY_Num_Lock }, { Qt::Key_ScrollLock, GDK_KEY_Scroll_Lock }, { Qt::Key_Pause, GDK_KEY_Pause },
    { Qt::Key_Print, GDK_KEY_Print }, { Qt::Key_Menu, GDK_KEY_Menu }, { Qt::Key_Help, GDK_KEY_Help },
    { Qt::Key_Space, GDK_KEY_space },
  };
  auto it = special.find (key);
  if (it != special.end ()) return *it;
  if (key >= Qt::Key_F1 && key <= Qt::Key_F24) return GDK_KEY_F1 + (key - Qt::Key_F1);
  if (!text.isEmpty () && text[0].unicode () >= 0x20 && !(mods & Qt::ControlModifier))
    return gdk_unicode_to_keyval (text.toUcs4 ().value (0));
  if (key >= Qt::Key_A && key <= Qt::Key_Z)
    return (mods & Qt::ShiftModifier) ? key : key + 0x20;
  if (key >= 0x20 && key < 0x7f) return key;
  return gdk_unicode_to_keyval (key);
}

static void update_allocation (GtkWidget *w, QWidget *qw)
{
  QPoint pos = qw->isWindow () ? QPoint (0, 0) : qw->mapTo (qw->window (), QPoint (0, 0));
  w->allocation.x = pos.x ();
  w->allocation.y = pos.y ();
  w->allocation.width = qw->width ();
  w->allocation.height = qw->height ();
}

static bool handle_window_close (GtkWidget *w, QCloseEvent *ce)
{
  ce->ignore ();
  g_object_ref (w);
  GdkEvent ev = {};
  ev.any.type = GDK_DELETE;
  ev.any.window = w->window;
  gboolean handled = gtkqt_emit_event (w, &ev, "delete-event");
  if (!handled && !w->priv->destroyed) {
    guint id = g_signal_lookup ("close-request", G_OBJECT_TYPE (w));
    if (id) g_signal_emit (w, id, 0, &handled);
  }
  if (!handled && !w->priv->destroyed) {
    if (g_object_get_data (G_OBJECT (w), "gtkqt-hide-on-close"))
      gtk_widget_hide (w);
    else
      gtk_widget_destroy (w);
  }
  g_object_unref (w);
  return true;
}

GtkQtBridge::~GtkQtBridge ()
{
  if (widget && widget->priv && widget->priv->bridge == this)
    widget->priv->bridge = nullptr;
}

QWidget *gtkqt_content (GtkWidget *w)
{
  QWidget *qw = gtkqt_qw (w);
  return w->priv->content ? w->priv->content.data () : qw;
}

bool GtkQtBridge::eventFilter (QObject *watched, QEvent *e)
{
  GtkWidget *w = widget;
  if (!w || watched != w->priv->obj.data ()) return false;
  QWidget *qw = qobject_cast<QWidget *> (watched);
  if (!qw) return false;
  GtkQtEmitGuard guard;
  struct Keep { GtkWidget *w; ~Keep () { g_object_unref (w); } } keep { (GtkWidget *) g_object_ref (w) };
  bool consumed = false;
  if (w->priv->controllers)
    gtkqt_controllers_dispatch (w, e, &consumed);
  if (consumed) return true;

  GdkEvent ev = {};
  switch (e->type ()) {
  case QEvent::Close:
    if (GTK_IS_WINDOW (w)) return handle_window_close (w, static_cast<QCloseEvent *> (e));
    return false;
  case QEvent::Resize:
  case QEvent::Move: {
    update_allocation (w, qw);
    GtkAllocation alloc = w->allocation;
    g_signal_emit (w, widget_signals[SIG_SIZE_ALLOCATE], 0, &alloc);
    ev.configure.type = GDK_CONFIGURE;
    ev.configure.window = w->window;
    ev.configure.x = qw->x ();
    ev.configure.y = qw->y ();
    ev.configure.width = qw->width ();
    ev.configure.height = qw->height ();
    gtkqt_emit_event (w, &ev, "configure-event");
    return false;
  }
  case QEvent::Show:
    ensure_realized (w);
    w->flags |= GTK_MAPPED | GTK_REALIZED;
    g_signal_emit (w, widget_signals[SIG_MAP], 0);
    return false;
  case QEvent::Hide:
    w->flags &= ~GTK_MAPPED;
    g_signal_emit (w, widget_signals[SIG_UNMAP], 0);
    return false;
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseButtonDblClick: {
    auto *me = static_cast<QMouseEvent *> (e);
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
    QPointF lp = me->position (), gp = me->globalPosition ();
#else
    QPointF lp = me->localPos (), gp = me->screenPos ();
#endif
    ev.button.type = e->type () == QEvent::MouseButtonRelease ? GDK_BUTTON_RELEASE : GDK_BUTTON_PRESS;
    ev.button.window = w->window;
    ev.button.time = me->timestamp ();
    ev.button.x = lp.x ();
    ev.button.y = lp.y ();
    ev.button.x_root = gp.x ();
    ev.button.y_root = gp.y ();
    ev.button.button = me->button () == Qt::RightButton ? 3 : me->button () == Qt::MiddleButton ? 2 : 1;
    ev.button.state = gtkqt_qt_mods_to_gdk (me->modifiers (), me->buttons ());
    const char *sig = ev.button.type == GDK_BUTTON_RELEASE ? "button-release-event" : "button-press-event";
    consumed = gtkqt_emit_event (w, &ev, sig);
    if (e->type () == QEvent::MouseButtonDblClick && !w->priv->destroyed) {
      ev.button.type = GDK_2BUTTON_PRESS;
      consumed = gtkqt_emit_event (w, &ev, sig) || consumed;
    }
    return consumed;
  }
  case QEvent::MouseMove: {
    auto *me = static_cast<QMouseEvent *> (e);
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
    QPointF lp = me->position (), gp = me->globalPosition ();
#else
    QPointF lp = me->localPos (), gp = me->screenPos ();
#endif
    ev.motion.type = GDK_MOTION_NOTIFY;
    ev.motion.window = w->window;
    ev.motion.time = me->timestamp ();
    ev.motion.x = lp.x ();
    ev.motion.y = lp.y ();
    ev.motion.x_root = gp.x ();
    ev.motion.y_root = gp.y ();
    ev.motion.state = gtkqt_qt_mods_to_gdk (me->modifiers (), me->buttons ());
    return gtkqt_emit_event (w, &ev, "motion-notify-event");
  }
  case QEvent::Wheel: {
    auto *we = static_cast<QWheelEvent *> (e);
    QPoint d = we->angleDelta ();
    ev.scroll.type = GDK_SCROLL;
    ev.scroll.window = w->window;
    ev.scroll.time = we->timestamp ();
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
    ev.scroll.x = we->position ().x ();
    ev.scroll.y = we->position ().y ();
#else
    ev.scroll.x = we->posF ().x ();
    ev.scroll.y = we->posF ().y ();
#endif
    ev.scroll.state = gtkqt_qt_mods_to_gdk (we->modifiers (), we->buttons ());
    if (qAbs (d.y ()) >= qAbs (d.x ()))
      ev.scroll.direction = d.y () > 0 ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
    else
      ev.scroll.direction = d.x () > 0 ? GDK_SCROLL_LEFT : GDK_SCROLL_RIGHT;
    ev.scroll.delta_x = -d.x () / 120.0;
    ev.scroll.delta_y = -d.y () / 120.0;
    return gtkqt_emit_event (w, &ev, "scroll-event");
  }
  case QEvent::KeyPress:
  case QEvent::KeyRelease: {
    auto *ke = static_cast<QKeyEvent *> (e);
    QByteArray txt = ke->text ().toUtf8 ();
    ev.key.type = e->type () == QEvent::KeyPress ? GDK_KEY_PRESS : GDK_KEY_RELEASE;
    ev.key.window = w->window;
    ev.key.time = ke->timestamp ();
    ev.key.state = gtkqt_qt_mods_to_gdk (ke->modifiers (), Qt::NoButton);
    ev.key.keyval = gtkqt_qt_key_to_keyval (ke->key (), ke->text (), ke->modifiers ());
    ev.key.hardware_keycode = (guint16) ke->nativeScanCode ();
    ev.key.string = txt.data ();
    ev.key.length = txt.size ();
    ev.key.is_modifier = ke->key () == Qt::Key_Shift || ke->key () == Qt::Key_Control ||
                         ke->key () == Qt::Key_Alt || ke->key () == Qt::Key_Meta;
    return gtkqt_emit_event (w, &ev, ev.key.type == GDK_KEY_PRESS ? "key-press-event" : "key-release-event");
  }
  case QEvent::Enter:
  case QEvent::Leave: {
    QPoint p = qw->mapFromGlobal (QCursor::pos ());
    ev.crossing.type = e->type () == QEvent::Enter ? GDK_ENTER_NOTIFY : GDK_LEAVE_NOTIFY;
    ev.crossing.window = w->window;
    ev.crossing.x = p.x ();
    ev.crossing.y = p.y ();
    return gtkqt_emit_event (w, &ev, e->type () == QEvent::Enter ? "enter-notify-event" : "leave-notify-event");
  }
  case QEvent::FocusIn:
  case QEvent::FocusOut:
    ev.focus_change.type = GDK_FOCUS_CHANGE;
    ev.focus_change.window = w->window;
    ev.focus_change.in = e->type () == QEvent::FocusIn;
    gtkqt_emit_event (w, &ev, ev.focus_change.in ? "focus-in-event" : "focus-out-event");
    return false;
  default:
    return false;
  }
}

/* ---------- GtkWidget API ---------- */
extern "C" {

void gtk_widget_destroy (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (widget->priv->in_destruction) return;
  g_object_run_dispose (G_OBJECT (widget));
}

void gtk_widget_destroyed (GtkWidget *, GtkWidget **widget_pointer)
{
  if (widget_pointer) *widget_pointer = nullptr;
}

void gtk_widget_unparent (GtkWidget *widget)
{
  if (widget->parent && GTK_IS_CONTAINER (widget->parent))
    gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
}

void gtk_widget_show (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (widget->priv->destroyed) return;
  g_object_ref (widget);   /* event handlers run during the show may destroy it */
  if (!widget->priv->visible) {
    widget->priv->visible = true;
    widget->flags |= GTK_VISIBLE;
    gtkqt_sync_visible (widget);
    if (!widget->priv->destroyed) {
      g_signal_emit (widget, widget_signals[SIG_SHOW], 0);
      g_object_notify (G_OBJECT (widget), "visible");
    }
  } else if (GTK_IS_WINDOW (widget)) {
    gtkqt_sync_visible (widget);
  }
  g_object_unref (widget);
}

void gtk_widget_show_now (GtkWidget *widget) { gtk_widget_show (widget); }

void gtk_widget_hide (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (widget->priv->destroyed || !widget->priv->visible) return;
  g_object_ref (widget);
  widget->priv->visible = false;
  widget->flags &= ~GTK_VISIBLE;
  gtkqt_sync_visible (widget);
  if (!widget->priv->destroyed) {
    g_signal_emit (widget, widget_signals[SIG_HIDE], 0);
    g_object_notify (G_OBJECT (widget), "visible");
  }
  g_object_unref (widget);
}

static void show_all_cb (GtkWidget *w, gpointer) { gtk_widget_show_all (w); }
static void hide_all_cb (GtkWidget *w, gpointer) { gtk_widget_hide_all (w); }

void gtk_widget_show_all (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (widget->priv->no_show_all) return;
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), show_all_cb, nullptr);
  if (GtkWidget *sub = (GtkWidget *) g_object_get_data (G_OBJECT (widget), "gtkqt-submenu"))
    gtk_widget_show_all (sub);
  gtk_widget_show (widget);
}

void gtk_widget_hide_all (GtkWidget *widget)
{
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), hide_all_cb, nullptr);
  gtk_widget_hide (widget);
}

gboolean gtk_widget_hide_on_delete (GtkWidget *widget)
{
  gtk_widget_hide (widget);
  return TRUE;
}

void gtk_widget_set_visible (GtkWidget *widget, gboolean visible)
{
  if (visible) gtk_widget_show (widget); else gtk_widget_hide (widget);
}

gboolean gtk_widget_get_visible (GtkWidget *widget) { return widget->priv->visible; }

gboolean gtk_widget_is_visible (GtkWidget *widget)
{
  QWidget *qw = gtkqt_qw (widget);
  return qw ? qw->isVisible () : widget->priv->visible;
}

void gtk_widget_set_no_show_all (GtkWidget *widget, gboolean no_show_all) { widget->priv->no_show_all = no_show_all; }

gboolean gtk_widget_get_mapped (GtkWidget *widget)
{
  QWidget *qw = gtkqt_qw (widget);
  return qw && qw->isVisible ();
}

gboolean gtk_widget_get_realized (GtkWidget *widget) { return gtkqt_obj (widget) != nullptr; }
void gtk_widget_realize (GtkWidget *widget) { gtkqt_obj (widget); ensure_realized (widget); }
void gtk_widget_map (GtkWidget *widget) { gtk_widget_show (widget); }

void gtk_widget_set_sensitive (GtkWidget *widget, gboolean sensitive)
{
  widget->priv->sensitive = sensitive;
  if (sensitive) widget->flags |= GTK_SENSITIVE; else widget->flags &= ~GTK_SENSITIVE;
  QObject *o = gtkqt_obj (widget);
  if (QWidget *qw = qobject_cast<QWidget *> (o)) qw->setEnabled (sensitive);
  else if (QAction *a = qobject_cast<QAction *> (o)) a->setEnabled (sensitive);
  g_object_notify (G_OBJECT (widget), "sensitive");
}

gboolean gtk_widget_get_sensitive (GtkWidget *widget) { return widget->priv->sensitive; }

gboolean gtk_widget_is_sensitive (GtkWidget *widget)
{
  for (GtkWidget *w = widget; w; w = w->parent)
    if (!w->priv->sensitive) return FALSE;
  return TRUE;
}

void gtk_widget_set_size_request (GtkWidget *widget, gint width, gint height)
{
  widget->priv->width_request = width;
  widget->priv->height_request = height;
  gtkqt_apply_size_request (widget);
}

void gtk_widget_get_size_request (GtkWidget *widget, gint *width, gint *height)
{
  if (width) *width = widget->priv->width_request;
  if (height) *height = widget->priv->height_request;
}

void gtk_widget_set_usize (GtkWidget *widget, gint width, gint height) { gtk_widget_set_size_request (widget, width, height); }

void gtk_widget_size_request (GtkWidget *widget, GtkRequisition *req)
{
  QWidget *qw = gtkqt_qw (widget);
  QSize s = qw ? qw->sizeHint ().expandedTo (qw->minimumSize ()) : QSize ();
  if (req) { req->width = s.width (); req->height = s.height (); }
  widget->requisition.width = s.width ();
  widget->requisition.height = s.height ();
}

void gtk_widget_get_preferred_size (GtkWidget *widget, GtkRequisition *minimum, GtkRequisition *natural)
{
  QWidget *qw = gtkqt_qw (widget);
  QSize mn = qw ? qw->minimumSizeHint () : QSize (), nat = qw ? qw->sizeHint () : QSize ();
  if (minimum) { minimum->width = mn.width (); minimum->height = mn.height (); }
  if (natural) { natural->width = nat.width (); natural->height = nat.height (); }
}

void gtk_widget_get_allocation (GtkWidget *widget, GtkAllocation *allocation)
{
  if (QWidget *qw = gtkqt_qw (widget)) update_allocation (widget, qw);
  if (allocation) *allocation = widget->allocation;
}

int gtk_widget_get_allocated_width (GtkWidget *widget) { QWidget *qw = gtkqt_qw (widget); return qw ? qw->width () : 0; }
int gtk_widget_get_allocated_height (GtkWidget *widget) { QWidget *qw = gtkqt_qw (widget); return qw ? qw->height () : 0; }
int gtk_widget_get_width (GtkWidget *widget) { return gtk_widget_get_allocated_width (widget); }
int gtk_widget_get_height (GtkWidget *widget) { return gtk_widget_get_allocated_height (widget); }

int gtk_widget_get_scale_factor (GtkWidget *widget)
{
  QWidget *qw = gtkqt_qw (widget);
  return qw ? qMax (1, (int) qw->devicePixelRatioF ()) : 1;
}

void gtk_widget_set_tooltip_text (GtkWidget *widget, const gchar *text)
{
  QObject *o = gtkqt_obj (widget);
  if (QWidget *qw = qobject_cast<QWidget *> (o)) qw->setToolTip (gtkqt_str (text).toHtmlEscaped ());
  else if (QAction *a = qobject_cast<QAction *> (o)) a->setToolTip (gtkqt_str (text));
}

gchar *gtk_widget_get_tooltip_text (GtkWidget *widget)
{
  QWidget *qw = gtkqt_qw (widget);
  return qw && !qw->toolTip ().isEmpty () ? g_strdup (qw->toolTip ().toUtf8 ().constData ()) : nullptr;
}

void gtk_widget_set_tooltip_markup (GtkWidget *widget, const gchar *markup)
{
  if (QWidget *qw = gtkqt_qw (widget)) qw->setToolTip (markup ? gtkqt_markup_to_html (markup) : QString ());
}

void gtk_widget_set_has_tooltip (GtkWidget *, gboolean) {}

void gtk_widget_set_name (GtkWidget *widget, const gchar *name)
{
  if (QObject *o = gtkqt_obj (widget)) o->setObjectName (gtkqt_str (name));
}

const gchar *gtk_widget_get_name (GtkWidget *widget)
{
  QObject *o = gtkqt_obj (widget);
  if (o && !o->objectName ().isEmpty ()) return gtkqt_ret (widget, o->objectName ());
  return G_OBJECT_TYPE_NAME (widget);
}

GtkWidget *gtk_widget_get_parent (GtkWidget *widget) { return widget->parent; }

void gtk_widget_set_parent (GtkWidget *widget, GtkWidget *parent)
{
  /* Used by custom GTK 4 widgets: treat like adding to a container if possible. */
  if (GTK_IS_CONTAINER (parent)) gtk_container_add (GTK_CONTAINER (parent), widget);
  else {
    widget->parent = parent;
    g_object_ref_sink (widget);
    if (QWidget *qw = gtkqt_qw (widget)) {
      if (QWidget *pw = gtkqt_qw (parent)) {
        if (!pw->layout ()) new QVBoxLayout (pw);
        pw->layout ()->setContentsMargins (0, 0, 0, 0);
        pw->layout ()->addWidget (qw);
      }
    }
    gtkqt_sync_visible (widget);
  }
}

GtkWidget *gtk_widget_get_toplevel (GtkWidget *widget)
{
  GtkWidget *w = widget;
  while (w->parent) w = w->parent;
  return w;
}

GtkWidget *gtk_widget_get_root (GtkWidget *widget)
{
  GtkWidget *t = gtk_widget_get_toplevel (widget);
  return GTK_IS_WINDOW (t) ? t : nullptr;
}

GtkWidget *gtk_widget_get_ancestor (GtkWidget *widget, GType widget_type)
{
  for (GtkWidget *w = widget; w; w = w->parent)
    if (g_type_is_a (G_OBJECT_TYPE (w), widget_type)) return w;
  return nullptr;
}

gboolean gtk_widget_is_toplevel (GtkWidget *widget) { return GTK_IS_WINDOW (widget); }

gboolean gtk_widget_is_ancestor (GtkWidget *widget, GtkWidget *ancestor)
{
  for (GtkWidget *w = widget->parent; w; w = w->parent)
    if (w == ancestor) return TRUE;
  return FALSE;
}

GtkWidget *gtk_widget_get_first_child (GtkWidget *widget)
{
  return widget->priv->children ? (GtkWidget *) widget->priv->children->data : nullptr;
}

GtkWidget *gtk_widget_get_last_child (GtkWidget *widget)
{
  GList *l = g_list_last (widget->priv->children);
  return l ? (GtkWidget *) l->data : nullptr;
}

GtkWidget *gtk_widget_get_next_sibling (GtkWidget *widget)
{
  if (!widget->parent) return nullptr;
  GList *l = g_list_find (widget->parent->priv->children, widget);
  return l && l->next ? (GtkWidget *) l->next->data : nullptr;
}

GtkWidget *gtk_widget_get_prev_sibling (GtkWidget *widget)
{
  if (!widget->parent) return nullptr;
  GList *l = g_list_find (widget->parent->priv->children, widget);
  return l && l->prev ? (GtkWidget *) l->prev->data : nullptr;
}

GdkWindow *gtk_widget_get_window (GtkWidget *widget) { gtkqt_obj (widget); return gtkqt_gdk_window (widget); }
GdkWindow *gtk_widget_get_parent_window (GtkWidget *widget) { return widget->parent ? gtk_widget_get_window (widget->parent) : nullptr; }
GdkDisplay *gtk_widget_get_display (GtkWidget *) { return gdk_display_get_default (); }
GdkScreen *gtk_widget_get_screen (GtkWidget *) { return gdk_screen_get_default (); }

void gtk_widget_queue_draw (GtkWidget *widget)
{
  if (QWidget *qw = gtkqt_qw (widget)) qw->update ();
}

void gtk_widget_queue_draw_area (GtkWidget *widget, gint x, gint y, gint width, gint height)
{
  if (QWidget *qw = gtkqt_qw (widget)) qw->update (x, y, width, height);
}

void gtk_widget_queue_resize (GtkWidget *widget)
{
  if (QWidget *qw = gtkqt_qw (widget)) qw->updateGeometry ();
}

void gtk_widget_queue_allocate (GtkWidget *widget) { gtk_widget_queue_resize (widget); }

void gtk_widget_set_can_focus (GtkWidget *widget, gboolean can_focus)
{
  if (can_focus) widget->flags |= GTK_CAN_FOCUS; else widget->flags &= ~GTK_CAN_FOCUS;
  if (QWidget *qw = gtkqt_qw (widget)) {
    if (can_focus && qw->focusPolicy () == Qt::NoFocus) qw->setFocusPolicy (Qt::StrongFocus);
    else if (!can_focus) qw->setFocusPolicy (Qt::NoFocus);
  }
}

gboolean gtk_widget_get_can_focus (GtkWidget *widget)
{
  QWidget *qw = gtkqt_qw (widget);
  return qw && qw->focusPolicy () != Qt::NoFocus;
}

void gtk_widget_set_focusable (GtkWidget *widget, gboolean focusable) { gtk_widget_set_can_focus (widget, focusable); }
gboolean gtk_widget_get_focusable (GtkWidget *widget) { return gtk_widget_get_can_focus (widget); }

void gtk_widget_set_focus_on_click (GtkWidget *widget, gboolean focus_on_click)
{
  if (QWidget *qw = gtkqt_qw (widget))
    if (!focus_on_click) qw->setFocusPolicy (Qt::TabFocus);
}

gboolean gtk_widget_has_focus (GtkWidget *widget) { QWidget *qw = gtkqt_qw (widget); return qw && qw->hasFocus (); }
gboolean gtk_widget_is_focus (GtkWidget *widget) { return gtk_widget_has_focus (widget); }

void gtk_widget_grab_focus (GtkWidget *widget)
{
  QWidget *qw = gtkqt_qw (widget);
  if (QWidget *inner = gtkqt_inner<QWidget> (widget)) qw = inner;
  if (qw) {
    qw->setFocus (Qt::OtherFocusReason);
    qw->window ()->setProperty ("gtkqt-focus-chosen", true);
  }
  g_signal_emit (widget, widget_signals[SIG_GRAB_FOCUS], 0);
}

void gtk_widget_set_can_default (GtkWidget *widget, gboolean can_default)
{
  if (can_default) widget->flags |= GTK_CAN_DEFAULT; else widget->flags &= ~GTK_CAN_DEFAULT;
}

void gtk_widget_grab_default (GtkWidget *widget)
{
  if (auto *b = qobject_cast<QPushButton *> (gtkqt_obj (widget))) b->setDefault (true);
}

void gtk_widget_set_receives_default (GtkWidget *, gboolean) {}
void gtk_widget_set_events (GtkWidget *widget, gint events) { widget->priv->events = events; gtk_widget_add_events (widget, 0); }

void gtk_widget_add_events (GtkWidget *widget, gint events)
{
  widget->priv->events |= events;
  if (QWidget *qw = gtkqt_qw (widget)) {
    if (widget->priv->events & (GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK))
      qw->setMouseTracking (true);
    if (widget->priv->events & (GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK) && qw->focusPolicy () == Qt::NoFocus)
      qw->setFocusPolicy (Qt::StrongFocus);
  }
}

gint gtk_widget_get_events (GtkWidget *widget) { return widget->priv->events; }
void gtk_widget_set_app_paintable (GtkWidget *widget, gboolean app_paintable)
{
  g_object_set_data (G_OBJECT (widget), "gtkqt-app-paintable", GINT_TO_POINTER (app_paintable));
  if (app_paintable) widget->flags |= GTK_APP_PAINTABLE; else widget->flags &= ~GTK_APP_PAINTABLE;
}
void gtk_widget_set_double_buffered (GtkWidget *, gboolean) {}
void gtk_widget_set_has_window (GtkWidget *widget, gboolean has_window)
{
  if (has_window) widget->flags &= ~GTK_NO_WINDOW; else widget->flags |= GTK_NO_WINDOW;
}
gboolean gtk_widget_get_has_window (GtkWidget *widget) { return !(widget->flags & GTK_NO_WINDOW); }

static void expand_changed (GtkWidget *widget)
{
  if (widget->parent) gtkqt_box_relayout (widget->parent);
  if (QWidget *qw = gtkqt_qw (widget)) {
    QSizePolicy sp = qw->sizePolicy ();
    if (widget->priv->hexpand) sp.setHorizontalPolicy (QSizePolicy::Expanding);
    if (widget->priv->vexpand) sp.setVerticalPolicy (QSizePolicy::Expanding);
    qw->setSizePolicy (sp);
  }
}

void gtk_widget_set_hexpand (GtkWidget *widget, gboolean expand)
{
  widget->priv->hexpand = expand; widget->priv->hexpand_set = true; expand_changed (widget);
}
gboolean gtk_widget_get_hexpand (GtkWidget *widget) { return widget->priv->hexpand; }
void gtk_widget_set_vexpand (GtkWidget *widget, gboolean expand)
{
  widget->priv->vexpand = expand; widget->priv->vexpand_set = true; expand_changed (widget);
}
gboolean gtk_widget_get_vexpand (GtkWidget *widget) { return widget->priv->vexpand; }
void gtk_widget_set_hexpand_set (GtkWidget *widget, gboolean set) { widget->priv->hexpand_set = set; }
void gtk_widget_set_vexpand_set (GtkWidget *widget, gboolean set) { widget->priv->vexpand_set = set; }

void gtk_widget_set_halign (GtkWidget *widget, GtkAlign align)
{
  widget->priv->halign = align;
  if (widget->parent) gtkqt_box_relayout (widget->parent);
}
GtkAlign gtk_widget_get_halign (GtkWidget *widget) { return widget->priv->halign; }
void gtk_widget_set_valign (GtkWidget *widget, GtkAlign align)
{
  widget->priv->valign = align;
  if (widget->parent) gtkqt_box_relayout (widget->parent);
}
GtkAlign gtk_widget_get_valign (GtkWidget *widget) { return widget->priv->valign; }

/* Margins: applied as layout margins for containers, and as extra space
 * around the widget for everything else (via a Qt style sheet margin). */
static void apply_margins (GtkWidget *widget)
{
  QWidget *qw = gtkqt_qw (widget);
  if (!qw) return;
  GtkWidgetPrivate *p = widget->priv;
  guint border = GTK_IS_CONTAINER (widget) ? GTK_CONTAINER (widget)->border_width : 0;
  if (GTK_IS_CONTAINER (widget) && gtkqt_content (widget) && gtkqt_content (widget)->layout ())
    qw = gtkqt_content (widget);
  if (qw->layout ()) {
    qw->layout ()->setContentsMargins (p->margin_start + border, p->margin_top + border,
                                       p->margin_end + border, p->margin_bottom + border);
  } else {
    qw->setContentsMargins (p->margin_start, p->margin_top, p->margin_end, p->margin_bottom);
  }
}

void gtk_widget_set_margin_start (GtkWidget *widget, gint margin) { widget->priv->margin_start = margin; apply_margins (widget); }
void gtk_widget_set_margin_end (GtkWidget *widget, gint margin) { widget->priv->margin_end = margin; apply_margins (widget); }
void gtk_widget_set_margin_top (GtkWidget *widget, gint margin) { widget->priv->margin_top = margin; apply_margins (widget); }
void gtk_widget_set_margin_bottom (GtkWidget *widget, gint margin) { widget->priv->margin_bottom = margin; apply_margins (widget); }
void gtk_widget_set_margin_left (GtkWidget *widget, gint margin) { gtk_widget_set_margin_start (widget, margin); }
void gtk_widget_set_margin_right (GtkWidget *widget, gint margin) { gtk_widget_set_margin_end (widget, margin); }
gint gtk_widget_get_margin_start (GtkWidget *widget) { return widget->priv->margin_start; }
gint gtk_widget_get_margin_end (GtkWidget *widget) { return widget->priv->margin_end; }
gint gtk_widget_get_margin_top (GtkWidget *widget) { return widget->priv->margin_top; }
gint gtk_widget_get_margin_bottom (GtkWidget *widget) { return widget->priv->margin_bottom; }

void gtk_widget_set_opacity (GtkWidget *widget, double opacity)
{
  QWidget *qw = gtkqt_qw (widget);
  if (!qw) return;
  if (qw->isWindow ()) qw->setWindowOpacity (opacity);
  else {
    auto *eff = new QGraphicsOpacityEffect (qw);
    eff->setOpacity (opacity);
    qw->setGraphicsEffect (eff);
  }
}

void gtk_widget_set_cursor_from_name (GtkWidget *widget, const char *name)
{
  QWidget *qw = gtkqt_qw (widget);
  if (!qw) return;
  QString n = gtkqt_str (name);
  if (n.isEmpty () || n == "default") qw->unsetCursor ();
  else if (n == "pointer") qw->setCursor (Qt::PointingHandCursor);
  else if (n == "text") qw->setCursor (Qt::IBeamCursor);
  else if (n == "crosshair") qw->setCursor (Qt::CrossCursor);
  else if (n == "wait" || n == "progress") qw->setCursor (Qt::WaitCursor);
  else if (n == "move" || n == "grab" || n == "grabbing") qw->setCursor (Qt::SizeAllCursor);
  else if (n == "not-allowed") qw->setCursor (Qt::ForbiddenCursor);
}

gboolean gtk_widget_activate (GtkWidget *widget)
{
  QObject *o = gtkqt_obj (widget);
  if (auto *b = qobject_cast<QAbstractButton *> (o)) { b->click (); return TRUE; }
  if (auto *a = qobject_cast<QAction *> (o)) { a->trigger (); return TRUE; }
  guint id = g_signal_lookup ("activate", G_OBJECT_TYPE (widget));
  if (id) { g_signal_emit (widget, id, 0); return TRUE; }
  return FALSE;
}

gboolean gtk_widget_mnemonic_activate (GtkWidget *widget, gboolean) { return gtk_widget_activate (widget); }

void gtk_widget_add_accelerator (GtkWidget *widget, const gchar *accel_signal, GtkAccelGroup *accel_group,
                                 guint accel_key, GdkModifierType accel_mods, guint)
{
  gchar *name = gtk_accelerator_name (accel_key, accel_mods);
  QKeySequence seq = gtkqt_accel_to_keyseq (name);
  g_free (name);
  QObject *o = gtkqt_obj (widget);
  if (auto *a = qobject_cast<QAction *> (o)) {
    QList<QKeySequence> all = a->shortcuts ();   /* GTK shows the first accelerator */
    if (!all.contains (seq)) all << seq;
    a->setShortcuts (all);
    if (accel_group) gtkqt_accel_group_add_action (accel_group, a);
    return;
  }
  /* For widgets: an action carrying the shortcut that emits the signal. */
  auto *act = new QAction (o);
  act->setShortcut (seq);
  QByteArray sig = accel_signal;
  gtkqt_connect (widget, act, &QAction::triggered, [sig] (GtkWidget *w, bool) { g_signal_emit_by_name (w, sig.constData ()); });
  if (accel_group) gtkqt_accel_group_add_action (accel_group, act);
  else if (QWidget *qw = qobject_cast<QWidget *> (o)) qw->addAction (act);
}

gboolean gtk_widget_translate_coordinates (GtkWidget *src, GtkWidget *dest, gint src_x, gint src_y, gint *dest_x, gint *dest_y)
{
  QWidget *a = gtkqt_qw (src), *b = gtkqt_qw (dest);
  if (!a || !b) return FALSE;
  QPoint p = b->mapFromGlobal (a->mapToGlobal (QPoint (src_x, src_y)));
  if (dest_x) *dest_x = p.x ();
  if (dest_y) *dest_y = p.y ();
  return TRUE;
}

void gtk_widget_insert_action_group (GtkWidget *widget, const gchar *name, GActionGroup *group)
{
  QByteArray key = name;
  if (GActionGroup *old = widget->priv->action_groups.value (key)) g_object_unref (old);
  if (group) widget->priv->action_groups.insert (key, G_ACTION_GROUP (g_object_ref (group)));
  else widget->priv->action_groups.remove (key);
}

gboolean gtk_widget_activate_action_variant (GtkWidget *widget, const char *name, GVariant *args)
{
  return gtkqt_activate_action (widget, name, args);
}

gboolean gtk_widget_activate_action (GtkWidget *widget, const char *name, const char *format_string, ...)
{
  GVariant *args = nullptr;
  if (format_string) {
    va_list ap;
    va_start (ap, format_string);
    args = g_variant_new_va (format_string, nullptr, &ap);
    va_end (ap);
    g_variant_ref_sink (args);
  }
  gboolean r = gtkqt_activate_action (widget, name, args);
  if (args) g_variant_unref (args);
  return r;
}

void gtkqt_widget_set_flags (GtkWidget *widget, guint32 flags, gboolean set)
{
  if (flags & GTK_CAN_FOCUS) gtk_widget_set_can_focus (widget, set);
  if (flags & GTK_CAN_DEFAULT) gtk_widget_set_can_default (widget, set);
  if (flags & GTK_SENSITIVE) gtk_widget_set_sensitive (widget, set);
  if (set) widget->flags |= flags; else widget->flags &= ~flags;
}

/* ---------- styling ---------- */
static void apply_css_class (QWidget *qw, const QString &cls, bool on)
{
  QFont f = qw->font ();
  if (cls == "title" || cls == "title-1" || cls == "title-2" || cls == "heading" || cls == "title-3") {
    f.setBold (on);
    if (cls.startsWith ("title")) f.setPointSizeF (on ? f.pointSizeF () * 1.4 : QApplication::font ().pointSizeF ());
    qw->setFont (f);
  } else if (cls == "monospace") {
    qw->setFont (on ? QFontDatabase::systemFont (QFontDatabase::FixedFont) : QApplication::font ());
  } else if (cls == "dim-label" || cls == "dim") {
    QPalette pal = qw->palette ();
    pal.setColor (QPalette::WindowText, on ? QApplication::palette ().color (QPalette::Disabled, QPalette::WindowText)
                                           : QApplication::palette ().color (QPalette::WindowText));
    qw->setPalette (pal);
  } else if (cls == "suggested-action" || cls == "destructive-action") {
    QPalette pal = qw->palette ();
    QColor c = cls == "suggested-action" ? QColor (0x35, 0x84, 0xe4) : QColor (0xe0, 0x1b, 0x24);
    if (on) { pal.setColor (QPalette::Button, c); pal.setColor (QPalette::ButtonText, Qt::white); }
    else pal = QApplication::palette ();
    qw->setPalette (pal);
    qw->setAutoFillBackground (on);
  } else if (cls == "error") {
    QPalette pal = qw->palette ();
    pal.setColor (QPalette::WindowText, on ? QColor (0xc0, 0x1c, 0x28) : QApplication::palette ().color (QPalette::WindowText));
    qw->setPalette (pal);
  }
}

void gtk_widget_add_css_class (GtkWidget *widget, const char *css_class)
{
  QString cls = gtkqt_str (css_class);
  if (widget->priv->css_classes.contains (cls)) return;
  widget->priv->css_classes.append (cls);
  if (QWidget *qw = qobject_cast<QWidget *> (widget->priv->obj.data ())) apply_css_class (qw, cls, true);
}

void gtk_widget_remove_css_class (GtkWidget *widget, const char *css_class)
{
  QString cls = gtkqt_str (css_class);
  if (!widget->priv->css_classes.removeAll (cls)) return;
  if (QWidget *qw = gtkqt_qw (widget)) apply_css_class (qw, cls, false);
}

gboolean gtk_widget_has_css_class (GtkWidget *widget, const char *css_class)
{
  return widget->priv->css_classes.contains (gtkqt_str (css_class));
}

void gtk_widget_set_css_classes (GtkWidget *widget, const char **classes)
{
  for (const QString &c : QStringList (widget->priv->css_classes))
    gtk_widget_remove_css_class (widget, c.toUtf8 ().constData ());
  for (int i = 0; classes && classes[i]; i++)
    gtk_widget_add_css_class (widget, classes[i]);
}

/* GtkStyleContext is just the widget in disguise. */
GtkStyleContext *gtk_widget_get_style_context (GtkWidget *widget) { return (GtkStyleContext *) widget; }
void gtk_style_context_add_class (GtkStyleContext *c, const gchar *n) { gtk_widget_add_css_class ((GtkWidget *) c, n); }
void gtk_style_context_remove_class (GtkStyleContext *c, const gchar *n) { gtk_widget_remove_css_class ((GtkWidget *) c, n); }
gboolean gtk_style_context_has_class (GtkStyleContext *c, const gchar *n) { return gtk_widget_has_css_class ((GtkWidget *) c, n); }
void gtk_style_context_add_provider (GtkStyleContext *, gpointer, guint) {}
void gtk_style_context_add_provider_for_screen (GdkScreen *, gpointer, guint) {}
void gtk_style_context_add_provider_for_display (GdkDisplay *, gpointer, guint) {}

struct _GtkCssProvider { GObject parent_instance; };
typedef struct { GObjectClass parent_class; } GtkCssProviderClass;
G_DEFINE_TYPE (GtkCssProvider, gtk_css_provider, G_TYPE_OBJECT)
static void gtk_css_provider_init (GtkCssProvider *) {}
static void gtk_css_provider_class_init (GtkCssProviderClass *) {}
GtkCssProvider *gtk_css_provider_new (void) { return (GtkCssProvider *) g_object_new (gtk_css_provider_get_type (), nullptr); }
gboolean gtk_css_provider_load_from_data (GtkCssProvider *, const gchar *, gssize, GError **) { return TRUE; }
void gtk_css_provider_load_from_string (GtkCssProvider *, const char *) {}
void gtk_css_provider_load_from_path (GtkCssProvider *, const char *) {}
void gtk_css_provider_load_from_resource (GtkCssProvider *, const char *) {}

} /* extern "C" */

/* ======================================================================
 * GtkContainer / GtkBin
 * ====================================================================== */
enum { SIG_ADD, SIG_REMOVE, N_CONTAINER_SIGNALS };
static guint container_signals[N_CONTAINER_SIGNALS];

G_DEFINE_TYPE (GtkContainer, gtk_container, GTK_TYPE_WIDGET)

void gtkqt_set_place (GtkContainerClass *klass, GtkQtPlaceFunc place, GtkQtPlaceFunc unplace)
{
  klass->gtkqt_place = (gpointer) place;
  klass->gtkqt_unplace = (gpointer) unplace;
}

/* Default placement: append to the container QWidget's layout. */
static void container_place_default (GtkContainer *c, GtkWidget *child)
{
  QWidget *pw = gtkqt_content (GTK_WIDGET (c));
  QWidget *cw = gtkqt_qw (child);
  if (!pw || !cw) return;
  if (!pw->layout ()) {
    auto *l = new QVBoxLayout (pw);
    l->setContentsMargins (c->border_width, c->border_width, c->border_width, c->border_width);
    l->setSpacing (0);
  }
  pw->layout ()->addWidget (cw);
}

static void container_unplace_default (GtkContainer *, GtkWidget *child)
{
  if (QWidget *cw = gtkqt_qw (child)) {
    if (QWidget *pw = cw->parentWidget ())
      if (pw->layout ()) pw->layout ()->removeWidget (cw);
    cw->hide ();
    cw->setParent (nullptr);
  }
}

void gtkqt_container_adopt (GtkContainer *container, GtkWidget *child)
{
  GtkWidget *cw = GTK_WIDGET (container);
  g_object_ref_sink (child);
  child->parent = cw;
  cw->priv->children = g_list_append (cw->priv->children, child);
  if (GTK_IS_BIN (container)) GTK_BIN (container)->child = child;
  auto place = (GtkQtPlaceFunc) GTK_CONTAINER_GET_CLASS (container)->gtkqt_place;
  if (place) place (container, child);
  gtkqt_sync_visible (child);
  g_object_notify (G_OBJECT (child), "parent");
}

void gtkqt_container_release (GtkContainer *container, GtkWidget *child)
{
  GtkWidget *cw = GTK_WIDGET (container);
  if (!g_list_find (cw->priv->children, child)) return;
  auto unplace = (GtkQtPlaceFunc) GTK_CONTAINER_GET_CLASS (container)->gtkqt_unplace;
  if (unplace) unplace (container, child);
  cw->priv->children = g_list_remove (cw->priv->children, child);
  if (GTK_IS_BIN (container) && GTK_BIN (container)->child == child) GTK_BIN (container)->child = nullptr;
  child->parent = nullptr;
  g_object_unref (child);
}

static void container_real_add (GtkContainer *container, GtkWidget *child)
{
  gtkqt_container_adopt (container, child);
}

static void container_real_remove (GtkContainer *container, GtkWidget *child)
{
  gtkqt_container_release (container, child);
}

static void container_real_forall (GtkContainer *container, gboolean, GtkCallback cb, gpointer data)
{
  GList *copy = g_list_copy (GTK_WIDGET (container)->priv->children);
  for (GList *l = copy; l; l = l->next) cb ((GtkWidget *) l->data, data);
  g_list_free (copy);
}

static void container_destroy (GtkWidget *w)
{
  /* GTK semantics: destroying a container destroys its children. */
  GList *copy = g_list_copy (w->priv->children);
  for (GList *l = copy; l; l = l->next) gtk_widget_destroy ((GtkWidget *) l->data);
  g_list_free (copy);
  GTK_WIDGET_CLASS (gtk_container_parent_class)->destroy (w);
}

static void gtk_container_init (GtkContainer *) {}

static void gtk_container_class_init (GtkContainerClass *klass)
{
  GTK_WIDGET_CLASS (klass)->destroy = container_destroy;
  klass->add = container_real_add;
  klass->remove = container_real_remove;
  klass->forall = container_real_forall;
  gtkqt_set_place (klass, container_place_default, container_unplace_default);
  GType t = G_TYPE_FROM_CLASS (klass);
  container_signals[SIG_ADD] = g_signal_new ("add", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkContainerClass, add),
                                             nullptr, nullptr, nullptr, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
  container_signals[SIG_REMOVE] = g_signal_new ("remove", t, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkContainerClass, remove),
                                                nullptr, nullptr, nullptr, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
  gtkqt_prop (klass, g_param_spec_uint ("border-width", nullptr, nullptr, 0, 65535, 0, RW),
              [] (GObject *o, const GValue *v) { gtk_container_set_border_width (GTK_CONTAINER (o), g_value_get_uint (v)); },
              [] (GObject *o, GValue *v) { g_value_set_uint (v, GTK_CONTAINER (o)->border_width); });
  gtkqt_prop (klass, g_param_spec_object ("child", nullptr, nullptr, GTK_TYPE_WIDGET, RW),
              [] (GObject *o, const GValue *v) {
                if (GtkWidget *c = (GtkWidget *) g_value_get_object (v)) gtk_container_add (GTK_CONTAINER (o), c);
              },
              [] (GObject *o, GValue *v) { g_value_set_object (v, gtk_widget_get_first_child (GTK_WIDGET (o))); });
}

G_DEFINE_TYPE (GtkBin, gtk_bin, GTK_TYPE_CONTAINER)

void gtkqt_bin_place_simple (GtkContainer *c, GtkWidget *child) { container_place_default (c, child); }
void gtkqt_bin_unplace_simple (GtkContainer *c, GtkWidget *child) { container_unplace_default (c, child); }

static void bin_add (GtkContainer *container, GtkWidget *child)
{
  if (GTK_BIN (container)->child) {
    g_warning ("Attempting to add a widget with type %s to a %s, but as a GtkBin subclass a %s can only contain "
               "one widget at a time; it already contains a widget of type %s",
               G_OBJECT_TYPE_NAME (child), G_OBJECT_TYPE_NAME (container), G_OBJECT_TYPE_NAME (container),
               G_OBJECT_TYPE_NAME (GTK_BIN (container)->child));
    return;
  }
  GTK_CONTAINER_CLASS (gtk_bin_parent_class)->add (container, child);
}

static void gtk_bin_init (GtkBin *) {}
static void gtk_bin_class_init (GtkBinClass *klass) { GTK_CONTAINER_CLASS (klass)->add = bin_add; }

extern "C" {

void gtk_container_add (GtkContainer *container, GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_CONTAINER (container));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (widget->parent) {
    g_warning ("Attempting to add a widget with type %s to a container of type %s, but the widget is already "
               "inside a container of type %s", G_OBJECT_TYPE_NAME (widget), G_OBJECT_TYPE_NAME (container),
               G_OBJECT_TYPE_NAME (widget->parent));
    return;
  }
  g_signal_emit (container, container_signals[SIG_ADD], 0, widget);
}

void gtk_container_remove (GtkContainer *container, GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_CONTAINER (container));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (widget->parent != GTK_WIDGET (container)) return;
  g_object_ref (widget);
  g_signal_emit (container, container_signals[SIG_REMOVE], 0, widget);
  g_object_unref (widget);
}

void gtk_container_set_border_width (GtkContainer *container, guint border_width)
{
  container->border_width = border_width;
  apply_margins (GTK_WIDGET (container));
}

guint gtk_container_get_border_width (GtkContainer *container) { return container->border_width; }
void gtk_container_border_width (GtkContainer *container, guint border_width) { gtk_container_set_border_width (container, border_width); }

GList *gtk_container_get_children (GtkContainer *container)
{
  return g_list_copy (GTK_WIDGET (container)->priv->children);
}

void gtk_container_foreach (GtkContainer *container, GtkCallback callback, gpointer data)
{
  GTK_CONTAINER_GET_CLASS (container)->forall (container, FALSE, callback, data);
}

void gtk_container_forall (GtkContainer *container, GtkCallback callback, gpointer data)
{
  GTK_CONTAINER_GET_CLASS (container)->forall (container, TRUE, callback, data);
}

void gtk_container_set_focus_child (GtkContainer *, GtkWidget *child) { if (child) gtk_widget_grab_focus (child); }

static void child_set_valist (GtkContainer *container, GtkWidget *child, const gchar *name, va_list ap)
{
  /* The common GTK 2/3 child properties of boxes, tables, grids, notebooks and paneds. */
  GtkWidgetPrivate *p = child->priv;
  while (name) {
    QByteArray n = QByteArray (name).replace ('_', '-');
    if (n == "expand") p->pack_expand = va_arg (ap, gboolean);
    else if (n == "fill") p->pack_fill = va_arg (ap, gboolean);
    else if (n == "padding") p->pack_padding = va_arg (ap, guint);
    else if (n == "pack-type") p->pack_type = (GtkPackType) va_arg (ap, int);
    else if (n == "left-attach") p->grid_left = va_arg (ap, int);
    else if (n == "top-attach") p->grid_top = va_arg (ap, int);
    else if (n == "width") p->grid_width = va_arg (ap, int);
    else if (n == "height") p->grid_height = va_arg (ap, int);
    else if (n == "position") (void) va_arg (ap, int);
    else if (n == "tab-label") p->tab_label = gtkqt_str (va_arg (ap, const gchar *));
    else if (n == "title") p->tab_label = gtkqt_str (va_arg (ap, const gchar *));
    else if (n == "name") p->stack_name = gtkqt_str (va_arg (ap, const gchar *));
    else if (n == "resize" || n == "shrink" || n == "tab-expand" || n == "tab-fill" || n == "reorderable")
      (void) va_arg (ap, gboolean);
    else {
      g_warning ("gtk_container_child_set: unsupported child property '%s'", name);
      return;
    }
    name = va_arg (ap, const gchar *);
  }
  (void) container;
}

void gtk_container_add_with_properties (GtkContainer *container, GtkWidget *widget, const gchar *first_prop_name, ...)
{
  va_list ap;
  va_start (ap, first_prop_name);
  child_set_valist (container, widget, first_prop_name, ap);
  va_end (ap);
  gtk_container_add (container, widget);
}

void gtk_container_child_set (GtkContainer *container, GtkWidget *child, const gchar *first_prop_name, ...)
{
  va_list ap;
  va_start (ap, first_prop_name);
  child_set_valist (container, child, first_prop_name, ap);
  va_end (ap);
  gtkqt_box_relayout (GTK_WIDGET (container));
}

void gtk_container_resize_children (GtkContainer *) {}
void gtk_container_check_resize (GtkContainer *) {}
GtkWidget *gtk_bin_get_child (GtkBin *bin) { return bin->child; }

} /* extern "C" */
