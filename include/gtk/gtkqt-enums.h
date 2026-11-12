/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Enumerations shared by all GTK generations. Values match real GTK.
 */
#ifndef GTKQT_ENUMS_H
#define GTKQT_ENUMS_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum { GTK_ORIENTATION_HORIZONTAL, GTK_ORIENTATION_VERTICAL } GtkOrientation;
typedef enum { GTK_ALIGN_FILL, GTK_ALIGN_START, GTK_ALIGN_END, GTK_ALIGN_CENTER, GTK_ALIGN_BASELINE } GtkAlign;
typedef enum { GTK_PACK_START, GTK_PACK_END } GtkPackType;
typedef enum { GTK_POS_LEFT, GTK_POS_RIGHT, GTK_POS_TOP, GTK_POS_BOTTOM } GtkPositionType;
typedef enum { GTK_POLICY_ALWAYS, GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL } GtkPolicyType;
typedef enum { GTK_SHADOW_NONE, GTK_SHADOW_IN, GTK_SHADOW_OUT, GTK_SHADOW_ETCHED_IN, GTK_SHADOW_ETCHED_OUT } GtkShadowType;
typedef enum { GTK_JUSTIFY_LEFT, GTK_JUSTIFY_RIGHT, GTK_JUSTIFY_CENTER, GTK_JUSTIFY_FILL } GtkJustification;
typedef enum { GTK_WRAP_NONE, GTK_WRAP_CHAR, GTK_WRAP_WORD, GTK_WRAP_WORD_CHAR } GtkWrapMode;
typedef enum { GTK_WINDOW_TOPLEVEL, GTK_WINDOW_POPUP } GtkWindowType;
typedef enum {
  GTK_WIN_POS_NONE, GTK_WIN_POS_CENTER, GTK_WIN_POS_MOUSE,
  GTK_WIN_POS_CENTER_ALWAYS, GTK_WIN_POS_CENTER_ON_PARENT
} GtkWindowPosition;
typedef enum {
  GTK_RESPONSE_NONE = -1, GTK_RESPONSE_REJECT = -2, GTK_RESPONSE_ACCEPT = -3,
  GTK_RESPONSE_DELETE_EVENT = -4, GTK_RESPONSE_OK = -5, GTK_RESPONSE_CANCEL = -6,
  GTK_RESPONSE_CLOSE = -7, GTK_RESPONSE_YES = -8, GTK_RESPONSE_NO = -9,
  GTK_RESPONSE_APPLY = -10, GTK_RESPONSE_HELP = -11
} GtkResponseType;
typedef enum { GTK_MESSAGE_INFO, GTK_MESSAGE_WARNING, GTK_MESSAGE_QUESTION, GTK_MESSAGE_ERROR, GTK_MESSAGE_OTHER } GtkMessageType;
typedef enum {
  GTK_BUTTONS_NONE, GTK_BUTTONS_OK, GTK_BUTTONS_CLOSE, GTK_BUTTONS_CANCEL,
  GTK_BUTTONS_YES_NO, GTK_BUTTONS_OK_CANCEL
} GtkButtonsType;
typedef enum {
  GTK_DIALOG_MODAL = 1 << 0, GTK_DIALOG_DESTROY_WITH_PARENT = 1 << 1,
  GTK_DIALOG_NO_SEPARATOR = 1 << 2, GTK_DIALOG_USE_HEADER_BAR = 1 << 2
} GtkDialogFlags;
typedef enum { GTK_EXPAND = 1 << 0, GTK_SHRINK = 1 << 1, GTK_FILL = 1 << 2 } GtkAttachOptions;
typedef enum { GTK_UPDATE_CONTINUOUS, GTK_UPDATE_DISCONTINUOUS, GTK_UPDATE_DELAYED } GtkUpdateType;
typedef enum {
  GTK_FILE_CHOOSER_ACTION_OPEN, GTK_FILE_CHOOSER_ACTION_SAVE,
  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
} GtkFileChooserAction;
typedef enum { GTK_SELECTION_NONE, GTK_SELECTION_SINGLE, GTK_SELECTION_BROWSE, GTK_SELECTION_MULTIPLE } GtkSelectionMode;
typedef enum {
  GTK_ICON_SIZE_INVALID, GTK_ICON_SIZE_MENU, GTK_ICON_SIZE_SMALL_TOOLBAR,
  GTK_ICON_SIZE_LARGE_TOOLBAR, GTK_ICON_SIZE_BUTTON, GTK_ICON_SIZE_DND, GTK_ICON_SIZE_DIALOG
} GtkIconSize;
#define GTK_ICON_SIZE_INHERIT GTK_ICON_SIZE_INVALID
#define GTK_ICON_SIZE_NORMAL  GTK_ICON_SIZE_SMALL_TOOLBAR
#define GTK_ICON_SIZE_LARGE   GTK_ICON_SIZE_DIALOG
typedef enum { GTK_RELIEF_NORMAL, GTK_RELIEF_HALF, GTK_RELIEF_NONE } GtkReliefStyle;
typedef enum { GTK_STATE_NORMAL, GTK_STATE_ACTIVE, GTK_STATE_PRELIGHT, GTK_STATE_SELECTED, GTK_STATE_INSENSITIVE } GtkStateType;
typedef enum { GTK_SORT_ASCENDING, GTK_SORT_DESCENDING } GtkSortType;
typedef enum { GTK_UPDATE_ALWAYS, GTK_UPDATE_IF_VALID } GtkSpinButtonUpdatePolicy;
typedef enum { GTK_ARROW_UP, GTK_ARROW_DOWN, GTK_ARROW_LEFT, GTK_ARROW_RIGHT, GTK_ARROW_NONE } GtkArrowType;
typedef enum { GTK_TOOLBAR_ICONS, GTK_TOOLBAR_TEXT, GTK_TOOLBAR_BOTH, GTK_TOOLBAR_BOTH_HORIZ } GtkToolbarStyle;
typedef enum {
  GTK_LICENSE_UNKNOWN, GTK_LICENSE_CUSTOM, GTK_LICENSE_GPL_2_0, GTK_LICENSE_GPL_3_0,
  GTK_LICENSE_LGPL_2_1, GTK_LICENSE_LGPL_3_0, GTK_LICENSE_BSD, GTK_LICENSE_MIT_X11,
  GTK_LICENSE_ARTISTIC, GTK_LICENSE_GPL_2_0_ONLY, GTK_LICENSE_GPL_3_0_ONLY,
  GTK_LICENSE_LGPL_2_1_ONLY, GTK_LICENSE_LGPL_3_0_ONLY, GTK_LICENSE_AGPL_3_0,
  GTK_LICENSE_AGPL_3_0_ONLY, GTK_LICENSE_BSD_3, GTK_LICENSE_APACHE_2_0, GTK_LICENSE_MPL_2_0
} GtkLicense;
typedef enum {
  GTK_STACK_TRANSITION_TYPE_NONE, GTK_STACK_TRANSITION_TYPE_CROSSFADE,
  GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT,
  GTK_STACK_TRANSITION_TYPE_SLIDE_UP, GTK_STACK_TRANSITION_TYPE_SLIDE_DOWN,
  GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT, GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN
} GtkStackTransitionType;
typedef enum { GTK_INPUT_PURPOSE_FREE_FORM, GTK_INPUT_PURPOSE_ALPHA, GTK_INPUT_PURPOSE_DIGITS,
  GTK_INPUT_PURPOSE_NUMBER, GTK_INPUT_PURPOSE_PHONE, GTK_INPUT_PURPOSE_URL,
  GTK_INPUT_PURPOSE_EMAIL, GTK_INPUT_PURPOSE_NAME, GTK_INPUT_PURPOSE_PASSWORD,
  GTK_INPUT_PURPOSE_PIN } GtkInputPurpose;
