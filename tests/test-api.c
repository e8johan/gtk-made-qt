/*
 * Behavioural tests for gtk-made-qt. Plain C, built three times (GTK 2, 3
 * and 4 API levels) and run headless with QT_QPA_PLATFORM=offscreen.
 */
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

static int counter;
static void count_cb (GtkWidget *w, gpointer data) { (void) w; counter += GPOINTER_TO_INT (data); }

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

static int test_signals_and_buttons (void)
{
  GtkWidget *b = gtk_button_new_with_label ("Press");
  g_object_ref_sink (b);
  counter = 0;
  g_signal_connect (b, "clicked", G_CALLBACK (count_cb), GINT_TO_POINTER (1));
  g_signal_connect (b, "clicked", G_CALLBACK (count_cb), GINT_TO_POINTER (10));
  gtk_button_clicked (GTK_BUTTON (b));
  CHECK (counter == 11);
  CHECK (strcmp (gtk_button_get_label (GTK_BUTTON (b)), "Press") == 0);
  g_object_unref (b);

  GtkWidget *t = gtk_check_button_new_with_label ("Check");
  g_object_ref_sink (t);
  counter = 0;
  g_signal_connect (t, "toggled", G_CALLBACK (count_cb), GINT_TO_POINTER (1));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (t), TRUE);
  CHECK (counter == 1);
  CHECK (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t)));
  CHECK (GTK_TOGGLE_BUTTON (t)->active);   /* GTK 2 public field is kept in sync */
  gtk_button_clicked (GTK_BUTTON (t));     /* clicking toggles */
  CHECK (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t)));
  g_object_unref (t);
  return 0;
}

static int destroyed_count;
static void on_destroy (GtkWidget *w, gpointer d) { (void) w; (void) d; destroyed_count++; }

static int test_containers_and_destroy (void)
{
#if GTK_MAJOR_VERSION >= 4
  GtkWidget *win = gtk_window_new ();
#else
  GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  GtkWidget *l1 = gtk_label_new ("one"), *l2 = gtk_label_new ("two");
  destroyed_count = 0;
  g_signal_connect (l1, "destroy", G_CALLBACK (on_destroy), NULL);
  g_signal_connect (box, "destroy", G_CALLBACK (on_destroy), NULL);
#if GTK_MAJOR_VERSION >= 4
  gtk_box_append (GTK_BOX (box), l1);
  gtk_box_append (GTK_BOX (box), l2);
  gtk_window_set_child (GTK_WINDOW (win), box);
  CHECK (gtk_widget_get_first_child (box) == l1);
  CHECK (gtk_widget_get_next_sibling (l1) == l2);
#else
  gtk_box_pack_start (GTK_BOX (box), l1, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (box), l2, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (win), box);
  GList *kids = gtk_container_get_children (GTK_CONTAINER (box));
  CHECK (g_list_length (kids) == 2);
  g_list_free (kids);
  CHECK (gtk_bin_get_child (GTK_BIN (win)) == box);
#endif
  CHECK (gtk_widget_get_parent (l1) == box);
  CHECK (gtk_widget_get_toplevel (l1) == win);
  CHECK (strcmp (gtk_label_get_text (GTK_LABEL (l2)), "two") == 0);
  g_object_ref (l2);
#if GTK_MAJOR_VERSION >= 4
  gtk_box_remove (GTK_BOX (box), l2);
#else
  gtk_container_remove (GTK_CONTAINER (box), l2);
#endif
  CHECK (gtk_widget_get_parent (l2) == NULL);
  g_object_unref (l2);   /* last reference: finalized */
  gtk_widget_destroy (win);   /* destroys children too */
  CHECK (destroyed_count == 2);
  return 0;
}

static int test_entry_and_adjustment (void)
{
  GtkWidget *e = gtk_entry_new ();
  g_object_ref_sink (e);
  counter = 0;
  g_signal_connect (e, "changed", G_CALLBACK (count_cb), GINT_TO_POINTER (1));
#if GTK_MAJOR_VERSION >= 4
  gtk_editable_set_text (GTK_EDITABLE (e), "hello");
  CHECK (strcmp (gtk_editable_get_text (GTK_EDITABLE (e)), "hello") == 0);
#else
  gtk_entry_set_text (GTK_ENTRY (e), "hello");
  CHECK (strcmp (gtk_entry_get_text (GTK_ENTRY (e)), "hello") == 0);
#endif
  CHECK (counter == 1);
  g_object_unref (e);

  GtkAdjustment *adj = GTK_ADJUSTMENT (gtk_adjustment_new (5, 0, 100, 1, 10, 0));
  GtkWidget *scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adj);
  g_object_ref_sink (scale);
  counter = 0;
  g_signal_connect (scale, "value-changed", G_CALLBACK (count_cb), GINT_TO_POINTER (1));
  gtk_adjustment_set_value (adj, 42);
  CHECK (counter == 1);
  CHECK (gtk_range_get_value (GTK_RANGE (scale)) == 42);
  CHECK (adj->value == 42);   /* GTK 2 public field */
  gtk_adjustment_set_value (adj, 1000);   /* clamped */
  CHECK (gtk_adjustment_get_value (adj) == 100);
  g_object_unref (scale);

  GtkWidget *spin = gtk_spin_button_new_with_range (0, 10, 0.5);
  g_object_ref_sink (spin);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 2.5);
  CHECK (gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin)) == 2.5);
  g_object_unref (spin);
  return 0;
}

