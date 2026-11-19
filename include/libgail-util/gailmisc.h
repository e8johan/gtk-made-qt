/*
 * GTK made Qt - GTK 2's gail-util: text attributes and coordinates for ATK.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#ifndef GTKQT_GAIL_MISC_H
#define GTKQT_GAIL_MISC_H

#include <gtk/gtk.h>
#include <atk/atk.h>
#include <pango/pango.h>

G_BEGIN_DECLS

AtkAttributeSet *gail_misc_add_attribute (AtkAttributeSet *attrib_set, AtkTextAttribute attr, gchar *value);
AtkAttributeSet *gail_misc_layout_get_run_attributes (AtkAttributeSet *attrib_set, PangoLayout *layout, gchar *text,
                                                     gint offset, gint *start_offset, gint *end_offset);
AtkAttributeSet *gail_misc_get_default_attributes (AtkAttributeSet *attrib_set, PangoLayout *layout, GtkWidget *widget);
void             gail_misc_get_extents_from_pango_rectangle (GtkWidget *widget, PangoRectangle *char_rect, gint x_layout,
                                                             gint y_layout, gint *x, gint *y, gint *width, gint *height,
                                                             AtkCoordType coords);
gint             gail_misc_get_index_at_point_in_layout (GtkWidget *widget, PangoLayout *layout, gint x_layout, gint y_layout,
                                                         gint x, gint y, AtkCoordType coords);
void             gail_misc_get_origins (GtkWidget *widget, gint *x_window, gint *y_window, gint *x_toplevel, gint *y_toplevel);
AtkAttributeSet *gail_misc_buffer_get_run_attributes (GtkTextBuffer *buffer, gint offset, gint *start_offset, gint *end_offset);

G_END_DECLS

#endif
