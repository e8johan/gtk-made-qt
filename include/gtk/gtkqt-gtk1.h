/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GTK 1.2 API (also still in GTK 2 as deprecated): the GtkType and
 * GtkSignal layer over GObject, GdkFont, GdkCursor, XPM pixmaps, GdkImage,
 * GdkRGB, window-manager calls, and the GTK 1 widgets: GtkPixmap,
 * GtkAspectFrame, GtkCList, GtkCTree, GtkList/GtkListItem, GtkText,
 * GtkProgress, GtkRuler, GtkCalendar, GtkPacker, GtkTooltips,
 * GtkColorSelection.
 */
#ifndef GTKQT_GTK1_H
#define GTKQT_GTK1_H

#include <gtk/gtkqt-gtk2.h>

G_BEGIN_DECLS

/* ======================================================================
 * Types
 * ====================================================================== */
typedef GType GtkType;
typedef void (*GtkClassInitFunc) (gpointer klass);
typedef void (*GtkObjectInitFunc) (gpointer object, gpointer klass);
typedef struct {
  gchar *type_name;
  guint object_size;
  guint class_size;
  GtkClassInitFunc class_init_func;
  GtkObjectInitFunc object_init_func;
  gpointer reserved_1;
  gpointer reserved_2;
  GtkClassInitFunc base_class_init_func;
} GtkTypeInfo;

GtkType     gtk_type_unique (GtkType parent_type, const GtkTypeInfo *gtkinfo);
gpointer    gtk_type_class (GtkType type);
gpointer    gtk_type_new (GtkType type);
const gchar *gtk_type_name (GtkType type);
GtkType     gtk_type_from_name (const gchar *name);
GtkType     gtk_type_parent (GtkType type);
gboolean    gtk_type_is_a (GtkType type, GtkType is_a_type);
void        gtk_type_init (void);
#define gtk_type_get_varargs_type(t) (t)

/* GTK 1 code often keeps types in a guint, which cuts a 64-bit GType in
 * half: these accept such truncated types. */
GType    gtkqt_type_fix (GType type);
gpointer gtkqt_check_cast (gpointer instance, GType type);
gpointer gtkqt_check_class_cast (gpointer klass, GType type);
#define GTK_CHECK_CAST(obj, type, cast)        ((cast *) gtkqt_check_cast ((obj), (type)))
#define GTK_CHECK_CLASS_CAST(klass, type, cast) ((cast *) gtkqt_check_class_cast ((klass), (type)))
#define GTK_CHECK_TYPE(obj, type)              G_TYPE_CHECK_INSTANCE_TYPE ((obj), gtkqt_type_fix (type))
#define GTK_CHECK_CLASS_TYPE(klass, type)      G_TYPE_CHECK_CLASS_TYPE ((klass), gtkqt_type_fix (type))
#define GTK_CHECK_GET_CLASS(obj, type, klass)  G_TYPE_INSTANCE_GET_CLASS ((obj), (type), klass)
#define GTK_CLASS_TYPE(klass)                  G_TYPE_FROM_CLASS (klass)
#define GTK_OBJECT_CLASS(klass)                ((GtkObjectClass *) (klass))
#define GTK_OBJECT_GET_CLASS(obj)              ((GtkObjectClass *) G_OBJECT_GET_CLASS (obj))
#define GTK_TYPE_IS_OBJECT(type)               g_type_is_a ((type), GTK_TYPE_OBJECT)
#define GTK_FUNDAMENTAL_TYPE                   G_TYPE_FUNDAMENTAL
#define GTK_OBJECT_DESTROYED(obj)              FALSE
#define GTK_OBJECT_FLOATING(obj)               g_object_is_floating (obj)
#define GTK_OBJECT_CONNECTED(obj)              TRUE

#define GTK_TYPE_INVALID  G_TYPE_INVALID
#define GTK_TYPE_NONE     G_TYPE_NONE
#define GTK_TYPE_CHAR     G_TYPE_CHAR
#define GTK_TYPE_UCHAR    G_TYPE_UCHAR
#define GTK_TYPE_BOOL     G_TYPE_BOOLEAN
#define GTK_TYPE_INT      G_TYPE_INT
#define GTK_TYPE_UINT     G_TYPE_UINT
#define GTK_TYPE_LONG     G_TYPE_LONG
#define GTK_TYPE_ULONG    G_TYPE_ULONG
#define GTK_TYPE_FLOAT    G_TYPE_FLOAT
#define GTK_TYPE_DOUBLE   G_TYPE_DOUBLE
#define GTK_TYPE_STRING   G_TYPE_STRING
#define GTK_TYPE_ENUM     G_TYPE_ENUM
#define GTK_TYPE_FLAGS    G_TYPE_FLAGS
#define GTK_TYPE_BOXED    G_TYPE_BOXED
#define GTK_TYPE_POINTER  G_TYPE_POINTER
#define GTK_TYPE_SIGNAL   G_TYPE_POINTER
#define GTK_TYPE_C_CALLBACK G_TYPE_POINTER
#define GTK_TYPE_GDK_EVENT G_TYPE_POINTER
#define GTK_TYPE_GDK_WINDOW G_TYPE_POINTER

