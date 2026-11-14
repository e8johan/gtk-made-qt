/*
 * What Galculator (GTK 2, GtkBuilder UI) needs: builder <accelerator>s that
 * wait for their window, toggle buttons that change state in "clicked"
 * (and "released" after it), radio menu items grouped in the builder file,
 * every keysym name, accelerator queries, tag tables, size groups, font
 * buttons, arrows. Built against the GTK 2 API level, run headless.
 */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

/* a button panel in a file of its own, in a dummy window (as Galculator's
 * are): the program moves the panel and the window's accel group over */
static const char *panel_ui =
  "<interface>"
  " <object class='GtkWindow' id='dummy'><child>"
  " <object class='GtkTable' id='panel'><property name='n_rows'>2</property><property name='n_columns'>1</property>"
  "  <child><object class='GtkToggleButton' id='button_8'><property name='label'>8</property>"
  "    <accelerator key='8' signal='clicked'/><accelerator key='KP_8' signal='clicked'/>"
  "    <signal name='clicked' handler='on_number_clicked'/><signal name='released' handler='on_released'/>"
  "   </object><packing><property name='top_attach'>0</property><property name='bottom_attach'>1</property></packing></child>"
  "  <child><object class='GtkToggleButton' id='button_mul'><property name='label'>*</property>"
  "    <accelerator key='asterisk' signal='clicked'/>"
  "    <signal name='clicked' handler='on_number_clicked'/><signal name='released' handler='on_released'/>"
  "   </object><packing><property name='top_attach'>1</property><property name='bottom_attach'>2</property></packing></child>"
  " </object></child></object>"
  "</interface>";

static const char *menu_ui =
  "<interface><object class='GtkMenu' id='menu'>"
  " <child><object class='GtkRadioMenuItem' id='basic'><property name='label'>Basic</property><property name='active'>True</property></object></child>"
  " <child><object class='GtkRadioMenuItem' id='sci'><property name='label'>Scientific</property><property name='group'>basic</property>"
  "   <signal name='toggled' handler='on_sci_toggled'/></object></child>"
  "</object></interface>";

static int clicks_active, clicks, releases, sci_toggled;
static GString *order;

/* Galculator's handlers: act only when the click switched the button on,
 * and switch it back off on release */
G_MODULE_EXPORT void on_number_clicked (GtkToggleButton *b, gpointer d)
{
  (void) d;
  clicks++;
  if (gtk_toggle_button_get_active (b)) clicks_active++;
  g_string_append (order, "c");
}
G_MODULE_EXPORT void on_released (GtkToggleButton *b, gpointer d)
{
  (void) d;
  releases++;
  g_string_append (order, "r");
  gtk_toggle_button_set_active (b, FALSE);
}
G_MODULE_EXPORT void on_sci_toggled (GtkCheckMenuItem *i, gpointer d) { (void) d; if (gtk_check_menu_item_get_active (i)) sci_toggled++; }

static gboolean finder (GtkAccelKey *key, GClosure *closure, gpointer data)
{
  (void) key;
  return closure == data;
}

static int test_keysyms (void)
{
  CHECK (gdk_keyval_from_name ("asterisk") == GDK_KEY_asterisk);
  CHECK (gdk_keyval_from_name ("KP_8") == GDK_KEY_KP_8);
  CHECK (gdk_keyval_from_name ("parenleft") == '(');
  CHECK (strcmp (gdk_keyval_name (GDK_KEY_KP_Multiply), "KP_Multiply") == 0);
  CHECK (gdk_keyval_from_name ("a") == 'a' && gdk_keyval_from_name ("A") == 'A');
  gchar *l = gtk_accelerator_get_label (GDK_KEY_s, GDK_CONTROL_MASK);
  CHECK (l && strstr (l, "S"));
  g_free (l);
  return 0;
}

