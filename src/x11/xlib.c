/*
 * GTK made Qt - the Xlib calls GTK programs make next to GDK, without an
 * X server. Built as libgtkqt-x11.a, which -lX11 / -lXext resolve to
 * (linker scripts), with hidden visibility: Qt's own use of the system
 * libX11 (xcb platform) never meets these functions.
 *
 * Everything goes through the public GDK API, so windows, pixmaps,
 * properties and atoms are shared with GDK (and, in the shared build,
 * between a program and its plugins): X ids come from gdk_x11_drawable_get_xid,
 * atoms are GDK atoms, window-manager requests (_NET_WM_STATE,
 * _NET_WM_MOVERESIZE, WM_CHANGE_STATE, _MOTIF_WM_HINTS ...) become Qt window
 * operations, and drawing uses GdkGC on the window's backing store. There
 * are no X events: GDK delivers events.
 *
 * The fake Display and the genuine X11 headers come from the Motif on Qt
 * project's Xlib layer.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/shape.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define XAPI __attribute__ ((visibility ("hidden")))

/* ---------- helpers ---------- */
static GdkWindow *xwin (Drawable d) { return (GdkWindow *) gdk_xid_table_lookup (d); }
static Atom atom (const char *name) { return gdk_x11_get_xatom_by_name (name); }

static GdkColor pixel_color (unsigned long p)
{
  GdkColor c;
  c.pixel = p & 0xffffff;
  c.red = ((p >> 16) & 0xff) * 257;
  c.green = ((p >> 8) & 0xff) * 257;
  c.blue = (p & 0xff) * 257;
  return c;
}

/* ======================================================================
 * Display
 * ====================================================================== */
XAPI Display *XOpenDisplay (_Xconst char *name) { (void) name; return gdk_x11_get_default_xdisplay (); }
XAPI int XCloseDisplay (Display *d) { (void) d; return 0; }
XAPI char *XDisplayName (_Xconst char *name) { return (char *) (name ? name : ":qt"); }
XAPI int XFlush (Display *d) { (void) d; return 1; }
XAPI int XSync (Display *d, Bool discard) { (void) d; (void) discard; return 1; }
XAPI int XNoOp (Display *d) { (void) d; return 1; }
XAPI int XFree (void *data) { free (data); return 1; }
XAPI int XBell (Display *d, int percent) { (void) d; (void) percent; gdk_beep (); return 1; }
XAPI int XDefaultScreen (Display *d) { (void) d; return 0; }
XAPI Window XDefaultRootWindow (Display *d) { (void) d; return gdk_x11_get_default_root_xwindow (); }
XAPI Window XRootWindow (Display *d, int s) { (void) d; (void) s; return gdk_x11_get_default_root_xwindow (); }
XAPI int XDisplayWidth (Display *d, int s) { (void) d; (void) s; return gdk_screen_width (); }
XAPI int XDisplayHeight (Display *d, int s) { (void) d; (void) s; return gdk_screen_height (); }
XAPI int XConnectionNumber (Display *d) { (void) d; return -1; }
XAPI char *XResourceManagerString (Display *d) { (void) d; return NULL; }
XAPI char *XGetDefault (Display *d, _Xconst char *program, _Xconst char *option) { (void) d; (void) program; (void) option; return NULL; }
XAPI Bool XSupportsLocale (void) { return True; }
XAPI char *XSetLocaleModifiers (_Xconst char *m) { (void) m; return (char *) ""; }
XAPI Status XInitThreads (void) { return 1; }

static XErrorHandler s_error_handler;
static XIOErrorHandler s_io_handler;
XAPI XErrorHandler XSetErrorHandler (XErrorHandler h) { XErrorHandler old = s_error_handler; s_error_handler = h; return old; }
XAPI XIOErrorHandler XSetIOErrorHandler (XIOErrorHandler h) { XIOErrorHandler old = s_io_handler; s_io_handler = h; return old; }
XAPI int XGetErrorText (Display *d, int code, char *buf, int len) { (void) d; snprintf (buf, len, "X error %d", code); return 0; }

/* ---------- events: GDK delivers them; there is no X event queue ---------- */
XAPI int XPending (Display *d) { (void) d; return 0; }
XAPI int XEventsQueued (Display *d, int mode) { (void) d; (void) mode; return 0; }
XAPI int XNextEvent (Display *d, XEvent *ev)
{
  (void) d;
  g_main_context_iteration (NULL, TRUE);
  memset (ev, 0, sizeof *ev);
  return 0;
}
XAPI int XPeekEvent (Display *d, XEvent *ev) { (void) d; memset (ev, 0, sizeof *ev); return 0; }
XAPI int XPutBackEvent (Display *d, XEvent *ev) { (void) d; (void) ev; return 0; }
XAPI Bool XCheckTypedEvent (Display *d, int t, XEvent *ev) { (void) d; (void) t; (void) ev; return False; }
XAPI Bool XCheckTypedWindowEvent (Display *d, Window w, int t, XEvent *ev) { (void) d; (void) w; (void) t; (void) ev; return False; }
XAPI Bool XCheckMaskEvent (Display *d, long m, XEvent *ev) { (void) d; (void) m; (void) ev; return False; }
XAPI Bool XCheckWindowEvent (Display *d, Window w, long m, XEvent *ev) { (void) d; (void) w; (void) m; (void) ev; return False; }
XAPI Bool XCheckIfEvent (Display *d, XEvent *ev, Bool (*pred) (Display *, XEvent *, XPointer), XPointer arg)
{
  (void) d; (void) ev; (void) pred; (void) arg;
  return False;
}
XAPI int XSelectInput (Display *d, Window w, long mask)
{
  (void) d;
  { GdkWindow *win = xwin (w); if (win) gdk_window_set_events (win, gdk_window_get_events (win) | (gint) mask); }
  return 1;
}

/* ======================================================================
 * Atoms
 * ====================================================================== */
XAPI Atom XInternAtom (Display *d, _Xconst char *name, Bool only_if_exists) { (void) d; (void) only_if_exists; return atom (name); }
XAPI Status XInternAtoms (Display *d, char **names, int count, Bool only_if_exists, Atom *out)
{
  for (int i = 0; i < count; i++) out[i] = XInternAtom (d, names[i], only_if_exists);
  return 1;
}
XAPI char *XGetAtomName (Display *d, Atom a) { (void) d; return strdup (gdk_x11_get_xatom_name (a)); }

/* ======================================================================
 * Properties, kept on the GdkWindow; a few are window-manager requests
 * ====================================================================== */
typedef struct { Atom type; int format; unsigned long nitems; unsigned char *data; } Prop;

static size_t unit_size (int format) { return format == 32 ? sizeof (long) : format == 16 ? sizeof (short) : 1; }
static void prop_free (gpointer p) { Prop *pr = p; g_free (pr->data); g_free (pr); }

