/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Dialogs: GtkDialog (+ gtk_dialog_run), message/about dialogs, the file
 * chooser (QFileDialog), GtkFileChooserNative, the GTK 4.10 async
 * GtkFileDialog / GtkAlertDialog, and GtkColorButton.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QMimeDatabase>
#include <QEventLoop>

#define RW (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

/* ======================================================================
 * GtkDialog
 * ====================================================================== */
G_DEFINE_TYPE (GtkDialog, gtk_dialog, GTK_TYPE_WINDOW)

static void gtk_dialog_init (GtkDialog *) {}

static void dialog_constructed (GObject *o)
{
  G_OBJECT_CLASS (gtk_dialog_parent_class)->constructed (o);
  GtkDialog *d = GTK_DIALOG (o);
  /* GTK layout: the content vbox fills the window; the action area is packed
   * at its end. GTK 2 code reaches both through the public fields. */
  d->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  d->action_area = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_container_set_border_width (GTK_CONTAINER (d->vbox), 8);
  gtk_box_pack_end (GTK_BOX (d->vbox), d->action_area, FALSE, TRUE, 0);
  gtk_widget_show (d->action_area);
  gtk_widget_show (d->vbox);
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (d)); qw && !qobject_cast<QFileDialog *> (qw))
    gtk_container_add (GTK_CONTAINER (d), d->vbox);
  else
    g_object_ref_sink (d->vbox);   /* file chooser: Qt draws everything itself */
}

static void dialog_dispose (GObject *o)
{
  GtkDialog *d = GTK_DIALOG (o);
  if (d->vbox && !d->vbox->parent) { GtkWidget *v = d->vbox; d->vbox = nullptr; gtk_widget_destroy (v); g_object_unref (v); }
  d->vbox = nullptr;
  d->action_area = nullptr;
  G_OBJECT_CLASS (gtk_dialog_parent_class)->dispose (o);
}

static gboolean dialog_delete_event (GtkWidget *w, GdkEventAny *)
{
  gtk_dialog_response (GTK_DIALOG (w), GTK_RESPONSE_DELETE_EVENT);
  return FALSE;   /* destroy by default, like GTK */
}

static void gtk_dialog_class_init (GtkDialogClass *klass)
{
  G_OBJECT_CLASS (klass)->constructed = dialog_constructed;
  G_OBJECT_CLASS (klass)->dispose = dialog_dispose;
  GTK_WIDGET_CLASS (klass)->delete_event = dialog_delete_event;
  g_signal_new ("response", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GtkDialogClass, response),
                nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_INT);
  g_signal_new ("close", G_TYPE_FROM_CLASS (klass), (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                G_STRUCT_OFFSET (GtkDialogClass, close), nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  klass->close = [] (GtkDialog *d) { gtk_window_close (GTK_WINDOW (d)); };
}

static const char *response_key = "gtkqt-response-id";

static void action_clicked (GtkWidget *button, gpointer dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), response_key)));
}

static void add_buttons_valist (GtkDialog *dialog, const gchar *first, va_list ap)
{
  for (const gchar *text = first; text; text = va_arg (ap, const gchar *)) {
    gint id = va_arg (ap, gint);
    gtk_dialog_add_button (dialog, text, id);
  }
}

int gtkqt_dialog_run_loop (GtkDialog *dialog)
{
  int result = GTK_RESPONSE_NONE;
  QEventLoop loop;
  g_object_ref (dialog);
  struct Ctx { int *result; QEventLoop *loop; };
  Ctx ctx { &result, &loop };
  gulong h1 = g_signal_connect (dialog, "response", G_CALLBACK (+[] (GtkDialog *, gint id, gpointer p) {
    auto *c = static_cast<Ctx *> (p); *c->result = id; c->loop->quit (); }), &ctx);
  gulong h2 = g_signal_connect (dialog, "delete-event", G_CALLBACK (+[] (GtkWidget *, GdkEvent *, gpointer p) -> gboolean {
    auto *c = static_cast<Ctx *> (p); *c->result = GTK_RESPONSE_DELETE_EVENT; c->loop->quit (); return TRUE; }), &ctx);
  gulong h3 = g_signal_connect (dialog, "destroy", G_CALLBACK (+[] (GtkWidget *, gpointer p) {
    auto *c = static_cast<Ctx *> (p); c->loop->quit (); }), &ctx);
  QWidget *qw = gtkqt_qw (GTK_WIDGET (dialog));
  if (qw && !qw->isVisible ()) qw->setWindowModality (Qt::ApplicationModal);
  gtk_widget_show (GTK_WIDGET (dialog));
  loop.exec ();
  if (g_signal_handler_is_connected (dialog, h1)) g_signal_handler_disconnect (dialog, h1);
  if (g_signal_handler_is_connected (dialog, h2)) g_signal_handler_disconnect (dialog, h2);
  if (g_signal_handler_is_connected (dialog, h3)) g_signal_handler_disconnect (dialog, h3);
  g_object_unref (dialog);
  return result;
}

extern "C" {

GtkWidget *gtk_dialog_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_DIALOG, nullptr); }

GtkWidget *gtk_dialog_new_with_buttons (const gchar *title, GtkWindow *parent, GtkDialogFlags flags, const gchar *first_button_text, ...)
{
  GtkWidget *d = gtk_dialog_new ();
  if (title) gtk_window_set_title (GTK_WINDOW (d), title);
  if (parent) gtk_window_set_transient_for (GTK_WINDOW (d), parent);
  if (flags & GTK_DIALOG_MODAL) gtk_window_set_modal (GTK_WINDOW (d), TRUE);
  va_list ap;
  va_start (ap, first_button_text);
  add_buttons_valist (GTK_DIALOG (d), first_button_text, ap);
  va_end (ap);
  return d;
}

GtkWidget *gtk_dialog_add_button (GtkDialog *dialog, const gchar *button_text, gint response_id)
{
  GtkWidget *b = QString (button_text).startsWith ("gtk-") ? gtk_button_new_from_stock (button_text)
                                                           : gtk_button_new_with_mnemonic (button_text);
  gtk_dialog_add_action_widget (dialog, b, response_id);
  gtk_widget_show (b);
  return b;
}

void gtk_dialog_add_buttons (GtkDialog *dialog, const gchar *first_button_text, ...)
{
  va_list ap;
  va_start (ap, first_button_text);
  add_buttons_valist (dialog, first_button_text, ap);
  va_end (ap);
}

