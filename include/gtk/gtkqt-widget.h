/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Core: GtkWidget, GtkContainer, GtkBin, GtkWindow, main loop.
 */
#ifndef GTKQT_WIDGET_H
#define GTKQT_WIDGET_H

#include <gdk/gdk.h>
#include <gtk/gtkqt-enums.h>

G_BEGIN_DECLS

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWidgetClass GtkWidgetClass;
typedef struct _GtkWidgetPrivate GtkWidgetPrivate;
typedef struct _GtkContainer GtkContainer;
typedef struct _GtkContainerClass GtkContainerClass;
typedef struct _GtkBin GtkBin;
typedef struct _GtkBinClass GtkBinClass;
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkWindowClass GtkWindowClass;
typedef struct _GtkAdjustment GtkAdjustment;
typedef struct _GtkApplication GtkApplication;
typedef struct _GtkAccelGroup GtkAccelGroup;
typedef struct _GtkStyle GtkStyle;
typedef struct _GtkStyleContext GtkStyleContext;
typedef struct _GtkCssProvider GtkCssProvider;
typedef struct _GtkEventController GtkEventController;
typedef struct _GtkTooltip GtkTooltip;

typedef GdkRectangle GtkAllocation;
typedef struct { gint width, height; } GtkRequisition;
typedef struct { gint16 left, right, top, bottom; } GtkBorder;

/* GtkObject (GTK 1/2) is the floating-reference base class: that is
 * GInitiallyUnowned in GTK 3+, and it is here too. */
#if GTKQT_API_VERSION <= 2 && !defined (GTKQT_COMPILATION)
/* GTK 1/2 programs reach the class through object->klass and read
 * klass->type, and set object_class->destroy: the same memory as GObject's
 * g_class / g_type, and GtkWidgetClass's destroy slot. */
struct _GtkObject;
typedef struct _GtkObjectClass {
  union { GInitiallyUnownedClass parent_class; GType type; };
  void (*destroy) (struct _GtkObject *object);
} GtkObjectClass;
typedef struct _GtkObject {
  union { GInitiallyUnowned parent_instance; GtkObjectClass *klass; };
} GtkObject;
#else
typedef GInitiallyUnowned GtkObject;
typedef GInitiallyUnownedClass GtkObjectClass;
#endif
#define GTK_TYPE_OBJECT       G_TYPE_INITIALLY_UNOWNED
#define GTK_OBJECT(obj)       ((GtkObject *) (obj))
#define GTK_IS_OBJECT(obj)    G_IS_INITIALLY_UNOWNED (obj)
#define GTK_OBJECT_TYPE(obj)  G_OBJECT_TYPE (obj)

typedef void (*GtkCallback) (GtkWidget *widget, gpointer data);

#define GTK_TYPE_WIDGET            (gtk_widget_get_type ())
#define GTK_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_WIDGET, GtkWidget))
#define GTK_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_WIDGET, GtkWidgetClass))
#define GTK_IS_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_WIDGET))
#define GTK_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_WIDGET, GtkWidgetClass))

struct _GtkWidget {
  GInitiallyUnowned parent_instance;
  /* GTK 2 public fields. They are kept current so old code can read them. */
  guint32 flags;
  GtkStyle *style;
  GtkRequisition requisition;
  GtkAllocation allocation;
  GdkWindow *window;
  GtkWidget *parent;
  guint8 state;               /* GtkStateType */
  guint8 saved_state;
  gchar *name;
  /*< private >*/
  GtkWidgetPrivate *priv;
};

struct _GtkWidgetClass {
  GInitiallyUnownedClass parent_class;