static int test_text_buffer (void)
{
  GtkWidget *view = gtk_text_view_new ();
  g_object_ref_sink (view);
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  GtkTextIter start, end;
  gtk_text_buffer_set_text (buf, "Hello\nworld", -1);
  CHECK (gtk_text_buffer_get_line_count (buf) == 2);
  gtk_text_buffer_get_end_iter (buf, &end);
  gtk_text_buffer_insert (buf, &end, "!", -1);
  gtk_text_buffer_get_bounds (buf, &start, &end);
  gchar *txt = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
  CHECK (strcmp (txt, "Hello\nworld!") == 0);
  g_free (txt);
  GtkTextIter m1, m2;
  CHECK (gtk_text_iter_forward_search (&start, "world", 0, &m1, &m2, NULL));
  CHECK (gtk_text_iter_get_offset (&m1) == 6);
  CHECK (gtk_text_iter_get_line (&m1) == 1);
  GtkTextTag *bold = gtk_text_buffer_create_tag (buf, "bold", "weight", 700, NULL);
  CHECK (bold != NULL);
  gtk_text_buffer_apply_tag_by_name (buf, "bold", &m1, &m2);
  g_object_unref (view);
  return 0;
}

static int test_tree_store (void)
{
  GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
  GtkTreeIter it;
  gtk_list_store_append (store, &it);
  gtk_list_store_set (store, &it, 0, "banana", 1, 3, -1);
  gtk_list_store_append (store, &it);
  gtk_list_store_set (store, &it, 0, "apple", 1, 7, -1);
  CHECK (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL) == 2);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), 0, GTK_SORT_ASCENDING);
  CHECK (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &it));
  gchar *name; gint n;
  gtk_tree_model_get (GTK_TREE_MODEL (store), &it, 0, &name, 1, &n, -1);
  CHECK (strcmp (name, "apple") == 0 && n == 7);
  g_free (name);

  GtkWidget *view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_ref_sink (view);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view),
    gtk_tree_view_column_new_with_attributes ("Fruit", gtk_cell_renderer_text_new (), "text", 0, NULL));
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_select_iter (sel, &it);
  GtkTreeModel *model;
  GtkTreeIter sel_it;
  CHECK (gtk_tree_selection_get_selected (sel, &model, &sel_it));
  GtkTreePath *p = gtk_tree_model_get_path (model, &sel_it);
  gchar *ps = gtk_tree_path_to_string (p);
  CHECK (strcmp (ps, "0") == 0);
  g_free (ps);
  gtk_tree_path_free (p);
  CHECK (gtk_list_store_remove (store, &it));   /* iter moves to the next row */
  CHECK (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL) == 1);
  g_object_unref (view);
  g_object_unref (store);
  return 0;
}

static gboolean respond_later (gpointer dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  return G_SOURCE_REMOVE;
}

#if GTK_MAJOR_VERSION < 4
static int test_dialog_run (void)
{
  GtkWidget *d = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, "Proceed %d?", 1);
  g_timeout_add (50, respond_later, d);
  gint r = gtk_dialog_run (GTK_DIALOG (d));
  CHECK (r == GTK_RESPONSE_OK);
  gtk_widget_destroy (d);
  return 0;
}
#endif

static const char *ui =
  "<interface>"
  "  <object class='GtkWindow' id='win'>"
  "    <property name='title'>Built</property>"
  "    <child>"
  "      <object class='GtkBox' id='box'>"
  "        <property name='orientation'>vertical</property>"
  "        <property name='spacing'>6</property>"
  "        <child><object class='GtkLabel' id='label'><property name='label'>From XML</property></object></child>"
  "        <child><object class='GtkButton' id='button'><property name='label'>_Go</property>"
  "               <property name='use-underline'>True</property></object></child>"
  "      </object>"
  "    </child>"
  "  </object>"
  "  <object class='GtkAdjustment' id='adj'><property name='upper'>50</property><property name='value'>25</property></object>"
  "  <menu id='menubar'><submenu><attribute name='label'>_File</attribute>"
  "    <item><attribute name='label'>_Quit</attribute><attribute name='action'>app.quit</attribute></item></submenu></menu>"
  "</interface>";

