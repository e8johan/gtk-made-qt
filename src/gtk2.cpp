/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * GTK 1/2 era API: GDK drawables and GCs (cairo), atoms, colormaps,
 * GtkStyle (from the Qt palette), Pango glue, GtkSettings, stock items and
 * icon factories, accelerators, GtkClipboard (QClipboard), drag-and-drop
 * targets (Qt drops), GtkIMContext, GtkItemFactory (menus from a table),
 * GtkFileSelection (QFileDialog), GtkFontSelectionDialog (QFontDialog).
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QClipboard>
#include <QMimeData>
#include <QScreen>

#include <cmath>

/* ======================================================================
 * GdkDrawable: widget windows, text view border windows, pixmaps
 * ====================================================================== */
typedef struct { GObjectClass parent_class; } GdkDrawableClass;
G_DEFINE_TYPE (GdkDrawable, gdk_drawable, G_TYPE_OBJECT)
static void gdk_drawable_init (GdkDrawable *) {}
static void drawable_finalize (GObject *o)
{
  auto *d = (GdkWindow *) o;
  if (d->kind == GTKQT_DRAWABLE_PIXMAP && d->surface) cairo_surface_destroy (d->surface);
  d->surface = nullptr;
  G_OBJECT_CLASS (gdk_drawable_parent_class)->finalize (o);
}
static void gdk_drawable_class_init (GdkDrawableClass *klass) { G_OBJECT_CLASS (klass)->finalize = drawable_finalize; }

GdkWindow *gtkqt_drawable_new (int kind, GtkWidget *widget)
{
  auto *d = (GdkWindow *) g_object_new (gdk_drawable_get_type (), nullptr);
  d->kind = kind;
  d->widget = widget;
  d->depth = 24;
  return d;
}

cairo_surface_t *gtkqt_drawable_target (GdkDrawable *d)
{
  if (!d) return nullptr;
  if (d->surface) return d->surface;
  if (d->kind == GTKQT_DRAWABLE_WINDOW && d->widget && d->widget->priv) return d->widget->priv->paint_surface;
  return nullptr;
}

static cairo_t *scratch_cr ()
{
  cairo_surface_t *s = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t *cr = cairo_create (s);
  cairo_surface_destroy (s);
  return cr;
}

extern "C" {

void gdk_drawable_get_size (GdkDrawable *d, gint *width, gint *height)
{
  int w = 0, h = 0;
  if (d) {
    if (d->kind == GTKQT_DRAWABLE_PIXMAP) { w = d->width; h = d->height; }
    else if (d->qw) { w = d->qw->width (); h = d->qw->height (); }
    else if (d->kind == GTKQT_DRAWABLE_ROOT) { w = gdk_screen_width (); h = gdk_screen_height (); }
    else if (d->widget) {
      QWidget *qw = d->widget->priv ? qobject_cast<QWidget *> (d->widget->priv->obj.data ()) : nullptr;
      if (qw) { w = qw->width (); h = qw->height (); }
    }
  }
  if (width) *width = w;
  if (height) *height = h;
}

gint gdk_drawable_get_depth (GdkDrawable *d) { return d ? d->depth : 24; }

void gdk_window_get_geometry (GdkWindow *window, gint *x, gint *y, gint *width, gint *height, gint *depth)
{
  if (x) *x = 0;
  if (y) *y = 0;
  QWidget *qw = window ? (window->qw ? window->qw : (window->widget ? gtkqt_qw (window->widget) : nullptr)) : nullptr;
  if (qw && !qw->isWindow ()) {
    if (x) *x = qw->x ();
    if (y) *y = qw->y ();
  }
  gdk_drawable_get_size (window, width, height);
  if (depth) *depth = 24;
}

void gdk_window_get_position (GdkWindow *window, gint *x, gint *y) { gdk_window_get_geometry (window, x, y, nullptr, nullptr, nullptr); }

gint gdk_window_get_origin (GdkWindow *window, gint *x, gint *y)
{
  QWidget *qw = window ? (window->qw ? window->qw : (window->widget ? gtkqt_qw (window->widget) : nullptr)) : nullptr;
  QPoint p = qw ? qw->mapToGlobal (QPoint (0, 0)) : QPoint ();
  if (x) *x = p.x ();
  if (y) *y = p.y ();
  return qw != nullptr;
}

GdkWindow *gdk_window_get_pointer (GdkWindow *window, gint *x, gint *y, GdkModifierType *mask)
{
  QWidget *qw = window ? (window->qw ? window->qw : (window->widget ? gtkqt_qw (window->widget) : nullptr)) : nullptr;
  QPoint p = qw ? qw->mapFromGlobal (QCursor::pos ()) : QCursor::pos ();
  if (x) *x = p.x ();
  if (y) *y = p.y ();
  if (mask) *mask = (GdkModifierType) gtkqt_qt_mods_to_gdk (QGuiApplication::queryKeyboardModifiers (), QGuiApplication::mouseButtons ());
  return window;
}

void gdk_window_get_user_data (GdkWindow *window, gpointer *data) { if (data) *data = window ? window->widget : nullptr; }

static QWidget *drawable_qwidget (GdkWindow *window)
{
  if (!window) return nullptr;
  if (window->qw) return window->qw;
  return window->widget ? gtkqt_qw (window->widget) : nullptr;
}

void gdk_window_clear (GdkWindow *window) { if (QWidget *qw = drawable_qwidget (window)) qw->update (); }
void gdk_window_clear_area (GdkWindow *window, gint x, gint y, gint width, gint height)
{
  if (QWidget *qw = drawable_qwidget (window)) qw->update (x, y, width ? width : qw->width (), height ? height : qw->height ());
}
void gdk_window_clear_area_e (GdkWindow *window, gint x, gint y, gint width, gint height) { gdk_window_clear_area (window, x, y, width, height); }
void gdk_window_set_background (GdkWindow *window, const GdkColor *color)
{
  if (!window || !color) return;
  window->background = *color;
  window->has_background = true;
}
void gdk_window_set_back_pixmap (GdkWindow *, GdkPixmap *, gboolean) {}
void gdk_window_process_updates (GdkWindow *window, gboolean) { if (QWidget *qw = drawable_qwidget (window)) qw->repaint (); }
void gdk_window_process_all_updates (void) { QCoreApplication::processEvents (QEventLoop::ExcludeUserInputEvents); }
void gdk_window_raise (GdkWindow *window) { if (QWidget *qw = drawable_qwidget (window)) qw->window ()->raise (); }
void gdk_window_lower (GdkWindow *window) { if (QWidget *qw = drawable_qwidget (window)) qw->window ()->lower (); }

GdkWindow *gdk_get_default_root_window (void)
{
  static GdkWindow *root = gtkqt_drawable_new (GTKQT_DRAWABLE_ROOT, nullptr);
  return root;
}

GdkPixmap *gdk_pixmap_new (GdkDrawable *, gint width, gint height, gint depth)
{
  GdkWindow *p = gtkqt_drawable_new (GTKQT_DRAWABLE_PIXMAP, nullptr);
  p->width = qMax (1, width);
  p->height = qMax (1, height);
  p->depth = depth < 0 ? 24 : depth;
  p->surface = cairo_image_surface_create (p->depth == 1 ? CAIRO_FORMAT_A8 : CAIRO_FORMAT_RGB24, p->width, p->height);
  return p;
}
GdkPixmap *gdk_pixmap_ref (GdkPixmap *pixmap) { return pixmap ? (GdkPixmap *) g_object_ref (pixmap) : nullptr; }
void gdk_pixmap_unref (GdkPixmap *pixmap) { if (pixmap) g_object_unref (pixmap); }

void gdk_cairo_set_source_pixmap (cairo_t *cr, GdkPixmap *pixmap, double x, double y)
{
  if (pixmap && pixmap->surface) cairo_set_source_surface (cr, pixmap->surface, x, y);
}

/* ======================================================================
 * GdkGC
 * ====================================================================== */
struct _GdkGC {
  GObject parent_instance;
  GdkGCValues v;
  GdkRectangle clip;
  bool has_clip;
  QVector<double> dashes;
  double dash_offset;
};
typedef struct { GObjectClass parent_class; } GdkGCClass;
G_DEFINE_TYPE (GdkGC, gdk_gc, G_TYPE_OBJECT)
static void gdk_gc_init (GdkGC *gc)
{
  new (&gc->dashes) QVector<double> ();
  gc->v.function = GDK_COPY;
  gc->v.line_width = 0;
  gc->v.cap_style = GDK_CAP_BUTT;
  gc->v.join_style = GDK_JOIN_MITER;
  gc->v.background.red = gc->v.background.green = gc->v.background.blue = 0xffff;
  gc->v.background.pixel = 0xffffff;
  gc->v.graphics_exposures = TRUE;
}
static void gc_finalize (GObject *o)
{
  auto *gc = (GdkGC *) o;
  gc->dashes.~QVector<double> ();
  if (gc->v.tile) g_object_unref (gc->v.tile);
  if (gc->v.stipple) g_object_unref (gc->v.stipple);
  if (gc->v.clip_mask) g_object_unref (gc->v.clip_mask);
  G_OBJECT_CLASS (gdk_gc_parent_class)->finalize (o);
}
static void gdk_gc_class_init (GdkGCClass *klass) { G_OBJECT_CLASS (klass)->finalize = gc_finalize; }

static void set_pixmap_ref (GdkPixmap **slot, GdkPixmap *p)
{
  if (p) g_object_ref (p);
  if (*slot) g_object_unref (*slot);
  *slot = p;
}

GdkGC *gdk_gc_new (GdkDrawable *) { return (GdkGC *) g_object_new (gdk_gc_get_type (), nullptr); }

void gdk_gc_set_values (GdkGC *gc, GdkGCValues *values, GdkGCValuesMask mask)
{
  if (!gc || !values) return;
  if (mask & GDK_GC_FOREGROUND) gc->v.foreground = values->foreground;
  if (mask & GDK_GC_BACKGROUND) gc->v.background = values->background;
  if (mask & GDK_GC_FONT) gc->v.font = values->font;
  if (mask & GDK_GC_FUNCTION) gc->v.function = values->function;
  if (mask & GDK_GC_FILL) gc->v.fill = values->fill;
  if (mask & GDK_GC_TILE) set_pixmap_ref (&gc->v.tile, values->tile);
  if (mask & GDK_GC_STIPPLE) set_pixmap_ref (&gc->v.stipple, values->stipple);
  if (mask & GDK_GC_CLIP_MASK) set_pixmap_ref (&gc->v.clip_mask, values->clip_mask);
  if (mask & GDK_GC_SUBWINDOW) gc->v.subwindow_mode = values->subwindow_mode;
  if (mask & GDK_GC_TS_X_ORIGIN) gc->v.ts_x_origin = values->ts_x_origin;
  if (mask & GDK_GC_TS_Y_ORIGIN) gc->v.ts_y_origin = values->ts_y_origin;
  if (mask & GDK_GC_CLIP_X_ORIGIN) gc->v.clip_x_origin = values->clip_x_origin;
  if (mask & GDK_GC_CLIP_Y_ORIGIN) gc->v.clip_y_origin = values->clip_y_origin;
  if (mask & GDK_GC_EXPOSURES) gc->v.graphics_exposures = values->graphics_exposures;
  if (mask & GDK_GC_LINE_WIDTH) gc->v.line_width = values->line_width;
  if (mask & GDK_GC_LINE_STYLE) gc->v.line_style = values->line_style;
  if (mask & GDK_GC_CAP_STYLE) gc->v.cap_style = values->cap_style;
  if (mask & GDK_GC_JOIN_STYLE) gc->v.join_style = values->join_style;
}

GdkGC *gdk_gc_new_with_values (GdkDrawable *d, GdkGCValues *values, GdkGCValuesMask mask)
{
  GdkGC *gc = gdk_gc_new (d);
  gdk_gc_set_values (gc, values, mask);
  return gc;
}

void gdk_gc_get_values (GdkGC *gc, GdkGCValues *values) { if (gc && values) *values = gc->v; }
void gdk_gc_set_foreground (GdkGC *gc, const GdkColor *c) { if (gc && c) gc->v.foreground = *c; }
void gdk_gc_set_background (GdkGC *gc, const GdkColor *c) { if (gc && c) gc->v.background = *c; }
void gdk_gc_set_rgb_fg_color (GdkGC *gc, const GdkColor *c) { gdk_gc_set_foreground (gc, c); }
void gdk_gc_set_rgb_bg_color (GdkGC *gc, const GdkColor *c) { gdk_gc_set_background (gc, c); }
void gdk_gc_set_font (GdkGC *gc, GdkFont *font) { if (gc) gc->v.font = font; }
void gdk_gc_set_function (GdkGC *gc, GdkFunction f) { if (gc) gc->v.function = f; }
void gdk_gc_set_fill (GdkGC *gc, GdkFill fill) { if (gc) gc->v.fill = fill; }
void gdk_gc_set_tile (GdkGC *gc, GdkPixmap *tile) { if (gc) set_pixmap_ref (&gc->v.tile, tile); }
void gdk_gc_set_stipple (GdkGC *gc, GdkPixmap *stipple) { if (gc) set_pixmap_ref (&gc->v.stipple, stipple); }
void gdk_gc_set_ts_origin (GdkGC *gc, gint x, gint y) { if (gc) { gc->v.ts_x_origin = x; gc->v.ts_y_origin = y; } }
void gdk_gc_set_clip_origin (GdkGC *gc, gint x, gint y) { if (gc) { gc->v.clip_x_origin = x; gc->v.clip_y_origin = y; } }
void gdk_gc_set_clip_mask (GdkGC *gc, GdkBitmap *mask) { if (gc) set_pixmap_ref (&gc->v.clip_mask, mask); }
void gdk_gc_set_clip_rectangle (GdkGC *gc, const GdkRectangle *r)
{
  if (!gc) return;
  gc->has_clip = r != nullptr;
  if (r) gc->clip = *r;
}
void gdk_gc_set_subwindow (GdkGC *gc, GdkSubwindowMode mode) { if (gc) gc->v.subwindow_mode = mode; }
void gdk_gc_set_exposures (GdkGC *gc, gboolean e) { if (gc) gc->v.graphics_exposures = e; }
void gdk_gc_set_line_attributes (GdkGC *gc, gint line_width, GdkLineStyle line_style, GdkCapStyle cap_style, GdkJoinStyle join_style)
{
  if (!gc) return;
  gc->v.line_width = line_width;
  gc->v.line_style = line_style;
  gc->v.cap_style = cap_style;
  gc->v.join_style = join_style;
}
void gdk_gc_set_dashes (GdkGC *gc, gint dash_offset, gint8 dash_list[], gint n)
{
  if (!gc) return;
  gc->dashes.clear ();
  for (int i = 0; i < n; i++) gc->dashes << (double) (guint8) dash_list[i];
  gc->dash_offset = dash_offset;
}
void gdk_gc_copy (GdkGC *dst, GdkGC *src)
{
  if (!dst || !src) return;
  gdk_gc_set_values (dst, &src->v, (GdkGCValuesMask) 0x3ffff);
  dst->clip = src->clip;
  dst->has_clip = src->has_clip;
  dst->dashes = src->dashes;
  dst->dash_offset = src->dash_offset;
}
GdkGC *gdk_gc_ref (GdkGC *gc) { return gc ? (GdkGC *) g_object_ref (gc) : nullptr; }
void gdk_gc_unref (GdkGC *gc) { if (gc) g_object_unref (gc); }

} /* extern "C" */