void gtk_dialog_add_action_widget (GtkDialog *dialog, GtkWidget *child, gint response_id)
{
  g_object_set_data (G_OBJECT (child), response_key, GINT_TO_POINTER (response_id));
  if (GTK_IS_BUTTON (child)) g_signal_connect (child, "clicked", G_CALLBACK (action_clicked), dialog);
  if (auto *fd = gtkqt_q<QFileDialog> (GTK_WIDGET (dialog))) {
    /* QFileDialog has its own buttons; remember which response means "accept" */
    if (response_id == GTK_RESPONSE_ACCEPT || response_id == GTK_RESPONSE_OK || response_id == GTK_RESPONSE_YES ||
        response_id == GTK_RESPONSE_APPLY) {
      g_object_set_data (G_OBJECT (dialog), "gtkqt-accept-id", GINT_TO_POINTER (response_id));
      if (const char *label = gtk_button_get_label (GTK_BUTTON (child)))
        fd->setLabelText (QFileDialog::Accept, gtkqt_mnemonic (label).remove ('&'));
    } else {
      g_object_set_data (G_OBJECT (dialog), "gtkqt-cancel-id", GINT_TO_POINTER (response_id));
    }
    g_object_ref_sink (child);
    g_object_unref (child);
    return;
  }
  gtk_box_pack_start (GTK_BOX (dialog->action_area), child, FALSE, TRUE, 0);
  if (QWidget *qw = gtkqt_qw (child)) qw->setMinimumWidth (80);
}

void gtk_dialog_set_default_response (GtkDialog *dialog, gint response_id)
{
  if (GtkWidget *b = gtk_dialog_get_widget_for_response (dialog, response_id)) gtk_widget_grab_default (b);
}

void gtk_dialog_set_response_sensitive (GtkDialog *dialog, gint response_id, gboolean setting)
{
  if (GtkWidget *b = gtk_dialog_get_widget_for_response (dialog, response_id)) gtk_widget_set_sensitive (b, setting);
}

GtkWidget *gtk_dialog_get_widget_for_response (GtkDialog *dialog, gint response_id)
{
  if (!dialog->action_area) return nullptr;
  for (GList *l = dialog->action_area->priv->children; l; l = l->next)
    if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (l->data), response_key)) == response_id) return (GtkWidget *) l->data;
  return nullptr;
}

void gtk_dialog_response (GtkDialog *dialog, gint response_id) { g_signal_emit_by_name (dialog, "response", response_id); }
gint gtk_dialog_run (GtkDialog *dialog) { return gtkqt_dialog_run_loop (dialog); }
GtkWidget *gtk_dialog_get_content_area (GtkDialog *dialog) { return dialog->vbox; }
GtkWidget *gtk_dialog_get_action_area (GtkDialog *dialog) { return dialog->action_area; }
void gtk_dialog_set_has_separator (GtkDialog *, gboolean) {}

} /* extern "C" */

/* ======================================================================
 * GtkMessageDialog
 * ====================================================================== */
G_DEFINE_TYPE (GtkMessageDialog, gtk_message_dialog, GTK_TYPE_DIALOG)
static void gtk_message_dialog_init (GtkMessageDialog *) {}
static void gtk_message_dialog_class_init (GtkMessageDialogClass *klass)
{
  gtkqt_prop (klass, g_param_spec_string ("text", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_label_set_text (GTK_LABEL (GTK_MESSAGE_DIALOG (o)->label), g_value_get_string (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_string ("secondary-text", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (o), "%s", g_value_get_string (v)); }, nullptr);
}

static GtkWidget *message_dialog_build (GtkWindow *parent, GtkDialogFlags flags, GtkMessageType type, GtkButtonsType buttons)
{
  auto *d = (GtkMessageDialog *) g_object_new (GTK_TYPE_MESSAGE_DIALOG, nullptr);
  GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  static const char *icons[] = { "dialog-information", "dialog-warning", "dialog-question", "dialog-error", nullptr };
  d->image = gtk_image_new_from_icon_name (icons[type <= GTK_MESSAGE_ERROR ? type : 4], GTK_ICON_SIZE_DIALOG);
  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  d->label = gtk_label_new (nullptr);
  gtk_label_set_xalign (GTK_LABEL (d->label), 0);
  gtk_label_set_wrap (GTK_LABEL (d->label), TRUE);
  if (QWidget *l = gtkqt_qw (d->label)) { QFont f = l->font (); f.setBold (true); l->setFont (f); }
  GtkWidget *secondary = gtk_label_new (nullptr);
  gtk_label_set_xalign (GTK_LABEL (secondary), 0);
  gtk_label_set_wrap (GTK_LABEL (secondary), TRUE);
  g_object_set_data (G_OBJECT (d), "gtkqt-secondary", secondary);
  gtk_box_pack_start (GTK_BOX (vbox), d->label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), secondary, FALSE, FALSE, 0);
  if (type <= GTK_MESSAGE_ERROR) gtk_box_pack_start (GTK_BOX (hbox), d->image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show_all (hbox);
  gtk_widget_hide (secondary);
  GtkDialog *dl = GTK_DIALOG (d);
  switch (buttons) {
  case GTK_BUTTONS_OK: gtk_dialog_add_button (dl, "_OK", GTK_RESPONSE_OK); break;
  case GTK_BUTTONS_CLOSE: gtk_dialog_add_button (dl, "_Close", GTK_RESPONSE_CLOSE); break;
  case GTK_BUTTONS_CANCEL: gtk_dialog_add_button (dl, "_Cancel", GTK_RESPONSE_CANCEL); break;
  case GTK_BUTTONS_YES_NO: gtk_dialog_add_button (dl, "_No", GTK_RESPONSE_NO); gtk_dialog_add_button (dl, "_Yes", GTK_RESPONSE_YES); break;
  case GTK_BUTTONS_OK_CANCEL: gtk_dialog_add_button (dl, "_Cancel", GTK_RESPONSE_CANCEL); gtk_dialog_add_button (dl, "_OK", GTK_RESPONSE_OK); break;
  default: break;
  }
  if (parent) gtk_window_set_transient_for (GTK_WINDOW (d), parent);
  if (flags & GTK_DIALOG_MODAL) gtk_window_set_modal (GTK_WINDOW (d), TRUE);
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (d))) qw->setMinimumWidth (320);
  return GTK_WIDGET (d);
}

