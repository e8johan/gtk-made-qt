/*
 * GTK made Qt - GDK's X11 interface (gdk/gdkx.h).
 *
 * The Display is one fake connection, set up like the Motif on Qt project's
 * Xlib layer so that Xlib's macros (DefaultScreen, RootWindow ...) can read
 * it. GDK windows and pixmaps get X ids on request; X atoms are GDK atoms
 * (X's predefined atoms keep their numbers). The Xlib functions themselves
 * live in the separate libgtkqt-x11.a (see src/x11/xlib.c).
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <string.h>

enum { ROOT_WINDOW = 1, DEFAULT_COLORMAP = 2 };

static Display *s_display;
static Screen s_screen;
static Visual s_visual;
static Depth s_depths[2];

Display *gdk_x11_get_default_xdisplay (void)
{
  if (s_display) return s_display;
  _XPrivDisplay d = (_XPrivDisplay) g_malloc0 (sizeof (*((_XPrivDisplay) 0)));
  d->fd = -1;
  d->proto_major_version = 11;
  d->proto_minor_version = 0;
  d->vendor = (char *) "GTK made Qt";
  d->release = 1;
  d->byte_order = LSBFirst;
  d->bitmap_unit = 32;
  d->bitmap_pad = 32;
  d->bitmap_bit_order = LSBFirst;
  d->display_name = (char *) ":qt";
  d->default_screen = 0;
  d->nscreens = 1;
  d->screens = &s_screen;
  d->min_keycode = 8;
  d->max_keycode = 255;
  s_display = (Display *) d;

  s_visual.visualid = 0x21;
  s_visual.class = TrueColor;   /* "c_class" in C++ */
  s_visual.red_mask = 0xff0000;
  s_visual.green_mask = 0x00ff00;
  s_visual.blue_mask = 0x0000ff;
  s_visual.bits_per_rgb = 8;
  s_visual.map_entries = 256;
  s_depths[0].depth = 24;
  s_depths[0].nvisuals = 1;
  s_depths[0].visuals = &s_visual;
  s_depths[1].depth = 1;

  s_screen.display = s_display;
  s_screen.root = ROOT_WINDOW;
  s_screen.width = gdk_screen_width ();
  s_screen.height = gdk_screen_height ();
  s_screen.mwidth = gdk_screen_width_mm () > 0 ? gdk_screen_width_mm () : s_screen.width * 254 / 960;
  s_screen.mheight = gdk_screen_height_mm () > 0 ? gdk_screen_height_mm () : s_screen.height * 254 / 960;
  s_screen.ndepths = 2;
  s_screen.depths = s_depths;
  s_screen.root_depth = 24;
  s_screen.root_visual = &s_visual;
  s_screen.cmap = DEFAULT_COLORMAP;
  s_screen.white_pixel = 0xffffff;
  s_screen.black_pixel = 0x000000;
  s_screen.max_maps = s_screen.min_maps = 1;
  s_screen.backing_store = WhenMapped;
  s_screen.save_unders = False;
  return s_display;
}

Window gdk_x11_get_default_root_xwindow (void) { return ROOT_WINDOW; }
gint gdk_x11_get_default_screen (void) { return 0; }
Screen *gdk_x11_screen_get_xscreen (GdkScreen *screen) { (void) screen; gdk_x11_get_default_xdisplay (); return &s_screen; }
Display *gdk_x11_drawable_get_xdisplay (GdkDrawable *drawable) { (void) drawable; return gdk_x11_get_default_xdisplay (); }
Visual *gdk_x11_visual_get_xvisual (GdkVisual *visual) { (void) visual; gdk_x11_get_default_xdisplay (); return &s_visual; }
GdkVisual *gdkx_visual_get (VisualID id) { (void) id; return gdk_visual_get_system (); }
Colormap gdk_x11_colormap_get_xcolormap (GdkColormap *colormap) { (void) colormap; return DEFAULT_COLORMAP; }
guint32 gdk_x11_get_server_time (GdkWindow *window) { (void) window; return (guint32) (g_get_monotonic_time () / 1000); }
void gdk_x11_window_set_user_time (GdkWindow *window, guint32 timestamp) { (void) window; (void) timestamp; }
void gdk_x11_grab_server (void) {}
void gdk_x11_ungrab_server (void) {}

