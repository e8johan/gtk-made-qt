/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GtkApplication (a real GApplication), GtkApplicationWindow and GtkBuilder.
 */
#ifndef GTKQT_APPLICATION_H
#define GTKQT_APPLICATION_H

#include <gtk/gtkqt-containers.h>

G_BEGIN_DECLS

struct _GtkApplication { GApplication parent_instance; gpointer priv; };
typedef struct _GtkApplicationClass {
  GApplicationClass parent_class;
  void (*window_added)   (GtkApplication *application, GtkWindow *window);
  void (*window_removed) (GtkApplication *application, GtkWindow *window);
} GtkApplicationClass;
#define GTK_TYPE_APPLICATION  (gtk_application_get_type ())
#define GTK_APPLICATION(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_APPLICATION, GtkApplication))
#define GTK_IS_APPLICATION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_APPLICATION))
GType           gtk_application_get_type (void) G_GNUC_CONST;
GtkApplication *gtkqt_application_new (const gchar *application_id, GApplicationFlags flags, int api_version);
GtkApplication *gtk_application_new (const gchar *application_id, GApplicationFlags flags);
void            gtk_application_add_window (GtkApplication *application, GtkWindow *window);
void            gtk_application_remove_window (GtkApplication *application, GtkWindow *window);
GList          *gtk_application_get_windows (GtkApplication *application);
GtkWindow      *gtk_application_get_active_window (GtkApplication *application);
GtkWindow      *gtk_application_get_window_by_id (GtkApplication *application, guint id);
void            gtk_application_set_menubar (GtkApplication *application, GMenuModel *menubar);
GMenuModel     *gtk_application_get_menubar (GtkApplication *application);
void            gtk_application_set_app_menu (GtkApplication *application, GMenuModel *app_menu);
GMenuModel     *gtk_application_get_menu_by_id (GtkApplication *application, const gchar *id);
void            gtk_application_set_accels_for_action (GtkApplication *application, const gchar *detailed_action_name,
                                                       const gchar * const *accels);
void            gtk_application_add_accelerator (GtkApplication *application, const gchar *accelerator,
                                                 const gchar *action_name, GVariant *parameter);
guint           gtk_application_inhibit (GtkApplication *application, GtkWindow *window, int flags, const gchar *reason);
void            gtk_application_uninhibit (GtkApplication *application, guint cookie);

/* GtkApplicationWindow: a GtkWindow that is also a GActionMap ("win." actions) */
typedef struct _GtkApplicationWindow { GtkWindow parent_instance; } GtkApplicationWindow;
typedef struct _GtkApplicationWindowClass { GtkWindowClass parent_class; } GtkApplicationWindowClass;
#define GTK_TYPE_APPLICATION_WINDOW (gtk_application_window_get_type ())
#define GTK_APPLICATION_WINDOW(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_APPLICATION_WINDOW, GtkApplicationWindow))
#define GTK_IS_APPLICATION_WINDOW(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_APPLICATION_WINDOW))
GType      gtk_application_window_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_application_window_new (GtkApplication *application);
void       gtk_application_window_set_show_menubar (GtkApplicationWindow *window, gboolean show_menubar);
gboolean   gtk_application_window_get_show_menubar (GtkApplicationWindow *window);
guint      gtk_application_window_get_id (GtkApplicationWindow *window);

/* GtkBuilder: loads GtkBuilder/Glade XML (GTK 2.12+ / 3 / 4 formats). */
typedef struct _GtkBuilder GtkBuilder;
#define GTK_TYPE_BUILDER (gtk_builder_get_type ())
#define GTK_BUILDER(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_BUILDER, GtkBuilder))
#define GTK_IS_BUILDER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_BUILDER))
#define GTK_BUILDER_ERROR (gtk_builder_error_quark ())
typedef enum { GTK_BUILDER_ERROR_INVALID_TYPE_FUNCTION, GTK_BUILDER_ERROR_UNHANDLED_TAG,
               GTK_BUILDER_ERROR_MISSING_ATTRIBUTE, GTK_BUILDER_ERROR_INVALID_ATTRIBUTE,
               GTK_BUILDER_ERROR_INVALID_TAG, GTK_BUILDER_ERROR_MISSING_PROPERTY_VALUE,
               GTK_BUILDER_ERROR_INVALID_VALUE, GTK_BUILDER_ERROR_VERSION_MISMATCH,
               GTK_BUILDER_ERROR_DUPLICATE_ID } GtkBuilderError;
typedef void (*GtkBuilderConnectFunc) (GtkBuilder *builder, GObject *object, const gchar *signal_name,
                                       const gchar *handler_name, GObject *connect_object,
                                       GConnectFlags flags, gpointer user_data);
GType       gtk_builder_get_type (void) G_GNUC_CONST;
GQuark      gtk_builder_error_quark (void);
GtkBuilder *gtk_builder_new (void);
GtkBuilder *gtk_builder_new_from_file (const gchar *filename);
GtkBuilder *gtk_builder_new_from_string (const gchar *string, gssize length);
GtkBuilder *gtk_builder_new_from_resource (const gchar *resource_path);
guint       gtk_builder_add_from_file (GtkBuilder *builder, const gchar *filename, GError **error);
guint       gtk_builder_add_from_string (GtkBuilder *builder, const gchar *buffer, gsize length, GError **error);
guint       gtk_builder_add_from_resource (GtkBuilder *builder, const gchar *resource_path, GError **error);
GObject    *gtk_builder_get_object (GtkBuilder *builder, const gchar *name);
GSList     *gtk_builder_get_objects (GtkBuilder *builder);
void        gtk_builder_connect_signals (GtkBuilder *builder, gpointer user_data);
void        gtk_builder_connect_signals_full (GtkBuilder *builder, GtkBuilderConnectFunc func, gpointer user_data);
void        gtk_builder_set_translation_domain (GtkBuilder *builder, const gchar *domain);
void        gtk_builder_expose_object (GtkBuilder *builder, const gchar *name, GObject *object);
void        gtk_builder_set_current_object (GtkBuilder *builder, GObject *current_object);
GType       gtk_builder_get_type_from_name (GtkBuilder *builder, const char *type_name);

G_END_DECLS

#endif
