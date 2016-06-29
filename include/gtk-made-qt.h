/*
 * GTK made Qt, implementing the GTK APIs through Qt
 * Copyright (C) 2010-2014 Johan Thelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GTKMADEQT_H
#define GTKMADEQT_H

#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QAction>

// Basic stuff here

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

// GLib stuff here

#define G_OBJECT(obj) ((QObject*)(obj))

typedef void* gpointer;
typedef bool gboolean;
typedef char gchar;

#define g_print qDebug

char *g_strdup(const char *s)
{
    char *d = new char[strlen(s)+1];
    strcpy(d, s);
    return d;
}

#include "glibtypes.h"
#include "qgtkcallbackbridge.h"
#include "qgtkeventfilter.h" 

#define G_CALLBACK(func) ((void*)(func))

void gqt_signal_connect(QObject *src, const char *cstrEventName, void *f, void *data, bool swapped)
{
    QString eventName = QString(cstrEventName);

    if (eventName.endsWith("_event") || eventName == "event")
    {
        // This is an event, f is an eventFuncPtr

        // Setup and install the appropriate event filter on src
        // Add the filter to some sort of global filter list (so that it can be removed)

        QObject *o = QGtkEventFilter::createFilter(eventName, eventFuncPtr(f), data, swapped);
        if(o)
        {
            src->installEventFilter(o);
        }
        else
        {
            qWarning("Failed to match GTK event '%s' to a Qt event filter.", cstrEventName);
        }
    }
    else
    {
        // This is a callback, f is an callbackFuncPtr
        // Create a callback bridge QObject with the data and function pointer, make the connection and add the object to a global list
        QObject *o = new QGtkCallbackBridge(src, callbackFuncPtr(f), data, swapped);

        const char *signalName = 0;

        if(eventName == "clicked")
            signalName = SIGNAL(clicked());
        else if(eventName == "activate")
            signalName = SIGNAL(triggered());

        if (signalName)
        {
            QObject::connect(src, signalName, o, SLOT(invoke()));
        }
        else
        {
            qWarning("Failed to match GTK signal '%s' to a Qt signal.", cstrEventName);
        }
    }
}

void g_signal_connect(QObject *src, const char *cstrEventName, void *f, void *data)
{
    gqt_signal_connect(src, cstrEventName, f, data, false);
}

void g_signal_connect_swapped(QObject *src, const char *cstrEventName, void *f, void *data)
{
    gqt_signal_connect(src, cstrEventName, f, data, true);
}

// GTK+ stuff here

// Types and type casting macros

typedef QObject GtkWidget;
typedef QObject GtkWindow;

#define QT_WIDGET_ASSERT(name) QWidget *name = qobject_cast<QWidget*>(o_##name); Q_ASSERT(name);
#define QT_WIDGET(name) QWidget *name = qobject_cast<QWidget*>(o_##name);
#define QT_PTR_ASSERT(type,name) type *name = qobject_cast<type*>(o_##name); Q_ASSERT(name);
#define QT_PTR(type,name) type *name = qobject_cast<type*>(o_##name);

#define GTK_WINDOW(obj) (obj)
#define GTK_WIDGET(obj) (obj)
#define GTK_CONTAINER(obj) (obj)
#define GTK_BOX(obj) (obj)
#define GTK_TABLE(obj) (obj)
#define GTK_MENU_SHELL(obj) (obj)
#define GTK_MENU_ITEM(x) dynamic_cast<QMenu *>(x) /*from gqt */
#define GTK_MENU(x) dynamic_cast<QMenu *>(x) /* from gqt */

// Application wide functions

void gtk_init(int *argc, char ***argv)
{
    QApplication *a = new QApplication(*argc,*argv);

    Q_UNUSED(a)
}

int gtk_main() { return qApp->exec(); }
void gtk_main_quit() { qApp->quit(); }

// Widget modification functions and defines

void gtk_widget_show(QObject *o_w)
{
    QT_WIDGET(w) // No assert, can be QAction

    if(!w)
        return; // TODO probably a QAction - how do we handle hide?

    if (!w->parent())
        w->show();
    else
        w->setVisible(true);
}

void gtk_widget_set_size_request(QObject *o_w, int width, int height)
{
    QT_WIDGET_ASSERT(w)

    w->setMinimumSize(QSize(width, height));
    w->setMaximumSize(QSize(width, height));
}

// Window modification functions and defines

#define GTK_WINDOW_TOPLEVEL (0) 
QWidget *gtk_window_new(int)
{
    // Seems like all windows can act as a container, see example tablepacking, pick HBoxLayout as std
    QWidget *w = new QWidget;
    w->setLayout(new QHBoxLayout);
    w->setContentsMargins(0,0,0,0);
    w->layout()->setContentsMargins(0,0,0,0);
    w->layout()->setSpacing(0);
    return w;
}

