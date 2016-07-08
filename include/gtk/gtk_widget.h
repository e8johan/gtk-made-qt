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

// Widget modification functions and defines

void gtk_widget_show(GtkWidget *o_w)
{
    QT_WIDGET(w) // No assert, can be QAction

    if(!w)
        return; // TODO probably a QAction - how do we handle hide?

    if (!w->parent())
        w->show();
    else
        w->setVisible(true);
}

void gtk_widget_set_size_request(GtkWidget *o_w, int width, int height)
{
    QT_WIDGET_ASSERT(w)

    w->setMinimumSize(QSize(width, height));
    w->setMaximumSize(QSize(width, height));
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
