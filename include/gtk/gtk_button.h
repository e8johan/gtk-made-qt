class GQTPushButton;
typedef GQTPushButton GtkButton;

class GQTPushButton : public QPushButton
{
	Q_OBJECT

	public:
		GQTPushButton(const char *text, QWidget *parent) : QPushButton(text, parent)
		{
			this->connect(this, SIGNAL(clicked()), SLOT(gqt_clicked()));

		}
	public slots:
		void gqt_clicked()
		{
			gqt_signal_execute(this, "clicked");
		}
};

#include <gtk/moc_gtk_button.h>

GtkWidget *gtk_button_new()
{
	return new GtkButton(NULL, NULL);
}

GtkWidget *gtk_button_new_with_label(const gchar *text)
{
	Q_ASSERT(text);

	return new GtkButton(text, NULL);
}

GtkWidget *gtk_toggle_button_new_with_label(const gchar *text)
{
	Q_ASSERT(text);

	GtkButton *b = new GtkButton(text, NULL);
	b->setCheckable(true);
	return b;
}