/* A cairo context on the drawable, set up from the GC (or nullptr when the
 * drawable cannot be drawn on right now). */
static cairo_t *gc_cairo (GdkDrawable *d, GdkGC *gc, bool for_lines)
{
  cairo_surface_t *target = gtkqt_drawable_target (d);
  if (!target) return nullptr;
  cairo_t *cr = cairo_create (target);
  if (!gc) return cr;
  if (gc->has_clip) {
    cairo_rectangle (cr, gc->clip.x + gc->v.clip_x_origin, gc->clip.y + gc->v.clip_y_origin, gc->clip.width, gc->clip.height);
    cairo_clip (cr);
  }
  const GdkColor &c = gc->v.foreground;
  cairo_set_source_rgb (cr, c.red / 65535.0, c.green / 65535.0, c.blue / 65535.0);
  switch (gc->v.function) {
  case GDK_XOR: case GDK_INVERT: case GDK_EQUIV:
    if (gc->v.function == GDK_INVERT) cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
    break;
  case GDK_CLEAR: cairo_set_source_rgb (cr, 0, 0, 0); break;
  case GDK_SET: cairo_set_source_rgb (cr, 1, 1, 1); break;
  case GDK_NOOP: cairo_set_operator (cr, CAIRO_OPERATOR_DEST); break;
  default: break;
  }
  if (gc->v.fill == GDK_TILED && gc->v.tile && gc->v.tile->surface) {
    cairo_set_source_surface (cr, gc->v.tile->surface, gc->v.ts_x_origin, gc->v.ts_y_origin);
    cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
  }
  if (for_lines) {
    cairo_set_line_width (cr, gc->v.line_width > 0 ? gc->v.line_width : 1);
    cairo_set_line_cap (cr, gc->v.cap_style == GDK_CAP_ROUND ? CAIRO_LINE_CAP_ROUND
                          : gc->v.cap_style == GDK_CAP_PROJECTING ? CAIRO_LINE_CAP_SQUARE : CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join (cr, gc->v.join_style == GDK_JOIN_ROUND ? CAIRO_LINE_JOIN_ROUND
                           : gc->v.join_style == GDK_JOIN_BEVEL ? CAIRO_LINE_JOIN_BEVEL : CAIRO_LINE_JOIN_MITER);
    if (gc->v.line_style != GDK_LINE_SOLID) {
      static const double dflt[] = { 4, 4 };
      if (gc->dashes.isEmpty ()) cairo_set_dash (cr, dflt, 2, 0);
      else cairo_set_dash (cr, gc->dashes.constData (), gc->dashes.size (), gc->dash_offset);
    }
  }
  cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
  return cr;
}

/* X pixel convention: a 1-pixel line through pixel centres */
static double lo (GdkGC *gc) { int w = gc && gc->v.line_width > 0 ? gc->v.line_width : 1; return (w % 2) ? 0.5 : 0.0; }

extern "C" {

void gdk_draw_point (GdkDrawable *d, GdkGC *gc, gint x, gint y)
{
  if (cairo_t *cr = gc_cairo (d, gc, false)) { cairo_rectangle (cr, x, y, 1, 1); cairo_fill (cr); cairo_destroy (cr); }
}

void gdk_draw_points (GdkDrawable *d, GdkGC *gc, const GdkPoint *pts, gint n)
{
  if (cairo_t *cr = gc_cairo (d, gc, false)) {
    for (int i = 0; i < n; i++) cairo_rectangle (cr, pts[i].x, pts[i].y, 1, 1);
    cairo_fill (cr);
    cairo_destroy (cr);
  }
}

void gdk_draw_line (GdkDrawable *d, GdkGC *gc, gint x1, gint y1, gint x2, gint y2)
{
  if (cairo_t *cr = gc_cairo (d, gc, true)) {
    double o = lo (gc);
    if (x1 == x2 && y1 == y2) { cairo_rectangle (cr, x1, y1, 1, 1); cairo_fill (cr); }
    else {
      cairo_move_to (cr, x1 + o, y1 + o);
      cairo_line_to (cr, x2 + o, y2 + o);
      cairo_stroke (cr);
    }
    cairo_destroy (cr);
  }
}

void gdk_draw_lines (GdkDrawable *d, GdkGC *gc, const GdkPoint *pts, gint n)
{
  if (n < 2) return;
  if (cairo_t *cr = gc_cairo (d, gc, true)) {
    double o = lo (gc);
    cairo_move_to (cr, pts[0].x + o, pts[0].y + o);
    for (int i = 1; i < n; i++) cairo_line_to (cr, pts[i].x + o, pts[i].y + o);
    cairo_stroke (cr);
    cairo_destroy (cr);
  }
}

void gdk_draw_segments (GdkDrawable *d, GdkGC *gc, const GdkSegment *segs, gint n)
{
  for (int i = 0; i < n; i++) gdk_draw_line (d, gc, segs[i].x1, segs[i].y1, segs[i].x2, segs[i].y2);
}

void gdk_draw_rectangle (GdkDrawable *d, GdkGC *gc, gboolean filled, gint x, gint y, gint width, gint height)
{
  int dw, dh;
  if (width < 0 || height < 0) {
    gdk_drawable_get_size (d, &dw, &dh);
    if (width < 0) width = dw;
    if (height < 0) height = dh;
  }
  if (cairo_t *cr = gc_cairo (d, gc, !filled)) {
    if (filled) { cairo_rectangle (cr, x, y, width, height); cairo_fill (cr); }
    else { double o = lo (gc); cairo_rectangle (cr, x + o, y + o, width, height); cairo_stroke (cr); }
    cairo_destroy (cr);
  }
}

void gdk_draw_arc (GdkDrawable *d, GdkGC *gc, gboolean filled, gint x, gint y, gint width, gint height, gint angle1, gint angle2)
{
  if (width <= 0 || height <= 0) return;
  if (cairo_t *cr = gc_cairo (d, gc, !filled)) {
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
    double o = filled ? 0 : lo (gc);
    double a1 = -angle1 / 64.0 * M_PI / 180.0, a2 = -(angle1 + angle2) / 64.0 * M_PI / 180.0;
    cairo_save (cr);
    cairo_translate (cr, x + o + width / 2.0, y + o + height / 2.0);
    cairo_scale (cr, width / 2.0, height / 2.0);
    if (filled) cairo_move_to (cr, 0, 0);
    if (angle2 >= 0) cairo_arc_negative (cr, 0, 0, 1, a1, a2);
    else cairo_arc (cr, 0, 0, 1, a1, a2);
    if (filled) cairo_close_path (cr);
    cairo_restore (cr);
    if (filled) cairo_fill (cr); else cairo_stroke (cr);
    cairo_destroy (cr);
  }
}

void gdk_draw_polygon (GdkDrawable *d, GdkGC *gc, gboolean filled, const GdkPoint *pts, gint n)
{
  if (n < 2) return;
  if (cairo_t *cr = gc_cairo (d, gc, !filled)) {
    double o = filled ? 0 : lo (gc);
    cairo_move_to (cr, pts[0].x + o, pts[0].y + o);
    for (int i = 1; i < n; i++) cairo_line_to (cr, pts[i].x + o, pts[i].y + o);
    cairo_close_path (cr);
    if (filled) cairo_fill (cr); else cairo_stroke (cr);
    cairo_destroy (cr);
  }
}

void gdk_draw_drawable (GdkDrawable *d, GdkGC *gc, GdkDrawable *src, gint xsrc, gint ysrc, gint xdest, gint ydest, gint width, gint height)
{
  cairo_surface_t *s = gtkqt_drawable_target (src);
  if (!s) return;
  int sw, sh;
  gdk_drawable_get_size (src, &sw, &sh);
  if (width < 0) width = sw - xsrc;
  if (height < 0) height = sh - ysrc;
  if (cairo_t *cr = gc_cairo (d, gc, false)) {
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface (cr, s, xdest - xsrc, ydest - ysrc);
    cairo_rectangle (cr, xdest, ydest, width, height);
    cairo_fill (cr);
    cairo_destroy (cr);
  }
}

void gdk_draw_pixbuf (GdkDrawable *d, GdkGC *gc, const GdkPixbuf *pixbuf, gint src_x, gint src_y, gint dest_x, gint dest_y,
                      gint width, gint height, gint, gint, gint)
{
  if (!pixbuf) return;
  if (width < 0) width = gdk_pixbuf_get_width (pixbuf) - src_x;
  if (height < 0) height = gdk_pixbuf_get_height (pixbuf) - src_y;
  if (cairo_t *cr = gc_cairo (d, gc, false)) {
    gdk_cairo_set_source_pixbuf (cr, pixbuf, dest_x - src_x, dest_y - src_y);
    cairo_rectangle (cr, dest_x, dest_y, width, height);
    cairo_fill (cr);
    cairo_destroy (cr);
  }
}

void gdk_draw_layout_with_colors (GdkDrawable *d, GdkGC *gc, gint x, gint y, PangoLayout *layout,
                                  const GdkColor *fg, const GdkColor *bg)
{
  if (cairo_t *cr = gc_cairo (d, gc, false)) {
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
    if (bg) {
      int w, h;
      pango_layout_get_pixel_size (layout, &w, &h);
      cairo_set_source_rgb (cr, bg->red / 65535.0, bg->green / 65535.0, bg->blue / 65535.0);
      cairo_rectangle (cr, x, y, w, h);
      cairo_fill (cr);
    }
    if (fg) cairo_set_source_rgb (cr, fg->red / 65535.0, fg->green / 65535.0, fg->blue / 65535.0);
    else if (gc) cairo_set_source_rgb (cr, gc->v.foreground.red / 65535.0, gc->v.foreground.green / 65535.0, gc->v.foreground.blue / 65535.0);
    cairo_move_to (cr, x, y);
    pango_cairo_show_layout (cr, layout);
    cairo_destroy (cr);
  }
}

void gdk_draw_layout (GdkDrawable *d, GdkGC *gc, gint x, gint y, PangoLayout *layout)
{
  gdk_draw_layout_with_colors (d, gc, x, y, layout, nullptr, nullptr);
}

void gdk_draw_layout_line (GdkDrawable *d, GdkGC *gc, gint x, gint y, PangoLayoutLine *line)
{
  if (cairo_t *cr = gc_cairo (d, gc, false)) {
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_move_to (cr, x, y);
    pango_cairo_show_layout_line (cr, line);
    cairo_destroy (cr);
  }
}

/* ======================================================================
 * Atoms
 * ====================================================================== */
static QList<QByteArray> &atom_names ()
{
  static QList<QByteArray> names = [] {
    QList<QByteArray> n (70);
    n[1] = "PRIMARY"; n[2] = "SECONDARY"; n[4] = "ATOM"; n[19] = "INTEGER"; n[31] = "STRING";
    n[69] = "CLIPBOARD";
    return n;
  } ();
  return names;
}

GdkAtom gdk_atom_intern (const gchar *name, gboolean)
{
  if (!name) return GDK_NONE;
  QList<QByteArray> &n = atom_names ();
  int i = n.indexOf (QByteArray (name));
  if (i <= 0) { n << QByteArray (name); i = n.size () - 1; }
  return _GDK_MAKE_ATOM (i);
}
GdkAtom gdk_atom_intern_static_string (const gchar *name) { return gdk_atom_intern (name, FALSE); }
gchar *gdk_atom_name (GdkAtom atom)
{
  int i = GPOINTER_TO_INT (atom);
  QList<QByteArray> &n = atom_names ();
  return g_strdup (i > 0 && i < n.size () ? n[i].constData () : "NONE");
}

/* ======================================================================
 * Colormaps and visuals (TrueColor)
 * ====================================================================== */
struct _GdkColormap { int dummy; };
struct _GdkVisual { int dummy; };
static GdkColormap system_colormap;
static GdkVisual system_visual;

GdkColormap *gdk_colormap_get_system (void) { return &system_colormap; }
GdkVisual *gdk_visual_get_system (void) { return &system_visual; }
GdkColormap *gdk_colormap_new (GdkVisual *, gboolean) { return &system_colormap; }
GdkColormap *gdk_colormap_ref (GdkColormap *cmap) { return cmap; }
void gdk_colormap_unref (GdkColormap *) {}
GdkColormap *gdk_drawable_get_colormap (GdkDrawable *) { return &system_colormap; }

gboolean gdk_colormap_alloc_color (GdkColormap *, GdkColor *c, gboolean, gboolean)
{
  if (!c) return FALSE;
  c->pixel = ((c->red >> 8) << 16) | ((c->green >> 8) << 8) | (c->blue >> 8);
  return TRUE;
}
gint gdk_colormap_alloc_colors (GdkColormap *cm, GdkColor *colors, gint n, gboolean w, gboolean b, gboolean *success)
{
  for (int i = 0; i < n; i++) {
    gdk_colormap_alloc_color (cm, &colors[i], w, b);
    if (success) success[i] = TRUE;
  }
  return 0;
}
void gdk_colormap_free_colors (GdkColormap *, const GdkColor *, gint) {}
void gdk_colormap_query_color (GdkColormap *, gulong pixel, GdkColor *r)
{
  if (!r) return;
  r->pixel = pixel;
  r->red = ((pixel >> 16) & 0xff) * 257;
  r->green = ((pixel >> 8) & 0xff) * 257;
  r->blue = (pixel & 0xff) * 257;
}
gboolean gdk_color_alloc (GdkColormap *cm, GdkColor *c) { return gdk_colormap_alloc_color (cm, c, FALSE, TRUE); }
gboolean gdk_color_white (GdkColormap *cm, GdkColor *c) { if (!c) return FALSE; c->red = c->green = c->blue = 0xffff; return gdk_color_alloc (cm, c); }
gboolean gdk_color_black (GdkColormap *cm, GdkColor *c) { if (!c) return FALSE; c->red = c->green = c->blue = 0; return gdk_color_alloc (cm, c); }
GdkColor *gdk_color_copy (const GdkColor *c) { return c ? (GdkColor *) g_memdup2 (c, sizeof (GdkColor)) : nullptr; }
void gdk_color_free (GdkColor *c) { g_free (c); }
gboolean gdk_color_equal (const GdkColor *a, const GdkColor *b) { return a->red == b->red && a->green == b->green && a->blue == b->blue; }
gchar *gdk_color_to_string (const GdkColor *c) { return g_strdup_printf ("#%04x%04x%04x", c->red, c->green, c->blue); }
GType gdk_color_get_type (void)
{
  static GType t = 0;
  if (!t) t = g_boxed_type_register_static ("GdkColor", (GBoxedCopyFunc) gdk_color_copy, (GBoxedFreeFunc) gdk_color_free);
  return t;
}

} /* extern "C" */

/* ======================================================================
 * Pango <-> QFont, Pango contexts for widgets
 * ====================================================================== */
QFont gtkqt_font_from_pango (const PangoFontDescription *desc)
{
  QFont f = QApplication::font ();
  if (!desc) return f;
  if (const char *fam = pango_font_description_get_family (desc)) {
    QStringList fams;
    for (const QString &s : QString::fromUtf8 (fam).split (',', Qt::SkipEmptyParts)) {
      QString t = s.trimmed ();
      fams << (t.compare ("Monospace", Qt::CaseInsensitive) == 0 ? QFontDatabase::systemFont (QFontDatabase::FixedFont).family ()
               : t.compare ("Sans", Qt::CaseInsensitive) == 0 ? QFontDatabase::systemFont (QFontDatabase::GeneralFont).family ()
               : t);
    }
    if (!fams.isEmpty ()) f.setFamilies (fams);
    if (fams.size () == 1 && QString::fromUtf8 (fam).trimmed ().compare ("Monospace", Qt::CaseInsensitive) == 0)
      f.setStyleHint (QFont::Monospace);
  }
  PangoFontMask set = pango_font_description_get_set_fields (desc);
  if (set & PANGO_FONT_MASK_SIZE) {
    double size = pango_font_description_get_size (desc) / (double) PANGO_SCALE;
    if (size > 0) {
      if (pango_font_description_get_size_is_absolute (desc)) f.setPixelSize (qMax (1, (int) std::lround (size)));
      else f.setPointSizeF (size);
    }
  }
  if (set & PANGO_FONT_MASK_WEIGHT) f.setWeight ((QFont::Weight) qBound (100, (int) pango_font_description_get_weight (desc), 900));
  if (set & PANGO_FONT_MASK_STYLE) {
    PangoStyle st = pango_font_description_get_style (desc);
    f.setStyle (st == PANGO_STYLE_ITALIC ? QFont::StyleItalic : st == PANGO_STYLE_OBLIQUE ? QFont::StyleOblique : QFont::StyleNormal);
  }
  return f;
}

PangoFontDescription *gtkqt_pango_from_font (const QFont &f)
{
  PangoFontDescription *d = pango_font_description_new ();
  pango_font_description_set_family (d, f.families ().join (',').toUtf8 ().constData ());
  if (f.pointSizeF () > 0) pango_font_description_set_size (d, (gint) std::lround (f.pointSizeF () * PANGO_SCALE));
  else if (f.pixelSize () > 0) pango_font_description_set_absolute_size (d, f.pixelSize () * PANGO_SCALE);
  pango_font_description_set_weight (d, (PangoWeight) qBound (100, (int) f.weight (), 1000));
  pango_font_description_set_style (d, f.style () == QFont::StyleItalic ? PANGO_STYLE_ITALIC
                                       : f.style () == QFont::StyleOblique ? PANGO_STYLE_OBLIQUE : PANGO_STYLE_NORMAL);
  return d;
}

static double screen_dpi ()
{
  gtkqt_ensure_app ();
  QScreen *s = QGuiApplication::primaryScreen ();
  return s ? s->logicalDotsPerInchY () : 96.0;
}

static PangoContext *new_pango_context (const PangoFontDescription *desc)
{
  PangoContext *ctx = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  pango_cairo_context_set_resolution (ctx, screen_dpi ());
  if (desc) pango_context_set_font_description (ctx, desc);
  pango_context_set_language (ctx, pango_language_get_default ());
  return ctx;
}

/* ======================================================================
 * GtkStyle
 * ====================================================================== */
G_DEFINE_TYPE (GtkStyle, gtk_style, G_TYPE_OBJECT)
static void gtk_style_init (GtkStyle *) {}

static GdkColor gdk_from_q (const QColor &q)
{
  GdkColor c;
  c.red = q.red () * 257; c.green = q.green () * 257; c.blue = q.blue () * 257;
  c.pixel = (q.red () << 16) | (q.green () << 8) | q.blue ();
  return c;
}

static void style_make_gcs (GtkStyle *s)
{
  GdkColor *sets[] = { s->fg, s->bg, s->light, s->dark, s->mid, s->text, s->base, s->text_aa };
  GdkGC **gcs[] = { s->fg_gc, s->bg_gc, s->light_gc, s->dark_gc, s->mid_gc, s->text_gc, s->base_gc, s->text_aa_gc };
  for (int k = 0; k < 8; k++)
    for (int i = 0; i < 5; i++) {
      if (!gcs[k][i]) gcs[k][i] = gdk_gc_new (nullptr);
      gdk_gc_set_foreground (gcs[k][i], &sets[k][i]);
      gdk_gc_set_background (gcs[k][i], &s->bg[i]);
    }
  if (!s->black_gc) s->black_gc = gdk_gc_new (nullptr);
  if (!s->white_gc) s->white_gc = gdk_gc_new (nullptr);
  gdk_gc_set_foreground (s->black_gc, &s->black);
  gdk_gc_set_foreground (s->white_gc, &s->white);
}

static void style_fill (GtkStyle *s, const QPalette &p, const QFont &font)
{
  struct { QPalette::ColorGroup g; bool selected, prelight, active; } st[5] = {
    { QPalette::Active, false, false, false }, { QPalette::Active, false, false, true },
    { QPalette::Active, false, true, false }, { QPalette::Active, true, false, false },
    { QPalette::Disabled, false, false, false } };
  for (int i = 0; i < 5; i++) {
    QPalette::ColorGroup g = st[i].g;
    QColor bg = p.color (g, QPalette::Window), fg = p.color (g, QPalette::WindowText);
    QColor base = p.color (g, QPalette::Base), text = p.color (g, QPalette::Text);
    if (st[i].selected) { bg = base = p.color (QPalette::Active, QPalette::Highlight); fg = text = p.color (QPalette::Active, QPalette::HighlightedText); }
    if (st[i].active) { bg = p.color (QPalette::Active, QPalette::Button).darker (110); base = p.color (QPalette::Inactive, QPalette::Highlight); text = p.color (QPalette::Inactive, QPalette::HighlightedText); }
    if (st[i].prelight) bg = p.color (QPalette::Active, QPalette::Button).lighter (110);
    s->fg[i] = gdk_from_q (fg);
    s->bg[i] = gdk_from_q (bg);
    s->base[i] = gdk_from_q (base);
    s->text[i] = gdk_from_q (text);
    s->light[i] = gdk_from_q (p.color (g, QPalette::Light));
    s->dark[i] = gdk_from_q (p.color (g, QPalette::Dark));
    s->mid[i] = gdk_from_q (p.color (g, QPalette::Mid));
    QColor aa ((text.red () + base.red ()) / 2, (text.green () + base.green ()) / 2, (text.blue () + base.blue ()) / 2);
    s->text_aa[i] = gdk_from_q (aa);
  }
  s->black = gdk_from_q (Qt::black);
  s->white = gdk_from_q (Qt::white);
  if (s->font_desc) pango_font_description_free (s->font_desc);
  s->font_desc = gtkqt_pango_from_font (font);
  s->xthickness = s->ythickness = 2;
  style_make_gcs (s);
}

static void style_finalize (GObject *o)
{
  auto *s = GTK_STYLE (o);
  if (s->font_desc) pango_font_description_free (s->font_desc);
  GdkGC **gcs[] = { s->fg_gc, s->bg_gc, s->light_gc, s->dark_gc, s->mid_gc, s->text_gc, s->base_gc, s->text_aa_gc };
  for (auto &set : gcs)
    for (int i = 0; i < 5; i++) if (set[i]) g_object_unref (set[i]);
  if (s->black_gc) g_object_unref (s->black_gc);
  if (s->white_gc) g_object_unref (s->white_gc);
  for (int i = 0; i < 5; i++) if (s->bg_pixmap[i]) g_object_unref (s->bg_pixmap[i]);
  G_OBJECT_CLASS (gtk_style_parent_class)->finalize (o);
}
static void gtk_style_class_init (GtkStyleClass *klass) { G_OBJECT_CLASS (klass)->finalize = style_finalize; }

GtkStyle *gtkqt_default_style_ref (void)
{
  static GtkStyle *def = nullptr;
  if (!def) {
    gtkqt_ensure_app ();
    def = (GtkStyle *) g_object_new (gtk_style_get_type (), nullptr);
    style_fill (def, QApplication::palette (), QApplication::font ());
  }
  return (GtkStyle *) g_object_ref (def);
}

/* the widget's own style object (copied from the shared one on first change) */
static GtkStyle *own_style (GtkWidget *w)
{
  if (!w->style || G_OBJECT (w->style)->ref_count > 1 || !g_object_get_data (G_OBJECT (w), "gtkqt-own-style")) {
    GtkStyle *s = w->style ? gtk_style_copy (w->style) : gtkqt_default_style_ref ();
    if (w->style) g_object_unref (w->style);
    w->style = s;
    g_object_set_data (G_OBJECT (w), "gtkqt-own-style", GINT_TO_POINTER (1));
  }
  return w->style;
}

static void apply_style_color (GtkWidget *w, QPalette::ColorRole role, GtkStateType state, const GdkColor *c)
{
  QWidget *qw = gtkqt_qw (w);
  if (!qw) return;
  QPalette p = qw->palette ();
  QColor q = c ? QColor (c->red >> 8, c->green >> 8, c->blue >> 8) : QApplication::palette ().color (role);
  if (state == GTK_STATE_INSENSITIVE) p.setColor (QPalette::Disabled, role, q);
  else if (state == GTK_STATE_SELECTED) p.setColor (role == QPalette::Base || role == QPalette::Window ? QPalette::Highlight : QPalette::HighlightedText, q);
  else if (state == GTK_STATE_NORMAL) { p.setColor (QPalette::Active, role, q); p.setColor (QPalette::Inactive, role, q); }
  qw->setPalette (p);
  if (auto *a = qobject_cast<QAbstractScrollArea *> (qw)) a->viewport ()->setPalette (p);
}

extern "C" {

GtkStyle *gtk_style_new (void) { return gtkqt_default_style_ref (); }

GtkStyle *gtk_style_copy (GtkStyle *src)
{
  auto *s = (GtkStyle *) g_object_new (gtk_style_get_type (), nullptr);
  if (!src) { style_fill (s, QApplication::palette (), QApplication::font ()); return s; }
  memcpy (s->fg, src->fg, sizeof s->fg); memcpy (s->bg, src->bg, sizeof s->bg);
  memcpy (s->light, src->light, sizeof s->light); memcpy (s->dark, src->dark, sizeof s->dark);
  memcpy (s->mid, src->mid, sizeof s->mid); memcpy (s->text, src->text, sizeof s->text);
  memcpy (s->base, src->base, sizeof s->base); memcpy (s->text_aa, src->text_aa, sizeof s->text_aa);
  s->black = src->black; s->white = src->white;
  s->font_desc = src->font_desc ? pango_font_description_copy (src->font_desc) : nullptr;
  s->xthickness = src->xthickness; s->ythickness = src->ythickness;
  s->font = src->font;
  style_make_gcs (s);
  return s;
}

GtkStyle *gtk_style_ref (GtkStyle *style) { return style ? (GtkStyle *) g_object_ref (style) : nullptr; }
void gtk_style_unref (GtkStyle *style) { if (style) g_object_unref (style); }
GtkStyle *gtk_style_attach (GtkStyle *style, GdkWindow *) { return style; }
void gtk_style_detach (GtkStyle *) {}
void gtk_style_set_background (GtkStyle *style, GdkWindow *window, GtkStateType state)
{
  if (style && window) gdk_window_set_background (window, &style->bg[state]);
}

GtkStyle *gtk_widget_get_style (GtkWidget *widget)
{
  if (!widget->style) widget->style = gtkqt_default_style_ref ();
  return widget->style;
}
GtkStyle *gtk_widget_get_default_style (void)
{
  static GtkStyle *def = gtkqt_default_style_ref ();
  return def;
}
GtkStyle *gtk_rc_get_style (GtkWidget *widget) { return gtk_widget_get_style (widget); }
void gtk_widget_ensure_style (GtkWidget *) {}

void gtk_widget_set_style (GtkWidget *widget, GtkStyle *style)
{
  if (!style) style = gtkqt_default_style_ref (); else g_object_ref (style);
  if (widget->style) g_object_unref (widget->style);
  widget->style = style;
  g_object_set_data (G_OBJECT (widget), "gtkqt-own-style", nullptr);
  if (style->font_desc) gtk_widget_modify_font (widget, style->font_desc);
}

void gtk_widget_modify_font (GtkWidget *widget, PangoFontDescription *font_desc)
{
  QFont f = gtkqt_font_from_pango (font_desc);
  if (QWidget *qw = gtkqt_qw (widget)) {
    if (font_desc) qw->setFont (f); else qw->setFont (QFont ());
  }
  PangoFontDescription *nd = font_desc ? pango_font_description_copy (font_desc) : gtkqt_pango_from_font (QApplication::font ());
  if (font_desc && !(pango_font_description_get_set_fields (nd) & PANGO_FONT_MASK_SIZE)) {
    PangoFontDescription *base = gtkqt_pango_from_font (QApplication::font ());
    pango_font_description_merge (nd, base, FALSE);
    pango_font_description_free (base);
  }
  GtkStyle *s = own_style (widget);
  if (s->font_desc) pango_font_description_free (s->font_desc);
  s->font_desc = nd;
  if (auto *ctx = (PangoContext *) g_object_get_data (G_OBJECT (widget), "gtkqt-pango-context"))
    pango_context_set_font_description (ctx, nd);
  g_signal_emit_by_name (widget, "style-set", nullptr);
}

void gtk_widget_modify_fg (GtkWidget *w, GtkStateType state, const GdkColor *c)
{
  own_style (w)->fg[state] = c ? *c : gtk_widget_get_default_style ()->fg[state];
  style_make_gcs (w->style);
  apply_style_color (w, QPalette::WindowText, state, c);
}
void gtk_widget_modify_bg (GtkWidget *w, GtkStateType state, const GdkColor *c)
{
  own_style (w)->bg[state] = c ? *c : gtk_widget_get_default_style ()->bg[state];
  style_make_gcs (w->style);
  apply_style_color (w, QPalette::Window, state, c);
  if (QWidget *qw = gtkqt_qw (w); qw && c && state == GTK_STATE_NORMAL) qw->setAutoFillBackground (true);
}
void gtk_widget_modify_text (GtkWidget *w, GtkStateType state, const GdkColor *c)
{
  own_style (w)->text[state] = c ? *c : gtk_widget_get_default_style ()->text[state];
  style_make_gcs (w->style);
  apply_style_color (w, QPalette::Text, state, c);
}
void gtk_widget_modify_base (GtkWidget *w, GtkStateType state, const GdkColor *c)
{
  own_style (w)->base[state] = c ? *c : gtk_widget_get_default_style ()->base[state];
  style_make_gcs (w->style);
  apply_style_color (w, QPalette::Base, state, c);
}

GtkStateType gtk_widget_get_state (GtkWidget *widget)
{
  auto st = (GtkStateType) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "gtkqt-state"));
  if (st == GTK_STATE_NORMAL && !gtk_widget_is_sensitive (widget)) return GTK_STATE_INSENSITIVE;
  return st;
}
void gtk_widget_set_state (GtkWidget *widget, GtkStateType state)
{
  g_object_set_data (G_OBJECT (widget), "gtkqt-state", GINT_TO_POINTER (state));
  gtk_widget_queue_draw (widget);
}