/* ======================================================================
 * Signals
 * ====================================================================== */
typedef enum {
  GTK_RUN_FIRST = G_SIGNAL_RUN_FIRST, GTK_RUN_LAST = G_SIGNAL_RUN_LAST,
  GTK_RUN_BOTH = G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST, GTK_RUN_NO_RECURSE = G_SIGNAL_NO_RECURSE,
  GTK_RUN_ACTION = G_SIGNAL_ACTION, GTK_RUN_NO_HOOKS = G_SIGNAL_NO_HOOKS
} GtkSignalRunType;
typedef gpointer GtkSignalMarshaller;
typedef void (*GtkDestroyNotify) (gpointer data);
typedef void (*GtkArgSetFunc) (GtkObject *object, gpointer arg, guint arg_id);
typedef void (*GtkArgGetFunc) (GtkObject *object, gpointer arg, guint arg_id);
#define GTK_SIGNAL_OFFSET G_STRUCT_OFFSET
typedef void (*GtkCallbackMarshal) (GtkObject *object, gpointer data, guint n_args, gpointer args);

/* The marshaller argument is ignored (GLib's generic marshaller is used). */
#define gtk_signal_default_marshaller NULL
#define gtk_marshal_NONE__NONE        NULL
#define gtk_marshal_NONE__INT         NULL
#define gtk_marshal_NONE__INT_INT     NULL
#define gtk_marshal_NONE__POINTER     NULL
#define gtk_marshal_NONE__POINTER_POINTER NULL
#define gtk_marshal_NONE__BOOL        NULL
#define gtk_marshal_NONE__UINT        NULL
#define gtk_marshal_NONE__ENUM        NULL
#define gtk_marshal_NONE__STRING      NULL
#define gtk_marshal_NONE__INT_INT_POINTER NULL
#define gtk_marshal_NONE__POINTER_INT NULL
#define gtk_marshal_BOOL__POINTER     NULL
#define gtk_marshal_BOOL__NONE        NULL
#define gtk_marshal_INT__POINTER      NULL
#define gtk_marshal_INT__INT          NULL

guint    gtk_signal_new (const gchar *name, GtkSignalRunType signal_flags, GtkType object_type, guint function_offset,
                         GtkSignalMarshaller marshaller, GtkType return_val, guint n_args, ...);
guint    gtk_signal_newv (const gchar *name, GtkSignalRunType signal_flags, GtkType object_type, guint function_offset,
                          GtkSignalMarshaller marshaller, GtkType return_val, guint n_args, GtkType *args);
void     gtk_signal_emit (GtkObject *object, guint signal_id, ...);
guint    gtk_signal_lookup (const gchar *name, GtkType object_type);
const gchar *gtk_signal_name (guint signal_id);
void     gtk_signal_emit_stop (GtkObject *object, guint signal_id);
void     gtk_object_class_add_signals (GtkObjectClass *klass, guint *signals, guint nsignals);
gulong   gtk_signal_connect_full (GtkObject *object, const gchar *name, GCallback func, GtkCallbackMarshal unsupported,
                                  gpointer data, GtkDestroyNotify destroy_func, gint object_signal, gint after);
void     gtk_signal_connect_while_alive (GtkObject *object, const gchar *name, GCallback func, gpointer func_data, GtkObject *alive_object);
void     gtk_signal_connect_object_while_alive (GtkObject *object, const gchar *name, GCallback func, GtkObject *alive_object);
void     gtk_signal_handlers_destroy (GtkObject *object);

