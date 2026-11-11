/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Menus, toolbars, GtkAction/GtkUIManager (GTK 2/3) and GMenuModel menus (GTK 3/4).
 */
#ifndef GTKQT_MENUS_H
#define GTKQT_MENUS_H

#include <gtk/gtkqt-controls.h>

G_BEGIN_DECLS

GTKQT_SIMPLE_TYPE (GtkMenuShell, GtkContainer)
GTKQT_SIMPLE_TYPE (GtkMenuBar, GtkMenuShell)
GTKQT_SIMPLE_TYPE (GtkMenu, GtkMenuShell)
#define GTK_TYPE_MENU_SHELL  (gtk_menu_shell_get_type ())
#define GTK_MENU_SHELL(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_MENU_SHELL, GtkMenuShell))
#define GTK_IS_MENU_SHELL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_MENU_SHELL))
#define GTK_TYPE_MENU_BAR    (gtk_menu_bar_get_type ())
#define GTK_MENU_BAR(o)      (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_MENU_BAR, GtkMenuBar))
#define GTK_IS_MENU_BAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_MENU_BAR))
#define GTK_TYPE_MENU        (gtk_menu_get_type ())
#define GTK_MENU(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_MENU, GtkMenu))
#define GTK_IS_MENU(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_MENU))
typedef void (*GtkMenuPositionFunc) (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data);
GType      gtk_menu_shell_get_type (void) G_GNUC_CONST;
GType      gtk_menu_bar_get_type (void) G_GNUC_CONST;
GType      gtk_menu_get_type (void) G_GNUC_CONST;
void       gtk_menu_shell_append (GtkMenuShell *menu_shell, GtkWidget *child);
void       gtk_menu_shell_prepend (GtkMenuShell *menu_shell, GtkWidget *child);
void       gtk_menu_shell_insert (GtkMenuShell *menu_shell, GtkWidget *child, gint position);
GtkWidget *gtk_menu_bar_new (void);
GtkWidget *gtk_menu_bar_new_from_model (GMenuModel *model);
GtkWidget *gtk_menu_new (void);
GtkWidget *gtk_menu_new_from_model (GMenuModel *model);
void       gtk_menu_popup (GtkMenu *menu, GtkWidget *parent_menu_shell, GtkWidget *parent_menu_item,
                           GtkMenuPositionFunc func, gpointer data, guint button, guint32 activate_time);
void       gtk_menu_popup_at_pointer (GtkMenu *menu, const GdkEvent *trigger_event);
void       gtk_menu_popup_at_widget (GtkMenu *menu, GtkWidget *widget, int widget_anchor, int menu_anchor, const GdkEvent *trigger_event);
void       gtk_menu_popdown (GtkMenu *menu);
void       gtk_menu_attach_to_widget (GtkMenu *menu, GtkWidget *attach_widget, gpointer detacher);
void       gtk_menu_set_accel_group (GtkMenu *menu, GtkAccelGroup *accel_group);
void       gtk_menu_set_title (GtkMenu *menu, const gchar *title);
/* GTK 1/2 spellings */
void       gtk_menu_append (GtkMenu *menu, GtkWidget *child);
void       gtk_menu_prepend (GtkMenu *menu, GtkWidget *child);
void       gtk_menu_bar_append (GtkMenuBar *menu, GtkWidget *child);

