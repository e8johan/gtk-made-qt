/* the module for test-plugin.c: calls GTK from a shared object */
#include <gtk/gtk.h>

G_MODULE_EXPORT GtkWidget *plugin_make (GtkWidget *box)
{
  GtkWidget *l = gtk_label_new ("from the plugin");
  gtk_box_pack_start (GTK_BOX (box), l, FALSE, FALSE, 0);
  return l;
}
