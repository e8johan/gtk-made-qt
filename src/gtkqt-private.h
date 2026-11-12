/*
 * GTK made Qt - internal header shared by the implementation files.
 *
 * Design in one paragraph: every GTK widget is a real GObject (so signals,
 * properties, refcounting and type checks behave like GTK). Each GtkWidget
 * owns one Qt object - normally a QWidget, but a QAction for menu items -
 * created lazily by the class's gtkqt_create hook. A small "bridge" QObject
 * (a child of that Qt object) forwards Qt signals and events back into
 * GObject signals; when the GtkWidget is destroyed the bridge is cut loose
 * and the Qt object is deleted from an idle callback, so a widget can
 * safely destroy itself from inside its own signal handler.
 */
#ifndef GTKQT_PRIVATE_H
#define GTKQT_PRIVATE_H

/* GLib first: gio uses identifiers that Qt's keywords would clobber (we
 * build with QT_NO_KEYWORDS anyway). */
#include <gtk/gtk.h>   /* already wrapped in G_BEGIN_DECLS */

#include <QtGlobal>
#include <QApplication>
#include <QWidget>
#include <QPointer>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QIcon>
#include <QAction>
#include <QHash>
#include <QFont>
#include <QImage>
#include <QPixmap>
#include <QKeySequence>
#include <QEvent>

#include <functional>

class QLayout;
class QBoxLayout;

/* ---------- runtime state ---------- */
extern int gtkqt_api;              /* 2, 3 or 4: the API level the program was written for */
void gtkqt_ensure_app (void);      /* create the QApplication if needed */

/* ---------- per-widget private data ---------- */
class GtkQtBridge;

struct _GtkWidgetPrivate {
  QPointer<QObject> obj;           /* QWidget, or QAction for menu items */
  QPointer<QObject> inner;         /* type-specific "interesting" object if different */
  QPointer<QWidget> content;       /* containers: the QWidget whose layout holds the children */
  GtkQtBridge *bridge = nullptr;
  bool creating = false;
  bool created = false;
  bool visible = false;            /* GTK's own notion of visibility */
  bool destroyed = false;
  bool in_destruction = false;
  bool no_show_all = false;
  bool sensitive = true;
  bool hexpand = false, vexpand = false;
  bool hexpand_set = false, vexpand_set = false;
  GtkAlign halign = GTK_ALIGN_FILL, valign = GTK_ALIGN_FILL;
  int margin_start = 0, margin_end = 0, margin_top = 0, margin_bottom = 0;
  int width_request = -1, height_request = -1;
  gint events = 0;

  /* Packing data owned by the parent container */
  gboolean pack_expand = FALSE, pack_fill = TRUE;
  guint pack_padding = 0;
  GtkPackType pack_type = GTK_PACK_START;
  int grid_left = 0, grid_top = 0, grid_width = 1, grid_height = 1;
  GtkAttachOptions xopts = (GtkAttachOptions) (GTK_EXPAND | GTK_FILL);
  GtkAttachOptions yopts = (GtkAttachOptions) (GTK_EXPAND | GTK_FILL);
  QString tab_label;               /* notebook / stack page title */
  QString stack_name;

  GList *children = nullptr;       /* containers: children in order */
  GdkWindow *gdk_window = nullptr;
  QByteArray scratch;              /* storage for returned const gchar* */
  QByteArray scratch2;
  QStringList css_classes;
  QMap<QByteArray, GActionGroup *> action_groups;   /* gtk_widget_insert_action_group */
  GList *controllers = nullptr;    /* GTK 4 event controllers */

  /* drawing (cairo) */
  cairo_surface_t *paint_surface = nullptr;
};

/* ---------- object access ---------- */
QObject   *gtkqt_obj (GtkWidget *widget);         /* creates the Qt side on first use */
QWidget   *gtkqt_qw (GtkWidget *widget);          /* the QWidget, or nullptr */
GtkWidget *gtkqt_from_qobject (QObject *obj);     /* reverse lookup */
template <class T> inline T *gtkqt_q (GtkWidget *w) { return qobject_cast<T *> (gtkqt_obj (w)); }
template <class T> inline T *gtkqt_inner (GtkWidget *w) {
  gtkqt_obj (w);
  return qobject_cast<T *> (w->priv->inner.data ());
}
GtkWidgetPrivate *gtkqt_priv (GtkWidget *widget);
GtkQtBridge *gtkqt_bridge (GtkWidget *widget);

