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

// glib overrides for signal bits

typedef void (*GCallback)(GtkWidget *target, gpointer);
#define G_CALLBACK(x) (GCallback)x
#define g_list_append(x, y) g_list_append(x, (void *)y)

class GQTSignalHandler
{
 private:
	GCallback callback;
	gpointer user_data;

 public:
	GQTSignalHandler(GCallback where_to, gpointer data) : callback(where_to), user_data(data)
	{
	}

	void invoke(GtkWidget *w)
	{
		this->callback(w, user_data);
	}
};

/*
 * Let the magic begin!
 *
 * This map will store all hooked signals.
 * The first 'void *' key is the instance that is hooked (i.e. 'this').
 * It stores a map of signals which are hooked, i.e. 'clicked' -> pointer.
 */
static QMap<void *, QMap<QString, GQTSignalHandler *> > gqt_signals;

void gqt_signal_connect(QObject *src, const char *cstrEventName, void *f, void *data, bool swapped)
{
    QString eventName = QString(cstrEventName);

    if (eventName.endsWith("_event") || eventName == "event")
    {
        // This is an event, f is an eventFuncPtr

        // Setup and install the appropriate event filter on src
        // Add the filter to some sort of global filter list (so that it can be removed)

        QObject *o = QGtkEventFilter::createFilter(eventName, eventFuncPtr(f), data, swapped);
        if(o)
        {
            src->installEventFilter(o);
        }
        else
        {
            qWarning("Failed to match GTK event '%s' to a Qt event filter.", cstrEventName);
        }
    }
    else
    {
        // This is a callback, f is an callbackFuncPtr
        // Create a callback bridge QObject with the data and function pointer, make the connection and add the object to a global list
        QObject *o = new QGtkCallbackBridge(src, callbackFuncPtr(f), data, swapped);

        const char *signalName = 0;

        if(eventName == "clicked")
            signalName = SIGNAL(clicked());
        else if(eventName == "activate")
            signalName = SIGNAL(triggered());

        if (signalName)
        {
            QObject::connect(src, signalName, o, SLOT(invoke()));
        }
        else
        {
            qWarning("Failed to match GTK signal '%s' to a Qt signal.", cstrEventName);
        }
    }
}

void g_signal_connect(QObject *src, const char *cstrEventName, void *f, void *data)
{
    gqt_signal_connect(src, cstrEventName, f, data, false);
}

void g_signal_connect_swapped(QObject *src, const char *cstrEventName, void *f, void *data)
{
    gqt_signal_connect(src, cstrEventName, f, data, true);
}

void gqt_signal_execute(void *widget, const gchar *signal)
{
	Q_ASSERT(widget && signal);

	GtkWidget *w = (GtkWidget *)widget;

	QMap<void *, QMap<QString, GQTSignalHandler *> >::ConstIterator it = gqt_signals.constFind(widget);

	if (it == gqt_signals.end())
	{
		qDebug("No signals hooked on target widget %p", widget);
		return;
	}

	QMap<QString, GQTSignalHandler *> m = *it;
	QMap<QString, GQTSignalHandler *>::ConstIterator it2 = m.constFind(signal);

	if (it == gqt_signals.end())
	{
		qDebug("Signal %s not hooked on target widget %p", signal, widget);
		return;
	}

	qDebug("Invoking %s on %p", signal, widget);
	GQTSignalHandler *s = *it2;
	s->invoke(w);
}
