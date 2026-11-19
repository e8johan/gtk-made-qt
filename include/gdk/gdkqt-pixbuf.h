/*
 * GTK made Qt - the GdkPixbuf API (gdk-pixbuf 2), on QImage.
 * Pixels are what gdk-pixbuf gives programs: 8-bit RGB or RGBA, not
 * premultiplied, rows rowstride bytes apart (gdk_pixbuf_get_pixels).
 * Licensed under the LGPL 2.1 or later; see LICENSE.LGPL.
 */
#ifndef GTKQT_PIXBUF_H
#define GTKQT_PIXBUF_H

G_BEGIN_DECLS

typedef enum { GDK_COLORSPACE_RGB } GdkColorspace;
typedef enum { GDK_INTERP_NEAREST, GDK_INTERP_TILES, GDK_INTERP_BILINEAR, GDK_INTERP_HYPER } GdkInterpType;
typedef enum { GDK_PIXBUF_ROTATE_NONE = 0, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE = 90, GDK_PIXBUF_ROTATE_UPSIDEDOWN = 180,
               GDK_PIXBUF_ROTATE_CLOCKWISE = 270 } GdkPixbufRotation;
typedef enum { GDK_PIXBUF_ALPHA_BILEVEL, GDK_PIXBUF_ALPHA_FULL } GdkPixbufAlphaMode;
typedef enum {
  GDK_PIXBUF_ERROR_CORRUPT_IMAGE, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY, GDK_PIXBUF_ERROR_BAD_OPTION,
  GDK_PIXBUF_ERROR_UNKNOWN_TYPE, GDK_PIXBUF_ERROR_UNSUPPORTED_OPERATION, GDK_PIXBUF_ERROR_FAILED,
  GDK_PIXBUF_ERROR_INCOMPLETE_ANIMATION
} GdkPixbufError;
#define GDK_PIXBUF_ERROR gdk_pixbuf_error_quark ()
GQuark gdk_pixbuf_error_quark (void);

typedef void (*GdkPixbufDestroyNotify) (guchar *pixels, gpointer data);
typedef gboolean (*GdkPixbufSaveFunc) (const gchar *buf, gsize count, GError **error, gpointer data);

#define GDK_TYPE_PIXBUF (gdk_pixbuf_get_type ())
#define GDK_PIXBUF(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_PIXBUF, GdkPixbuf))
#define GDK_IS_PIXBUF(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_PIXBUF))
GType      gdk_pixbuf_get_type (void);

GdkPixbuf *gdk_pixbuf_new (GdkColorspace colorspace, gboolean has_alpha, int bits_per_sample, int width, int height);
GdkPixbuf *gdk_pixbuf_new_from_data (const guchar *data, GdkColorspace colorspace, gboolean has_alpha, int bits_per_sample,
                                     int width, int height, int rowstride, GdkPixbufDestroyNotify destroy_fn, gpointer destroy_fn_data);
GdkPixbuf *gdk_pixbuf_new_from_bytes (GBytes *data, GdkColorspace colorspace, gboolean has_alpha, int bits_per_sample,
                                      int width, int height, int rowstride);
GdkPixbuf *gdk_pixbuf_new_from_file (const char *filename, GError **error);
GdkPixbuf *gdk_pixbuf_new_from_file_at_size (const char *filename, int width, int height, GError **error);
GdkPixbuf *gdk_pixbuf_new_from_file_at_scale (const char *filename, int width, int height, gboolean preserve_aspect_ratio, GError **error);
GdkPixbuf *gdk_pixbuf_new_from_xpm_data (const char **data);
GdkPixbuf *gdk_pixbuf_new_from_inline (gint data_length, const guint8 *data, gboolean copy_pixels, GError **error);
GdkPixbuf *gdk_pixbuf_new_from_resource (const char *resource_path, GError **error);
GdkPixbuf *gdk_pixbuf_new_subpixbuf (GdkPixbuf *src_pixbuf, int src_x, int src_y, int width, int height);
GdkPixbuf *gdk_pixbuf_copy (const GdkPixbuf *pixbuf);

GdkPixbuf *gdk_pixbuf_ref (GdkPixbuf *pixbuf);
void       gdk_pixbuf_unref (GdkPixbuf *pixbuf);

GdkColorspace gdk_pixbuf_get_colorspace (const GdkPixbuf *pixbuf);
int        gdk_pixbuf_get_n_channels (const GdkPixbuf *pixbuf);
gboolean   gdk_pixbuf_get_has_alpha (const GdkPixbuf *pixbuf);
int        gdk_pixbuf_get_bits_per_sample (const GdkPixbuf *pixbuf);
guchar    *gdk_pixbuf_get_pixels (const GdkPixbuf *pixbuf);
guchar    *gdk_pixbuf_get_pixels_with_length (const GdkPixbuf *pixbuf, guint *length);
const guint8 *gdk_pixbuf_read_pixels (const GdkPixbuf *pixbuf);
int        gdk_pixbuf_get_width (const GdkPixbuf *pixbuf);
int        gdk_pixbuf_get_height (const GdkPixbuf *pixbuf);
int        gdk_pixbuf_get_rowstride (const GdkPixbuf *pixbuf);
gsize      gdk_pixbuf_get_byte_length (const GdkPixbuf *pixbuf);
const gchar *gdk_pixbuf_get_option (GdkPixbuf *pixbuf, const gchar *key);
gboolean   gdk_pixbuf_set_option (GdkPixbuf *pixbuf, const gchar *key, const gchar *value);