  /* Class closures of the signals of the same name. Subclasses may override. */
  void     (*destroy)              (GtkWidget *widget);
  void     (*show)                 (GtkWidget *widget);
  void     (*hide)                 (GtkWidget *widget);
  void     (*map)                  (GtkWidget *widget);
  void     (*unmap)                (GtkWidget *widget);
  void     (*realize)              (GtkWidget *widget);
  void     (*unrealize)            (GtkWidget *widget);
  void     (*size_allocate)        (GtkWidget *widget, GtkAllocation *allocation);
  void     (*size_request)         (GtkWidget *widget, GtkRequisition *requisition);   /* GTK 1/2 */
  gboolean (*draw)                 (GtkWidget *widget, cairo_t *cr);
  gboolean (*event)                (GtkWidget *widget, GdkEvent *event);
  gboolean (*button_press_event)   (GtkWidget *widget, GdkEventButton *event);
  gboolean (*button_release_event) (GtkWidget *widget, GdkEventButton *event);
  gboolean (*scroll_event)         (GtkWidget *widget, GdkEventScroll *event);
  gboolean (*motion_notify_event)  (GtkWidget *widget, GdkEventMotion *event);
  gboolean (*delete_event)         (GtkWidget *widget, GdkEventAny *event);
  gboolean (*key_press_event)      (GtkWidget *widget, GdkEventKey *event);
  gboolean (*key_release_event)    (GtkWidget *widget, GdkEventKey *event);
  gboolean (*enter_notify_event)   (GtkWidget *widget, GdkEventCrossing *event);
  gboolean (*leave_notify_event)   (GtkWidget *widget, GdkEventCrossing *event);
  gboolean (*configure_event)      (GtkWidget *widget, GdkEventConfigure *event);
  gboolean (*focus_in_event)       (GtkWidget *widget, GdkEventFocus *event);
  gboolean (*focus_out_event)      (GtkWidget *widget, GdkEventFocus *event);
  gboolean (*expose_event)         (GtkWidget *widget, GdkEventExpose *event);

  /*< private >*/
  gpointer gtkqt_create;      /* QObject *(*)(GtkWidget *): builds the Qt side */
  gpointer gtkqt_reserved[8];
};

#define GTK_TYPE_CONTAINER            (gtk_container_get_type ())
#define GTK_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CONTAINER, GtkContainer))
#define GTK_IS_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CONTAINER))
#define GTK_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CONTAINER, GtkContainerClass))
#define GTK_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CONTAINER, GtkContainerClass))

struct _GtkContainer {
  GtkWidget widget;
  guint border_width;
};

struct _GtkContainerClass {
  GtkWidgetClass parent_class;
  void (*add)    (GtkContainer *container, GtkWidget *widget);
  void (*remove) (GtkContainer *container, GtkWidget *widget);
  void (*forall) (GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer data);
  /*< private >*/
  gpointer gtkqt_place;      /* void (*)(GtkContainer *, GtkWidget *child): put child's QWidget in place */
  gpointer gtkqt_unplace;    /* void (*)(GtkContainer *, GtkWidget *child) */
  gpointer gtkqt_reserved[6];
};

#define GTK_TYPE_BIN          (gtk_bin_get_type ())
#define GTK_BIN(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_BIN, GtkBin))
#define GTK_IS_BIN(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_BIN))
struct _GtkBin { GtkContainer container; GtkWidget *child; };
struct _GtkBinClass { GtkContainerClass parent_class; };

#define GTK_TYPE_WINDOW          (gtk_window_get_type ())
#define GTK_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_WINDOW, GtkWindow))
#define GTK_IS_WINDOW(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_WINDOW))
#define GTK_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_WINDOW, GtkWindowClass))
struct _GtkWindow { GtkBin bin; gchar *title; };
struct _GtkWindowClass {
  GtkBinClass parent_class;
  gboolean (*close_request) (GtkWindow *window);
  void     (*activate_default) (GtkWindow *window);
};

GType gtk_widget_get_type (void) G_GNUC_CONST;
GType gtk_container_get_type (void) G_GNUC_CONST;
GType gtk_bin_get_type (void) G_GNUC_CONST;
GType gtk_window_get_type (void) G_GNUC_CONST;