typedef enum { GTK_ENTRY_ICON_PRIMARY, GTK_ENTRY_ICON_SECONDARY } GtkEntryIconPosition;
typedef enum { GTK_TREE_VIEW_COLUMN_GROW_ONLY, GTK_TREE_VIEW_COLUMN_AUTOSIZE, GTK_TREE_VIEW_COLUMN_FIXED } GtkTreeViewColumnSizing;
typedef enum { GTK_TREE_MODEL_ITERS_PERSIST = 1 << 0, GTK_TREE_MODEL_LIST_ONLY = 1 << 1 } GtkTreeModelFlags;

/* Pango enums that leak into the GTK API */
#include <pango/pango.h>

/* GTK 2 widget flags (GTK_WIDGET_SET_FLAGS etc.) */
typedef enum {
  GTK_TOPLEVEL = 1 << 4, GTK_NO_WINDOW = 1 << 5, GTK_REALIZED = 1 << 6,
  GTK_MAPPED = 1 << 7, GTK_VISIBLE = 1 << 8, GTK_SENSITIVE = 1 << 9,
  GTK_PARENT_SENSITIVE = 1 << 10, GTK_CAN_FOCUS = 1 << 11, GTK_HAS_FOCUS = 1 << 12,
  GTK_CAN_DEFAULT = 1 << 13, GTK_HAS_DEFAULT = 1 << 14
} GtkWidgetFlags;


/* Enum GTypes (for properties and GtkBuilder) */
GType gtk_align_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_ALIGN (gtk_align_get_type ())
GType gtk_orientation_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_ORIENTATION (gtk_orientation_get_type ())
GType gtk_wrap_mode_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_WRAP_MODE (gtk_wrap_mode_get_type ())
GType gtk_justification_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_JUSTIFICATION (gtk_justification_get_type ())
GType gtk_policy_type_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_POLICY_TYPE (gtk_policy_type_get_type ())
GType gtk_position_type_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_POSITION_TYPE (gtk_position_type_get_type ())
GType gtk_selection_mode_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_SELECTION_MODE (gtk_selection_mode_get_type ())
GType gtk_window_position_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_WINDOW_POSITION (gtk_window_position_get_type ())
GType gtk_shadow_type_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_SHADOW_TYPE (gtk_shadow_type_get_type ())
GType gtk_relief_style_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_RELIEF_STYLE (gtk_relief_style_get_type ())
GType gtk_message_type_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_MESSAGE_TYPE (gtk_message_type_get_type ())
GType gtk_buttons_type_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_BUTTONS_TYPE (gtk_buttons_type_get_type ())
GType gtk_stack_transition_type_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_STACK_TRANSITION_TYPE (gtk_stack_transition_type_get_type ())
GType gtk_license_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_LICENSE (gtk_license_get_type ())
GType gtk_input_purpose_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_INPUT_PURPOSE (gtk_input_purpose_get_type ())
GType gtk_window_type_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_WINDOW_TYPE (gtk_window_type_get_type ())
GType gtk_pack_type_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_PACK_TYPE (gtk_pack_type_get_type ())
GType gtk_icon_size_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_ICON_SIZE (gtk_icon_size_get_type ())
GType gtk_file_chooser_action_get_type (void) G_GNUC_CONST;
#define GTK_TYPE_FILE_CHOOSER_ACTION (gtk_file_chooser_action_get_type ())

G_END_DECLS

#endif
