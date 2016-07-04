#include <gtk/gtk.h>

int main (int argc, char *argv[])
{
  /*-- Declare the GTK Widgets used in the program --*/
  GtkWidget *window;
  GtkWidget *check1;
  GtkWidget *check2;
  GtkWidget *check3;
  GtkWidget *hbox;
  
  /*--  Initialize GTK --*/
  gtk_init (&argc, &argv);

  /*-- Create the new window --*/
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /*-- Create the hbox --*/
  hbox = gtk_hbox_new(FALSE, 0);

  /*-- Create the first radio button with a label --*/
  check1 = gtk_check_button_new_with_label ("Check 1");
  check2 = gtk_check_button_new_with_label ("Check 2");
  check3 = gtk_check_button_new_with_label ("Check 3");    
  
  /*-- Pack all the radio buttons into the hbox --*/
  gtk_box_pack_start(GTK_BOX(hbox), check1, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), check2, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), check3, TRUE, TRUE, 2);

  /*-- Add the hbox to the window --*/
  gtk_container_add(GTK_CONTAINER(window), hbox);

  /*-- Add a border to the window to give the button a little room --*/
  gtk_container_border_width (GTK_CONTAINER (window), 15);

  /*-- Display the widgets --*/
  gtk_widget_show(check1);
  gtk_widget_show(check2);
  gtk_widget_show(check3);
  gtk_widget_show(hbox);
  gtk_widget_show(window);

  /*-- Start the GTK event loop --*/
  gtk_main();

  /*-- Return 0 if exit is successful --*/
  return 0;
}
