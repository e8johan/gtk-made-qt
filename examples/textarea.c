#include <gtk/gtk.h>

int main (int argc, char *argv[])
{
  /*-- Declare the GTK Widgets used in the program --*/
  GtkWidget *window;
  GtkWidget *text;

  const gchar *buffer = "This is some sample text";

  /*--  Initialize GTK --*/
  gtk_init (&argc, &argv);

  /*-- Create the new window --*/
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /*-- Create a text area --*/
  text = gtk_text_view_new();

  /*-- Set text area to be editable --*/
  gtk_text_view_set_editable(GTK_TEXT_VIEW (text), TRUE);

  /*-- Add the text area to the window --*/
  gtk_container_add(GTK_CONTAINER(window), text);

  /* Get the buffer */
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW (text));

  gtk_text_buffer_set_text(buf,  "Hello Text View!", -1);

  /*-- Set window border to zero so that text area takes up the whole window --*/
  gtk_container_border_width (GTK_CONTAINER (window), 0);

  /*-- Set the window to be 640 x 200 pixels --*/
  gtk_window_set_default_size (GTK_WINDOW(window), 640, 200);

  /*-- Set the window title --*/
  gtk_window_set_title(GTK_WINDOW (window), "Text Area");

  /*-- Display the widgets --*/
  gtk_widget_show(text);
  gtk_widget_show(window);

  /*-- Start the GTK event loop --*/
  gtk_main();

  /*-- Return 0 if exit is successful --*/
  return 0;
}