GTKQT_SIMPLE_TYPE (GtkMenuItem, GtkBin)
#define GTK_TYPE_MENU_ITEM  (gtk_menu_item_get_type ())
#define GTK_MENU_ITEM(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_MENU_ITEM, GtkMenuItem))
#define GTK_IS_MENU_ITEM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_MENU_ITEM))
GType      gtk_menu_item_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_menu_item_new (void);
GtkWidget *gtk_menu_item_new_with_label (const gchar *label);
GtkWidget *gtk_menu_item_new_with_mnemonic (const gchar *label);
void       gtk_menu_item_set_submenu (GtkMenuItem *menu_item, GtkWidget *submenu);
GtkWidget *gtk_menu_item_get_submenu (GtkMenuItem *menu_item);
void       gtk_menu_item_remove_submenu (GtkMenuItem *menu_item);
void       gtk_menu_item_set_label (GtkMenuItem *menu_item, const gchar *label);
const gchar *gtk_menu_item_get_label (GtkMenuItem *menu_item);
void       gtk_menu_item_set_use_underline (GtkMenuItem *menu_item, gboolean setting);
void       gtk_menu_item_set_right_justified (GtkMenuItem *menu_item, gboolean right_justified);
void       gtk_menu_item_activate (GtkMenuItem *menu_item);
void       gtk_menu_item_set_accel_path (GtkMenuItem *menu_item, const gchar *accel_path);

typedef struct _GtkCheckMenuItem { GtkMenuItem menu_item; gboolean active; } GtkCheckMenuItem;
typedef struct _GtkCheckMenuItemClass { GtkMenuItemClass parent_class; } GtkCheckMenuItemClass;
#define GTK_TYPE_CHECK_MENU_ITEM (gtk_check_menu_item_get_type ())
#define GTK_CHECK_MENU_ITEM(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_CHECK_MENU_ITEM, GtkCheckMenuItem))
#define GTK_IS_CHECK_MENU_ITEM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_CHECK_MENU_ITEM))
GType      gtk_check_menu_item_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_check_menu_item_new (void);
GtkWidget *gtk_check_menu_item_new_with_label (const gchar *label);
GtkWidget *gtk_check_menu_item_new_with_mnemonic (const gchar *label);
void       gtk_check_menu_item_set_active (GtkCheckMenuItem *check_menu_item, gboolean is_active);
gboolean   gtk_check_menu_item_get_active (GtkCheckMenuItem *check_menu_item);
void       gtk_check_menu_item_set_draw_as_radio (GtkCheckMenuItem *check_menu_item, gboolean draw_as_radio);

GTKQT_SIMPLE_TYPE (GtkRadioMenuItem, GtkCheckMenuItem)
#define GTK_TYPE_RADIO_MENU_ITEM (gtk_radio_menu_item_get_type ())
#define GTK_RADIO_MENU_ITEM(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_RADIO_MENU_ITEM, GtkRadioMenuItem))
#define GTK_IS_RADIO_MENU_ITEM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_RADIO_MENU_ITEM))
GType      gtk_radio_menu_item_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_radio_menu_item_new_with_label (GSList *group, const gchar *label);
GtkWidget *gtk_radio_menu_item_new_with_mnemonic (GSList *group, const gchar *label);
GSList    *gtk_radio_menu_item_get_group (GtkRadioMenuItem *radio_menu_item);

GTKQT_SIMPLE_TYPE (GtkSeparatorMenuItem, GtkMenuItem)
#define GTK_TYPE_SEPARATOR_MENU_ITEM (gtk_separator_menu_item_get_type ())
GType      gtk_separator_menu_item_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_separator_menu_item_new (void);

GTKQT_SIMPLE_TYPE (GtkImageMenuItem, GtkMenuItem)
#define GTK_TYPE_IMAGE_MENU_ITEM (gtk_image_menu_item_get_type ())
#define GTK_IMAGE_MENU_ITEM(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_IMAGE_MENU_ITEM, GtkImageMenuItem))
#define GTK_IS_IMAGE_MENU_ITEM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_IMAGE_MENU_ITEM))
GType      gtk_image_menu_item_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_image_menu_item_new_from_stock (const gchar *stock_id, GtkAccelGroup *accel_group);
GtkWidget *gtk_image_menu_item_new_with_label (const gchar *label);
GtkWidget *gtk_image_menu_item_new_with_mnemonic (const gchar *label);
void       gtk_image_menu_item_set_image (GtkImageMenuItem *image_menu_item, GtkWidget *image);

