/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Basic controls.
 */
#ifndef GTKQT_CONTROLS_H
#define GTKQT_CONTROLS_H

#include <gtk/gtkqt-containers.h>

G_BEGIN_DECLS

/* GtkMisc (GTK 2/3): alignment/padding helpers for labels and images */
typedef GtkWidget GtkMisc;
#define GTK_MISC(o) ((GtkMisc *) (o))
void gtk_misc_set_alignment (GtkMisc *misc, gfloat xalign, gfloat yalign);
void gtk_misc_set_padding (GtkMisc *misc, gint xpad, gint ypad);

/* GtkLabel */
GTKQT_SIMPLE_TYPE (GtkLabel, GtkWidget)
#define GTK_TYPE_LABEL  (gtk_label_get_type ())
#define GTK_LABEL(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_LABEL, GtkLabel))
#define GTK_IS_LABEL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_LABEL))
GType        gtk_label_get_type (void) G_GNUC_CONST;
GtkWidget   *gtk_label_new (const gchar *str);
GtkWidget   *gtk_label_new_with_mnemonic (const gchar *str);
void         gtk_label_set_text (GtkLabel *label, const gchar *str);
const gchar *gtk_label_get_text (GtkLabel *label);
void         gtk_label_set_label (GtkLabel *label, const gchar *str);
const gchar *gtk_label_get_label (GtkLabel *label);
void         gtk_label_set_markup (GtkLabel *label, const gchar *str);
void         gtk_label_set_markup_with_mnemonic (GtkLabel *label, const gchar *str);
void         gtk_label_set_text_with_mnemonic (GtkLabel *label, const gchar *str);
void         gtk_label_set_use_markup (GtkLabel *label, gboolean setting);
void         gtk_label_set_use_underline (GtkLabel *label, gboolean setting);
void         gtk_label_set_mnemonic_widget (GtkLabel *label, GtkWidget *widget);
void         gtk_label_set_justify (GtkLabel *label, GtkJustification jtype);
void         gtk_label_set_xalign (GtkLabel *label, gfloat xalign);
void         gtk_label_set_yalign (GtkLabel *label, gfloat yalign);
gfloat       gtk_label_get_xalign (GtkLabel *label);
void         gtk_label_set_line_wrap (GtkLabel *label, gboolean wrap);
void         gtk_label_set_wrap (GtkLabel *label, gboolean wrap);
void         gtk_label_set_wrap_mode (GtkLabel *label, PangoWrapMode wrap_mode);
void         gtk_label_set_selectable (GtkLabel *label, gboolean setting);
void         gtk_label_set_ellipsize (GtkLabel *label, PangoEllipsizeMode mode);
void         gtk_label_set_width_chars (GtkLabel *label, gint n_chars);
void         gtk_label_set_max_width_chars (GtkLabel *label, gint n_chars);
void         gtk_label_set_pattern (GtkLabel *label, const gchar *pattern);

