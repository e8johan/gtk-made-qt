/*
 * What Asunder (a Glade 2 era GTK 2 program) needs: GTK calls from worker
 * threads between gdk_threads_enter/leave, GtkEntryCompletion, the file
 * chooser button, frame and notebook label widgets, ATK names, menu
 * attach widgets. Built against the GTK 2 API level, run headless.
 */
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

static GThread *gui_thread;
static GtkWidget *label;
static GtkListStore *store;
static volatile int worker_on_gui, worker_rows, worker_done;

/* a worker as GTK 2 programs write them: GTK calls under the GDK lock */
static gpointer worker (gpointer data)
{
  (void) data;
  for (int i = 0; i < 20; i++) {
    gdk_threads_enter ();
    if (g_thread_self () == gui_thread) worker_on_gui++;
    char buf[32];
    snprintf (buf, sizeof buf, "<b>row %d</b>", i);
    gtk_label_set_markup (GTK_LABEL (label), buf);
    GtkTreeIter it;
    gtk_list_store_append (store, &it);
    gtk_list_store_set (store, &it, 0, buf, -1);
    gdk_threads_enter ();   /* nested */
    worker_rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL);
    gdk_threads_leave ();
    gdk_threads_leave ();
    g_usleep (2000);
  }
  gdk_threads_enter ();
  worker_done = 1;
  gtk_main_quit ();
  gdk_threads_leave ();
  return NULL;
}

static gboolean start_worker (gpointer data)
{
  (void) data;
  g_thread_unref (g_thread_new ("worker", worker, NULL));
  return FALSE;
}

static gboolean timeout_quit (gpointer data) { (void) data; gtk_main_quit (); return FALSE; }

static int test_threads (GtkWidget *window)
{
  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  store = gtk_list_store_new (1, G_TYPE_STRING);
  GtkWidget *view = gtk_tree_view_new ();
  /* columns before the model, as Glade code does */
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, "Text", gtk_cell_renderer_text_new (), "markup", 0, NULL);
  gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (store));
  gtk_box_pack_start (GTK_BOX (box), view, TRUE, TRUE, 0);
  gtk_widget_show_all (window);

  g_idle_add (start_worker, NULL);
  guint t = g_timeout_add (10000, timeout_quit, NULL);
  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();
  g_source_remove (t);
  CHECK (worker_done);
  CHECK (worker_on_gui == 20);
  CHECK (worker_rows == 20);
  CHECK (strcmp (gtk_label_get_text (GTK_LABEL (label)), "row 19") == 0);
  gtk_widget_destroy (box);
  return 0;
}

static int selected;
static gboolean on_match (GtkEntryCompletion *c, GtkTreeModel *m, GtkTreeIter *it, gpointer data)
{
  (void) c; (void) m; (void) it; (void) data;
  selected++;
  return FALSE;   /* let the default handler fill in the entry */
}

static int test_completion (void)
{
  GtkListStore *list = gtk_list_store_new (1, G_TYPE_STRING);
  const char *names[] = { "Pink Floyd", "Pixies", "Portishead", "Queen" };
  for (int i = 0; i < 4; i++) {
    GtkTreeIter it;
    gtk_list_store_append (list, &it);
    gtk_list_store_set (list, &it, 0, names[i], -1);
  }
  GtkEntryCompletion *c = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (c, GTK_TREE_MODEL (list));
  g_object_unref (list);
  CHECK (gtk_entry_completion_get_text_column (c) == 0);   /* the first string column */
  gtk_entry_completion_set_text_column (c, 0);
  GtkWidget *entry = gtk_entry_new ();
  gtk_entry_set_completion (GTK_ENTRY (entry), c);
  g_object_unref (c);
  CHECK (gtk_entry_get_completion (GTK_ENTRY (entry)) == c);
  CHECK (gtk_entry_completion_get_entry (c) == entry);
  CHECK (gtk_entry_completion_get_model (c) == GTK_TREE_MODEL (list));
  gchar *p = gtk_entry_completion_compute_prefix (c, "pi");
  CHECK (p && strcmp (p, "Pi") == 0);
  g_free (p);
  p = gtk_entry_completion_compute_prefix (c, "por");
  CHECK (p && strcmp (p, "Portishead") == 0);
  g_free (p);
  CHECK (gtk_entry_completion_compute_prefix (c, "x") == NULL);

  /* choosing a match: match-selected, whose default handler sets the text */
  g_signal_connect (c, "match-selected", G_CALLBACK (on_match), NULL);
  GtkTreeIter it;
  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (list), &it, NULL, 2);
  gboolean handled = FALSE;
  g_signal_emit_by_name (c, "match-selected", GTK_TREE_MODEL (list), &it, &handled);
  CHECK (selected == 1 && handled);
  CHECK (strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), "Portishead") == 0);

  /* inline completion inserts the common prefix, selected */
  gtk_entry_set_text (GTK_ENTRY (entry), "Q");
  gtk_entry_completion_set_inline_completion (c, TRUE);
  gtk_entry_completion_set_popup_completion (c, FALSE);
  gtk_entry_completion_complete (c);
  CHECK (strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), "Queen") == 0);
  gint start, end;
  CHECK (gtk_editable_get_selection_bounds (GTK_EDITABLE (entry), &start, &end) && start == 1 && end == 5);

  gtk_entry_set_completion (GTK_ENTRY (entry), NULL);
  CHECK (gtk_entry_get_completion (GTK_ENTRY (entry)) == NULL);
  gtk_widget_destroy (entry);
  return 0;
}