PangoContext *gtk_widget_create_pango_context (GtkWidget *widget)
{
  return new_pango_context (gtk_widget_get_style (widget)->font_desc);
}

PangoContext *gtk_widget_get_pango_context (GtkWidget *widget)
{
  auto *ctx = (PangoContext *) g_object_get_data (G_OBJECT (widget), "gtkqt-pango-context");
  if (!ctx) {
    ctx = gtk_widget_create_pango_context (widget);
    g_object_set_data_full (G_OBJECT (widget), "gtkqt-pango-context", ctx, g_object_unref);
  }
  return ctx;
}

PangoLayout *gtk_widget_create_pango_layout (GtkWidget *widget, const gchar *text)
{
  PangoLayout *l = pango_layout_new (gtk_widget_get_pango_context (widget));
  if (text) pango_layout_set_text (l, text, -1);
  return l;
}

PangoContext *gdk_pango_context_get (void) { return new_pango_context (gtk_widget_get_default_style ()->font_desc); }
PangoLanguage *gtk_get_default_language (void) { return pango_language_get_default (); }

void gtk_paint_layout (GtkStyle *style, GdkWindow *window, GtkStateType state, gboolean use_text, const GdkRectangle *area,
                       GtkWidget *, const gchar *, gint x, gint y, PangoLayout *layout)
{
  cairo_surface_t *target = gtkqt_drawable_target (window);
  if (!target || !layout) return;
  cairo_t *cr = cairo_create (target);
  if (area) { gdk_cairo_rectangle (cr, area); cairo_clip (cr); }
  if (style) gdk_cairo_set_source_color (cr, use_text ? &style->text[state] : &style->fg[state]);
  cairo_move_to (cr, x, y);
  pango_cairo_show_layout (cr, layout);
  cairo_destroy (cr);
}

