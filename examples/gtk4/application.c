/* GTK 4: GtkApplication with "app." and "win." actions, a GMenu menubar,
 * keyboard accelerators, a header bar, a stack with a switcher, a drop-down,
 * a switch and an async alert dialog.
 * Build with real GTK:  gcc application.c $(pkg-config --cflags --libs gtk4)
 */
#include <gtk/gtk.h>

static void quit_activated (GSimpleAction *action, GVariant *param, gpointer app)
{
  g_application_quit (G_APPLICATION (app));
  (void) action; (void) param;
}

static void about_activated (GSimpleAction *action, GVariant *param, gpointer win)
{
  const char *authors[] = { "Johan Thelin", "Robin Burchell", NULL };
  gtk_show_about_dialog (GTK_WINDOW (win), "program-name", "GTK made Qt", "version", "0.2",
                         "comments", "A GTK 4 program running on Qt", "authors", authors,
                         "license-type", GTK_LICENSE_LGPL_2_1, NULL);
  (void) action; (void) param;
}

static void greet_activated (GSimpleAction *action, GVariant *param, gpointer win)
{
  GtkAlertDialog *dlg = gtk_alert_dialog_new ("Hello from a GTK 4 action");
  gtk_alert_dialog_set_detail (dlg, "This message box is a QMessageBox.");
  gtk_alert_dialog_show (dlg, GTK_WINDOW (win));
  g_object_unref (dlg);
  (void) action; (void) param;
}

static void flavour_changed (GObject *dropdown, GParamSpec *pspec, gpointer label)
{
  GtkStringObject *item = gtk_drop_down_get_selected_item (GTK_DROP_DOWN (dropdown));
  char *text = g_strdup_printf ("You picked: %s", item ? gtk_string_object_get_string (item) : "nothing");
  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);
  (void) pspec;
}

static void switch_changed (GObject *sw, GParamSpec *pspec, gpointer label)
{
  gtk_label_set_text (GTK_LABEL (label), gtk_switch_get_active (GTK_SWITCH (sw)) ? "Dark mode: on" : "Dark mode: off");
  (void) pspec;
}

static GtkWidget *settings_page (void)
{
  GtkWidget *grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 8);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_widget_set_margin_start (grid, 18);
  gtk_widget_set_margin_end (grid, 18);
  gtk_widget_set_margin_top (grid, 18);
  gtk_widget_set_margin_bottom (grid, 18);

  const char *flavours[] = { "Vanilla", "Chocolate", "Pistachio", NULL };
  GtkWidget *dd = gtk_drop_down_new_from_strings (flavours);
  GtkWidget *picked = gtk_label_new ("You picked: Vanilla");
  gtk_label_set_xalign (GTK_LABEL (picked), 0);
  g_signal_connect (dd, "notify::selected", G_CALLBACK (flavour_changed), picked);

  GtkWidget *sw = gtk_switch_new ();
  gtk_widget_set_halign (sw, GTK_ALIGN_START);
  GtkWidget *sw_label = gtk_label_new ("Dark mode: off");
  gtk_label_set_xalign (GTK_LABEL (sw_label), 0);
  g_signal_connect (sw, "notify::active", G_CALLBACK (switch_changed), sw_label);

  gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Flavour"), 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), dd, 1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), picked, 1, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Theme"), 0, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), sw, 1, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), sw_label, 1, 3, 1, 1);
  return grid;
}

static GtkWidget *welcome_page (void)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
  GtkWidget *title = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (title), "<span size='x-large' weight='bold'>Welcome</span>");
  GtkWidget *text = gtk_label_new ("This window is a GtkApplicationWindow.\nTry the menus, Ctrl+Q, and F1.");
  gtk_label_set_justify (GTK_LABEL (text), GTK_JUSTIFY_CENTER);
  GtkWidget *button = gtk_button_new_with_mnemonic ("_Say hello");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.greet");
  gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (button, "suggested-action");
  gtk_box_append (GTK_BOX (box), title);
  gtk_box_append (GTK_BOX (box), text);
  gtk_box_append (GTK_BOX (box), button);
  return box;
}

static void activate (GtkApplication *app, gpointer user_data)
{
  GtkWidget *win = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (win), "GTK 4 on Qt");
  gtk_window_set_default_size (GTK_WINDOW (win), 460, 300);

  const GActionEntry win_entries[] = {
    { "about", about_activated, NULL, NULL, NULL, { 0 } },
    { "greet", greet_activated, NULL, NULL, NULL, { 0 } },
  };
  g_action_map_add_action_entries (G_ACTION_MAP (win), win_entries, G_N_ELEMENTS (win_entries), win);
  gtk_application_window_set_show_menubar (GTK_APPLICATION_WINDOW (win), TRUE);

  GtkWidget *stack = gtk_stack_new ();
  gtk_stack_add_titled (GTK_STACK (stack), welcome_page (), "welcome", "Welcome");
  gtk_stack_add_titled (GTK_STACK (stack), settings_page (), "settings", "Settings");
  GtkWidget *switcher = gtk_stack_switcher_new ();
  gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (switcher), GTK_STACK (stack));

  GtkWidget *header = gtk_header_bar_new ();
  gtk_header_bar_set_title_widget (GTK_HEADER_BAR (header), switcher);
  GtkWidget *quit = gtk_button_new_from_icon_name ("application-exit");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (quit), "app.quit");
  gtk_widget_set_tooltip_text (quit, "Quit");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (header), quit);
  gtk_window_set_titlebar (GTK_WINDOW (win), header);

  gtk_window_set_child (GTK_WINDOW (win), stack);
  gtk_window_present (GTK_WINDOW (win));
  (void) user_data;
}

static void startup (GApplication *app, gpointer user_data)
{
  const GActionEntry app_entries[] = { { "quit", quit_activated, NULL, NULL, NULL, { 0 } } };
  g_action_map_add_action_entries (G_ACTION_MAP (app), app_entries, G_N_ELEMENTS (app_entries), app);
  const char *quit_accels[] = { "<Control>q", NULL };
  const char *about_accels[] = { "F1", NULL };
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "app.quit", quit_accels);
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.about", about_accels);

  GMenu *menubar = g_menu_new ();
  GMenu *file = g_menu_new ();
  g_menu_append (file, "_Quit", "app.quit");
  g_menu_append_submenu (menubar, "_File", G_MENU_MODEL (file));
  GMenu *help = g_menu_new ();
  g_menu_append (help, "Say _hello", "win.greet");
  g_menu_append (help, "_About", "win.about");
  g_menu_append_submenu (menubar, "_Help", G_MENU_MODEL (help));
  gtk_application_set_menubar (GTK_APPLICATION (app), G_MENU_MODEL (menubar));
  g_object_unref (file);
  g_object_unref (help);
  g_object_unref (menubar);
  (void) user_data;
}

int main (int argc, char *argv[])
{
  GtkApplication *app = gtk_application_new ("org.example.GtkMadeQt", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "startup", G_CALLBACK (startup), NULL);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}