static GHashTable *props (GdkWindow *win)
{
  GHashTable *h = g_object_get_data (G_OBJECT (win), "gtkqt-xprops");
  if (!h) {
    h = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, prop_free);
    g_object_set_data_full (G_OBJECT (win), "gtkqt-xprops", h, (GDestroyNotify) g_hash_table_unref);
  }
  return h;
}

static void prop_set (GdkWindow *win, Atom name, Atom type, int format, const void *data, unsigned long n)
{
  Prop *p = g_new0 (Prop, 1);
  p->type = type;
  p->format = format;
  p->nitems = n;
  p->data = g_memdup2 (data, n * unit_size (format) + 1);
  g_hash_table_replace (props (win), GSIZE_TO_POINTER (name), p);
}

/* what this "window manager" supports, announced on the root window */
static void root_init (void)
{
  static gboolean done;
  if (done) return;
  done = TRUE;
  GdkWindow *root = gdk_get_default_root_window ();
  const char *supported[] = { "_NET_WM_STATE", "_NET_WM_STATE_ABOVE", "_NET_WM_STATE_STAYS_ON_TOP", "_NET_WM_STATE_FULLSCREEN",
                              "_NET_WM_STATE_MAXIMIZED_VERT", "_NET_WM_STATE_MAXIMIZED_HORZ", "_NET_WM_STATE_HIDDEN",
                              "_NET_WM_MOVERESIZE", "_NET_ACTIVE_WINDOW", "_NET_WM_NAME", "_NET_SUPPORTING_WM_CHECK" };
  long list[G_N_ELEMENTS (supported)];
  for (guint i = 0; i < G_N_ELEMENTS (supported); i++) list[i] = (long) atom (supported[i]);
  prop_set (root, atom ("_NET_SUPPORTED"), XA_ATOM, 32, list, G_N_ELEMENTS (supported));
  long self = (long) gdk_x11_get_default_root_xwindow ();
  prop_set (root, atom ("_NET_SUPPORTING_WM_CHECK"), XA_WINDOW, 32, &self, 1);
  long desk = 0;
  prop_set (root, atom ("_NET_CURRENT_DESKTOP"), XA_CARDINAL, 32, &desk, 1);
  prop_set (root, atom ("_NET_WM_NAME"), atom ("UTF8_STRING"), 8, "GTK made Qt", 11);
}

static void apply_net_state (GdkWindow *win, Atom state, int action)
{
  /* action: 0 remove, 1 add, 2 toggle */
  char key[64];
  snprintf (key, sizeof key, "gtkqt-netstate-%lu", (unsigned long) state);
  gboolean on = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (win), key));
  gboolean want = action == 2 ? !on : action == 1;
  g_object_set_data (G_OBJECT (win), key, GINT_TO_POINTER (want));
  if (state == atom ("_NET_WM_STATE_ABOVE") || state == atom ("_NET_WM_STATE_STAYS_ON_TOP")) gdk_window_set_keep_above (win, want);
  else if (state == atom ("_NET_WM_STATE_BELOW")) gdk_window_set_keep_below (win, want);
  else if (state == atom ("_NET_WM_STATE_FULLSCREEN")) { if (want) gdk_window_fullscreen (win); else gdk_window_unfullscreen (win); }
  else if (state == atom ("_NET_WM_STATE_MAXIMIZED_VERT") || state == atom ("_NET_WM_STATE_MAXIMIZED_HORZ")) {
    if (want) gdk_window_maximize (win); else gdk_window_unmaximize (win);
  } else if (state == atom ("_NET_WM_STATE_HIDDEN")) { if (want) gdk_window_iconify (win); }
  else if (state == atom ("_NET_WM_STATE_SKIP_TASKBAR")) gdk_window_set_skip_taskbar_hint (win, want);
  else if (state == atom ("_NET_WM_STATE_SKIP_PAGER")) gdk_window_set_skip_pager_hint (win, want);
}

static void property_side_effects (GdkWindow *win, Atom name, int format, const unsigned char *data, int n)
{
  if (name == XA_WM_NAME || name == atom ("_NET_WM_NAME")) {
    gchar *s = g_strndup ((const char *) data, n);
    gdk_window_set_title (win, s);
    g_free (s);
  } else if (name == XA_WM_ICON_NAME || name == atom ("_NET_WM_ICON_NAME")) {
    gchar *s = g_strndup ((const char *) data, n);
    gdk_window_set_icon_name (win, s);
    g_free (s);
  } else if (name == atom ("_MOTIF_WM_HINTS") && format == 32 && n >= 3) {
    const long *h = (const long *) data;
    if (h[0] & 2) gdk_window_set_decorations (win, (gint) h[2]);   /* MWM_HINTS_DECORATIONS */
  } else if (name == atom ("_NET_WM_STATE") && format == 32) {
    for (int i = 0; i < n; i++) apply_net_state (win, (Atom) ((const long *) data)[i], 1);
  } else if (name == atom ("_WIN_LAYER") && format == 32 && n >= 1) {
    gdk_window_set_keep_above (win, ((const long *) data)[0] >= 6);   /* WIN_LAYER_ONTOP */
  }
}

XAPI int XChangeProperty (Display *d, Window w, Atom name, Atom type, int format, int mode, _Xconst unsigned char *data, int n)
{
  (void) d;
  GdkWindow *win = xwin (w);
  if (!win) return BadWindow;
  if (w == gdk_x11_get_default_root_xwindow ()) root_init ();
  size_t u = unit_size (format);
  Prop *old = g_hash_table_lookup (props (win), GSIZE_TO_POINTER (name));
  if (mode != PropModeReplace && old && old->format == format) {
    size_t total = old->nitems + n;
    unsigned char *buf = g_malloc (total * u + 1);
    if (mode == PropModeAppend) { memcpy (buf, old->data, old->nitems * u); memcpy (buf + old->nitems * u, data, n * u); }
    else { memcpy (buf, data, n * u); memcpy (buf + n * u, old->data, old->nitems * u); }
    prop_set (win, name, type, format, buf, total);
    g_free (buf);
  } else {
    prop_set (win, name, type, format, data, n);
  }
  property_side_effects (win, name, format, data, n);
  return 1;
}

