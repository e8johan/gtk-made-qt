/*
 * GTK made Qt - GDK's X11 interface.
 *
 * There is no X server: Xlib calls (X11/Xlib.h, -lX11) are served by GTK
 * made Qt's Xlib layer on top of GDK. A GdkWindow or GdkPixmap has an X id
 * that those calls understand, atoms are GDK atoms, and the Display is a
 * single fake connection whose structure Xlib's macros can read.
 */
#ifndef GTKQT_GDKX_H
#define GTKQT_GDKX_H

#include <gdk/gdk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#if !defined (GTKQT_COMPILATION) || defined (GTKQT_EXPORT_API)
#pragma GCC visibility push(default)
#define GTKQT_VISIBILITY_PUSHED_GDKX
#endif
G_BEGIN_DECLS

typedef guint32 GdkNativeWindow;

Display   *gdk_x11_get_default_xdisplay (void);
Window     gdk_x11_get_default_root_xwindow (void);
gint       gdk_x11_get_default_screen (void);
Screen    *gdk_x11_screen_get_xscreen (GdkScreen *screen);
XID        gdk_x11_drawable_get_xid (GdkDrawable *drawable);
Display   *gdk_x11_drawable_get_xdisplay (GdkDrawable *drawable);
gpointer   gdk_xid_table_lookup (XID xid);
GdkWindow *gdk_window_lookup (GdkNativeWindow anid);
GdkWindow *gdk_window_foreign_new (GdkNativeWindow anid);
GdkPixmap *gdk_pixmap_lookup (GdkNativeWindow anid);
GdkPixmap *gdk_pixmap_foreign_new (GdkNativeWindow anid);
Atom       gdk_x11_atom_to_xatom (GdkAtom atom);
GdkAtom    gdk_x11_xatom_to_atom (Atom xatom);
Atom       gdk_x11_get_xatom_by_name (const gchar *atom_name);
const gchar *gdk_x11_get_xatom_name (Atom xatom);
Visual    *gdk_x11_visual_get_xvisual (GdkVisual *visual);
GdkVisual *gdkx_visual_get (VisualID xvisualid);
Colormap   gdk_x11_colormap_get_xcolormap (GdkColormap *colormap);
guint32    gdk_x11_get_server_time (GdkWindow *window);
void       gdk_x11_window_set_user_time (GdkWindow *window, guint32 timestamp);
void       gdk_x11_grab_server (void);
void       gdk_x11_ungrab_server (void);

#define GDK_DISPLAY()               (gdk_x11_get_default_xdisplay ())
#define GDK_ROOT_WINDOW()           (gdk_x11_get_default_root_xwindow ())
#define GDK_WINDOW_XDISPLAY(win)    (gdk_x11_drawable_get_xdisplay (win))
#define GDK_WINDOW_XID(win)         (gdk_x11_drawable_get_xid (win))
#define GDK_WINDOW_XWINDOW(win)     (gdk_x11_drawable_get_xid (win))
#define GDK_DRAWABLE_XDISPLAY(win)  (gdk_x11_drawable_get_xdisplay (win))
#define GDK_DRAWABLE_XID(win)       (gdk_x11_drawable_get_xid (win))
#define GDK_PIXMAP_XDISPLAY(win)    (gdk_x11_drawable_get_xdisplay (win))
#define GDK_PIXMAP_XID(win)         (gdk_x11_drawable_get_xid (win))
#define GDK_VISUAL_XVISUAL(vis)     (gdk_x11_visual_get_xvisual (vis))
#define GDK_COLORMAP_XDISPLAY(cmap) (gdk_x11_get_default_xdisplay ())
#define GDK_COLORMAP_XCOLORMAP(cmap) (gdk_x11_colormap_get_xcolormap (cmap))
#define GDK_SCREEN_XDISPLAY(s)      (gdk_x11_get_default_xdisplay ())
#define GDK_SCREEN_XSCREEN(s)       (gdk_x11_screen_get_xscreen (s))
#define GDK_DISPLAY_XDISPLAY(d)     (gdk_x11_get_default_xdisplay ())
#define GDK_FONT_XDISPLAY(font)     (gdk_x11_get_default_xdisplay ())
XImage    *gdk_x11_image_get_ximage (struct _GdkImage *image);
#define GDK_IMAGE_XIMAGE(image)     (gdk_x11_image_get_ximage (image))
#define GDK_IMAGE_XDISPLAY(image)   (gdk_x11_get_default_xdisplay ())
/* GTK 1: the Display was a variable */
#define gdk_display                 (gdk_x11_get_default_xdisplay ())
#define gdk_root_window             (gdk_x11_get_default_root_xwindow ())
#define gdk_screen                  (gdk_x11_get_default_screen ())
#define gdk_leader_window           (gdk_x11_get_default_root_xwindow ())

G_END_DECLS
#ifdef GTKQT_VISIBILITY_PUSHED_GDKX
#pragma GCC visibility pop
#endif

#endif
