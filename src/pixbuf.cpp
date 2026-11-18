/*
 * GTK made Qt - GdkPixbuf on QImage.
 *
 * A pixbuf's QImage is always Format_RGBA8888 (with alpha) or Format_RGB888
 * (without): byte for byte what gdk-pixbuf gives programs from
 * gdk_pixbuf_get_pixels (8-bit samples, R G B [A], not premultiplied,
 * rows 4-byte aligned), so programs can read and write the pixels.
 *
 * Licensed under the LGPL 2.1 or later; see LICENSE.LGPL.
 */
#include "gtkqt-private.h"
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QBuffer>
#include <QPainter>
#include <QFile>
#include <QHash>
#include <cstring>

struct _GdkPixbuf {
  GObject parent_instance;
  gpointer image;                        /* QImage */
  QHash<QByteArray, QByteArray> *options; /* "orientation", "gamma" ... from the loader */
};
typedef struct { GObjectClass parent_class; } GdkPixbufClass;

G_DEFINE_TYPE (GdkPixbuf, gdk_pixbuf, G_TYPE_OBJECT)
static void gdk_pixbuf_init (GdkPixbuf *p) { p->options = new QHash<QByteArray, QByteArray>; }
static void pixbuf_finalize (GObject *o)
{
  auto *p = (GdkPixbuf *) o;
  delete static_cast<QImage *> (p->image);
  delete p->options;
  G_OBJECT_CLASS (gdk_pixbuf_parent_class)->finalize (o);
}
static void gdk_pixbuf_class_init (GdkPixbufClass *klass) { G_OBJECT_CLASS (klass)->finalize = pixbuf_finalize; }

static QImage *img_of (const GdkPixbuf *p) { return p ? static_cast<QImage *> (p->image) : nullptr; }