XAPI int XGetWindowProperty (Display *d, Window w, Atom name, long offset, long length, Bool del, Atom req_type,
                             Atom *actual_type, int *actual_format, unsigned long *nitems, unsigned long *bytes_after,
                             unsigned char **prop_return)
{
  (void) d;
  *actual_type = None;
  *actual_format = 0;
  *nitems = 0;
  *bytes_after = 0;
  *prop_return = NULL;
  if (w == gdk_x11_get_default_root_xwindow ()) root_init ();
  GdkWindow *win = xwin (w);
  if (!win) return BadWindow;
  Prop *p = g_hash_table_lookup (props (win), GSIZE_TO_POINTER (name));
  if (!p) return Success;
  *actual_type = p->type;
  *actual_format = p->format;
  if (req_type != AnyPropertyType && req_type != p->type) {
    *bytes_after = p->nitems * (p->format / 8);
    return Success;
  }
  size_t u = unit_size (p->format), bytes_per = p->format / 8;
  unsigned long start = (unsigned long) offset * 4 / bytes_per;
  if (start > p->nitems) return BadValue;
  unsigned long want = (unsigned long) length * 4 / bytes_per, avail = p->nitems - start;
  unsigned long n = want < avail ? want : avail;
  *nitems = n;
  *bytes_after = (avail - n) * bytes_per;
  unsigned char *out = malloc (n * u + 1);
  memcpy (out, p->data + start * u, n * u);
  out[n * u] = 0;
  *prop_return = out;
  if (del && *bytes_after == 0) g_hash_table_remove (props (win), GSIZE_TO_POINTER (name));
  return Success;
}

XAPI int XDeleteProperty (Display *d, Window w, Atom name)
{
  (void) d;
  { GdkWindow *win = xwin (w); if (win) g_hash_table_remove (props (win), GSIZE_TO_POINTER (name)); }
  return 1;
}

XAPI Atom *XListProperties (Display *d, Window w, int *n)
{
  (void) d;
  *n = 0;
  GdkWindow *win = xwin (w);
  if (!win) return NULL;
  GList *keys = g_hash_table_get_keys (props (win));
  Atom *out = malloc (sizeof (Atom) * (g_list_length (keys) + 1));
  for (GList *l = keys; l; l = l->next) out[(*n)++] = (Atom) GPOINTER_TO_SIZE (l->data);
  g_list_free (keys);
  return out;
}

/* client messages to the root window are window-manager requests */
XAPI Status XSendEvent (Display *d, Window w, Bool propagate, long mask, XEvent *ev)
{
  (void) d; (void) w; (void) propagate; (void) mask;
  if (!ev || ev->type != ClientMessage) return 1;
  root_init ();
  XClientMessageEvent *cm = &ev->xclient;
  GdkWindow *win = xwin (cm->window);
  if (!win) return 1;
  Atom t = cm->message_type;
  if (t == atom ("_NET_WM_STATE")) {
    apply_net_state (win, (Atom) cm->data.l[1], (int) cm->data.l[0]);
    if (cm->data.l[2]) apply_net_state (win, (Atom) cm->data.l[2], (int) cm->data.l[0]);
  } else if (t == atom ("_NET_WM_MOVERESIZE")) {
    long dir = cm->data.l[2];
    /* X directions: 0 top-left, clockwise to 7 left; 8 move */
    static const GdkWindowEdge edges[] = { GDK_WINDOW_EDGE_NORTH_WEST, GDK_WINDOW_EDGE_NORTH, GDK_WINDOW_EDGE_NORTH_EAST,
                                           GDK_WINDOW_EDGE_EAST, GDK_WINDOW_EDGE_SOUTH_EAST, GDK_WINDOW_EDGE_SOUTH,
                                           GDK_WINDOW_EDGE_SOUTH_WEST, GDK_WINDOW_EDGE_WEST };
    if (dir == 8 || dir == 10) gdk_window_begin_move_drag (win, (gint) cm->data.l[3], (gint) cm->data.l[0], (gint) cm->data.l[1], 0);
    else if (dir >= 0 && dir < 8) gdk_window_begin_resize_drag (win, edges[dir], (gint) cm->data.l[3], (gint) cm->data.l[0], (gint) cm->data.l[1], 0);
  } else if (t == atom ("WM_CHANGE_STATE")) {
    if (cm->data.l[0] == IconicState) gdk_window_iconify (win);
  } else if (t == atom ("_NET_ACTIVE_WINDOW")) {
    gdk_window_focus (win, 0);
  } else if (t == atom ("_WIN_LAYER")) {
    gdk_window_set_keep_above (win, cm->data.l[0] >= 6);
  } else if (t == atom ("_NET_CLOSE_WINDOW")) {
    gdk_window_hide (win);
  }
  return 1;
}

/* ======================================================================
 * Windows and window-manager hints
 * ====================================================================== */
XAPI Window XCreateWindow (Display *d, Window parent, int x, int y, unsigned int width, unsigned int height,
                           unsigned int border, int depth, unsigned int wclass, Visual *visual, unsigned long mask,
                           XSetWindowAttributes *attrs)
{
  (void) d; (void) parent; (void) border; (void) depth; (void) wclass; (void) visual; (void) mask; (void) attrs;
  GdkWindowAttr a;
  memset (&a, 0, sizeof a);
  a.x = x; a.y = y; a.width = width; a.height = height;
  a.window_type = GDK_WINDOW_CHILD;
  GdkWindow *win = gdk_window_new (xwin (parent), &a, 0);
  return gdk_x11_drawable_get_xid (win);
}
XAPI Window XCreateSimpleWindow (Display *d, Window parent, int x, int y, unsigned int w, unsigned int h, unsigned int bw,
                                 unsigned long border, unsigned long bg)
{
  (void) border; (void) bg;
  return XCreateWindow (d, parent, x, y, w, h, bw, 24, InputOutput, NULL, 0, NULL);
}
XAPI int XDestroyWindow (Display *d, Window w) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_destroy (win); } return 1; }
XAPI int XMapWindow (Display *d, Window w) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_show (win); } return 1; }
XAPI int XMapRaised (Display *d, Window w) { XMapWindow (d, w); { GdkWindow *win = xwin (w); if (win) gdk_window_raise (win); } return 1; }
XAPI int XUnmapWindow (Display *d, Window w) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_hide (win); } return 1; }
XAPI Status XWithdrawWindow (Display *d, Window w, int s) { (void) s; return XUnmapWindow (d, w); }
XAPI Status XIconifyWindow (Display *d, Window w, int s) { (void) d; (void) s; { GdkWindow *win = xwin (w); if (win) gdk_window_iconify (win); } return 1; }
XAPI int XRaiseWindow (Display *d, Window w) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_raise (win); } return 1; }
XAPI int XLowerWindow (Display *d, Window w) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_lower (win); } return 1; }
XAPI int XMoveWindow (Display *d, Window w, int x, int y) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_move (win, x, y); } return 1; }
XAPI int XResizeWindow (Display *d, Window w, unsigned int wd, unsigned int h) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_resize (win, wd, h); } return 1; }
XAPI int XMoveResizeWindow (Display *d, Window w, int x, int y, unsigned int wd, unsigned int h)
{
  (void) d;
  { GdkWindow *win = xwin (w); if (win) gdk_window_move_resize (win, x, y, wd, h); }
  return 1;
}
XAPI int XReparentWindow (Display *d, Window w, Window p, int x, int y) { (void) d; (void) w; (void) p; (void) x; (void) y; return 1; }
XAPI int XSetInputFocus (Display *d, Window w, int revert, Time t) { (void) d; (void) revert; { GdkWindow *win = xwin (w); if (win) gdk_window_focus (win, (guint32) t); } return 1; }
XAPI int XGetInputFocus (Display *d, Window *w, int *revert) { (void) d; *w = None; *revert = RevertToParent; return 1; }