#ifndef GTKQT_COMPILATION
#define gtk_signal_connect_object_after(o, name, f, slot) g_signal_connect_data ((o), (name), (f), (slot), NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER)
#define gtk_signal_disconnect_by_func(o, f, d)      g_signal_handlers_disconnect_matched ((o), (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, NULL, (gpointer) (f), (d))
#define gtk_signal_disconnect_by_data(o, d)         g_signal_handlers_disconnect_matched ((o), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (d))
#define gtk_signal_handler_block(o, id)             g_signal_handler_block ((o), (id))
#define gtk_signal_handler_unblock(o, id)           g_signal_handler_unblock ((o), (id))
#define gtk_signal_handler_block_by_func(o, f, d)   g_signal_handlers_block_matched ((o), (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, NULL, (gpointer) (f), (d))
#define gtk_signal_handler_unblock_by_func(o, f, d) g_signal_handlers_unblock_matched ((o), (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, NULL, (gpointer) (f), (d))
#define gtk_signal_handler_block_by_data(o, d)      g_signal_handlers_block_matched ((o), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (d))
#define gtk_signal_handler_unblock_by_data(o, d)    g_signal_handlers_unblock_matched ((o), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (d))
#define gtk_signal_handler_pending(o, id, b)        g_signal_has_handler_pending ((o), (id), 0, (b))
#define gtk_object_set_user_data(o, d)              g_object_set_data (G_OBJECT (o), "user_data", (d))
#define gtk_object_get_user_data(o)                 g_object_get_data (G_OBJECT (o), "user_data")
#define gtk_object_set_data_full(o, k, d, f)        g_object_set_data_full (G_OBJECT (o), (k), (d), (f))
#define gtk_object_remove_data(o, k)                g_object_set_data (G_OBJECT (o), (k), NULL)
#define gtk_object_remove_no_notify(o, k)           g_object_steal_data (G_OBJECT (o), (k))
#define gtk_object_set_data_by_id(o, id, d)         g_datalist_id_set_data (&G_OBJECT (o)->qdata, (id), (d))
#define gtk_object_set_data_by_id_full(o, id, d, f) g_object_set_qdata_full (G_OBJECT (o), (id), (d), (f))
#define gtk_object_get_data_by_id(o, id)            g_object_get_qdata (G_OBJECT (o), (id))
#define gtk_object_remove_data_by_id(o, id)         g_object_set_qdata (G_OBJECT (o), (id), NULL)
#define gtk_object_data_try_key                     g_quark_try_string
#define gtk_object_data_force_id                    g_quark_from_string
#define gtk_object_weakref(o, f, d)                 g_object_weak_ref (G_OBJECT (o), (GWeakNotify) (f), (d))
#define gtk_object_weakunref(o, f, d)               g_object_weak_unref (G_OBJECT (o), (GWeakNotify) (f), (d))
#define gtk_object_get_type()                       GTK_TYPE_OBJECT
#endif

/* GtkArg (GTK 1 object arguments): "GtkWidget::width" style names are
 * mapped onto GObject properties. */
typedef struct _GtkArg {
  GtkType type;
  gchar *name;
  union {
    gchar char_data; guchar uchar_data; gboolean bool_data; gint int_data; guint uint_data; glong long_data;
    gulong ulong_data; gfloat float_data; gdouble double_data; gchar *string_data; GtkObject *object_data;
    gpointer pointer_data;
    struct { GCallback f; gpointer d; } signal_data;
  } d;
} GtkArg;
#define GTK_VALUE_CHAR(a)    ((a).d.char_data)
#define GTK_VALUE_BOOL(a)    ((a).d.bool_data)
#define GTK_VALUE_INT(a)     ((a).d.int_data)
#define GTK_VALUE_UINT(a)    ((a).d.uint_data)
#define GTK_VALUE_LONG(a)    ((a).d.long_data)
#define GTK_VALUE_FLOAT(a)   ((a).d.float_data)
#define GTK_VALUE_DOUBLE(a)  ((a).d.double_data)
#define GTK_VALUE_STRING(a)  ((a).d.string_data)
#define GTK_VALUE_POINTER(a) ((a).d.pointer_data)
#define GTK_VALUE_OBJECT(a)  ((a).d.object_data)
#define GTK_VALUE_ENUM(a)    ((a).d.int_data)
#define GTK_VALUE_FLAGS(a)   ((a).d.uint_data)
void gtk_object_arg_get (GtkObject *object, GtkArg *arg, gpointer info);
void gtk_object_arg_set (GtkObject *object, GtkArg *arg, gpointer info);
void gtk_widget_set (GtkWidget *widget, const gchar *first_property_name, ...);
void gtk_object_set (GtkObject *object, const gchar *first_property_name, ...);
void gtk_object_get (GtkObject *object, const gchar *first_property_name, ...);
GtkObject *gtk_object_new (GtkType type, const gchar *first_property_name, ...);
GtkWidget *gtk_widget_new (GtkType type, const gchar *first_property_name, ...);

/* ======================================================================
 * Misc GTK 1 calls
 * ====================================================================== */
