
// TODO: respect expand, fill, padding
void gtk_box_pack_start(GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding)
{
	Q_ASSERT(box && child);

	if (!box->layout())
	{
		// XXX: is this behaviour correct? we need it for gtk_container_add() and the like on a main window (ugh)
		GtkHBox *h = new GtkHBox(NULL);
		h->setParent(box);
		box->setLayout(h);
	}

	// GtkBox is actually a QWidget with a layout set to work around GTK's
	// odd idea of layouts being widgets. Retrieve the set layout, and pack 
	box->layout()->addWidget(child);
	child->setParent(box);
}

