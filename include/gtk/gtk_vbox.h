
// XXX: implement homogenous
GtkWidget *gtk_vbox_new(gboolean homogenous, gint spacing)
{
	// Oh, dear god.
	// In GTK, layouts are children of widgets. In Qt, they aren't.
	// This fragile hack will work around that by creating a widget
	// and a layout, setting the layout on the widget, 
	// and returning the created widget. UGH.
	GtkWidget *g = new GtkWidget(NULL);
	GtkVBox *h = new GtkVBox(NULL);
	h->setParent(g);
	h->setSpacing(spacing);
	g->setLayout(h);
	return g;
}