gchar     *gtk_set_locale (void);
void       gtk_rc_init (void);
void       gtk_widget_reparent (GtkWidget *widget, GtkWidget *new_parent);
void       gtk_window_position (GtkWindow *window, GtkWindowPosition position);
void       gtk_paned_set_handle_size (GtkPaned *paned, guint16 size);
void       gtk_paned_set_gutter_size (GtkPaned *paned, guint16 size);
GdkFont   *gtk_font_selection_dialog_get_font (GtkFontSelectionDialog *fsd);
GList     *gtk_container_children (GtkContainer *container);
#define    gtk_packer_configure gtk_packer_set_child_packing
void       gtk_exit (gint error_code);
void       gtk_widget_draw (GtkWidget *widget, const GdkRectangle *area);
void       gtk_widget_draw_focus (GtkWidget *widget);
void       gtk_widget_draw_default (GtkWidget *widget);
void       gtk_widget_set_uposition (GtkWidget *widget, gint x, gint y);
void       gtk_widget_shape_combine_mask (GtkWidget *widget, GdkBitmap *shape_mask, gint offset_x, gint offset_y);
gboolean   gtk_widget_event (GtkWidget *widget, GdkEvent *event);
void       gtk_widget_set_default_colormap (GdkColormap *colormap);
void       gtk_widget_set_default_visual (GdkVisual *visual);
void       gtk_widget_push_colormap (GdkColormap *cmap);
void       gtk_widget_pop_colormap (void);
void       gtk_widget_push_visual (GdkVisual *visual);
void       gtk_widget_pop_visual (void);
void       gtk_widget_push_style (GtkStyle *style);
void       gtk_widget_pop_style (void);
GdkColormap *gtk_widget_get_colormap (GtkWidget *widget);
GdkVisual  *gtk_widget_get_visual (GtkWidget *widget);
void       gtk_widget_set_rc_style (GtkWidget *widget);
void       gtk_widget_restore_default_style (GtkWidget *widget);
void       gtk_window_set_policy (GtkWindow *window, gint allow_shrink, gint allow_grow, gint auto_shrink);
typedef enum { GDK_GRAVITY_NORTH_WEST = 1, GDK_GRAVITY_NORTH, GDK_GRAVITY_NORTH_EAST, GDK_GRAVITY_WEST, GDK_GRAVITY_CENTER,
               GDK_GRAVITY_EAST, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_SOUTH, GDK_GRAVITY_SOUTH_EAST, GDK_GRAVITY_STATIC } GdkGravity;
typedef enum { GDK_HINT_POS = 1 << 0, GDK_HINT_MIN_SIZE = 1 << 1, GDK_HINT_MAX_SIZE = 1 << 2, GDK_HINT_BASE_SIZE = 1 << 3,
               GDK_HINT_ASPECT = 1 << 4, GDK_HINT_RESIZE_INC = 1 << 5, GDK_HINT_WIN_GRAVITY = 1 << 6,
               GDK_HINT_USER_POS = 1 << 7, GDK_HINT_USER_SIZE = 1 << 8 } GdkWindowHints;
typedef struct {
  gint min_width, min_height, max_width, max_height, base_width, base_height, width_inc, height_inc;
  gdouble min_aspect, max_aspect;
  GdkGravity win_gravity;
} GdkGeometry;
void       gtk_window_set_geometry_hints (GtkWindow *window, GtkWidget *geometry_widget, GdkGeometry *geometry, GdkWindowHints geom_mask);
void       gtk_label_get (GtkLabel *label, gchar **str);
void       gtk_label_set (GtkLabel *label, const gchar *str);
guint      gtk_label_parse_uline (GtkLabel *label, const gchar *string);
GSList    *gtk_radio_button_group (GtkRadioButton *radio_button);
GSList    *gtk_radio_menu_item_group (GtkRadioMenuItem *radio_menu_item);
void       gtk_notebook_set_page (GtkNotebook *notebook, gint page_num);
GtkWidget *gtk_menu_get_active (GtkMenu *menu);
void       gtk_menu_set_active (GtkMenu *menu, guint index);
void       gtk_option_menu_remove_menu (GtkOptionMenu *option_menu);
void       gtk_button_box_set_spacing (GtkButtonBox *widget, gint spacing);
void       gtk_button_box_set_child_size (GtkButtonBox *widget, gint min_width, gint min_height);
void       gtk_button_box_set_child_ipadding (GtkButtonBox *widget, gint ipad_x, gint ipad_y);
gboolean   gtk_accel_group_activate (GtkAccelGroup *accel_group, guint accel_key, GdkModifierType accel_mods);
void       gtk_item_factory_parse_rc (const gchar *file_name);
void       gtk_item_factory_dump_rc (const gchar *file_name, gpointer path_pspec, gboolean modified_only);
void       gtk_item_factory_add_foreign (GtkWidget *accel_widget, const gchar *full_path, GtkAccelGroup *accel_group,
                                         guint keyval, GdkModifierType modifiers);
