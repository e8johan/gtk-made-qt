
GtkWidget *gtk_label_new(const gchar *text)
{
	GtkLabel *g = new GtkLabel(text);

	// Set alignment so we mimic GTK's default.
	g->setAlignment(Qt::AlignCenter);

	return g;
}

const gchar *gtk_label_get_text(GtkLabel *l)
{
	Q_ASSERT(l);

	return l->text().toAscii().constData();
}

void gtk_label_set_text(GtkLabel *l, const gchar *text)
{
	Q_ASSERT(l && text);

	l->setText(text);
}