XAPI Status XGetGeometry (Display *d, Drawable dr, Window *root, int *x, int *y, unsigned int *w, unsigned int *h,
                          unsigned int *border, unsigned int *depth)
{
  (void) d;
  GdkWindow *win = xwin (dr);
  if (!win) return 0;
  gint gx, gy, gw, gh, gd;
  gdk_window_get_geometry (win, &gx, &gy, &gw, &gh, &gd);
  if (root) *root = gdk_x11_get_default_root_xwindow ();
  if (x) *x = gx;
  if (y) *y = gy;
  if (w) *w = gw;
  if (h) *h = gh;
  if (border) *border = 0;
  if (depth) *depth = gdk_drawable_get_depth (win);
  return 1;
}

XAPI Status XGetWindowAttributes (Display *d, Window w, XWindowAttributes *a)
{
  memset (a, 0, sizeof *a);
  GdkWindow *win = xwin (w);
  if (!win) return 0;
  unsigned int ww, hh, bw, dep;
  Window root;
  XGetGeometry (d, w, &root, &a->x, &a->y, &ww, &hh, &bw, &dep);
  a->width = ww;
  a->height = hh;
  a->depth = dep;
  a->root = root;
  a->visual = gdk_x11_visual_get_xvisual (NULL);
  a->colormap = gdk_x11_colormap_get_xcolormap (NULL);
  a->map_state = gdk_window_is_visible (win) ? IsViewable : IsUnmapped;
  a->screen = gdk_x11_screen_get_xscreen (NULL);
  a->class = InputOutput;
  return 1;
}

XAPI Bool XTranslateCoordinates (Display *d, Window src, Window dst, int sx, int sy, int *dx, int *dy, Window *child)
{
  (void) d;
  gint ox = 0, oy = 0, tx = 0, ty = 0;
  if (src != gdk_x11_get_default_root_xwindow () && xwin (src)) gdk_window_get_origin (xwin (src), &ox, &oy);
  if (dst != gdk_x11_get_default_root_xwindow () && xwin (dst)) gdk_window_get_origin (xwin (dst), &tx, &ty);
  *dx = sx + ox - tx;
  *dy = sy + oy - ty;
  if (child) *child = None;
  return True;
}

XAPI Bool XQueryPointer (Display *d, Window w, Window *root, Window *child, int *rx, int *ry, int *wx, int *wy, unsigned int *mask)
{
  (void) d;
  gint x, y;
  GdkModifierType m;
  gdk_window_get_pointer (gdk_get_default_root_window (), &x, &y, &m);
  *root = gdk_x11_get_default_root_xwindow ();
  *child = None;
  *rx = x;
  *ry = y;
  gint ox = 0, oy = 0;
  if (xwin (w) && w != *root) gdk_window_get_origin (xwin (w), &ox, &oy);
  *wx = x - ox;
  *wy = y - oy;
  *mask = (unsigned int) m;
  return True;
}

XAPI Status XQueryTree (Display *d, Window w, Window *root, Window *parent, Window **children, unsigned int *n)
{
  (void) d;
  *root = gdk_x11_get_default_root_xwindow ();
  GdkWindow *win = xwin (w);
  *parent = win && w != *root ? gdk_x11_drawable_get_xid (gdk_window_get_parent (win)) : None;
  *children = NULL;
  *n = 0;
  return 1;
}

XAPI int XStoreName (Display *d, Window w, _Xconst char *name)
{
  return XChangeProperty (d, w, XA_WM_NAME, XA_STRING, 8, PropModeReplace, (const unsigned char *) name, name ? strlen (name) : 0);
}
XAPI int XSetIconName (Display *d, Window w, _Xconst char *name)
{
  return XChangeProperty (d, w, XA_WM_ICON_NAME, XA_STRING, 8, PropModeReplace, (const unsigned char *) name, name ? strlen (name) : 0);
}
XAPI Status XFetchName (Display *d, Window w, char **name)
{
  Atom t; int f; unsigned long n, after; unsigned char *data;
  XGetWindowProperty (d, w, XA_WM_NAME, 0, 1024, False, AnyPropertyType, &t, &f, &n, &after, &data);
  *name = (char *) data;
  return data != NULL;
}
XAPI void XSetWMName (Display *d, Window w, XTextProperty *tp) { if (tp) XChangeProperty (d, w, XA_WM_NAME, tp->encoding, tp->format, PropModeReplace, tp->value, tp->nitems); }
XAPI void XSetWMIconName (Display *d, Window w, XTextProperty *tp) { if (tp) XChangeProperty (d, w, XA_WM_ICON_NAME, tp->encoding, tp->format, PropModeReplace, tp->value, tp->nitems); }
XAPI Status XStringListToTextProperty (char **list, int count, XTextProperty *tp)
{
  GString *s = g_string_new (NULL);
  for (int i = 0; i < count; i++) { if (i) g_string_append_c (s, 0); g_string_append (s, list[i]); }
  tp->nitems = s->len;
  tp->value = malloc (s->len + 1);
  memcpy (tp->value, s->str, s->len + 1);
  tp->encoding = XA_STRING;
  tp->format = 8;
  g_string_free (s, TRUE);
  return 1;
}
XAPI Status XSetWMProtocols (Display *d, Window w, Atom *protocols, int count)
{
  long *l = g_new (long, count + 1);
  for (int i = 0; i < count; i++) l[i] = (long) protocols[i];
  XChangeProperty (d, w, atom ("WM_PROTOCOLS"), XA_ATOM, 32, PropModeReplace, (unsigned char *) l, count);
  g_free (l);
  return 1;
}
XAPI Status XGetWMProtocols (Display *d, Window w, Atom **protocols, int *count)
{
  Atom t; int f; unsigned long n, after; unsigned char *data;
  XGetWindowProperty (d, w, atom ("WM_PROTOCOLS"), 0, 1024, False, XA_ATOM, &t, &f, &n, &after, &data);
  *count = (int) n;
  *protocols = malloc (sizeof (Atom) * (n + 1));
  for (unsigned long i = 0; i < n; i++) (*protocols)[i] = (Atom) ((long *) data)[i];
  free (data);
  return n > 0;
}
XAPI int XSetCommand (Display *d, Window w, char **argv, int argc) { (void) d; (void) w; (void) argv; (void) argc; return 1; }
XAPI XWMHints *XAllocWMHints (void) { return calloc (1, sizeof (XWMHints)); }
XAPI XSizeHints *XAllocSizeHints (void) { return calloc (1, sizeof (XSizeHints)); }
XAPI XClassHint *XAllocClassHint (void) { return calloc (1, sizeof (XClassHint)); }
XAPI int XSetWMHints (Display *d, Window w, XWMHints *h) { (void) d; (void) w; (void) h; return 1; }
XAPI XWMHints *XGetWMHints (Display *d, Window w) { (void) d; (void) w; return XAllocWMHints (); }
XAPI int XSetClassHint (Display *d, Window w, XClassHint *h) { (void) d; (void) w; (void) h; return 1; }
XAPI int XSetTransientForHint (Display *d, Window w, Window p) { (void) d; (void) w; (void) p; return 1; }
XAPI void XSetWMNormalHints (Display *d, Window w, XSizeHints *h)
{
  (void) d;
  GdkWindow *win = xwin (w);
  if (!win || !h) return;
  gdk_window_set_hints (win, h->x, h->y, h->min_width, h->min_height, h->max_width, h->max_height,
                        ((h->flags & PMinSize) ? GDK_HINT_MIN_SIZE : 0) | ((h->flags & PMaxSize) ? GDK_HINT_MAX_SIZE : 0));
}
XAPI int XSetNormalHints (Display *d, Window w, XSizeHints *h) { XSetWMNormalHints (d, w, h); return 1; }
XAPI void XSetWMSizeHints (Display *d, Window w, XSizeHints *h, Atom p) { (void) p; XSetWMNormalHints (d, w, h); }