/* the two formats a pixbuf has */
static QImage normalized (const QImage &image)
{
  if (image.format () == QImage::Format_RGBA8888 || image.format () == QImage::Format_RGB888) return image;
  return image.convertToFormat (image.hasAlphaChannel () ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
}

GdkPixbuf *gtkqt_pixbuf_from_image (const QImage &image)
{
  auto *p = (GdkPixbuf *) g_object_new (gdk_pixbuf_get_type (), nullptr);
  p->image = new QImage (normalized (image));
  return p;
}

QImage gtkqt_pixbuf_image (GdkPixbuf *pixbuf)
{
  QImage *img = img_of (pixbuf);
  return img ? *img : QImage ();
}

/* GdkPixdata, what gdk-pixbuf-csource writes for gdk_pixbuf_new_from_inline */
static QImage from_pixdata (const guint8 *data, gint length, QString *why)
{
  auto be32 = [data] (int off) { return (guint32) data[off] << 24 | (guint32) data[off + 1] << 16 | (guint32) data[off + 2] << 8 | data[off + 3]; };
  if (!data || (length >= 0 && length < 24) || be32 (0) != 0x47646b50) { *why = "not GdkPixdata"; return QImage (); }
  guint32 type = be32 (8), rowstride = be32 (12), width = be32 (16), height = be32 (20);
  bool alpha = (type & 0xff) == 0x02;
  int bpp = alpha ? 4 : 3;
  if ((type & 0xff0000) != 0x010000 || ((type & 0xff) != 0x01 && (type & 0xff) != 0x02) || !width || !height) {
    *why = "unsupported GdkPixdata";
    return QImage ();
  }
  QImage img (width, height, alpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
  const guint8 *p = data + 24, *end = length >= 0 ? data + length : nullptr;
  if ((type & 0x0f000000) == 0x01000000) {   /* raw rows */
    for (guint32 y = 0; y < height; y++) memcpy (img.scanLine (y), p + y * rowstride, width * bpp);
  } else {                                   /* run-length encoded pixels */
    guint32 n = 0, total = width * height;
    while (n < total && (!end || p < end)) {
      guint8 c = *p++;
      bool run = c & 0x80;
      guint32 count = run ? c - 128 : c;
      for (guint32 i = 0; i < count && n < total; i++, n++) {
        memcpy (img.scanLine (n / width) + (n % width) * bpp, p, bpp);
        if (!run) p += bpp;
      }
      if (run) p += bpp;
    }
  }
  return img;
}

/* what the image file says about itself, as gdk-pixbuf's loaders report it */
static void read_options (GdkPixbuf *p, QImageReader &reader)
{
  int o = 1;
  switch (reader.transformation ()) {
  case QImageIOHandler::TransformationNone: o = 1; break;
  case QImageIOHandler::TransformationMirror: o = 2; break;
  case QImageIOHandler::TransformationRotate180: o = 3; break;
  case QImageIOHandler::TransformationFlip: o = 4; break;
  case QImageIOHandler::TransformationFlipAndRotate90: o = 5; break;
  case QImageIOHandler::TransformationRotate90: o = 6; break;
  case QImageIOHandler::TransformationMirrorAndRotate90: o = 7; break;
  case QImageIOHandler::TransformationRotate270: o = 8; break;
  }
  if (o != 1) p->options->insert ("orientation", QByteArray::number (o));
}

static QImage::Format format_for (gboolean has_alpha) { return has_alpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888; }

static const char *qt_type (const char *type)
{
  if (!type) return "png";
  if (!g_ascii_strcasecmp (type, "jpeg")) return "jpg";
  if (!g_ascii_strcasecmp (type, "ico")) return "ico";
  return type;
}

static bool save_to (GdkPixbuf *pixbuf, QIODevice *dev, const char *type, char **keys, char **values, GError **error)
{
  QImage *img = img_of (pixbuf);
  QImageWriter writer (dev, qt_type (type));
  for (int i = 0; keys && keys[i]; i++)
    if (!strcmp (keys[i], "quality") || !strcmp (keys[i], "compression")) {
      int v = atoi (values[i]);
      writer.setQuality (!strcmp (keys[i], "compression") ? 100 - v * 10 : v);
    }
  if (!img || !writer.write (*img)) {
    g_set_error (error, GDK_PIXBUF_ERROR, writer.error () == QImageWriter::UnsupportedFormatError ? GDK_PIXBUF_ERROR_UNKNOWN_TYPE : GDK_PIXBUF_ERROR_FAILED,
                 "Could not save image: %s", qPrintable (writer.errorString ()));
    return false;
  }
  return true;
}

static void collect_options (va_list ap, QList<QByteArray> &keys, QList<QByteArray> &values)
{
  while (const char *k = va_arg (ap, const char *)) {
    const char *v = va_arg (ap, const char *);
    keys << QByteArray (k);
    values << QByteArray (v ? v : "");
  }
}

extern "C" {

GQuark gdk_pixbuf_error_quark (void) { return g_quark_from_static_string ("gdk-pixbuf-error-quark"); }

GdkPixbuf *gdk_pixbuf_new (GdkColorspace, gboolean has_alpha, int bits_per_sample, int width, int height)
{
  if (bits_per_sample != 8 || width <= 0 || height <= 0) return nullptr;
  QImage img (width, height, format_for (has_alpha));
  if (img.isNull ()) return nullptr;
  return gtkqt_pixbuf_from_image (img);
}

GdkPixbuf *gdk_pixbuf_new_from_data (const guchar *data, GdkColorspace, gboolean has_alpha, int bits_per_sample,
                                     int width, int height, int rowstride, GdkPixbufDestroyNotify destroy_fn, gpointer destroy_fn_data)
{
  if (!data || bits_per_sample != 8) return nullptr;
  /* the program's own memory, as in gdk-pixbuf: writes through the pixbuf reach it */
  struct Owner { GdkPixbufDestroyNotify fn; gpointer data; guchar *pixels; };
  auto *owner = new Owner { destroy_fn, destroy_fn_data, (guchar *) data };
  QImage img ((uchar *) data, width, height, rowstride, format_for (has_alpha),
              [] (void *info) { auto *o = static_cast<Owner *> (info); if (o->fn) o->fn (o->pixels, o->data); delete o; }, owner);
  auto *p = (GdkPixbuf *) g_object_new (gdk_pixbuf_get_type (), nullptr);
  p->image = new QImage (img);
  return p;
}

GdkPixbuf *gdk_pixbuf_new_from_bytes (GBytes *bytes, GdkColorspace cs, gboolean has_alpha, int bps, int width, int height, int rowstride)
{
  g_bytes_ref (bytes);
  return gdk_pixbuf_new_from_data ((const guchar *) g_bytes_get_data (bytes, nullptr), cs, has_alpha, bps, width, height, rowstride,
                                   [] (guchar *, gpointer b) { g_bytes_unref ((GBytes *) b); }, bytes);
}

GdkPixbuf *gdk_pixbuf_new_from_file (const char *filename, GError **error)
{
  QImageReader reader (QString::fromLocal8Bit (filename));
  reader.setAutoTransform (false);   /* gdk_pixbuf_apply_embedded_orientation does that */
  QImage img = reader.read ();
  if (img.isNull ()) {
    bool missing = !QFile::exists (QString::fromLocal8Bit (filename));
    if (missing) g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT, "Failed to open file '%s': No such file or directory", filename);
    else g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_UNKNOWN_TYPE, "Couldn't recognize the image file format for file '%s'", filename);
    return nullptr;
  }
  GdkPixbuf *p = gtkqt_pixbuf_from_image (img);
  read_options (p, reader);
  return p;
}

GdkPixbuf *gdk_pixbuf_new_from_file_at_scale (const char *filename, int width, int height, gboolean preserve_aspect_ratio, GError **error)
{
  GdkPixbuf *p = gdk_pixbuf_new_from_file (filename, error);
  if (!p) return nullptr;
  QImage *img = img_of (p);
  int w = width > 0 ? width : img->width (), h = height > 0 ? height : img->height ();
  if (width <= 0 && height > 0 && preserve_aspect_ratio) w = img->width () * height / qMax (1, img->height ());
  if (height <= 0 && width > 0 && preserve_aspect_ratio) h = img->height () * width / qMax (1, img->width ());
  *img = normalized (img->scaled (qMax (1, w), qMax (1, h), preserve_aspect_ratio ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio,
                                  Qt::SmoothTransformation));
  return p;
}

GdkPixbuf *gdk_pixbuf_new_from_file_at_size (const char *filename, int width, int height, GError **error)
{
  return gdk_pixbuf_new_from_file_at_scale (filename, width, height, TRUE, error);
}

GdkPixbuf *gdk_pixbuf_new_from_xpm_data (const char **data)
{
  QImage img (data);
  return img.isNull () ? nullptr : gtkqt_pixbuf_from_image (img);
}

GdkPixbuf *gdk_pixbuf_new_from_inline (gint data_length, const guint8 *data, gboolean, GError **error)
{
  QString why;
  QImage img = from_pixdata (data, data_length, &why);
  if (img.isNull ()) {
    g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE, "%s", qPrintable (why));
    return nullptr;
  }
  return gtkqt_pixbuf_from_image (img);
}

