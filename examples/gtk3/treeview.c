/* GTK 3: a GtkListStore shown in a GtkTreeView with text, toggle and
 * progress cells, sortable columns, a header bar and a status bar.
 * Build with real GTK:  gcc treeview.c $(pkg-config --cflags --libs gtk+-3.0)
 */
#include <gtk/gtk.h>

enum { COL_DONE, COL_TASK, COL_OWNER, COL_PROGRESS, N_COLS };

static GtkWidget *statusbar;

static void done_toggled (GtkCellRendererToggle *cell, gchar *path, gpointer store)
{
  GtkTreeIter it;
  gboolean done;
  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (store), &it, path);
  gtk_tree_model_get (GTK_TREE_MODEL (store), &it, COL_DONE, &done, -1);
  gtk_list_store_set (GTK_LIST_STORE (store), &it, COL_DONE, !done, COL_PROGRESS, done ? 50 : 100, -1);
  (void) cell;
}

static void selection_changed (GtkTreeSelection *sel, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  if (gtk_tree_selection_get_selected (sel, &model, &it)) {
    gchar *task, *owner;
    gtk_tree_model_get (model, &it, COL_TASK, &task, COL_OWNER, &owner, -1);
    gchar *msg = g_strdup_printf ("Selected \"%s\" (%s)", task, owner);
    gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, msg);
    g_free (msg);
    g_free (task);
    g_free (owner);
  }
  (void) data;
}

int main (int argc, char *argv[])
{
  static const struct { gboolean done; const char *task, *owner; int progress; } rows[] = {
    { TRUE, "Port headers to plain C", "Robin", 100 },
    { TRUE, "Back widgets with GObject", "Johan", 100 },
    { FALSE, "GTK 4 event controllers", "you", 60 },
    { FALSE, "GtkListBox", "someone", 0 },
    { FALSE, "Translate CSS to Qt stylesheets", "someone", 10 },
  };
  gtk_init (&argc, &argv);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 520, 300);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  GtkWidget *header = gtk_header_bar_new ();
  gtk_header_bar_set_title (GTK_HEADER_BAR (header), "Tasks");
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (header), gtk_button_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON));
  gtk_window_set_titlebar (GTK_WINDOW (window), header);

  GtkListStore *store = gtk_list_store_new (N_COLS, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
  for (guint i = 0; i < G_N_ELEMENTS (rows); i++)
    gtk_list_store_insert_with_values (store, NULL, -1, COL_DONE, rows[i].done, COL_TASK, rows[i].task,
                                       COL_OWNER, rows[i].owner, COL_PROGRESS, rows[i].progress, -1);

  GtkWidget *view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  GtkCellRenderer *toggle = gtk_cell_renderer_toggle_new ();
  g_signal_connect (toggle, "toggled", G_CALLBACK (done_toggled), store);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, "Done", toggle, "active", COL_DONE, NULL);

  GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes ("Task", gtk_cell_renderer_text_new (), "text", COL_TASK, NULL);
  gtk_tree_view_column_set_sort_column_id (col, COL_TASK);
  gtk_tree_view_column_set_expand (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

  col = gtk_tree_view_column_new_with_attributes ("Owner", gtk_cell_renderer_text_new (), "text", COL_OWNER, NULL);
  gtk_tree_view_column_set_sort_column_id (col, COL_OWNER);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, "Progress", gtk_cell_renderer_progress_new (),
                                               "value", COL_PROGRESS, NULL);
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)), "changed", G_CALLBACK (selection_changed), NULL);

  GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroll), view);

  statusbar = gtk_statusbar_new ();
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Click a row; tick the boxes; click a header to sort");

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (box), scroll, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), statusbar, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);
  gtk_main ();
  g_object_unref (store);
  return 0;
}