void gtk_window_set_title(QObject *o_w, const char *t)
{
    QT_WIDGET_ASSERT(w)

    w->setWindowTitle(QString(t));
}

// Container modification functions and defines

QWidget *gtk_hbox_new(bool homogenus, int spacing)
{
    Q_UNUSED(homogenus)
    Q_UNUSED(spacing)

    QWidget *w = new QWidget;
    w->setLayout(new QHBoxLayout);
    w->layout()->setContentsMargins(0,0,0,0);
    w->layout()->setSpacing(0);
    return w;
}

QWidget *gtk_vbox_new(bool homogenus, int spacing)
{
    Q_UNUSED(homogenus)
    Q_UNUSED(spacing)

    QWidget *w = new QWidget;
    w->setLayout(new QVBoxLayout);
    w->layout()->setContentsMargins(0,0,0,0);
    w->layout()->setSpacing(0);
    return w;
}

void gtk_container_set_border_width(QObject *o_w, int spacing)
{
    QT_WIDGET_ASSERT(w)

    w->setContentsMargins(spacing,spacing,spacing,spacing);
}

void gtk_container_add(QObject *o_p, QObject *o_c)
{
    QT_WIDGET_ASSERT(p)
    QT_WIDGET_ASSERT(c)

    Q_ASSERT(p->layout());

    p->layout()->addWidget(c);
}

QWidget *gtk_table_new(int, int, bool)
{
    QWidget *w = new QWidget;
    w->setLayout(new QGridLayout);
    w->layout()->setContentsMargins(0,0,0,0);
    w->layout()->setSpacing(0);
    return w;
}

void gtk_table_attach_defaults(QObject *o_p, QObject *o_cw, int l, int r, int t, int b)
{
    QT_WIDGET_ASSERT(p)
    QT_WIDGET_ASSERT(cw)

    QGridLayout *gl = qobject_cast<QGridLayout*>(p->layout());
    Q_ASSERT(gl);
    gl->addWidget(cw, t, l, b-t, r-l);
}

// Push button functions and defines

QPushButton *gtk_button_new_with_label(const char *t)
{
    QPushButton *b = new QPushButton(QString(t));
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return b;
}

// Box and packing functions and defines

void gtk_box_pack_start(QObject *o_p, QObject *o_w, bool expand, bool fill, int padding)
{
    Q_UNUSED(expand)
    Q_UNUSED(fill)

    QT_WIDGET_ASSERT(p)
    QT_WIDGET_ASSERT(w)

    QBoxLayout *l = qobject_cast<QBoxLayout*>(p->layout());
    Q_ASSERT(l);
    l->addWidget(w);
    if (padding)
        l->addSpacerItem(new QSpacerItem(padding, padding, QSizePolicy::Fixed, QSizePolicy::Fixed));
}

void gtk_box_pack_end(QObject *o_p, QObject *o_w, bool expand, bool fill, int padding)
{
    Q_UNUSED(expand)
    Q_UNUSED(fill)

    QT_WIDGET_ASSERT(p)
    QT_WIDGET_ASSERT(w)

    QBoxLayout *l = qobject_cast<QBoxLayout*>(p->layout());
    Q_ASSERT(l);
    l->addWidget(w);
    if (padding)
        l->addSpacerItem(new QSpacerItem(padding, padding, QSizePolicy::Fixed, QSizePolicy::Fixed));
}

// Menu functions and defines

QMenu *gtk_menu_new()
{
    return new QMenu;
}

QAction *gtk_menu_item_new_with_label(const char *t)
{
    return new QAction(QString(t), qApp);
}

void gtk_menu_shell_append(QObject *o_m, QObject *o_a)
{
    // Menu can be either QMenu or QMenuBar
    QMenu *m = qobject_cast<QMenu*>(o_m);
    QMenuBar *mb = qobject_cast<QMenuBar*>(o_m);
    Q_ASSERT(m || mb);

    // Menu item can be either QAction or QMenu
    QT_PTR_ASSERT(QAction,a)

    if(m) {
        m->addAction(a);
    } else {
        mb->addAction(a);
    }
}

void gtk_menu_item_set_submenu(QObject *o_rm, QObject *o_m)
{
    QT_PTR_ASSERT(QAction,rm)
    QT_PTR_ASSERT(QMenu,m)

    rm->setMenu(m);
}

QWidget *gtk_menu_bar_new()
{
    return new QMenuBar;
}

void gtk_menu_popup(QObject *o_m, void*, void*, void*, void*, int, int)
{
    QT_PTR_ASSERT(QMenu, m)

    // TODO uses static point
    m->popup(QPoint(100, 100));
}

#endif // GTKMADEQT_H