void gtk_paint_flat_box (GtkStyle *style, GdkWindow *window, GtkStateType state, GtkShadowType, const GdkRectangle *area,
                         GtkWidget *, const gchar *, gint x, gint y, gint width, gint height)
{
  cairo_surface_t *target = gtkqt_drawable_target (window);
  if (!target || !style) return;
  if (width < 0 || height < 0) { int w, h; gdk_drawable_get_size (window, &w, &h); if (width < 0) width = w; if (height < 0) height = h; }
  cairo_t *cr = cairo_create (target);
  if (area) { gdk_cairo_rectangle (cr, area); cairo_clip (cr); }
  gdk_cairo_set_source_color (cr, &style->bg[state]);
  cairo_rectangle (cr, x, y, width, height);
  cairo_fill (cr);
  cairo_destroy (cr);
}

void gtk_paint_box (GtkStyle *style, GdkWindow *window, GtkStateType state, GtkShadowType shadow, const GdkRectangle *area,
                    GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height)
{
  gtk_paint_flat_box (style, window, state, shadow, area, widget, detail, x, y, width, height);
  if (!style || shadow == GTK_SHADOW_NONE) return;
  if (width < 0 || height < 0) { int w, h; gdk_drawable_get_size (window, &w, &h); if (width < 0) width = w; if (height < 0) height = h; }
  bool in = shadow == GTK_SHADOW_IN || shadow == GTK_SHADOW_ETCHED_IN;
  gdk_draw_line (window, in ? style->dark_gc[state] : style->light_gc[state], x, y, x + width - 1, y);
  gdk_draw_line (window, in ? style->dark_gc[state] : style->light_gc[state], x, y, x, y + height - 1);
  gdk_draw_line (window, in ? style->light_gc[state] : style->dark_gc[state], x, y + height - 1, x + width - 1, y + height - 1);
  gdk_draw_line (window, in ? style->light_gc[state] : style->dark_gc[state], x + width - 1, y, x + width - 1, y + height - 1);
}

void gtk_paint_hline (GtkStyle *style, GdkWindow *window, GtkStateType state, const GdkRectangle *, GtkWidget *, const gchar *,
                      gint x1, gint x2, gint y)
{
  if (!style) return;
  gdk_draw_line (window, style->dark_gc[state], x1, y, x2, y);
  gdk_draw_line (window, style->light_gc[state], x1, y + 1, x2, y + 1);
}

void gtk_paint_vline (GtkStyle *style, GdkWindow *window, GtkStateType state, const GdkRectangle *, GtkWidget *, const gchar *,
                      gint y1_, gint y2_, gint x)
{
  if (!style) return;
  gdk_draw_line (window, style->dark_gc[state], x, y1_, x, y2_);
  gdk_draw_line (window, style->light_gc[state], x + 1, y1_, x + 1, y2_);
}

} /* extern "C" */

/* ======================================================================
 * GtkSettings
 * ====================================================================== */
struct _GtkSettings { GObject parent_instance; QHash<QByteArray, QVariant> *values; };
typedef struct { GObjectClass parent_class; } GtkSettingsClass;
G_DEFINE_TYPE (GtkSettings, gtk_settings, G_TYPE_OBJECT)
static void gtk_settings_init (GtkSettings *s) { s->values = new QHash<QByteArray, QVariant>; }
static void settings_finalize (GObject *o) { delete ((GtkSettings *) o)->values; G_OBJECT_CLASS (gtk_settings_parent_class)->finalize (o); }

static void settings_set_property (GObject *o, guint, const GValue *v, GParamSpec *pspec)
{
  QHash<QByteArray, QVariant> &h = *((GtkSettings *) o)->values;
  if (G_VALUE_HOLDS_STRING (v)) h.insert (pspec->name, QString::fromUtf8 (g_value_get_string (v)));
  else if (G_VALUE_HOLDS_INT (v)) h.insert (pspec->name, g_value_get_int (v));
  else if (G_VALUE_HOLDS_BOOLEAN (v)) h.insert (pspec->name, (bool) g_value_get_boolean (v));
  else if (G_VALUE_HOLDS_ENUM (v)) h.insert (pspec->name, g_value_get_enum (v));
}

static void settings_get_property (GObject *o, guint, GValue *v, GParamSpec *pspec)
{
  QHash<QByteArray, QVariant> &h = *((GtkSettings *) o)->values;
  QByteArray name = pspec->name;
  if (G_VALUE_HOLDS_STRING (v)) {
    if (h.contains (name)) { g_value_set_string (v, h.value (name).toString ().toUtf8 ().constData ()); return; }
    if (name == "gtk-font-name") {
      PangoFontDescription *d = gtkqt_pango_from_font (QApplication::font ());
      gchar *s = pango_font_description_to_string (d);
      g_value_take_string (v, s);
      pango_font_description_free (d);
      return;
    }
    if (name == "gtk-theme-name") { g_value_set_string (v, QApplication::style ()->name ().toUtf8 ().constData ()); return; }
    if (name == "gtk-icon-theme-name") { g_value_set_string (v, QIcon::themeName ().toUtf8 ().constData ()); return; }
    g_value_set_string (v, G_PARAM_SPEC_STRING (pspec)->default_value);
  } else if (G_VALUE_HOLDS_INT (v)) {
    if (h.contains (name)) { g_value_set_int (v, h.value (name).toInt ()); return; }
    if (name == "gtk-double-click-time") { g_value_set_int (v, QApplication::doubleClickInterval ()); return; }
    if (name == "gtk-cursor-blink-time") { g_value_set_int (v, QApplication::cursorFlashTime ()); return; }
    if (name == "gtk-dnd-drag-threshold") { g_value_set_int (v, QApplication::startDragDistance ()); return; }
    if (name == "gtk-xft-dpi") { g_value_set_int (v, (int) (screen_dpi () * PANGO_SCALE)); return; }
    if (name == "gtk-xft-antialias" || name == "gtk-xft-hinting") { g_value_set_int (v, 1); return; }
    g_value_set_int (v, G_PARAM_SPEC_INT (pspec)->default_value);
  } else if (G_VALUE_HOLDS_BOOLEAN (v)) {
    g_value_set_boolean (v, h.contains (name) ? h.value (name).toBool () : G_PARAM_SPEC_BOOLEAN (pspec)->default_value);
  }
}

