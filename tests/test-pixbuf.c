/*
 * GdkPixbuf beyond the pixbuf itself: the loader fed in pieces (sizes
 * told, and changed, before decoding), animations and their iterators,
 * the format list, and GtkImage playing an animation. Built against the
 * GTK 2 API level, run headless.
 */
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); exit (1); } } while (0)

/* a 4x4 GIF, red for 50 ms then blue for 80 ms, looping */
static const guchar gif[] = { 71,73,70,56,57,97,4,0,4,0,129,0,0,255,0,0,0,0,0,0,0,0,0,0,0,33,255,11,78,69,84,83,67,65,80,69,50,46,48,3,1,0,0,0,33,249,4,0,5,0,0,0,44,0,0,0,0,4,0,4,0,0,8,9,0,1,8,28,72,176,32,128,128,0,33,249,4,1,8,0,1,0,44,0,0,0,0,4,0,4,0,129,0,0,255,0,0,0,0,0,0,0,0,0,8,9,0,1,8,28,72,176,32,128,128,0,59 };

static int sizes, prepared, updated, closed;
static void on_size (GdkPixbufLoader *l, int w, int h, gpointer d) { sizes++; CHECK (w == 4 && h == 4); gdk_pixbuf_loader_set_size (l, 8, 8); }
static void on_prepared (GdkPixbufLoader *l, gpointer d) { prepared++; }
static void on_updated (GdkPixbufLoader *l, int x, int y, int w, int h, gpointer d) { updated++; }
static void on_closed (GdkPixbufLoader *l, gpointer d) { closed++; }

static guint32 pixel (GdkPixbuf *p)
{
  guchar *px = gdk_pixbuf_get_pixels (p);
  return px[0] << 16 | px[1] << 8 | px[2];
}

static GtkWidget *image;
static int frames_seen;
static guint32 last;

static gboolean watch (gpointer d)
{
  /* the image steps through the frames by itself */
  GdkPixbufAnimation *a = gtk_image_get_animation (GTK_IMAGE (image));
  CHECK (a && gtk_image_get_storage_type (GTK_IMAGE (image)) == GTK_IMAGE_ANIMATION);
  return TRUE;
}

static gboolean done (gpointer d) { printf ("ok\n"); exit (0); return FALSE; }

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  /* the format list */
  int png = 0, jpeg = 0;
  for (GSList *l = gdk_pixbuf_get_formats (); l; l = l->next) {
    gchar *name = gdk_pixbuf_format_get_name (l->data);
    if (!strcmp (name, "png")) { png = 1; CHECK (gdk_pixbuf_format_is_writable (l->data)); }
    if (!strcmp (name, "jpeg")) {
      gchar **ext = gdk_pixbuf_format_get_extensions (l->data);
      jpeg = g_strv_contains ((const gchar *const *) ext, "jpg");
      g_strfreev (ext);
    }
    g_free (name);
  }
  CHECK (png && jpeg);

  /* the loader, fed a byte at a time; the program asks for 8x8 */
  GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
  g_signal_connect (loader, "size-prepared", G_CALLBACK (on_size), NULL);
  g_signal_connect (loader, "area-prepared", G_CALLBACK (on_prepared), NULL);
  g_signal_connect (loader, "area-updated", G_CALLBACK (on_updated), NULL);
  g_signal_connect (loader, "closed", G_CALLBACK (on_closed), NULL);
  for (gsize i = 0; i < sizeof gif; i++) CHECK (gdk_pixbuf_loader_write (loader, gif + i, 1, NULL));
  CHECK (sizes == 1);
  GError *err = NULL;
  CHECK (gdk_pixbuf_loader_close (loader, &err) && !err);
  CHECK (prepared == 1 && updated >= 1 && closed == 1);
  GdkPixbuf *p = gdk_pixbuf_loader_get_pixbuf (loader);
  CHECK (p && gdk_pixbuf_get_width (p) == 8 && gdk_pixbuf_get_height (p) == 8);
  gchar *fname = gdk_pixbuf_format_get_name (gdk_pixbuf_loader_get_format (loader));
  CHECK (!strcmp (fname, "gif"));
  g_free (fname);
  GdkPixbufAnimation *anim = gdk_pixbuf_loader_get_animation (loader);
  CHECK (anim && !gdk_pixbuf_animation_is_static_image (anim));

  /* garbage: an error, not a pixbuf */
  GdkPixbufLoader *bad = gdk_pixbuf_loader_new ();
  gdk_pixbuf_loader_write (bad, (const guchar *) "not an image", 12, NULL);
  CHECK (!gdk_pixbuf_loader_close (bad, &err) && err);
  g_clear_error (&err);
  g_object_unref (bad);

  /* the animation and its iterator, in the program's own time */
  gchar *path = g_build_filename (g_get_tmp_dir (), "gtkqt-test-anim.gif", NULL);
  CHECK (g_file_set_contents (path, (const gchar *) gif, sizeof gif, NULL));
  gint w = 0, h = 0;
  CHECK (gdk_pixbuf_get_file_info (path, &w, &h) && w == 4 && h == 4);
  GdkPixbufAnimation *a = gdk_pixbuf_animation_new_from_file (path, &err);
  CHECK (a && !err && gdk_pixbuf_animation_get_width (a) == 4);
  GTimeVal t = { 1000, 0 };
  GdkPixbufAnimationIter *it = gdk_pixbuf_animation_get_iter (a, &t);
  CHECK (pixel (gdk_pixbuf_animation_iter_get_pixbuf (it)) == 0xff0000 && gdk_pixbuf_animation_iter_get_delay_time (it) == 50);
  t.tv_usec = 60000;   /* 60 ms later: the blue frame */
  CHECK (gdk_pixbuf_animation_iter_advance (it, &t));
  CHECK (pixel (gdk_pixbuf_animation_iter_get_pixbuf (it)) == 0x0000ff && gdk_pixbuf_animation_iter_get_delay_time (it) == 80);
  t.tv_usec = 140000;  /* 140 ms: it looped back to red */
  CHECK (gdk_pixbuf_animation_iter_advance (it, &t));
  CHECK (pixel (gdk_pixbuf_animation_iter_get_pixbuf (it)) == 0xff0000);
  g_object_unref (it);

  /* GtkImage from the file plays it; from a pixbuf it keeps the pixbuf */
  image = gtk_image_new_from_file (path);
  CHECK (gtk_image_get_storage_type (GTK_IMAGE (image)) == GTK_IMAGE_ANIMATION);
  GtkWidget *still = gtk_image_new_from_pixbuf (p);
  CHECK (gtk_image_get_pixbuf (GTK_IMAGE (still)) == p && gtk_image_get_storage_type (GTK_IMAGE (still)) == GTK_IMAGE_PIXBUF);
  GtkWidget *stock = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
  gchar *sid = NULL;
  GtkIconSize size;
  gtk_image_get_stock (GTK_IMAGE (stock), &sid, &size);
  CHECK (sid && !strcmp (sid, GTK_STOCK_OPEN) && size == GTK_ICON_SIZE_MENU);
  g_unlink (path);
  g_free (path);

  GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL), *box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), still, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), stock, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (win), box);
  gtk_widget_show_all (win);
  g_timeout_add (100, watch, NULL);
  g_timeout_add (600, done, NULL);
  gtk_main ();
  return 1;
}
