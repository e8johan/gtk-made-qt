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

// Box and packing functions and defines

void gtk_box_pack_start(QObject *o_p, QObject *o_w, bool expand, bool fill, int padding)
{
    Q_UNUSED(expand)
    Q_UNUSED(fill)

    QT_WIDGET_ASSERT(p)
    QT_WIDGET_ASSERT(w)

    QBoxLayout *l = qobject_cast<QBoxLayout*>(p->layout());
    Q_ASSERT(l);
    l->addWidget(w);
    if (padding)
        l->addSpacerItem(new QSpacerItem(padding, padding, QSizePolicy::Fixed, QSizePolicy::Fixed));
}

void gtk_box_pack_end(QObject *o_p, QObject *o_w, bool expand, bool fill, int padding)
{
    Q_UNUSED(expand)
    Q_UNUSED(fill)

    QT_WIDGET_ASSERT(p)
    QT_WIDGET_ASSERT(w)

    QBoxLayout *l = qobject_cast<QBoxLayout*>(p->layout());
    Q_ASSERT(l);
    l->addWidget(w);
    if (padding)
        l->addSpacerItem(new QSpacerItem(padding, padding, QSizePolicy::Fixed, QSizePolicy::Fixed));
}






