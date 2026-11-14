/*
 * GTK made Qt - the part of ATK (the accessibility toolkit) GTK programs use.
 *
 * A widget's accessible object is the widget itself: names and descriptions
 * set through ATK become Qt's accessible name and description, which Qt
 * hands to the platform's accessibility (AT-SPI on Linux). ATK actions are
 * Qt's to provide: a program's own list of them is empty.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#ifndef GTKQT_ATK_H
#define GTKQT_ATK_H

#include <glib-object.h>

#if !defined (GTKQT_COMPILATION) || defined (GTKQT_EXPORT_API)
#pragma GCC visibility push(default)
#define GTKQT_VISIBILITY_PUSHED_ATK
#endif
G_BEGIN_DECLS

typedef struct _AtkObject AtkObject;
typedef struct _AtkAction AtkAction;
#define ATK_OBJECT(o) ((AtkObject *) (o))
#define ATK_ACTION(o) ((AtkAction *) (o))

void         atk_object_set_name (AtkObject *accessible, const gchar *name);
const gchar *atk_object_get_name (AtkObject *accessible);
void         atk_object_set_description (AtkObject *accessible, const gchar *description);
const gchar *atk_object_get_description (AtkObject *accessible);
gint         atk_action_get_n_actions (AtkAction *action);
const gchar *atk_action_get_name (AtkAction *action, gint i);
const gchar *atk_action_get_description (AtkAction *action, gint i);
gboolean     atk_action_set_description (AtkAction *action, gint i, const gchar *desc);

G_END_DECLS
#ifdef GTKQT_VISIBILITY_PUSHED_ATK
#pragma GCC visibility pop
#endif

#endif