GdkPixbuf *gdk_pixbuf_new_from_resource (const char *path, GError **error)
{
  GBytes *bytes = g_resources_lookup_data (path, G_RESOURCE_LOOKUP_FLAGS_NONE, error);
  if (!bytes) return nullptr;
  gsize n = 0;
  const guint8 *d = (const guint8 *) g_bytes_get_data (bytes, &n);
  QImage img = QImage::fromData (d, (int) n);
  QString why;
  if (img.isNull ()) img = from_pixdata (d, (gint) n, &why);
  g_bytes_unref (bytes);
  if (img.isNull ()) {
    g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_UNKNOWN_TYPE, "Couldn't recognize the image format of %s", path);
    return nullptr;
  }
  return gtkqt_pixbuf_from_image (img);
}

GdkPixbuf *gdk_pixbuf_new_subpixbuf (GdkPixbuf *src, int x, int y, int width, int height)
{
  QImage *img = img_of (src);
  if (!img || x < 0 || y < 0 || x + width > img->width () || y + height > img->height ()) return nullptr;
  /* shares the parent's pixels, as in gdk-pixbuf */
  int bpp = img->hasAlphaChannel () ? 4 : 3;
  g_object_ref (src);
  return gdk_pixbuf_new_from_data (img->bits () + y * img->bytesPerLine () + x * bpp, GDK_COLORSPACE_RGB, img->hasAlphaChannel (), 8,
                                   width, height, img->bytesPerLine (), [] (guchar *, gpointer s) { g_object_unref (s); }, src);
}

GdkPixbuf *gdk_pixbuf_copy (const GdkPixbuf *pixbuf)
{
  QImage *img = img_of (pixbuf);
  if (!img) return nullptr;
  GdkPixbuf *p = gtkqt_pixbuf_from_image (img->copy ());
  *p->options = *pixbuf->options;
  return p;
}