extern "C" {

GtkWidget *gtk_message_dialog_new (GtkWindow *parent, GtkDialogFlags flags, GtkMessageType type, GtkButtonsType buttons,
                                   const gchar *message_format, ...)
{
  GtkWidget *d = message_dialog_build (parent, flags, type, buttons);
  if (message_format) {
    va_list ap;
    va_start (ap, message_format);
    gchar *msg = g_strdup_vprintf (message_format, ap);
    va_end (ap);
    gtk_label_set_text (GTK_LABEL (GTK_MESSAGE_DIALOG (d)->label), msg);
    g_free (msg);
  }
  return d;
}

GtkWidget *gtk_message_dialog_new_with_markup (GtkWindow *parent, GtkDialogFlags flags, GtkMessageType type, GtkButtonsType buttons,
                                               const gchar *message_format, ...)
{
  GtkWidget *d = message_dialog_build (parent, flags, type, buttons);
  if (message_format) {
    va_list ap;
    va_start (ap, message_format);
    gchar *msg = g_markup_vprintf_escaped (message_format, ap);
    va_end (ap);
    gtk_label_set_markup (GTK_LABEL (GTK_MESSAGE_DIALOG (d)->label), msg);
    g_free (msg);
  }
  return d;
}

void gtk_message_dialog_set_markup (GtkMessageDialog *d, const gchar *str) { gtk_label_set_markup (GTK_LABEL (d->label), str); }

static void secondary (GtkMessageDialog *d, gchar *msg, bool markup)
{
  auto *l = (GtkWidget *) g_object_get_data (G_OBJECT (d), "gtkqt-secondary");
  if (markup) gtk_label_set_markup (GTK_LABEL (l), msg); else gtk_label_set_text (GTK_LABEL (l), msg);
  gtk_widget_set_visible (l, msg != nullptr);
  g_free (msg);
}

void gtk_message_dialog_format_secondary_text (GtkMessageDialog *d, const gchar *fmt, ...)
{
  gchar *msg = nullptr;
  if (fmt) { va_list ap; va_start (ap, fmt); msg = g_strdup_vprintf (fmt, ap); va_end (ap); }
  secondary (d, msg, false);
}

void gtk_message_dialog_format_secondary_markup (GtkMessageDialog *d, const gchar *fmt, ...)
{
  gchar *msg = nullptr;
  if (fmt) { va_list ap; va_start (ap, fmt); msg = g_strdup_vprintf (fmt, ap); va_end (ap); }
  secondary (d, msg, true);
}

GtkWidget *gtk_message_dialog_get_message_area (GtkMessageDialog *d) { return d->label->parent; }

} /* extern "C" */

/* ======================================================================
 * GtkAboutDialog
 * ====================================================================== */
struct AboutData {
  QString name, version, copyright, comments, license, website, website_label, translators, logo_icon;
  QStringList authors, documenters, artists;
  QLabel *title = nullptr, *body = nullptr, *logo = nullptr;
  QTextEdit *license_view = nullptr;
};

static AboutData *about (GtkAboutDialog *a) { return static_cast<AboutData *> (g_object_get_data (G_OBJECT (a), "gtkqt-about")); }

static void about_refresh (GtkAboutDialog *a)
{
  AboutData *d = about (a);
  if (!d || !d->title) return;
  QString name = d->name.isEmpty () ? QCoreApplication::applicationName () : d->name;
  d->title->setText (QStringLiteral ("<span style='font-size:large; font-weight:bold'>%1 %2</span>")
                       .arg (name.toHtmlEscaped (), d->version.toHtmlEscaped ()));
  QStringList parts;
  if (!d->comments.isEmpty ()) parts << d->comments.toHtmlEscaped ();
  if (!d->website.isEmpty ())
    parts << QStringLiteral ("<a href='%1'>%2</a>").arg (d->website, (d->website_label.isEmpty () ? d->website : d->website_label).toHtmlEscaped ());
  if (!d->copyright.isEmpty ()) parts << QStringLiteral ("<small>%1</small>").arg (d->copyright.toHtmlEscaped ());
  if (!d->authors.isEmpty ()) parts << QStringLiteral ("<small><b>Written by</b><br>%1</small>").arg (d->authors.join ("<br>").toHtmlEscaped ().replace ("&lt;br&gt;", "<br>"));
  d->body->setText (parts.join ("<br><br>").replace ('\n', "<br>"));
  d->license_view->setPlainText (d->license);
  d->license_view->setVisible (!d->license.isEmpty ());
  QIcon icon = gtkqt_icon (d->logo_icon.isEmpty () ? nullptr : d->logo_icon.toUtf8 ().constData ());
  if (icon.isNull ()) icon = QApplication::windowIcon ();
  d->logo->setPixmap (icon.pixmap (64, 64));
  d->logo->setVisible (!icon.isNull ());
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (a))) qw->setWindowTitle (QStringLiteral ("About %1").arg (name));
}

static void about_constructed (GObject *o);

G_DEFINE_TYPE (GtkAboutDialog, gtk_about_dialog, GTK_TYPE_DIALOG)
static void gtk_about_dialog_init (GtkAboutDialog *a)
{
  g_object_set_data_full (G_OBJECT (a), "gtkqt-about", new AboutData, [] (gpointer p) { delete static_cast<AboutData *> (p); });
}

#define ABOUT_STR(prop, field)                                                                          \
  gtkqt_prop (klass, g_param_spec_string (prop, nullptr, nullptr, nullptr, RW),                         \
              [] (GObject *o, const GValue *v) { about (GTK_ABOUT_DIALOG (o))->field = gtkqt_str (g_value_get_string (v)); \
                                                 about_refresh (GTK_ABOUT_DIALOG (o)); },                \
              [] (GObject *o, GValue *v) { g_value_set_string (v, about (GTK_ABOUT_DIALOG (o))->field.toUtf8 ().constData ()); })
#define ABOUT_STRV(prop, field)                                                                         \
  gtkqt_prop (klass, g_param_spec_boxed (prop, nullptr, nullptr, G_TYPE_STRV, RW),                      \
              [] (GObject *o, const GValue *v) {                                                        \
                QStringList l; for (auto **s = (gchar **) g_value_get_boxed (v); s && *s; s++) l << gtkqt_str (*s); \
                about (GTK_ABOUT_DIALOG (o))->field = l; about_refresh (GTK_ABOUT_DIALOG (o)); }, nullptr)

