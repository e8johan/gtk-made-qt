/*
 * GTK 1.2 API tests: a custom widget class registered the GTK 1 way (its
 * type kept in a guint), GTK 1 signals, GtkCList, XPM pixmaps, GdkFont,
 * GtkArg-style setters. Built against the GTK 1.2 API level, run headless.
 */
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

/* ---- a GTK 1 custom widget ---- */
typedef struct { GtkWidget widget; gint clicks; } Blob;
typedef struct { GtkWidgetClass parent_class; void (*blobbed) (Blob *b); } BlobClass;
static guint blob_signal;
static int realized, exposed, drew_text, blobbed_default;
static guint blob_type;   /* a guint, as in GTK 1 code: the GType gets cut to 32 bits */

static void blob_realize (GtkWidget *w)
{
  GdkWindowAttr attr;
  memset (&attr, 0, sizeof attr);
  attr.window_type = GDK_WINDOW_CHILD;
  attr.width = w->allocation.width;
  attr.height = w->allocation.height;
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.event_mask = gtk_widget_get_events (w) | GDK_EXPOSURE_MASK;
  GTK_WIDGET_SET_FLAGS (w, GTK_REALIZED);
  w->window = gdk_window_new (w->parent->window, &attr, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (w->window, w);
  realized++;
}
static void blob_size_request (GtkWidget *w, GtkRequisition *r) { (void) w; r->width = 60; r->height = 40; }
static gint blob_expose (GtkWidget *w, GdkEventExpose *ev)
{
  (void) ev;
  GdkFont *font = gdk_font_load ("-*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*");
  gdk_draw_rectangle (w->window, w->style->black_gc, TRUE, 0, 0, 10, 10);
  gdk_draw_string (w->window, font, w->style->black_gc, 12, 20, "blob");
  drew_text = gdk_string_width (font, "blob") > 0 && font->ascent > 0;
  gdk_font_unref (font);
  exposed++;
  return FALSE;
}
static void blob_class_init (BlobClass *klass)
{
  GtkObjectClass *object_class = (GtkObjectClass *) klass;
  GtkWidgetClass *wc = (GtkWidgetClass *) klass;
  wc->realize = blob_realize;
  wc->size_request = blob_size_request;
  wc->expose_event = blob_expose;
  blob_signal = gtk_signal_new ("blobbed", GTK_RUN_FIRST, object_class->type, GTK_SIGNAL_OFFSET (BlobClass, blobbed),
                                gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, &blob_signal, 1);
  klass->blobbed = (void (*) (Blob *)) 0;
}
static void blob_blobbed_default (Blob *b) { b->clicks++; blobbed_default++; }
static void blob_init (Blob *b) { b->clicks = 0; }
static guint blob_get_type (void)
{
  if (!blob_type) {
    GtkTypeInfo info = { "Blob", sizeof (Blob), sizeof (BlobClass), (GtkClassInitFunc) blob_class_init,
                         (GtkObjectInitFunc) blob_init, (GtkArgSetFunc) NULL, (GtkArgGetFunc) NULL, NULL };
    blob_type = gtk_type_unique (gtk_widget_get_type (), &info);
  }
  return blob_type;
}
#define BLOB(obj) GTK_CHECK_CAST (obj, blob_get_type (), Blob)

static int handler_calls;
static void on_blobbed (GtkWidget *w, gpointer d) { (void) w; handler_calls += GPOINTER_TO_INT (d); }

static int selected_row = -1;
static void on_select (GtkWidget *w, gint row, gint col, GdkEventButton *ev, gpointer d)
{
  (void) w; (void) col; (void) ev; (void) d;
  selected_row = row;
}

/* selections: owner side and requesting side */
static int sel_cleared;
static char sel_received[64];
static void on_sel_get (GtkWidget *w, GtkSelectionData *sd, guint info, guint time_, gpointer d)
{
  (void) w; (void) info; (void) time_; (void) d;
  gtk_selection_data_set (sd, GDK_SELECTION_TYPE_STRING, 8, (const guchar *) "hello sel", 9);
}
static gint on_sel_clear (GtkWidget *w, GdkEventSelection *ev, gpointer d)
{
  (void) w; (void) d;
  if (ev->type == GDK_SELECTION_CLEAR) sel_cleared++;
  return TRUE;
}
static void on_sel_received (GtkWidget *w, GtkSelectionData *sd, guint time_, gpointer d)
{
  (void) w; (void) time_; (void) d;
  if (sd->type == GDK_SELECTION_TYPE_STRING && sd->length > 0 && sd->length < 64) memcpy (sel_received, sd->data, sd->length);
}
static int item_activated;
static void on_item_activate (GtkWidget *w, gpointer d) { (void) w; (void) d; item_activated++; }
/* GTK 1.2's menu position function has no push_in argument */
static gpointer pos_data;
static void menu_pos (GtkMenu *m, gint *x, gint *y, gpointer data) { (void) m; *x = 5; *y = 5; pos_data = data; }
static void pump (void) { for (int i = 0; i < 10; i++) { while (gtk_events_pending ()) gtk_main_iteration (); g_usleep (5000); } }

static const char *xpm[] = { "4 2 2 1", ". c #ff0000", "  c None", ".. .", " .. " };

int main (int argc, char **argv)
{
  gtk_set_locale ();
  gtk_init (&argc, &argv);
  CHECK (GTK_MAJOR_VERSION == 1 && gtk_major_version == 1);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (window, 300, 200);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  /* custom widget */
  GtkWidget *blob = GTK_WIDGET (gtk_type_new (blob_get_type ()));
  CHECK (GTK_CHECK_TYPE (blob, blob_get_type ()));
  BLOB (blob)->clicks = 0;
  ((BlobClass *) GTK_OBJECT (blob)->klass)->blobbed = blob_blobbed_default;
  gtk_signal_connect (GTK_OBJECT (blob), "blobbed", GTK_SIGNAL_FUNC (on_blobbed), GINT_TO_POINTER (10));
  gtk_signal_emit (GTK_OBJECT (blob), blob_signal);
  CHECK (handler_calls == 10 && blobbed_default == 1 && BLOB (blob)->clicks == 1);
  CHECK (strcmp (gtk_type_name (blob_get_type ()), "Blob") == 0);
  gtk_box_pack_start (GTK_BOX (vbox), blob, FALSE, FALSE, 0);

  /* clist */
  gchar *titles[] = { "Name", "Size" };
  GtkWidget *clist = gtk_clist_new_with_titles (2, titles);
  gchar *row1[] = { "beta", "2" }, *row2[] = { "alpha", "1" };
  gtk_clist_append (GTK_CLIST (clist), row1);
  gtk_clist_append (GTK_CLIST (clist), row2);
  gtk_clist_set_row_data (GTK_CLIST (clist), 1, (gpointer) "data-alpha");
  CHECK (GTK_CLIST (clist)->rows == 2);
  gtk_signal_connect (GTK_OBJECT (clist), "select_row", GTK_SIGNAL_FUNC (on_select), NULL);
  gtk_clist_sort (GTK_CLIST (clist));
  gchar *text = NULL;
  gtk_clist_get_text (GTK_CLIST (clist), 0, 0, &text);
  CHECK (text && strcmp (text, "alpha") == 0);
  CHECK (gtk_clist_find_row_from_data (GTK_CLIST (clist), (gpointer) "data-alpha") == 0);
  gtk_clist_select_row (GTK_CLIST (clist), 1, 0);
  CHECK (selected_row == 1);
  CHECK (GTK_CLIST (clist)->selection && GPOINTER_TO_INT (GTK_CLIST (clist)->selection->data) == 1);
  gtk_box_pack_start (GTK_BOX (vbox), clist, TRUE, TRUE, 0);

  /* XPM pixmap with a mask */
  gtk_widget_realize (window);
  GdkBitmap *mask = NULL;
  GdkPixmap *pm = gdk_pixmap_create_from_xpm_d (window->window, &mask, NULL, (gchar **) xpm);
  CHECK (pm && mask);
  gint w, h;
  gdk_window_get_size (pm, &w, &h);
  CHECK (w == 4 && h == 2);
  GtkWidget *pixmap = gtk_pixmap_new (pm, mask);
  gtk_box_pack_start (GTK_BOX (vbox), pixmap, FALSE, FALSE, 0);

  /* GtkArg setters */
  GtkWidget *label = gtk_widget_new (GTK_TYPE_LABEL, "GtkLabel::label", "via args", "GtkWidget::visible", TRUE, NULL);
  CHECK (strcmp (gtk_label_get_text (GTK_LABEL (label)), "via args") == 0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  gtk_widget_show_all (window);
  for (int i = 0; i < 20; i++) { while (gtk_events_pending ()) gtk_main_iteration (); g_usleep (10000); }
  CHECK (realized == 1);
  CHECK (exposed > 0 && drew_text);
  CHECK (blob->allocation.width >= 60 && blob->allocation.height >= 40);
  /* a labelled button has a GtkLabel child, as in GTK: programs relabel it */
  gchar *str = NULL;
  GtkWidget *button = gtk_button_new_with_label ("Cancel");
  CHECK (GTK_BIN (button)->child && GTK_IS_LABEL (GTK_BIN (button)->child));
  gtk_label_get (GTK_LABEL (GTK_BIN (button)->child), &str);
  CHECK (str && strcmp (str, "Cancel") == 0);
  gtk_container_remove (GTK_CONTAINER (button), GTK_BIN (button)->child);
  CHECK (GTK_BIN (button)->child == NULL);
  GtkWidget *close_label = gtk_label_new ("Close");
  gtk_container_add (GTK_CONTAINER (button), close_label);
  gtk_label_set_text (GTK_LABEL (close_label), "Close it");
  gtk_label_get (GTK_LABEL (GTK_BIN (button)->child), &str);
  CHECK (strcmp (str, "Close it") == 0);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show_all (button);

  /* the option menu shows the chosen item's label as its child */
  GtkWidget *om = gtk_option_menu_new (), *om_menu = gtk_menu_new ();
  const char *names[] = { "one", "two" };
  for (int i = 0; i < 2; i++) {
    GtkWidget *it = gtk_menu_item_new_with_label (names[i]);
    gtk_menu_append (GTK_MENU (om_menu), it);
    gtk_widget_show (it);
  }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (om), om_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (om), 1);
  CHECK (GTK_BIN (om)->child && GTK_IS_LABEL (GTK_BIN (om)->child));
  gtk_label_get (GTK_LABEL (GTK_BIN (om)->child), &str);
  CHECK (strcmp (str, "two") == 0);
  CHECK (gtk_option_menu_get_history (GTK_OPTION_MENU (om)) == 1);

  /* setting a check menu item activates it (item factory callbacks run) */
  GtkWidget *check = gtk_check_menu_item_new_with_label ("Graphical EQ");
  gtk_signal_connect (GTK_OBJECT (check), "activate", GTK_SIGNAL_FUNC (on_item_activate), NULL);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (check), TRUE);
  CHECK (item_activated == 1 && GTK_CHECK_MENU_ITEM (check)->active);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (check), TRUE);
  CHECK (item_activated == 1);
  gtk_menu_append (GTK_MENU (om_menu), check);

  /* menu position function, GTK 1 signature */
  static int marker;
  GtkWidget *popup = gtk_menu_new ();
  GtkWidget *pitem = gtk_menu_item_new_with_label ("item");
  gtk_menu_append (GTK_MENU (popup), pitem);
  gtk_widget_show (pitem);
  gtk_menu_popup (GTK_MENU (popup), NULL, NULL, menu_pos, &marker, 3, 0);
  CHECK (pos_data == &marker);
  gtk_menu_popdown (GTK_MENU (popup));

  /* selections: the owner supplies the text, a requester receives it, and
     the owner is told when another widget claims the selection */
  GtkWidget *owner = gtk_button_new_with_label ("owner"), *other = gtk_button_new_with_label ("other");
  gtk_box_pack_start (GTK_BOX (vbox), owner, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), other, FALSE, FALSE, 0);
  gtk_widget_show (owner);
  gtk_widget_show (other);
  gtk_signal_connect (GTK_OBJECT (owner), "selection_get", GTK_SIGNAL_FUNC (on_sel_get), NULL);
  gtk_signal_connect (GTK_OBJECT (owner), "selection_clear_event", GTK_SIGNAL_FUNC (on_sel_clear), NULL);
  gtk_signal_connect (GTK_OBJECT (other), "selection_received", GTK_SIGNAL_FUNC (on_sel_received), NULL);
  gtk_selection_add_target (owner, GDK_SELECTION_PRIMARY, GDK_SELECTION_TYPE_STRING, 1);
  CHECK (gtk_selection_owner_set (owner, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME));
  CHECK (gdk_selection_owner_get (GDK_SELECTION_PRIMARY) == owner->window);
  CHECK (gtk_selection_convert (other, GDK_SELECTION_PRIMARY, GDK_SELECTION_TYPE_STRING, GDK_CURRENT_TIME));
  pump ();
  CHECK (strcmp (sel_received, "hello sel") == 0);
  gtk_selection_owner_set (other, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
  CHECK (sel_cleared == 1);

  /* GtkFileSelection is made of GTK widgets programs can reach into */
  GtkWidget *fsw = gtk_file_selection_new ("Load files");
  GtkFileSelection *fs = GTK_FILE_SELECTION (fsw);
  CHECK (GTK_IS_CLIST (fs->file_list) && GTK_IS_CLIST (fs->dir_list) && GTK_IS_ENTRY (fs->selection_entry));
  CHECK (GTK_IS_BOX (fs->action_area) && GTK_IS_BUTTON (fs->ok_button) && GTK_BIN (fs->ok_button)->child);
  gtk_clist_set_selection_mode (GTK_CLIST (fs->file_list), GTK_SELECTION_EXTENDED);
  gchar *tmpdir = g_dir_make_tmp ("gtkqt-fsXXXXXX", NULL);
  CHECK (tmpdir != NULL);
  gchar *fa = g_build_filename (tmpdir, "a.wav", NULL), *fb = g_build_filename (tmpdir, "b.txt", NULL);
  gchar *sub = g_build_filename (tmpdir, "sub", NULL);
  g_file_set_contents (fa, "x", 1, NULL);
  g_file_set_contents (fb, "x", 1, NULL);
  g_mkdir (sub, 0700);
  gtk_file_selection_set_filename (fs, fa);
  CHECK (strcmp (gtk_file_selection_get_filename (fs), fa) == 0);
  CHECK (GTK_CLIST (fs->file_list)->rows == 2 && GTK_CLIST (fs->dir_list)->rows == 3);   /* ./ ../ sub/ */
  gtk_clist_get_text (GTK_CLIST (fs->dir_list), 2, 0, &str);
  CHECK (strcmp (str, "sub/") == 0);
  gtk_label_get (GTK_LABEL (GTK_BIN (fs->history_pulldown)->child), &str);
  gchar *dirslash = g_strconcat (tmpdir, "/", NULL);
  CHECK (strcmp (str, dirslash) == 0);
  gtk_file_selection_complete (fs, "*.wav");
  CHECK (GTK_CLIST (fs->file_list)->rows == 1);
  gtk_widget_destroy (fsw);
  g_remove (fa); g_remove (fb); g_rmdir (sub); g_rmdir (tmpdir);
  g_free (fa); g_free (fb); g_free (sub); g_free (dirslash); g_free (tmpdir);

  printf ("gtk1 tests passed\n");
  return 0;
}