/* ---- initialisation and main loop ---- */
void     gtkqt_init (int *argc, char ***argv, int api_version);
gboolean gtkqt_init_check (int *argc, char ***argv, int api_version);
void     gtk_init (int *argc, char ***argv);       /* GTK 2/3 signature; see gtkqt-compat.h */
gboolean gtk_init_check (int *argc, char ***argv);
gboolean gtk_is_initialized (void);
void     gtk_main (void);
void     gtk_main_quit (void);
guint    gtk_main_level (void);
gboolean gtk_main_iteration (void);
gboolean gtk_main_iteration_do (gboolean blocking);
gboolean gtk_events_pending (void);
gboolean gtk_true (void);
gboolean gtk_false (void);
guint    gtk_get_major_version (void);
guint    gtk_get_minor_version (void);
guint    gtk_get_micro_version (void);
const gchar *gtk_check_version (guint required_major, guint required_minor, guint required_micro);
void     gtk_disable_setlocale (void);

/* ---- GtkWidget ---- */
void       gtk_widget_destroy (GtkWidget *widget);
void       gtk_widget_destroyed (GtkWidget *widget, GtkWidget **widget_pointer);
AtkObject *gtk_widget_get_accessible (GtkWidget *widget);   /* see atk/atk.h */
void       gtk_widget_unparent (GtkWidget *widget);
void       gtk_widget_show (GtkWidget *widget);
void       gtk_widget_show_now (GtkWidget *widget);
void       gtk_widget_show_all (GtkWidget *widget);
void       gtk_widget_hide (GtkWidget *widget);
void       gtk_widget_hide_all (GtkWidget *widget);
gboolean   gtk_widget_hide_on_delete (GtkWidget *widget);
void       gtk_widget_set_visible (GtkWidget *widget, gboolean visible);
gboolean   gtk_widget_get_visible (GtkWidget *widget);
gboolean   gtk_widget_is_visible (GtkWidget *widget);
void       gtk_widget_set_no_show_all (GtkWidget *widget, gboolean no_show_all);
gboolean   gtk_widget_get_mapped (GtkWidget *widget);
gboolean   gtk_widget_get_realized (GtkWidget *widget);
void       gtk_widget_realize (GtkWidget *widget);
void       gtk_widget_map (GtkWidget *widget);
void       gtk_widget_set_sensitive (GtkWidget *widget, gboolean sensitive);
gboolean   gtk_widget_get_sensitive (GtkWidget *widget);
gboolean   gtk_widget_is_sensitive (GtkWidget *widget);
void       gtk_widget_set_size_request (GtkWidget *widget, gint width, gint height);
void       gtk_widget_get_size_request (GtkWidget *widget, gint *width, gint *height);
void       gtk_widget_set_usize (GtkWidget *widget, gint width, gint height);
void       gtk_widget_size_request (GtkWidget *widget, GtkRequisition *requisition);
void       gtk_widget_get_preferred_size (GtkWidget *widget, GtkRequisition *minimum, GtkRequisition *natural);
void       gtk_widget_get_allocation (GtkWidget *widget, GtkAllocation *allocation);
int        gtk_widget_get_allocated_width (GtkWidget *widget);
int        gtk_widget_get_allocated_height (GtkWidget *widget);
int        gtk_widget_get_width (GtkWidget *widget);
int        gtk_widget_get_height (GtkWidget *widget);
int        gtk_widget_get_scale_factor (GtkWidget *widget);
void       gtk_widget_set_tooltip_text (GtkWidget *widget, const gchar *text);
gchar     *gtk_widget_get_tooltip_text (GtkWidget *widget);
void       gtk_widget_set_tooltip_markup (GtkWidget *widget, const gchar *markup);
void       gtk_widget_set_has_tooltip (GtkWidget *widget, gboolean has_tooltip);
void       gtk_widget_set_name (GtkWidget *widget, const gchar *name);
const gchar *gtk_widget_get_name (GtkWidget *widget);
GtkWidget *gtk_widget_get_parent (GtkWidget *widget);
void       gtk_widget_set_parent (GtkWidget *widget, GtkWidget *parent);
GtkWidget *gtk_widget_get_toplevel (GtkWidget *widget);
GtkWidget *gtk_widget_get_ancestor (GtkWidget *widget, GType widget_type);
gboolean   gtk_widget_is_toplevel (GtkWidget *widget);
gboolean   gtk_widget_is_ancestor (GtkWidget *widget, GtkWidget *ancestor);
GtkWidget *gtk_widget_get_root (GtkWidget *widget);
GtkWidget *gtk_widget_get_first_child (GtkWidget *widget);
GtkWidget *gtk_widget_get_last_child (GtkWidget *widget);
GtkWidget *gtk_widget_get_next_sibling (GtkWidget *widget);
GtkWidget *gtk_widget_get_prev_sibling (GtkWidget *widget);
GdkWindow *gtk_widget_get_window (GtkWidget *widget);
GdkWindow *gtk_widget_get_parent_window (GtkWidget *widget);
GdkDisplay *gtk_widget_get_display (GtkWidget *widget);
GdkScreen *gtk_widget_get_screen (GtkWidget *widget);
void       gtk_widget_queue_draw (GtkWidget *widget);
void       gtk_widget_queue_draw_area (GtkWidget *widget, gint x, gint y, gint width, gint height);
void       gtk_widget_queue_resize (GtkWidget *widget);
void       gtk_widget_queue_allocate (GtkWidget *widget);
void       gtk_widget_set_can_focus (GtkWidget *widget, gboolean can_focus);
gboolean   gtk_widget_get_can_focus (GtkWidget *widget);
void       gtk_widget_set_focusable (GtkWidget *widget, gboolean focusable);
gboolean   gtk_widget_get_focusable (GtkWidget *widget);
void       gtk_widget_set_focus_on_click (GtkWidget *widget, gboolean focus_on_click);
gboolean   gtk_widget_has_focus (GtkWidget *widget);
gboolean   gtk_widget_is_focus (GtkWidget *widget);
void       gtk_widget_grab_focus (GtkWidget *widget);
void       gtk_widget_set_can_default (GtkWidget *widget, gboolean can_default);
void       gtk_widget_grab_default (GtkWidget *widget);
void       gtk_widget_set_receives_default (GtkWidget *widget, gboolean receives_default);
void       gtk_widget_set_events (GtkWidget *widget, gint events);
void       gtk_widget_add_events (GtkWidget *widget, gint events);
gint       gtk_widget_get_events (GtkWidget *widget);
void       gtk_widget_set_app_paintable (GtkWidget *widget, gboolean app_paintable);
void       gtk_widget_set_double_buffered (GtkWidget *widget, gboolean double_buffered);
void       gtk_widget_set_has_window (GtkWidget *widget, gboolean has_window);
gboolean   gtk_widget_get_has_window (GtkWidget *widget);
void       gtk_widget_set_hexpand (GtkWidget *widget, gboolean expand);
gboolean   gtk_widget_get_hexpand (GtkWidget *widget);
void       gtk_widget_set_vexpand (GtkWidget *widget, gboolean expand);
gboolean   gtk_widget_get_vexpand (GtkWidget *widget);
void       gtk_widget_set_hexpand_set (GtkWidget *widget, gboolean set);
void       gtk_widget_set_vexpand_set (GtkWidget *widget, gboolean set);
void       gtk_widget_set_halign (GtkWidget *widget, GtkAlign align);
GtkAlign   gtk_widget_get_halign (GtkWidget *widget);
void       gtk_widget_set_valign (GtkWidget *widget, GtkAlign align);
GtkAlign   gtk_widget_get_valign (GtkWidget *widget);
void       gtk_widget_set_margin_start (GtkWidget *widget, gint margin);
void       gtk_widget_set_margin_end (GtkWidget *widget, gint margin);
void       gtk_widget_set_margin_top (GtkWidget *widget, gint margin);
void       gtk_widget_set_margin_bottom (GtkWidget *widget, gint margin);
void       gtk_widget_set_margin_left (GtkWidget *widget, gint margin);
void       gtk_widget_set_margin_right (GtkWidget *widget, gint margin);
gint       gtk_widget_get_margin_start (GtkWidget *widget);
gint       gtk_widget_get_margin_end (GtkWidget *widget);
gint       gtk_widget_get_margin_top (GtkWidget *widget);
gint       gtk_widget_get_margin_bottom (GtkWidget *widget);
void       gtk_widget_set_opacity (GtkWidget *widget, double opacity);
void       gtk_widget_set_cursor_from_name (GtkWidget *widget, const char *name);
gboolean   gtk_widget_activate (GtkWidget *widget);
gboolean   gtk_widget_mnemonic_activate (GtkWidget *widget, gboolean group_cycling);
void       gtk_widget_add_accelerator (GtkWidget *widget, const gchar *accel_signal, GtkAccelGroup *accel_group,
                                       guint accel_key, GdkModifierType accel_mods, guint accel_flags);
