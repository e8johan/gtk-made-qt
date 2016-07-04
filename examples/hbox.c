#include <gtk/gtk.h>

void buttonone_click(GtkWidget *button, gpointer data)
{
	g_print("One clicked\n");
}
void buttontwo_click(GtkWidget *button, gpointer data)
{
	g_print("Two clicked\n");
}
void buttonthree_click(GtkWidget *button, gpointer data)
{
	g_print("Three clicked\n");
}

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
  GtkWidget *hbox;

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
  g_signal_connect(GTK_OBJECT(button1), "clicked", G_CALLBACK(buttonone_click), NULL);
  button2 = gtk_button_new_with_label("Button 2");
  g_signal_connect(GTK_OBJECT(button2), "clicked", G_CALLBACK(buttontwo_click), NULL);
  button3 = gtk_button_new_with_label("Button 3");
  g_signal_connect(GTK_OBJECT(button3), "clicked", G_CALLBACK(buttonthree_click), NULL);

  /*-- Create the hbox --*/
  hbox = gtk_hbox_new(FALSE,0);

  /*-- Add all the buttons to the vbox --*/
  gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 2); 
  gtk_box_pack_start(GTK_BOX(hbox), button1, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), label2, FALSE, FALSE, 2); 
  gtk_box_pack_start(GTK_BOX(hbox), button2, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), label3, FALSE, FALSE, 2);   
  gtk_box_pack_start(GTK_BOX(hbox), button3, FALSE, FALSE, 2);
   
  /*-- Add the button to the window --*/
  gtk_container_add(GTK_CONTAINER (window), hbox);

  /*-- Add a border to the window to give the buttons a little room --*/
  gtk_container_border_width (GTK_CONTAINER (window), 15);

  /*-- Display the widgets --*/
  gtk_widget_show(hbox);
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
