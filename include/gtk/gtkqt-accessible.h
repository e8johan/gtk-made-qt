/*
 * GTK made Qt - GtkAccessible (GTK 2/3), for programs built with the real
 * ATK (pkg-config gail): the base of accessible objects made for widgets.
 * Implemented in libgailutil. Without it a widget's accessible object is
 * the widget itself (see atk/atk.h).
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#ifndef GTKQT_ACCESSIBLE_H
#define GTKQT_ACCESSIBLE_H

#ifdef GTKQT_REAL_ATK
G_BEGIN_DECLS

#define GTK_TYPE_ACCESSIBLE           (gtk_accessible_get_type ())
#define GTK_ACCESSIBLE(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ACCESSIBLE, GtkAccessible))
#define GTK_ACCESSIBLE_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GTK_TYPE_ACCESSIBLE, GtkAccessibleClass))
#define GTK_IS_ACCESSIBLE(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ACCESSIBLE))
#define GTK_ACCESSIBLE_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), GTK_TYPE_ACCESSIBLE, GtkAccessibleClass))

typedef struct _GtkAccessible {
  AtkObject parent;
  GtkWidget *widget;
} GtkAccessible;
typedef struct _GtkAccessibleClass {
  AtkObjectClass parent_class;
  void (*connect_widget_destroyed) (GtkAccessible *accessible);
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
} GtkAccessibleClass;

GType      gtk_accessible_get_type (void) G_GNUC_CONST;
void       gtk_accessible_connect_widget_destroyed (GtkAccessible *accessible);
GtkWidget *gtk_accessible_get_widget (GtkAccessible *accessible);
void       gtk_accessible_set_widget (GtkAccessible *accessible, GtkWidget *widget);

G_END_DECLS
#endif

#endif