static void gtk_settings_class_init (GtkSettingsClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  oc->finalize = settings_finalize;
  oc->set_property = settings_set_property;
  oc->get_property = settings_get_property;
  GParamFlags rw = (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  guint id = 1;
  static const char *strings[][2] = {
    { "gtk-key-theme-name", "Default" }, { "gtk-theme-name", "Qt" }, { "gtk-icon-theme-name", "hicolor" },
    { "gtk-font-name", "Sans 10" }, { "gtk-fallback-icon-theme", "hicolor" }, { "gtk-cursor-theme-name", "" },
    { "gtk-icon-sizes", "" }, { "gtk-color-scheme", "" }, { "gtk-im-module", "" }, { "gtk-menu-bar-accel", "F10" },
    { "gtk-decoration-layout", "menu:minimize,maximize,close" },
  };
  for (auto &s : strings) g_object_class_install_property (oc, id++, g_param_spec_string (s[0], nullptr, nullptr, s[1], rw));
  static const char *ints[] = { "gtk-double-click-time", "gtk-double-click-distance", "gtk-cursor-blink-time",
                                "gtk-dnd-drag-threshold", "gtk-toolbar-style", "gtk-toolbar-icon-size",
                                "gtk-xft-dpi", "gtk-xft-antialias", "gtk-xft-hinting", "gtk-cursor-theme-size" };
  for (auto &s : ints) g_object_class_install_property (oc, id++, g_param_spec_int (s, nullptr, nullptr, G_MININT, G_MAXINT, 0, rw));
  static const struct { const char *n; gboolean d; } bools[] = {
    { "gtk-cursor-blink", TRUE }, { "gtk-can-change-accels", FALSE }, { "gtk-menu-images", TRUE },
    { "gtk-button-images", TRUE }, { "gtk-enable-animations", TRUE }, { "gtk-show-input-method-menu", FALSE },
    { "gtk-show-unicode-menu", FALSE }, { "gtk-enable-mnemonics", TRUE }, { "gtk-enable-accels", TRUE },
    { "gtk-application-prefer-dark-theme", FALSE }, { "gtk-alternative-button-order", FALSE },
    { "gtk-keynav-cursor-only", FALSE }, { "gtk-error-bell", TRUE }, { "gtk-touchscreen-mode", FALSE } };
  for (auto &b : bools) g_object_class_install_property (oc, id++, g_param_spec_boolean (b.n, nullptr, nullptr, b.d, rw));
}

extern "C" {

GtkSettings *gtk_settings_get_default (void)
{
  static GtkSettings *s = (GtkSettings *) g_object_new (gtk_settings_get_type (), nullptr);
  return s;
}
GtkSettings *gtk_widget_get_settings (GtkWidget *) { return gtk_settings_get_default (); }
void gtk_settings_set_string_property (GtkSettings *s, const gchar *name, const gchar *v, const gchar *)
{
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (s), name)) g_object_set (s, name, v, nullptr);
}
void gtk_settings_set_long_property (GtkSettings *s, const gchar *name, glong v, const gchar *)
{
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (s), name)) g_object_set (s, name, (gint) v, nullptr);
}
void gtk_rc_parse_string (const gchar *) {}
void gtk_rc_parse (const gchar *) {}
void gtk_rc_add_default_file (const gchar *) {}

GOptionGroup *gtk_get_option_group (gboolean)
{
  return g_option_group_new ("gtk", "GTK+ Options", "Show GTK+ Options", nullptr, nullptr);
}

/* ======================================================================
 * Stock items, icon factories, icon theme, window icons
 * ====================================================================== */
void gtk_stock_add (const GtkStockItem *items, guint n)
{
  for (guint i = 0; i < n; i++)
    if (items[i].stock_id)
      gtkqt_user_stock_labels.insert (QString::fromUtf8 (items[i].stock_id), QByteArray (items[i].label ? items[i].label : ""));
}
void gtk_stock_add_static (const GtkStockItem *items, guint n) { gtk_stock_add (items, n); }
gboolean gtk_stock_lookup (const gchar *stock_id, GtkStockItem *item)
{
  QString label = gtkqt_stock_label (stock_id);
  if (label.isEmpty ()) return FALSE;
  if (item) {
    static QHash<QString, QByteArray> keep;
    QByteArray &l = keep[QString::fromUtf8 (stock_id)];
    l = label.replace ("&&", "\x01").replace ('&', '_').replace ("\x01", "&").toUtf8 ();
    item->stock_id = (gchar *) stock_id;
    item->label = l.data ();
    item->modifier = (GdkModifierType) 0;
    item->keyval = 0;
    item->translation_domain = nullptr;
  }
  return TRUE;
}

} /* extern "C" */

struct _GtkIconSet { int ref; QIcon icon; };
struct _GtkIconFactory { GObject parent_instance; QHash<QString, GtkIconSet *> *sets; };
typedef struct { GObjectClass parent_class; } GtkIconFactoryClass;
G_DEFINE_TYPE (GtkIconFactory, gtk_icon_factory, G_TYPE_OBJECT)
static void gtk_icon_factory_init (GtkIconFactory *f) { f->sets = new QHash<QString, GtkIconSet *>; }
static void icon_factory_finalize (GObject *o)
{
  auto *f = (GtkIconFactory *) o;
  for (GtkIconSet *s : *f->sets) gtk_icon_set_unref (s);
  delete f->sets;
  G_OBJECT_CLASS (gtk_icon_factory_parent_class)->finalize (o);
}
static void gtk_icon_factory_class_init (GtkIconFactoryClass *klass) { G_OBJECT_CLASS (klass)->finalize = icon_factory_finalize; }

extern "C" {

GtkIconFactory *gtk_icon_factory_new (void) { return (GtkIconFactory *) g_object_new (gtk_icon_factory_get_type (), nullptr); }
void gtk_icon_factory_add (GtkIconFactory *f, const gchar *stock_id, GtkIconSet *set)
{
  if (!f || !stock_id || !set) return;
  gtk_icon_set_ref (set);
  if (GtkIconSet *old = f->sets->take (QString::fromUtf8 (stock_id))) gtk_icon_set_unref (old);
  f->sets->insert (QString::fromUtf8 (stock_id), set);
}
GtkIconSet *gtk_icon_factory_lookup (GtkIconFactory *f, const gchar *stock_id) { return f ? f->sets->value (QString::fromUtf8 (stock_id)) : nullptr; }
void gtk_icon_factory_add_default (GtkIconFactory *f)
{
  if (!f) return;
  for (auto it = f->sets->begin (); it != f->sets->end (); ++it) gtkqt_user_icons.insert (it.key (), it.value ()->icon);
}
void gtk_icon_factory_remove_default (GtkIconFactory *f)
{
  if (!f) return;
  for (auto it = f->sets->begin (); it != f->sets->end (); ++it) gtkqt_user_icons.remove (it.key ());
}
GtkIconSet *gtk_icon_factory_lookup_default (const gchar *stock_id)
{
  /* GTK returns a borrowed set; keep one per id */
  static QHash<QString, GtkIconSet *> cache;
  QString id = QString::fromUtf8 (stock_id ? stock_id : "");
  if (!cache.contains (id)) {
    GtkIconSet *s = gtk_icon_set_new ();
    s->icon = gtkqt_icon (stock_id);
    cache.insert (id, s);
  }
  return cache.value (id);
}
GtkIconSet *gtk_icon_set_new (void) { auto *s = new GtkIconSet; s->ref = 1; return s; }
GtkIconSet *gtk_icon_set_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  GtkIconSet *s = gtk_icon_set_new ();
  if (pixbuf) s->icon = QIcon (QPixmap::fromImage (gtkqt_pixbuf_image (pixbuf)));
  return s;
}
GtkIconSet *gtk_icon_set_ref (GtkIconSet *s) { if (s) s->ref++; return s; }
void gtk_icon_set_unref (GtkIconSet *s) { if (s && --s->ref == 0) delete s; }

gboolean gtk_icon_size_lookup (GtkIconSize size, gint *width, gint *height)
{
  int px = gtkqt_icon_size_px (size);
  if (width) *width = px;
  if (height) *height = px;
  return TRUE;
}

struct _GtkIconTheme { int dummy; };
GtkIconTheme *gtk_icon_theme_get_default (void) { static GtkIconTheme t; return &t; }
gboolean gtk_icon_theme_has_icon (GtkIconTheme *, const gchar *icon_name) { gtkqt_ensure_app (); return QIcon::hasThemeIcon (gtkqt_str (icon_name)); }
GdkPixbuf *gtk_icon_theme_load_icon (GtkIconTheme *, const gchar *icon_name, gint size, GtkIconLookupFlags, GError **error)
{
  gtkqt_ensure_app ();
  QIcon icon = QIcon::fromTheme (gtkqt_str (icon_name));
  if (icon.isNull ()) {
    g_set_error (error, g_quark_from_static_string ("gtk-icon-theme-error-quark"), 0, "Icon '%s' not present in theme", icon_name);
    return nullptr;
  }
  return gtkqt_pixbuf_from_image (icon.pixmap (size, size).toImage ());
}

void gtk_window_set_icon (GtkWindow *window, GdkPixbuf *icon)
{
  if (QWidget *qw = gtkqt_qw (GTK_WIDGET (window)))
    qw->setWindowIcon (icon ? QIcon (QPixmap::fromImage (gtkqt_pixbuf_image (icon))) : QIcon ());
}
void gtk_window_set_default_icon (GdkPixbuf *icon)
{
  gtkqt_ensure_app ();
  if (icon) QApplication::setWindowIcon (QIcon (QPixmap::fromImage (gtkqt_pixbuf_image (icon))));
}

/* ======================================================================
 * Accelerators connected to closures
 * ====================================================================== */
void gtk_accel_group_connect (GtkAccelGroup *group, guint key, GdkModifierType mods, guint, GClosure *closure)
{
  if (!group || !closure) return;
  gtkqt_ensure_app ();
  gchar *name = gtk_accelerator_name (key, mods);
  QKeySequence seq = gtkqt_accel_to_keyseq (name);
  g_free (name);
  auto *act = new QAction (qApp);
  act->setShortcut (seq);
  act->setShortcutContext (Qt::WindowShortcut);
  g_closure_ref (closure);
  g_closure_sink (closure);
  if (!closure->marshal) g_closure_set_marshal (closure, g_cclosure_marshal_generic);
  act->setProperty ("gtkqt-closure", QVariant::fromValue ((void *) closure));
  QObject::connect (act, &QAction::triggered, act, [group, key, mods, closure] {
    GtkQtEmitGuard guard;
    GValue params[4] = { G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT };
    GValue ret = G_VALUE_INIT;
    g_value_init (&params[0], G_TYPE_OBJECT); g_value_set_object (&params[0], group);
    g_value_init (&params[1], G_TYPE_OBJECT);
    QWidget *win = QApplication::activeWindow ();
    g_value_set_object (&params[1], win ? (GObject *) gtkqt_from_qobject (win) : nullptr);
    g_value_init (&params[2], G_TYPE_UINT); g_value_set_uint (&params[2], key);
    g_value_init (&params[3], G_TYPE_UINT); g_value_set_uint (&params[3], mods);
    g_value_init (&ret, G_TYPE_BOOLEAN);
    g_closure_invoke (closure, &ret, 4, params, nullptr);
    for (GValue &v : params) g_value_unset (&v);
    g_value_unset (&ret);
  });
  QObject::connect (act, &QObject::destroyed, [closure] { g_closure_unref (closure); });
  g_object_weak_ref (G_OBJECT (group), [] (gpointer a, GObject *) { delete static_cast<QAction *> (a); }, act);
  gtkqt_accel_group_add_action (group, act);
}

gboolean gtk_accel_group_disconnect (GtkAccelGroup *, GClosure *closure)
{
  for (QAction *a : qApp->findChildren<QAction *> (QString (), Qt::FindDirectChildrenOnly))
    if (a->property ("gtkqt-closure").value<void *> () == (void *) closure) { a->setShortcut (QKeySequence ()); return TRUE; }
  return FALSE;
}

gboolean gtk_widget_remove_accelerator (GtkWidget *widget, GtkAccelGroup *, guint key, GdkModifierType mods)
{
  gchar *name = gtk_accelerator_name (key, mods);
  QKeySequence seq = gtkqt_accel_to_keyseq (name);
  g_free (name);
  QObject *o = gtkqt_obj (widget);
  QList<QAction *> acts;
  if (auto *a = qobject_cast<QAction *> (o)) acts << a;
  acts << o->findChildren<QAction *> (QString (), Qt::FindDirectChildrenOnly);
  for (QAction *a : acts) {
    QList<QKeySequence> all = a->shortcuts ();
    if (all.removeAll (seq)) { a->setShortcuts (all); return TRUE; }
  }
  return FALSE;
}

/* ======================================================================
 * GtkClipboard
 * ====================================================================== */
struct _GtkClipboard { QClipboard::Mode mode; };

GtkClipboard *gtk_clipboard_get (GdkAtom selection)
{
  static GtkClipboard clip { QClipboard::Clipboard }, primary { QClipboard::Selection };
  gtkqt_ensure_app ();
  return selection == GDK_SELECTION_PRIMARY ? &primary : &clip;
}
GtkClipboard *gtk_widget_get_clipboard (GtkWidget *, GdkAtom selection) { return gtk_clipboard_get (selection); }