static int test_builder (void)
{
  GtkBuilder *b = gtk_builder_new_from_string (ui, -1);
  GObject *win = gtk_builder_get_object (b, "win");
  CHECK (GTK_IS_WINDOW (win));
  CHECK (strcmp (gtk_window_get_title (GTK_WINDOW (win)), "Built") == 0);
  GObject *label = gtk_builder_get_object (b, "label");
  CHECK (GTK_IS_LABEL (label));
  CHECK (gtk_widget_get_parent (GTK_WIDGET (label)) == GTK_WIDGET (gtk_builder_get_object (b, "box")));
  CHECK (gtk_adjustment_get_value (GTK_ADJUSTMENT (gtk_builder_get_object (b, "adj"))) == 25);
  CHECK (G_IS_MENU_MODEL (gtk_builder_get_object (b, "menubar")));
  CHECK (g_menu_model_get_n_items (G_MENU_MODEL (gtk_builder_get_object (b, "menubar"))) == 1);
  gtk_widget_destroy (GTK_WIDGET (win));
  g_object_unref (b);
  return 0;
}

static int test_menus (void)
{
#if GTK_MAJOR_VERSION < 4
  GtkWidget *bar = gtk_menu_bar_new ();
  g_object_ref_sink (bar);
  GtkWidget *file = gtk_menu_item_new_with_mnemonic ("_File");
  GtkWidget *menu = gtk_menu_new ();
  GtkWidget *quit = gtk_menu_item_new_with_label ("Quit");
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), quit);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (file), menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (bar), file);
  counter = 0;
  g_signal_connect (quit, "activate", G_CALLBACK (count_cb), GINT_TO_POINTER (5));
  gtk_menu_item_activate (GTK_MENU_ITEM (quit));
  CHECK (counter == 5);
  CHECK (gtk_menu_item_get_submenu (GTK_MENU_ITEM (file)) == menu);
  gtk_widget_destroy (bar);
  g_object_unref (bar);
#endif
  return 0;
}

static gboolean quit_loop (gpointer data) { (void) data; counter = 99; gtk_main_quit (); return G_SOURCE_REMOVE; }

static int test_main_loop (void)
{
#if GTK_MAJOR_VERSION < 4
  counter = 0;
  g_timeout_add (20, quit_loop, NULL);
  gtk_main ();
  CHECK (counter == 99);
  CHECK (gtk_main_level () == 0);
#endif
  return 0;
}

/* GtkApplication: activate, a window, an action, quit when the window goes */
static int app_ok;
static void quit_activated (GSimpleAction *a, GVariant *p, gpointer app) { (void) a; (void) p; app_ok++; g_application_quit (G_APPLICATION (app)); }

static gboolean close_later (gpointer win)
{
  gtk_widget_activate_action (GTK_WIDGET (win), "app.quit", NULL);
  return G_SOURCE_REMOVE;
}

static void on_activate (GtkApplication *app, gpointer d)
{
  (void) d;
  GtkWidget *win = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (win), "App");
  if (gtk_window_get_application (GTK_WINDOW (win)) != app) return;
  app_ok++;
  gtk_window_present (GTK_WINDOW (win));
  g_timeout_add (30, close_later, win);
}

static int test_application (void)
{
  GtkApplication *app = gtk_application_new ("org.example.GtkMadeQtTest", G_APPLICATION_NON_UNIQUE);
  GSimpleAction *quit = g_simple_action_new ("quit", NULL);
  g_signal_connect (quit, "activate", G_CALLBACK (quit_activated), app);
  g_action_map_add_action (G_ACTION_MAP (app), G_ACTION (quit));
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  app_ok = 0;
  int status = g_application_run (G_APPLICATION (app), 0, NULL);
  CHECK (status == 0);
  CHECK (app_ok == 2);
  g_object_unref (quit);
  g_object_unref (app);
  return 0;
}

int main (int argc, char **argv)
{
  (void) argc; (void) argv;
#if GTK_MAJOR_VERSION >= 4
  gtk_init ();
#else
  gtk_init (&argc, &argv);
#endif
  int fails = 0;
#define RUN(t) do { int r = t (); printf ("%-32s %s\n", #t, r ? "FAILED" : "ok"); fails += r; } while (0)
  RUN (test_signals_and_buttons);
  RUN (test_containers_and_destroy);
  RUN (test_entry_and_adjustment);
  RUN (test_text_buffer);
  RUN (test_tree_store);
#if GTK_MAJOR_VERSION < 4
  RUN (test_dialog_run);
#endif
  RUN (test_builder);
  RUN (test_menus);
  RUN (test_main_loop);
  RUN (test_application);
  printf ("GTK %d API: %s\n", GTK_MAJOR_VERSION, fails ? "FAILURES" : "all passed");
  return fails != 0;
}