gboolean   gtk_widget_translate_coordinates (GtkWidget *src_widget, GtkWidget *dest_widget,
                                             gint src_x, gint src_y, gint *dest_x, gint *dest_y);
void       gtk_widget_insert_action_group (GtkWidget *widget, const gchar *name, GActionGroup *group);
gboolean   gtk_widget_activate_action (GtkWidget *widget, const char *name, const char *format_string, ...);
gboolean   gtk_widget_activate_action_variant (GtkWidget *widget, const char *name, GVariant *args);
void       gtk_widget_add_controller (GtkWidget *widget, GtkEventController *controller);
void       gtk_widget_remove_controller (GtkWidget *widget, GtkEventController *controller);

/* Styling. CSS is not interpreted; style classes are recorded and a few
 * common ones ("suggested-action", "destructive-action", "title",
 * "dim-label", "monospace") get a rough Qt equivalent. */
void       gtk_widget_add_css_class (GtkWidget *widget, const char *css_class);
void       gtk_widget_remove_css_class (GtkWidget *widget, const char *css_class);
gboolean   gtk_widget_has_css_class (GtkWidget *widget, const char *css_class);
void       gtk_widget_set_css_classes (GtkWidget *widget, const char **classes);
GtkStyleContext *gtk_widget_get_style_context (GtkWidget *widget);
void       gtk_style_context_add_class (GtkStyleContext *context, const gchar *class_name);
void       gtk_style_context_remove_class (GtkStyleContext *context, const gchar *class_name);
gboolean   gtk_style_context_has_class (GtkStyleContext *context, const gchar *class_name);
void       gtk_style_context_add_provider (GtkStyleContext *context, gpointer provider, guint priority);
void       gtk_style_context_add_provider_for_screen (GdkScreen *screen, gpointer provider, guint priority);
void       gtk_style_context_add_provider_for_display (GdkDisplay *display, gpointer provider, guint priority);
GtkCssProvider *gtk_css_provider_new (void);
gboolean   gtk_css_provider_load_from_data (GtkCssProvider *provider, const gchar *data, gssize length, GError **error);
void       gtk_css_provider_load_from_string (GtkCssProvider *provider, const char *string);
void       gtk_css_provider_load_from_path (GtkCssProvider *provider, const char *path);
void       gtk_css_provider_load_from_resource (GtkCssProvider *provider, const char *resource_path);
#define GTK_STYLE_PROVIDER(o) ((gpointer) (o))
#define GTK_STYLE_PROVIDER_PRIORITY_FALLBACK     1
#define GTK_STYLE_PROVIDER_PRIORITY_THEME      200
#define GTK_STYLE_PROVIDER_PRIORITY_SETTINGS   400
#define GTK_STYLE_PROVIDER_PRIORITY_APPLICATION 600
#define GTK_STYLE_PROVIDER_PRIORITY_USER       800