/* GtkButton family. In GTK 2/3 a button is a GtkBin; any child widget works. */
typedef struct _GtkButton { GtkBin bin; } GtkButton;
typedef struct _GtkButtonClass {
  GtkBinClass parent_class;
  void (*pressed)  (GtkButton *button);
  void (*released) (GtkButton *button);
  void (*clicked)  (GtkButton *button);
  void (*enter)    (GtkButton *button);
  void (*leave)    (GtkButton *button);
  void (*activate) (GtkButton *button);
} GtkButtonClass;
#define GTK_TYPE_BUTTON  (gtk_button_get_type ())
#define GTK_BUTTON(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_BUTTON, GtkButton))
#define GTK_BUTTON_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GTK_TYPE_BUTTON, GtkButtonClass))
#define GTK_BUTTON_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), GTK_TYPE_BUTTON, GtkButtonClass))
#define GTK_IS_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_BUTTON))
GType        gtk_button_get_type (void) G_GNUC_CONST;
GtkWidget   *gtk_button_new (void);
GtkWidget   *gtk_button_new_with_label (const gchar *label);
GtkWidget   *gtk_button_new_with_mnemonic (const gchar *label);
GtkWidget   *gtk_button_new_from_stock (const gchar *stock_id);
GtkWidget   *gtk_button_new_from_icon_name (const gchar *icon_name, GtkIconSize size);  /* GTK 4: one arg */
void         gtk_button_set_label (GtkButton *button, const gchar *label);
const gchar *gtk_button_get_label (GtkButton *button);
void         gtk_button_set_use_underline (GtkButton *button, gboolean use_underline);
void         gtk_button_set_use_stock (GtkButton *button, gboolean use_stock);
void         gtk_button_set_icon_name (GtkButton *button, const gchar *icon_name);
void         gtk_button_set_image (GtkButton *button, GtkWidget *image);
void         gtk_button_set_always_show_image (GtkButton *button, gboolean always_show);
void         gtk_button_set_relief (GtkButton *button, GtkReliefStyle relief);
void         gtk_button_set_has_frame (GtkButton *button, gboolean has_frame);
void         gtk_button_set_child (GtkButton *button, GtkWidget *child);
GtkWidget   *gtk_button_get_child (GtkButton *button);
void         gtk_button_clicked (GtkButton *button);
void         gtk_button_pressed (GtkButton *button);
void         gtk_button_released (GtkButton *button);

/* GtkActionable (buttons with an "action-name", GTK 3/4) */
typedef struct _GtkActionable GtkActionable;
#define GTK_ACTIONABLE(o) ((GtkActionable *) (o))
void gtk_actionable_set_action_name (GtkActionable *actionable, const gchar *action_name);
const gchar *gtk_actionable_get_action_name (GtkActionable *actionable);
void gtk_actionable_set_action_target_value (GtkActionable *actionable, GVariant *target_value);
void gtk_actionable_set_detailed_action_name (GtkActionable *actionable, const gchar *detailed_action_name);

typedef struct _GtkToggleButton { GtkButton button; gboolean active; } GtkToggleButton;
typedef struct _GtkToggleButtonClass { GtkButtonClass parent_class; void (*toggled) (GtkToggleButton *b); } GtkToggleButtonClass;
#define GTK_TYPE_TOGGLE_BUTTON  (gtk_toggle_button_get_type ())
#define GTK_TOGGLE_BUTTON(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_TOGGLE_BUTTON, GtkToggleButton))
#define GTK_IS_TOGGLE_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_TOGGLE_BUTTON))
GType      gtk_toggle_button_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_toggle_button_new (void);
GtkWidget *gtk_toggle_button_new_with_label (const gchar *label);
GtkWidget *gtk_toggle_button_new_with_mnemonic (const gchar *label);
void       gtk_toggle_button_set_active (GtkToggleButton *toggle_button, gboolean is_active);
gboolean   gtk_toggle_button_get_active (GtkToggleButton *toggle_button);
void       gtk_toggle_button_toggled (GtkToggleButton *toggle_button);
void       gtk_toggle_button_set_mode (GtkToggleButton *toggle_button, gboolean draw_indicator);
void       gtk_toggle_button_set_inconsistent (GtkToggleButton *toggle_button, gboolean setting);
void       gtk_toggle_button_set_group (GtkToggleButton *toggle_button, GtkToggleButton *group);

GTKQT_SIMPLE_TYPE (GtkCheckButton, GtkToggleButton)
#define GTK_TYPE_CHECK_BUTTON  (gtk_check_button_get_type ())
#define GTK_CHECK_BUTTON(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_CHECK_BUTTON, GtkCheckButton))
#define GTK_IS_CHECK_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_CHECK_BUTTON))
GType      gtk_check_button_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_check_button_new (void);
GtkWidget *gtk_check_button_new_with_label (const gchar *label);
GtkWidget *gtk_check_button_new_with_mnemonic (const gchar *label);
void       gtk_check_button_set_active (GtkCheckButton *self, gboolean setting);
gboolean   gtk_check_button_get_active (GtkCheckButton *self);
void       gtk_check_button_set_label (GtkCheckButton *self, const char *label);
void       gtk_check_button_set_group (GtkCheckButton *self, GtkCheckButton *group);   /* GTK 4 radio groups */
void       gtk_check_button_set_inconsistent (GtkCheckButton *self, gboolean inconsistent);