/* GtkOptionMenu (GTK 1/2): a button that pops up a GtkMenu */
GTKQT_SIMPLE_TYPE (GtkOptionMenu, GtkButton)
#define GTK_TYPE_OPTION_MENU (gtk_option_menu_get_type ())
#define GTK_OPTION_MENU(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_OPTION_MENU, GtkOptionMenu))
#define GTK_IS_OPTION_MENU(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_OPTION_MENU))
GType      gtk_option_menu_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_option_menu_new (void);
void       gtk_option_menu_set_menu (GtkOptionMenu *option_menu, GtkWidget *menu);
GtkWidget *gtk_option_menu_get_menu (GtkOptionMenu *option_menu);
void       gtk_option_menu_set_history (GtkOptionMenu *option_menu, guint index_);
gint       gtk_option_menu_get_history (GtkOptionMenu *option_menu);

/* GtkMenuButton / GtkPopoverMenuBar / GtkPopoverMenu (GMenuModel based) */
GTKQT_SIMPLE_TYPE (GtkMenuButton, GtkWidget)
GTKQT_SIMPLE_TYPE (GtkPopoverMenuBar, GtkWidget)
GTKQT_SIMPLE_TYPE (GtkPopover, GtkBin)
GTKQT_SIMPLE_TYPE (GtkPopoverMenu, GtkPopover)
#define GTK_TYPE_MENU_BUTTON      (gtk_menu_button_get_type ())
#define GTK_MENU_BUTTON(o)        (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_MENU_BUTTON, GtkMenuButton))
#define GTK_IS_MENU_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_MENU_BUTTON))
#define GTK_TYPE_POPOVER_MENU_BAR (gtk_popover_menu_bar_get_type ())
#define GTK_POPOVER_MENU_BAR(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_POPOVER_MENU_BAR, GtkPopoverMenuBar))
#define GTK_IS_POPOVER_MENU_BAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_POPOVER_MENU_BAR))
#define GTK_TYPE_POPOVER          (gtk_popover_get_type ())
#define GTK_POPOVER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_POPOVER, GtkPopover))
#define GTK_IS_POPOVER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_POPOVER))
#define GTK_TYPE_POPOVER_MENU     (gtk_popover_menu_get_type ())
#define GTK_POPOVER_MENU(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_POPOVER_MENU, GtkPopoverMenu))
#define GTK_IS_POPOVER_MENU(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_POPOVER_MENU))
GType      gtk_menu_button_get_type (void) G_GNUC_CONST;
GType      gtk_popover_menu_bar_get_type (void) G_GNUC_CONST;
GType      gtk_popover_get_type (void) G_GNUC_CONST;
GType      gtk_popover_menu_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_menu_button_new (void);
void       gtk_menu_button_set_menu_model (GtkMenuButton *menu_button, GMenuModel *menu_model);
void       gtk_menu_button_set_popup (GtkMenuButton *menu_button, GtkWidget *menu);
void       gtk_menu_button_set_popover (GtkMenuButton *menu_button, GtkWidget *popover);
void       gtk_menu_button_set_icon_name (GtkMenuButton *menu_button, const char *icon_name);
void       gtk_menu_button_set_label (GtkMenuButton *menu_button, const char *label);
GtkWidget *gtk_popover_menu_bar_new_from_model (GMenuModel *model);
GtkWidget *gtk_popover_menu_new_from_model (GMenuModel *model);
GtkWidget *gtk_popover_new (GtkWidget *relative_to);   /* GTK 4: no args, pass NULL */
void       gtk_popover_set_child (GtkPopover *popover, GtkWidget *child);
void       gtk_popover_popup (GtkPopover *popover);
void       gtk_popover_popdown (GtkPopover *popover);
void       gtk_popover_set_relative_to (GtkPopover *popover, GtkWidget *relative_to);

