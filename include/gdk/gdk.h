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
#include <pango/pangocairo.h>
#include <gdk/gdkkeysyms.h>

/* The shared build exports the API; the static one keeps it hidden, so a
 * program never exports GTK symbols (even when linked with -rdynamic). */
#if !defined (GTKQT_COMPILATION) || defined (GTKQT_EXPORT_API)
#pragma GCC visibility push(default)
#define GTKQT_VISIBILITY_PUSHED_GDK
#endif
G_BEGIN_DECLS

typedef struct _GdkWindow GdkWindow;   /* opaque; one per widget */
#if GTKQT_API_VERSION == 1
typedef gulong GdkAtom;                /* GTK 1: an atom is a number, the same as X's Atom */
#else
typedef struct _GdkAtom *GdkAtom;
#endif
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
  GDK_PROPERTY_NOTIFY = 16,
  GDK_SELECTION_CLEAR = 17,
  GDK_SELECTION_REQUEST = 18,
  GDK_SELECTION_NOTIFY = 19,
  GDK_PROXIMITY_IN = 20,
  GDK_PROXIMITY_OUT = 21,
  GDK_DRAG_ENTER = 22,
  GDK_DRAG_LEAVE = 23,
  GDK_DRAG_MOTION = 24,
  GDK_DRAG_STATUS = 25,
  GDK_DROP_START = 26,
  GDK_DROP_FINISHED = 27,
  GDK_CLIENT_EVENT = 28,
  GDK_VISIBILITY_NOTIFY = 29,
  GDK_NO_EXPOSE = 30,
  GDK_SCROLL = 31
} GdkEventType;

typedef enum {
  GDK_SHIFT_MASK   = 1 << 0,
  GDK_LOCK_MASK    = 1 << 1,
  GDK_CONTROL_MASK = 1 << 2,
  GDK_MOD1_MASK    = 1 << 3,
  GDK_ALT_MASK     = GDK_MOD1_MASK,
  GDK_MOD2_MASK    = 1 << 4,   /* Num Lock on X11 */
  GDK_MOD3_MASK    = 1 << 5,
  GDK_MOD4_MASK    = 1 << 6,
  GDK_MOD5_MASK    = 1 << 7,
  GDK_BUTTON1_MASK = 1 << 8,
  GDK_BUTTON2_MASK = 1 << 9,
  GDK_BUTTON3_MASK = 1 << 10,
  GDK_BUTTON4_MASK = 1 << 11,
  GDK_BUTTON5_MASK = 1 << 12,
  GDK_SUPER_MASK   = 1 << 26,
  GDK_HYPER_MASK   = 1 << 27,
  GDK_META_MASK    = 1 << 28,
  GDK_RELEASE_MASK = 1 << 30,
  GDK_MODIFIER_MASK = 0x5c001fff
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

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  GdkAtom message_type;
  gushort data_format;
  union { char b[20]; short s[10]; long l[5]; } data;
} GdkEventClient;

typedef struct {
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  GdkAtom selection, target, property;
  guint32 requestor;
  guint32 time;
} GdkEventSelection;

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
  GdkEventClient client;
  GdkEventSelection selection;
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
/* The GDK lock: GUI work happens on the main thread with Qt */
#define GDK_THREADS_ENTER() gdk_threads_enter ()
#define GDK_THREADS_LEAVE() gdk_threads_leave ()
guint       gdk_threads_add_idle (GSourceFunc function, gpointer data);
guint       gdk_threads_add_timeout (guint interval, GSourceFunc function, gpointer data);

/* ======================================================================
 * GTK 1/2 GDK: drawables, graphics contexts, atoms, colormaps.
 * A GdkWindow is a widget's window (or a GtkTextView border window); a
 * GdkPixmap is an off-screen image. Both are GdkDrawables (GObjects).
 * Drawing on a window works while its expose handler runs, like on X
 * without backing store; pixmaps keep their contents.
 * ====================================================================== */
typedef GdkWindow GdkDrawable;
typedef GdkWindow GdkPixmap;
typedef GdkWindow GdkBitmap;
typedef struct _GdkGC GdkGC;
typedef struct _GdkFont GdkFont;
typedef struct _GdkColormap GdkColormap;
typedef struct _GdkVisual GdkVisual;
typedef struct _GdkDragContext GdkDragContext;
typedef struct { gint x, y; } GdkPoint;
typedef struct { gint x1, y1, x2, y2; } GdkSegment;