static void gtk_about_dialog_class_init (GtkAboutDialogClass *klass)
{
  G_OBJECT_CLASS (klass)->constructed = about_constructed;
  ABOUT_STR ("program-name", name);
  ABOUT_STR ("name", name);
  ABOUT_STR ("version", version);
  ABOUT_STR ("copyright", copyright);
  ABOUT_STR ("comments", comments);
  ABOUT_STR ("license", license);
  ABOUT_STR ("website", website);
  ABOUT_STR ("website-label", website_label);
  ABOUT_STR ("translator-credits", translators);
  ABOUT_STR ("logo-icon-name", logo_icon);
  ABOUT_STRV ("authors", authors);
  ABOUT_STRV ("documenters", documenters);
  ABOUT_STRV ("artists", artists);
  gtkqt_prop (klass, g_param_spec_enum ("license-type", nullptr, nullptr, gtk_license_get_type (), GTK_LICENSE_UNKNOWN, RW),
              [] (GObject *o, const GValue *v) { gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (o), (GtkLicense) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("wrap-license", nullptr, nullptr, FALSE, RW), [] (GObject *, const GValue *) {}, nullptr);
  gtkqt_prop (klass, g_param_spec_object ("logo", nullptr, nullptr, G_TYPE_OBJECT, RW), [] (GObject *, const GValue *) {}, nullptr);
}

static void about_constructed (GObject *o)
{
  G_OBJECT_CLASS (gtk_about_dialog_parent_class)->constructed (o);
  auto *a = GTK_ABOUT_DIALOG (o);
  AboutData *d = about (a);
  QWidget *content = gtkqt_qw (GTK_DIALOG (a)->vbox);
  auto *panel = new QWidget;
  auto *l = new QVBoxLayout (panel);
  d->logo = new QLabel (panel);
  d->logo->setAlignment (Qt::AlignCenter);
  d->title = new QLabel (panel);
  d->title->setAlignment (Qt::AlignCenter);
  d->body = new QLabel (panel);
  d->body->setAlignment (Qt::AlignCenter);
  d->body->setWordWrap (true);
  d->body->setOpenExternalLinks (true);
  d->license_view = new QTextEdit (panel);
  d->license_view->setReadOnly (true);
  d->license_view->setMinimumHeight (120);
  l->addWidget (d->logo);
  l->addWidget (d->title);
  l->addWidget (d->body);
  l->addWidget (d->license_view, 1);
  static_cast<QBoxLayout *> (content->layout ())->insertWidget (0, panel, 1);
  gtk_dialog_add_button (GTK_DIALOG (a), "_Close", GTK_RESPONSE_CLOSE);
  content->setMinimumWidth (360);
  about_refresh (a);
  if (gtkqt_api >= 4) gtk_window_set_hide_on_close (GTK_WINDOW (a), TRUE);
}

extern "C" {

GtkWidget *gtk_about_dialog_new (void) { return (GtkWidget *) g_object_new (GTK_TYPE_ABOUT_DIALOG, nullptr); }

void gtk_show_about_dialog (GtkWindow *parent, const gchar *first_property_name, ...)
{
  va_list ap;
  va_start (ap, first_property_name);
  GtkWidget *d = (GtkWidget *) g_object_new_valist (GTK_TYPE_ABOUT_DIALOG, first_property_name, ap);
  va_end (ap);
  if (parent) gtk_window_set_transient_for (GTK_WINDOW (d), parent);
  g_signal_connect (d, "response", G_CALLBACK (+[] (GtkWidget *w, gint) { gtk_widget_destroy (w); }), nullptr);
  gtk_window_present (GTK_WINDOW (d));
}

#define ABOUT_SETTER(fn, field) \
  void fn (GtkAboutDialog *a, const gchar *s) { about (a)->field = gtkqt_str (s); about_refresh (a); }
ABOUT_SETTER (gtk_about_dialog_set_program_name, name)
ABOUT_SETTER (gtk_about_dialog_set_name, name)
ABOUT_SETTER (gtk_about_dialog_set_version, version)
ABOUT_SETTER (gtk_about_dialog_set_copyright, copyright)
ABOUT_SETTER (gtk_about_dialog_set_comments, comments)
ABOUT_SETTER (gtk_about_dialog_set_license, license)
ABOUT_SETTER (gtk_about_dialog_set_website, website)
ABOUT_SETTER (gtk_about_dialog_set_website_label, website_label)
ABOUT_SETTER (gtk_about_dialog_set_translator_credits, translators)
ABOUT_SETTER (gtk_about_dialog_set_logo_icon_name, logo_icon)

static QStringList strv (const gchar **v) { QStringList l; for (; v && *v; v++) l << gtkqt_str (*v); return l; }
void gtk_about_dialog_set_authors (GtkAboutDialog *a, const gchar **v) { about (a)->authors = strv (v); about_refresh (a); }
void gtk_about_dialog_set_documenters (GtkAboutDialog *a, const gchar **v) { about (a)->documenters = strv (v); about_refresh (a); }
void gtk_about_dialog_set_artists (GtkAboutDialog *a, const gchar **v) { about (a)->artists = strv (v); about_refresh (a); }
void gtk_about_dialog_set_wrap_license (GtkAboutDialog *, gboolean) {}
void gtk_about_dialog_set_logo (GtkAboutDialog *, GdkPixbuf *) {}

void gtk_about_dialog_set_license_type (GtkAboutDialog *a, GtkLicense t)
{
  static const char *names[] = { nullptr, nullptr, "GNU General Public License, version 2 or later",
    "GNU General Public License, version 3 or later", "GNU Lesser General Public License, version 2.1 or later",
    "GNU Lesser General Public License, version 3 or later", "BSD 2-Clause License", "The MIT License (MIT)",
    "Artistic License 2.0", "GNU General Public License, version 2 only", "GNU General Public License, version 3 only",
    "GNU Lesser General Public License, version 2.1 only", "GNU Lesser General Public License, version 3 only",
    "GNU Affero General Public License, version 3 or later", "GNU Affero General Public License, version 3 only",
    "BSD 3-Clause License", "Apache License, Version 2.0", "Mozilla Public License 2.0" };
  if ((unsigned) t < G_N_ELEMENTS (names) && names[t])
    gtk_about_dialog_set_license (a, QStringLiteral ("This program comes with absolutely no warranty.\nSee the %1 for details.")
                                       .arg (names[t]).toUtf8 ().constData ());
}

const gchar *gtk_about_dialog_get_program_name (GtkAboutDialog *a) { return gtkqt_ret (GTK_WIDGET (a), about (a)->name); }
const gchar *gtk_about_dialog_get_version (GtkAboutDialog *a) { return gtkqt_ret (GTK_WIDGET (a), about (a)->version); }

} /* extern "C" */

/* ======================================================================
 * GtkFileFilter
 * ====================================================================== */
struct _GtkFileFilter { GInitiallyUnowned parent_instance; gchar *name; GPtrArray *patterns; };
typedef struct { GInitiallyUnownedClass parent_class; } GtkFileFilterClass;
G_DEFINE_TYPE (GtkFileFilter, gtk_file_filter, G_TYPE_INITIALLY_UNOWNED)
static void gtk_file_filter_init (GtkFileFilter *f) { f->patterns = g_ptr_array_new_with_free_func (g_free); }
static void gtk_file_filter_finalize (GObject *o)
{
  auto *f = GTK_FILE_FILTER (o);
  g_free (f->name);
  g_ptr_array_unref (f->patterns);
  G_OBJECT_CLASS (gtk_file_filter_parent_class)->finalize (o);
}
static void gtk_file_filter_class_init (GtkFileFilterClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = gtk_file_filter_finalize;
  gtkqt_prop (klass, g_param_spec_string ("name", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_file_filter_set_name (GTK_FILE_FILTER (o), g_value_get_string (v)); },
              [] (GObject *o, GValue *v) { g_value_set_string (v, GTK_FILE_FILTER (o)->name); });
}

static QString filter_string (GtkFileFilter *f)
{
  QStringList pats;
  for (guint i = 0; i < f->patterns->len; i++) pats << gtkqt_str ((const char *) g_ptr_array_index (f->patterns, i));
  if (pats.isEmpty ()) pats << "*";
  QString name = f->name ? gtkqt_str (f->name) : pats.join (' ');
  return QStringLiteral ("%1 (%2)").arg (name, pats.join (' '));
}

extern "C" {
GtkFileFilter *gtk_file_filter_new (void) { return (GtkFileFilter *) g_object_new (GTK_TYPE_FILE_FILTER, nullptr); }
void gtk_file_filter_set_name (GtkFileFilter *f, const gchar *name) { g_free (f->name); f->name = g_strdup (name); }
const gchar *gtk_file_filter_get_name (GtkFileFilter *f) { return f->name; }
void gtk_file_filter_add_pattern (GtkFileFilter *f, const gchar *pattern) { g_ptr_array_add (f->patterns, g_strdup (pattern)); }
void gtk_file_filter_add_suffix (GtkFileFilter *f, const gchar *suffix) { g_ptr_array_add (f->patterns, g_strdup_printf ("*.%s", suffix)); }

void gtk_file_filter_add_mime_type (GtkFileFilter *f, const gchar *mime_type)
{
  QMimeDatabase db;
  QString m = gtkqt_str (mime_type);
  QStringList globs;
  if (m.endsWith ("/*")) {
    for (const QMimeType &t : db.allMimeTypes ()) if (t.name ().startsWith (m.chopped (1))) globs += t.globPatterns ();
  } else {
    globs = db.mimeTypeForName (m).globPatterns ();
  }
  for (const QString &g : globs) g_ptr_array_add (f->patterns, g_strdup (g.toUtf8 ().constData ()));
}

void gtk_file_filter_add_pixbuf_formats (GtkFileFilter *f)
{
  for (const char *p : { "*.png", "*.jpg", "*.jpeg", "*.gif", "*.bmp", "*.svg", "*.webp", "*.xpm", "*.ico" })
    gtk_file_filter_add_pattern (f, p);
}
}

/* ======================================================================
 * GtkFileChooserDialog (a QFileDialog) and GtkFileChooserNative
 * ====================================================================== */
static QObject *file_chooser_create (GtkWidget *w)
{
  auto *fd = new QFileDialog;
  fd->setOption (QFileDialog::DontUseNativeDialog, true);
  fd->setWindowFlag (Qt::Window);
  gtkqt_connect (w, fd, &QDialog::finished, [] (GtkWidget *dw, int result) {
    int id = result == QDialog::Accepted
             ? GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dw), "gtkqt-accept-id"))
             : GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dw), "gtkqt-cancel-id"));
    if (!id) id = result == QDialog::Accepted ? GTK_RESPONSE_ACCEPT : GTK_RESPONSE_CANCEL;
    dw->priv->visible = false;
    gtk_dialog_response (GTK_DIALOG (dw), id);
  });
  return fd;
}