GTKQT_SIMPLE_TYPE (GtkRadioButton, GtkCheckButton)
#define GTK_TYPE_RADIO_BUTTON  (gtk_radio_button_get_type ())
#define GTK_RADIO_BUTTON(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_RADIO_BUTTON, GtkRadioButton))
#define GTK_IS_RADIO_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_RADIO_BUTTON))
GType      gtk_radio_button_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_radio_button_new (GSList *group);
GtkWidget *gtk_radio_button_new_with_label (GSList *group, const gchar *label);
GtkWidget *gtk_radio_button_new_with_mnemonic (GSList *group, const gchar *label);
GtkWidget *gtk_radio_button_new_from_widget (GtkRadioButton *radio_group_member);
GtkWidget *gtk_radio_button_new_with_label_from_widget (GtkRadioButton *radio_group_member, const gchar *label);
GtkWidget *gtk_radio_button_new_with_mnemonic_from_widget (GtkRadioButton *radio_group_member, const gchar *label);
GSList    *gtk_radio_button_get_group (GtkRadioButton *radio_button);
void       gtk_radio_button_set_group (GtkRadioButton *radio_button, GSList *group);
void       gtk_radio_button_join_group (GtkRadioButton *radio_button, GtkRadioButton *group_source);

GTKQT_SIMPLE_TYPE (GtkLinkButton, GtkButton)
#define GTK_TYPE_LINK_BUTTON (gtk_link_button_get_type ())
#define GTK_LINK_BUTTON(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_LINK_BUTTON, GtkLinkButton))
#define GTK_IS_LINK_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_LINK_BUTTON))
GType      gtk_link_button_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_link_button_new (const gchar *uri);
GtkWidget *gtk_link_button_new_with_label (const gchar *uri, const gchar *label);

/* GtkSwitch (GTK 3/4) */
GTKQT_SIMPLE_TYPE (GtkSwitch, GtkWidget)
#define GTK_TYPE_SWITCH (gtk_switch_get_type ())
#define GTK_SWITCH(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_SWITCH, GtkSwitch))
#define GTK_IS_SWITCH(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_SWITCH))
GType      gtk_switch_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_switch_new (void);
void       gtk_switch_set_active (GtkSwitch *self, gboolean is_active);
gboolean   gtk_switch_get_active (GtkSwitch *self);
void       gtk_switch_set_state (GtkSwitch *self, gboolean state);
gboolean   gtk_switch_get_state (GtkSwitch *self);

/* GtkEditable: implemented by GtkEntry and GtkSpinButton */
typedef struct _GtkEditable GtkEditable;
#define GTK_EDITABLE(o)    ((GtkEditable *) (o))
/* GTK 1's public GtkEditable fields (the start of a GtkEntry), kept current */
struct _GtkEditable {
  GtkWidget widget;
  guint current_pos;
  guint selection_start_pos, selection_end_pos;
  guint has_selection : 1, editable : 1, visible : 1;
  gpointer ic, ic_attr;
  gchar *clipboard_text;
};
typedef void (*GtkTextFunction) (GtkEditable *editable, guint32 time_);
#define GTK_IS_EDITABLE(o) (GTK_IS_ENTRY (o))
const char *gtk_editable_get_text (GtkEditable *editable);
void        gtk_editable_set_text (GtkEditable *editable, const char *text);
gchar      *gtk_editable_get_chars (GtkEditable *editable, gint start_pos, gint end_pos);
void        gtk_editable_insert_text (GtkEditable *editable, const gchar *new_text, gint new_text_length, gint *position);
void        gtk_editable_delete_text (GtkEditable *editable, gint start_pos, gint end_pos);
void        gtk_editable_select_region (GtkEditable *editable, gint start_pos, gint end_pos);
gboolean    gtk_editable_get_selection_bounds (GtkEditable *editable, gint *start_pos, gint *end_pos);
void        gtk_editable_set_position (GtkEditable *editable, gint position);
gint        gtk_editable_get_position (GtkEditable *editable);
void        gtk_editable_set_editable (GtkEditable *editable, gboolean is_editable);
gboolean    gtk_editable_get_editable (GtkEditable *editable);
void        gtk_editable_cut_clipboard (GtkEditable *editable);
void        gtk_editable_copy_clipboard (GtkEditable *editable);
void        gtk_editable_paste_clipboard (GtkEditable *editable);
void        gtk_editable_delete_selection (GtkEditable *editable);
void        gtk_editable_set_width_chars (GtkEditable *editable, gint n_chars);
void        gtk_editable_set_alignment (GtkEditable *editable, gfloat xalign);