#define GDK_TYPE_DRAWABLE (gdk_drawable_get_type ())
#define GDK_TYPE_WINDOW   GDK_TYPE_DRAWABLE
#define GDK_TYPE_PIXMAP   GDK_TYPE_DRAWABLE
#define GDK_DRAWABLE(o)   ((GdkDrawable *) (o))
#define GDK_WINDOW(o)     ((GdkWindow *) (o))
#define GDK_PIXMAP(o)     ((GdkPixmap *) (o))
#define GDK_IS_DRAWABLE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_DRAWABLE))
#define GDK_IS_WINDOW(o)  GDK_IS_DRAWABLE (o)
GType gdk_drawable_get_type (void) G_GNUC_CONST;
#define GDK_TYPE_GC       (gdk_gc_get_type ())
#define GDK_GC(o)         ((GdkGC *) (o))
#define GDK_IS_GC(o)      (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_GC))
GType gdk_gc_get_type (void) G_GNUC_CONST;

#if GTKQT_API_VERSION == 1
#define _GDK_MAKE_ATOM(val)     ((GdkAtom) (val))
#define GDK_NONE                0L   /* GTK 1: also "no window" / "no cursor" (gdk_pointer_grab) */
#else
#define _GDK_MAKE_ATOM(val)     ((GdkAtom) GUINT_TO_POINTER (val))
#define GDK_NONE                _GDK_MAKE_ATOM (0)
#endif
#define GDK_SELECTION_PRIMARY   _GDK_MAKE_ATOM (1)
#define GDK_SELECTION_SECONDARY _GDK_MAKE_ATOM (2)
#define GDK_SELECTION_CLIPBOARD _GDK_MAKE_ATOM (69)
#define GDK_TARGET_STRING       _GDK_MAKE_ATOM (31)
#define GDK_SELECTION_TYPE_STRING _GDK_MAKE_ATOM (31)
GdkAtom gdk_atom_intern (const gchar *atom_name, gboolean only_if_exists);
GdkAtom gdk_atom_intern_static_string (const gchar *atom_name);
gchar  *gdk_atom_name (GdkAtom atom);

typedef enum { GDK_COPY, GDK_INVERT, GDK_XOR, GDK_CLEAR, GDK_AND, GDK_AND_REVERSE, GDK_AND_INVERT, GDK_NOOP,
               GDK_OR, GDK_EQUIV, GDK_OR_REVERSE, GDK_COPY_INVERT, GDK_OR_INVERT, GDK_NAND, GDK_NOR, GDK_SET } GdkFunction;
typedef enum { GDK_SOLID, GDK_TILED, GDK_STIPPLED, GDK_OPAQUE_STIPPLED } GdkFill;
typedef enum { GDK_LINE_SOLID, GDK_LINE_ON_OFF_DASH, GDK_LINE_DOUBLE_DASH } GdkLineStyle;
typedef enum { GDK_CAP_NOT_LAST, GDK_CAP_BUTT, GDK_CAP_ROUND, GDK_CAP_PROJECTING } GdkCapStyle;
typedef enum { GDK_JOIN_MITER, GDK_JOIN_ROUND, GDK_JOIN_BEVEL } GdkJoinStyle;
typedef enum { GDK_CLIP_BY_CHILDREN, GDK_INCLUDE_INFERIORS } GdkSubwindowMode;
typedef enum {
  GDK_GC_FOREGROUND = 1 << 0, GDK_GC_BACKGROUND = 1 << 1, GDK_GC_FONT = 1 << 2, GDK_GC_FUNCTION = 1 << 3,
  GDK_GC_FILL = 1 << 4, GDK_GC_TILE = 1 << 5, GDK_GC_STIPPLE = 1 << 6, GDK_GC_CLIP_MASK = 1 << 7,
  GDK_GC_SUBWINDOW = 1 << 8, GDK_GC_TS_X_ORIGIN = 1 << 9, GDK_GC_TS_Y_ORIGIN = 1 << 10,
  GDK_GC_CLIP_X_ORIGIN = 1 << 11, GDK_GC_CLIP_Y_ORIGIN = 1 << 12, GDK_GC_EXPOSURES = 1 << 13,
  GDK_GC_LINE_WIDTH = 1 << 14, GDK_GC_LINE_STYLE = 1 << 15, GDK_GC_CAP_STYLE = 1 << 16, GDK_GC_JOIN_STYLE = 1 << 17
} GdkGCValuesMask;
typedef struct {
  GdkColor foreground, background;
  GdkFont *font;
  GdkFunction function;
  GdkFill fill;
  GdkPixmap *tile, *stipple, *clip_mask;
  GdkSubwindowMode subwindow_mode;
  gint ts_x_origin, ts_y_origin, clip_x_origin, clip_y_origin;
  gint graphics_exposures;
  gint line_width;
  GdkLineStyle line_style;
  GdkCapStyle cap_style;
  GdkJoinStyle join_style;
} GdkGCValues;

