/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Dialogs: GtkDialog, message/about/file-chooser dialogs, GTK 4 async dialogs.
 */
#ifndef GTKQT_DIALOGS_H
#define GTKQT_DIALOGS_H

#include <gtk/gtkqt-containers.h>

G_BEGIN_DECLS

/* GtkDialog. vbox/action_area are the GTK 2 public fields. */
typedef struct _GtkDialog {
  GtkWindow window;
  GtkWidget *vbox;
  GtkWidget *action_area;
} GtkDialog;
typedef struct _GtkDialogClass {
  GtkWindowClass parent_class;
  void (*response) (GtkDialog *dialog, gint response_id);
  void (*close) (GtkDialog *dialog);
} GtkDialogClass;
#define GTK_TYPE_DIALOG  (gtk_dialog_get_type ())
#define GTK_DIALOG(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_DIALOG, GtkDialog))
#define GTK_IS_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_DIALOG))
GType      gtk_dialog_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_dialog_new (void);
GtkWidget *gtk_dialog_new_with_buttons (const gchar *title, GtkWindow *parent, GtkDialogFlags flags,
                                        const gchar *first_button_text, ...) G_GNUC_NULL_TERMINATED;
GtkWidget *gtk_dialog_add_button (GtkDialog *dialog, const gchar *button_text, gint response_id);
void       gtk_dialog_add_buttons (GtkDialog *dialog, const gchar *first_button_text, ...) G_GNUC_NULL_TERMINATED;
void       gtk_dialog_add_action_widget (GtkDialog *dialog, GtkWidget *child, gint response_id);
void       gtk_dialog_set_default_response (GtkDialog *dialog, gint response_id);
void       gtk_dialog_set_response_sensitive (GtkDialog *dialog, gint response_id, gboolean setting);
GtkWidget *gtk_dialog_get_widget_for_response (GtkDialog *dialog, gint response_id);
void       gtk_dialog_response (GtkDialog *dialog, gint response_id);
gint       gtk_dialog_run (GtkDialog *dialog);
GtkWidget *gtk_dialog_get_content_area (GtkDialog *dialog);
GtkWidget *gtk_dialog_get_action_area (GtkDialog *dialog);
void       gtk_dialog_set_has_separator (GtkDialog *dialog, gboolean setting);

/* GtkMessageDialog */
typedef struct _GtkMessageDialog { GtkDialog parent_instance; GtkWidget *image; GtkWidget *label; } GtkMessageDialog;
typedef struct _GtkMessageDialogClass { GtkDialogClass parent_class; } GtkMessageDialogClass;
#define GTK_TYPE_MESSAGE_DIALOG (gtk_message_dialog_get_type ())
#define GTK_MESSAGE_DIALOG(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_MESSAGE_DIALOG, GtkMessageDialog))
#define GTK_IS_MESSAGE_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_MESSAGE_DIALOG))
GType      gtk_message_dialog_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_message_dialog_new (GtkWindow *parent, GtkDialogFlags flags, GtkMessageType type,
                                   GtkButtonsType buttons, const gchar *message_format, ...) G_GNUC_PRINTF (5, 6);
GtkWidget *gtk_message_dialog_new_with_markup (GtkWindow *parent, GtkDialogFlags flags, GtkMessageType type,
                                               GtkButtonsType buttons, const gchar *message_format, ...) G_GNUC_PRINTF (5, 6);
void       gtk_message_dialog_set_markup (GtkMessageDialog *message_dialog, const gchar *str);
void       gtk_message_dialog_format_secondary_text (GtkMessageDialog *message_dialog, const gchar *message_format, ...) G_GNUC_PRINTF (2, 3);
void       gtk_message_dialog_format_secondary_markup (GtkMessageDialog *message_dialog, const gchar *message_format, ...) G_GNUC_PRINTF (2, 3);
GtkWidget *gtk_message_dialog_get_message_area (GtkMessageDialog *message_dialog);

/* GtkAboutDialog. All the settable fields are also GObject properties, so
 * gtk_show_about_dialog (parent, "program-name", "...", NULL) works. */