static QClipboard::Mode clip_mode (GtkClipboard *c)
{
  QClipboard::Mode m = c ? c->mode : QClipboard::Clipboard;
  if (m == QClipboard::Selection && !QGuiApplication::clipboard ()->supportsSelection ()) m = QClipboard::Clipboard;
  return m;
}
void gtk_clipboard_set_text (GtkClipboard *c, const gchar *text, gint len)
{
  QGuiApplication::clipboard ()->setText (len < 0 ? gtkqt_str (text) : QString::fromUtf8 (text, len), clip_mode (c));
}
gchar *gtk_clipboard_wait_for_text (GtkClipboard *c)
{
  const QMimeData *m = QGuiApplication::clipboard ()->mimeData (clip_mode (c));
  if (!m || !m->hasText ()) return nullptr;
  return g_strdup (m->text ().toUtf8 ().constData ());
}
gboolean gtk_clipboard_wait_is_text_available (GtkClipboard *c)
{
  const QMimeData *m = QGuiApplication::clipboard ()->mimeData (clip_mode (c));
  return m && m->hasText ();
}
void gtk_clipboard_request_text (GtkClipboard *c, GtkClipboardTextReceivedFunc callback, gpointer data)
{
  gchar *t = gtk_clipboard_wait_for_text (c);
  if (callback) callback (c, t, data);
  g_free (t);
}
void gtk_clipboard_clear (GtkClipboard *c) { QGuiApplication::clipboard ()->clear (clip_mode (c)); }
void gtk_clipboard_store (GtkClipboard *) {}
void gtk_clipboard_set_can_store (GtkClipboard *, gconstpointer, gint) {}

/* ======================================================================
 * Selection data and drops
 * ====================================================================== */
static GtkSelectionData *selection_data_copy (const GtkSelectionData *d)
{
  auto *n = (GtkSelectionData *) g_memdup2 (d, sizeof *d);
  n->data = d->data ? (guchar *) g_memdup2 (d->data, d->length + 1) : nullptr;
  return n;
}
static void selection_data_free (GtkSelectionData *d) { if (d) { g_free (d->data); g_free (d); } }
GType gtk_selection_data_get_type (void)
{
  static GType t = 0;
  if (!t) t = g_boxed_type_register_static ("GtkSelectionData", (GBoxedCopyFunc) selection_data_copy, (GBoxedFreeFunc) selection_data_free);
  return t;
}

void gtk_selection_data_set (GtkSelectionData *d, GdkAtom type, gint format, const guchar *data, gint length)
{
  g_free (d->data);
  if (length < 0) { d->data = nullptr; d->length = -1; }
  else {
    d->data = (guchar *) g_malloc (length + 1);
    if (data) memcpy (d->data, data, length);
    d->data[length] = 0;
    d->length = length;
  }
  d->type = type;
  d->format = format;
}

gboolean gtk_selection_data_set_text (GtkSelectionData *d, const gchar *str, gint len)
{
  if (len < 0) len = strlen (str);
  gtk_selection_data_set (d, gdk_atom_intern ("UTF8_STRING", FALSE), 8, (const guchar *) str, len);
  return TRUE;
}

guchar *gtk_selection_data_get_text (const GtkSelectionData *d)
{
  if (!d || !d->data || d->length < 0) return nullptr;
  gchar *t = gdk_atom_name (d->type);
  bool text = !strcmp (t, "UTF8_STRING") || !strcmp (t, "STRING") || !strcmp (t, "TEXT") || !strcmp (t, "COMPOUND_TEXT")
              || g_str_has_prefix (t, "text/plain");
  g_free (t);
  return text ? (guchar *) g_strndup ((const gchar *) d->data, d->length) : nullptr;
}

gchar **gtk_selection_data_get_uris (const GtkSelectionData *d)
{
  if (!d || !d->data) return nullptr;
  gchar *t = gdk_atom_name (d->type);
  bool uris = !strcmp (t, "text/uri-list");
  g_free (t);
  return uris ? g_uri_list_extract_uris ((const gchar *) d->data) : nullptr;
}

const guchar *gtk_selection_data_get_data (const GtkSelectionData *d) { return d ? d->data : nullptr; }
gint gtk_selection_data_get_length (const GtkSelectionData *d) { return d ? d->length : -1; }
GdkAtom gtk_selection_data_get_target (const GtkSelectionData *d) { return d ? d->target : GDK_NONE; }

} /* extern "C" */

/* GdkDragContext */
typedef struct { GObjectClass parent_class; } GdkDragContextClass;
G_DEFINE_TYPE (GdkDragContext, gdk_drag_context, G_TYPE_OBJECT)
static void gdk_drag_context_init (GdkDragContext *) {}
static void drag_context_finalize (GObject *o)
{
  g_list_free (((GdkDragContext *) o)->targets);
  G_OBJECT_CLASS (gdk_drag_context_parent_class)->finalize (o);
}
static void gdk_drag_context_class_init (GdkDragContextClass *klass) { G_OBJECT_CLASS (klass)->finalize = drag_context_finalize; }

struct DropTarget { QByteArray name; guint flags, info; };
struct DropData { QList<DropTarget> targets; GtkDestDefaults flags; GdkDragAction actions; };

/* the formats a Qt drop offers, as GTK target names */
static QList<QByteArray> drop_targets (const QMimeData *m)
{
  QList<QByteArray> t;
  if (m->hasUrls ()) t << "text/uri-list";
  if (m->hasText ()) t << "UTF8_STRING" << "text/plain" << "COMPOUND_TEXT" << "STRING" << "TEXT";
  for (const QString &f : m->formats ()) if (!t.contains (f.toUtf8 ())) t << f.toUtf8 ();
  return t;
}

static GdkDragAction qt_to_gdk_action (Qt::DropAction a)
{
  return a == Qt::MoveAction ? GDK_ACTION_MOVE : a == Qt::LinkAction ? GDK_ACTION_LINK : GDK_ACTION_COPY;
}

class GtkQtDropFilter : public QObject {
public:
  GtkWidget *widget;
  explicit GtkQtDropFilter (GtkWidget *w, QObject *parent) : QObject (parent), widget (w) {}
  bool eventFilter (QObject *, QEvent *e) override
  {
    if (e->type () != QEvent::DragEnter && e->type () != QEvent::DragMove && e->type () != QEvent::Drop &&
        e->type () != QEvent::DragLeave)
      return false;
    auto *dd = static_cast<DropData *> (g_object_get_data (G_OBJECT (widget), "gtkqt-drop"));
    if (!dd || !widget->priv || widget->priv->destroyed) return false;
    GtkQtEmitGuard guard;
    if (e->type () == QEvent::DragLeave) {
      g_signal_emit_by_name (widget, "drag-leave", nullptr, (guint) 0);
      return false;
    }
    auto *de = static_cast<QDropEvent *> (e);
    const QMimeData *m = de->mimeData ();
    QList<QByteArray> offered = drop_targets (m);
    const DropTarget *match = nullptr;
    for (const DropTarget &t : dd->targets)
      if (offered.contains (t.name)) { match = &t; break; }
    /* plain text into a text view: Qt's own drop handling does it */
    bool text_view = GTK_IS_TEXT_VIEW (widget);
    if (text_view && (!match || match->name != "text/uri-list")) return false;
    auto *ctx = (GdkDragContext *) g_object_new (gdk_drag_context_get_type (), nullptr);
    ctx->protocol = GDK_DRAG_PROTO_XDND;
    for (const QByteArray &t : offered) ctx->targets = g_list_append (ctx->targets, gdk_atom_intern (t.constData (), FALSE));
    ctx->actions = (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE);
    ctx->suggested_action = ctx->action = qt_to_gdk_action (de->proposedAction ());
    QPoint pos = de->position ().toPoint ();
    if (e->type () != QEvent::Drop) {
      gboolean handled = FALSE;
      g_signal_emit_by_name (widget, "drag-motion", ctx, pos.x (), pos.y (), (guint) 0, &handled);
      if (match || handled) de->acceptProposedAction (); else de->ignore ();
      g_object_unref (ctx);
      return true;
    }
    if (!match) { g_object_unref (ctx); de->ignore (); return true; }
    gboolean handled = FALSE;
    if (!(dd->flags & GTK_DEST_DEFAULT_DROP)) g_signal_emit_by_name (widget, "drag-drop", ctx, pos.x (), pos.y (), (guint) 0, &handled);
    GtkSelectionData sel = {};
    sel.selection = gdk_atom_intern ("XdndSelection", FALSE);
    sel.target = gdk_atom_intern (match->name.constData (), FALSE);
    sel.format = 8;
    QByteArray bytes;
    if (match->name == "text/uri-list") {
      for (const QUrl &u : m->urls ()) bytes += u.toEncoded () + "\r\n";
    } else if (m->hasText () && (match->name == "UTF8_STRING" || match->name.startsWith ("text/plain") ||
                                 match->name == "STRING" || match->name == "TEXT" || match->name == "COMPOUND_TEXT")) {
      bytes = m->text ().toUtf8 ();
    } else {
      bytes = m->data (QString::fromUtf8 (match->name));
    }
    gtk_selection_data_set (&sel, sel.target, 8, (const guchar *) bytes.constData (), bytes.size ());
    g_object_ref (widget);
    g_signal_emit_by_name (widget, "drag-data-received", ctx, pos.x (), pos.y (), &sel, match->info, (guint) 0);
    g_object_unref (widget);
    g_free (sel.data);
    de->acceptProposedAction ();
    g_object_unref (ctx);
    return true;
  }
};

extern "C" {

void gtk_drag_dest_set (GtkWidget *widget, GtkDestDefaults flags, const GtkTargetEntry *targets, gint n, GdkDragAction actions)
{
  auto *dd = new DropData;
  dd->flags = flags;
  dd->actions = actions;
  for (int i = 0; i < n; i++) dd->targets << DropTarget { QByteArray (targets[i].target), targets[i].flags, targets[i].info };
  g_object_set_data_full (G_OBJECT (widget), "gtkqt-drop", dd, [] (gpointer p) { delete static_cast<DropData *> (p); });
  QWidget *qw = gtkqt_qw (widget);
  if (!qw) return;
  QWidget *target = qw;
  if (auto *a = qobject_cast<QAbstractScrollArea *> (qw)) target = a->viewport ();
  target->setAcceptDrops (true);
  qw->setAcceptDrops (true);
  if (!target->findChild<GtkQtDropFilter *> (QString (), Qt::FindDirectChildrenOnly))
    target->installEventFilter (new GtkQtDropFilter (widget, target));
}

void gtk_drag_dest_unset (GtkWidget *widget) { g_object_set_data (G_OBJECT (widget), "gtkqt-drop", nullptr); }
void gtk_drag_finish (GdkDragContext *, gboolean, gboolean, guint32) {}
void gtk_drag_get_data (GtkWidget *, GdkDragContext *, GdkAtom, guint32) {}
void gtk_drag_highlight (GtkWidget *) {}
void gtk_drag_unhighlight (GtkWidget *) {}
void gdk_drag_status (GdkDragContext *context, GdkDragAction action, guint32) { if (context) context->action = action; }
GList *gdk_drag_context_list_targets (GdkDragContext *c) { return c ? c->targets : nullptr; }
GdkDragAction gdk_drag_context_get_actions (GdkDragContext *c) { return c ? c->actions : (GdkDragAction) 0; }
GdkDragAction gdk_drag_context_get_suggested_action (GdkDragContext *c) { return c ? c->suggested_action : (GdkDragAction) 0; }
GdkDragAction gdk_drag_context_get_selected_action (GdkDragContext *c) { return c ? c->action : (GdkDragAction) 0; }

} /* extern "C" */

/* ======================================================================
 * GtkIMContext (Qt does input methods; nothing is pre-edited from here)
 * ====================================================================== */
