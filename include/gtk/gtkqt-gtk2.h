/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GTK 1/2 era API: GtkStyle and drawing with GdkGC, Pango glue, GtkSettings,
 * stock items and icon factories, GtkClipboard, drag-and-drop targets,
 * GtkItemFactory, GtkFileSelection, GtkFontSelectionDialog, GtkIMContext.
 */
#ifndef GTKQT_GTK2_H
#define GTKQT_GTK2_H

#include <gtk/gtkqt-dialogs.h>
#include <gtk/gtkqt-menus.h>

G_BEGIN_DECLS

/* ---------- GtkStyle (colours from the Qt palette, font from the widget) ---------- */
struct _GtkStyle {
  GObject parent_instance;
  GdkColor fg[5], bg[5], light[5], dark[5], mid[5], text[5], base[5], text_aa[5];
  GdkColor black, white;
  PangoFontDescription *font_desc;
  gint xthickness, ythickness;
  GdkGC *fg_gc[5], *bg_gc[5], *light_gc[5], *dark_gc[5], *mid_gc[5], *text_gc[5], *base_gc[5], *text_aa_gc[5];
  GdkGC *black_gc, *white_gc;
  GdkPixmap *bg_pixmap[5];
  GdkFont *font;              /* GTK 1 */
  /*< private >*/
  gint attach_count;
  gpointer priv;
};
typedef struct { GObjectClass parent_class; } GtkStyleClass;
#define GTK_TYPE_STYLE      (gtk_style_get_type ())
#define GTK_STYLE(o)        (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_STYLE, GtkStyle))
#define GTK_IS_STYLE(o)     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_STYLE))
GType     gtk_style_get_type (void) G_GNUC_CONST;
GtkStyle *gtk_style_new (void);
GtkStyle *gtk_style_copy (GtkStyle *style);
GtkStyle *gtk_style_ref (GtkStyle *style);
void      gtk_style_unref (GtkStyle *style);
GtkStyle *gtk_style_attach (GtkStyle *style, GdkWindow *window);
void      gtk_style_detach (GtkStyle *style);
void      gtk_style_set_background (GtkStyle *style, GdkWindow *window, GtkStateType state_type);
GtkStyle *gtk_widget_get_style (GtkWidget *widget);
GtkStyle *gtk_widget_get_default_style (void);
void      gtk_widget_set_style (GtkWidget *widget, GtkStyle *style);
void      gtk_widget_ensure_style (GtkWidget *widget);
GtkStyle *gtk_rc_get_style (GtkWidget *widget);
void      gtk_widget_modify_font (GtkWidget *widget, PangoFontDescription *font_desc);
void      gtk_widget_modify_fg (GtkWidget *widget, GtkStateType state, const GdkColor *color);
void      gtk_widget_modify_bg (GtkWidget *widget, GtkStateType state, const GdkColor *color);
void      gtk_widget_modify_text (GtkWidget *widget, GtkStateType state, const GdkColor *color);
void      gtk_widget_modify_base (GtkWidget *widget, GtkStateType state, const GdkColor *color);
GtkStateType gtk_widget_get_state (GtkWidget *widget);
void      gtk_widget_set_state (GtkWidget *widget, GtkStateType state);
#define   GTK_WIDGET_STATE(w)       (gtk_widget_get_state (GTK_WIDGET (w)))
#define   GTK_WIDGET_SAVED_STATE(w) (gtk_widget_get_state (GTK_WIDGET (w)))

PangoContext *gtk_widget_get_pango_context (GtkWidget *widget);
PangoContext *gtk_widget_create_pango_context (GtkWidget *widget);
PangoLayout  *gtk_widget_create_pango_layout (GtkWidget *widget, const gchar *text);
PangoLanguage *gtk_get_default_language (void);

void gtk_paint_layout (GtkStyle *style, GdkWindow *window, GtkStateType state_type, gboolean use_text,
                       const GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, PangoLayout *layout);