GTKQT_SIMPLE_TYPE (GtkAboutDialog, GtkDialog)
#define GTK_TYPE_ABOUT_DIALOG (gtk_about_dialog_get_type ())
#define GTK_ABOUT_DIALOG(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ABOUT_DIALOG, GtkAboutDialog))
#define GTK_IS_ABOUT_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ABOUT_DIALOG))
GType      gtk_about_dialog_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_about_dialog_new (void);
void       gtk_show_about_dialog (GtkWindow *parent, const gchar *first_property_name, ...) G_GNUC_NULL_TERMINATED;
void       gtk_about_dialog_set_program_name (GtkAboutDialog *about, const gchar *name);
void       gtk_about_dialog_set_name (GtkAboutDialog *about, const gchar *name);
void       gtk_about_dialog_set_version (GtkAboutDialog *about, const gchar *version);
void       gtk_about_dialog_set_copyright (GtkAboutDialog *about, const gchar *copyright);
void       gtk_about_dialog_set_comments (GtkAboutDialog *about, const gchar *comments);
void       gtk_about_dialog_set_license (GtkAboutDialog *about, const gchar *license);
void       gtk_about_dialog_set_license_type (GtkAboutDialog *about, GtkLicense license_type);
void       gtk_about_dialog_set_wrap_license (GtkAboutDialog *about, gboolean wrap_license);
void       gtk_about_dialog_set_website (GtkAboutDialog *about, const gchar *website);
void       gtk_about_dialog_set_website_label (GtkAboutDialog *about, const gchar *website_label);
void       gtk_about_dialog_set_authors (GtkAboutDialog *about, const gchar **authors);
void       gtk_about_dialog_set_documenters (GtkAboutDialog *about, const gchar **documenters);
void       gtk_about_dialog_set_artists (GtkAboutDialog *about, const gchar **artists);
void       gtk_about_dialog_set_translator_credits (GtkAboutDialog *about, const gchar *translator_credits);
void       gtk_about_dialog_set_logo_icon_name (GtkAboutDialog *about, const gchar *icon_name);
void       gtk_about_dialog_set_logo (GtkAboutDialog *about, GdkPixbuf *logo);
const gchar *gtk_about_dialog_get_program_name (GtkAboutDialog *about);
const gchar *gtk_about_dialog_get_version (GtkAboutDialog *about);

/* GtkFileFilter */
typedef struct _GtkFileFilter GtkFileFilter;
#define GTK_TYPE_FILE_FILTER (gtk_file_filter_get_type ())
#define GTK_FILE_FILTER(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FILE_FILTER, GtkFileFilter))
#define GTK_IS_FILE_FILTER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_FILE_FILTER))
GType          gtk_file_filter_get_type (void) G_GNUC_CONST;
GtkFileFilter *gtk_file_filter_new (void);
void           gtk_file_filter_set_name (GtkFileFilter *filter, const gchar *name);
const gchar   *gtk_file_filter_get_name (GtkFileFilter *filter);
void           gtk_file_filter_add_pattern (GtkFileFilter *filter, const gchar *pattern);
void           gtk_file_filter_add_suffix (GtkFileFilter *filter, const gchar *suffix);
void           gtk_file_filter_add_mime_type (GtkFileFilter *filter, const gchar *mime_type);
void           gtk_file_filter_add_pixbuf_formats (GtkFileFilter *filter);

/* GtkFileChooser: implemented by GtkFileChooserDialog and GtkFileChooserNative */
typedef struct _GtkFileChooser GtkFileChooser;
#define GTK_FILE_CHOOSER(o) ((GtkFileChooser *) (o))
GTKQT_SIMPLE_TYPE (GtkFileChooserDialog, GtkDialog)
#define GTK_TYPE_FILE_CHOOSER_DIALOG (gtk_file_chooser_dialog_get_type ())
#define GTK_FILE_CHOOSER_DIALOG(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FILE_CHOOSER_DIALOG, GtkFileChooserDialog))
#define GTK_IS_FILE_CHOOSER_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_FILE_CHOOSER_DIALOG))
GType      gtk_file_chooser_dialog_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_file_chooser_dialog_new (const gchar *title, GtkWindow *parent, GtkFileChooserAction action,
                                        const gchar *first_button_text, ...) G_GNUC_NULL_TERMINATED;