void       gtk_combo_set_use_arrows_always (GtkCombo *combo, gboolean val);
void       gtk_entry_select_region (GtkEntry *entry, gint start, gint end);
void       gtk_entry_set_position (GtkEntry *entry, gint position);
void       gtk_spin_button_set_shadow_type (GtkSpinButton *spin_button, GtkShadowType shadow_type);
void       gtk_spin_button_set_snap_to_ticks (GtkSpinButton *spin_button, gboolean snap_to_ticks);
gfloat     gtk_spin_button_get_value_as_float (GtkSpinButton *spin_button);
void       gtk_container_set_resize_mode (GtkContainer *container, gint resize_mode);
void       gtk_container_set_focus_vadjustment (GtkContainer *container, GtkAdjustment *adjustment);
void       gtk_container_set_focus_hadjustment (GtkContainer *container, GtkAdjustment *adjustment);
void       gtk_grab_add (GtkWidget *widget);
void       gtk_grab_remove (GtkWidget *widget);
GdkWindow *gdk_selection_owner_get (GdkAtom selection);
void       gtk_selection_add_target (GtkWidget *widget, GdkAtom selection, GdkAtom target, guint info);
gboolean   gtk_selection_owner_set (GtkWidget *widget, GdkAtom selection, guint32 time_);
gboolean   gtk_selection_convert (GtkWidget *widget, GdkAtom selection, GdkAtom target, guint32 time_);
#define GDK_SELECTION_TYPE_ATOM    _GDK_MAKE_ATOM (4)
#define GDK_SELECTION_TYPE_INTEGER _GDK_MAKE_ATOM (19)
#define GDK_TARGET_BITMAP          _GDK_MAKE_ATOM (5)
#define GDK_TARGET_PIXMAP          _GDK_MAKE_ATOM (20)

/* ======================================================================
 * GDK 1: windows, fonts, cursors, images, RGB, input
 * ====================================================================== */
typedef enum { GDK_INPUT_OUTPUT, GDK_INPUT_ONLY } GdkWindowClass;
typedef enum { GDK_WINDOW_ROOT, GDK_WINDOW_TOPLEVEL, GDK_WINDOW_CHILD, GDK_WINDOW_DIALOG, GDK_WINDOW_TEMP,
               GDK_WINDOW_PIXMAP, GDK_WINDOW_FOREIGN } GdkWindowType;
typedef enum { GDK_WA_TITLE = 1 << 1, GDK_WA_X = 1 << 2, GDK_WA_Y = 1 << 3, GDK_WA_CURSOR = 1 << 4,
               GDK_WA_COLORMAP = 1 << 5, GDK_WA_VISUAL = 1 << 6, GDK_WA_WMCLASS = 1 << 7,
               GDK_WA_NOREDIR = 1 << 8 } GdkWindowAttributesType;
typedef struct _GdkCursor GdkCursor;
typedef struct {
  gchar *title;
  gint event_mask;
  gint16 x, y;
  gint16 width, height;
  GdkWindowClass wclass;
  GdkVisual *visual;
  GdkColormap *colormap;
  GdkWindowType window_type;
  GdkCursor *cursor;
  gchar *wmclass_name, *wmclass_class;
  gboolean override_redirect;
} GdkWindowAttr;
GdkWindow *gdk_window_new (GdkWindow *parent, GdkWindowAttr *attributes, gint attributes_mask);
void       gdk_window_destroy (GdkWindow *window);
void       gdk_window_reparent (GdkWindow *window, GdkWindow *new_parent, gint x, gint y);
#define    gdk_window_ref(w)   ((GdkWindow *) g_object_ref (w))
#define    gdk_window_unref(w) g_object_unref (w)
void       gdk_window_set_user_data (GdkWindow *window, gpointer user_data);
void       gdk_window_show (GdkWindow *window);
void       gdk_window_hide (GdkWindow *window);
void       gdk_window_move (GdkWindow *window, gint x, gint y);
void       gdk_window_resize (GdkWindow *window, gint width, gint height);
void       gdk_window_move_resize (GdkWindow *window, gint x, gint y, gint width, gint height);
gboolean   gdk_window_get_root_origin (GdkWindow *window, gint *x, gint *y);
gboolean   gdk_window_get_deskrelative_origin (GdkWindow *window, gint *x, gint *y);
void       gdk_window_set_geometry_hints (GdkWindow *window, GdkGeometry *geometry, GdkWindowHints flags);
void       gdk_window_set_hints (GdkWindow *window, gint x, gint y, gint min_width, gint min_height,
                                 gint max_width, gint max_height, gint flags);
void       gdk_window_set_decorations (GdkWindow *window, gint decorations);
void       gdk_window_set_functions (GdkWindow *window, gint functions);
void       gdk_window_set_override_redirect (GdkWindow *window, gboolean override_redirect);
void       gdk_window_set_icon (GdkWindow *window, GdkWindow *icon_window, GdkPixmap *pixmap, GdkBitmap *mask);
void       gdk_window_set_icon_name (GdkWindow *window, const gchar *name);
void       gdk_window_set_title (GdkWindow *window, const gchar *title);
void       gdk_window_set_group (GdkWindow *window, GdkWindow *leader);
void       gdk_window_set_cursor (GdkWindow *window, GdkCursor *cursor);
void       gdk_window_shape_combine_mask (GdkWindow *window, GdkBitmap *mask, gint x, gint y);
void       gdk_window_set_events (GdkWindow *window, gint event_mask);
gint       gdk_window_get_events (GdkWindow *window);
GdkWindow *gdk_window_get_parent (GdkWindow *window);
GdkWindow *gdk_window_get_toplevel (GdkWindow *window);
gboolean   gdk_window_is_visible (GdkWindow *window);
gboolean   gdk_window_is_viewable (GdkWindow *window);
void       gdk_window_add_filter (GdkWindow *window, gpointer function, gpointer data);
/* window-manager requests (GDK 2) */
typedef enum { GDK_WINDOW_EDGE_NORTH_WEST, GDK_WINDOW_EDGE_NORTH, GDK_WINDOW_EDGE_NORTH_EAST, GDK_WINDOW_EDGE_WEST,
               GDK_WINDOW_EDGE_EAST, GDK_WINDOW_EDGE_SOUTH_WEST, GDK_WINDOW_EDGE_SOUTH, GDK_WINDOW_EDGE_SOUTH_EAST } GdkWindowEdge;