GdkGC   *gdk_gc_new (GdkDrawable *drawable);
GdkGC   *gdk_gc_new_with_values (GdkDrawable *drawable, GdkGCValues *values, GdkGCValuesMask values_mask);
void     gdk_gc_get_values (GdkGC *gc, GdkGCValues *values);
void     gdk_gc_set_values (GdkGC *gc, GdkGCValues *values, GdkGCValuesMask values_mask);
void     gdk_gc_set_foreground (GdkGC *gc, const GdkColor *color);
void     gdk_gc_set_background (GdkGC *gc, const GdkColor *color);
void     gdk_gc_set_rgb_fg_color (GdkGC *gc, const GdkColor *color);
void     gdk_gc_set_rgb_bg_color (GdkGC *gc, const GdkColor *color);
void     gdk_gc_set_font (GdkGC *gc, GdkFont *font);
void     gdk_gc_set_function (GdkGC *gc, GdkFunction function);
void     gdk_gc_set_fill (GdkGC *gc, GdkFill fill);
void     gdk_gc_set_tile (GdkGC *gc, GdkPixmap *tile);
void     gdk_gc_set_stipple (GdkGC *gc, GdkPixmap *stipple);
void     gdk_gc_set_ts_origin (GdkGC *gc, gint x, gint y);
void     gdk_gc_set_clip_origin (GdkGC *gc, gint x, gint y);
void     gdk_gc_set_clip_mask (GdkGC *gc, GdkBitmap *mask);
void     gdk_gc_set_clip_rectangle (GdkGC *gc, const GdkRectangle *rectangle);
void     gdk_gc_set_subwindow (GdkGC *gc, GdkSubwindowMode mode);
void     gdk_gc_set_exposures (GdkGC *gc, gboolean exposures);
void     gdk_gc_set_line_attributes (GdkGC *gc, gint line_width, GdkLineStyle line_style, GdkCapStyle cap_style, GdkJoinStyle join_style);
void     gdk_gc_set_dashes (GdkGC *gc, gint dash_offset, gint8 dash_list[], gint n);
void     gdk_gc_copy (GdkGC *dst_gc, GdkGC *src_gc);
GdkGC   *gdk_gc_ref (GdkGC *gc);
void     gdk_gc_unref (GdkGC *gc);
#define  gdk_gc_destroy gdk_gc_unref

void     gdk_draw_point (GdkDrawable *drawable, GdkGC *gc, gint x, gint y);
void     gdk_draw_points (GdkDrawable *drawable, GdkGC *gc, const GdkPoint *points, gint n_points);
void     gdk_draw_line (GdkDrawable *drawable, GdkGC *gc, gint x1, gint y1, gint x2, gint y2);
void     gdk_draw_lines (GdkDrawable *drawable, GdkGC *gc, const GdkPoint *points, gint n_points);
void     gdk_draw_segments (GdkDrawable *drawable, GdkGC *gc, const GdkSegment *segs, gint n_segs);
void     gdk_draw_rectangle (GdkDrawable *drawable, GdkGC *gc, gboolean filled, gint x, gint y, gint width, gint height);
void     gdk_draw_arc (GdkDrawable *drawable, GdkGC *gc, gboolean filled, gint x, gint y, gint width, gint height, gint angle1, gint angle2);
void     gdk_draw_polygon (GdkDrawable *drawable, GdkGC *gc, gboolean filled, const GdkPoint *points, gint n_points);
void     gdk_draw_drawable (GdkDrawable *drawable, GdkGC *gc, GdkDrawable *src, gint xsrc, gint ysrc, gint xdest, gint ydest, gint width, gint height);
#define  gdk_draw_pixmap gdk_draw_drawable
#define  gdk_window_copy_area(d, gc, x, y, s, sx, sy, w, h) gdk_draw_drawable ((d), (gc), (s), (sx), (sy), (x), (y), (w), (h))
void     gdk_draw_pixbuf (GdkDrawable *drawable, GdkGC *gc, const GdkPixbuf *pixbuf, gint src_x, gint src_y, gint dest_x, gint dest_y,
                          gint width, gint height, gint dither, gint x_dither, gint y_dither);
