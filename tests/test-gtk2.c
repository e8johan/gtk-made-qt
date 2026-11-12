/*
 * GTK 2 era API tests (the things Leafpad and similar programs use):
 * GtkItemFactory, GtkTextBuffer insert-text/delete-range semantics,
 * GtkEditable signals, GtkTextView border windows drawn with GdkGC,
 * GtkStyle, Pango layouts, GtkPrintOperation to a PDF file.
 * Built against the GTK 2 API level, run headless.
 */
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

static int activated;
static void menu_cb (gpointer data, guint action, GtkWidget *w) { (void) data; (void) w; activated = action; }

static GtkItemFactoryEntry entries[] = {
  { "/_File", NULL, NULL, 0, "<Branch>", NULL },
  { "/File/_Open...", "<control>O", (GtkItemFactoryCallback) menu_cb, 1, "<StockItem>", GTK_STOCK_OPEN },
  { "/File/---", NULL, NULL, 0, "<Separator>", NULL },
  { "/File/_Quit", "<control>Q", (GtkItemFactoryCallback) menu_cb, 2, NULL, NULL },
  { "/_Options", NULL, NULL, 0, "<Branch>", NULL },
  { "/Options/_Wrap", NULL, (GtkItemFactoryCallback) menu_cb, 3, "<CheckItem>", NULL },
  { "/Options/_One", NULL, NULL, 4, "<RadioItem>", NULL },
  { "/Options/_Two", NULL, NULL, 5, "/Options/One", NULL },
};

static int test_item_factory (GtkWidget *window)
{
  GtkAccelGroup *accel = gtk_accel_group_new ();
  GtkItemFactory *f = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel);
  gtk_item_factory_create_items (f, G_N_ELEMENTS (entries), entries, NULL);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel);
  GtkWidget *bar = gtk_item_factory_get_widget (f, "<main>");
  CHECK (GTK_IS_MENU_BAR (bar));
  CHECK (gtk_item_factory_from_widget (bar) == f);
  GtkWidget *open = gtk_item_factory_get_widget (f, "/File/Open...");
  CHECK (GTK_IS_MENU_ITEM (open));
  CHECK (GTK_IS_MENU (gtk_item_factory_get_widget (f, "/File")));
  CHECK (GTK_IS_CHECK_MENU_ITEM (gtk_item_factory_get_item (f, "/Options/Wrap")));
  GtkWidget *two = gtk_item_factory_get_item (f, "/Options/Two");
  CHECK (GTK_IS_RADIO_MENU_ITEM (two));
  CHECK (g_slist_length (gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (two))) == 2);
  activated = 0;
  gtk_menu_item_activate (GTK_MENU_ITEM (open));
  CHECK (activated == 1);
  CHECK (gtk_accel_groups_from_object (G_OBJECT (window))->data == accel);
  return 0;
}

static GString *log_;
static void on_insert (GtkTextBuffer *b, GtkTextIter *it, gchar *text, gint len, gpointer d)
{
  (void) b; (void) d;
  g_string_append_printf (log_, "i%d:%.*s;", gtk_text_iter_get_offset (it), len, text);
}
static void on_insert_after (GtkTextBuffer *b, GtkTextIter *it, gchar *text, gint len, gpointer d)
{
  (void) b; (void) text; (void) len; (void) d;
  g_string_append_printf (log_, "a%d;", gtk_text_iter_get_offset (it));   /* revalidated to the end of the insertion */
}
static void on_delete (GtkTextBuffer *b, GtkTextIter *s, GtkTextIter *e, gpointer d)
{
  (void) b; (void) d;
  g_string_append_printf (log_, "d%d-%d;", gtk_text_iter_get_offset (s), gtk_text_iter_get_offset (e));
}
static void veto_x (GtkTextBuffer *b, GtkTextIter *it, gchar *text, gint len, gpointer d)
{
  (void) it; (void) len; (void) d;
  if (text[0] == 'x') g_signal_stop_emission_by_name (b, "insert-text");
}
static void on_user (GtkTextBuffer *b, gpointer d) { (void) b; g_string_append (log_, (const char *) d); }