G_DEFINE_TYPE (GtkFileChooserDialog, gtk_file_chooser_dialog, GTK_TYPE_DIALOG)
static void gtk_file_chooser_dialog_init (GtkFileChooserDialog *) {}
static void gtk_file_chooser_dialog_class_init (GtkFileChooserDialogClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), file_chooser_create);
  gtkqt_prop (klass, g_param_spec_enum ("action", nullptr, nullptr, gtk_file_chooser_action_get_type (), 0, RW),
              [] (GObject *o, const GValue *v) { gtk_file_chooser_set_action ((GtkFileChooser *) o, (GtkFileChooserAction) g_value_get_enum (v)); }, nullptr);
  gtkqt_prop (klass, g_param_spec_boolean ("select-multiple", nullptr, nullptr, FALSE, RW),
              [] (GObject *o, const GValue *v) { gtk_file_chooser_set_select_multiple ((GtkFileChooser *) o, g_value_get_boolean (v)); }, nullptr);
}

/* GtkNativeDialog: owns a GtkFileChooserDialog and forwards to it */
G_DEFINE_TYPE (GtkNativeDialog, gtk_native_dialog, G_TYPE_OBJECT)
static void gtk_native_dialog_init (GtkNativeDialog *) {}
static void native_dispose (GObject *o)
{
  auto *n = (GtkNativeDialog *) o;
  if (n->priv) { GtkWidget *d = (GtkWidget *) n->priv; n->priv = nullptr; gtk_widget_destroy (d); g_object_unref (d); }
  G_OBJECT_CLASS (gtk_native_dialog_parent_class)->dispose (o);
}
static void gtk_native_dialog_class_init (GtkNativeDialogClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = native_dispose;
  g_signal_new ("response", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_INT);
}

static QFileDialog *chooser (GtkFileChooser *c)
{
  GObject *o = G_OBJECT (c);
  if (G_TYPE_CHECK_INSTANCE_TYPE (o, gtk_native_dialog_get_type ())) o = (GObject *) ((GtkNativeDialog *) o)->priv;
  return o ? gtkqt_q<QFileDialog> (GTK_WIDGET (o)) : nullptr;
}

