#include <gtk/gtk.h>

int main (int argc, char *argv[])
{
  /*-- Declare the GTK Widgets used in the program --*/
  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *entry1;
  GtkWidget *entry2;

  /*--  Initialize GTK --*/
  gtk_init (&argc, &argv);

  /*-- Create the new window --*/
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /*-- Create the entry widgets --*/
  entry1 = gtk_entry_new();
  entry2 = gtk_entry_new();

  /*-- Set entry2 to be non-editable --*/
  gtk_entry_set_editable(GTK_ENTRY(entry2), FALSE);

  /*-- Create the text --*/
  gtk_entry_set_text(GTK_ENTRY(entry1), "Some text...");
  gtk_entry_append_text(GTK_ENTRY(entry1), "Some more text appended");
  gtk_entry_set_text(GTK_ENTRY(entry2), "Some text...");
  gtk_entry_append_text(GTK_ENTRY(entry2), "Some more text appended");

  /*--Create the labels --*/
  label1 = gtk_label_new("Editable text entry widget:");
  label2 = gtk_label_new("Non-editable text entry widget:");
    
  /*-- Create the 2x2 table --*/
  table = gtk_table_new(2, 2, FALSE);

  /*-- Add the items to the table --*/
  gtk_table_attach(GTK_TABLE(table), label1, 0, 1, 0, 1, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), label2, 0, 1, 1, 2, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry1, 1, 2, 0, 1, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry2, 1, 2, 1, 2, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  /*-- Add the table to the window --*/
  gtk_container_add(GTK_CONTAINER(window), table);

  /*-- Set window border to zero so that text area takes up the whole window --*/
  gtk_container_border_width (GTK_CONTAINER (window), 5);

  /*-- Set the window to be 640 x 480 pixels --*/
  gtk_window_set_default_size (GTK_WINDOW(window), 500, 100);

  /*-- Set the window title --*/
  gtk_window_set_title(GTK_WINDOW (window), "GtkEntry");

  /*-- Display the widgets --*/
  gtk_widget_show(table);
  gtk_widget_show(entry1);
  gtk_widget_show(entry2);
  gtk_widget_show(label1);
  gtk_widget_show(label2);
  gtk_widget_show(window);

  /*-- Start the GTK event loop --*/
  gtk_main();

  /*-- Return 0 if exit is successful --*/
  return 0;
}