static int test_builder_accels (GtkWidget *window)
{
  GtkBuilder *b = gtk_builder_new ();
  CHECK (gtk_builder_add_from_string (b, panel_ui, -1, NULL));
  gtk_builder_connect_signals (b, NULL);
  GtkWidget *panel = GTK_WIDGET (gtk_builder_get_object (b, "panel"));
  GtkWidget *dummy = GTK_WIDGET (gtk_builder_get_object (b, "dummy"));
  GtkWidget *b8 = GTK_WIDGET (gtk_builder_get_object (b, "button_8"));
  CHECK (strcmp (gtk_buildable_get_name (GTK_BUILDABLE (b8)), "button_8") == 0);
  /* the builder put the accelerators in the dummy window's group */
  GList *closures = gtk_widget_list_accel_closures (b8);
  CHECK (g_list_length (closures) == 2);
  GtkAccelGroup *group = gtk_accel_group_from_accel_closure (closures->data);
  CHECK (group && g_slist_find (gtk_accel_groups_from_object (G_OBJECT (dummy)), group));
  GtkAccelKey *key = gtk_accel_group_find (group, finder, closures->data);
  CHECK (key && key->accel_key == GDK_KEY_8 && key->accel_mods == 0);
  g_list_free (closures);
  /* what Galculator does: the group and the panel move to the main window */
  gtk_window_add_accel_group (GTK_WINDOW (window), group);
  g_object_ref (panel);
  gtk_container_remove (GTK_CONTAINER (dummy), panel);
  gtk_container_add (GTK_CONTAINER (window), panel);
  g_object_unref (panel);
  gtk_widget_destroy (dummy);
  CHECK (g_slist_find (gtk_accel_groups_from_object (G_OBJECT (window)), group));

  /* a click as GTK 2 orders it: clicked (active) before released */
  order = g_string_new ("");
  gtk_button_clicked (GTK_BUTTON (b8));
  CHECK (clicks == 1 && clicks_active == 1);
  CHECK (strcmp (order->str, "c") == 0 && releases == 0);   /* gtk_button_clicked: no release */
  CHECK (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b8)));
  /* an accelerator emitting "clicked" toggles the button too */
  g_signal_emit_by_name (b8, "clicked");
  CHECK (clicks == 2 && clicks_active == 1);   /* it was on: now off */
  g_signal_emit_by_name (b8, "clicked");
  CHECK (clicks == 3 && clicks_active == 2);
  g_object_unref (b);
  return 0;
}

static int test_radio_menu (void)
{
  GtkBuilder *b = gtk_builder_new ();
  CHECK (gtk_builder_add_from_string (b, menu_ui, -1, NULL));
  gtk_builder_connect_signals (b, NULL);
  GtkWidget *basic = GTK_WIDGET (gtk_builder_get_object (b, "basic"));
  GtkWidget *sci = GTK_WIDGET (gtk_builder_get_object (b, "sci"));
  CHECK (gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (basic)) == gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (sci)));
  CHECK (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (basic)));
  CHECK (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (sci)));
  gtk_menu_item_activate (GTK_MENU_ITEM (sci));
  CHECK (sci_toggled == 1);
  CHECK (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (basic)));
  g_object_unref (b);
  return 0;
}

static int test_widgets (void)
{
  /* tag tables: lookup, remove; paragraph properties */
  GtkTextBuffer *buf = gtk_text_buffer_new (NULL);
  gtk_text_buffer_create_tag (buf, "result", "justification", GTK_JUSTIFY_RIGHT, "font-desc", NULL, NULL);
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table (buf);
  GtkTextTag *tag = gtk_text_tag_table_lookup (table, "result");
  CHECK (tag && gtk_text_tag_table_get_size (table) == 1);
  gchar *lang = NULL;
  g_object_get (tag, "language", &lang, NULL);
  CHECK (lang != NULL);
  g_free (lang);
  gtk_text_tag_table_remove (table, tag);
  CHECK (gtk_text_tag_table_lookup (table, "result") == NULL);
  g_object_unref (buf);

  GtkWidget *fb = gtk_font_button_new_with_font ("Sans Bold 14");
  CHECK (strcmp (gtk_font_button_get_font_name (GTK_FONT_BUTTON (fb)), "Sans Bold 14") == 0);
  GdkColor red = { 0, 65535, 0, 0 }, got;
  GtkWidget *cb = gtk_color_button_new ();
  gtk_color_button_set_color (GTK_COLOR_BUTTON (cb), &red);
  gtk_color_button_get_color (GTK_COLOR_BUTTON (cb), &got);
  CHECK (got.red == 65535 && got.green == 0);
  GtkWidget *arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  CHECK (GTK_ARROW (arrow)->arrow_type == GTK_ARROW_DOWN);

  GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkWidget *l1 = gtk_label_new ("x"), *l2 = gtk_label_new ("a much longer label");
  gtk_size_group_add_widget (sg, l1);
  gtk_size_group_add_widget (sg, l2);
  g_object_unref (sg);   /* the widgets keep it */
  CHECK (g_slist_length (gtk_size_group_get_widgets (sg)) == 2);
  while (gtk_events_pending ()) gtk_main_iteration ();
  GtkRequisition r1, r2;
  gtk_widget_size_request (l1, &r1);
  gtk_widget_size_request (l2, &r2);
  CHECK (r1.width == r2.width);
  gtk_widget_destroy (l1);
  gtk_widget_destroy (l2);
  gtk_widget_destroy (fb);
  gtk_widget_destroy (cb);
  gtk_widget_destroy (arrow);
  return 0;
}

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (test_keysyms () || test_builder_accels (window) || test_radio_menu () || test_widgets ()) return 1;
  gtk_widget_destroy (window);
  printf ("ok\n");
  return 0;
}