extern "C" {

GtkWidget *gtk_file_chooser_dialog_new (const gchar *title, GtkWindow *parent, GtkFileChooserAction action, const gchar *first_button_text, ...)
{
  GtkWidget *d = (GtkWidget *) g_object_new (GTK_TYPE_FILE_CHOOSER_DIALOG, nullptr);
  if (title) gtk_window_set_title (GTK_WINDOW (d), title);
  if (parent) gtk_window_set_transient_for (GTK_WINDOW (d), parent);
  gtk_file_chooser_set_action ((GtkFileChooser *) d, action);
  va_list ap;
  va_start (ap, first_button_text);
  add_buttons_valist (GTK_DIALOG (d), first_button_text, ap);
  va_end (ap);
  return d;
}

void gtk_file_chooser_set_action (GtkFileChooser *c, GtkFileChooserAction action)
{
  QFileDialog *fd = chooser (c);
  if (!fd) return;
  switch (action) {
  case GTK_FILE_CHOOSER_ACTION_SAVE:
    fd->setAcceptMode (QFileDialog::AcceptSave);
    fd->setFileMode (QFileDialog::AnyFile);
    break;
  case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
  case GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
    fd->setAcceptMode (QFileDialog::AcceptOpen);
    fd->setFileMode (QFileDialog::Directory);
    fd->setOption (QFileDialog::ShowDirsOnly, true);
    break;
  default:
    fd->setAcceptMode (QFileDialog::AcceptOpen);
    fd->setFileMode (QFileDialog::ExistingFile);
  }
}

void gtk_file_chooser_set_select_multiple (GtkFileChooser *c, gboolean multiple)
{
  QFileDialog *fd = chooser (c);
  if (fd && fd->acceptMode () == QFileDialog::AcceptOpen && fd->fileMode () != QFileDialog::Directory)
    fd->setFileMode (multiple ? QFileDialog::ExistingFiles : QFileDialog::ExistingFile);
}

void gtk_file_chooser_set_do_overwrite_confirmation (GtkFileChooser *c, gboolean confirm)
{
  if (QFileDialog *fd = chooser (c)) fd->setOption (QFileDialog::DontConfirmOverwrite, !confirm);
}

void gtk_file_chooser_set_current_name (GtkFileChooser *c, const gchar *name) { if (QFileDialog *fd = chooser (c)) fd->selectFile (gtkqt_str (name)); }
gboolean gtk_file_chooser_set_filename (GtkFileChooser *c, const char *filename) { if (QFileDialog *fd = chooser (c)) fd->selectFile (gtkqt_str (filename)); return TRUE; }
gboolean gtk_file_chooser_set_current_folder (GtkFileChooser *c, const gchar *filename) { if (QFileDialog *fd = chooser (c)) fd->setDirectory (gtkqt_str (filename)); return TRUE; }

gboolean gtk_file_chooser_set_current_folder_file (GtkFileChooser *c, GFile *file, GError **)
{
  gchar *p = file ? g_file_get_path (file) : nullptr;
  gtk_file_chooser_set_current_folder (c, p);
  g_free (p);
  return TRUE;
}

gboolean gtk_file_chooser_set_file (GtkFileChooser *c, GFile *file, GError **)
{
  gchar *p = file ? g_file_get_path (file) : nullptr;
  gtk_file_chooser_set_filename (c, p);
  g_free (p);
  return TRUE;
}

gchar *gtk_file_chooser_get_filename (GtkFileChooser *c)
{
  QFileDialog *fd = chooser (c);
  QStringList f = fd ? fd->selectedFiles () : QStringList ();
  return f.isEmpty () ? nullptr : g_strdup (f.first ().toUtf8 ().constData ());
}

GSList *gtk_file_chooser_get_filenames (GtkFileChooser *c)
{
  GSList *l = nullptr;
  if (QFileDialog *fd = chooser (c))
    for (const QString &f : fd->selectedFiles ()) l = g_slist_append (l, g_strdup (f.toUtf8 ().constData ()));
  return l;
}

gchar *gtk_file_chooser_get_uri (GtkFileChooser *c)
{
  gchar *f = gtk_file_chooser_get_filename (c);
  gchar *uri = f ? g_filename_to_uri (f, nullptr, nullptr) : nullptr;
  g_free (f);
  return uri;
}

gchar *gtk_file_chooser_get_current_folder (GtkFileChooser *c)
{
  QFileDialog *fd = chooser (c);
  return fd ? g_strdup (fd->directory ().absolutePath ().toUtf8 ().constData ()) : nullptr;
}

GFile *gtk_file_chooser_get_file (GtkFileChooser *c)
{
  gchar *f = gtk_file_chooser_get_filename (c);
  GFile *file = f ? g_file_new_for_path (f) : nullptr;
  g_free (f);
  return file;
}

GListModel *gtk_file_chooser_get_files (GtkFileChooser *c)
{
  GListStore *s = g_list_store_new (G_TYPE_FILE);
  if (QFileDialog *fd = chooser (c))
    for (const QString &f : fd->selectedFiles ()) {
      GFile *file = g_file_new_for_path (f.toUtf8 ().constData ());
      g_list_store_append (s, file);
      g_object_unref (file);
    }
  return G_LIST_MODEL (s);
}

void gtk_file_chooser_add_filter (GtkFileChooser *c, GtkFileFilter *filter)
{
  QFileDialog *fd = chooser (c);
  g_object_ref_sink (filter);
  if (fd) fd->setNameFilters (fd->nameFilters ().filter (QRegularExpression (".")) << filter_string (filter));
  g_object_unref (filter);
  /* QFileDialog starts with a default "All files" entry; drop it once real filters exist */
  if (fd && fd->nameFilters ().size () > 1 && fd->nameFilters ().first () == QFileDialog::tr ("All Files (*)")) {
    QStringList l = fd->nameFilters ();
    l.removeFirst ();
    fd->setNameFilters (l);
  }
}

void gtk_file_chooser_set_filter (GtkFileChooser *c, GtkFileFilter *filter)
{
  if (QFileDialog *fd = chooser (c)) fd->selectNameFilter (filter_string (filter));
}

GtkFileChooserNative *gtk_file_chooser_native_new (const gchar *title, GtkWindow *parent, GtkFileChooserAction action,
                                                   const gchar *accept_label, const gchar *cancel_label)
{
  auto *n = (GtkNativeDialog *) g_object_new (gtk_native_dialog_get_type (), nullptr);
  GtkWidget *d = gtk_file_chooser_dialog_new (title, parent, action,
                                              cancel_label ? cancel_label : "_Cancel", GTK_RESPONSE_CANCEL,
                                              accept_label ? accept_label : (action == GTK_FILE_CHOOSER_ACTION_SAVE ? "_Save" : "_Open"),
                                              GTK_RESPONSE_ACCEPT, nullptr);
  g_object_ref_sink (d);
  n->priv = d;
  g_signal_connect_object (d, "response", G_CALLBACK (+[] (GtkWidget *, gint id, gpointer native) {
    g_signal_emit_by_name (native, "response", id);
  }), n, (GConnectFlags) 0);
  return n;
}

void gtk_native_dialog_show (GtkNativeDialog *self) { if (self->priv) gtk_widget_show ((GtkWidget *) self->priv); }
void gtk_native_dialog_hide (GtkNativeDialog *self) { if (self->priv) gtk_widget_hide ((GtkWidget *) self->priv); }
void gtk_native_dialog_destroy (GtkNativeDialog *self) { g_object_run_dispose (G_OBJECT (self)); }
gint gtk_native_dialog_run (GtkNativeDialog *self) { return self->priv ? gtk_dialog_run (GTK_DIALOG (self->priv)) : GTK_RESPONSE_CANCEL; }
void gtk_native_dialog_set_modal (GtkNativeDialog *self, gboolean modal) { if (self->priv) gtk_window_set_modal (GTK_WINDOW (self->priv), modal); }

} /* extern "C" */

