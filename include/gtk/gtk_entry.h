class GQTEntry : public QLineEdit
{
	Q_OBJECT
 protected:
 public:
	GQTEntry() : QLineEdit()
	{
		this->connect(this, SIGNAL(textChanged(const QString&)), SLOT(gqt_changed(const QString&)));

	}
 public slots:
	void gqt_changed(const QString &text)
	{
		// Fire signal.
		gqt_signal_execute(this, "changed");
	}
};

#include <gtk/moc_gtk_entry.h>

typedef QLineEdit GtkEntry;
#define GTK_ENTRY(x) dynamic_cast<GQTEntry *>(x)

GtkWidget *gtk_entry_new()
{
	return new GQTEntry();
}

// XXX: is this deprecated?
void gtk_entry_set_editable(GtkEntry *entry, gboolean editable)
{
	Q_ASSERT(entry);

	// The extra negation here is because gtk_entry_set_editable() wants 'false' for non-readable,
	// but setReadOnly will of course want 'true' in the same condition.
	entry->setReadOnly(!editable);
}

void gtk_entry_set_text(GtkEntry *entry, const gchar *text)
{
	Q_ASSERT(entry);

	entry->setText(text);
}

const gchar *gtk_entry_get_text(GtkEntry *entry)
{
	Q_ASSERT(entry);

	return entry->text().toAscii().constData();
}

void gtk_entry_append_text(GtkEntry *entry, const gchar *text)
{
	Q_ASSERT(entry);

	// Yuck, QLineEdit has no append() method, unlike QTextEdit
	entry->setText(entry->text() + text);
}

void gtk_entry_set_max_length(GtkEntry *entry, gint max_length)
{
	Q_ASSERT(entry);

	entry->setMaxLength(max_length);
}
