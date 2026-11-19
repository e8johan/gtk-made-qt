/*
 * What Xournal (on libgnomecanvas) needs, GTK 2 API level, run headless:
 * GtkLayout's bin_window (expose in layout coordinates, drawing from idle
 * handlers, scrolling by the adjustments), a scrolled window sharing the
 * layout's adjustments with its scroll bars, "scroll-child", children
 * allocated their requisition, a GtkTextView sized to its text with
 * "check-resize" on the toplevel, pixbufs on caller memory, the clipboard
 * with program-provided targets, cursors, radio tool buttons, regions and
 * gtk_object_destroy on objects that are not widgets.
 */
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); exit (1); } } while (0)

static GtkWidget *win, *sw, *layout, *text;
static int exposes, last_expose_y, check_resizes;

static gboolean on_expose (GtkWidget *w, GdkEventExpose *e, gpointer d)
{
  CHECK (e->window == GTK_LAYOUT (w)->bin_window);
  exposes++;
  last_expose_y = e->area.y;
  /* paint the exposed part, in layout coordinates: grey, a red square at 100,600 */
  GdkGC *gc = gdk_gc_new (e->window);
  GdkColor grey = { 0, 0x8000, 0x8000, 0x8000 }, red = { 0, 0xffff, 0, 0 };
  gdk_gc_set_rgb_fg_color (gc, &grey);
  gdk_draw_rectangle (e->window, gc, TRUE, e->area.x, e->area.y, e->area.width, e->area.height);
  gdk_gc_set_rgb_fg_color (gc, &red);
  gdk_draw_rectangle (e->window, gc, TRUE, 100, 600, 20, 20);
  g_object_unref (gc);
  return FALSE;
}

static void on_check_resize (GtkContainer *c, gpointer d) { check_resizes++; }

static guint32 pixel_at (GdkDrawable *d, int x, int y)
{
  GdkPixbuf *p = gdk_pixbuf_get_from_drawable (NULL, d, NULL, x, y, 0, 0, 1, 1);
  CHECK (p);
  guchar *px = gdk_pixbuf_get_pixels (p);
  guint32 v = px[0] << 16 | px[1] << 8 | px[2];
  g_object_unref (p);
  return v;
}