void     gdk_draw_layout (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, PangoLayout *layout);
void     gdk_draw_layout_line (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, PangoLayoutLine *line);
void     gdk_draw_layout_with_colors (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, PangoLayout *layout,
                                      const GdkColor *foreground, const GdkColor *background);
void     gdk_drawable_get_size (GdkDrawable *drawable, gint *width, gint *height);
gint     gdk_drawable_get_depth (GdkDrawable *drawable);
#define  gdk_window_get_size gdk_drawable_get_size
void     gdk_window_get_geometry (GdkWindow *window, gint *x, gint *y, gint *width, gint *height, gint *depth);
void     gdk_window_get_position (GdkWindow *window, gint *x, gint *y);
gint     gdk_window_get_origin (GdkWindow *window, gint *x, gint *y);
GdkWindow *gdk_window_get_pointer (GdkWindow *window, gint *x, gint *y, GdkModifierType *mask);

/* Devices (GTK 3): one pointer and one keyboard, Qt's */
typedef struct _GdkSeat GdkSeat;
typedef struct _GdkDeviceManager GdkDeviceManager;
typedef enum { GDK_SOURCE_MOUSE, GDK_SOURCE_PEN, GDK_SOURCE_ERASER, GDK_SOURCE_CURSOR, GDK_SOURCE_KEYBOARD,
               GDK_SOURCE_TOUCHSCREEN, GDK_SOURCE_TOUCHPAD, GDK_SOURCE_TRACKPOINT, GDK_SOURCE_TABLET_PAD } GdkInputSource;
GdkWindow  *gdk_window_get_device_position (GdkWindow *window, GdkDevice *device, gint *x, gint *y, GdkModifierType *mask);
void        gdk_device_get_position (GdkDevice *device, GdkScreen **screen, gint *x, gint *y);
const gchar *gdk_device_get_name (GdkDevice *device);
GdkInputSource gdk_device_get_source (GdkDevice *device);
GdkDevice  *gdk_device_get_associated_device (GdkDevice *device);
GdkSeat    *gdk_display_get_default_seat (GdkDisplay *display);
GdkDevice  *gdk_seat_get_pointer (GdkSeat *seat);
GdkDevice  *gdk_seat_get_keyboard (GdkSeat *seat);
GdkDeviceManager *gdk_display_get_device_manager (GdkDisplay *display);
GdkDevice  *gdk_device_manager_get_client_pointer (GdkDeviceManager *device_manager);
void     gdk_window_clear (GdkWindow *window);
void     gdk_window_clear_area (GdkWindow *window, gint x, gint y, gint width, gint height);
void     gdk_window_clear_area_e (GdkWindow *window, gint x, gint y, gint width, gint height);
void     gdk_window_set_background (GdkWindow *window, const GdkColor *color);
void     gdk_window_set_back_pixmap (GdkWindow *window, GdkPixmap *pixmap, gboolean parent_relative);
void     gdk_window_process_updates (GdkWindow *window, gboolean update_children);
void     gdk_window_process_all_updates (void);
void     gdk_window_raise (GdkWindow *window);
void     gdk_window_lower (GdkWindow *window);
void     gdk_window_get_user_data (GdkWindow *window, gpointer *data);
GdkWindow *gdk_get_default_root_window (void);
#define  GDK_ROOT_PARENT() gdk_get_default_root_window ()
GdkPixmap *gdk_pixmap_new (GdkDrawable *drawable, gint width, gint height, gint depth);
GdkPixmap *gdk_pixmap_ref (GdkPixmap *pixmap);
void       gdk_pixmap_unref (GdkPixmap *pixmap);
#define    gdk_bitmap_ref   gdk_pixmap_ref
#define    gdk_bitmap_unref gdk_pixmap_unref

