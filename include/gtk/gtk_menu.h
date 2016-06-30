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

void gtk_menu_bar_append(GtkMenuBar *menubar, GtkWidget *widget)
{
	Q_ASSERT(menubar && widget);

	QMenu *m = dynamic_cast<QMenu *>(widget);
	qDebug("gtk_menu_bar_append(): appending %p(%s) to menubar %p", m, m->title().toLatin1().data(), menubar);

	Q_ASSERT(menubar && m);

	menubar->addMenu(m);
	m->QObject::setParent(menubar);
}

QMap<void *, QMenu *> gqt_menu_hack_lookup;

void gtk_menu_append(GtkMenuShell *menu_shell, GtkWidget *child)
{
	Q_ASSERT(menu_shell && child);

	QMap<void *,  QMenu *>::ConstIterator it = gqt_menu_hack_lookup.find(menu_shell);

	// If it's the end, we didn't see it go into gtk_menu_item_set_submenu
	Q_ASSERT(it != gqt_menu_hack_lookup.end());

	// so, we're really appending child_item to m
	QMenu *m = *it;
	QMenu *child_item = dynamic_cast<QMenu *>(child);

	Q_ASSERT(m && child_item);

	m->addMenu(child_item);
	child_item->QObject::setParent(m);
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




