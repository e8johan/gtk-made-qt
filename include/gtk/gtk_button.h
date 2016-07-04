/*
 * GTK made Qt, implementing the GTK APIs through Qt
 * Copyright (C) 2010-2014 Johan Thelin
  * 
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

GtkWidget *gtk_toggle_button_new_with_label(const gchar *text)
{
	Q_ASSERT(text);

	GtkButton *b = new GtkButton(text, NULL);
	b->setCheckable(true);
	return b;
}

// Push button functions and defines
QPushButton *gtk_button_new_with_label(const char *t)
{
    QPushButton *b = new QPushButton(QString(t));
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return b;
}