typedef QObject *(*GtkQtCreateFunc) (GtkWidget *widget);
typedef void (*GtkQtPlaceFunc) (GtkContainer *container, GtkWidget *child);

/* ---------- Qt -> GObject signal forwarding ----------
 * Connect a Qt signal so that `fn(widget, args...)` runs while the GtkWidget
 * is alive. The connection dies with the bridge. */
class GtkQtBridge : public QObject {
public:
  explicit GtkQtBridge (GtkWidget *w) : widget (w) {}
  ~GtkQtBridge () override;
  GtkWidget *widget;
  bool eventFilter (QObject *watched, QEvent *event) override;
};

/* Increments while GTK callbacks run from Qt; deferred deletes wait for 0. */
struct GtkQtEmitGuard {
  GtkQtEmitGuard ();
  ~GtkQtEmitGuard ();
};

template <typename Sender, typename Signal, typename F>
void gtkqt_connect (GtkWidget *w, Sender *sender, Signal sig, F fn) {
  GtkQtBridge *b = gtkqt_bridge (w);
  QObject::connect (sender, sig, b, [b, fn] (auto &&...args) {
    if (!b->widget) return;
    GtkQtEmitGuard guard;
    GtkWidget *keep = b->widget;
    g_object_ref (keep);
    fn (keep, args...);
    g_object_unref (keep);
  });
}

/* Generic "connect to any QObject": same as above but without a widget. */
void gtkqt_defer_delete (QObject *obj);

/* ---------- visibility / placement ---------- */
void gtkqt_sync_visible (GtkWidget *widget);      /* push priv->visible to the Qt widget */
void gtkqt_apply_size_request (GtkWidget *widget);
void gtkqt_container_adopt (GtkContainer *container, GtkWidget *child);  /* bookkeeping + place */
void gtkqt_container_release (GtkContainer *container, GtkWidget *child);
void gtkqt_set_place (GtkContainerClass *klass, GtkQtPlaceFunc place, GtkQtPlaceFunc unplace);
void gtkqt_set_create (GtkWidgetClass *klass, GtkQtCreateFunc create);
void gtkqt_toplevel_register (GtkWidget *window);
void gtkqt_layout_insert_widget (QBoxLayout *layout, int index, GtkWidget *child, Qt::Orientation orientation,
                                 gboolean expand, gboolean fill);
Qt::Alignment gtkqt_child_alignment (GtkWidget *child, Qt::Orientation box_orientation, gboolean fill);
int  gtkqt_child_stretch (GtkWidget *child, Qt::Orientation orientation, gboolean expand);
void gtkqt_box_relayout (GtkWidget *box);         /* recompute stretches after hexpand changes */
void gtkqt_bin_place_simple (GtkContainer *container, GtkWidget *child);
void gtkqt_bin_unplace_simple (GtkContainer *container, GtkWidget *child);
QWidget *gtkqt_content (GtkWidget *container);   /* where children go (defaults to the QWidget) */
void gtkqt_window_before_show (GtkWidget *window);
void gtkqt_window_set_menubar_widget (GtkWindow *window, QWidget *menubar);
void gtkqt_window_set_application_internal (GtkWindow *window, GtkApplication *app);

/* ---------- strings ---------- */
QString     gtkqt_str (const gchar *s);                     /* UTF-8 -> QString (NULL -> empty) */
const char *gtkqt_ret (GtkWidget *w, const QString &s);     /* keep a UTF-8 copy alive in w */
QString     gtkqt_mnemonic (const gchar *s);                /* "_File" -> "&File", "__" -> "_" */
QString     gtkqt_markup_to_html (const gchar *markup);     /* Pango markup -> Qt rich text */
QString     gtkqt_stock_label (const gchar *stock_id);      /* "gtk-ok" -> "&OK" (or empty) */
QIcon       gtkqt_icon (const gchar *icon_name);            /* theme icon or stock id, with fallbacks */
int         gtkqt_icon_size_px (GtkIconSize size);
QKeySequence gtkqt_accel_to_keyseq (const gchar *accel);    /* "<Control>q" -> Ctrl+Q */

/* ---------- properties ----------
 * Install a GObject property backed by two plain functions. Used to give the
 * widget classes real properties (g_object_set, GtkBuilder, notify::). */
