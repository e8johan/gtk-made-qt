#include <gtk/gtk.h>

static GtkWidget *label = NULL;
static GtkWidget *hbox = NULL;

void button_clicked(GtkWidget *widget, gpointer data)
{
	if (label == NULL)
	{
		label = gtk_label_new("I got pressed.");
		gtk_container_add(GTK_CONTAINER(hbox), label);
		gtk_widget_show(label);
	}
	else
	{
		gtk_widget_destroy(label);
		label = NULL;
	}

	g_print("Button clicked\n");
}

void about_clicked(GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg = gtk_about_dialog_new();

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dlg), "GCIF");
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dlg), "free as in beer");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), "1.0");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dlg), "i don't want comments");
	g_signal_connect(G_OBJECT(dlg), "response", G_CALLBACK(gtk_widget_destroy), (gpointer)dlg);

	gtk_widget_show(dlg);
}

int main (int argc, char *argv[])
{
  /*-- Declare the GTK Widgets used in the program --*/
  GtkWidget *button;
  GtkWidget *window;

  /*--  Initialize GTK --*/
  gtk_init (&argc, &argv);

  /*-- Create the new window --*/
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  hbox = gtk_hbox_new(FALSE, 15);

  /*-- Create a button --*/
  button = gtk_button_new_with_label("Click Me");

  g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(button_clicked), NULL);

  /*-- Add the button to the window --*/
  gtk_container_add(GTK_CONTAINER (hbox), button);

  /* Add a second one.. */
  button = gtk_button_new_with_label("View About dialog");

  g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(about_clicked), NULL);

  gtk_container_add(GTK_CONTAINER(hbox), button);

  /*-- Add a border to the window to give the button a little room --*/
  gtk_container_border_width (GTK_CONTAINER (hbox), 15);

  /* Add layout to the window */
  gtk_container_add(GTK_CONTAINER(window), hbox);

  /*-- Display the widgets --*/
  gtk_widget_show(button);
  gtk_widget_show(hbox);
  gtk_widget_show(window);

  /*-- Start the GTK event loop --*/
  gtk_main();

  /*-- Return 0 if exit is successful --*/
  return 0;
}