/* ---------- X ids for drawables ---------- */
static GHashTable *s_xids;   /* XID -> GdkDrawable (not referenced) */
static XID s_next_xid = 0x400000;

static void drawable_gone (gpointer xid, GObject *dead)
{
  (void) dead;
  if (s_xids) g_hash_table_remove (s_xids, xid);
}

XID gdk_x11_drawable_get_xid (GdkDrawable *drawable)
{
  if (!drawable) return None;
  if (drawable == gdk_get_default_root_window ()) return ROOT_WINDOW;
  XID xid = (XID) GPOINTER_TO_SIZE (g_object_get_data (G_OBJECT (drawable), "gtkqt-xid"));
  if (xid) return xid;
  if (!s_xids) s_xids = g_hash_table_new (g_direct_hash, g_direct_equal);
  xid = s_next_xid++;
  g_object_set_data (G_OBJECT (drawable), "gtkqt-xid", GSIZE_TO_POINTER (xid));
  g_hash_table_insert (s_xids, GSIZE_TO_POINTER (xid), drawable);
  g_object_weak_ref (G_OBJECT (drawable), drawable_gone, GSIZE_TO_POINTER (xid));
  return xid;
}

gpointer gdk_xid_table_lookup (XID xid)
{
  if (xid == ROOT_WINDOW) return gdk_get_default_root_window ();
  return s_xids ? g_hash_table_lookup (s_xids, GSIZE_TO_POINTER (xid)) : NULL;
}
GdkWindow *gdk_window_lookup (GdkNativeWindow anid) { return (GdkWindow *) gdk_xid_table_lookup (anid); }
GdkWindow *gdk_window_foreign_new (GdkNativeWindow anid)
{
  GdkWindow *w = gdk_window_lookup (anid);
  return w ? (GdkWindow *) g_object_ref (w) : NULL;
}
GdkPixmap *gdk_pixmap_lookup (GdkNativeWindow anid) { return (GdkPixmap *) gdk_xid_table_lookup (anid); }
GdkPixmap *gdk_pixmap_foreign_new (GdkNativeWindow anid) { return gdk_window_foreign_new (anid); }

/* ---------- atoms ---------- */
Atom gdk_x11_atom_to_xatom (GdkAtom atom) { return (Atom) GPOINTER_TO_SIZE (atom); }
GdkAtom gdk_x11_xatom_to_atom (Atom xatom) { return (GdkAtom) GSIZE_TO_POINTER (xatom); }
Atom gdk_x11_get_xatom_by_name (const gchar *name) { return gdk_x11_atom_to_xatom (gdk_atom_intern (name, FALSE)); }
const gchar *gdk_x11_get_xatom_name (Atom xatom)
{
  gchar *n = gdk_atom_name (gdk_x11_xatom_to_atom (xatom));
  const gchar *r = g_intern_string (n);
  g_free (n);
  return r;
}

/* GDK_IMAGE_XIMAGE: an XImage looking at the same pixels (32 bits per pixel) */
XImage *gdk_x11_image_get_ximage (GdkImage *image)
{
  static GHashTable *views;
  if (!image) return NULL;
  if (!views) views = g_hash_table_new (g_direct_hash, g_direct_equal);
  XImage *x = g_hash_table_lookup (views, image);
  if (!x) {
    x = g_new0 (XImage, 1);
    g_hash_table_insert (views, image, x);
  }
  x->width = image->width;
  x->height = image->height;
  x->format = ZPixmap;
  x->data = (char *) image->mem;
  x->byte_order = LSBFirst;
  x->bitmap_unit = 32;
  x->bitmap_pad = 32;
  x->depth = 24;
  x->bits_per_pixel = 32;
  x->bytes_per_line = image->bpl;
  x->red_mask = 0xff0000;
  x->green_mask = 0x00ff00;
  x->blue_mask = 0x0000ff;
  return x;
}