/* grabs: Qt grabs the pointer for a press already */
XAPI int XGrabPointer (Display *d, Window w, Bool oe, unsigned int m, int pm, int km, Window c, Cursor cu, Time t)
{
  (void) d; (void) w; (void) oe; (void) m; (void) pm; (void) km; (void) c; (void) cu; (void) t;
  return GrabSuccess;
}
XAPI int XUngrabPointer (Display *d, Time t) { (void) d; (void) t; return 1; }
XAPI int XGrabKeyboard (Display *d, Window w, Bool oe, int pm, int km, Time t) { (void) d; (void) w; (void) oe; (void) pm; (void) km; (void) t; return GrabSuccess; }
XAPI int XUngrabKeyboard (Display *d, Time t) { (void) d; (void) t; return 1; }
XAPI int XGrabKey (Display *d, int k, unsigned int m, Window w, Bool oe, int pm, int km) { (void) d; (void) k; (void) m; (void) w; (void) oe; (void) pm; (void) km; return 1; }
XAPI int XUngrabKey (Display *d, int k, unsigned int m, Window w) { (void) d; (void) k; (void) m; (void) w; return 1; }
XAPI int XGrabServer (Display *d) { (void) d; return 1; }
XAPI int XUngrabServer (Display *d) { (void) d; return 1; }
XAPI int XAllowEvents (Display *d, int m, Time t) { (void) d; (void) m; (void) t; return 1; }
XAPI int XWarpPointer (Display *d, Window s, Window dst, int sx, int sy, unsigned int sw, unsigned int sh, int dx, int dy)
{
  (void) d; (void) s; (void) dst; (void) sx; (void) sy; (void) sw; (void) sh; (void) dx; (void) dy;
  return 1;
}
XAPI Window XGetSelectionOwner (Display *d, Atom s) { (void) d; (void) s; return None; }
XAPI int XSetSelectionOwner (Display *d, Atom s, Window w, Time t) { (void) d; (void) s; (void) w; (void) t; return 1; }
XAPI int XConvertSelection (Display *d, Atom s, Atom t, Atom p, Window w, Time tm) { (void) d; (void) s; (void) t; (void) p; (void) w; (void) tm; return 1; }

/* ---------- cursors ---------- */
XAPI Cursor XCreateFontCursor (Display *d, unsigned int shape)
{
  (void) d;
  return (Cursor) GPOINTER_TO_SIZE (gdk_cursor_new ((GdkCursorType) shape));
}
XAPI int XDefineCursor (Display *d, Window w, Cursor c) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_set_cursor (win, (GdkCursor *) GSIZE_TO_POINTER (c)); } return 1; }
XAPI int XUndefineCursor (Display *d, Window w) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_set_cursor (win, NULL); } return 1; }
XAPI int XFreeCursor (Display *d, Cursor c) { (void) d; if (c) gdk_cursor_destroy ((GdkCursor *) GSIZE_TO_POINTER (c)); return 1; }

/* ======================================================================
 * Keyboard: GDK key values are X keysyms; keycodes are taken as keysyms
 * ====================================================================== */
XAPI KeyCode XKeysymToKeycode (Display *d, KeySym k) { (void) d; return (KeyCode) (k < 256 ? k : 0); }
XAPI KeySym XKeycodeToKeysym (Display *d, KeyCode k, int index) { (void) d; (void) index; return k; }
XAPI KeySym XStringToKeysym (_Xconst char *s) { guint k = gdk_keyval_from_name (s); return k ? k : NoSymbol; }
XAPI char *XKeysymToString (KeySym k) { return (char *) gdk_keyval_name ((guint) k); }
XAPI int XLookupString (XKeyEvent *ev, char *buf, int len, KeySym *keysym, XComposeStatus *st)
{
  (void) st;
  KeySym k = ev ? ev->keycode : NoSymbol;
  if (keysym) *keysym = k;
  guint32 uc = gdk_keyval_to_unicode ((guint) k);
  if (!uc || len <= 0) return 0;
  gchar u[8];
  int n = g_unichar_to_utf8 (uc, u);
  if (n > len) n = len;
  memcpy (buf, u, n);
  return n;
}

/* ======================================================================
 * Colours (TrueColor: the pixel is the RGB value)
 * ====================================================================== */