GdkPixbuf *gdk_pixbuf_ref (GdkPixbuf *pixbuf) { return pixbuf ? (GdkPixbuf *) g_object_ref (pixbuf) : nullptr; }
void gdk_pixbuf_unref (GdkPixbuf *pixbuf) { if (pixbuf) g_object_unref (pixbuf); }

GdkColorspace gdk_pixbuf_get_colorspace (const GdkPixbuf *) { return GDK_COLORSPACE_RGB; }
gboolean gdk_pixbuf_get_has_alpha (const GdkPixbuf *p) { return img_of (p) && img_of (p)->hasAlphaChannel (); }
int gdk_pixbuf_get_n_channels (const GdkPixbuf *p) { return gdk_pixbuf_get_has_alpha (p) ? 4 : 3; }
int gdk_pixbuf_get_bits_per_sample (const GdkPixbuf *) { return 8; }
int gdk_pixbuf_get_width (const GdkPixbuf *p) { return img_of (p) ? img_of (p)->width () : 0; }
int gdk_pixbuf_get_height (const GdkPixbuf *p) { return img_of (p) ? img_of (p)->height () : 0; }
int gdk_pixbuf_get_rowstride (const GdkPixbuf *p) { return img_of (p) ? img_of (p)->bytesPerLine () : 0; }
guchar *gdk_pixbuf_get_pixels (const GdkPixbuf *p) { return img_of (p) ? img_of (p)->bits () : nullptr; }
const guint8 *gdk_pixbuf_read_pixels (const GdkPixbuf *p) { return img_of (p) ? img_of (p)->constBits () : nullptr; }
/* the bytes up to the end of the last row's pixels, as gdk-pixbuf counts them */
gsize gdk_pixbuf_get_byte_length (const GdkPixbuf *p)
{
  QImage *img = img_of (p);
  if (!img) return 0;
  return (gsize) (img->height () - 1) * img->bytesPerLine () + (gsize) img->width () * gdk_pixbuf_get_n_channels (p);
}
guchar *gdk_pixbuf_get_pixels_with_length (const GdkPixbuf *p, guint *length)
{
  if (length) *length = (guint) gdk_pixbuf_get_byte_length (p);
  return gdk_pixbuf_get_pixels (p);
}

const gchar *gdk_pixbuf_get_option (GdkPixbuf *p, const gchar *key)
{
  if (!p || !key) return nullptr;
  auto it = p->options->constFind (key);
  return it == p->options->constEnd () ? nullptr : it->constData ();
}
gboolean gdk_pixbuf_set_option (GdkPixbuf *p, const gchar *key, const gchar *value)
{
  if (!p || !key || p->options->contains (key)) return FALSE;
  p->options->insert (key, value ? value : "");
  return TRUE;
}

void gdk_pixbuf_fill (GdkPixbuf *p, guint32 pixel)
{
  QImage *img = img_of (p);
  if (img) img->fill (QColor ((pixel >> 24) & 0xff, (pixel >> 16) & 0xff, (pixel >> 8) & 0xff, pixel & 0xff));
}

GdkPixbuf *gdk_pixbuf_add_alpha (const GdkPixbuf *p, gboolean substitute_color, guchar r, guchar g, guchar b)
{
  QImage *img = img_of (p);
  if (!img) return nullptr;
  QImage out = img->convertToFormat (QImage::Format_RGBA8888);
  if (substitute_color && !img->hasAlphaChannel ())
    for (int y = 0; y < out.height (); y++) {
      uchar *row = out.scanLine (y);
      for (int x = 0; x < out.width (); x++, row += 4)
        if (row[0] == r && row[1] == g && row[2] == b) row[3] = 0;
    }
  return gtkqt_pixbuf_from_image (out);
}

void gdk_pixbuf_copy_area (const GdkPixbuf *src, int sx, int sy, int width, int height, GdkPixbuf *dest, int dx, int dy)
{
  QImage *s = img_of (src), *d = img_of (dest);
  if (!s || !d) return;
  QImage part = s->copy (sx, sy, width, height).convertToFormat (d->format ());
  int bpp = d->hasAlphaChannel () ? 4 : 3;
  for (int y = 0; y < part.height () && dy + y < d->height (); y++)
    memcpy (d->scanLine (dy + y) + dx * bpp, part.constScanLine (y), qMin (part.width (), d->width () - dx) * bpp);
}