void       gdk_window_iconify (GdkWindow *window);
void       gdk_window_deiconify (GdkWindow *window);
void       gdk_window_maximize (GdkWindow *window);
void       gdk_window_unmaximize (GdkWindow *window);
void       gdk_window_fullscreen (GdkWindow *window);
void       gdk_window_unfullscreen (GdkWindow *window);
void       gdk_window_stick (GdkWindow *window);
void       gdk_window_unstick (GdkWindow *window);
void       gdk_window_set_keep_above (GdkWindow *window, gboolean setting);
void       gdk_window_set_keep_below (GdkWindow *window, gboolean setting);
void       gdk_window_set_skip_taskbar_hint (GdkWindow *window, gboolean skips);
void       gdk_window_set_skip_pager_hint (GdkWindow *window, gboolean skips);
void       gdk_window_focus (GdkWindow *window, guint32 timestamp);
void       gdk_window_begin_move_drag (GdkWindow *window, gint button, gint root_x, gint root_y, guint32 timestamp);
void       gdk_window_begin_resize_drag (GdkWindow *window, GdkWindowEdge edge, gint button, gint root_x, gint root_y, guint32 timestamp);
void       gdk_window_remove_filter (GdkWindow *window, gpointer function, gpointer data);
void       gdk_add_client_message_filter (GdkAtom message_type, gpointer func, gpointer data);
typedef enum { GDK_PROP_MODE_REPLACE, GDK_PROP_MODE_PREPEND, GDK_PROP_MODE_APPEND } GdkPropMode;
void       gdk_property_change (GdkWindow *window, GdkAtom property, GdkAtom type, gint format, GdkPropMode mode,
                                const guchar *data, gint nelements);
void       gdk_set_sm_client_id (const gchar *sm_client_id);
void       gdk_error_trap_push (void);
gint       gdk_error_trap_pop (void);
gint       gdk_pointer_grab (GdkWindow *window, gboolean owner_events, gint event_mask, GdkWindow *confine_to,
                             GdkCursor *cursor, guint32 time_);
void       gdk_pointer_ungrab (guint32 time_);
gint       gdk_keyboard_grab (GdkWindow *window, gboolean owner_events, guint32 time_);
void       gdk_keyboard_ungrab (guint32 time_);
gboolean   gdk_pointer_is_grabbed (void);
gint       gdk_screen_width_mm (void);
gint       gdk_screen_height_mm (void);
gchar     *gdk_get_display (void);
#define GDK_DECOR_ALL      (1 << 0)
#define GDK_DECOR_BORDER   (1 << 1)
#define GDK_DECOR_TITLE    (1 << 3)
#define GDK_GRAB_SUCCESS   0

typedef enum { GDK_INPUT_READ = 1 << 0, GDK_INPUT_WRITE = 1 << 1, GDK_INPUT_EXCEPTION = 1 << 2 } GdkInputCondition;
typedef void (*GdkInputFunction) (gpointer data, gint source, GdkInputCondition condition);
gint gdk_input_add (gint source, GdkInputCondition condition, GdkInputFunction function, gpointer data);
gint gdk_input_add_full (gint source, GdkInputCondition condition, GdkInputFunction function, gpointer data, GDestroyNotify destroy);
void gdk_input_remove (gint tag);
#define gtk_input_add_full(s, c, f, m, d, n) gdk_input_add_full ((s), (c), (GdkInputFunction) (f), (d), (n))
#define gtk_input_remove(t) gdk_input_remove (t)