static int step;
static gboolean drive (gpointer d)
{
  GtkAdjustment *vadj = gtk_layout_get_vadjustment (GTK_LAYOUT (layout));
  switch (step++) {
  case 0:
    /* shown: exposed in layout coordinates, the scrolled window shares
       the layout's adjustments and its scroll bar uses them too */
    CHECK (exposes > 0);
    CHECK (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw)) == vadj);
    CHECK (vadj->upper == 2000 && vadj->page_size > 0 && vadj->page_size < 2000);
    GtkWidget *bar = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (sw));
    CHECK (GTK_IS_VSCROLLBAR (bar) && gtk_range_get_adjustment (GTK_RANGE (bar)) == vadj);
    CHECK (gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (sw)) == bar);   /* the same widget each time */
    /* the scroll-child key binding signal moves by a step */
    gboolean handled = FALSE;
    g_signal_emit_by_name (sw, "scroll-child", GTK_SCROLL_STEP_FORWARD, FALSE, &handled);
    if (!(handled && vadj->value == vadj->step_increment)) fprintf (stderr, "handled=%d value=%g step=%g\n", handled, vadj->value, vadj->step_increment);
    CHECK (handled && vadj->value == vadj->step_increment);
    gtk_adjustment_set_value (vadj, 500);
    exposes = 0;
    break;
  case 1:
    /* scrolled: exposed again, at the new place; the red square is in
       the bin_window at its layout position */
    CHECK (exposes > 0 && last_expose_y >= 500);
    CHECK (GTK_LAYOUT (layout)->bin_window != NULL);
    CHECK (pixel_at (GTK_LAYOUT (layout)->bin_window, 105, 605) == 0xff0000);
    CHECK (pixel_at (GTK_LAYOUT (layout)->bin_window, 150, 650) == 0x808080);
    /* drawing outside expose (GnomeCanvas does it from an idle) lands too */
    {
      GdkGC *gc = gdk_gc_new (GTK_LAYOUT (layout)->bin_window);
      GdkColor blue = { 0, 0, 0, 0xffff };
      gdk_gc_set_rgb_fg_color (gc, &blue);
      gdk_draw_rectangle (GTK_LAYOUT (layout)->bin_window, gc, TRUE, 200, 520, 10, 10);
      g_object_unref (gc);
      CHECK (pixel_at (GTK_LAYOUT (layout)->bin_window, 205, 525) == 0x0000ff);
    }
    /* a text view in the layout: its size is its text's, and growing text
       asks for a new size ("check-resize" on the toplevel) */
    text = gtk_text_view_new ();
    gtk_layout_put (GTK_LAYOUT (layout), text, 50, 520);
    gtk_widget_show (text);
    check_resizes = 0;
    gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (text)), "short", -1);
    break;
  case 2: {
    GtkTextView *tv = GTK_TEXT_VIEW (text);
    CHECK (tv->width > 0 && tv->height > 0);
    CHECK (text->allocation.width < 200);   /* not a default-sized text box */
    int w = tv->width;
    check_resizes = 0;
    gtk_text_buffer_set_text (gtk_text_view_get_buffer (tv), "a much longer line of text than before", -1);
    g_object_set_data (G_OBJECT (text), "w", GINT_TO_POINTER (w));
    break;
  }
  case 3: {
    GtkTextView *tv = GTK_TEXT_VIEW (text);
    CHECK (tv->width > GPOINTER_TO_INT (g_object_get_data (G_OBJECT (text), "w")));
    CHECK (check_resizes > 0);
    /* a size request is the child's allocation in a layout */
    gtk_widget_set_size_request (text, 300, 40);
    break;
  }
  case 4:
    CHECK (text->allocation.width == 300 && text->allocation.height == 40);
    gtk_main_quit ();
    return FALSE;
  }
  return TRUE;
}

/* clipboard contents produced on request */
static int clip_gets, clears;
static void clip_get (GtkClipboard *c, GtkSelectionData *sel, guint info, gpointer data)
{
  clip_gets++;
  if (info == 1) gtk_selection_data_set (sel, gdk_atom_intern ("_TEST_DATA", FALSE), 8, (const guchar *) "abc\0def", 7);
  else if (info == 2) gtk_selection_data_set_text (sel, (const gchar *) data, -1);
}
static void clip_clear (GtkClipboard *c, gpointer data) { clears++; }