/* GtkToolbar (GTK 2/3) */
GTKQT_SIMPLE_TYPE (GtkToolbar, GtkContainer)
GTKQT_SIMPLE_TYPE (GtkToolItem, GtkBin)
GTKQT_SIMPLE_TYPE (GtkToolButton, GtkToolItem)
GTKQT_SIMPLE_TYPE (GtkToggleToolButton, GtkToolButton)
GTKQT_SIMPLE_TYPE (GtkSeparatorToolItem, GtkToolItem)
#define GTK_TYPE_TOOLBAR      (gtk_toolbar_get_type ())
#define GTK_TOOLBAR(o)        (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TOOLBAR, GtkToolbar))
#define GTK_IS_TOOLBAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TOOLBAR))
#define GTK_TYPE_TOOL_ITEM    (gtk_tool_item_get_type ())
#define GTK_TOOL_ITEM(o)      (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TOOL_ITEM, GtkToolItem))
#define GTK_IS_TOOL_ITEM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TOOL_ITEM))
#define GTK_TYPE_TOOL_BUTTON  (gtk_tool_button_get_type ())
#define GTK_TOOL_BUTTON(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TOOL_BUTTON, GtkToolButton))
#define GTK_IS_TOOL_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TOOL_BUTTON))
#define GTK_TYPE_TOGGLE_TOOL_BUTTON (gtk_toggle_tool_button_get_type ())
#define GTK_TOGGLE_TOOL_BUTTON(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TOGGLE_TOOL_BUTTON, GtkToggleToolButton))
#define GTK_IS_TOGGLE_TOOL_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TOGGLE_TOOL_BUTTON))
#define GTK_TYPE_SEPARATOR_TOOL_ITEM (gtk_separator_tool_item_get_type ())
GType        gtk_toolbar_get_type (void) G_GNUC_CONST;
GType        gtk_tool_item_get_type (void) G_GNUC_CONST;
GType        gtk_tool_button_get_type (void) G_GNUC_CONST;
GType        gtk_toggle_tool_button_get_type (void) G_GNUC_CONST;
GType        gtk_separator_tool_item_get_type (void) G_GNUC_CONST;
GtkWidget   *gtk_toolbar_new (void);
void         gtk_toolbar_insert (GtkToolbar *toolbar, GtkToolItem *item, gint pos);
void         gtk_toolbar_set_style (GtkToolbar *toolbar, GtkToolbarStyle style);
void         gtk_toolbar_set_icon_size (GtkToolbar *toolbar, GtkIconSize icon_size);
gint         gtk_toolbar_get_n_items (GtkToolbar *toolbar);
GtkToolItem *gtk_tool_item_new (void);
GtkToolItem *gtk_tool_button_new (GtkWidget *icon_widget, const gchar *label);
GtkToolItem *gtk_tool_button_new_from_stock (const gchar *stock_id);
void         gtk_tool_button_set_icon_name (GtkToolButton *button, const gchar *icon_name);
void         gtk_tool_button_set_label (GtkToolButton *button, const gchar *label);
GtkToolItem *gtk_toggle_tool_button_new (void);
GtkToolItem *gtk_toggle_tool_button_new_from_stock (const gchar *stock_id);
void         gtk_toggle_tool_button_set_active (GtkToggleToolButton *button, gboolean is_active);
gboolean     gtk_toggle_tool_button_get_active (GtkToggleToolButton *button);
GtkToolItem *gtk_separator_tool_item_new (void);
void         gtk_tool_item_set_tooltip_text (GtkToolItem *tool_item, const gchar *text);
void         gtk_tool_item_set_expand (GtkToolItem *tool_item, gboolean expand);
void         gtk_tool_item_set_is_important (GtkToolItem *tool_item, gboolean is_important);

