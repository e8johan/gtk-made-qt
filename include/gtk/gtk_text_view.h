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
