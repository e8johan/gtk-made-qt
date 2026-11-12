/*
 * GTK 1.2 API tests: a custom widget class registered the GTK 1 way (its
 * type kept in a guint), GTK 1 signals, GtkCList, XPM pixmaps, GdkFont,
 * GtkArg-style setters. Built against the GTK 1.2 API level, run headless.
 */
#include <gtk/gtk.h>
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
  printf ("gtk1 tests passed\n");
  return 0;
}