void       gtk_file_chooser_set_action (GtkFileChooser *chooser, GtkFileChooserAction action);
void       gtk_file_chooser_set_select_multiple (GtkFileChooser *chooser, gboolean select_multiple);
void       gtk_file_chooser_set_do_overwrite_confirmation (GtkFileChooser *chooser, gboolean do_overwrite_confirmation);
void       gtk_file_chooser_set_current_name (GtkFileChooser *chooser, const gchar *name);
gboolean   gtk_file_chooser_set_filename (GtkFileChooser *chooser, const char *filename);
gboolean   gtk_file_chooser_set_current_folder (GtkFileChooser *chooser, const gchar *filename);  /* GTK 4: GFile*, see compat */
gboolean   gtk_file_chooser_set_file (GtkFileChooser *chooser, GFile *file, GError **error);
gchar     *gtk_file_chooser_get_filename (GtkFileChooser *chooser);
GSList    *gtk_file_chooser_get_filenames (GtkFileChooser *chooser);
gchar     *gtk_file_chooser_get_uri (GtkFileChooser *chooser);
gchar     *gtk_file_chooser_get_current_folder (GtkFileChooser *chooser);
GFile     *gtk_file_chooser_get_file (GtkFileChooser *chooser);
GListModel *gtk_file_chooser_get_files (GtkFileChooser *chooser);
void       gtk_file_chooser_add_filter (GtkFileChooser *chooser, GtkFileFilter *filter);
void       gtk_file_chooser_set_filter (GtkFileChooser *chooser, GtkFileFilter *filter);
gboolean   gtk_file_chooser_set_current_folder_file (GtkFileChooser *chooser, GFile *file, GError **error);

/* GtkNativeDialog / GtkFileChooserNative (GTK 3.20+, 4) */
typedef struct _GtkNativeDialog { GObject parent_instance; gpointer priv; } GtkNativeDialog;
typedef struct _GtkNativeDialogClass { GObjectClass parent_class; } GtkNativeDialogClass;
typedef GtkNativeDialog GtkFileChooserNative;
#define GTK_TYPE_NATIVE_DIALOG (gtk_native_dialog_get_type ())
#define GTK_NATIVE_DIALOG(o)   ((GtkNativeDialog *) (o))
GType      gtk_native_dialog_get_type (void) G_GNUC_CONST;
GtkFileChooserNative *gtk_file_chooser_native_new (const gchar *title, GtkWindow *parent, GtkFileChooserAction action,
                                                   const gchar *accept_label, const gchar *cancel_label);
void       gtk_native_dialog_show (GtkNativeDialog *self);
void       gtk_native_dialog_hide (GtkNativeDialog *self);
void       gtk_native_dialog_destroy (GtkNativeDialog *self);
gint       gtk_native_dialog_run (GtkNativeDialog *self);
void       gtk_native_dialog_set_modal (GtkNativeDialog *self, gboolean modal);

/* GtkFileDialog / GtkAlertDialog (GTK 4.10+), asynchronous */
typedef struct _GtkFileDialog GtkFileDialog;
typedef struct _GtkAlertDialog GtkAlertDialog;
#define GTK_TYPE_FILE_DIALOG  (gtk_file_dialog_get_type ())
#define GTK_FILE_DIALOG(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FILE_DIALOG, GtkFileDialog))
#define GTK_IS_FILE_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_FILE_DIALOG))
#define GTK_TYPE_ALERT_DIALOG (gtk_alert_dialog_get_type ())
#define GTK_ALERT_DIALOG(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ALERT_DIALOG, GtkAlertDialog))
#define GTK_IS_ALERT_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ALERT_DIALOG))
GType          gtk_file_dialog_get_type (void) G_GNUC_CONST;
GType          gtk_alert_dialog_get_type (void) G_GNUC_CONST;
GtkFileDialog *gtk_file_dialog_new (void);
void           gtk_file_dialog_set_title (GtkFileDialog *self, const char *title);
void           gtk_file_dialog_set_initial_name (GtkFileDialog *self, const char *name);
void           gtk_file_dialog_set_initial_folder (GtkFileDialog *self, GFile *folder);
void           gtk_file_dialog_set_filters (GtkFileDialog *self, GListModel *filters);
void           gtk_file_dialog_set_default_filter (GtkFileDialog *self, GtkFileFilter *filter);
void           gtk_file_dialog_open (GtkFileDialog *self, GtkWindow *parent, GCancellable *cancellable,
                                     GAsyncReadyCallback callback, gpointer user_data);
GFile         *gtk_file_dialog_open_finish (GtkFileDialog *self, GAsyncResult *result, GError **error);
void           gtk_file_dialog_save (GtkFileDialog *self, GtkWindow *parent, GCancellable *cancellable,
                                     GAsyncReadyCallback callback, gpointer user_data);
GFile         *gtk_file_dialog_save_finish (GtkFileDialog *self, GAsyncResult *result, GError **error);
void           gtk_file_dialog_select_folder (GtkFileDialog *self, GtkWindow *parent, GCancellable *cancellable,
                                              GAsyncReadyCallback callback, gpointer user_data);
