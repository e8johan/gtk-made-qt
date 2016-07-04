/*
 * gqt, implementing the GTK APIs through Qt
 * Copyright (C) Robin Burchell
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the Lesser GNU General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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

	return entry->text().toLatin1().constData();
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