void       gdk_pixbuf_fill (GdkPixbuf *pixbuf, guint32 pixel);
GdkPixbuf *gdk_pixbuf_add_alpha (const GdkPixbuf *pixbuf, gboolean substitute_color, guchar r, guchar g, guchar b);
void       gdk_pixbuf_copy_area (const GdkPixbuf *src_pixbuf, int src_x, int src_y, int width, int height,
                                 GdkPixbuf *dest_pixbuf, int dest_x, int dest_y);
void       gdk_pixbuf_saturate_and_pixelate (const GdkPixbuf *src, GdkPixbuf *dest, gfloat saturation, gboolean pixelate);
GdkPixbuf *gdk_pixbuf_scale_simple (const GdkPixbuf *src, int dest_width, int dest_height, GdkInterpType interp_type);
void       gdk_pixbuf_scale (const GdkPixbuf *src, GdkPixbuf *dest, int dest_x, int dest_y, int dest_width, int dest_height,
                             double offset_x, double offset_y, double scale_x, double scale_y, GdkInterpType interp_type);
void       gdk_pixbuf_composite (const GdkPixbuf *src, GdkPixbuf *dest, int dest_x, int dest_y, int dest_width, int dest_height,
                                 double offset_x, double offset_y, double scale_x, double scale_y, GdkInterpType interp_type,
                                 int overall_alpha);
GdkPixbuf *gdk_pixbuf_rotate_simple (const GdkPixbuf *src, GdkPixbufRotation angle);
GdkPixbuf *gdk_pixbuf_flip (const GdkPixbuf *src, gboolean horizontal);
GdkPixbuf *gdk_pixbuf_apply_embedded_orientation (GdkPixbuf *src);

gboolean   gdk_pixbuf_save (GdkPixbuf *pixbuf, const char *filename, const char *type, GError **error, ...) G_GNUC_NULL_TERMINATED;
gboolean   gdk_pixbuf_savev (GdkPixbuf *pixbuf, const char *filename, const char *type, char **option_keys, char **option_values, GError **error);
gboolean   gdk_pixbuf_save_to_buffer (GdkPixbuf *pixbuf, gchar **buffer, gsize *buffer_size, const char *type, GError **error, ...) G_GNUC_NULL_TERMINATED;
gboolean   gdk_pixbuf_save_to_bufferv (GdkPixbuf *pixbuf, gchar **buffer, gsize *buffer_size, const char *type,
                                       char **option_keys, char **option_values, GError **error);
gboolean   gdk_pixbuf_save_to_callback (GdkPixbuf *pixbuf, GdkPixbufSaveFunc save_func, gpointer user_data, const char *type, GError **error, ...) G_GNUC_NULL_TERMINATED;

/* Image formats: the ones Qt's image plugins read */
typedef struct _GdkPixbufFormat GdkPixbufFormat;
GSList     *gdk_pixbuf_get_formats (void);
GdkPixbufFormat *gdk_pixbuf_get_file_info (const gchar *filename, gint *width, gint *height);
gchar      *gdk_pixbuf_format_get_name (GdkPixbufFormat *format);
gchar      *gdk_pixbuf_format_get_description (GdkPixbufFormat *format);
gchar     **gdk_pixbuf_format_get_mime_types (GdkPixbufFormat *format);
gchar     **gdk_pixbuf_format_get_extensions (GdkPixbufFormat *format);
gboolean    gdk_pixbuf_format_is_writable (GdkPixbufFormat *format);
gboolean    gdk_pixbuf_format_is_scalable (GdkPixbufFormat *format);
gboolean    gdk_pixbuf_format_is_disabled (GdkPixbufFormat *format);
void        gdk_pixbuf_format_set_disabled (GdkPixbufFormat *format, gboolean disabled);
gchar      *gdk_pixbuf_format_get_license (GdkPixbufFormat *format);
GdkPixbufFormat *gdk_pixbuf_format_copy (const GdkPixbufFormat *format);
void        gdk_pixbuf_format_free (GdkPixbufFormat *format);