XAPI Colormap XCreateColormap (Display *d, Window w, Visual *v, int alloc) { (void) d; (void) w; (void) v; (void) alloc; return gdk_x11_colormap_get_xcolormap (NULL); }
XAPI int XFreeColormap (Display *d, Colormap c) { (void) d; (void) c; return 1; }
XAPI int XInstallColormap (Display *d, Colormap c) { (void) d; (void) c; return 1; }
XAPI Status XAllocColor (Display *d, Colormap c, XColor *col)
{
  (void) d; (void) c;
  col->pixel = ((unsigned long) (col->red >> 8) << 16) | ((col->green >> 8) << 8) | (col->blue >> 8);
  return 1;
}
XAPI int XQueryColor (Display *d, Colormap c, XColor *col)
{
  (void) d; (void) c;
  GdkColor g = pixel_color (col->pixel);
  col->red = g.red; col->green = g.green; col->blue = g.blue;
  col->flags = DoRed | DoGreen | DoBlue;
  return 1;
}
XAPI int XQueryColors (Display *d, Colormap c, XColor *cols, int n) { for (int i = 0; i < n; i++) XQueryColor (d, c, &cols[i]); return 1; }
XAPI Status XParseColor (Display *d, Colormap c, _Xconst char *spec, XColor *col)
{
  (void) d; (void) c;
  GdkColor g;
  if (!gdk_color_parse (spec, &g)) return 0;
  col->red = g.red; col->green = g.green; col->blue = g.blue;
  col->flags = DoRed | DoGreen | DoBlue;
  return 1;
}
XAPI Status XLookupColor (Display *d, Colormap c, _Xconst char *spec, XColor *exact, XColor *screen)
{
  if (!XParseColor (d, c, spec, exact)) return 0;
  *screen = *exact;
  XAllocColor (d, c, screen);
  return 1;
}
XAPI Status XAllocNamedColor (Display *d, Colormap c, _Xconst char *spec, XColor *screen, XColor *exact)
{
  return XLookupColor (d, c, spec, exact, screen);
}
XAPI int XFreeColors (Display *d, Colormap c, unsigned long *p, int n, unsigned long planes) { (void) d; (void) c; (void) p; (void) n; (void) planes; return 1; }

/* ======================================================================
 * Geometry strings
 * ====================================================================== */
static int read_int (const char **s, int *v)
{
  const char *p = *s;
  if (!isdigit ((unsigned char) *p)) return 0;
  *v = 0;
  while (isdigit ((unsigned char) *p)) *v = *v * 10 + (*p++ - '0');
  *s = p;
  return 1;
}

XAPI int XParseGeometry (_Xconst char *str, int *x, int *y, unsigned int *width, unsigned int *height)
{
  int mask = 0, v;
  const char *s = str;
  if (!s || !*s) return 0;
  if (*s == '=') s++;
  if (*s != '+' && *s != '-' && *s != 'x') {
    if (read_int (&s, &v)) { *width = v; mask |= WidthValue; }
  }
  if (*s == 'x' || *s == 'X') {
    s++;
    if (read_int (&s, &v)) { *height = v; mask |= HeightValue; }
  }
  for (int axis = 0; axis < 2 && (*s == '+' || *s == '-'); axis++) {
    int neg = *s++ == '-';
    if (!read_int (&s, &v)) return 0;
    if (axis == 0) { *x = neg ? -v : v; mask |= XValue | (neg ? XNegative : 0); }
    else { *y = neg ? -v : v; mask |= YValue | (neg ? YNegative : 0); }
  }
  return *s ? 0 : mask;
}

/* ======================================================================
 * Pixmaps, GCs and drawing on GDK drawables
 * ====================================================================== */
struct _XGC { GdkGC *gdk; XGCValues v; GdkFont *font; };

XAPI Pixmap XCreatePixmap (Display *d, Drawable dr, unsigned int w, unsigned int h, unsigned int depth)
{
  (void) d;
  return gdk_x11_drawable_get_xid (gdk_pixmap_new (xwin (dr), w, h, depth));
}
XAPI int XFreePixmap (Display *d, Pixmap p) { (void) d; { GdkWindow *pm = xwin (p); if (pm) g_object_unref (pm); } return 1; }
XAPI Pixmap XCreateBitmapFromData (Display *d, Drawable dr, _Xconst char *data, unsigned int w, unsigned int h)
{
  (void) d;
  return gdk_x11_drawable_get_xid (gdk_bitmap_create_from_data (xwin (dr), data, w, h));
}
XAPI Pixmap XCreatePixmapFromBitmapData (Display *d, Drawable dr, char *data, unsigned int w, unsigned int h,
                                         unsigned long fg, unsigned long bg, unsigned int depth)
{
  (void) d;
  GdkColor f = pixel_color (fg), b = pixel_color (bg);
  return gdk_x11_drawable_get_xid (gdk_pixmap_create_from_data (xwin (dr), data, w, h, depth, &f, &b));
}

static void gc_apply (GC gc, unsigned long mask)
{
  if (mask & GCForeground) { GdkColor c = pixel_color (gc->v.foreground); gdk_gc_set_foreground (gc->gdk, &c); }
  if (mask & GCBackground) { GdkColor c = pixel_color (gc->v.background); gdk_gc_set_background (gc->gdk, &c); }
  if (mask & GCFunction) gdk_gc_set_function (gc->gdk, gc->v.function == GXxor ? GDK_XOR : gc->v.function == GXinvert ? GDK_INVERT : GDK_COPY);
  if (mask & (GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle))
    gdk_gc_set_line_attributes (gc->gdk, gc->v.line_width, (GdkLineStyle) gc->v.line_style, (GdkCapStyle) gc->v.cap_style,
                                (GdkJoinStyle) gc->v.join_style);
  if (mask & GCClipMask) gdk_gc_set_clip_mask (gc->gdk, gc->v.clip_mask ? xwin (gc->v.clip_mask) : NULL);
  if (mask & (GCClipXOrigin | GCClipYOrigin)) gdk_gc_set_clip_origin (gc->gdk, gc->v.clip_x_origin, gc->v.clip_y_origin);
}

