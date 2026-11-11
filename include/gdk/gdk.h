/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Copyright (C) 2010-2014 Johan Thelin, Robin Burchell and contributors
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version. See COPYING for details.
 */
#ifndef GTKQT_GDK_H
#define GTKQT_GDK_H

#include <gtk/gtkqt-version.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>

G_BEGIN_DECLS

typedef struct _GdkWindow GdkWindow;   /* opaque; one per widget */
typedef GdkWindow GdkSurface;          /* GTK 4 name */
typedef struct _GdkDisplay GdkDisplay;
typedef struct _GdkScreen GdkScreen;
typedef struct _GdkPixbuf GdkPixbuf;   /* opaque, wraps a QImage */
typedef struct _GdkDevice GdkDevice;
typedef guint32 GdkAtom_;

typedef struct { int x, y, width, height; } GdkRectangle;
typedef struct { double red, green, blue, alpha; } GdkRGBA;
typedef struct { guint32 pixel; guint16 red, green, blue; } GdkColor;

typedef enum {
  GDK_NOTHING = -1,
  GDK_DELETE = 0,
  GDK_DESTROY = 1,
  GDK_EXPOSE = 2,
  GDK_MOTION_NOTIFY = 3,
  GDK_BUTTON_PRESS = 4,
  GDK_2BUTTON_PRESS = 5,
  GDK_DOUBLE_BUTTON_PRESS = GDK_2BUTTON_PRESS,
  GDK_3BUTTON_PRESS = 6,
  GDK_TRIPLE_BUTTON_PRESS = GDK_3BUTTON_PRESS,
  GDK_BUTTON_RELEASE = 7,
  GDK_KEY_PRESS = 8,
  GDK_KEY_RELEASE = 9,
  GDK_ENTER_NOTIFY = 10,
  GDK_LEAVE_NOTIFY = 11,
  GDK_FOCUS_CHANGE = 12,
  GDK_CONFIGURE = 13,
  GDK_MAP = 14,
  GDK_UNMAP = 15,
  GDK_SCROLL = 31
} GdkEventType;

typedef enum {
  GDK_SHIFT_MASK   = 1 << 0,
  GDK_LOCK_MASK    = 1 << 1,
  GDK_CONTROL_MASK = 1 << 2,
  GDK_MOD1_MASK    = 1 << 3,
  GDK_ALT_MASK     = GDK_MOD1_MASK,
  GDK_BUTTON1_MASK = 1 << 8,
  GDK_BUTTON2_MASK = 1 << 9,
  GDK_BUTTON3_MASK = 1 << 10,
  GDK_SUPER_MASK   = 1 << 26,
  GDK_META_MASK    = 1 << 28
} GdkModifierType;

typedef enum {
  GDK_EXPOSURE_MASK            = 1 << 1,
  GDK_POINTER_MOTION_MASK      = 1 << 2,
  GDK_POINTER_MOTION_HINT_MASK = 1 << 3,
  GDK_BUTTON_MOTION_MASK       = 1 << 4,
  GDK_BUTTON1_MOTION_MASK      = 1 << 5,
  GDK_BUTTON2_MOTION_MASK      = 1 << 6,
  GDK_BUTTON3_MOTION_MASK      = 1 << 7,
  GDK_BUTTON_PRESS_MASK        = 1 << 8,
  GDK_BUTTON_RELEASE_MASK      = 1 << 9,
  GDK_KEY_PRESS_MASK           = 1 << 10,
  GDK_KEY_RELEASE_MASK         = 1 << 11,
  GDK_ENTER_NOTIFY_MASK        = 1 << 12,
  GDK_LEAVE_NOTIFY_MASK        = 1 << 13,
  GDK_FOCUS_CHANGE_MASK        = 1 << 14,
  GDK_STRUCTURE_MASK           = 1 << 15,
  GDK_SCROLL_MASK              = 1 << 21,
  GDK_SMOOTH_SCROLL_MASK       = 1 << 23,
  GDK_ALL_EVENTS_MASK          = 0x3FFFFFE
} GdkEventMask;

typedef enum {
  GDK_SCROLL_UP, GDK_SCROLL_DOWN, GDK_SCROLL_LEFT, GDK_SCROLL_RIGHT, GDK_SCROLL_SMOOTH
} GdkScrollDirection;

typedef enum {
  GDK_CROSSING_NORMAL, GDK_CROSSING_GRAB, GDK_CROSSING_UNGRAB
} GdkCrossingMode;

#define GDK_BUTTON_PRIMARY   1
#define GDK_BUTTON_MIDDLE    2
#define GDK_BUTTON_SECONDARY 3
#define GDK_CURRENT_TIME     0L

/* GTK 2/3 style public event structures. In GTK 4 GdkEvent is opaque; the
 * gdk_event_get_*() accessors below work in every API level. */
typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
} GdkEventAny;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  GdkRectangle area;
  void *region;
  gint count;
} GdkEventExpose;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  guint32 time;
  gdouble x, y;
  gdouble *axes;
  guint state;
  gint16 is_hint;
  GdkDevice *device;
  gdouble x_root, y_root;
} GdkEventMotion;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  guint32 time;
  gdouble x, y;
  gdouble *axes;
  guint state;
  guint button;
  GdkDevice *device;
  gdouble x_root, y_root;
} GdkEventButton;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  guint32 time;
  gdouble x, y;
  guint state;
  GdkScrollDirection direction;
  GdkDevice *device;
  gdouble x_root, y_root;
  gdouble delta_x, delta_y;
  guint is_stop : 1;
} GdkEventScroll;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  guint32 time;
  guint state;
  guint keyval;
  gint length;
  gchar *string;
  guint16 hardware_keycode;
  guint8 group;
  guint is_modifier : 1;
} GdkEventKey;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  GdkWindow *subwindow;
  guint32 time;
  gdouble x, y;
  gdouble x_root, y_root;
  GdkCrossingMode mode;
  gint detail;
  gboolean focus;
  guint state;
} GdkEventCrossing;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  gint16 in;
} GdkEventFocus;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  gint x, y;
  gint width, height;
} GdkEventConfigure;

typedef union _GdkEvent {
  GdkEventType type;
  GdkEventAny any;
  GdkEventExpose expose;
  GdkEventMotion motion;
  GdkEventButton button;
  GdkEventScroll scroll;
  GdkEventKey key;
  GdkEventCrossing crossing;
  GdkEventFocus focus_change;
  GdkEventConfigure configure;
} GdkEvent;

#define GDK_TYPE_EVENT (gdk_event_get_type ())
GType gdk_event_get_type (void);
#define GDK_TYPE_RECTANGLE (gdk_rectangle_get_type ())
GType gdk_rectangle_get_type (void);
#define GDK_TYPE_RGBA (gdk_rgba_get_type ())
GType gdk_rgba_get_type (void);

GdkEventType gdk_event_get_event_type (GdkEvent *event);
gboolean     gdk_event_get_coords (GdkEvent *event, gdouble *x, gdouble *y);
gboolean     gdk_event_get_root_coords (GdkEvent *event, gdouble *x, gdouble *y);
gboolean     gdk_event_get_button (GdkEvent *event, guint *button);
gboolean     gdk_event_get_keyval (GdkEvent *event, guint *keyval);
gboolean     gdk_event_get_state (GdkEvent *event, GdkModifierType *state);
guint32      gdk_event_get_time (GdkEvent *event);
GdkEvent    *gdk_event_copy (GdkEvent *event);
void         gdk_event_free (GdkEvent *event);

/* Key values */
guint    gdk_unicode_to_keyval (guint32 wc);
guint32  gdk_keyval_to_unicode (guint keyval);
const gchar *gdk_keyval_name (guint keyval);
guint    gdk_keyval_from_name (const gchar *keyval_name);
guint    gdk_keyval_to_lower (guint keyval);
guint    gdk_keyval_to_upper (guint keyval);

/* Colours */
gboolean gdk_rgba_parse (GdkRGBA *rgba, const gchar *spec);
gchar   *gdk_rgba_to_string (const GdkRGBA *rgba);
gboolean gdk_color_parse (const gchar *spec, GdkColor *color);
void     gdk_cairo_set_source_rgba (cairo_t *cr, const GdkRGBA *rgba);
void     gdk_cairo_set_source_color (cairo_t *cr, const GdkColor *color);
void     gdk_cairo_rectangle (cairo_t *cr, const GdkRectangle *rectangle);

/* Drawing: valid while the widget's draw/expose handler is running. */
cairo_t *gdk_cairo_create (GdkWindow *window);
int      gdk_window_get_width (GdkWindow *window);
int      gdk_window_get_height (GdkWindow *window);
void     gdk_window_invalidate_rect (GdkWindow *window, const GdkRectangle *rect, gboolean invalidate_children);
cairo_surface_t *gdk_window_create_similar_surface (GdkWindow *window, cairo_content_t content, int width, int height);

/* Pixbufs (minimal: load from file, scale, query size) */
#define GDK_TYPE_PIXBUF (gdk_pixbuf_get_type ())
GType      gdk_pixbuf_get_type (void);
GdkPixbuf *gdk_pixbuf_new_from_file (const char *filename, GError **error);
GdkPixbuf *gdk_pixbuf_new_from_file_at_size (const char *filename, int width, int height, GError **error);
int        gdk_pixbuf_get_width (const GdkPixbuf *pixbuf);
int        gdk_pixbuf_get_height (const GdkPixbuf *pixbuf);
void       gdk_cairo_set_source_pixbuf (cairo_t *cr, const GdkPixbuf *pixbuf, double pixbuf_x, double pixbuf_y);

/* Display / screen (just enough for common queries) */
GdkDisplay *gdk_display_get_default (void);
GdkScreen  *gdk_screen_get_default (void);
gint        gdk_screen_get_width (GdkScreen *screen);
gint        gdk_screen_get_height (GdkScreen *screen);
gint        gdk_screen_width (void);
gint        gdk_screen_height (void);
void        gdk_beep (void);
void        gdk_flush (void);
void        gdk_display_beep (GdkDisplay *display);
void        gdk_threads_init (void);
void        gdk_threads_enter (void);
void        gdk_threads_leave (void);
guint       gdk_threads_add_idle (GSourceFunc function, gpointer data);
guint       gdk_threads_add_timeout (guint interval, GSourceFunc function, gpointer data);

G_END_DECLS

#endif /* GTKQT_GDK_H */
