
#include "../gtk-made-qt.h"

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

