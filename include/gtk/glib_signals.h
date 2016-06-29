// glib overrides for signal bits

#include "../gtk-made-qt.h"

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
