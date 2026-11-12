/*
 * GTK made Qt: GLib 1.2 names that GLib 2 dropped. glib-config adds
 * -include gtkqt-glib1.h; the rest of GLib 1.2's API is still in GLib 2.
 */
#ifndef GTKQT_GLIB1_H
#define GTKQT_GLIB1_H
#include <glib.h>
#define G_HOOK_DEFERRED_DESTROY ((GHookFreeFunc) 0x01)
#endif
