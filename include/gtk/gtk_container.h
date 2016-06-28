
void gtk_container_add(GtkContainer *container, GtkWidget *widget)
{
	Q_ASSERT(container && widget);

	// XXX: no idea if this is correct!
	gtk_box_pack_start(container, widget, false, false, 0);
}

void gtk_container_border_width(GtkContainer *container, guint border_width)
{
	Q_ASSERT(container && container->layout());
	container->layout()->setContentsMargins(border_width, border_width, border_width, border_width);
}