static int test_buffer_signals (void)
{
  GtkTextBuffer *b = gtk_text_buffer_new (NULL);
  GtkTextIter it, s, e;
  log_ = g_string_new ("");
  g_signal_connect (b, "insert-text", G_CALLBACK (on_insert), NULL);
  g_signal_connect_after (b, "insert-text", G_CALLBACK (on_insert_after), NULL);
  g_signal_connect (b, "delete-range", G_CALLBACK (on_delete), NULL);
  g_signal_connect (b, "begin-user-action", G_CALLBACK (on_user), (gpointer) "[");
  g_signal_connect (b, "end-user-action", G_CALLBACK (on_user), (gpointer) "]");
  gtk_text_buffer_set_text (b, "hello", -1);
  gtk_text_buffer_get_end_iter (b, &it);
  gtk_text_buffer_begin_user_action (b);
  gtk_text_buffer_begin_user_action (b);   /* nested: one signal pair */
  gtk_text_buffer_insert (b, &it, " world", -1);
  CHECK (gtk_text_iter_get_offset (&it) == 11);
  gtk_text_buffer_end_user_action (b);
  gtk_text_buffer_end_user_action (b);
  gtk_text_buffer_get_iter_at_offset (b, &s, 0);
  gtk_text_buffer_get_iter_at_offset (b, &e, 6);
  gtk_text_buffer_delete (b, &s, &e);
  CHECK (gtk_text_iter_get_offset (&s) == 0 && gtk_text_iter_get_offset (&e) == 0);
  CHECK (strcmp (log_->str, "i0:hello;a5;[i5: world;a11;]d0-6;") == 0);
  g_signal_connect (b, "insert-text", G_CALLBACK (veto_x), NULL);
  gtk_text_buffer_get_end_iter (b, &it);
  gtk_text_buffer_insert (b, &it, "x", 1);
  gtk_text_buffer_get_bounds (b, &s, &e);
  gchar *t = gtk_text_buffer_get_text (b, &s, &e, FALSE);
  CHECK (strcmp (t, "world") == 0);
  g_free (t);
  /* named marks and slices */
  gtk_text_buffer_get_iter_at_offset (b, &it, 2);
  gtk_text_buffer_create_mark (b, "m", &it, TRUE);
  CHECK (gtk_text_buffer_get_mark (b, "m") != NULL);
  gtk_text_buffer_get_iter_at_offset (b, &s, 1);
  gtk_text_buffer_move_mark_by_name (b, "m", &s);
  gtk_text_buffer_get_iter_at_mark (b, &it, gtk_text_buffer_get_mark (b, "m"));
  CHECK (gtk_text_iter_get_offset (&it) == 1);
  gtk_text_iter_set_line_offset (&it, 3);
  CHECK (gtk_text_iter_get_offset (&it) == 3);
  g_string_free (log_, TRUE);
  g_object_unref (b);
  return 0;
}

static int entry_inserts;
static void entry_insert (GtkEditable *e, gchar *text, gint len, gint *pos, gpointer d)
{
  (void) text; (void) len; (void) pos; (void) d;
  entry_inserts++;
  if (text[0] >= '0' && text[0] <= '9') g_signal_stop_emission_by_name (e, "insert-text");   /* letters only */
}

static int test_editable_signals (void)
{
  GtkWidget *e = gtk_entry_new ();
  g_object_ref_sink (e);
  g_signal_connect (e, "insert-text", G_CALLBACK (entry_insert), NULL);
  gint pos = 0;
  gtk_editable_insert_text (GTK_EDITABLE (e), "abc", 3, &pos);
  CHECK (pos == 3);
  gtk_editable_insert_text (GTK_EDITABLE (e), "42", 2, &pos);
  CHECK (strcmp (gtk_entry_get_text (GTK_ENTRY (e)), "abc") == 0);
  gtk_editable_delete_text (GTK_EDITABLE (e), 0, 1);
  CHECK (strcmp (gtk_entry_get_text (GTK_ENTRY (e)), "bc") == 0);
  CHECK (entry_inserts == 2);
  g_object_unref (e);
  return 0;
}

static int exposed;
static gboolean border_expose (GtkWidget *w, GdkEventExpose *ev, gpointer d)
{
  (void) d;
  GtkTextView *tv = GTK_TEXT_VIEW (w);
  if (ev->window != gtk_text_view_get_window (tv, GTK_TEXT_WINDOW_LEFT)) return FALSE;
  GdkGC *gc = gdk_gc_new (ev->window);
  GdkColor red = { 0, 0xffff, 0, 0 };
  gdk_gc_set_foreground (gc, &red);
  gdk_draw_rectangle (ev->window, gc, TRUE, 0, 0, 5, 5);
  g_object_unref (gc);
  PangoLayout *l = gtk_widget_create_pango_layout (w, "1");
  gtk_paint_layout (w->style, ev->window, GTK_WIDGET_STATE (w), FALSE, NULL, w, NULL, 6, 0, l);
  g_object_unref (l);
  exposed++;
  return FALSE;
}