/* GtkEntry + GtkEntryBuffer */
/* GTK 1/2 public fields (text as wide characters, as in GTK 1.2) */
typedef struct _GtkEntry {
  GtkEditable editable;
  GdkWindow *text_area;
  gpointer backing_pixmap, cursor;
  guint32 *text;
  guint16 text_size, text_length, text_max_length;
  gint scroll_offset;
  guint visible : 1;
  guint32 timer;
  guint button;
  gint *char_offset;
  gchar *text_mb;
  guint text_mb_dirty : 1, use_wchar : 1;
} GtkEntry;
typedef struct _GtkEntryClass { GtkWidgetClass parent_class; } GtkEntryClass;
#define GTK_TYPE_ENTRY  (gtk_entry_get_type ())
#define GTK_ENTRY(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ENTRY, GtkEntry))
#define GTK_IS_ENTRY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ENTRY))
typedef struct _GtkEntryBuffer GtkEntryBuffer;
#define GTK_ENTRY_BUFFER(o) ((GtkEntryBuffer *) (o))
GType        gtk_entry_get_type (void) G_GNUC_CONST;
GtkWidget   *gtk_entry_new (void);
GtkWidget   *gtk_entry_new_with_max_length (gint max);
void         gtk_entry_set_text (GtkEntry *entry, const gchar *text);
const gchar *gtk_entry_get_text (GtkEntry *entry);
guint16      gtk_entry_get_text_length (GtkEntry *entry);
void         gtk_entry_append_text (GtkEntry *entry, const gchar *text);
void         gtk_entry_prepend_text (GtkEntry *entry, const gchar *text);
void         gtk_entry_set_editable (GtkEntry *entry, gboolean editable);
void         gtk_entry_set_visibility (GtkEntry *entry, gboolean visible);
gboolean     gtk_entry_get_visibility (GtkEntry *entry);
void         gtk_entry_set_invisible_char (GtkEntry *entry, gunichar ch);
void         gtk_entry_set_max_length (GtkEntry *entry, gint max);
gint         gtk_entry_get_max_length (GtkEntry *entry);
void         gtk_entry_set_width_chars (GtkEntry *entry, gint n_chars);
void         gtk_entry_set_activates_default (GtkEntry *entry, gboolean setting);
void         gtk_entry_set_placeholder_text (GtkEntry *entry, const gchar *text);
void         gtk_entry_set_alignment (GtkEntry *entry, gfloat xalign);
void         gtk_entry_set_has_frame (GtkEntry *entry, gboolean setting);
void         gtk_entry_set_input_purpose (GtkEntry *entry, GtkInputPurpose purpose);
void         gtk_entry_set_icon_from_icon_name (GtkEntry *entry, GtkEntryIconPosition icon_pos, const gchar *icon_name);
void         gtk_entry_select_region (GtkEntry *entry, gint start, gint end);
void         gtk_entry_set_position (GtkEntry *entry, gint position);
GtkEntryBuffer *gtk_entry_get_buffer (GtkEntry *entry);
const char  *gtk_entry_buffer_get_text (GtkEntryBuffer *buffer);
void         gtk_entry_buffer_set_text (GtkEntryBuffer *buffer, const char *chars, int n_chars);
guint        gtk_entry_buffer_get_length (GtkEntryBuffer *buffer);