/* Colormaps: TrueColor only, every colour can be allocated. */
GdkColormap *gdk_colormap_get_system (void);
GdkVisual   *gdk_visual_get_system (void);
GdkColormap *gdk_colormap_new (GdkVisual *visual, gboolean allocate);
GdkColormap *gdk_colormap_ref (GdkColormap *cmap);
void         gdk_colormap_unref (GdkColormap *cmap);
gboolean     gdk_colormap_alloc_color (GdkColormap *colormap, GdkColor *color, gboolean writeable, gboolean best_match);
gint         gdk_colormap_alloc_colors (GdkColormap *colormap, GdkColor *colors, gint n_colors, gboolean writeable,
                                        gboolean best_match, gboolean *success);
void         gdk_colormap_free_colors (GdkColormap *colormap, const GdkColor *colors, gint n_colors);
void         gdk_colormap_query_color (GdkColormap *colormap, gulong pixel, GdkColor *result);
gboolean     gdk_color_alloc (GdkColormap *colormap, GdkColor *color);
gboolean     gdk_color_white (GdkColormap *colormap, GdkColor *color);
gboolean     gdk_color_black (GdkColormap *colormap, GdkColor *color);
GdkColor    *gdk_color_copy (const GdkColor *color);
void         gdk_color_free (GdkColor *color);
gboolean     gdk_color_equal (const GdkColor *a, const GdkColor *b);
gchar       *gdk_color_to_string (const GdkColor *color);
#define GDK_TYPE_COLOR (gdk_color_get_type ())
GType        gdk_color_get_type (void) G_GNUC_CONST;
GdkColormap *gdk_drawable_get_colormap (GdkDrawable *drawable);
#define gdk_window_get_colormap gdk_drawable_get_colormap

/* Pango on GDK */
PangoContext *gdk_pango_context_get (void);
void gdk_cairo_set_source_pixmap (cairo_t *cr, GdkPixmap *pixmap, double x, double y);

/* Drag and drop (the receiving side) */
typedef enum {
  GDK_ACTION_DEFAULT = 1 << 0, GDK_ACTION_COPY = 1 << 1, GDK_ACTION_MOVE = 1 << 2,
  GDK_ACTION_LINK = 1 << 3, GDK_ACTION_PRIVATE = 1 << 4, GDK_ACTION_ASK = 1 << 5
} GdkDragAction;
typedef enum { GDK_DRAG_PROTO_MOTIF, GDK_DRAG_PROTO_XDND, GDK_DRAG_PROTO_ROOTWIN, GDK_DRAG_PROTO_NONE,
               GDK_DRAG_PROTO_WIN32_DROPFILES, GDK_DRAG_PROTO_OLE2, GDK_DRAG_PROTO_LOCAL } GdkDragProtocol;
struct _GdkDragContext {
  GObject parent_instance;
  GdkDragProtocol protocol;
  gboolean is_source;
  GdkWindow *source_window;
  GdkWindow *dest_window;
  GList *targets;            /* GdkAtom */
  GdkDragAction actions;
  GdkDragAction suggested_action;
  GdkDragAction action;
  guint32 start_time;
  /*< private >*/
  gpointer windowing_data;
};
#define GDK_TYPE_DRAG_CONTEXT (gdk_drag_context_get_type ())
#define GDK_DRAG_CONTEXT(o)   ((GdkDragContext *) (o))
GType gdk_drag_context_get_type (void) G_GNUC_CONST;
void  gdk_drag_status (GdkDragContext *context, GdkDragAction action, guint32 time_);
GList *gdk_drag_context_list_targets (GdkDragContext *context);
GdkDragAction gdk_drag_context_get_actions (GdkDragContext *context);
GdkDragAction gdk_drag_context_get_suggested_action (GdkDragContext *context);
GdkDragAction gdk_drag_context_get_selected_action (GdkDragContext *context);

G_END_DECLS
#ifdef GTKQT_VISIBILITY_PUSHED_GDK
#pragma GCC visibility pop
#endif

#endif /* GTKQT_GDK_H */
