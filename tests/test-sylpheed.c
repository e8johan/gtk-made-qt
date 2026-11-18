/*
 * What Sylpheed (GTK 2) needs: GtkTreeViewColumn's public fields and header
 * button, row presses in bin_window coordinates, select functions, row
 * references, store moves; text tags at a position, tag priority, images
 * and child anchors; key bindings; the accel map; radio groups, text combo
 * models, menu items' label child, GtkTable's size, tool buttons' inner
 * button, the status icon. Built against the GTK 2 API level, run headless.
 */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); exit (1); } } while (0)

static GtkWidget *tv, *clist;
static GtkListStore *store;
static int pressed_row = -1, header_allocs, header_clicks, rejected;

static gboolean on_press (GtkWidget *w, GdkEventButton *e, gpointer d)
{
  GtkTreePath *p = NULL;
  /* the event is in the rows' window: its coordinates find the row */
  CHECK (e->window == gtk_tree_view_get_bin_window (GTK_TREE_VIEW (w)));
  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (w), e->x, e->y, &p, NULL, NULL, NULL)) {
    pressed_row = gtk_tree_path_get_indices (p)[0];
    gtk_tree_path_free (p);
  }
  return FALSE;
}

static void on_header_alloc (GtkWidget *b, GtkAllocation *a, gpointer d) { if (a->width > 0) header_allocs++; }
static void on_header_clicked (GtkWidget *b, gpointer d) { header_clicks++; }

/* row 3 may not be selected */
static gboolean select_func (GtkTreeSelection *s, GtkTreeModel *m, GtkTreePath *p, gboolean cur, gpointer d)
{
  if (gtk_tree_path_get_indices (p)[0] == 3 && !cur) { rejected++; return FALSE; }
  return TRUE;
}

static int selected_row (void)
{
  GtkTreeIter it;
  GtkTreeModel *m;
  if (!gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (tv)), &m, &it)) return -1;
  GtkTreePath *p = gtk_tree_model_get_path (m, &it);
  int r = gtk_tree_path_get_indices (p)[0];
  gtk_tree_path_free (p);
  return r;
}

static gboolean check_clicks (gpointer d)
{
  /* the script clicked the first row, then pressed Ctrl+N in the CList */
  CHECK (pressed_row == 0);
  CHECK (selected_row () == 0);
  CHECK (header_allocs > 0);   /* the column's button followed its section */
  CHECK (GTK_CLIST (clist)->focus_row == 1);   /* the key binding moved the focus row */
  printf ("ok\n");
  exit (0);
  return FALSE;
}

static void scroll_vertical_count (GtkCList *c, GtkScrollType t, gfloat pos, gpointer d) { (*(int *) d)++; }

