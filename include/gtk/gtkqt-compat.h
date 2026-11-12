/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 *
 * Per-generation API differences. The library exports one set of symbols;
 * here the header maps each GTK generation's spelling onto it. This file is
 * included last, after all prototypes, because several macros reuse the name
 * of the function they wrap (the preprocessor does not re-expand those).
 */
#ifndef GTKQT_COMPAT_H
#define GTKQT_COMPAT_H

typedef GtkWidget GtkRoot;
typedef GtkWidget GtkNative;
#define GTK_ROOT(o)   ((GtkRoot *) (o))
#define GTK_NATIVE(o) ((GtkNative *) (o))

#ifndef GTKQT_COMPILATION

/* ---- gtk_init / GtkApplication: tell the library which API level the
 *      program was written against (it changes a few runtime defaults, e.g.
 *      GTK 4 widgets are visible by default). ---- */
#if GTKQT_API_VERSION >= 4
#define gtk_init()       gtkqt_init (NULL, NULL, 4)
#define gtk_init_check() gtkqt_init_check (NULL, NULL, 4)
#else
#define gtk_init(argc, argv)       gtkqt_init ((argc), (argv), GTKQT_API_VERSION)
#define gtk_init_check(argc, argv) gtkqt_init_check ((argc), (argv), GTKQT_API_VERSION)
#endif
#define gtk_application_new(id, flags) gtkqt_application_new ((id), (flags), GTKQT_API_VERSION)

/* ---- GTK 4: functions that lost or changed arguments ---- */
#if GTKQT_API_VERSION >= 4
#define gtk_window_new()                         gtk_window_new (GTK_WINDOW_TOPLEVEL)
#define gtk_scrolled_window_new()                gtk_scrolled_window_new (NULL, NULL)
#define gtk_button_new_from_icon_name(name)      gtk_button_new_from_icon_name ((name), GTK_ICON_SIZE_BUTTON)
#define gtk_image_new_from_icon_name(name)       gtk_image_new_from_icon_name ((name), GTK_ICON_SIZE_BUTTON)
#define gtk_image_set_from_icon_name(img, name)  gtk_image_set_from_icon_name ((img), (name), GTK_ICON_SIZE_BUTTON)
#define gtk_popover_new()                        gtk_popover_new (NULL)
#define gtk_css_provider_load_from_data(p, d, l) gtk_css_provider_load_from_data ((p), (d), (l), NULL)
#define gtk_file_chooser_set_current_folder(c, f, e) gtk_file_chooser_set_current_folder_file ((c), (f), (e))
#endif

/* ---- GTK 1.x / 2.x spellings ---- */
#if GTKQT_API_VERSION <= 2 && !defined (GTK_DISABLE_DEPRECATED)
#if GTKQT_API_VERSION == 1
typedef void (*GtkSignalFunc) ();   /* GTK 1: unprototyped, assignable to any handler type */
#define GTK_SIGNAL_FUNC(f)                          ((GtkSignalFunc) (f))
#else
typedef GCallback GtkSignalFunc;
#define GTK_SIGNAL_FUNC(f)                          G_CALLBACK (f)
#endif
typedef GSourceFunc GtkFunction;
/* GTK 1/2 code often passes handlers without GTK_SIGNAL_FUNC (); that
 * compiled for decades, and GCC 14 / clang 16 would now reject it */
#define gtk_signal_connect(o, name, f, d)           g_signal_connect ((o), (name), G_CALLBACK (f), (d))
#define gtk_signal_connect_after(o, name, f, d)     g_signal_connect_after ((o), (name), G_CALLBACK (f), (d))
#define gtk_signal_connect_object(o, name, f, slot) g_signal_connect_swapped ((o), (name), G_CALLBACK (f), (slot))
#define gtk_signal_disconnect(o, id)                g_signal_handler_disconnect ((o), (id))
#define gtk_signal_emit_by_name                     g_signal_emit_by_name
#define gtk_signal_emit_stop_by_name(o, name)       g_signal_stop_emission_by_name ((o), (name))
#define gtk_object_set_data(o, k, v)                g_object_set_data (G_OBJECT (o), (k), (v))
#define gtk_object_get_data(o, k)                   g_object_get_data (G_OBJECT (o), (k))
#define gtk_object_ref(o)                           g_object_ref (o)
#define gtk_object_unref(o)                         g_object_unref (o)
#define gtk_object_sink(o)                          g_object_ref_sink (o)
#define gtk_object_destroy(o)                       gtk_widget_destroy (GTK_WIDGET (o))
#define gtk_widget_ref(w)                           g_object_ref (w)
#define gtk_widget_unref(w)                         g_object_unref (w)
#define gtk_timeout_add(ms, f, d)                   g_timeout_add ((ms), (GSourceFunc) (f), (d))
#define gtk_timeout_remove(id)                      g_source_remove (id)
#define gtk_idle_add(f, d)                          g_idle_add ((GSourceFunc) (f), (d))
#define gtk_idle_remove(id)                         g_source_remove (id)
#define gtk_toggle_button_set_state                 gtk_toggle_button_set_active
#define gtk_menu_item_right_justify(item)           gtk_menu_item_set_right_justified ((item), TRUE)
/* GTK 2 returned GtkObject* here; a gpointer is assignable to either. */
#define gtk_adjustment_new(v, l, u, s, p, ps)       ((gpointer) gtk_adjustment_new ((v), (l), (u), (s), (p), (ps)))
#endif

#endif /* GTKQT_COMPILATION */

#endif