void gdk_pixbuf_saturate_and_pixelate (const GdkPixbuf *src, GdkPixbuf *dest, gfloat saturation, gboolean pixelate)
{
  QImage *s = img_of (src), *d = img_of (dest);
  if (!s || !d) return;
  QImage out = s->convertToFormat (d->format ());
  int bpp = out.hasAlphaChannel () ? 4 : 3;
  for (int y = 0; y < out.height (); y++) {
    uchar *row = out.scanLine (y);
    for (int x = 0; x < out.width (); x++, row += bpp) {
      /* as gdk-pixbuf: towards the intensity (saturation < 1) or away from
       * it, and darkened on a checkerboard when pixelating */
      int intensity = (int) (row[0] * 0.30 + row[1] * 0.59 + row[2] * 0.11);
      bool dark = pixelate && ((x + y) % 2) == 0;
      for (int c = 0; c < 3; c++) {
        int v = (int) ((1.0 - saturation) * intensity + saturation * row[c]);
        row[c] = (uchar) qBound (0, dark ? v * 7 / 10 : v, 255);
      }
    }
  }
  *d = out;
}

GdkPixbuf *gdk_pixbuf_scale_simple (const GdkPixbuf *src, int width, int height, GdkInterpType interp)
{
  QImage *img = img_of (src);
  if (!img || width <= 0 || height <= 0) return nullptr;
  return gtkqt_pixbuf_from_image (img->scaled (width, height, Qt::IgnoreAspectRatio,
                                               interp == GDK_INTERP_NEAREST ? Qt::FastTransformation : Qt::SmoothTransformation));
}

/* scale and composite: the source, offset and scaled, drawn into a region of
 * the destination (replacing it, or over it with an overall alpha) */
static void draw_scaled (const GdkPixbuf *src, GdkPixbuf *dest, int dx, int dy, int dw, int dh, double ox, double oy,
                         double sx, double sy, GdkInterpType interp, int alpha, bool replace)
{
  QImage *s = img_of (src), *d = img_of (dest);
  if (!s || !d) return;
  QImage target = d->convertToFormat (QImage::Format_ARGB32);
  {
    QPainter p (&target);
    p.setClipRect (dx, dy, dw, dh);
    p.setRenderHint (QPainter::SmoothPixmapTransform, interp != GDK_INTERP_NEAREST);
    p.setCompositionMode (replace ? QPainter::CompositionMode_Source : QPainter::CompositionMode_SourceOver);
    p.setOpacity (qBound (0, alpha, 255) / 255.0);
    p.translate (ox, oy);
    p.scale (sx, sy);
    p.drawImage (0, 0, *s);
  }
  *d = target.convertToFormat (d->format ());
}

void gdk_pixbuf_scale (const GdkPixbuf *src, GdkPixbuf *dest, int dx, int dy, int dw, int dh, double ox, double oy,
                       double sx, double sy, GdkInterpType interp)
{
  draw_scaled (src, dest, dx, dy, dw, dh, ox, oy, sx, sy, interp, 255, true);
}

void gdk_pixbuf_composite (const GdkPixbuf *src, GdkPixbuf *dest, int dx, int dy, int dw, int dh, double ox, double oy,
                           double sx, double sy, GdkInterpType interp, int overall_alpha)
{
  draw_scaled (src, dest, dx, dy, dw, dh, ox, oy, sx, sy, interp, overall_alpha, false);
}

GdkPixbuf *gdk_pixbuf_rotate_simple (const GdkPixbuf *src, GdkPixbufRotation angle)
{
  QImage *img = img_of (src);
  if (!img) return nullptr;
  QTransform t;
  t.rotate (-(int) angle);   /* gdk-pixbuf's angles are counterclockwise */
  return gtkqt_pixbuf_from_image (img->transformed (t));
}

GdkPixbuf *gdk_pixbuf_flip (const GdkPixbuf *src, gboolean horizontal)
{
  QImage *img = img_of (src);
  if (!img) return nullptr;
  /* (QImage::flipped is Qt 6.9+; a transform works with every Qt) */
  return gtkqt_pixbuf_from_image (img->transformed (QTransform::fromScale (horizontal ? -1 : 1, horizontal ? 1 : -1)));
}