XAPI GC XCreateGC (Display *d, Drawable dr, unsigned long mask, XGCValues *values)
{
  (void) d;
  GC gc = calloc (1, sizeof *gc);
  gc->gdk = gdk_gc_new (xwin (dr));
  gc->v.function = GXcopy;
  gc->v.background = 0xffffff;
  if (values) {
    unsigned long keep = gc->v.background;
    gc->v = *values;
    if (!(mask & GCBackground)) gc->v.background = keep;
    if (!(mask & GCFunction)) gc->v.function = GXcopy;
  }
  gc_apply (gc, mask | GCBackground | GCFunction);
  return gc;
}
XAPI int XFreeGC (Display *d, GC gc) { (void) d; if (gc) { g_object_unref (gc->gdk); free (gc); } return 1; }
XAPI int XChangeGC (Display *d, GC gc, unsigned long mask, XGCValues *v)
{
  (void) d;
  if (mask & GCForeground) gc->v.foreground = v->foreground;
  if (mask & GCBackground) gc->v.background = v->background;
  if (mask & GCFunction) gc->v.function = v->function;
  if (mask & GCLineWidth) gc->v.line_width = v->line_width;
  if (mask & GCLineStyle) gc->v.line_style = v->line_style;
  if (mask & GCCapStyle) gc->v.cap_style = v->cap_style;
  if (mask & GCJoinStyle) gc->v.join_style = v->join_style;
  if (mask & GCClipMask) gc->v.clip_mask = v->clip_mask;
  if (mask & GCClipXOrigin) gc->v.clip_x_origin = v->clip_x_origin;
  if (mask & GCClipYOrigin) gc->v.clip_y_origin = v->clip_y_origin;
  gc_apply (gc, mask);
  return 1;
}
XAPI int XSetForeground (Display *d, GC gc, unsigned long p) { (void) d; gc->v.foreground = p; gc_apply (gc, GCForeground); return 1; }
XAPI int XSetBackground (Display *d, GC gc, unsigned long p) { (void) d; gc->v.background = p; gc_apply (gc, GCBackground); return 1; }
XAPI int XSetFunction (Display *d, GC gc, int f) { (void) d; gc->v.function = f; gc_apply (gc, GCFunction); return 1; }
XAPI int XSetLineAttributes (Display *d, GC gc, unsigned int w, int ls, int cs, int js)
{
  (void) d;
  gc->v.line_width = w; gc->v.line_style = ls; gc->v.cap_style = cs; gc->v.join_style = js;
  gc_apply (gc, GCLineWidth);
  return 1;
}
XAPI int XSetClipMask (Display *d, GC gc, Pixmap p) { (void) d; gc->v.clip_mask = p; gc_apply (gc, GCClipMask); return 1; }
XAPI int XSetClipOrigin (Display *d, GC gc, int x, int y) { (void) d; gc->v.clip_x_origin = x; gc->v.clip_y_origin = y; gc_apply (gc, GCClipXOrigin); return 1; }
XAPI int XSetClipRectangles (Display *d, GC gc, int x, int y, XRectangle *r, int n, int order)
{
  (void) d; (void) order;
  if (n > 0) {
    GdkRectangle g = { x + r[0].x, y + r[0].y, r[0].width, r[0].height };
    gdk_gc_set_clip_rectangle (gc->gdk, &g);
  } else {
    gdk_gc_set_clip_rectangle (gc->gdk, NULL);
  }
  return 1;
}
XAPI int XSetGraphicsExposures (Display *d, GC gc, Bool on) { (void) d; gc->v.graphics_exposures = on; return 1; }
XAPI int XSetFillStyle (Display *d, GC gc, int s) { (void) d; gc->v.fill_style = s; return 1; }
XAPI int XSetDashes (Display *d, GC gc, int off, _Xconst char *list, int n) { (void) d; gdk_gc_set_dashes (gc->gdk, off, (gint8 *) list, n); return 1; }

XAPI int XDrawPoint (Display *d, Drawable dr, GC gc, int x, int y) { (void) d; gdk_draw_point (xwin (dr), gc->gdk, x, y); return 1; }
XAPI int XDrawPoints (Display *d, Drawable dr, GC gc, XPoint *p, int n, int mode)
{
  (void) d; (void) mode;
  for (int i = 0; i < n; i++) gdk_draw_point (xwin (dr), gc->gdk, p[i].x, p[i].y);
  return 1;
}
XAPI int XDrawLine (Display *d, Drawable dr, GC gc, int x1, int y1, int x2, int y2) { (void) d; gdk_draw_line (xwin (dr), gc->gdk, x1, y1, x2, y2); return 1; }
XAPI int XDrawLines (Display *d, Drawable dr, GC gc, XPoint *p, int n, int mode)
{
  (void) d;
  GdkPoint *g = g_new (GdkPoint, n);
  int cx = 0, cy = 0;
  for (int i = 0; i < n; i++) {
    cx = mode == CoordModePrevious && i ? cx + p[i].x : p[i].x;
    cy = mode == CoordModePrevious && i ? cy + p[i].y : p[i].y;
    g[i].x = cx; g[i].y = cy;
  }
  gdk_draw_lines (xwin (dr), gc->gdk, g, n);
  g_free (g);
  return 1;
}
XAPI int XDrawSegments (Display *d, Drawable dr, GC gc, XSegment *s, int n)
{
  (void) d;
  for (int i = 0; i < n; i++) gdk_draw_line (xwin (dr), gc->gdk, s[i].x1, s[i].y1, s[i].x2, s[i].y2);
  return 1;
}
XAPI int XDrawRectangle (Display *d, Drawable dr, GC gc, int x, int y, unsigned int w, unsigned int h) { (void) d; gdk_draw_rectangle (xwin (dr), gc->gdk, FALSE, x, y, w, h); return 1; }
XAPI int XFillRectangle (Display *d, Drawable dr, GC gc, int x, int y, unsigned int w, unsigned int h) { (void) d; gdk_draw_rectangle (xwin (dr), gc->gdk, TRUE, x, y, w, h); return 1; }
XAPI int XFillRectangles (Display *d, Drawable dr, GC gc, XRectangle *r, int n)
{
  for (int i = 0; i < n; i++) XFillRectangle (d, dr, gc, r[i].x, r[i].y, r[i].width, r[i].height);
  return 1;
}
XAPI int XDrawRectangles (Display *d, Drawable dr, GC gc, XRectangle *r, int n)
{
  for (int i = 0; i < n; i++) XDrawRectangle (d, dr, gc, r[i].x, r[i].y, r[i].width, r[i].height);
  return 1;
}
XAPI int XDrawArc (Display *d, Drawable dr, GC gc, int x, int y, unsigned int w, unsigned int h, int a1, int a2) { (void) d; gdk_draw_arc (xwin (dr), gc->gdk, FALSE, x, y, w, h, a1, a2); return 1; }
XAPI int XFillArc (Display *d, Drawable dr, GC gc, int x, int y, unsigned int w, unsigned int h, int a1, int a2) { (void) d; gdk_draw_arc (xwin (dr), gc->gdk, TRUE, x, y, w, h, a1, a2); return 1; }
XAPI int XFillPolygon (Display *d, Drawable dr, GC gc, XPoint *p, int n, int shape, int mode)
{
  (void) d; (void) shape; (void) mode;
  GdkPoint *g = g_new (GdkPoint, n);
  for (int i = 0; i < n; i++) { g[i].x = p[i].x; g[i].y = p[i].y; }
  gdk_draw_polygon (xwin (dr), gc->gdk, TRUE, g, n);
  g_free (g);
  return 1;
}
XAPI int XCopyArea (Display *d, Drawable src, Drawable dst, GC gc, int sx, int sy, unsigned int w, unsigned int h, int dx, int dy)
{
  (void) d;
  gdk_draw_drawable (xwin (dst), gc->gdk, xwin (src), sx, sy, dx, dy, w, h);
  return 1;
}
XAPI int XCopyPlane (Display *d, Drawable src, Drawable dst, GC gc, int sx, int sy, unsigned int w, unsigned int h, int dx, int dy, unsigned long plane)
{
  (void) plane;
  return XCopyArea (d, src, dst, gc, sx, sy, w, h, dx, dy);
}
XAPI int XClearWindow (Display *d, Window w) { (void) d; { GdkWindow *win = xwin (w); if (win) gdk_window_clear (win); } return 1; }
XAPI int XClearArea (Display *d, Window w, int x, int y, unsigned int wd, unsigned int h, Bool exposures)
{
  (void) d;
  GdkWindow *win = xwin (w);
  if (!win) return 1;
  if (exposures) gdk_window_clear_area_e (win, x, y, wd, h);
  else gdk_window_clear_area (win, x, y, wd, h);
  return 1;
}

