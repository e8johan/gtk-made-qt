/* GTK 3: a window described in GtkBuilder XML (as Glade writes it), with
 * handlers connected by name through gtk_builder_connect_signals().
 * Build with real GTK:  gcc builder.c $(pkg-config --cflags --libs gtk+-3.0) -rdynamic
 */
#include <gtk/gtk.h>

static const char *ui =
  "<?xml version='1.0' encoding='UTF-8'?>"
  "<interface>"
  "  <requires lib='gtk+' version='3.20'/>"
  "  <object class='GtkWindow' id='window'>"
  "    <property name='title'>GtkBuilder on Qt</property>"
  "    <property name='border-width'>12</property>"
  "    <signal name='destroy' handler='gtk_main_quit' swapped='no'/>"
  "    <child>"
  "      <object class='GtkGrid' id='grid'>"
  "        <property name='row-spacing'>6</property>"
  "        <property name='column-spacing'>12</property>"
  "        <child>"
  "          <object class='GtkLabel'><property name='label'>_Name:</property>"
  "            <property name='use-underline'>True</property><property name='xalign'>1</property></object>"
  "          <packing><property name='left-attach'>0</property><property name='top-attach'>0</property></packing>"
  "        </child>"
  "        <child>"
  "          <object class='GtkEntry' id='name'><property name='hexpand'>True</property>"
  "            <property name='placeholder-text'>Your name</property></object>"
  "          <packing><property name='left-attach'>1</property><property name='top-attach'>0</property></packing>"
  "        </child>"
  "        <child>"
  "          <object class='GtkLabel'><property name='label'>Level:</property><property name='xalign'>1</property></object>"
  "          <packing><property name='left-attach'>0</property><property name='top-attach'>1</property></packing>"
  "        </child>"
  "        <child>"
  "          <object class='GtkComboBoxText' id='level'>"
  "            <property name='active'>1</property>"
  "            <items><item>Beginner</item><item>Hobbyist</item><item>Expert</item></items>"
  "          </object>"
  "          <packing><property name='left-attach'>1</property><property name='top-attach'>1</property></packing>"
  "        </child>"
  "        <child>"
  "          <object class='GtkCheckButton' id='news'><property name='label'>Subscribe to news</property></object>"
  "          <packing><property name='left-attach'>1</property><property name='top-attach'>2</property></packing>"
  "        </child>"
  "        <child>"
  "          <object class='GtkButton' id='greet'>"
  "            <property name='label'>_Greet</property><property name='use-underline'>True</property>"
  "            <signal name='clicked' handler='on_greet_clicked' swapped='no'/>"
  "          </object>"
  "          <packing><property name='left-attach'>1</property><property name='top-attach'>3</property></packing>"
  "        </child>"
  "        <child>"
  "          <object class='GtkLabel' id='result'><property name='label'> </property></object>"
  "          <packing><property name='left-attach'>0</property><property name='top-attach'>4</property>"
  "                   <property name='width'>2</property></packing>"
  "        </child>"
  "      </object>"
  "    </child>"
  "  </object>"
  "</interface>";

static GtkBuilder *builder;

/* Exported so gtk_builder_connect_signals() can find it by name. */
G_MODULE_EXPORT void on_greet_clicked (GtkButton *button, gpointer user_data)
{
  GtkEntry *name = GTK_ENTRY (gtk_builder_get_object (builder, "name"));
  GtkComboBoxText *level = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "level"));
  GtkToggleButton *news = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "news"));
  GtkLabel *result = GTK_LABEL (gtk_builder_get_object (builder, "result"));
  gchar *lvl = gtk_combo_box_text_get_active_text (level);
  gchar *msg = g_markup_printf_escaped ("Hello <b>%s</b>, %s%s!",
                                        *gtk_entry_get_text (name) ? gtk_entry_get_text (name) : "stranger",
                                        lvl ? lvl : "?", gtk_toggle_button_get_active (news) ? " (subscribed)" : "");
  gtk_label_set_markup (result, msg);
  g_print ("%s\n", gtk_label_get_text (result));
  g_free (msg);
  g_free (lvl);
  (void) button;
  (void) user_data;
}

int main (int argc, char *argv[])
{
  gtk_init (&argc, &argv);
  builder = gtk_builder_new_from_string (ui, -1);
  gtk_builder_connect_signals (builder, NULL);
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "name")), "Johan");
  on_greet_clicked (NULL, NULL);
  gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (builder, "window")));
  gtk_main ();
  g_object_unref (builder);
  return 0;
}