struct _GtkIMContext { GObject parent_instance; };
typedef struct { GObjectClass parent_class; } GtkIMContextClass;
G_DEFINE_TYPE (GtkIMContext, gtk_im_context, G_TYPE_OBJECT)
static void gtk_im_context_init (GtkIMContext *) {}
static void gtk_im_context_class_init (GtkIMContextClass *klass)
{
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("commit", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_STRING);
  g_signal_new ("preedit-changed", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("preedit-start", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
  g_signal_new ("preedit-end", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}

extern "C" {

void gtk_im_context_get_preedit_string (GtkIMContext *, gchar **str, PangoAttrList **attrs, gint *cursor_pos)
{
  if (str) *str = g_strdup ("");
  if (attrs) *attrs = pango_attr_list_new ();
  if (cursor_pos) *cursor_pos = 0;
}
void gtk_im_context_reset (GtkIMContext *) {}

} /* extern "C" */

/* ======================================================================
 * GtkItemFactory
 * ====================================================================== */
struct IfItem { QString path; GtkWidget *item; GtkWidget *submenu; guint action; };
struct IfData { QList<IfItem> items; gpointer popup_data = nullptr; GDestroyNotify popup_destroy = nullptr; };

G_DEFINE_TYPE (GtkItemFactory, gtk_item_factory, G_TYPE_OBJECT)
static IfData *ifd (GtkItemFactory *f) { return static_cast<IfData *> (g_object_get_data (G_OBJECT (f), "gtkqt-if")); }
static void gtk_item_factory_init (GtkItemFactory *f)
{
  g_object_set_data_full (G_OBJECT (f), "gtkqt-if", new IfData, [] (gpointer p) { delete static_cast<IfData *> (p); });
}
static void item_factory_finalize (GObject *o)
{
  auto *f = GTK_ITEM_FACTORY (o);
  if (f->translate_notify) f->translate_notify (f->translate_data);
  if (f->widget) g_object_unref (f->widget);
  if (f->accel_group) g_object_unref (f->accel_group);
  g_free (f->path);
  G_OBJECT_CLASS (gtk_item_factory_parent_class)->finalize (o);
}
static void gtk_item_factory_class_init (GtkItemFactoryClass *klass) { G_OBJECT_CLASS (klass)->finalize = item_factory_finalize; }

/* "/_File/Save _As..." -> "/File/Save As..." (lookups ignore mnemonics) */
static QString if_strip (const QString &p)
{
  QString r;
  for (int i = 0; i < p.size (); i++) {
    if (p[i] == '_') { if (i + 1 < p.size () && p[i + 1] == '_') { r += '_'; i++; } continue; }
    r += p[i];
  }
  return r;
}

static const IfItem *if_find (GtkItemFactory *f, const QString &path)
{
  QString key = if_strip (path);
  for (const IfItem &it : ifd (f)->items)
    if (it.path == key) return &it;
  return nullptr;
}

struct IfCallback { GtkItemFactoryCallback cb; gpointer data; guint action; guint type; };

static void if_activate (GtkWidget *item, gpointer p)
{
  auto *c = static_cast<IfCallback *> (p);
  if (!c->cb) return;
  if (c->type == 2) ((GtkItemFactoryCallback2) c->cb) (item, c->data, c->action);
  else ((GtkItemFactoryCallback1) c->cb) (c->data, c->action, item);
}

extern "C" {

GtkItemFactory *gtk_item_factory_new (GType container_type, const gchar *path, GtkAccelGroup *accel_group)
{
  auto *f = (GtkItemFactory *) g_object_new (gtk_item_factory_get_type (), nullptr);
  gtk_item_factory_construct (f, container_type, path, accel_group);
  return f;
}

void gtk_item_factory_construct (GtkItemFactory *f, GType container_type, const gchar *path, GtkAccelGroup *accel_group)
{
  f->path = g_strdup (path);
  f->accel_group = accel_group ? (GtkAccelGroup *) g_object_ref (accel_group) : gtk_accel_group_new ();
  f->widget = (GtkWidget *) g_object_ref_sink (g_object_new (container_type, nullptr));
  g_object_set_data (G_OBJECT (f->widget), "gtkqt-item-factory", f);
  if (GTK_IS_MENU (f->widget)) gtk_menu_set_accel_group (GTK_MENU (f->widget), f->accel_group);
  if (GTK_IS_OPTION_MENU (f->widget)) {
    GtkWidget *menu = gtk_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (f->widget), menu);
  }
  gtk_widget_show (f->widget);
  /* the factory lives as long as its widget */
  g_object_weak_ref (G_OBJECT (f->widget), [] (gpointer fp, GObject *) { g_object_unref (fp); }, g_object_ref (f));
}

void gtk_item_factory_set_translate_func (GtkItemFactory *f, GtkTranslateFunc func, gpointer data, GDestroyNotify notify)
{
  if (f->translate_notify) f->translate_notify (f->translate_data);
  f->translate_func = func;
  f->translate_data = data;
  f->translate_notify = notify;
}

static GtkWidget *if_menu_shell (GtkItemFactory *f, const QString &parent_path)
{
  if (parent_path.isEmpty ()) {
    if (GTK_IS_OPTION_MENU (f->widget)) return gtk_option_menu_get_menu (GTK_OPTION_MENU (f->widget));
    return f->widget;
  }
  const IfItem *p = if_find (f, parent_path);
  if (!p) {
    /* create the missing branch on the fly, as GTK does */
    GtkItemFactoryEntry e = {};
    QByteArray pp = parent_path.toUtf8 ();
    e.path = pp.data ();
    e.item_type = (gchar *) "<Branch>";
    gtk_item_factory_create_item (f, &e, nullptr, 1);
    p = if_find (f, parent_path);
    if (!p) return nullptr;
  }
  return p->submenu;
}

void gtk_item_factory_create_item (GtkItemFactory *f, GtkItemFactoryEntry *entry, gpointer callback_data, guint callback_type)
{
  if (!f || !entry || !entry->path) return;
  QString path = QString::fromUtf8 (entry->path);
  int slash = path.lastIndexOf ('/');
  if (slash < 0) return;
  QString parent_path = path.left (slash), name = path.mid (slash + 1);
  QString type = QString::fromUtf8 (entry->item_type ? entry->item_type : "");
  if (type.isEmpty () || type == "<Item>") type = "<Item>";

  /* the label: translated path's last component */
  QString label = name;
  if (f->translate_func) {
    gchar *t = f->translate_func (entry->path, f->translate_data);
    if (t) {
      QString tp = QString::fromUtf8 (t);
      int s = tp.lastIndexOf ('/');
      label = s >= 0 ? tp.mid (s + 1) : tp;
    }
  }
  QByteArray label8 = label.toUtf8 ();

  GtkWidget *shell = if_menu_shell (f, parent_path);
  if (!shell) return;
  GtkWidget *item = nullptr, *submenu = nullptr;
  if (type == "<Separator>") item = gtk_separator_menu_item_new ();
  else if (type == "<Tearoff>") return;   /* Qt menus are not torn off */
  else if (type == "<CheckItem>" || type == "<ToggleItem>") item = gtk_check_menu_item_new_with_mnemonic (label8.constData ());
  else if (type == "<RadioItem>") item = gtk_radio_menu_item_new_with_mnemonic (nullptr, label8.constData ());
  else if (type.startsWith ('/')) {
    /* a radio item joining the group of the item at that path */
    const IfItem *leader = if_find (f, type);
    GSList *group = leader && GTK_IS_RADIO_MENU_ITEM (leader->item) ? gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (leader->item)) : nullptr;
    item = gtk_radio_menu_item_new_with_mnemonic (group, label8.constData ());
  } else if (type == "<StockItem>" && entry->extra_data) {
    item = gtk_image_menu_item_new_from_stock ((const gchar *) entry->extra_data, nullptr);
    gtk_menu_item_set_label (GTK_MENU_ITEM (item), label8.constData ());
    gtk_menu_item_set_use_underline (GTK_MENU_ITEM (item), TRUE);
  } else if (type == "<ImageItem>") item = gtk_image_menu_item_new_with_mnemonic (label8.constData ());
  else if (type == "<Title>") { item = gtk_menu_item_new_with_label (label8.constData ()); gtk_widget_set_sensitive (item, FALSE); }
  else item = gtk_menu_item_new_with_mnemonic (label8.constData ());

  if (type == "<Branch>" || type == "<LastBranch>") {
    submenu = gtk_menu_new ();
    gtk_menu_set_accel_group (GTK_MENU (submenu), f->accel_group);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
    if (type == "<LastBranch>") gtk_menu_item_set_right_justified (GTK_MENU_ITEM (item), TRUE);
  }
  gtk_menu_shell_append (GTK_MENU_SHELL (shell), item);
  gtk_widget_show (item);
  g_object_set_data (G_OBJECT (item), "gtkqt-item-factory", f);

  if (entry->accelerator && *entry->accelerator && !submenu) {
    guint key; GdkModifierType mods;
    gtk_accelerator_parse (entry->accelerator, &key, &mods);
    if (key) gtk_widget_add_accelerator (item, "activate", f->accel_group, key, mods, GTK_ACCEL_VISIBLE);
  }
  if (entry->callback && !submenu && type != "<Separator>") {
    auto *c = new IfCallback { entry->callback, callback_data, entry->callback_action, callback_type };
    g_signal_connect_data (item, "activate", G_CALLBACK (if_activate), c,
                           [] (gpointer p, GClosure *) { delete static_cast<IfCallback *> (p); }, (GConnectFlags) 0);
  }
  QString key = if_strip (path);
  ifd (f)->items << IfItem { key, item, submenu, entry->callback_action };
  g_object_set_data_full (G_OBJECT (item), "gtkqt-item-path", g_strdup (key.toUtf8 ().constData ()), g_free);
  f->items = g_slist_append (f->items, item);
}

void gtk_item_factory_create_items_ac (GtkItemFactory *f, guint n, GtkItemFactoryEntry *entries, gpointer data, guint type)
{
  for (guint i = 0; i < n; i++) gtk_item_factory_create_item (f, &entries[i], data, type);
}

void gtk_item_factory_create_items (GtkItemFactory *f, guint n, GtkItemFactoryEntry *entries, gpointer data)
{
  gtk_item_factory_create_items_ac (f, n, entries, data, 1);
}

GtkItemFactory *gtk_item_factory_from_widget (GtkWidget *widget)
{
  for (GtkWidget *w = widget; w; w = w->parent)
    if (auto *f = (GtkItemFactory *) g_object_get_data (G_OBJECT (w), "gtkqt-item-factory")) return f;
  return nullptr;
}

const gchar *gtk_item_factory_path_from_widget (GtkWidget *widget)
{
  return (const gchar *) g_object_get_data (G_OBJECT (widget), "gtkqt-item-path");
}

GtkWidget *gtk_item_factory_get_item (GtkItemFactory *f, const gchar *path)
{
  if (!f || !path) return nullptr;
  if (f->path && !strcmp (path, f->path)) return f->widget;
  QString p = QString::fromUtf8 (path);
  if (f->path && p.startsWith (QString::fromUtf8 (f->path))) p = p.mid (strlen (f->path));
  const IfItem *it = if_find (f, p);
  return it ? it->item : nullptr;
}

GtkWidget *gtk_item_factory_get_widget (GtkItemFactory *f, const gchar *path)
{
  if (!f || !path) return nullptr;
  if (f->path && !strcmp (path, f->path)) return f->widget;
  QString p = QString::fromUtf8 (path);
  if (f->path && p.startsWith (QString::fromUtf8 (f->path))) p = p.mid (strlen (f->path));
  const IfItem *it = if_find (f, p);
  return it ? (it->submenu ? it->submenu : it->item) : nullptr;
}

GtkWidget *gtk_item_factory_get_item_by_action (GtkItemFactory *f, guint action)
{
  for (const IfItem &it : ifd (f)->items) if (it.action == action) return it.item;
  return nullptr;
}

GtkWidget *gtk_item_factory_get_widget_by_action (GtkItemFactory *f, guint action)
{
  for (const IfItem &it : ifd (f)->items) if (it.action == action) return it.submenu ? it.submenu : it.item;
  return nullptr;
}

void gtk_item_factory_delete_item (GtkItemFactory *f, const gchar *path)
{
  QString key = if_strip (QString::fromUtf8 (path));
  QList<IfItem> &items = ifd (f)->items;
  for (int i = 0; i < items.size (); i++)
    if (items[i].path == key) {
      GtkWidget *item = items[i].item;
      items.removeAt (i);
      f->items = g_slist_remove (f->items, item);
      gtk_widget_destroy (item);
      return;
    }
}
void gtk_item_factory_delete_entry (GtkItemFactory *f, GtkItemFactoryEntry *e) { gtk_item_factory_delete_item (f, e->path); }
void gtk_item_factory_delete_entries (GtkItemFactory *f, guint n, GtkItemFactoryEntry *e)
{
  for (guint i = 0; i < n; i++) gtk_item_factory_delete_item (f, e[i].path);
}

void gtk_item_factory_popup_with_data (GtkItemFactory *f, gpointer data, GDestroyNotify destroy, guint, guint, guint button, guint32 time_)
{
  IfData *d = ifd (f);
  if (d->popup_destroy) d->popup_destroy (d->popup_data);
  d->popup_data = data;
  d->popup_destroy = destroy;
  if (GTK_IS_MENU (f->widget)) gtk_menu_popup (GTK_MENU (f->widget), nullptr, nullptr, nullptr, nullptr, button, time_);
}
void gtk_item_factory_popup (GtkItemFactory *f, guint x, guint y, guint button, guint32 time_)
{
  gtk_item_factory_popup_with_data (f, nullptr, nullptr, x, y, button, time_);
}
gpointer gtk_item_factory_popup_data (GtkItemFactory *f) { return ifd (f)->popup_data; }
gpointer gtk_item_factory_popup_data_from_widget (GtkWidget *w)
{
  GtkItemFactory *f = gtk_item_factory_from_widget (w);
  return f ? ifd (f)->popup_data : nullptr;
}

} /* extern "C" */

/* ======================================================================
 * GtkFileSelection: a QFileDialog; ok_button/cancel_button are real
 * buttons whose "clicked" fires when the dialog is accepted/cancelled.
 * ====================================================================== */
G_DEFINE_TYPE (GtkFileSelection, gtk_file_selection, GTK_TYPE_DIALOG)

static QObject *file_selection_create (GtkWidget *w)
{
  auto *fd = new QFileDialog;
  fd->setProperty ("gtkqt-self-drawn", true);
  fd->setOption (QFileDialog::DontUseNativeDialog, true);
  fd->setWindowFlag (Qt::Window);
  fd->setFileMode (QFileDialog::AnyFile);
  gtkqt_connect (w, fd, &QDialog::finished, [] (GtkWidget *dw, int result) {
    auto *fs = GTK_FILE_SELECTION (dw);
    g_object_ref (dw);
    dw->priv->visible = false;
    if (result == QDialog::Accepted) {
      auto *qfd = gtkqt_q<QFileDialog> (dw);
      QStringList sel = qfd ? qfd->selectedFiles () : QStringList ();
      g_object_set_data_full (G_OBJECT (dw), "gtkqt-filename", g_strdup (sel.isEmpty () ? "" : QFile::encodeName (sel.first ()).constData ()), g_free);
    }
    GtkWidget *button = result == QDialog::Accepted ? fs->ok_button : fs->cancel_button;
    if (button) g_signal_emit_by_name (button, "clicked");
    if (dw->priv && !dw->priv->destroyed)
      gtk_dialog_response (GTK_DIALOG (dw), result == QDialog::Accepted ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL);
    g_object_unref (dw);
  });
  return fd;
}

static void gtk_file_selection_init (GtkFileSelection *fs)
{
  fs->ok_button = (GtkWidget *) g_object_ref_sink (gtk_button_new_from_stock ("gtk-ok"));
  fs->cancel_button = (GtkWidget *) g_object_ref_sink (gtk_button_new_from_stock ("gtk-cancel"));
  fs->main_vbox = (GtkWidget *) g_object_ref_sink (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  fs->selection_entry = (GtkWidget *) g_object_ref_sink (gtk_entry_new ());
}

static void file_selection_constructed (GObject *o)
{
  G_OBJECT_CLASS (gtk_file_selection_parent_class)->constructed (o);
  auto *fs = GTK_FILE_SELECTION (o);
  fs->action_area = fs->button_area = GTK_DIALOG (o)->action_area;
  g_object_set_data (G_OBJECT (o), "gtkqt-accept-id", GINT_TO_POINTER (GTK_RESPONSE_OK));
  g_object_set_data (G_OBJECT (o), "gtkqt-cancel-id", GINT_TO_POINTER (GTK_RESPONSE_CANCEL));
  gtk_widget_show (fs->main_vbox);
  gtk_file_chooser_set_extra_widget ((GtkFileChooser *) o, fs->main_vbox);
}

static void file_selection_dispose (GObject *o)
{
  auto *fs = GTK_FILE_SELECTION (o);
  GtkWidget **owned[] = { &fs->ok_button, &fs->cancel_button, &fs->main_vbox, &fs->selection_entry };
  for (GtkWidget **w : owned)
    if (*w) { GtkWidget *t = *w; *w = nullptr; gtk_widget_destroy (t); g_object_unref (t); }
  G_OBJECT_CLASS (gtk_file_selection_parent_class)->dispose (o);
}

static void gtk_file_selection_class_init (GtkFileSelectionClass *klass)
{
  G_OBJECT_CLASS (klass)->constructed = file_selection_constructed;
  G_OBJECT_CLASS (klass)->dispose = file_selection_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), file_selection_create);
}

