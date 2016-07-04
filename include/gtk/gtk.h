/*
 * GTK made Qt, implementing the GTK APIs through Qt
 * Copyright (C) 2010-2014 Johan Thelin
  * 
 * gqt, implementing the GTK APIs through Qt
 * Copyright (C) Robin Burchell
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the Lesser GNU General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __GTK__
#define __GTK__

// Basic stuff here
#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

// GLib stuff here
#ifdef USE_GLIB2
#include <glib.h> /* temporary? re-map to Qt? */
#else
typedef void* gpointer;
typedef bool gboolean;
typedef char gchar;
#endif

#define G_OBJECT(obj) ((QObject*)(obj))
#define G_CALLBACK(func) ((void*)(func))
#define g_print qDebug

//Qt includes
#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QAction>

#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QMap>
#include <QGroupBox>
#include <QActionGroup>

char *g_strdup(const char *s)
{
    char *d = new char[strlen(s)+1];
    strcpy(d, s);
    return d;
}

//gtk-made-qt includes
#include "../glibtypes.h"
#include "../qgtkcallbackbridge.h"
#include "../qgtkeventfilter.h" 

typedef QWidget GtkWidget;
typedef QWidget GtkWindow;
typedef QWidget GtkBox; /* ugh */
typedef QWidget GtkContainer; /* ugh */
typedef QHBoxLayout GtkHBox;
typedef QVBoxLayout GtkVBox;
typedef QWidget GtkTable;

typedef QTextEdit GtkTextView;
typedef QTextEdit GtkTextBuffer;
typedef QMenuBar GtkMenuBar;
typedef QMenu GtkMenuShell;
typedef QMenu GtkMenuItem;

typedef QLabel GtkLabel;

// XXX: will need porting for win32 (and non-gcc compilers..)
#define MARK_DEPRECATED __attribute((deprecated))

// Types and type casting macros
#define QT_WIDGET_ASSERT(name) QWidget *name = qobject_cast<QWidget*>(o_##name); Q_ASSERT(name);
#define QT_WIDGET(name) QWidget *name = qobject_cast<QWidget*>(o_##name);
#define QT_PTR_ASSERT(type,name) type *name = qobject_cast<type*>(o_##name); Q_ASSERT(name);
#define QT_PTR(type,name) type *name = qobject_cast<type*>(o_##name);

// Stupid GTK typecasting crap.
#define GTK_WINDOW(obj) (obj)
#define GTK_WIDGET(obj) (obj)
#define GTK_CONTAINER(obj) (obj)
#define GTK_BOX(obj) (obj)
#define GTK_TABLE(obj) (obj)
#define GTK_MENU_SHELL(obj) (obj)
#define GTK_MENU_ITEM(x) dynamic_cast<QMenu *>(x) /*from gqt */
#define GTK_MENU(x) dynamic_cast<QMenu *>(x) /* from gqt */

#define GTK_TEXT_VIEW(x) dynamic_cast<QTextEdit *>(x)
#define GTK_OBJECT(x) (void *)x /* obsolete */
#define GTK_MENU_BAR(x) dynamic_cast<QMenuBar *>(x)
#define GTK_LABEL(x) dynamic_cast<QLabel *>(x)
#define GTK_ABOUT_DIALOG(x) dynamic_cast<GQTAboutDialog *>(x)

// Stock stuff
#define GTK_STOCK_QUIT "gtk-quit"
#define GTK_STOCK_ABOUT "gtk-about"

enum GtkAttachOptions
{
	GTK_EXPAND,
	GTK_SHRINK,
	GTK_FILL
};

// Now, override the parts of glib that we need to.
#include <gtk/glib_signals.h>


#include <gtk/gqt_dialog.h>
#include <gtk/gtk_box.h>
#include <gtk/gtk_main.h>
#include <gtk/gtk_button.h>
#include <gtk/gtk_window.h>
#include <gtk/gtk_hbox.h>
#include <gtk/gtk_vbox.h>
#include <gtk/gtk_container.h>
#include <gtk/gtk_label.h>
#include <gtk/gtk_entry.h>
#include <gtk/gtk_table.h>
#include <gtk/gtk_combo.h>
#include <gtk/gtk_check_button.h>
#include <gtk/gtk_text_view.h>
#include <gtk/gtk_menu.h>
#include <gtk/gtk_about_dialog.h>
#include <gtk/gtk_widget.h>
#include <gtk/gtk_frame.h>
#include <gtk/gtk_action_group.h>
#include <gtk/gtk_ui_manager.h>


#endif
