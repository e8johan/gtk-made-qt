/*
 * GTK made Qt, implementing the GTK APIs through Qt
 * Copyright (C) 2010-2014 Johan Thelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "qgtkdeleteeventfilter.h"

#include <QEvent>

QGtkDeleteEventFilter::QGtkDeleteEventFilter(eventFuncPtr f, void *data) :
    QGtkEventFilter(f, data)
{
}

bool QGtkDeleteEventFilter::eventFilter(QObject *src, QEvent *e)
{
    GdkEvent ge;

    if( e->type() == QEvent::Close)
    {
        ge.type = GDK_DELETE;

        // TODO check the sign of the return value, and the interpretation
        if(invoke(src, &ge))
            e->accept();
        else
            e->ignore();
    }

    // Let everyone else see the event too
    return false;
}
