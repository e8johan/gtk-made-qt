#include <gtk/gtk.h>

int main (int argc, char *argv[])
{

        /*-- Declare the GTK Widgets used in the program --*/
        GtkWidget *window;
        GtkWidget *combo;
        GList *items = NULL;

        /*--Initialize GTK--*/
        gtk_init (&argc, &argv);

        /*--Create the new window--*/
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

        /*-- Items to be added to combo widget--*/
        items = g_list_append (items, "Banana");
        items = g_list_append (items, "Orange");
        items = g_list_append (items, "Peach");
        items = g_list_append (items, "Strawberry");

        /*--Create a new combo widget --*/
        combo = gtk_combo_new();
        /*-- Set items in the popup list--*/
        gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);

        /*--- Add combo widget to window-*/
        gtk_container_add(GTK_CONTAINER (window), combo);

        /*-- Add a border to the window to give the button a little room --*/
        gtk_container_border_width (GTK_CONTAINER (window), 15);
         
        /*-- Display the widgets --*/
        gtk_widget_show(combo);
        gtk_widget_show(window);

        /*-- Start the GTK event loop--*/
        gtk_main();

        /*--Return 0 if exit is successful--*/
        return 0;

}
