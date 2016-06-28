
enum GtkWindowType
{
	GTK_WINDOW_TOPLEVEL,
	GTK_WINDOW_POPUP
};

class GQTWindow : public QWidget
{
 protected:
	void closeEvent(QCloseEvent *event)
	{
		// Fire response signal.
		gqt_signal_execute(this, "delete-event");
	}
};


GtkWidget *gtk_window_new(GtkWindowType type)
{
	// XXX: handle GtkWindowType properly.
	GtkWidget *g = new GQTWindow();
	g->setAttribute(Qt::WA_DeleteOnClose);
	return g;
}

void gtk_window_set_title(GtkWindow *window, const gchar *title)
{
	Q_ASSERT(window && title);

	window->setWindowTitle(title);
}

// XXX: I am not sure this implementation is correct. GTK's documentation on it sucks ass. (it implies this should resize, but then, why 'default size'?)
void gtk_window_set_default_size(GtkWindow *window, gint width, gint height)
{
	Q_ASSERT(window);

	// Per GTK's documentation, width and size 0 are impossible, so they are changed to 1.
	if (width == 0)
		width = 1;

	if (height == 0)
		height = 1;

	// Special case, per GTK documentation: width/height of -1 means that it goes back to the default size.
	if (width == -1 && height == -1)
	{
		window->resize(window->sizeHint());
		return;
	}

	// GTK's docs don't mention what happens with negative sizes, and I really don't want to imagine how to handle this.
	Q_ASSERT(width > 0 && height > 0);

	window->resize(width, height);
}

void gtk_window_set_modal(GtkWindow *window,gboolean modal)
{
	Q_ASSERT(window);

	window->setWindowModality(modal ? Qt::ApplicationModal : Qt::NonModal);

	// Qt doesn't allow modality to be changed on a visible window, so force a redisplay to
	// emulate this since it seems GTK doesn't care. Hacks forever!
	if (window->isVisible())
	{
		window->hide();
		window->show();
	}
}

#define GtkAccelGroup void
void gtk_window_add_accel_group(GtkWindow *window, GtkAccelGroup *g)
{

}
