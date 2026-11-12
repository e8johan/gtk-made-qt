/*
 * Xlib next to GDK: X ids of GDK windows, properties, atoms, window-manager
 * requests, and Xlib drawing landing in the GDK window's pixels.
 */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

static int drew;
static gboolean expose (GtkWidget *w, GdkEventExpose *ev, gpointer data)
{
  (void) ev; (void) data;
  Display *dpy = GDK_DISPLAY ();
  Window win = GDK_WINDOW_XWINDOW (w->window);
  GC gc = XCreateGC (dpy, win, 0, NULL);
  XSetForeground (dpy, gc, 0xff0000);
  XFillRectangle (dpy, win, gc, 0, 0, 20, 20);
  XFreeGC (dpy, gc);
  drew = 1;
  return FALSE;
}

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  Display *dpy = XOpenDisplay (NULL);
  CHECK (dpy && dpy == GDK_DISPLAY ());
  CHECK (DefaultScreen (dpy) == 0 && RootWindow (dpy, 0) == GDK_ROOT_WINDOW ());
  CHECK (DisplayWidth (dpy, 0) > 0 && DefaultDepth (dpy, 0) == 24);

  /* atoms: X's predefined numbers and GDK atoms agree */
  CHECK (XInternAtom (dpy, "WM_NAME", False) == XA_WM_NAME);
  Atom a = XInternAtom (dpy, "_GTKQT_TEST", False);
  char *name = XGetAtomName (dpy, a);
  CHECK (strcmp (name, "_GTKQT_TEST") == 0);
  XFree (name);
  CHECK (gdk_x11_atom_to_xatom (gdk_atom_intern ("_GTKQT_TEST", FALSE)) == a);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (area, 40, 40);
  gtk_container_add (GTK_CONTAINER (window), area);
  g_signal_connect (area, "expose-event", G_CALLBACK (expose), NULL);
  gtk_widget_show_all (window);
  Window xw = GDK_WINDOW_XWINDOW (window->window);
  CHECK (xw != None && gdk_window_lookup (xw) == window->window);

  /* properties, and WM_NAME setting the title */
  XStoreName (dpy, xw, "from Xlib");
  char *title = NULL;
  CHECK (XFetchName (dpy, xw, &title) && strcmp (title, "from Xlib") == 0);
  XFree (title);
  long v[3] = { 1, 2, 3 };
  XChangeProperty (dpy, xw, a, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) v, 3);
  XChangeProperty (dpy, xw, a, XA_CARDINAL, 32, PropModeAppend, (unsigned char *) v, 1);
  Atom type; int format; unsigned long n, after; unsigned char *data = NULL;
  CHECK (XGetWindowProperty (dpy, xw, a, 0, 100, False, AnyPropertyType, &type, &format, &n, &after, &data) == Success);
  CHECK (type == XA_CARDINAL && format == 32 && n == 4 && ((long *) data)[3] == 1);
  XFree (data);

  /* the "window manager" announces what it supports */
  CHECK (XGetWindowProperty (dpy, GDK_ROOT_WINDOW (), XInternAtom (dpy, "_NET_SUPPORTED", False), 0, 100, False, XA_ATOM,
                             &type, &format, &n, &after, &data) == Success && n > 0);
  XFree (data);
  XEvent ev;
  memset (&ev, 0, sizeof ev);
  ev.xclient.type = ClientMessage;
  ev.xclient.window = xw;
  ev.xclient.message_type = XInternAtom (dpy, "_NET_WM_STATE", False);
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = 1;
  ev.xclient.data.l[1] = XInternAtom (dpy, "_NET_WM_STATE_ABOVE", False);
  CHECK (XSendEvent (dpy, GDK_ROOT_WINDOW (), False, SubstructureRedirectMask, &ev));

  int x = 0, y = 0;
  unsigned int w = 0, h = 0;
  CHECK (XParseGeometry ("200x100-10+20", &x, &y, &w, &h) == (WidthValue | HeightValue | XValue | YValue | XNegative));
  CHECK (w == 200 && h == 100 && x == -10 && y == 20);

  for (int i = 0; i < 20 && !drew; i++) { while (gtk_events_pending ()) gtk_main_iteration (); g_usleep (10000); }
  CHECK (drew);
  /* the Xlib drawing is in the GDK window's pixels */
  XImage *img = XGetImage (dpy, GDK_WINDOW_XWINDOW (area->window), 5, 5, 1, 1, AllPlanes, ZPixmap);
  CHECK (img && (XGetPixel (img, 0, 0) & 0xffffff) == 0xff0000);
  XDestroyImage (img);
  printf ("x11 tests passed\n");
  return 0;
}
