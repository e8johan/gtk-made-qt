// XXX: implement homogenous
// NOTE: columns and rows are ignored here, Qt doesn't require them
GtkWidget *gtk_table_new(guint rows, guint columns, gboolean homogenous)
{
	// Oh, dear god.
	// In GTK, layouts are children of widgets. In Qt, they aren't.
	// This fragile hack will work around that by creating a widget
	// and a layout, setting the layout on the widget, 
	// and returning the created widget. UGH.
	GtkWidget *g = new GtkWidget(NULL);
	QGridLayout *h = new QGridLayout(NULL);
	h->setParent(g);
	g->setLayout(h);
	return g;
}

void gtk_table_attach(GtkTable *table, GtkWidget *child,
		guint left_attach, guint right_attach,
		guint top_attach, guint bottom_attach,
		int xopt, int yopt,
		guint xpadding, guint ypadding)
{
	Q_ASSERT(table && table->layout() && child);

	// Make sure the column/rowspan values are going to be kosher
	Q_ASSERT(right_attach - left_attach > 0);
	Q_ASSERT(bottom_attach - top_attach > 0);

	qDebug("Adding item to table, left: %d, top %d, width %d, height %d", left_attach, top_attach, right_attach - left_attach, bottom_attach - top_attach);

	QGridLayout *g = dynamic_cast<QGridLayout *>(table->layout());
	Q_ASSERT(g);

	g->addWidget(child, top_attach, left_attach, right_attach - left_attach, bottom_attach - top_attach, Qt::AlignCenter);
	child->setParent(table);
}