int main (int argc, char **argv)
{
  setenv ("GTKQT_SCRIPT", "wait 400; click treeview 1 20 3; wait 200; focus clist; key Ctrl+N; wait 300", 1);
  gtk_init (&argc, &argv);

  /* ---- radio groups: the first button of a new group is active ---- */
  GtkWidget *r1 = gtk_radio_button_new_with_label (NULL, "one");
  GtkWidget *r2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (r1), "two");
  CHECK (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (r1)));
  CHECK (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (r2)));

  /* ---- GtkTable keeps its size fields ---- */
  GtkWidget *table = gtk_table_new (1, 2, FALSE);
  CHECK (GTK_TABLE (table)->nrows == 1 && GTK_TABLE (table)->ncols == 2);
  gtk_table_attach_defaults (GTK_TABLE (table), gtk_label_new ("x"), 0, 1, 2, 3);
  CHECK (GTK_TABLE (table)->nrows == 3);

  /* ---- menu items hold a GtkLabel; the public child list survives the
   * destroy-while-walking loop programs use ---- */
  GtkWidget *menu = gtk_menu_new ();
  GtkWidget *item = gtk_menu_item_new_with_label ("Account");
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
  for (int i = 0; i < 3; i++) gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_menu_item_new_with_label ("extra"));
  CHECK (GTK_IS_LABEL (GTK_BIN (item)->child));
  CHECK (!strcmp (gtk_label_get_text (GTK_LABEL (GTK_BIN (item)->child)), "Account"));
  GList *cur;
  for (cur = GTK_MENU_SHELL (menu)->children; cur; cur = cur->next)
    if (GTK_BIN (cur->data)->child == NULL) { cur = cur->next; break; }
  while (cur) {
    GList *next = cur->next;
    gtk_widget_destroy (GTK_WIDGET (cur->data));
    cur = next;
  }
  CHECK (g_list_length (GTK_MENU_SHELL (menu)->children) == 2);

  /* ---- text combo boxes: the model is a list store programs edit ---- */
  GtkWidget *combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Signature 1");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Signature 2");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);
  GtkTreeModel *cm = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
  CHECK (cm && gtk_tree_model_iter_n_children (cm, NULL) == 2);
  GtkTreeIter ci;
  CHECK (gtk_tree_model_iter_nth_child (cm, &ci, NULL, 1));
  gtk_list_store_set (GTK_LIST_STORE (cm), &ci, 0, "Work", -1);
  gchar *active = gtk_combo_box_get_active_text (GTK_COMBO_BOX (combo));
  CHECK (active && !strcmp (active, "Work"));
  g_free (active);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Signature 3");
  CHECK (gtk_tree_model_iter_n_children (cm, NULL) == 3);

  /* ---- tool buttons have an inner button ---- */
  GtkToolItem *tb = gtk_tool_button_new (NULL, "Get");
  CHECK (GTK_IS_BUTTON (GTK_BIN (tb)->child));

  /* ---- the accel map: item factory paths, change, save, load ---- */
  GtkAccelGroup *ag = gtk_accel_group_new ();
  GtkItemFactory *f = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<Main>", ag);
  GtkItemFactoryEntry entries[] = { { "/_File", NULL, NULL, 0, "<Branch>" }, { "/_File/_Quit", "<control>Q", NULL, 0, NULL } };
  gtk_item_factory_create_items (f, 2, entries, NULL);
  GtkAccelKey key;
  CHECK (gtk_accel_map_lookup_entry ("<Main>/File/Quit", &key) && key.accel_key == GDK_q && key.accel_mods == GDK_CONTROL_MASK);
  CHECK (gtk_accel_map_change_entry ("<Main>/File/Quit", GDK_x, GDK_CONTROL_MASK, TRUE));
  gchar *rc = g_build_filename (g_get_tmp_dir (), "gtkqt-accelrc-test", NULL);
  gtk_accel_map_save (rc);
  gchar *saved = NULL;
  CHECK (g_file_get_contents (rc, &saved, NULL, NULL));
  CHECK (strstr (saved, "(gtk_accel_path \"<Main>/File/Quit\" \"<Control>x\")") != NULL);
  g_free (saved);
  g_file_set_contents (rc, "; test\n(gtk_accel_path \"<Main>/File/Quit\" \"<Alt>z\")\n", -1, NULL);
  gtk_accel_map_load (rc);
  CHECK (gtk_accel_map_lookup_entry ("<Main>/File/Quit", &key) && key.accel_key == GDK_z && key.accel_mods == GDK_MOD1_MASK);
  g_unlink (rc);
  g_free (rc);

  /* ---- text: tags at a position, priority, images, child anchors ---- */
  GtkWidget *text = gtk_text_view_new ();
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  PangoFontDescription *font = pango_font_description_from_string ("Sans 10");
  GtkTextTag *header = gtk_text_buffer_create_tag (buf, "header", "font-desc", font, NULL);
  GtkTextTag *title = gtk_text_buffer_create_tag (buf, "title", "weight", PANGO_WEIGHT_BOLD, NULL);
  GtkTextTag *link = gtk_text_buffer_create_tag (buf, "link", "foreground", "blue", NULL);
  pango_font_description_free (font);
  GtkTextIter it, s, e;
  gtk_text_buffer_get_end_iter (buf, &it);
  /* applied title first, header after: title (made later) still wins */
  gtk_text_buffer_insert_with_tags (buf, &it, "From:", -1, title, header, NULL);
  gtk_text_buffer_insert (buf, &it, " see ", -1);
  gtk_text_buffer_insert_with_tags (buf, &it, "http://x", -1, link, NULL);
  gtk_text_buffer_insert (buf, &it, " end", -1);
  gtk_text_buffer_get_iter_at_offset (buf, &s, 0);
  GSList *tags = gtk_text_iter_get_tags (&s);
  CHECK (g_slist_length (tags) == 2 && tags->data == header && tags->next->data == title);   /* by priority */
  g_slist_free (tags);
  CHECK (gtk_text_iter_begins_tag (&s, title) && gtk_text_iter_has_tag (&s, header));
  gtk_text_buffer_get_iter_at_offset (buf, &s, 7);   /* inside " see " */
  CHECK (!gtk_text_iter_has_tag (&s, link));
  CHECK (gtk_text_iter_forward_to_tag_toggle (&s, link) && gtk_text_iter_get_offset (&s) == 10);
  CHECK (gtk_text_iter_begins_tag (&s, link));
  e = s;
  CHECK (gtk_text_iter_forward_to_tag_toggle (&e, link) && gtk_text_iter_get_offset (&e) == 18 && gtk_text_iter_ends_tag (&e, link));
  gtk_text_buffer_get_iter_at_offset (buf, &s, 14);
  CHECK (gtk_text_iter_backward_to_tag_toggle (&s, link) && gtk_text_iter_get_offset (&s) == 10);
  /* an image and a widget in the text: one character each, U+FFFC in a slice */
  gtk_text_buffer_get_end_iter (buf, &it);
  GdkPixbuf *pix = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 8, 8);
  gdk_pixbuf_fill (pix, 0xff0000ff);
  gtk_text_buffer_insert_pixbuf (buf, &it, pix);
  GtkTextChildAnchor *anchor = gtk_text_buffer_create_child_anchor (buf, &it);
  GtkWidget *button = gtk_button_new_with_label ("Save");
  gtk_text_view_add_child_at_anchor (GTK_TEXT_VIEW (text), button, anchor);
  GList *ws = gtk_text_child_anchor_get_widgets (anchor);
  CHECK (ws && ws->data == button && !ws->next);
  g_list_free (ws);
  int n = gtk_text_buffer_get_char_count (buf);
  gtk_text_buffer_get_iter_at_offset (buf, &s, n - 2);
  CHECK (gtk_text_iter_get_pixbuf (&s) == pix);
  gtk_text_buffer_get_iter_at_offset (buf, &s, n - 1);
  CHECK (gtk_text_iter_get_child_anchor (&s) == anchor);
  gtk_text_buffer_get_bounds (buf, &s, &e);
  gchar *all = gtk_text_buffer_get_text (buf, &s, &e, TRUE), *slice = gtk_text_buffer_get_slice (buf, &s, &e, TRUE);
  CHECK (!strcmp (all, "From: see http://x end"));
  CHECK (g_str_has_suffix (slice, "end\xef\xbf\xbc\xef\xbf\xbc"));
  g_free (all);
  g_free (slice);

  /* ---- tree view: public column fields, header button, select function,
   * row references, moves ---- */
  store = gtk_list_store_new (1, G_TYPE_STRING);
  for (int i = 0; i < 6; i++) {
    gchar *t = g_strdup_printf ("row %d", i);
    gtk_list_store_insert_with_values (store, NULL, -1, 0, t, -1);
    g_free (t);
  }
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes ("Subject", gtk_cell_renderer_text_new (), "text", 0, NULL);
  gtk_tree_view_column_set_sort_column_id (col, 0);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);
  CHECK (GTK_IS_WIDGET (col->button) && col->sort_column_id == 0 && col->tree_view == tv);
  g_signal_connect (col->button, "size-allocate", G_CALLBACK (on_header_alloc), NULL);
  g_signal_connect (col->button, "clicked", G_CALLBACK (on_header_clicked), NULL);
  g_signal_connect (tv, "button_press_event", G_CALLBACK (on_press), NULL);
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  gtk_tree_selection_set_select_function (sel, select_func, NULL, NULL);
  GtkTreePath *p3 = gtk_tree_path_new_from_string ("3");
  gtk_tree_selection_select_path (sel, p3);
  CHECK (rejected == 1 && !gtk_tree_selection_path_is_selected (sel, p3));
  gtk_tree_path_free (p3);
  /* clicking the header sorts (descending after the first click on an
   * ascending sort), unless the program set sort_column_id to -1 */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), 0, GTK_SORT_ASCENDING);
  g_signal_emit_by_name (col->button, "clicked");
  GtkSortType order;
  gint sort_col;
  CHECK (header_clicks == 1);
  CHECK (gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (store), &sort_col, &order) && order == GTK_SORT_DESCENDING);
  CHECK (col->show_sort_indicator && col->sort_order == GTK_SORT_DESCENDING);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
  /* row references follow their row */
  GtkTreeIter first, third;
  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), &first, NULL, 0);
  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), &third, NULL, 2);
  GtkTreePath *tp = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &third);
  GtkTreeRowReference *ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), tp);
  gtk_tree_path_free (tp);
  gtk_list_store_move_after (GTK_LIST_STORE (store), &first, NULL);   /* to the front: no change */
  gtk_list_store_move_before (GTK_LIST_STORE (store), &first, NULL);  /* to the end */
  tp = gtk_tree_row_reference_get_path (ref);
  CHECK (tp && !strcmp (gtk_tree_path_to_string (tp), "1"));
  gtk_tree_path_free (tp);
  gtk_list_store_remove (GTK_LIST_STORE (store), &third);
  CHECK (!gtk_tree_row_reference_valid (ref));
  gtk_tree_row_reference_free (ref);
  GtkTreePath *a = gtk_tree_path_new_from_string ("1"), *b = gtk_tree_path_new_from_string ("1:0:2");
  CHECK (gtk_tree_path_is_ancestor (a, b) && !gtk_tree_path_is_ancestor (b, a));
  gtk_tree_path_free (a);
  gtk_tree_path_free (b);

  /* ---- a CList with a key binding ---- */
  clist = gtk_clist_new (1);
  gtk_widget_set_name (clist, "clist");
  for (int i = 0; i < 4; i++) { gchar *row[] = { "x" }; gtk_clist_append (GTK_CLIST (clist), row); }
  gtk_clist_select_row (GTK_CLIST (clist), 0, 0);
  GtkBindingSet *bs = gtk_binding_set_by_class (GTK_CLIST_GET_CLASS (clist));
  gtk_binding_entry_add_signal (bs, GDK_n, GDK_CONTROL_MASK, "scroll_vertical", 2, G_TYPE_ENUM, GTK_SCROLL_STEP_FORWARD, G_TYPE_FLOAT, 0.0);
  int scrolls = 0;
  g_signal_connect (clist, "scroll_vertical", G_CALLBACK (scroll_vertical_count), &scrolls);
  CHECK (gtk_bindings_activate (G_OBJECT (clist), GDK_n, GDK_CONTROL_MASK) && scrolls == 1);
  CHECK (GTK_CLIST (clist)->focus_row == 1);
  gtk_clist_select_row (GTK_CLIST (clist), 0, 0);

  /* ---- the status icon ---- */
  GtkStatusIcon *icon = gtk_status_icon_new_from_pixbuf (pix);
  gtk_status_icon_set_tooltip (icon, "Sylpheed");
  gtk_status_icon_set_blinking (icon, TRUE);
  CHECK (gtk_status_icon_get_blinking (icon));
  gtk_status_icon_set_blinking (icon, FALSE);
  gtk_status_icon_set_visible (icon, FALSE);
  CHECK (!gtk_status_icon_get_visible (icon));
  g_object_unref (icon);

  GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL), *box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), tv, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), clist, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), text, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (win), box);
  gtk_window_set_default_size (GTK_WINDOW (win), 300, 400);
  gtk_widget_show_all (win);
  g_timeout_add (1500, check_clicks, NULL);
  gtk_main ();
  return 1;
}