/* GdkPixbufAnimation: every frame decoded, with its delay */
typedef struct _GdkPixbufAnimation GdkPixbufAnimation;
typedef struct _GdkPixbufAnimationIter GdkPixbufAnimationIter;
#define GDK_TYPE_PIXBUF_ANIMATION        (gdk_pixbuf_animation_get_type ())
#define GDK_PIXBUF_ANIMATION(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_PIXBUF_ANIMATION, GdkPixbufAnimation))
#define GDK_IS_PIXBUF_ANIMATION(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_PIXBUF_ANIMATION))
#define GDK_TYPE_PIXBUF_ANIMATION_ITER   (gdk_pixbuf_animation_iter_get_type ())
#define GDK_PIXBUF_ANIMATION_ITER(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_PIXBUF_ANIMATION_ITER, GdkPixbufAnimationIter))
#define GDK_IS_PIXBUF_ANIMATION_ITER(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_PIXBUF_ANIMATION_ITER))
GType       gdk_pixbuf_animation_get_type (void) G_GNUC_CONST;
GType       gdk_pixbuf_animation_iter_get_type (void) G_GNUC_CONST;
GdkPixbufAnimation *gdk_pixbuf_animation_new_from_file (const char *filename, GError **error);
GdkPixbufAnimation *gdk_pixbuf_animation_new_from_resource (const char *resource_path, GError **error);
GdkPixbufAnimation *gdk_pixbuf_animation_ref (GdkPixbufAnimation *animation);
void        gdk_pixbuf_animation_unref (GdkPixbufAnimation *animation);
int         gdk_pixbuf_animation_get_width (GdkPixbufAnimation *animation);
int         gdk_pixbuf_animation_get_height (GdkPixbufAnimation *animation);
gboolean    gdk_pixbuf_animation_is_static_image (GdkPixbufAnimation *animation);
GdkPixbuf  *gdk_pixbuf_animation_get_static_image (GdkPixbufAnimation *animation);
/* GTimeVal is deprecated in GLib; gdk-pixbuf's header keeps it too */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
GdkPixbufAnimationIter *gdk_pixbuf_animation_get_iter (GdkPixbufAnimation *animation, const GTimeVal *start_time);
int         gdk_pixbuf_animation_iter_get_delay_time (GdkPixbufAnimationIter *iter);
GdkPixbuf  *gdk_pixbuf_animation_iter_get_pixbuf (GdkPixbufAnimationIter *iter);
gboolean    gdk_pixbuf_animation_iter_on_currently_loading_frame (GdkPixbufAnimationIter *iter);
gboolean    gdk_pixbuf_animation_iter_advance (GdkPixbufAnimationIter *iter, const GTimeVal *current_time);
G_GNUC_END_IGNORE_DEPRECATIONS

/* GdkPixbufLoader: data given in pieces, decoded when complete */
typedef struct _GdkPixbufLoader { GObject parent_instance; gpointer priv; } GdkPixbufLoader;
typedef struct _GdkPixbufLoaderClass {
  GObjectClass parent_class;
  void (*size_prepared) (GdkPixbufLoader *loader, int width, int height);
  void (*area_prepared) (GdkPixbufLoader *loader);
  void (*area_updated) (GdkPixbufLoader *loader, int x, int y, int width, int height);
  void (*closed) (GdkPixbufLoader *loader);
} GdkPixbufLoaderClass;
#define GDK_TYPE_PIXBUF_LOADER        (gdk_pixbuf_loader_get_type ())
#define GDK_PIXBUF_LOADER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_PIXBUF_LOADER, GdkPixbufLoader))
#define GDK_IS_PIXBUF_LOADER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_PIXBUF_LOADER))
GType       gdk_pixbuf_loader_get_type (void) G_GNUC_CONST;
GdkPixbufLoader *gdk_pixbuf_loader_new (void);
GdkPixbufLoader *gdk_pixbuf_loader_new_with_type (const char *image_type, GError **error);
GdkPixbufLoader *gdk_pixbuf_loader_new_with_mime_type (const char *mime_type, GError **error);
void        gdk_pixbuf_loader_set_size (GdkPixbufLoader *loader, int width, int height);
gboolean    gdk_pixbuf_loader_write (GdkPixbufLoader *loader, const guchar *buf, gsize count, GError **error);
gboolean    gdk_pixbuf_loader_write_bytes (GdkPixbufLoader *loader, GBytes *buffer, GError **error);
gboolean    gdk_pixbuf_loader_close (GdkPixbufLoader *loader, GError **error);
GdkPixbuf  *gdk_pixbuf_loader_get_pixbuf (GdkPixbufLoader *loader);
GdkPixbufAnimation *gdk_pixbuf_loader_get_animation (GdkPixbufLoader *loader);
GdkPixbufFormat *gdk_pixbuf_loader_get_format (GdkPixbufLoader *loader);

/* the version gdk-pixbuf reports */
#define GDK_PIXBUF_MAJOR 2
#define GDK_PIXBUF_MINOR 42
#define GDK_PIXBUF_MICRO 10
#define GDK_PIXBUF_VERSION "2.42.10"

G_END_DECLS

#endif