/* GtkComboBox / GtkComboBoxText (+ GTK 2 text-combo functions) */
GTKQT_SIMPLE_TYPE (GtkComboBox, GtkBin)
GTKQT_SIMPLE_TYPE (GtkComboBoxText, GtkComboBox)
#define GTK_TYPE_COMBO_BOX       (gtk_combo_box_get_type ())
#define GTK_COMBO_BOX(o)         (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_COMBO_BOX, GtkComboBox))
#define GTK_IS_COMBO_BOX(o)      (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_COMBO_BOX))
#define GTK_TYPE_COMBO_BOX_TEXT  (gtk_combo_box_text_get_type ())
#define GTK_COMBO_BOX_TEXT(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_COMBO_BOX_TEXT, GtkComboBoxText))
#define GTK_IS_COMBO_BOX_TEXT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_COMBO_BOX_TEXT))
GType      gtk_combo_box_get_type (void) G_GNUC_CONST;
GType      gtk_combo_box_text_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_combo_box_new (void);
GtkWidget *gtk_combo_box_new_text (void);
GtkWidget *gtk_combo_box_new_with_entry (void);
GtkWidget *gtk_combo_box_text_new (void);
GtkWidget *gtk_combo_box_text_new_with_entry (void);
void       gtk_combo_box_append_text (GtkComboBox *combo_box, const gchar *text);
void       gtk_combo_box_prepend_text (GtkComboBox *combo_box, const gchar *text);
void       gtk_combo_box_insert_text (GtkComboBox *combo_box, gint position, const gchar *text);
void       gtk_combo_box_remove_text (GtkComboBox *combo_box, gint position);
gchar     *gtk_combo_box_get_active_text (GtkComboBox *combo_box);
void       gtk_combo_box_text_append_text (GtkComboBoxText *combo_box, const gchar *text);
void       gtk_combo_box_text_prepend_text (GtkComboBoxText *combo_box, const gchar *text);
void       gtk_combo_box_text_insert_text (GtkComboBoxText *combo_box, gint position, const gchar *text);
void       gtk_combo_box_text_append (GtkComboBoxText *combo_box, const gchar *id, const gchar *text);
void       gtk_combo_box_text_prepend (GtkComboBoxText *combo_box, const gchar *id, const gchar *text);
void       gtk_combo_box_text_insert (GtkComboBoxText *combo_box, gint position, const gchar *id, const gchar *text);
void       gtk_combo_box_text_remove (GtkComboBoxText *combo_box, gint position);
void       gtk_combo_box_text_remove_all (GtkComboBoxText *combo_box);
gchar     *gtk_combo_box_text_get_active_text (GtkComboBoxText *combo_box);
void       gtk_combo_box_set_active (GtkComboBox *combo_box, gint index_);
gint       gtk_combo_box_get_active (GtkComboBox *combo_box);
const gchar *gtk_combo_box_get_active_id (GtkComboBox *combo_box);
gboolean   gtk_combo_box_set_active_id (GtkComboBox *combo_box, const gchar *active_id);
void       gtk_combo_box_popup (GtkComboBox *combo_box);

/* GtkCombo (GTK 1/2): an entry with a drop-down list */
typedef struct _GtkCombo { GtkBox box; GtkWidget *entry; GtkWidget *list; } GtkCombo;
typedef struct _GtkComboClass { GtkBoxClass parent_class; } GtkComboClass;
#define GTK_TYPE_COMBO (gtk_combo_get_type ())
#define GTK_COMBO(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_COMBO, GtkCombo))
#define GTK_IS_COMBO(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_COMBO))
GType      gtk_combo_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_combo_new (void);
void       gtk_combo_set_popdown_strings (GtkCombo *combo, GList *strings);
void       gtk_combo_set_value_in_list (GtkCombo *combo, gboolean val, gboolean ok_if_empty);
void       gtk_combo_set_item_string (GtkCombo *combo, GtkItem *item, const gchar *item_value);