extern "C" {

GtkWidget *gtk_file_selection_new (const gchar *title)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_FILE_SELECTION, nullptr);
  if (title) gtk_window_set_title (GTK_WINDOW (w), title);
  return w;
}

void gtk_file_selection_set_filename (GtkFileSelection *fs, const gchar *filename)
{
  if (auto *fd = gtkqt_q<QFileDialog> (GTK_WIDGET (fs))) {
    QString f = QFile::decodeName (filename);
    if (f.endsWith ('/')) fd->setDirectory (f);
    else {
      QFileInfo fi (f);
      if (!fi.path ().isEmpty ()) fd->setDirectory (fi.absolutePath ());
      fd->selectFile (fi.fileName ());
    }
  }
  g_object_set_data_full (G_OBJECT (fs), "gtkqt-filename", g_strdup (filename), g_free);
}

const gchar *gtk_file_selection_get_filename (GtkFileSelection *fs)
{
  if (auto *fd = gtkqt_q<QFileDialog> (GTK_WIDGET (fs)); fd && fd->isVisible ()) {
    QStringList sel = fd->selectedFiles ();
    if (!sel.isEmpty ())
      g_object_set_data_full (G_OBJECT (fs), "gtkqt-filename", g_strdup (QFile::encodeName (sel.first ()).constData ()), g_free);
  }
  const gchar *f = (const gchar *) g_object_get_data (G_OBJECT (fs), "gtkqt-filename");
  return f ? f : "";
}

gchar **gtk_file_selection_get_selections (GtkFileSelection *fs)
{
  QStringList sel;
  if (auto *fd = gtkqt_q<QFileDialog> (GTK_WIDGET (fs))) sel = fd->selectedFiles ();
  gchar **v = g_new0 (gchar *, sel.size () + 1);
  for (int i = 0; i < sel.size (); i++) v[i] = g_strdup (QFile::encodeName (sel[i]).constData ());
  return v;
}

void gtk_file_selection_set_select_multiple (GtkFileSelection *fs, gboolean multiple)
{
  if (auto *fd = gtkqt_q<QFileDialog> (GTK_WIDGET (fs))) fd->setFileMode (multiple ? QFileDialog::ExistingFiles : QFileDialog::AnyFile);
}
void gtk_file_selection_complete (GtkFileSelection *fs, const gchar *pattern)
{
  if (auto *fd = gtkqt_q<QFileDialog> (GTK_WIDGET (fs))) fd->setNameFilter (gtkqt_str (pattern));
}
void gtk_file_selection_show_fileop_buttons (GtkFileSelection *) {}
void gtk_file_selection_hide_fileop_buttons (GtkFileSelection *) {}

void gtk_file_chooser_set_extra_widget (GtkFileChooser *chooser, GtkWidget *extra)
{
  auto *fd = gtkqt_q<QFileDialog> (GTK_WIDGET (chooser));
  if (!fd) return;
  if (auto *old = (GtkWidget *) g_object_get_data (G_OBJECT (chooser), "gtkqt-extra-widget")) {
    if (QWidget *oq = gtkqt_qw (old)) { oq->hide (); oq->setParent (nullptr); }
  }
  if (extra) g_object_ref_sink (extra);
  g_object_set_data_full (G_OBJECT (chooser), "gtkqt-extra-widget", extra, extra ? g_object_unref : nullptr);
  if (!extra) return;
  auto *grid = qobject_cast<QGridLayout *> (fd->layout ());
  QWidget *eq = gtkqt_qw (extra);
  if (!grid || !eq) return;
  grid->addWidget (eq, grid->rowCount (), 0, 1, grid->columnCount ());
  if (gtk_widget_get_visible (extra)) eq->show ();
}

GtkWidget *gtk_file_chooser_get_extra_widget (GtkFileChooser *chooser)
{
  return (GtkWidget *) g_object_get_data (G_OBJECT (chooser), "gtkqt-extra-widget");
}

} /* extern "C" */

/* ======================================================================
 * GtkFontSelectionDialog: a QFontDialog
 * ====================================================================== */
G_DEFINE_TYPE (GtkFontSelectionDialog, gtk_font_selection_dialog, GTK_TYPE_DIALOG)

static QObject *font_dialog_create (GtkWidget *w)
{
  auto *fd = new QFontDialog;
  fd->setProperty ("gtkqt-self-drawn", true);
  fd->setOption (QFontDialog::DontUseNativeDialog, true);
  fd->setWindowFlag (Qt::Window);
  gtkqt_connect (w, fd, &QDialog::finished, [] (GtkWidget *dw, int result) {
    auto *fsd = GTK_FONT_SELECTION_DIALOG (dw);
    g_object_ref (dw);
    dw->priv->visible = false;
    GtkWidget *button = result == QDialog::Accepted ? fsd->ok_button : fsd->cancel_button;
    if (button) g_signal_emit_by_name (button, "clicked");
    if (dw->priv && !dw->priv->destroyed)
      gtk_dialog_response (GTK_DIALOG (dw), result == QDialog::Accepted ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL);
    g_object_unref (dw);
  });
  return fd;
}

static void gtk_font_selection_dialog_init (GtkFontSelectionDialog *d)
{
  d->ok_button = (GtkWidget *) g_object_ref_sink (gtk_button_new_from_stock ("gtk-ok"));
  d->cancel_button = (GtkWidget *) g_object_ref_sink (gtk_button_new_from_stock ("gtk-cancel"));
  d->apply_button = (GtkWidget *) g_object_ref_sink (gtk_button_new_from_stock ("gtk-apply"));
}
static void font_dialog_dispose (GObject *o)
{
  auto *d = GTK_FONT_SELECTION_DIALOG (o);
  GtkWidget **owned[] = { &d->ok_button, &d->cancel_button, &d->apply_button };
  for (GtkWidget **w : owned)
    if (*w) { GtkWidget *t = *w; *w = nullptr; gtk_widget_destroy (t); g_object_unref (t); }
  G_OBJECT_CLASS (gtk_font_selection_dialog_parent_class)->dispose (o);
}
static void gtk_font_selection_dialog_class_init (GtkFontSelectionDialogClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = font_dialog_dispose;
  gtkqt_set_create (GTK_WIDGET_CLASS (klass), font_dialog_create);
}

extern "C" {

GtkWidget *gtk_font_selection_dialog_new (const gchar *title)
{
  GtkWidget *w = (GtkWidget *) g_object_new (GTK_TYPE_FONT_SELECTION_DIALOG, nullptr);
  if (title) gtk_window_set_title (GTK_WINDOW (w), title);
  auto *d = GTK_FONT_SELECTION_DIALOG (w);
  d->main_vbox = GTK_DIALOG (w)->vbox;
  d->action_area = GTK_DIALOG (w)->action_area;
  return w;
}

gchar *gtk_font_selection_dialog_get_font_name (GtkFontSelectionDialog *d)
{
  auto *fd = gtkqt_q<QFontDialog> (GTK_WIDGET (d));
  if (!fd) return nullptr;
  PangoFontDescription *desc = gtkqt_pango_from_font (fd->currentFont ());
  gchar *s = pango_font_description_to_string (desc);
  pango_font_description_free (desc);
  return s;
}

gboolean gtk_font_selection_dialog_set_font_name (GtkFontSelectionDialog *d, const gchar *fontname)
{
  auto *fd = gtkqt_q<QFontDialog> (GTK_WIDGET (d));
  if (!fd || !fontname) return FALSE;
  PangoFontDescription *desc = pango_font_description_from_string (fontname);
  fd->setCurrentFont (gtkqt_font_from_pango (desc));
  pango_font_description_free (desc);
  return TRUE;
}

void gtk_font_selection_dialog_set_preview_text (GtkFontSelectionDialog *, const gchar *) {}

/* ======================================================================
 * Misc
 * ====================================================================== */
void gtk_button_box_set_child_secondary (GtkButtonBox *, GtkWidget *, gboolean) {}
void gtk_viewport_set_shadow_type (GtkViewport *viewport, GtkShadowType type)
{
  if (auto *f = qobject_cast<QFrame *> (gtkqt_qw (GTK_WIDGET (viewport))))
    f->setFrameShape (type == GTK_SHADOW_NONE ? QFrame::NoFrame : QFrame::StyledPanel);
}

} /* extern "C" */

/* GtkScrolledWindow adjustments, kept in step with the Qt scroll bars */
static GtkAdjustment *scroll_adjustment (GtkScrolledWindow *sw, bool vertical)
{
  const char *key = vertical ? "gtkqt-vadj" : "gtkqt-hadj";
  if (auto *a = (GtkAdjustment *) g_object_get_data (G_OBJECT (sw), key)) return a;
  auto *adj = (GtkAdjustment *) g_object_ref_sink (gtk_adjustment_new (0, 0, 0, 1, 10, 0));
  g_object_set_data_full (G_OBJECT (sw), key, adj, g_object_unref);
  QWidget *qw = gtkqt_qw (GTK_WIDGET (sw));
  QAbstractScrollArea *area = qw ? qw->findChild<QAbstractScrollArea *> (QString (), Qt::FindDirectChildrenOnly) : nullptr;
  if (!area) return adj;
  QScrollBar *sb = vertical ? area->verticalScrollBar () : area->horizontalScrollBar ();
  auto sync = [adj, sb] {
    g_object_set_data (G_OBJECT (adj), "gtkqt-syncing", GINT_TO_POINTER (1));
    gtk_adjustment_configure (adj, sb->value (), sb->minimum (), sb->maximum () + sb->pageStep (), sb->singleStep (),
                              sb->pageStep (), sb->pageStep ());
    g_object_set_data (G_OBJECT (adj), "gtkqt-syncing", nullptr);
  };
  sync ();
  QObject::connect (sb, &QScrollBar::rangeChanged, sb, sync);
  QObject::connect (sb, &QScrollBar::valueChanged, sb, sync);
  QPointer<QScrollBar> guard (sb);
  g_signal_connect_data (adj, "value-changed", G_CALLBACK (+[] (GtkAdjustment *a, gpointer p) {
    if (g_object_get_data (G_OBJECT (a), "gtkqt-syncing")) return;
    auto *g = static_cast<QPointer<QScrollBar> *> (p);
    if (*g) (*g)->setValue ((int) std::lround (gtk_adjustment_get_value (a)));
  }), new QPointer<QScrollBar> (guard), [] (gpointer p, GClosure *) { delete static_cast<QPointer<QScrollBar> *> (p); }, (GConnectFlags) 0);
  return adj;
}

extern "C" {
GtkAdjustment *gtk_scrolled_window_get_hadjustment (GtkScrolledWindow *sw) { return scroll_adjustment (sw, false); }
GtkAdjustment *gtk_scrolled_window_get_vadjustment (GtkScrolledWindow *sw) { return scroll_adjustment (sw, true); }
}
