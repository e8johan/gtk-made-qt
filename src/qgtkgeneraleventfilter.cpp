/*
 * GTK made Qt, implementing the GTK APIs through Qt
 * Copyright (C) 2010-2014 Johan Thelin
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

#include <qgtkgeneraleventfilter.h>

#include <QEvent>

QGtkGeneralEventFilter::QGtkGeneralEventFilter(eventFuncPtr f, void *data) :
    QGtkEventFilter(f, data)
{
}

bool QGtkGeneralEventFilter::eventFilter(QObject *src, QEvent *e)
{
    GdkEvent ge;

    // TODO ignores the return value of invoke...
    switch (e->type())
    {
    case QEvent::Close:
        ge.type = GDK_DELETE;
        invoke(src, &ge);
        break;
    case QEvent::MouseButtonPress:
        ge.type = GDK_BUTTON_PRESS;
        invoke(src, &ge);
        break;

    default:
// TODO (noicy!)        qDebug("QGtkGeneralEventFilter ignored event of type %d", e->type());
        break;
    }

    // Let everyone else see the event too
    return false;
}