/* GtkDropDown + GtkStringList (GTK 4) */
GTKQT_SIMPLE_TYPE (GtkDropDown, GtkWidget)
#define GTK_TYPE_DROP_DOWN (gtk_drop_down_get_type ())
#define GTK_DROP_DOWN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_DROP_DOWN, GtkDropDown))
#define GTK_IS_DROP_DOWN(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_DROP_DOWN))
#define GTK_INVALID_LIST_POSITION ((guint) 0xffffffff)
typedef struct _GtkStringList GtkStringList;
typedef struct _GtkStringObject GtkStringObject;
#define GTK_TYPE_STRING_LIST   (gtk_string_list_get_type ())
#define GTK_STRING_LIST(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_STRING_LIST, GtkStringList))
#define GTK_IS_STRING_LIST(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_STRING_LIST))
#define GTK_TYPE_STRING_OBJECT (gtk_string_object_get_type ())
#define GTK_STRING_OBJECT(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_STRING_OBJECT, GtkStringObject))
#define GTK_IS_STRING_OBJECT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_STRING_OBJECT))
GType      gtk_drop_down_get_type (void) G_GNUC_CONST;
GType      gtk_string_list_get_type (void) G_GNUC_CONST;
GType      gtk_string_object_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_drop_down_new (GListModel *model, gpointer expression);
GtkWidget *gtk_drop_down_new_from_strings (const char * const *strings);
guint      gtk_drop_down_get_selected (GtkDropDown *self);
void       gtk_drop_down_set_selected (GtkDropDown *self, guint position);
gpointer   gtk_drop_down_get_selected_item (GtkDropDown *self);
GListModel *gtk_drop_down_get_model (GtkDropDown *self);
GtkStringList *gtk_string_list_new (const char * const *strings);
void        gtk_string_list_append (GtkStringList *self, const char *string);
void        gtk_string_list_remove (GtkStringList *self, guint position);
const char *gtk_string_list_get_string (GtkStringList *self, guint position);
const char *gtk_string_object_get_string (GtkStringObject *self);

/* GtkImage */
GTKQT_SIMPLE_TYPE (GtkImage, GtkWidget)
#define GTK_TYPE_IMAGE (gtk_image_get_type ())
#define GTK_IMAGE(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_IMAGE, GtkImage))
#define GTK_IS_IMAGE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_IMAGE))
GType      gtk_image_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_image_new (void);
GtkWidget *gtk_image_new_from_file (const gchar *filename);
GtkWidget *gtk_image_new_from_icon_name (const gchar *icon_name, GtkIconSize size);  /* GTK 4: one arg */
GtkWidget *gtk_image_new_from_stock (const gchar *stock_id, GtkIconSize size);
GtkWidget *gtk_image_new_from_pixbuf (GdkPixbuf *pixbuf);
void       gtk_image_set_from_file (GtkImage *image, const gchar *filename);
void       gtk_image_set_from_icon_name (GtkImage *image, const gchar *icon_name, GtkIconSize size);
void       gtk_image_set_from_pixbuf (GtkImage *image, GdkPixbuf *pixbuf);
void       gtk_image_set_pixel_size (GtkImage *image, gint pixel_size);
void       gtk_image_clear (GtkImage *image);
GtkWidget *gtk_picture_new_for_filename (const char *filename);

/* GtkSpinner */
GTKQT_SIMPLE_TYPE (GtkSpinner, GtkWidget)
#define GTK_TYPE_SPINNER (gtk_spinner_get_type ())
#define GTK_SPINNER(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_SPINNER, GtkSpinner))
#define GTK_IS_SPINNER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_SPINNER))
GType      gtk_spinner_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_spinner_new (void);
void       gtk_spinner_start (GtkSpinner *spinner);
void       gtk_spinner_stop (GtkSpinner *spinner);
void       gtk_spinner_set_spinning (GtkSpinner *spinner, gboolean spinning);

/* GtkDrawingArea: drawn through cairo ("expose-event" in GTK 2, "draw"
 * in GTK 3, a draw function in GTK 4) */
