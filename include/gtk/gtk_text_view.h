GtkWidget *gtk_text_view_new()
{
	return new QTextEdit(NULL);
}

void gtk_text_view_set_editable(GtkTextView *text_view, gboolean setting)
{
	Q_ASSERT(text_view);

	// GTK is backwards from Qt, hence negation
	text_view->setReadOnly(!setting);
}


// Implementation of GtkTextBuffer goes here for now (sigh).
GtkTextBuffer *gtk_text_view_get_buffer(GtkTextView *text_view)
{
	// In GTK, text model is seperated from view control, in Qt this is not the case, so let's just fake doing something.
	return text_view;
}

void gtk_text_buffer_set_text(GtkTextBuffer *buffer, const gchar *text, gint len)
{
	Q_ASSERT(buffer);

	if (len == -1)
		len = strlen(text);

	// QByteArray is necessary here in case text isn't a regular NULL terminated string.
	buffer->setText(QString(QByteArray(text, len)));
}
