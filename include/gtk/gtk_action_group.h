/* 
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

typedef QActionGroup GtkActionGroup;
//typedef QAction GtkActionEntry;

// This really needs to be a struct thanks to the way userland code (and things like gtk_action_group_add_actions() works).
typedef struct
{
	const gchar     *name;
	const gchar     *stock_id;
	const gchar     *label;
	const gchar     *accelerator;
	const gchar     *tooltip;
	GCallback  callback;
} GtkActionEntry;


GtkActionGroup *gtk_action_group_new(const gchar *name)
{
	// XXX 2: we ignore name, this requires implementing, perhaps via QObject::setProperty()?
	return NULL;
}

void gtk_action_group_add_actions(GtkActionGroup *ag, GtkActionEntry *actions, gint action_count, gpointer user_data)
{

}