/* ======================================================================
 * GtkFileDialog / GtkAlertDialog (GTK 4.10+)
 * ====================================================================== */
struct _GtkFileDialog { GObject parent_instance; gchar *title, *initial_name; GFile *initial_folder; GListModel *filters; };
typedef struct { GObjectClass parent_class; } GtkFileDialogClass;
G_DEFINE_TYPE (GtkFileDialog, gtk_file_dialog, G_TYPE_OBJECT)
static void gtk_file_dialog_init (GtkFileDialog *) {}
static void gtk_file_dialog_finalize (GObject *o)
{
  auto *d = GTK_FILE_DIALOG (o);
  g_free (d->title);
  g_free (d->initial_name);
  g_clear_object (&d->initial_folder);
  g_clear_object (&d->filters);
  G_OBJECT_CLASS (gtk_file_dialog_parent_class)->finalize (o);
}
static void gtk_file_dialog_class_init (GtkFileDialogClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = gtk_file_dialog_finalize;
  gtkqt_prop (klass, g_param_spec_string ("title", nullptr, nullptr, nullptr, RW),
              [] (GObject *o, const GValue *v) { gtk_file_dialog_set_title (GTK_FILE_DIALOG (o), g_value_get_string (v)); }, nullptr);
}

static void file_dialog_run (GtkFileDialog *self, GtkWindow *parent, QFileDialog::AcceptMode mode, QFileDialog::FileMode fmode,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  gtkqt_ensure_app ();
  GTask *task = g_task_new (self, cancellable, callback, user_data);
  QWidget *pw = parent ? gtkqt_qw (GTK_WIDGET (parent)) : nullptr;
  auto *fd = new QFileDialog (pw, gtkqt_str (self->title));
  fd->setOption (QFileDialog::DontUseNativeDialog, true);
  fd->setAcceptMode (mode);
  fd->setFileMode (fmode);
  if (self->initial_folder) { gchar *p = g_file_get_path (self->initial_folder); fd->setDirectory (gtkqt_str (p)); g_free (p); }
  if (self->initial_name) fd->selectFile (gtkqt_str (self->initial_name));
  if (self->filters) {
    QStringList f;
    for (guint i = 0; i < g_list_model_get_n_items (self->filters); i++) {
      auto *flt = (GtkFileFilter *) g_list_model_get_item (self->filters, i);
      f << filter_string (flt);
      g_object_unref (flt);
    }
    fd->setNameFilters (f);
  }
  fd->setAttribute (Qt::WA_DeleteOnClose);
  QObject::connect (fd, &QDialog::finished, fd, [fd, task] (int result) {
    GtkQtEmitGuard guard;
    QStringList files = fd->selectedFiles ();
    if (result == QDialog::Accepted && !files.isEmpty ())
      g_task_return_pointer (task, g_file_new_for_path (files.first ().toUtf8 ().constData ()), g_object_unref);
    else
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Dismissed by user");
    g_object_unref (task);
  });
  fd->open ();
}

struct _GtkAlertDialog { GObject parent_instance; gchar *message, *detail; GStrv buttons; int cancel, def; };
typedef struct { GObjectClass parent_class; } GtkAlertDialogClass;
G_DEFINE_TYPE (GtkAlertDialog, gtk_alert_dialog, G_TYPE_OBJECT)
static void gtk_alert_dialog_init (GtkAlertDialog *a) { a->cancel = a->def = -1; }
static void gtk_alert_dialog_finalize (GObject *o)
{
  auto *a = GTK_ALERT_DIALOG (o);
  g_free (a->message);
  g_free (a->detail);
  g_strfreev (a->buttons);
  G_OBJECT_CLASS (gtk_alert_dialog_parent_class)->finalize (o);
}
static void gtk_alert_dialog_class_init (GtkAlertDialogClass *klass) { G_OBJECT_CLASS (klass)->finalize = gtk_alert_dialog_finalize; }

static void alert_run (GtkAlertDialog *self, GtkWindow *parent, GTask *task)
{
  gtkqt_ensure_app ();
  QWidget *pw = parent ? gtkqt_qw (GTK_WIDGET (parent)) : nullptr;
  auto *box = new QMessageBox (QMessageBox::NoIcon, QString (), gtkqt_str (self->message), QMessageBox::NoButton, pw);
  box->setInformativeText (gtkqt_str (self->detail));
  box->setAttribute (Qt::WA_DeleteOnClose);
  QList<QAbstractButton *> buttons;
  if (self->buttons && *self->buttons) {
    for (int i = 0; self->buttons[i]; i++)
      buttons << box->addButton (gtkqt_mnemonic (self->buttons[i]), i == self->cancel ? QMessageBox::RejectRole : QMessageBox::AcceptRole);
  } else {
    buttons << box->addButton (QMessageBox::Close);
  }
  if (self->def >= 0 && self->def < buttons.size ()) box->setDefaultButton (qobject_cast<QPushButton *> (buttons[self->def]));
  QObject::connect (box, &QDialog::finished, box, [box, buttons, task, self] (int) {
    if (!task) return;
    GtkQtEmitGuard guard;
    int idx = buttons.indexOf (box->clickedButton ());
    if (idx < 0) idx = self->cancel;
    if (idx < 0) g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Dismissed by user");
    else g_task_return_int (task, idx);
    g_object_unref (task);
  });
  box->open ();
}