/* GTK 2 flag macros */
#define GTK_WIDGET_FLAGS(wid)             (GTK_WIDGET (wid)->flags)
#define GTK_WIDGET_SET_FLAGS(wid, flag)   gtkqt_widget_set_flags (GTK_WIDGET (wid), (flag), TRUE)
#define GTK_WIDGET_UNSET_FLAGS(wid, flag) gtkqt_widget_set_flags (GTK_WIDGET (wid), (flag), FALSE)
#define GTK_WIDGET_VISIBLE(wid)           (gtk_widget_get_visible (GTK_WIDGET (wid)))
#define GTK_WIDGET_SENSITIVE(wid)         (gtk_widget_get_sensitive (GTK_WIDGET (wid)))
#define GTK_WIDGET_IS_SENSITIVE(wid)      (gtk_widget_is_sensitive (GTK_WIDGET (wid)))
#define GTK_WIDGET_HAS_FOCUS(wid)         (gtk_widget_has_focus (GTK_WIDGET (wid)))
#define GTK_WIDGET_MAPPED(wid)            (gtk_widget_get_mapped (GTK_WIDGET (wid)))
#define GTK_WIDGET_REALIZED(wid)          (gtk_widget_get_realized (GTK_WIDGET (wid)))
#define GTK_WIDGET_TOPLEVEL(wid)          (gtk_widget_is_toplevel (GTK_WIDGET (wid)))
#define GTK_WIDGET_CAN_FOCUS(wid)         (gtk_widget_get_can_focus (GTK_WIDGET (wid)))
void gtkqt_widget_set_flags (GtkWidget *widget, guint32 flags, gboolean set);