typedef struct { GtkObject parent; } TestObj;
typedef struct { GtkObjectClass parent_class; } TestObjClass;
G_DEFINE_TYPE (TestObj, test_obj, GTK_TYPE_OBJECT)
static void test_obj_init (TestObj *o) {}
static void test_obj_class_init (TestObjClass *k) {}
static int destroyed;
static void on_destroy (GtkObject *o, gpointer d) { destroyed++; }

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  /* gtk_object_destroy on a GtkObject that is not a widget (canvas items) */
  GtkObject *o = g_object_new (test_obj_get_type (), NULL);
  g_object_ref_sink (o);
  g_signal_connect (o, "destroy", G_CALLBACK (on_destroy), NULL);
  gtk_object_destroy (o);
  CHECK (destroyed == 1);
  g_object_unref (o);

  /* a pixbuf on the caller's memory: compositing writes there */
  guchar buf[4 * 4 * 3];
  memset (buf, 0, sizeof buf);
  GdkPixbuf *dst = gdk_pixbuf_new_from_data (buf, GDK_COLORSPACE_RGB, FALSE, 8, 4, 4, 12, NULL, NULL);
  GdkPixbuf *src = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 2, 2);
  gdk_pixbuf_fill (src, 0x00ff00ff);
  gdk_pixbuf_composite (src, dst, 1, 1, 2, 2, 1, 1, 1, 1, GDK_INTERP_NEAREST, 255);
  CHECK (buf[(1 * 4 + 1) * 3 + 1] == 0xff && buf[0] == 0 && buf[1] == 0);
  g_object_unref (dst);
  g_object_unref (src);

  /* cursors: GTK 2's public type field, references, from a pixbuf */
  GdkCursor *cur = gdk_cursor_new (GDK_TOP_SIDE);
  CHECK (cur->type == GDK_TOP_SIDE && gdk_cursor_get_cursor_type (cur) == GDK_TOP_SIDE);
  gdk_cursor_ref (cur);
  gdk_cursor_unref (cur);
  gdk_cursor_unref (cur);
  GdkPixbuf *cp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 16, 16);
  gdk_pixbuf_fill (cp, 0xff0000ff);
  cur = gdk_cursor_new_from_pixbuf (gdk_display_get_default (), cp, 8, 8);
  CHECK (cur->type == GDK_CURSOR_IS_PIXMAP);
  GdkPixbuf *ci = gdk_cursor_get_image (cur);
  CHECK (ci && gdk_pixbuf_get_width (ci) == 16);
  g_object_unref (ci);
  gdk_cursor_unref (cur);
  g_object_unref (cp);

  /* regions */
  GdkRectangle r1 = { 0, 0, 10, 10 }, r2 = { 5, 5, 10, 10 }, box;
  GdkRegion *rg = gdk_region_rectangle (&r1);
  gdk_region_union_with_rect (rg, &r2);
  CHECK (gdk_region_point_in (rg, 12, 12) && !gdk_region_point_in (rg, 12, 2));
  gdk_region_get_clipbox (rg, &box);
  CHECK (box.x == 0 && box.y == 0 && box.width == 15 && box.height == 15);
  GdkRegion *r2g = gdk_region_rectangle (&r2);
  gdk_region_intersect (rg, r2g);
  gdk_region_get_clipbox (rg, &box);
  CHECK (box.x == 5 && box.width == 10);
  gdk_region_destroy (rg);
  gdk_region_destroy (r2g);

  /* target lists and tables */
  GtkTargetList *tl = gtk_target_list_new (NULL, 0);
  gtk_target_list_add (tl, gdk_atom_intern ("_TEST_DATA", FALSE), 0, 1);
  gtk_target_list_add_text_targets (tl, 2);
  gtk_target_list_add_image_targets (tl, 3, TRUE);
  guint info = 0;
  CHECK (gtk_target_list_find (tl, gdk_atom_intern ("UTF8_STRING", FALSE), &info) && info == 2);
  CHECK (gtk_target_list_find (tl, gdk_atom_intern ("image/png", FALSE), &info) && info == 3);
  gint n = 0;
  GtkTargetEntry *table = gtk_target_table_new_from_list (tl, &n);
  CHECK (n > 3 && !strcmp (table[0].target, "_TEST_DATA") && table[0].info == 1);

  /* the clipboard asks the program for each target when it is read */
  GtkClipboard *clip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  CHECK (gtk_clipboard_set_with_data (clip, table, n, clip_get, clip_clear, (gpointer) "pasted text"));
  gtk_target_table_free (table, n);
  gtk_target_list_unref (tl);
  CHECK (clip_gets == 0);
  GtkSelectionData *sel = gtk_clipboard_wait_for_contents (clip, gdk_atom_intern ("_TEST_DATA", FALSE));
  CHECK (sel && gtk_selection_data_get_length (sel) == 7 && !memcmp (gtk_selection_data_get_data (sel), "abc\0def", 7));
  CHECK (gtk_selection_data_get_data_type (sel) == gdk_atom_intern ("_TEST_DATA", FALSE));
  gtk_selection_data_free (sel);
  gchar *t = gtk_clipboard_wait_for_text (clip);
  CHECK (t && !strcmp (t, "pasted text") && clip_gets == 2);
  g_free (t);
  CHECK (gtk_clipboard_wait_is_target_available (clip, gdk_atom_intern ("_TEST_DATA", FALSE)));
  sel = gtk_clipboard_wait_for_contents (clip, gdk_atom_intern ("_MISSING", FALSE));
  CHECK (sel && gtk_selection_data_get_length (sel) < 0);
  gtk_selection_data_free (sel);
  gtk_clipboard_set_text (clip, "other", -1);   /* the clipboard changes hands */
  CHECK (clears == 1);
  /* an image on the clipboard */
  GdkPixbuf *ip = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 3, 2);
  gdk_pixbuf_fill (ip, 0x0000ffff);
  gtk_clipboard_set_image (clip, ip);
  GdkPixbuf *got = gtk_clipboard_wait_for_image (clip);
  CHECK (got && gdk_pixbuf_get_width (got) == 3 && gdk_pixbuf_get_pixels (got)[2] == 0xff);
  g_object_unref (got);
  g_object_unref (ip);

  /* radio tool buttons: one group, the first starts active, one at a time */
  GtkToolItem *b1 = gtk_radio_tool_button_new (NULL);
  GtkToolItem *b2 = gtk_radio_tool_button_new (gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (b1)));
  GtkToolItem *b3 = gtk_radio_tool_button_new (NULL);
  gtk_radio_tool_button_set_group (GTK_RADIO_TOOL_BUTTON (b3), gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (b2)));
  CHECK (g_slist_length (gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (b1))) == 3);
  CHECK (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (b1)));
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (b3), TRUE);
  CHECK (!gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (b1)) && gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (b3)));

  /* color button alpha */
  GtkWidget *cb = gtk_color_button_new ();
  gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (cb), TRUE);
  gtk_color_button_set_alpha (GTK_COLOR_BUTTON (cb), 0x8000);
  CHECK (gtk_color_button_get_use_alpha (GTK_COLOR_BUTTON (cb)) && abs (gtk_color_button_get_alpha (GTK_COLOR_BUTTON (cb)) - 0x8000) < 0x100);

  /* the window: a toolbar with the tool buttons, a layout in a scrolled window */
  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (win), 400, 300);
  g_signal_connect (win, "check_resize", G_CALLBACK (on_check_resize), NULL);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0), *tb = gtk_toolbar_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (tb), b1, -1);
  gtk_toolbar_insert (GTK_TOOLBAR (tb), b2, -1);
  gtk_toolbar_insert (GTK_TOOLBAR (tb), b3, -1);
  gtk_tool_item_set_visible_vertical (b2, FALSE);
  CHECK (!gtk_tool_item_get_visible_vertical (b2) && gtk_tool_item_get_visible_horizontal (b2));
  gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (sw), GTK_CORNER_TOP_RIGHT);
  CHECK (gtk_scrolled_window_get_placement (GTK_SCROLLED_WINDOW (sw)) == GTK_CORNER_TOP_RIGHT);
  layout = gtk_layout_new (NULL, NULL);
  gtk_layout_set_size (GTK_LAYOUT (layout), 1000, 2000);
  g_signal_connect (layout, "expose-event", G_CALLBACK (on_expose), NULL);
  gtk_container_add (GTK_CONTAINER (sw), layout);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (win), vbox);
  gtk_widget_show_all (win);
  CHECK (!(gdk_window_get_state (win->window) & GDK_WINDOW_STATE_WITHDRAWN));
  g_timeout_add (200, drive, NULL);
  g_timeout_add (8000, (GSourceFunc) exit, GINT_TO_POINTER (2));
  gtk_main ();
  printf ("xournal ok\n");
  return 0;
}
