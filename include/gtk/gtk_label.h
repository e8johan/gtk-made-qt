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

	return l->text().toLatin1().constData();
}

void gtk_label_set_text(GtkLabel *l, const gchar *text)
{
	Q_ASSERT(l && text);

	l->setText(text);
}