/* ---- GtkContainer / GtkBin ---- */
void     gtk_container_add (GtkContainer *container, GtkWidget *widget);
void     gtk_container_remove (GtkContainer *container, GtkWidget *widget);
void     gtk_container_set_border_width (GtkContainer *container, guint border_width);
guint    gtk_container_get_border_width (GtkContainer *container);
void     gtk_container_border_width (GtkContainer *container, guint border_width);   /* GTK 1 */
GList   *gtk_container_get_children (GtkContainer *container);
void     gtk_container_foreach (GtkContainer *container, GtkCallback callback, gpointer callback_data);
void     gtk_container_forall (GtkContainer *container, GtkCallback callback, gpointer callback_data);
void     gtk_container_set_focus_child (GtkContainer *container, GtkWidget *child);
void     gtk_container_add_with_properties (GtkContainer *container, GtkWidget *widget, const gchar *first_prop_name, ...);
void     gtk_container_child_set (GtkContainer *container, GtkWidget *child, const gchar *first_prop_name, ...);
void     gtk_container_resize_children (GtkContainer *container);
void     gtk_container_check_resize (GtkContainer *container);
GtkWidget *gtk_bin_get_child (GtkBin *bin);

/* ---- GtkWindow ---- */
GtkWidget *gtk_window_new (GtkWindowType type);  /* GTK 4: gtk_window_new (void); see gtkqt-compat.h */
void       gtk_window_set_title (GtkWindow *window, const gchar *title);
const gchar *gtk_window_get_title (GtkWindow *window);
void       gtk_window_set_default_size (GtkWindow *window, gint width, gint height);
void       gtk_window_get_default_size (GtkWindow *window, gint *width, gint *height);
void       gtk_window_resize (GtkWindow *window, gint width, gint height);
void       gtk_window_get_size (GtkWindow *window, gint *width, gint *height);
void       gtk_window_move (GtkWindow *window, gint x, gint y);
void       gtk_window_get_position (GtkWindow *window, gint *root_x, gint *root_y);
void       gtk_window_set_position (GtkWindow *window, GtkWindowPosition position);
void       gtk_window_set_resizable (GtkWindow *window, gboolean resizable);
gboolean   gtk_window_get_resizable (GtkWindow *window);
void       gtk_window_set_modal (GtkWindow *window, gboolean modal);
gboolean   gtk_window_get_modal (GtkWindow *window);
void       gtk_window_set_transient_for (GtkWindow *window, GtkWindow *parent);
GtkWindow *gtk_window_get_transient_for (GtkWindow *window);
void       gtk_window_set_destroy_with_parent (GtkWindow *window, gboolean setting);
void       gtk_window_set_decorated (GtkWindow *window, gboolean setting);
void       gtk_window_set_deletable (GtkWindow *window, gboolean setting);
void       gtk_window_set_keep_above (GtkWindow *window, gboolean setting);
void       gtk_window_set_icon_name (GtkWindow *window, const gchar *name);
void       gtk_window_set_default_icon_name (const gchar *name);
gboolean   gtk_window_set_icon_from_file (GtkWindow *window, const gchar *filename, GError **err);
typedef enum {
  GDK_WINDOW_TYPE_HINT_NORMAL, GDK_WINDOW_TYPE_HINT_DIALOG, GDK_WINDOW_TYPE_HINT_MENU, GDK_WINDOW_TYPE_HINT_TOOLBAR,
  GDK_WINDOW_TYPE_HINT_SPLASHSCREEN, GDK_WINDOW_TYPE_HINT_UTILITY, GDK_WINDOW_TYPE_HINT_DOCK,
  GDK_WINDOW_TYPE_HINT_DESKTOP, GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU, GDK_WINDOW_TYPE_HINT_POPUP_MENU,
  GDK_WINDOW_TYPE_HINT_TOOLTIP, GDK_WINDOW_TYPE_HINT_NOTIFICATION, GDK_WINDOW_TYPE_HINT_COMBO,
  GDK_WINDOW_TYPE_HINT_DND
} GdkWindowTypeHint;
void       gtk_window_set_type_hint (GtkWindow *window, GdkWindowTypeHint hint);
GdkWindowTypeHint gtk_window_get_type_hint (GtkWindow *window);
void       gtk_window_set_wmclass (GtkWindow *window, const gchar *wmclass_name, const gchar *wmclass_class);
void       gtk_window_set_role (GtkWindow *window, const gchar *role);
void       gtk_window_present (GtkWindow *window);
void       gtk_window_present_with_time (GtkWindow *window, guint32 timestamp);
void       gtk_window_close (GtkWindow *window);
void       gtk_window_destroy (GtkWindow *window);
void       gtk_window_maximize (GtkWindow *window);
void       gtk_window_unmaximize (GtkWindow *window);
gboolean   gtk_window_is_maximized (GtkWindow *window);
void       gtk_window_fullscreen (GtkWindow *window);
void       gtk_window_unfullscreen (GtkWindow *window);
void       gtk_window_iconify (GtkWindow *window);
void       gtk_window_minimize (GtkWindow *window);
gboolean   gtk_window_is_active (GtkWindow *window);
void       gtk_window_set_focus (GtkWindow *window, GtkWidget *focus);
GtkWidget *gtk_window_get_focus (GtkWindow *window);
void       gtk_window_set_default (GtkWindow *window, GtkWidget *default_widget);
void       gtk_window_set_default_widget (GtkWindow *window, GtkWidget *default_widget);
void       gtk_window_set_child (GtkWindow *window, GtkWidget *child);
GtkWidget *gtk_window_get_child (GtkWindow *window);
void       gtk_window_set_titlebar (GtkWindow *window, GtkWidget *titlebar);
void       gtk_window_set_hide_on_close (GtkWindow *window, gboolean setting);
gboolean   gtk_window_get_hide_on_close (GtkWindow *window);
void       gtk_window_set_application (GtkWindow *window, GtkApplication *application);
GtkApplication *gtk_window_get_application (GtkWindow *window);
void       gtk_window_add_accel_group (GtkWindow *window, GtkAccelGroup *accel_group);
GList     *gtk_window_list_toplevels (void);
GListModel *gtk_window_get_toplevels (void);

/* ---- GtkAccelGroup (accelerators are attached to the window's actions) ---- */
#define GTK_TYPE_ACCEL_GROUP (gtk_accel_group_get_type ())
#define GTK_ACCEL_GROUP(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ACCEL_GROUP, GtkAccelGroup))
#define GTK_IS_ACCEL_GROUP(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ACCEL_GROUP))
GType          gtk_accel_group_get_type (void) G_GNUC_CONST;
GtkAccelGroup *gtk_accel_group_new (void);
typedef enum { GTK_ACCEL_VISIBLE = 1 << 0, GTK_ACCEL_LOCKED = 1 << 1, GTK_ACCEL_MASK = 0x07 } GtkAccelFlags;
void  gtk_accelerator_parse (const gchar *accelerator, guint *accelerator_key, GdkModifierType *accelerator_mods);
gchar *gtk_accelerator_name (guint accelerator_key, GdkModifierType accelerator_mods);

G_END_DECLS

#endif
