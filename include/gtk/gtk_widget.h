
void gtk_widget_show(GtkWidget *widget)
{
	Q_ASSERT(widget);

	widget->show();
}

void gtk_widget_destroy(GtkWidget *widget)
{
	Q_ASSERT(widget);

	delete widget;
}

void gtk_widget_set_tooltip_text(GtkWidget *widget, const gchar *text)
{
	Q_ASSERT(widget && text);

	widget->setToolTip(text);
}
