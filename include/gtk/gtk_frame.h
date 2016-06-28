GtkWidget *gtk_frame_new(const gchar *text)
{
	QGroupBox *f = new QGroupBox();
	f->setTitle(text);
	return f;
}