void gtk_paint_flat_box (GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type,
                         const GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
void gtk_paint_box (GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type,
                    const GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
void gtk_paint_hline (GtkStyle *style, GdkWindow *window, GtkStateType state_type, const GdkRectangle *area,
                      GtkWidget *widget, const gchar *detail, gint x1, gint x2, gint y);
void gtk_paint_vline (GtkStyle *style, GdkWindow *window, GtkStateType state_type, const GdkRectangle *area,
                      GtkWidget *widget, const gchar *detail, gint y1_, gint y2_, gint x);

/* ---------- GtkSettings / resource files ---------- */
typedef struct _GtkSettings GtkSettings;
#define GTK_TYPE_SETTINGS (gtk_settings_get_type ())
#define GTK_SETTINGS(o)   ((GtkSettings *) (o))
GType        gtk_settings_get_type (void) G_GNUC_CONST;
GtkSettings *gtk_settings_get_default (void);
GtkSettings *gtk_widget_get_settings (GtkWidget *widget);
void         gtk_settings_set_string_property (GtkSettings *settings, const gchar *name, const gchar *v_string, const gchar *origin);
void         gtk_settings_set_long_property (GtkSettings *settings, const gchar *name, glong v_long, const gchar *origin);
void         gtk_rc_parse_string (const gchar *rc_string);
void         gtk_rc_parse (const gchar *filename);
void         gtk_rc_add_default_file (const gchar *filename);
GOptionGroup *gtk_get_option_group (gboolean open_default_display);

/* ---------- stock items, icon factories, icon theme ---------- */
typedef struct {
  gchar *stock_id;
  gchar *label;
  GdkModifierType modifier;
  guint keyval;
  gchar *translation_domain;
} GtkStockItem;
void     gtk_stock_add (const GtkStockItem *items, guint n_items);
void     gtk_stock_add_static (const GtkStockItem *items, guint n_items);
gboolean gtk_stock_lookup (const gchar *stock_id, GtkStockItem *item);

typedef struct _GtkIconSet GtkIconSet;
typedef struct _GtkIconSource GtkIconSource;
typedef struct _GtkIconFactory GtkIconFactory;
#define GTK_TYPE_ICON_FACTORY (gtk_icon_factory_get_type ())
GType           gtk_icon_factory_get_type (void) G_GNUC_CONST;
GtkIconFactory *gtk_icon_factory_new (void);
void            gtk_icon_factory_add (GtkIconFactory *factory, const gchar *stock_id, GtkIconSet *icon_set);
GtkIconSet     *gtk_icon_factory_lookup (GtkIconFactory *factory, const gchar *stock_id);
void            gtk_icon_factory_add_default (GtkIconFactory *factory);
void            gtk_icon_factory_remove_default (GtkIconFactory *factory);
GtkIconSet     *gtk_icon_factory_lookup_default (const gchar *stock_id);
GtkIconSet     *gtk_icon_set_new (void);
GtkIconSet     *gtk_icon_set_new_from_pixbuf (GdkPixbuf *pixbuf);
GtkIconSet     *gtk_icon_set_ref (GtkIconSet *icon_set);
void            gtk_icon_set_unref (GtkIconSet *icon_set);
gboolean        gtk_icon_size_lookup (GtkIconSize size, gint *width, gint *height);

typedef struct _GtkIconTheme GtkIconTheme;
typedef enum { GTK_ICON_LOOKUP_NO_SVG = 1 << 0, GTK_ICON_LOOKUP_FORCE_SVG = 1 << 1,
               GTK_ICON_LOOKUP_USE_BUILTIN = 1 << 2, GTK_ICON_LOOKUP_GENERIC_FALLBACK = 1 << 3,
               GTK_ICON_LOOKUP_FORCE_SIZE = 1 << 4 } GtkIconLookupFlags;
GtkIconTheme *gtk_icon_theme_get_default (void);
GdkPixbuf    *gtk_icon_theme_load_icon (GtkIconTheme *icon_theme, const gchar *icon_name, gint size,
                                        GtkIconLookupFlags flags, GError **error);
gboolean      gtk_icon_theme_has_icon (GtkIconTheme *icon_theme, const gchar *icon_name);
void          gtk_window_set_icon (GtkWindow *window, GdkPixbuf *icon);
void          gtk_window_set_default_icon (GdkPixbuf *icon);
gboolean      gtk_window_set_icon_from_file (GtkWindow *window, const gchar *filename, GError **err);

/* ---------- accelerators ---------- */
typedef gboolean (*GtkAccelGroupActivate) (GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier);
void     gtk_accel_group_connect (GtkAccelGroup *accel_group, guint accel_key, GdkModifierType accel_mods,
                                  guint accel_flags, GClosure *closure);
gboolean gtk_accel_group_disconnect (GtkAccelGroup *accel_group, GClosure *closure);
GSList  *gtk_accel_groups_from_object (GObject *object);
void     gtk_window_remove_accel_group (GtkWindow *window, GtkAccelGroup *accel_group);
gboolean gtk_widget_remove_accelerator (GtkWidget *widget, GtkAccelGroup *accel_group, guint accel_key, GdkModifierType accel_mods);

/* ---------- GtkClipboard ---------- */
typedef struct _GtkClipboard GtkClipboard;
typedef void (*GtkClipboardTextReceivedFunc) (GtkClipboard *clipboard, const gchar *text, gpointer data);
GtkClipboard *gtk_clipboard_get (GdkAtom selection);
GtkClipboard *gtk_widget_get_clipboard (GtkWidget *widget, GdkAtom selection);
void          gtk_clipboard_set_text (GtkClipboard *clipboard, const gchar *text, gint len);
gchar        *gtk_clipboard_wait_for_text (GtkClipboard *clipboard);
gboolean      gtk_clipboard_wait_is_text_available (GtkClipboard *clipboard);
void          gtk_clipboard_request_text (GtkClipboard *clipboard, GtkClipboardTextReceivedFunc callback, gpointer user_data);
void          gtk_clipboard_clear (GtkClipboard *clipboard);
void          gtk_clipboard_store (GtkClipboard *clipboard);
void          gtk_clipboard_set_can_store (GtkClipboard *clipboard, gconstpointer targets, gint n_targets);

/* ---------- selections and drag-and-drop (receiving side) ---------- */
typedef struct {
  GdkAtom selection, target, type;
  gint format;
  guchar *data;
  gint length;
  GdkDisplay *display;
} GtkSelectionData;
typedef struct { gchar *target; guint flags; guint info; } GtkTargetEntry;
typedef struct _GtkTargetList GtkTargetList;
typedef enum { GTK_TARGET_SAME_APP = 1 << 0, GTK_TARGET_SAME_WIDGET = 1 << 1,
               GTK_TARGET_OTHER_APP = 1 << 2, GTK_TARGET_OTHER_WIDGET = 1 << 3 } GtkTargetFlags;
typedef enum { GTK_DEST_DEFAULT_MOTION = 1 << 0, GTK_DEST_DEFAULT_HIGHLIGHT = 1 << 1,
               GTK_DEST_DEFAULT_DROP = 1 << 2, GTK_DEST_DEFAULT_ALL = 0x07 } GtkDestDefaults;
#define GTK_TYPE_SELECTION_DATA (gtk_selection_data_get_type ())
GType     gtk_selection_data_get_type (void) G_GNUC_CONST;
gboolean  gtk_selection_data_set_text (GtkSelectionData *selection_data, const gchar *str, gint len);
guchar   *gtk_selection_data_get_text (const GtkSelectionData *selection_data);
gchar   **gtk_selection_data_get_uris (const GtkSelectionData *selection_data);
const guchar *gtk_selection_data_get_data (const GtkSelectionData *selection_data);
gint      gtk_selection_data_get_length (const GtkSelectionData *selection_data);
GdkAtom   gtk_selection_data_get_target (const GtkSelectionData *selection_data);
void      gtk_selection_data_set (GtkSelectionData *selection_data, GdkAtom type, gint format, const guchar *data, gint length);
void      gtk_drag_dest_set (GtkWidget *widget, GtkDestDefaults flags, const GtkTargetEntry *targets, gint n_targets, GdkDragAction actions);
void      gtk_drag_dest_unset (GtkWidget *widget);
void      gtk_drag_finish (GdkDragContext *context, gboolean success, gboolean del, guint32 time_);
void      gtk_drag_get_data (GtkWidget *widget, GdkDragContext *context, GdkAtom target, guint32 time_);
void      gtk_drag_highlight (GtkWidget *widget);
void      gtk_drag_unhighlight (GtkWidget *widget);

/* ---------- GtkIMContext (input methods are Qt's; this reports no preedit) ---------- */
#define GTK_TYPE_IM_CONTEXT (gtk_im_context_get_type ())
GType gtk_im_context_get_type (void) G_GNUC_CONST;
void  gtk_im_context_get_preedit_string (GtkIMContext *context, gchar **str, PangoAttrList **attrs, gint *cursor_pos);
void  gtk_im_context_reset (GtkIMContext *context);

/* ---------- GtkItemFactory (GTK 1.2 / 2.x menus from a table) ---------- */
#if GTKQT_API_VERSION == 1
typedef void (*GtkItemFactoryCallback) ();   /* GTK 1: unprototyped, takes any handler */
#else
typedef void (*GtkItemFactoryCallback) (void);
#endif
typedef void (*GtkItemFactoryCallback1) (gpointer callback_data, guint callback_action, GtkWidget *widget);
typedef void (*GtkItemFactoryCallback2) (GtkWidget *widget, gpointer callback_data, guint callback_action);
typedef gchar *(*GtkTranslateFunc) (const gchar *path, gpointer func_data);
typedef struct {
  gchar *path;
  gchar *accelerator;
  GtkItemFactoryCallback callback;
  guint callback_action;
  gchar *item_type;
  gconstpointer extra_data;
} GtkItemFactoryEntry;
typedef struct _GtkItemFactory {
  GObject object;
  gchar *path;
  GtkAccelGroup *accel_group;
  GtkWidget *widget;
  GSList *items;
  GtkTranslateFunc translate_func;
  gpointer translate_data;
  GDestroyNotify translate_notify;
} GtkItemFactory;
typedef struct { GObjectClass object_class; } GtkItemFactoryClass;
#define GTK_TYPE_ITEM_FACTORY  (gtk_item_factory_get_type ())
#define GTK_ITEM_FACTORY(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ITEM_FACTORY, GtkItemFactory))
#define GTK_IS_ITEM_FACTORY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ITEM_FACTORY))
GType           gtk_item_factory_get_type (void) G_GNUC_CONST;
GtkItemFactory *gtk_item_factory_new (GType container_type, const gchar *path, GtkAccelGroup *accel_group);
void            gtk_item_factory_construct (GtkItemFactory *ifactory, GType container_type, const gchar *path, GtkAccelGroup *accel_group);
void            gtk_item_factory_create_item (GtkItemFactory *ifactory, GtkItemFactoryEntry *entry, gpointer callback_data, guint callback_type);
void            gtk_item_factory_create_items (GtkItemFactory *ifactory, guint n_entries, GtkItemFactoryEntry *entries, gpointer callback_data);
void            gtk_item_factory_create_items_ac (GtkItemFactory *ifactory, guint n_entries, GtkItemFactoryEntry *entries,
                                                  gpointer callback_data, guint callback_type);
GtkItemFactory *gtk_item_factory_from_widget (GtkWidget *widget);
const gchar    *gtk_item_factory_path_from_widget (GtkWidget *widget);
GtkWidget      *gtk_item_factory_get_item (GtkItemFactory *ifactory, const gchar *path);
GtkWidget      *gtk_item_factory_get_widget (GtkItemFactory *ifactory, const gchar *path);
GtkWidget      *gtk_item_factory_get_widget_by_action (GtkItemFactory *ifactory, guint action);
GtkWidget      *gtk_item_factory_get_item_by_action (GtkItemFactory *ifactory, guint action);
void            gtk_item_factory_delete_item (GtkItemFactory *ifactory, const gchar *path);
void            gtk_item_factory_delete_entry (GtkItemFactory *ifactory, GtkItemFactoryEntry *entry);
void            gtk_item_factory_delete_entries (GtkItemFactory *ifactory, guint n_entries, GtkItemFactoryEntry *entries);
void            gtk_item_factory_popup (GtkItemFactory *ifactory, guint x, guint y, guint mouse_button, guint32 time_);
void            gtk_item_factory_popup_with_data (GtkItemFactory *ifactory, gpointer popup_data, GDestroyNotify destroy,
                                                  guint x, guint y, guint mouse_button, guint32 time_);
gpointer        gtk_item_factory_popup_data (GtkItemFactory *ifactory);
gpointer        gtk_item_factory_popup_data_from_widget (GtkWidget *widget);
void            gtk_item_factory_set_translate_func (GtkItemFactory *ifactory, GtkTranslateFunc func, gpointer data, GDestroyNotify notify);

/* ---------- GtkFileSelection (shown as the Qt file dialog) ---------- */
typedef struct _GtkFileSelection {
  GtkDialog parent_instance;
  GtkWidget *dir_list, *file_list, *selection_entry, *selection_text, *main_vbox;
  GtkWidget *ok_button, *cancel_button, *help_button, *history_pulldown, *history_menu;
  GList *history_list;
  GtkWidget *fileop_dialog, *fileop_entry;
  gchar *fileop_file;
  gpointer cmpl_state;
  GtkWidget *fileop_c_dir, *fileop_del_file, *fileop_ren_file;
  GtkWidget *button_area, *action_area;
} GtkFileSelection;
typedef struct { GtkDialogClass parent_class; } GtkFileSelectionClass;
#define GTK_TYPE_FILE_SELECTION  (gtk_file_selection_get_type ())
#define GTK_FILE_SELECTION(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FILE_SELECTION, GtkFileSelection))
#define GTK_IS_FILE_SELECTION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_FILE_SELECTION))
GType        gtk_file_selection_get_type (void) G_GNUC_CONST;
GtkWidget   *gtk_file_selection_new (const gchar *title);
void         gtk_file_selection_set_filename (GtkFileSelection *filesel, const gchar *filename);
const gchar *gtk_file_selection_get_filename (GtkFileSelection *filesel);
gchar      **gtk_file_selection_get_selections (GtkFileSelection *filesel);
void         gtk_file_selection_set_select_multiple (GtkFileSelection *filesel, gboolean select_multiple);
void         gtk_file_selection_complete (GtkFileSelection *filesel, const gchar *pattern);
void         gtk_file_selection_show_fileop_buttons (GtkFileSelection *filesel);
void         gtk_file_selection_hide_fileop_buttons (GtkFileSelection *filesel);
void         gtk_file_chooser_set_extra_widget (GtkFileChooser *chooser, GtkWidget *extra_widget);
GtkWidget   *gtk_file_chooser_get_extra_widget (GtkFileChooser *chooser);

/* ---------- GtkFontSelectionDialog (shown as the Qt font dialog) ---------- */
typedef struct _GtkFontSelectionDialog {
  GtkDialog parent_instance;
  GtkWidget *fontsel, *main_vbox, *action_area;
  GtkWidget *ok_button, *apply_button, *cancel_button;
} GtkFontSelectionDialog;
typedef struct { GtkDialogClass parent_class; } GtkFontSelectionDialogClass;
#define GTK_TYPE_FONT_SELECTION_DIALOG  (gtk_font_selection_dialog_get_type ())
#define GTK_FONT_SELECTION_DIALOG(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FONT_SELECTION_DIALOG, GtkFontSelectionDialog))
#define GTK_IS_FONT_SELECTION_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_FONT_SELECTION_DIALOG))
GType      gtk_font_selection_dialog_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_font_selection_dialog_new (const gchar *title);
gchar     *gtk_font_selection_dialog_get_font_name (GtkFontSelectionDialog *fsd);
gboolean   gtk_font_selection_dialog_set_font_name (GtkFontSelectionDialog *fsd, const gchar *fontname);
void       gtk_font_selection_dialog_set_preview_text (GtkFontSelectionDialog *fsd, const gchar *text);

/* ---------- misc GTK 2 calls ---------- */
void           gtk_button_box_set_child_secondary (GtkButtonBox *widget, GtkWidget *child, gboolean is_secondary);
GtkAdjustment *gtk_scrolled_window_get_hadjustment (GtkScrolledWindow *scrolled_window);
GtkAdjustment *gtk_scrolled_window_get_vadjustment (GtkScrolledWindow *scrolled_window);
void           gtk_viewport_set_shadow_type (GtkViewport *viewport, GtkShadowType type);

/* ---------- accelerator queries (keyboard shortcuts in tooltips ...) ---------- */
typedef struct { guint accel_key; GdkModifierType accel_mods; guint accel_flags : 16; } GtkAccelKey;
typedef gboolean (*GtkAccelGroupFindFunc) (GtkAccelKey *key, GClosure *closure, gpointer data);
GtkAccelKey   *gtk_accel_group_find (GtkAccelGroup *accel_group, GtkAccelGroupFindFunc find_func, gpointer data);
GtkAccelGroup *gtk_accel_group_from_accel_closure (GClosure *closure);
GList         *gtk_widget_list_accel_closures (GtkWidget *widget);
void           gtk_accelerator_set_default_mod_mask (GdkModifierType default_mod_mask);
GdkModifierType gtk_accelerator_get_default_mod_mask (void);

/* ---------- key snoopers (GTK 2): see every key event first ---------- */
typedef gint (*GtkKeySnoopFunc) (GtkWidget *grab_widget, GdkEventKey *event, gpointer func_data);
guint gtk_key_snooper_install (GtkKeySnoopFunc snooper, gpointer func_data);
void  gtk_key_snooper_remove (guint snooper_handler_id);

/* ---------- GtkBuildable: the id an object had in the builder file ---------- */
typedef struct _GtkBuildable GtkBuildable;
#define GTK_BUILDABLE(o) ((GtkBuildable *) (o))
void         gtk_buildable_set_name (GtkBuildable *buildable, const gchar *name);
const gchar *gtk_buildable_get_name (GtkBuildable *buildable);

GdkAtom   gtk_selection_data_get_data_type (const GtkSelectionData *selection_data);
gint      gtk_selection_data_get_format (const GtkSelectionData *selection_data);
void      gtk_widget_get_pointer (GtkWidget *widget, gint *x, gint *y);
void      gtk_widget_get_child_requisition (GtkWidget *widget, GtkRequisition *requisition);
void      gtk_check_menu_item_toggled (GtkCheckMenuItem *check_menu_item);

/* GTK 3.0-3.16 per-widget colours and fonts (the GTK 2 modify_* calls) */
void       gtk_widget_override_font (GtkWidget *widget, const PangoFontDescription *font_desc);
void       gtk_widget_override_color (GtkWidget *widget, GtkStateFlags state, const GdkRGBA *color);
void       gtk_widget_override_background_color (GtkWidget *widget, GtkStateFlags state, const GdkRGBA *color);
GdkDevice *gtk_get_current_event_device (void);
G_END_DECLS

#endif
