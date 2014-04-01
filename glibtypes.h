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

#ifndef GLIBTYPES_H
#define GLIBTYPES_H

enum EventType { GDK_BUTTON_PRESS,
                 GDK_BUTTON_RELEASE,
                 GDK_SCROLL,
                 GDK_MOTION_NOTIFY,
                 GDK_DELETE,
                 GDK_DESTROY,
                 GDK_EXPOSE,
                 GDK_KEY_PRESS,
                 GDK_KEY_RELEASE,
                 GDK_ENTER_NOTIFY,
                 GDK_LEAVE_NOTIFY,
                 GDK_CONFIGURE,
                 GDK_FOCUS_IN,
                 GDK_FOCUS_OUT,
                 GDK_MAP,
                 GDK_UNMAP,
                 GDK_PROPERTY_NOTIFY,
                 GDK_SELECTION_CLEAR,
                 GDK_SELECTION_REQUEST,
                 GDK_SELECTION_NOTIFY,
                 GDK_PROXIMITY_IN,
                 GDK_PROXIMITY_OUT,
                 GDK_VISIBILITY_NOTIFY,
                 GDK_CLIENT,
                 GDK_NO_EXPOSE,
                 GDK_WINDOW_STATE };

// TODO we need to investigate how this structure looks in detail it is a union
struct GdkEvent {

    EventType type;
};

typedef void (*callbackFuncPtr)(QObject*, void*);
typedef void (*swappedCallbackFuncPtr)(void*, QObject*);
typedef bool (*eventFuncPtr)(QObject*, GdkEvent*, void*);
typedef bool (*swappedEventFuncPtr)(void*, GdkEvent*, QObject*);

#endif // GLIBTYPES_H
