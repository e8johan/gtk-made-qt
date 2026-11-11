#include <gtk/gtk.h>

int main (int argc, char *argv[])
{
  /*-- Declare the GTK Widgets used in the program --*/
  GtkWidget *window;
  GtkWidget *toggle1;
  GtkWidget *toggle2;
  GtkWidget *toggle3;
  GtkWidget *toggle4;
  GtkWidget *toggle5;
  GtkWidget *hbox;
  
  /*--  Initialize GTK --*/
  gtk_init (&argc, &argv);

  /*-- Create the new window --*/
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /*-- Create the hbox --*/
  hbox = gtk_hbox_new(FALSE, 0);

  /*-- Create the first radio button with a label --*/
  toggle1 = gtk_toggle_button_new_with_label ("Toggle 1");
  toggle2 = gtk_toggle_button_new_with_label ("Toggle 2");
  toggle3 = gtk_toggle_button_new_with_label ("Toggle 3");    
  toggle4 = gtk_toggle_button_new_with_label ("Toggle 4");
  toggle5 = gtk_toggle_button_new_with_label ("Toggle 5");    
  
  /*-- Pack all the radio buttons into the hbox --*/
  gtk_box_pack_start(GTK_BOX(hbox), toggle1, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), toggle2, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), toggle3, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), toggle4, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), toggle5, TRUE, TRUE, 2);


  /*-- Add the hbox to the window --*/
  gtk_container_add(GTK_CONTAINER(window), hbox);

  /*-- Add a border to the window to give the button a little room --*/
  gtk_container_border_width (GTK_CONTAINER (window), 15);

  /*-- Display the widgets --*/
  gtk_widget_show(toggle1);
  gtk_widget_show(toggle2);
  gtk_widget_show(toggle3);
  gtk_widget_show(toggle4);
  gtk_widget_show(toggle5);
  gtk_widget_show(hbox);
  gtk_widget_show(window);

  /*-- Start the GTK event loop --*/
  gtk_main();

  /*-- Return 0 if exit is successful --*/
  return 0;
}
