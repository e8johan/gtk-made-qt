/*
 * Programs with plugins (XMMS ...) link the shared library: the program
 * and a dlopen'ed module that both call GTK must share one toolkit.
 */
#include <gtk/gtk.h>
#include <gmodule.h>
#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

typedef GtkWidget *(*MakeFunc) (GtkWidget *box);

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  CHECK (argc > 1);
  GModule *m = g_module_open (argv[1], G_MODULE_BIND_LOCAL);
  CHECK (m != NULL);
  MakeFunc make = NULL;
  CHECK (g_module_symbol (m, "plugin_make", (gpointer *) &make));
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  GtkWidget *label = make (box);
  CHECK (GTK_IS_LABEL (label));                       /* same GType registry */
  CHECK (label->parent == box);
  gtk_widget_show_all (window);
  while (gtk_events_pending ()) gtk_main_iteration ();
  printf ("plugin test passed\n");
  return 0;
}