typedef void (*GtkQtPropSet) (GObject *object, const GValue *value);
typedef void (*GtkQtPropGet) (GObject *object, GValue *value);
void gtkqt_prop (gpointer klass, GParamSpec *pspec, GtkQtPropSet set, GtkQtPropGet get);

/* ---------- events ---------- */
gboolean gtkqt_emit_event (GtkWidget *widget, GdkEvent *event, const char *signal_name);
guint    gtkqt_qt_key_to_keyval (int qt_key, const QString &text, Qt::KeyboardModifiers mods);
guint    gtkqt_qt_mods_to_gdk (Qt::KeyboardModifiers mods, Qt::MouseButtons buttons);
void     gtkqt_controllers_dispatch (GtkWidget *widget, QEvent *event, bool *consumed);

/* ---------- actions (GAction lookup from a widget, "app."/"win."/custom prefixes) ---------- */
gboolean gtkqt_activate_action (GtkWidget *widget, const gchar *detailed_name, GVariant *target);
GActionGroup *gtkqt_find_action_group (GtkWidget *widget, const QByteArray &prefix);
QMenu *gtkqt_menu_from_model (GMenuModel *model, GtkWidget *context, QWidget *parent);
void   gtkqt_menubar_fill_from_model (QWidget *menubar, GMenuModel *model, GtkWidget *context);
QStringList gtkqt_application_accels (GtkApplication *app, const QString &detailed_action);

/* ---------- GDK drawables (widget windows, text view border windows, pixmaps) ---------- */
enum GtkQtDrawableKind { GTKQT_DRAWABLE_WINDOW, GTKQT_DRAWABLE_TEXT_BORDER, GTKQT_DRAWABLE_PIXMAP, GTKQT_DRAWABLE_ROOT };
struct _GdkWindow {
  GObject parent_instance;
  GtkWidget *widget;              /* the owning widget (windows) */
  int kind;                       /* GtkQtDrawableKind */
  int text_window_type;           /* GtkTextWindowType for border windows */
  QWidget *qw;                    /* the QWidget showing it, when not the widget's own */
  cairo_surface_t *surface;       /* pixmap contents, or the surface being painted right now */
  int width, height, depth;       /* pixmaps */
  GdkColor background;
  bool has_background;
  GdkPixmap *back_pixmap;         /* window background pixmap (gdk_window_set_back_pixmap) */
  QImage *image;                  /* GdkImage-free fast path unused; reserved */
  gint event_mask;
};
QObject   *gtkqt_canvas_create (GtkWidget *w);             /* a widget the program draws on (backing store) */
cairo_surface_t *gtkqt_canvas_surface (GtkWidget *w);
QPixmap    gtkqt_qpixmap (GdkPixmap *pixmap, GdkBitmap *mask);      /* its backing image, for drawing outside expose */
GdkWindow *gtkqt_drawable_new (int kind, GtkWidget *widget);
cairo_surface_t *gtkqt_drawable_target (GdkDrawable *d);   /* nullptr when it cannot be drawn on now */
GtkStyle  *gtkqt_default_style_ref (void);
QFont      gtkqt_font_from_pango (const PangoFontDescription *desc);
PangoFontDescription *gtkqt_pango_from_font (const QFont &font);
extern QHash<QString, QByteArray> gtkqt_user_stock_labels;
extern QHash<QString, QIcon> gtkqt_user_icons;
void       gtkqt_accel_group_detach (GtkAccelGroup *group, QWidget *window);
gboolean   gtkqt_accel_group_trigger (GtkAccelGroup *group, const QKeySequence &seq);
QObject   *gtkqt_file_dialog_create (GtkWidget *w);

/* ---------- misc ---------- */
GdkWindow *gtkqt_gdk_window (GtkWidget *widget);
QImage     gtkqt_pixbuf_image (GdkPixbuf *pixbuf);
GdkPixbuf *gtkqt_pixbuf_from_image (const QImage &image);
GtkWidget *gtkqt_text_buffer_any_view (GtkTextBuffer *buffer);
int        gtkqt_dialog_run_loop (GtkDialog *dialog);
GtkAccelGroup *gtkqt_accel_group_dummy (void);
void       gtkqt_accel_group_attach (GtkAccelGroup *group, QWidget *window);
void       gtkqt_accel_group_add_action (GtkAccelGroup *group, QAction *action);

#define GTKQT_WARN_UNSUPPORTED(what) \
  do { static bool once = false; if (!once) { once = true; g_message ("gtk-made-qt: %s is not supported yet", what); } } while (0)

#endif