static int test_widgets (GtkWidget *window)
{
  /* file chooser button: a folder button reports the folder it was given */
  GtkWidget *fb = gtk_file_chooser_button_new ("Destination folder", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  CHECK (GTK_IS_FILE_CHOOSER_BUTTON (fb));
  CHECK (strcmp (gtk_file_chooser_button_get_title (GTK_FILE_CHOOSER_BUTTON (fb)), "Destination folder") == 0);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fb), "/tmp");
  gchar *f = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fb));
  CHECK (f && strcmp (f, "/tmp") == 0);
  g_free (f);
  f = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (fb));
  CHECK (f && strcmp (f, "/tmp") == 0);
  g_free (f);

  /* frames: a GtkLabel is the title, other label widgets are children */
  GtkWidget *frame = gtk_frame_new (NULL);
  GtkWidget *title = gtk_label_new ("<b>Formats</b>");
  gtk_label_set_use_markup (GTK_LABEL (title), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), title);
  CHECK (gtk_frame_get_label_widget (GTK_FRAME (frame)) == title);
  CHECK (strcmp (gtk_frame_get_label (GTK_FRAME (frame)), "Formats") == 0);
  GtkWidget *check = gtk_check_button_new_with_mnemonic ("_MP3");
  gtk_frame_set_label_widget (GTK_FRAME (frame), check);
  GtkWidget *inner = gtk_label_new ("inside");
  gtk_container_add (GTK_CONTAINER (frame), inner);
  CHECK (gtk_frame_get_label_widget (GTK_FRAME (frame)) == check);
  CHECK (gtk_widget_get_parent (check) == frame);   /* Glade's lookup_widget walks parents */
  CHECK (gtk_bin_get_child (GTK_BIN (frame)) == inner);

  /* notebook tab label widgets set after the page is added */
  GtkWidget *nb = gtk_notebook_new ();
  GtkWidget *page = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (nb), page);
  GtkWidget *tab = gtk_label_new ("General");
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (nb), page, tab);
  CHECK (gtk_notebook_get_tab_label (GTK_NOTEBOOK (nb), page) == tab);
  CHECK (strcmp (gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (nb), page), "General") == 0);

  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), fb, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), nb, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show_all (window);

  /* menus know what they are attached to */
  GtkWidget *item = gtk_menu_item_new_with_label ("File");
  GtkWidget *menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  CHECK (gtk_menu_get_attach_widget (GTK_MENU (menu)) == item);
  GtkWidget *popup = gtk_menu_new ();
  gtk_menu_attach_to_widget (GTK_MENU (popup), window, NULL);
  CHECK (gtk_menu_get_attach_widget (GTK_MENU (popup)) == window);

  /* toolbar icon size, window type hint, ATK names, the GTK 2 ref functions */
  GtkWidget *tb = gtk_toolbar_new ();
  CHECK (gtk_toolbar_get_icon_size (GTK_TOOLBAR (tb)) == GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
  CHECK (gtk_window_get_type_hint (GTK_WINDOW (window)) == GDK_WINDOW_TYPE_HINT_DIALOG);
  AtkObject *a = gtk_widget_get_accessible (check);
  atk_object_set_name (a, "MP3 encoding");
  CHECK (strcmp (atk_object_get_name (a), "MP3 encoding") == 0);
  CHECK (atk_action_get_n_actions (ATK_ACTION (a)) == 0);
  GDestroyNotify unref = (GDestroyNotify) gtk_widget_unref;   /* Glade's GLADE_HOOKUP_OBJECT */
  gtk_widget_ref (tb);
  unref (tb);
  gtk_widget_destroy (tb);
  gtk_widget_destroy (item);
  gtk_widget_destroy (popup);
  gtk_widget_destroy (box);
  return 0;
}

int main (int argc, char **argv)
{
  gdk_threads_init ();
  gtk_init (&argc, &argv);
  gui_thread = g_thread_self ();
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (test_threads (window) || test_completion () || test_widgets (window)) return 1;
  gtk_widget_destroy (window);
  printf ("ok\n");
  return 0;
}