/* cursors */
typedef enum {
  GDK_X_CURSOR = 0, GDK_ARROW = 2, GDK_CROSSHAIR = 34, GDK_FLEUR = 52, GDK_HAND1 = 58, GDK_HAND2 = 60,
  GDK_LEFT_PTR = 68, GDK_QUESTION_ARROW = 92, GDK_SB_H_DOUBLE_ARROW = 108, GDK_SB_V_DOUBLE_ARROW = 116,
  GDK_TOP_LEFT_CORNER = 134, GDK_BOTTOM_RIGHT_CORNER = 14, GDK_WATCH = 150, GDK_XTERM = 152,
  GDK_PENCIL = 86, GDK_PLUS = 90, GDK_TCROSS = 130, GDK_CIRCLE = 24, GDK_SIZING = 120, GDK_BLANK_CURSOR = -2
} GdkCursorType;
GdkCursor *gdk_cursor_new (GdkCursorType cursor_type);
GdkCursor *gdk_cursor_new_from_pixmap (GdkPixmap *source, GdkPixmap *mask, const GdkColor *fg, const GdkColor *bg, gint x, gint y);
void       gdk_cursor_destroy (GdkCursor *cursor);
#define    gdk_cursor_unref gdk_cursor_destroy

/* fonts (X font names are matched to system fonts) */
typedef enum { GDK_FONT_FONT, GDK_FONT_FONTSET } GdkFontType;
struct _GdkFont { GdkFontType type; gint ascent; gint descent; };
typedef guint32 GdkWChar;
GdkFont *gdk_font_load (const gchar *font_name);
GdkFont *gdk_fontset_load (const gchar *fontset_name);
GdkFont *gdk_font_from_description (PangoFontDescription *font_desc);
GdkFont *gdk_font_ref (GdkFont *font);
void     gdk_font_unref (GdkFont *font);
gint     gdk_font_id (const GdkFont *font);
gboolean gdk_font_equal (const GdkFont *a, const GdkFont *b);
gint     gdk_string_width (GdkFont *font, const gchar *string);
gint     gdk_text_width (GdkFont *font, const gchar *text, gint text_length);
gint     gdk_text_width_wc (GdkFont *font, const GdkWChar *text, gint text_length);
gint     gdk_char_width (GdkFont *font, gchar character);
gint     gdk_string_measure (GdkFont *font, const gchar *string);
gint     gdk_text_measure (GdkFont *font, const gchar *text, gint text_length);
gint     gdk_string_height (GdkFont *font, const gchar *string);
gint     gdk_text_height (GdkFont *font, const gchar *text, gint text_length);
gint     gdk_char_height (GdkFont *font, gchar character);
void     gdk_string_extents (GdkFont *font, const gchar *string, gint *lbearing, gint *rbearing, gint *width, gint *ascent, gint *descent);
void     gdk_text_extents (GdkFont *font, const gchar *text, gint text_length, gint *lbearing, gint *rbearing,
                           gint *width, gint *ascent, gint *descent);
void     gdk_draw_string (GdkDrawable *drawable, GdkFont *font, GdkGC *gc, gint x, gint y, const gchar *string);
void     gdk_draw_text (GdkDrawable *drawable, GdkFont *font, GdkGC *gc, gint x, gint y, const gchar *text, gint text_length);
void     gdk_draw_text_wc (GdkDrawable *drawable, GdkFont *font, GdkGC *gc, gint x, gint y, const GdkWChar *text, gint text_length);
gint     gdk_mbstowcs (GdkWChar *dest, const gchar *src, gint dest_max);
gchar   *gdk_wcstombs (const GdkWChar *src);
gboolean gdk_iswalnum (GdkWChar wc);
gboolean gdk_iswspace (GdkWChar wc);
gchar   *gdk_set_locale (void);
GdkFont *gtk_style_get_font (GtkStyle *style);
void     gtk_draw_string (GtkStyle *style, GdkWindow *window, GtkStateType state_type, gint x, gint y, const gchar *string);
void     gtk_paint_string (GtkStyle *style, GdkWindow *window, GtkStateType state_type, const GdkRectangle *area,
                           GtkWidget *widget, const gchar *detail, gint x, gint y, const gchar *string);
void     gtk_draw_polygon (GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type,
                           GdkPoint *points, gint npoints, gboolean fill);
void     gtk_paint_polygon (GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type,
                            const GdkRectangle *area, GtkWidget *widget, const gchar *detail, const GdkPoint *points,
                            gint npoints, gboolean fill);
void     gtk_draw_shadow (GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type,
                          gint x, gint y, gint width, gint height);
