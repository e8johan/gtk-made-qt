#include <gtk/gtk.h>

int main (int argc, char *argv[])
{
  /*-- Declare the GTK Widgets used in the program --*/
  GtkWidget *window;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;
  GtkWidget *vbox;

  /*--  Initialize GTK --*/
  gtk_init (&argc, &argv);

  /*-- Create the new window --*/
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /*-- Create the labels --*/
  label1 = gtk_label_new("Label1");
  label2 = gtk_label_new("Label2");
  label3 = gtk_label_new("Label3");

  /*-- Create some buttons to fill the vbox with --*/
  button1 = gtk_button_new_with_label("Button 1");
  button2 = gtk_button_new_with_label("Button 2");
  button3 = gtk_button_new_with_label("Button 3");

  /*-- Create the vbox --*/
  vbox = gtk_vbox_new(FALSE,0);

  /*-- Add all the buttons to the vbox --*/
  gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 0); 
  gtk_box_pack_start(GTK_BOX(vbox), button1, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), label2, FALSE, FALSE, 0); 
  gtk_box_pack_start(GTK_BOX(vbox), button2, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), label3, FALSE, FALSE, 0);   
  gtk_box_pack_start(GTK_BOX(vbox), button3, FALSE, FALSE, 0);
   
  /*-- Add the button to the window --*/
  gtk_container_add(GTK_CONTAINER (window), vbox);

  /*-- Add a border to the window to give the buttons a little room --*/
  gtk_container_border_width (GTK_CONTAINER (window), 15);

  /*-- Display the widgets --*/
  gtk_widget_show(vbox);
  gtk_widget_show(label1);
  gtk_widget_show(label2);
  gtk_widget_show(label3);
  gtk_widget_show(button1);
  gtk_widget_show(button2);
  gtk_widget_show(button3);
  gtk_widget_show(window);

  /*-- Start the GTK event loop --*/
  gtk_main();

  /*-- Return 0 if exit is successful --*/
  return 0;
}