/* the EXIF orientation the loader found ("orientation" option), applied */
GdkPixbuf *gdk_pixbuf_apply_embedded_orientation (GdkPixbuf *src)
{
  if (!src) return nullptr;
  const gchar *o = gdk_pixbuf_get_option (src, "orientation");
  int orientation = o ? atoi (o) : 1;
  GdkPixbuf *a = nullptr, *b = nullptr;
  switch (orientation) {
  case 2: return gdk_pixbuf_flip (src, TRUE);
  case 3: return gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
  case 4: return gdk_pixbuf_flip (src, FALSE);
  case 5: a = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE); b = gdk_pixbuf_flip (a, TRUE); g_object_unref (a); return b;
  case 6: return gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
  case 7: a = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE); b = gdk_pixbuf_flip (a, TRUE); g_object_unref (a); return b;
  case 8: return gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
  default: return (GdkPixbuf *) g_object_ref (src);
  }
}

gboolean gdk_pixbuf_savev (GdkPixbuf *pixbuf, const char *filename, const char *type, char **keys, char **values, GError **error)
{
  QFile f (QString::fromLocal8Bit (filename));
  if (!f.open (QIODevice::WriteOnly)) {
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_ACCES, "Failed to open '%s' for writing", filename);
    return FALSE;
  }
  return save_to (pixbuf, &f, type, keys, values, error);
}

static gboolean save_varargs (GdkPixbuf *pixbuf, const char *filename, gchar **buffer, gsize *size, GdkPixbufSaveFunc fn, gpointer fn_data,
                              const char *type, GError **error, va_list ap)
{
  QList<QByteArray> keys, values;
  collect_options (ap, keys, values);
  QVector<char *> k, v;
  for (int i = 0; i < keys.size (); i++) { k << keys[i].data (); v << values[i].data (); }
  k << nullptr;
  v << nullptr;
  if (filename) return gdk_pixbuf_savev (pixbuf, filename, type, k.data (), v.data (), error);
  QBuffer buf;
  buf.open (QIODevice::WriteOnly);
  if (!save_to (pixbuf, &buf, type, k.data (), v.data (), error)) return FALSE;
  if (fn) return fn (buf.data ().constData (), buf.data ().size (), error, fn_data);
  *buffer = (gchar *) g_memdup2 (buf.data ().constData (), buf.data ().size ());
  if (size) *size = buf.data ().size ();
  return TRUE;
}

gboolean gdk_pixbuf_save (GdkPixbuf *pixbuf, const char *filename, const char *type, GError **error, ...)
{
  va_list ap;
  va_start (ap, error);
  gboolean ok = save_varargs (pixbuf, filename, nullptr, nullptr, nullptr, nullptr, type, error, ap);
  va_end (ap);
  return ok;
}

gboolean gdk_pixbuf_save_to_buffer (GdkPixbuf *pixbuf, gchar **buffer, gsize *buffer_size, const char *type, GError **error, ...)
{
  va_list ap;
  va_start (ap, error);
  gboolean ok = save_varargs (pixbuf, nullptr, buffer, buffer_size, nullptr, nullptr, type, error, ap);
  va_end (ap);
  return ok;
}

gboolean gdk_pixbuf_save_to_bufferv (GdkPixbuf *pixbuf, gchar **buffer, gsize *buffer_size, const char *type, char **keys, char **values, GError **error)
{
  QBuffer buf;
  buf.open (QIODevice::WriteOnly);
  if (!save_to (pixbuf, &buf, type, keys, values, error)) return FALSE;
  *buffer = (gchar *) g_memdup2 (buf.data ().constData (), buf.data ().size ());
  if (buffer_size) *buffer_size = buf.data ().size ();
  return TRUE;
}

gboolean gdk_pixbuf_save_to_callback (GdkPixbuf *pixbuf, GdkPixbufSaveFunc fn, gpointer data, const char *type, GError **error, ...)
{
  va_list ap;
  va_start (ap, error);
  gboolean ok = save_varargs (pixbuf, nullptr, nullptr, nullptr, fn, data, type, error, ap);
  va_end (ap);
  return ok;
}

} /* extern "C" */