extern "C" {

GtkFileDialog *gtk_file_dialog_new (void) { return (GtkFileDialog *) g_object_new (GTK_TYPE_FILE_DIALOG, nullptr); }
void gtk_file_dialog_set_title (GtkFileDialog *self, const char *title) { g_free (self->title); self->title = g_strdup (title); }
void gtk_file_dialog_set_initial_name (GtkFileDialog *self, const char *name) { g_free (self->initial_name); self->initial_name = g_strdup (name); }
void gtk_file_dialog_set_initial_folder (GtkFileDialog *self, GFile *folder) { g_set_object (&self->initial_folder, folder); }
void gtk_file_dialog_set_filters (GtkFileDialog *self, GListModel *filters) { g_set_object (&self->filters, filters); }
void gtk_file_dialog_set_default_filter (GtkFileDialog *, GtkFileFilter *) {}

void gtk_file_dialog_open (GtkFileDialog *self, GtkWindow *parent, GCancellable *c, GAsyncReadyCallback cb, gpointer data)
{ file_dialog_run (self, parent, QFileDialog::AcceptOpen, QFileDialog::ExistingFile, c, cb, data); }
void gtk_file_dialog_save (GtkFileDialog *self, GtkWindow *parent, GCancellable *c, GAsyncReadyCallback cb, gpointer data)
{ file_dialog_run (self, parent, QFileDialog::AcceptSave, QFileDialog::AnyFile, c, cb, data); }
void gtk_file_dialog_select_folder (GtkFileDialog *self, GtkWindow *parent, GCancellable *c, GAsyncReadyCallback cb, gpointer data)
{ file_dialog_run (self, parent, QFileDialog::AcceptOpen, QFileDialog::Directory, c, cb, data); }
GFile *gtk_file_dialog_open_finish (GtkFileDialog *, GAsyncResult *r, GError **e) { return (GFile *) g_task_propagate_pointer (G_TASK (r), e); }
GFile *gtk_file_dialog_save_finish (GtkFileDialog *, GAsyncResult *r, GError **e) { return (GFile *) g_task_propagate_pointer (G_TASK (r), e); }
GFile *gtk_file_dialog_select_folder_finish (GtkFileDialog *, GAsyncResult *r, GError **e) { return (GFile *) g_task_propagate_pointer (G_TASK (r), e); }

GtkAlertDialog *gtk_alert_dialog_new (const char *format, ...)
{
  auto *a = (GtkAlertDialog *) g_object_new (GTK_TYPE_ALERT_DIALOG, nullptr);
  va_list ap;
  va_start (ap, format);
  a->message = g_strdup_vprintf (format, ap);
  va_end (ap);
  return a;
}

void gtk_alert_dialog_set_detail (GtkAlertDialog *self, const char *detail) { g_free (self->detail); self->detail = g_strdup (detail); }
void gtk_alert_dialog_set_buttons (GtkAlertDialog *self, const char * const *labels) { g_strfreev (self->buttons); self->buttons = g_strdupv ((gchar **) labels); }
void gtk_alert_dialog_set_cancel_button (GtkAlertDialog *self, int button) { self->cancel = button; }
void gtk_alert_dialog_set_default_button (GtkAlertDialog *self, int button) { self->def = button; }
void gtk_alert_dialog_set_modal (GtkAlertDialog *, gboolean) {}
void gtk_alert_dialog_show (GtkAlertDialog *self, GtkWindow *parent) { alert_run (self, parent, nullptr); }

void gtk_alert_dialog_choose (GtkAlertDialog *self, GtkWindow *parent, GCancellable *c, GAsyncReadyCallback cb, gpointer data)
{
  alert_run (self, parent, g_task_new (self, c, cb, data));
}

int gtk_alert_dialog_choose_finish (GtkAlertDialog *, GAsyncResult *r, GError **e)
{
  gssize v = g_task_propagate_int (G_TASK (r), e);
  return (int) v;
}

} /* extern "C" */

/* ======================================================================
 * GtkColorButton
 * ====================================================================== */
static void color_button_paint (GtkWidget *w)
{
  auto *b = gtkqt_q<QPushButton> (w);
  auto *c = static_cast<QColor *> (g_object_get_data (G_OBJECT (w), "gtkqt-color"));
  if (!b || !c) return;
  QPixmap pm (32, 16);
  pm.fill (*c);
  b->setIcon (QIcon (pm));
  b->setIconSize (pm.size ());
}

static QObject *color_button_create (GtkWidget *w)
{
  auto *b = new QPushButton;
  gtkqt_connect (w, b, &QPushButton::clicked, [] (GtkWidget *cw, bool) {
    auto *c = static_cast<QColor *> (g_object_get_data (G_OBJECT (cw), "gtkqt-color"));
    QColor chosen = QColorDialog::getColor (*c, gtkqt_qw (cw), QString (), QColorDialog::ShowAlphaChannel);
    if (!chosen.isValid ()) return;
    *c = chosen;
    color_button_paint (cw);
    g_signal_emit_by_name (cw, "color-set");
    g_object_notify (G_OBJECT (cw), "rgba");
  });
  return b;
}

G_DEFINE_TYPE (GtkColorButton, gtk_color_button, GTK_TYPE_BUTTON)
static void gtk_color_button_init (GtkColorButton *b)
{
  g_object_set_data_full (G_OBJECT (b), "gtkqt-color", new QColor (Qt::black), [] (gpointer p) { delete static_cast<QColor *> (p); });
}
static void gtk_color_button_class_init (GtkColorButtonClass *klass)
{
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), color_button_create);
  g_signal_new ("color-set", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  gtkqt_prop (klass, g_param_spec_boxed ("rgba", nullptr, nullptr, GDK_TYPE_RGBA, RW),
              [] (GObject *o, const GValue *v) { if (auto *c = (GdkRGBA *) g_value_get_boxed (v)) gtk_color_chooser_set_rgba ((GtkColorChooser *) o, c); },
              [] (GObject *o, GValue *v) { GdkRGBA c; gtk_color_chooser_get_rgba ((GtkColorChooser *) o, &c); g_value_set_boxed (v, &c); });
}

extern "C" {
GtkWidget *gtk_color_button_new (void)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_COLOR_BUTTON, nullptr);
  color_button_paint (w);
  return w;
}

GtkWidget *gtk_color_button_new_with_rgba (const GdkRGBA *rgba)
{
  GtkWidget *w = gtk_color_button_new ();
  gtk_color_chooser_set_rgba ((GtkColorChooser *) w, rgba);
  return w;
}

void gtk_color_chooser_get_rgba (GtkColorChooser *chooser, GdkRGBA *color)
{
  auto *c = static_cast<QColor *> (g_object_get_data (G_OBJECT (chooser), "gtkqt-color"));
  if (!c || !color) return;
  color->red = c->redF (); color->green = c->greenF (); color->blue = c->blueF (); color->alpha = c->alphaF ();
}

void gtk_color_chooser_set_rgba (GtkColorChooser *chooser, const GdkRGBA *color)
{
  auto *c = static_cast<QColor *> (g_object_get_data (G_OBJECT (chooser), "gtkqt-color"));
  if (!c || !color) return;
  *c = QColor::fromRgbF (color->red, color->green, color->blue, color->alpha);
  color_button_paint ((GtkWidget *) chooser);
}
}