GTKQT_SIMPLE_TYPE (GtkDrawingArea, GtkWidget)
#define GTK_TYPE_DRAWING_AREA  (gtk_drawing_area_get_type ())
#define GTK_DRAWING_AREA(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_DRAWING_AREA, GtkDrawingArea))
#define GTK_IS_DRAWING_AREA(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_DRAWING_AREA))
typedef void (*GtkDrawingAreaDrawFunc) (GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer user_data);
GType      gtk_drawing_area_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_drawing_area_new (void);
void       gtk_drawing_area_size (GtkDrawingArea *darea, gint width, gint height);
void       gtk_drawing_area_set_draw_func (GtkDrawingArea *self, GtkDrawingAreaDrawFunc draw_func, gpointer user_data, GDestroyNotify destroy);
void       gtk_drawing_area_set_content_width (GtkDrawingArea *self, int width);
void       gtk_drawing_area_set_content_height (GtkDrawingArea *self, int height);

/* GTK 4 event controllers */
struct _GtkEventController { GObject parent_instance; gpointer priv; };
typedef struct _GtkEventControllerClass { GObjectClass parent_class; } GtkEventControllerClass;
typedef GtkEventController GtkGesture;
typedef GtkEventController GtkGestureSingle;
typedef GtkEventController GtkGestureClick;
typedef GtkEventController GtkGestureDrag;
typedef GtkEventController GtkEventControllerKey;
typedef GtkEventController GtkEventControllerMotion;
typedef GtkEventController GtkEventControllerScroll;
typedef GtkEventController GtkEventControllerFocus;
typedef enum {
  GTK_EVENT_CONTROLLER_SCROLL_NONE = 0, GTK_EVENT_CONTROLLER_SCROLL_VERTICAL = 1 << 0,
  GTK_EVENT_CONTROLLER_SCROLL_HORIZONTAL = 1 << 1, GTK_EVENT_CONTROLLER_SCROLL_DISCRETE = 1 << 2,
  GTK_EVENT_CONTROLLER_SCROLL_KINETIC = 1 << 3, GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES = 3
} GtkEventControllerScrollFlags;
#define GTK_TYPE_EVENT_CONTROLLER (gtk_event_controller_get_type ())
#define GTK_EVENT_CONTROLLER(o)   ((GtkEventController *) (o))
#define GTK_GESTURE(o)            ((GtkGesture *) (o))
#define GTK_GESTURE_SINGLE(o)     ((GtkGestureSingle *) (o))
#define GTK_GESTURE_CLICK(o)      ((GtkGestureClick *) (o))
#define GTK_GESTURE_DRAG(o)       ((GtkGestureDrag *) (o))
GType gtk_event_controller_get_type (void) G_GNUC_CONST;
GtkGesture *gtk_gesture_click_new (void);
GtkGesture *gtk_gesture_drag_new (void);
GtkEventController *gtk_event_controller_key_new (void);
GtkEventController *gtk_event_controller_motion_new (void);
GtkEventController *gtk_event_controller_scroll_new (GtkEventControllerScrollFlags flags);
GtkEventController *gtk_event_controller_focus_new (void);
GtkWidget *gtk_event_controller_get_widget (GtkEventController *controller);
void       gtk_gesture_single_set_button (GtkGestureSingle *gesture, guint button);
guint      gtk_gesture_single_get_current_button (GtkGestureSingle *gesture);
gboolean   gtk_gesture_drag_get_start_point (GtkGestureDrag *gesture, double *x, double *y);
gboolean   gtk_gesture_drag_get_offset (GtkGestureDrag *gesture, double *x, double *y);

/* GtkArrow (GTK 2/3): an arrow drawn by the Qt style */
typedef struct _GtkArrow { GtkWidget widget; GtkArrowType arrow_type; GtkShadowType shadow_type; } GtkArrow;
typedef struct _GtkArrowClass { GtkWidgetClass parent_class; } GtkArrowClass;
#define GTK_TYPE_ARROW   (gtk_arrow_get_type ())
#define GTK_ARROW(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ARROW, GtkArrow))
#define GTK_IS_ARROW(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ARROW))
GType      gtk_arrow_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_arrow_new (GtkArrowType arrow_type, GtkShadowType shadow_type);
void       gtk_arrow_set (GtkArrow *arrow, GtkArrowType arrow_type, GtkShadowType shadow_type);

G_END_DECLS

#endif
