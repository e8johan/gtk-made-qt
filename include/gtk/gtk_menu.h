
void gtk_menu_bar_append(GtkMenuBar *menubar, GtkWidget *widget)
{
	Q_ASSERT(menubar && widget);

	QMenu *m = dynamic_cast<QMenu *>(widget);
	qDebug("gtk_menu_bar_append(): appending %p(%s) to menubar %p", m, m->title().toAscii().data(), menubar);

	Q_ASSERT(menubar && m);

	menubar->addMenu(m);
	m->QObject::setParent(menubar);
}

// Create a menu bar
GtkWidget *gtk_menu_bar_new()
{
	return new QMenuBar(NULL);
}

// Create a submenu
GtkWidget *gtk_menu_new()
{
	QMenu *q = new QMenu(NULL);
	qDebug("gtk_menu_new(): returning %p", q);
	return q;
}
QMap<void *, QMenu *> gqt_menu_hack_lookup;

void gtk_menu_item_set_submenu(GtkMenuItem *menu_item, GtkWidget *submenu)
{
	// grargh.. gtk_menu_new() isn't really needed by us (it way overcomplicates things) so use a map to
	// find the 'real' item and totally ignore the pointer returned by gtk_menu_new except for lookups..
	//qDebug("gtk_menu_item_set_submenu(): Setting %p to lookup to %p", submenu, menu_item);
	gqt_menu_hack_lookup[submenu] = menu_item;

	// but ensure the fake gtk_menu_new() item is deleted when the lookup menu is deleted.
	submenu->QObject::setParent(menu_item);
}

GtkWidget *gtk_menu_item_new_with_label(const gchar *text)
{
	Q_ASSERT(text);

	QWidget *q = new QMenu(text, NULL);
	//qDebug("gtk_menu_item_new_with_label(): returning %p", q);
	return q;
}

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

