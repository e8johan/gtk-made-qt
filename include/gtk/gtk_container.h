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

void gtk_container_add(QObject *o_p, QObject *o_c)
{
    QT_WIDGET_ASSERT(p)
    QT_WIDGET_ASSERT(c)

    Q_ASSERT(p->layout());

    p->layout()->addWidget(c);
}

void gtk_container_set_border_width(QObject *o_w, int spacing)
{
    QT_WIDGET_ASSERT(w)

    w->setContentsMargins(spacing,spacing,spacing,spacing);
}

void gtk_container_border_width(GtkContainer *container, guint border_width)
{
	Q_ASSERT(container && container->layout());
	container->layout()->setContentsMargins(border_width, border_width, border_width, border_width);
}