/* GtkAction / GtkActionGroup / GtkUIManager (GTK 2.4 - 3.x) */
typedef struct _GtkAction GtkAction;
typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _GtkUIManager GtkUIManager;
#define GTK_TYPE_ACTION        (gtk_action_get_type ())
#define GTK_ACTION(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ACTION, GtkAction))
#define GTK_IS_ACTION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ACTION))
#define GTK_TYPE_TOGGLE_ACTION (gtk_toggle_action_get_type ())
#define GTK_TOGGLE_ACTION(o)   ((GtkToggleAction *) (o))
#define GTK_TYPE_ACTION_GROUP  (gtk_action_group_get_type ())
#define GTK_ACTION_GROUP(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ACTION_GROUP, GtkActionGroup))
#define GTK_IS_ACTION_GROUP(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ACTION_GROUP))
#define GTK_TYPE_UI_MANAGER    (gtk_ui_manager_get_type ())
#define GTK_UI_MANAGER(o)      (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_UI_MANAGER, GtkUIManager))
#define GTK_IS_UI_MANAGER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_UI_MANAGER))
typedef GtkAction GtkToggleAction;
typedef GtkAction GtkRadioAction;
typedef struct {
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  GCallback    callback;
} GtkActionEntry;
typedef struct {
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  GCallback    callback;
  gboolean     is_active;
} GtkToggleActionEntry;
typedef struct {
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  gint         value;
} GtkRadioActionEntry;
GType           gtk_action_get_type (void) G_GNUC_CONST;
GType           gtk_toggle_action_get_type (void) G_GNUC_CONST;
GType           gtk_action_group_get_type (void) G_GNUC_CONST;
GType           gtk_ui_manager_get_type (void) G_GNUC_CONST;
GtkAction      *gtk_action_new (const gchar *name, const gchar *label, const gchar *tooltip, const gchar *stock_id);
const gchar    *gtk_action_get_name (GtkAction *action);
void            gtk_action_activate (GtkAction *action);
void            gtk_action_set_sensitive (GtkAction *action, gboolean sensitive);
void            gtk_action_set_visible (GtkAction *action, gboolean visible);
gboolean        gtk_toggle_action_get_active (GtkToggleAction *action);
void            gtk_toggle_action_set_active (GtkToggleAction *action, gboolean is_active);
GtkActionGroup *gtk_action_group_new (const gchar *name);
void            gtk_action_group_add_action (GtkActionGroup *action_group, GtkAction *action);
void            gtk_action_group_add_action_with_accel (GtkActionGroup *action_group, GtkAction *action, const gchar *accelerator);
void            gtk_action_group_add_actions (GtkActionGroup *action_group, const GtkActionEntry *entries, guint n_entries, gpointer user_data);
void            gtk_action_group_add_toggle_actions (GtkActionGroup *action_group, const GtkToggleActionEntry *entries, guint n_entries, gpointer user_data);
void            gtk_action_group_add_radio_actions (GtkActionGroup *action_group, const GtkRadioActionEntry *entries, guint n_entries,
                                                    gint value, GCallback on_change, gpointer user_data);
GtkAction      *gtk_action_group_get_action (GtkActionGroup *action_group, const gchar *action_name);
void            gtk_action_group_set_translation_domain (GtkActionGroup *action_group, const gchar *domain);
GtkUIManager   *gtk_ui_manager_new (void);
void            gtk_ui_manager_insert_action_group (GtkUIManager *manager, GtkActionGroup *action_group, gint pos);
guint           gtk_ui_manager_add_ui_from_string (GtkUIManager *manager, const gchar *buffer, gssize length, GError **error);
guint           gtk_ui_manager_add_ui_from_file (GtkUIManager *manager, const gchar *filename, GError **error);
GtkWidget      *gtk_ui_manager_get_widget (GtkUIManager *manager, const gchar *path);
GtkAction      *gtk_ui_manager_get_action (GtkUIManager *manager, const gchar *path);
GtkAccelGroup  *gtk_ui_manager_get_accel_group (GtkUIManager *manager);
void            gtk_ui_manager_ensure_update (GtkUIManager *manager);

G_END_DECLS

#endif