/* ---------- fonts ---------- */
static GPtrArray *s_fonts;   /* Font id - 1 -> GdkFont */
static GdkFont *font_of (Font fid) { return s_fonts && fid > 0 && fid <= s_fonts->len ? g_ptr_array_index (s_fonts, fid - 1) : NULL; }

XAPI Font XLoadFont (Display *d, _Xconst char *name)
{
  (void) d;
  if (!s_fonts) s_fonts = g_ptr_array_new ();
  g_ptr_array_add (s_fonts, gdk_font_load (name));
  return s_fonts->len;
}
XAPI XFontStruct *XQueryFont (Display *d, XID fid)
{
  (void) d;
  GdkFont *f = font_of (fid);
  if (!f) return NULL;
  XFontStruct *fs = calloc (1, sizeof *fs);
  fs->fid = fid;
  fs->ascent = f->ascent;
  fs->descent = f->descent;
  fs->min_char_or_byte2 = 32;
  fs->max_char_or_byte2 = 255;
  fs->max_bounds.width = (short) gdk_char_width (f, 'W');
  fs->min_bounds.width = (short) gdk_char_width (f, 'i');
  fs->max_bounds.ascent = fs->min_bounds.ascent = (short) f->ascent;
  fs->max_bounds.descent = fs->min_bounds.descent = (short) f->descent;
  return fs;
}
XAPI XFontStruct *XLoadQueryFont (Display *d, _Xconst char *name) { return XQueryFont (d, XLoadFont (d, name)); }
XAPI int XUnloadFont (Display *d, Font fid) { (void) d; (void) fid; return 1; }
XAPI int XFreeFont (Display *d, XFontStruct *fs) { (void) d; free (fs); return 1; }
XAPI int XFreeFontInfo (char **names, XFontStruct *info, int n) { (void) names; (void) n; free (info); return 1; }
XAPI char **XListFonts (Display *d, _Xconst char *pattern, int max, int *count) { (void) d; (void) pattern; (void) max; *count = 0; return NULL; }
XAPI int XFreeFontNames (char **list) { (void) list; return 1; }
XAPI int XSetFont (Display *d, GC gc, Font fid) { (void) d; gc->v.font = fid; gc->font = font_of (fid); return 1; }
XAPI int XTextWidth (XFontStruct *fs, _Xconst char *s, int n) { return fs ? gdk_text_width (font_of (fs->fid), s, n) : 0; }
XAPI int XDrawString (Display *d, Drawable dr, GC gc, int x, int y, _Xconst char *s, int n)
{
  (void) d;
  GdkFont *f = gc->font ? gc->font : gdk_font_load ("fixed");
  gdk_draw_text (xwin (dr), f, gc->gdk, x, y, s, n);
  return 1;
}
XAPI int XDrawImageString (Display *d, Drawable dr, GC gc, int x, int y, _Xconst char *s, int n)
{
  GdkFont *f = gc->font ? gc->font : gdk_font_load ("fixed");
  GdkGC *bg = gdk_gc_new (xwin (dr));
  GdkColor c = pixel_color (gc->v.background);
  gdk_gc_set_foreground (bg, &c);
  gdk_draw_rectangle (xwin (dr), bg, TRUE, x, y - f->ascent, gdk_text_width (f, s, n), f->ascent + f->descent);
  g_object_unref (bg);
  return XDrawString (d, dr, gc, x, y, s, n);
}

/* ---------- images (32 bits per pixel) ---------- */
static int img_destroy (XImage *img) { free (img->data); free (img); return 1; }
static unsigned long img_get (XImage *img, int x, int y) { return ((unsigned int *) (img->data + y * img->bytes_per_line))[x] & 0xffffff; }
static int img_put (XImage *img, int x, int y, unsigned long p) { ((unsigned int *) (img->data + y * img->bytes_per_line))[x] = (unsigned int) p; return 1; }

XAPI XImage *XCreateImage (Display *d, Visual *v, unsigned int depth, int format, int offset, char *data,
                           unsigned int w, unsigned int h, int pad, int bpl)
{
  (void) d; (void) v; (void) offset; (void) pad;
  XImage *img = calloc (1, sizeof *img);
  img->width = w;
  img->height = h;
  img->format = format;
  img->data = data;
  img->byte_order = LSBFirst;
  img->bitmap_unit = 32;
  img->bitmap_pad = 32;
  img->depth = depth;
  img->bits_per_pixel = 32;
  img->bytes_per_line = bpl ? bpl : (int) w * 4;
  img->red_mask = 0xff0000;
  img->green_mask = 0x00ff00;
  img->blue_mask = 0x0000ff;
  img->f.destroy_image = img_destroy;
  img->f.get_pixel = img_get;
  img->f.put_pixel = img_put;
  return img;
}

XAPI int XPutImage (Display *d, Drawable dr, GC gc, XImage *img, int sx, int sy, int dx, int dy, unsigned int w, unsigned int h)
{
  (void) d;
  if (!img || !img->data) return 1;
  gdk_draw_rgb_32_image (xwin (dr), gc->gdk, dx, dy, w, h, GDK_RGB_DITHER_NONE,
                         (const guchar *) img->data + sy * img->bytes_per_line + sx * 4, img->bytes_per_line);
  return 1;
}

XAPI XImage *XGetImage (Display *d, Drawable dr, int x, int y, unsigned int w, unsigned int h, unsigned long planes, int format)
{
  (void) planes;
  GdkImage *g = gdk_image_get (xwin (dr), x, y, w, h);
  if (!g) return NULL;
  char *data = malloc ((size_t) g->bpl * h);
  memcpy (data, g->mem, (size_t) g->bpl * h);
  XImage *img = XCreateImage (d, NULL, 24, format, 0, data, w, h, 32, g->bpl);
  gdk_image_destroy (g);
  return img;
}

/* ======================================================================
 * SHAPE extension
 * ====================================================================== */
XAPI Bool XShapeQueryExtension (Display *d, int *event_base, int *error_base) { (void) d; *event_base = 64; *error_base = 128; return True; }
XAPI Status XShapeQueryVersion (Display *d, int *major, int *minor) { (void) d; *major = 1; *minor = 1; return 1; }
XAPI void XShapeCombineMask (Display *d, Window w, int kind, int x, int y, Pixmap mask, int op)
{
  (void) d; (void) op;
  if (kind != ShapeBounding) return;
  { GdkWindow *win = xwin (w); if (win) gdk_window_shape_combine_mask (win, mask ? xwin (mask) : NULL, x, y); }
}
