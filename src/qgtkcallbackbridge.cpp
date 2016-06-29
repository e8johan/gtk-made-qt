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

#include <qgtkcallbackbridge.h>

QGtkCallbackBridge::QGtkCallbackBridge(QObject *src, callbackFuncPtr f, void *data, bool swapped) :
    QObject(0), m_f(f), m_data(data), m_swapped(swapped)
{
    Q_UNUSED(src)
}

void QGtkCallbackBridge::invoke()
{
    if (!m_swapped)
    {
        m_f(sender(), m_data);
    } else {
        swappedCallbackFuncPtr m_fs = (swappedCallbackFuncPtr)m_f;
        m_fs(m_data, sender());
    }
}
