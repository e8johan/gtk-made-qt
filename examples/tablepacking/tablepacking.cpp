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

// TODO #include <gtk/gtk.h>
#include "gtk-made-qt.h"

/* Our callback.
 * The data passed to this function is printed to stdout */
static void callback( GtkWidget *widget,
                      gpointer   data )
{
    g_print ("Hello again - %s was pressed\n", (char *) data);
}

/* This callback quits the program */
static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    gtk_main_quit ();
    return FALSE;
}

int main( int   argc,
          char *argv[] )
{
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *table;

    gtk_init (&argc, &argv);

    /* Create a new window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* Set the window title */
    gtk_window_set_title (GTK_WINDOW (window), "Table");

    /* Set a handler for delete_event that immediately
     * exits GTK. */
    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (delete_event), NULL);

    /* Sets the border width of the window. */
    gtk_container_set_border_width (GTK_CONTAINER (window), 20);

    /* Create a 2x2 table */
    table = gtk_table_new (2, 2, TRUE);

    /* Put the table in the main window */
    gtk_container_add (GTK_CONTAINER (window), table);

    /* Create first button */
    button = gtk_button_new_with_label ("button 1");

    /* When the button is clicked, we call the "callback" function
     * with a pointer to "button 1" as its argument */
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (callback), (gpointer) "button 1");


    /* Insert button 1 into the upper left quadrant of the table */
    gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 0, 1);

    gtk_widget_show (button);

    /* Create second button */

    button = gtk_button_new_with_label ("button 2");

    /* When the button is clicked, we call the "callback" function
     * with a pointer to "button 2" as its argument */
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (callback), (gpointer) "button 2");
    /* Insert button 2 into the upper right quadrant of the table */
    gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 0, 1);

    gtk_widget_show (button);

    /* Create "Quit" button */
    button = gtk_button_new_with_label ("Quit");

    /* When the button is clicked, we call the "delete_event" function
     * and the program exits */
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (delete_event), NULL);

    /* Insert the quit button into the both
     * lower quadrants of the table */
    gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 1, 2);

    gtk_widget_show (button);

    gtk_widget_show (table);
    gtk_widget_show (window);

    gtk_main ();

    return 0;
}