GFile         *gtk_file_dialog_select_folder_finish (GtkFileDialog *self, GAsyncResult *result, GError **error);
GtkAlertDialog *gtk_alert_dialog_new (const char *format, ...) G_GNUC_PRINTF (1, 2);
void           gtk_alert_dialog_set_detail (GtkAlertDialog *self, const char *detail);
void           gtk_alert_dialog_set_buttons (GtkAlertDialog *self, const char * const *labels);
void           gtk_alert_dialog_set_cancel_button (GtkAlertDialog *self, int button);
void           gtk_alert_dialog_set_default_button (GtkAlertDialog *self, int button);
void           gtk_alert_dialog_set_modal (GtkAlertDialog *self, gboolean modal);
void           gtk_alert_dialog_show (GtkAlertDialog *self, GtkWindow *parent);
void           gtk_alert_dialog_choose (GtkAlertDialog *self, GtkWindow *parent, GCancellable *cancellable,
                                        GAsyncReadyCallback callback, gpointer user_data);
int            gtk_alert_dialog_choose_finish (GtkAlertDialog *self, GAsyncResult *result, GError **error);

/* Colour chooser (dialog + button) */
GTKQT_SIMPLE_TYPE (GtkColorButton, GtkButton)
#define GTK_TYPE_COLOR_BUTTON (gtk_color_button_get_type ())
#define GTK_COLOR_BUTTON(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_COLOR_BUTTON, GtkColorButton))
#define GTK_IS_COLOR_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_COLOR_BUTTON))
typedef struct _GtkColorChooser GtkColorChooser;
#define GTK_COLOR_CHOOSER(o)  ((GtkColorChooser *) (o))
GType      gtk_color_button_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_color_button_new (void);
GtkWidget *gtk_color_button_new_with_rgba (const GdkRGBA *rgba);
void       gtk_color_chooser_get_rgba (GtkColorChooser *chooser, GdkRGBA *color);
void       gtk_color_chooser_set_rgba (GtkColorChooser *chooser, const GdkRGBA *color);