static int test_text_view_borders (GtkWidget *window, GtkWidget *box)
{
  GtkWidget *tv = gtk_text_view_new ();
  gtk_box_pack_start (GTK_BOX (box), tv, TRUE, TRUE, 0);
  CHECK (GTK_TEXT_VIEW (tv)->buffer == gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv)));
  CHECK (GTK_TEXT_VIEW (tv)->im_context != NULL);
  gtk_text_buffer_set_text (GTK_TEXT_VIEW (tv)->buffer, "one\ntwo\nthree", -1);
  gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (tv), GTK_TEXT_WINDOW_LEFT, 30);
  CHECK (gtk_text_view_get_border_window_size (GTK_TEXT_VIEW (tv), GTK_TEXT_WINDOW_LEFT) == 30);
  GdkWindow *left = gtk_text_view_get_window (GTK_TEXT_VIEW (tv), GTK_TEXT_WINDOW_LEFT);
  CHECK (left != NULL);
  CHECK (gtk_text_view_get_window_type (GTK_TEXT_VIEW (tv), left) == GTK_TEXT_WINDOW_LEFT);
  g_signal_connect (tv, "expose-event", G_CALLBACK (border_expose), NULL);
  gtk_widget_show_all (window);
  for (int i = 0; i < 20; i++) { while (gtk_events_pending ()) gtk_main_iteration (); g_usleep (10000); }
  CHECK (exposed > 0);
  GtkTextIter it;
  gint y, h, top;
  gtk_text_buffer_get_iter_at_line (GTK_TEXT_VIEW (tv)->buffer, &it, 1);
  gtk_text_view_get_line_yrange (GTK_TEXT_VIEW (tv), &it, &y, &h);
  CHECK (h > 0 && y >= h - 1);
  gtk_text_view_get_line_at_y (GTK_TEXT_VIEW (tv), &it, y + h / 2, &top);
  CHECK (gtk_text_iter_get_line (&it) == 1 && top == y);
  gint wx, wy, bx, by;
  gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (tv), GTK_TEXT_WINDOW_LEFT, 0, y, &wx, &wy);
  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (tv), GTK_TEXT_WINDOW_LEFT, wx, wy, &bx, &by);
  CHECK (by == y);
  return 0;
}

static int test_style_and_pango (GtkWidget *w)
{
  GtkStyle *s = gtk_widget_get_style (w);
  CHECK (s && s->font_desc);
  CHECK (s->fg_gc[GTK_STATE_NORMAL] != NULL);
  PangoFontDescription *d = pango_font_description_from_string ("Monospace 14");
  gtk_widget_modify_font (w, d);
  pango_font_description_free (d);
  CHECK (pango_font_description_get_size (gtk_widget_get_style (w)->font_desc) == 14 * PANGO_SCALE);
  PangoLayout *l = gtk_widget_create_pango_layout (w, "MMMM");
  int pw, ph;
  pango_layout_get_pixel_size (l, &pw, &ph);
  CHECK (pw > 20 && ph > 10);
  g_object_unref (l);
  gchar *name = NULL;
  g_object_get (gtk_settings_get_default (), "gtk-key-theme-name", &name, NULL);
  CHECK (name && strcmp (name, "Default") == 0);
  g_free (name);
  CHECK (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD) != NULL);
  gchar *atom = gdk_atom_name (gdk_atom_intern ("text/uri-list", FALSE));
  CHECK (strcmp (atom, "text/uri-list") == 0);
  g_free (atom);
  return 0;
}

static int pages_drawn;
static void begin_print (GtkPrintOperation *op, GtkPrintContext *ctx, gpointer d)
{
  (void) ctx; (void) d;
  gtk_print_operation_set_n_pages (op, 2);
}
static void draw_page (GtkPrintOperation *op, GtkPrintContext *ctx, gint page, gpointer d)
{
  (void) op; (void) page; (void) d;
  cairo_t *cr = gtk_print_context_get_cairo_context (ctx);
  PangoLayout *l = gtk_print_context_create_pango_layout (ctx);
  pango_layout_set_text (l, "printed", -1);
  cairo_move_to (cr, 0, 0);
  pango_cairo_show_layout (cr, l);
  g_object_unref (l);
  pages_drawn++;
}

static int test_print (void)
{
  char path[] = "/tmp/gtkqt-print-XXXXXX.pdf";
  int fd = mkstemps (path, 4);
  CHECK (fd >= 0);
  close (fd);
  GtkPrintOperation *op = gtk_print_operation_new ();
  GtkPageSetup *setup = gtk_page_setup_new ();
  gtk_page_setup_set_top_margin (setup, 25.0, GTK_UNIT_MM);
  CHECK (gtk_page_setup_get_top_margin (setup, GTK_UNIT_MM) > 24.9);
  gtk_print_operation_set_default_page_setup (op, setup);
  gtk_print_operation_set_export_filename (op, path);
  g_signal_connect (op, "begin-print", G_CALLBACK (begin_print), NULL);
  g_signal_connect (op, "draw-page", G_CALLBACK (draw_page), NULL);
  CHECK (gtk_print_operation_run (op, GTK_PRINT_OPERATION_ACTION_EXPORT, NULL, NULL) == GTK_PRINT_OPERATION_RESULT_APPLY);
  CHECK (pages_drawn == 2);
  gchar *data = NULL;
  gsize len = 0;
  CHECK (g_file_get_contents (path, &data, &len, NULL));
  CHECK (len > 100 && memcmp (data, "%PDF", 4) == 0);
  g_free (data);
  unlink (path);
  g_object_unref (setup);
  g_object_unref (op);
  return 0;
}

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 300, 200);
  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  if (test_item_factory (window)) return 1;
  if (test_buffer_signals ()) return 1;
  if (test_editable_signals ()) return 1;
  if (test_text_view_borders (window, box)) return 1;
  if (test_style_and_pango (window)) return 1;
  if (test_print ()) return 1;
  printf ("gtk2 tests passed\n");
  return 0;
}
