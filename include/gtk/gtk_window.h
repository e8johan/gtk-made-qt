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

// Window modification functions and defines
enum GtkWindowType
{
	GTK_WINDOW_TOPLEVEL,
	GTK_WINDOW_POPUP
};

QWidget *gtk_window_new(int)
{
    // Seems like all windows can act as a container, see example tablepacking, pick HBoxLayout as std
    QWidget *w = new QWidget;
    w->setLayout(new QHBoxLayout);
    w->setContentsMargins(0,0,0,0);
    w->layout()->setContentsMargins(0,0,0,0);
    w->layout()->setSpacing(0);
    return w;
}

void gtk_window_set_title(QObject *o_w, const char *t)
{
    QT_WIDGET_ASSERT(w)

    w->setWindowTitle(QString(t));
}

class GQTWindow : public QWidget
{
 protected:
	void closeEvent(QCloseEvent *event)
	{
		// Fire response signal.
		gqt_signal_execute(this, "delete-event");
	}
};

// XXX: I am not sure this implementation is correct. GTK's documentation on it sucks ass. (it implies this should resize, but then, why 'default size'?)
void gtk_window_set_default_size(GtkWindow *window, gint width, gint height)
{
	Q_ASSERT(window);

	// Per GTK's documentation, width and size 0 are impossible, so they are changed to 1.
	if (width == 0)
		width = 1;

	if (height == 0)
		height = 1;

	// Special case, per GTK documentation: width/height of -1 means that it goes back to the default size.
	if (width == -1 && height == -1)
	{
		window->resize(window->sizeHint());
		return;
	}

	// GTK's docs don't mention what happens with negative sizes, and I really don't want to imagine how to handle this.
	Q_ASSERT(width > 0 && height > 0);

	window->resize(width, height);
}

void gtk_window_set_modal(GtkWindow *window,gboolean modal)
{
	Q_ASSERT(window);

	window->setWindowModality(modal ? Qt::ApplicationModal : Qt::NonModal);

	// Qt doesn't allow modality to be changed on a visible window, so force a redisplay to
	// emulate this since it seems GTK doesn't care. Hacks forever!
	if (window->isVisible())
	{
		window->hide();
		window->show();
	}
}

#define GtkAccelGroup void
void gtk_window_add_accel_group(GtkWindow *window, GtkAccelGroup *g)
{

}