/* Stock item ids (GTK 2/3) */
#define GTK_STOCK_ABOUT       "gtk-about"
#define GTK_STOCK_ADD         "gtk-add"
#define GTK_STOCK_APPLY       "gtk-apply"
#define GTK_STOCK_BOLD        "gtk-bold"
#define GTK_STOCK_CANCEL      "gtk-cancel"
#define GTK_STOCK_CLEAR       "gtk-clear"
#define GTK_STOCK_CLOSE       "gtk-close"
#define GTK_STOCK_COPY        "gtk-copy"
#define GTK_STOCK_CUT         "gtk-cut"
#define GTK_STOCK_DELETE      "gtk-delete"
#define GTK_STOCK_DIALOG_ERROR    "gtk-dialog-error"
#define GTK_STOCK_DIALOG_INFO     "gtk-dialog-info"
#define GTK_STOCK_DIALOG_QUESTION "gtk-dialog-question"
#define GTK_STOCK_DIALOG_WARNING  "gtk-dialog-warning"
#define GTK_STOCK_EDIT        "gtk-edit"
#define GTK_STOCK_EXECUTE     "gtk-execute"
#define GTK_STOCK_FIND        "gtk-find"
#define GTK_STOCK_FIND_AND_REPLACE "gtk-find-and-replace"
#define GTK_STOCK_GO_BACK     "gtk-go-back"
#define GTK_STOCK_GO_DOWN     "gtk-go-down"
#define GTK_STOCK_GO_FORWARD  "gtk-go-forward"
#define GTK_STOCK_GO_UP       "gtk-go-up"
#define GTK_STOCK_HELP        "gtk-help"
#define GTK_STOCK_HOME        "gtk-home"
#define GTK_STOCK_INFO        "gtk-info"
#define GTK_STOCK_ITALIC      "gtk-italic"
#define GTK_STOCK_NEW         "gtk-new"
#define GTK_STOCK_NO          "gtk-no"
#define GTK_STOCK_OK          "gtk-ok"
#define GTK_STOCK_OPEN        "gtk-open"
#define GTK_STOCK_PASTE       "gtk-paste"
#define GTK_STOCK_PREFERENCES "gtk-preferences"
#define GTK_STOCK_PRINT       "gtk-print"
#define GTK_STOCK_PROPERTIES  "gtk-properties"
#define GTK_STOCK_QUIT        "gtk-quit"
#define GTK_STOCK_REDO        "gtk-redo"
#define GTK_STOCK_REFRESH     "gtk-refresh"
#define GTK_STOCK_REMOVE      "gtk-remove"
#define GTK_STOCK_SAVE        "gtk-save"
#define GTK_STOCK_SAVE_AS     "gtk-save-as"
#define GTK_STOCK_SELECT_ALL  "gtk-select-all"
#define GTK_STOCK_STOP        "gtk-stop"
#define GTK_STOCK_UNDERLINE   "gtk-underline"
#define GTK_STOCK_UNDO        "gtk-undo"
#define GTK_STOCK_YES         "gtk-yes"
#define GTK_STOCK_ZOOM_IN     "gtk-zoom-in"
#define GTK_STOCK_ZOOM_OUT    "gtk-zoom-out"
#define GTK_STOCK_PRINT_PREVIEW      "gtk-print-preview"
#define GTK_STOCK_JUMP_TO            "gtk-jump-to"
#define GTK_STOCK_SELECT_FONT        "gtk-select-font"
#define GTK_STOCK_PAGE_SETUP         "gtk-page-setup"
#define GTK_STOCK_REVERT_TO_SAVED    "gtk-revert-to-saved"
#define GTK_STOCK_GOTO_FIRST         "gtk-goto-first"
#define GTK_STOCK_GOTO_LAST          "gtk-goto-last"
#define GTK_STOCK_GOTO_TOP           "gtk-goto-top"
#define GTK_STOCK_GOTO_BOTTOM        "gtk-goto-bottom"
#define GTK_STOCK_INDEX              "gtk-index"
#define GTK_STOCK_INDENT             "gtk-indent"
#define GTK_STOCK_UNINDENT           "gtk-unindent"
#define GTK_STOCK_CONVERT            "gtk-convert"
#define GTK_STOCK_SPELL_CHECK        "gtk-spell-check"
#define GTK_STOCK_FULLSCREEN         "gtk-fullscreen"
#define GTK_STOCK_LEAVE_FULLSCREEN   "gtk-leave-fullscreen"
#define GTK_STOCK_MEDIA_PLAY         "gtk-media-play"
#define GTK_STOCK_MEDIA_PAUSE        "gtk-media-pause"
#define GTK_STOCK_MEDIA_STOP         "gtk-media-stop"
#define GTK_STOCK_MEDIA_NEXT         "gtk-media-next"
#define GTK_STOCK_MEDIA_PREVIOUS     "gtk-media-previous"
#define GTK_STOCK_DIRECTORY          "gtk-directory"
#define GTK_STOCK_FILE               "gtk-file"
#define GTK_STOCK_HARDDISK           "gtk-harddisk"
#define GTK_STOCK_NETWORK            "gtk-network"
#define GTK_STOCK_COLOR_PICKER       "gtk-color-picker"
#define GTK_STOCK_SORT_ASCENDING     "gtk-sort-ascending"
#define GTK_STOCK_SORT_DESCENDING    "gtk-sort-descending"
#define GTK_STOCK_STRIKETHROUGH      "gtk-strikethrough"
#define GTK_STOCK_JUSTIFY_LEFT       "gtk-justify-left"
#define GTK_STOCK_JUSTIFY_RIGHT      "gtk-justify-right"
#define GTK_STOCK_JUSTIFY_CENTER     "gtk-justify-center"
#define GTK_STOCK_JUSTIFY_FILL       "gtk-justify-fill"
#define GTK_STOCK_MISSING_IMAGE      "gtk-missing-image"
#define GTK_STOCK_DISCARD            "gtk-discard"
#define GTK_STOCK_CONNECT            "gtk-connect"
#define GTK_STOCK_DISCONNECT         "gtk-disconnect"
#define GTK_STOCK_DND                "gtk-dnd"
#define GTK_STOCK_CAPS_LOCK_WARNING  "gtk-caps-lock-warning"
#define GTK_STOCK_ZOOM_100           "gtk-zoom-100"
#define GTK_STOCK_ZOOM_FIT           "gtk-zoom-fit"
#define GTK_STOCK_FLOPPY             "gtk-floppy"
#define GTK_STOCK_CDROM              "gtk-cdrom"
#define GTK_STOCK_SELECT_COLOR       "gtk-select-color"
#define GTK_STOCK_DIALOG_AUTHENTICATION "gtk-dialog-authentication"
#define GTK_STOCK_UNDELETE           "gtk-undelete"
#define GTK_STOCK_MEDIA_RECORD       "gtk-media-record"
#define GTK_STOCK_MEDIA_FORWARD      "gtk-media-forward"
#define GTK_STOCK_MEDIA_REWIND       "gtk-media-rewind"
#define GTK_STOCK_ORIENTATION_PORTRAIT "gtk-orientation-portrait"
#define GTK_STOCK_ORIENTATION_LANDSCAPE "gtk-orientation-landscape"

G_END_DECLS

#endif
