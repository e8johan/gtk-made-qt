/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Copyright (C) 2010-2014 Johan Thelin, Robin Burchell and contributors
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version. See COPYING for details.
 *
 * Umbrella header: a drop-in replacement for <gtk/gtk.h> of GTK 2, 3 and 4.
 * Select the API level with -DGTKQT_API_VERSION=2|3|4 (the bundled
 * pkg-config files gtk+-2.0.pc / gtk+-3.0.pc / gtk4.pc do that for you).
 */
#ifndef GTKQT_GTK_H
#define GTKQT_GTK_H

#include <gtk/gtkqt-version.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gmodule.h>
#include <gdk/gdk.h>

#if !defined (GTKQT_COMPILATION) || defined (GTKQT_EXPORT_API)
#pragma GCC visibility push(default)
#define GTKQT_VISIBILITY_PUSHED_GTK
#endif
#include <gtk/gtkqt-enums.h>
#include <gtk/gtkqt-widget.h>
#include <gtk/gtkqt-containers.h>
#include <gtk/gtkqt-controls.h>
#include <gtk/gtkqt-range.h>
#include <gtk/gtkqt-menus.h>
#include <gtk/gtkqt-text.h>
#include <gtk/gtkqt-tree.h>
#include <gtk/gtkqt-dialogs.h>
#include <gtk/gtkqt-application.h>
#include <gtk/gtkqt-gtk2.h>
#include <gtk/gtkqt-print.h>
#include <gtk/gtkqt-gtk1.h>
#include <gtk/gtkqt-compat.h>
#ifdef GTKQT_VISIBILITY_PUSHED_GTK
#pragma GCC visibility pop
#endif

#endif