void     gtk_paint_shadow (GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type,
                           const GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
void     gtk_draw_hline (GtkStyle *style, GdkWindow *window, GtkStateType state_type, gint x1, gint x2, gint y);
void     gtk_draw_vline (GtkStyle *style, GdkWindow *window, GtkStateType state_type, gint y1_, gint y2_, gint x);

/* XPM pixmaps */
GdkPixmap *gdk_pixmap_create_from_xpm (GdkWindow *window, GdkBitmap **mask, const GdkColor *transparent_color, const gchar *filename);
GdkPixmap *gdk_pixmap_create_from_xpm_d (GdkWindow *window, GdkBitmap **mask, const GdkColor *transparent_color, gchar **data);
GdkPixmap *gdk_pixmap_colormap_create_from_xpm (GdkWindow *window, GdkColormap *colormap, GdkBitmap **mask,
                                                const GdkColor *transparent_color, const gchar *filename);
GdkPixmap *gdk_pixmap_colormap_create_from_xpm_d (GdkWindow *window, GdkColormap *colormap, GdkBitmap **mask,
                                                  const GdkColor *transparent_color, gchar **data);
GdkBitmap *gdk_bitmap_create_from_data (GdkWindow *window, const gchar *data, gint width, gint height);
GdkPixmap *gdk_pixmap_create_from_data (GdkWindow *window, const gchar *data, gint width, gint height, gint depth,
                                        const GdkColor *fg, const GdkColor *bg);

/* visuals and images */
typedef enum { GDK_VISUAL_STATIC_GRAY, GDK_VISUAL_GRAYSCALE, GDK_VISUAL_STATIC_COLOR, GDK_VISUAL_PSEUDO_COLOR,
               GDK_VISUAL_TRUE_COLOR, GDK_VISUAL_DIRECT_COLOR } GdkVisualType;
typedef enum { GDK_LSB_FIRST, GDK_MSB_FIRST } GdkByteOrder;
struct _GdkVisual {
  GdkVisualType type;
  gint depth;
  GdkByteOrder byte_order;
  gint colormap_size;
  gint bits_per_rgb;
  guint32 red_mask; gint red_shift; gint red_prec;
  guint32 green_mask; gint green_shift; gint green_prec;
  guint32 blue_mask; gint blue_shift; gint blue_prec;
};
struct _GdkColormap { gint size; GdkColor *colors; };
GdkVisual *gdk_visual_get_best (void);
GdkVisual *gdk_visual_get_best_with_depth (gint depth);
gint       gdk_visual_get_best_depth (void);
typedef enum { GDK_IMAGE_NORMAL, GDK_IMAGE_SHARED, GDK_IMAGE_FASTEST } GdkImageType;
typedef struct _GdkImage {
  GdkImageType type;
  GdkVisual *visual;
  GdkByteOrder byte_order;
  guint16 width, height;
  guint16 depth;
  guint16 bpp;          /* bytes per pixel */
  guint16 bpl;          /* bytes per line */
  gpointer mem;
} GdkImage;
GdkImage *gdk_image_new (GdkImageType type, GdkVisual *visual, gint width, gint height);
GdkImage *gdk_image_get (GdkWindow *window, gint x, gint y, gint width, gint height);
void      gdk_image_put_pixel (GdkImage *image, gint x, gint y, guint32 pixel);
guint32   gdk_image_get_pixel (GdkImage *image, gint x, gint y);
void      gdk_image_destroy (GdkImage *image);
#define   gdk_image_unref gdk_image_destroy
void      gdk_draw_image (GdkDrawable *drawable, GdkGC *gc, GdkImage *image, gint xsrc, gint ysrc,
                          gint xdest, gint ydest, gint width, gint height);

/* GdkRGB */
typedef enum { GDK_RGB_DITHER_NONE, GDK_RGB_DITHER_NORMAL, GDK_RGB_DITHER_MAX } GdkRgbDither;
typedef struct { guint32 colors[256]; gint n_colors; guchar *lut; } GdkRgbCmap;
void         gdk_rgb_init (void);
GdkVisual   *gdk_rgb_get_visual (void);
GdkColormap *gdk_rgb_get_cmap (void);
#define      gdk_rgb_get_colormap gdk_rgb_get_cmap
gulong       gdk_rgb_xpixel_from_rgb (guint32 rgb);
void         gdk_rgb_gc_set_foreground (GdkGC *gc, guint32 rgb);
void         gdk_rgb_gc_set_background (GdkGC *gc, guint32 rgb);
void         gdk_rgb_set_install (gboolean install);
void         gdk_rgb_set_min_colors (gint min_colors);
void         gdk_rgb_set_verbose (gboolean verbose);
GdkRgbCmap  *gdk_rgb_cmap_new (guint32 *colors, gint n_colors);
void         gdk_rgb_cmap_free (GdkRgbCmap *cmap);
void gdk_draw_rgb_image (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, gint width, gint height, GdkRgbDither dith,
                         const guchar *rgb_buf, gint rowstride);
void gdk_draw_rgb_image_dithalign (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, gint width, gint height, GdkRgbDither dith,
                                   const guchar *rgb_buf, gint rowstride, gint xdith, gint ydith);
void gdk_draw_rgb_32_image (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, gint width, gint height, GdkRgbDither dith,
                            const guchar *buf, gint rowstride);
void gdk_draw_gray_image (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, gint width, gint height, GdkRgbDither dith,
                          const guchar *buf, gint rowstride);
void gdk_draw_indexed_image (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, gint width, gint height, GdkRgbDither dith,
                             const guchar *buf, gint rowstride, GdkRgbCmap *cmap);

G_END_DECLS

#include <gtk/gtkqt-gtk1widgets.h>

#endif
