#include <gtk/gtk.h>

/*-- This function allows the program to exit properly when the window is closed --*/
gint destroyapp (GtkWidget *widget, gpointer gdata)
{
  g_print ("Quitting...\n");
  gtk_main_quit();
  return (FALSE);
}

/*-- This function allows the program to exit properly when the window is closed --*/
gint ClosingAppWindow (GtkWidget *widget, gpointer gdata)
{
  g_print ("Quitting...\n");
  gtk_main_quit();
  return (FALSE);
}

int main (int argc, char *argv[])
{
  /*-- Declare the GTK Widgets used in the program --*/
  GtkWidget *window;
  GtkWidget *menuFile;
  GtkWidget *menuEdit;
  GtkWidget *menuHelp;  
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *vbox;
  GtkWidget *text;

  /*--  Initialize GTK --*/
  gtk_init (&argc, &argv);

  /*-- Create the new window --*/
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /*-- Create the vbox --*/
  vbox = gtk_vbox_new(FALSE, 0);

  /*-- Create a text area --*/
  text = gtk_text_view_new();

  /*-- Set text area to be editable --*/
  gtk_text_view_set_editable(GTK_TEXT_VIEW (text), TRUE);

  /*-- Connect the window to the destroyapp function  --*/
  g_signal_connect(GTK_OBJECT(window), "delete_event", G_CALLBACK(destroyapp), NULL);

  /*-- Create the menu bar --*/
  menubar = gtk_menu_bar_new();

  /*-- Add the menubar to the vbox --*/
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show(menubar);

  /*-- Add the text area to the window --*/
  gtk_container_add(GTK_CONTAINER(vbox), text);  

  /*-- Add the vbox to the main window --*/
  gtk_container_add(GTK_CONTAINER(window), vbox);
  
  /*---------------- Create File menu items ------------------*/

  menuFile = gtk_menu_item_new_with_label ("File");
  gtk_menu_bar_append (GTK_MENU_BAR(menubar), menuFile);
  gtk_widget_show(menuFile);

  /*-- Create File submenu  --*/
  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuFile), menu);

  /*-- Create New menu item under File submenu --*/
  menuitem = gtk_menu_item_new_with_label ("New");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_widget_show (menuitem);

  /*-- Create Open menu item under File submenu --*/
  menuitem = gtk_menu_item_new_with_label ("Open");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_widget_show (menuitem);

  /*-- Create Exit menu item under File submenu --*/
  menuitem = gtk_menu_item_new_with_label ("Exit");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  g_signal_connect(GTK_OBJECT (menuitem), "activate", G_CALLBACK (ClosingAppWindow), NULL);
  gtk_widget_show (menuitem);
  /*---------------- End File menu declarations ----------------*/

  /*---------------- Create Edit menu items --------------------*/

  menuEdit = gtk_menu_item_new_with_label ("Edit");
  gtk_menu_bar_append (GTK_MENU_BAR(menubar), menuEdit);
  gtk_widget_show(menuEdit);

  /*-- Create File submenu --*/
  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuEdit), menu);

  /*-- Create Undo menu item under Edit submenu --*/
  menuitem = gtk_menu_item_new_with_label ("Undo");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_widget_show (menuitem);

  /*-- Create Copy menu item under File submenu --*/
  menuitem = gtk_menu_item_new_with_label ("Copy");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_widget_show (menuitem);

  /*-- Create Cut menu item under File submenu --*/
  menuitem = gtk_menu_item_new_with_label ("Cut");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_widget_show (menuitem);
  /*---------------- End Edit menu declarations ----------------*/

  /*---------------- Start Help menu declarations ----------------*/
  menuHelp = gtk_menu_item_new_with_label ("Help");
  gtk_menu_bar_append (GTK_MENU_BAR(menubar), menuHelp);
  gtk_widget_show(menuHelp);

  /*-- Create Help submenu --*/
  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuHelp), menu);

  /*-- Create About menu item under Help submenu --*/
  menuitem = gtk_menu_item_new_with_label ("About");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_widget_show (menuitem);
  /*---------------- End Help menu declarations ----------------*/

  /*-- Set window border to zero so that text area takes up the whole window --*/
  gtk_container_border_width (GTK_CONTAINER (window), 0);

  /*-- Set the window to be 640 x 480 pixels --*/
  gtk_window_set_default_size (GTK_WINDOW(window), 640, 200);

  /*-- Set the window title --*/
  gtk_window_set_title(GTK_WINDOW (window), "Text Area");

  /*-- Display the widgets --*/
  gtk_widget_show(text);
  gtk_widget_show(menuitem);
  gtk_widget_show(menuFile);
  gtk_widget_show(menuEdit);
  gtk_widget_show(menuHelp);
  gtk_widget_show(vbox);
  gtk_widget_show(window);

  /*-- Start the GTK event loop --*/
  gtk_main();

  /*-- Return 0 if exit is successful --*/
  return 0;
}

