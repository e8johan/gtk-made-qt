/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Printing: GtkPrintOperation on QPrinter. The program draws each page with
 * cairo in points (as with GTK's PDF/PostScript backends); the page is
 * rasterised at the printer's resolution (at most 600 dpi) and handed to Qt,
 * which prints it or writes a PDF. Dialogs: QPrintDialog, QPrintPreviewDialog,
 * QPageSetupDialog.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>

#include <cmath>

static double to_mm (double v, GtkUnit unit)
{
  switch (unit) {
  case GTK_UNIT_INCH: return v * 25.4;
  case GTK_UNIT_POINTS: case GTK_UNIT_NONE: return v * 25.4 / 72.0;
  default: return v;
  }
}
static double from_mm (double mm, GtkUnit unit)
{
  switch (unit) {
  case GTK_UNIT_INCH: return mm / 25.4;
  case GTK_UNIT_POINTS: case GTK_UNIT_NONE: return mm * 72.0 / 25.4;
  default: return mm;
  }
}

/* ---------- GtkPageSetup ---------- */
struct _GtkPageSetup {
  GObject parent_instance;
  double top, bottom, left, right;     /* mm */
  double paper_w, paper_h;             /* mm, portrait */
  GtkPageOrientation orientation;
};
typedef struct { GObjectClass parent_class; } GtkPageSetupClass;
G_DEFINE_TYPE (GtkPageSetup, gtk_page_setup, G_TYPE_OBJECT)
static void gtk_page_setup_init (GtkPageSetup *s)
{
  QPageSize def = QPageSize (QLocale::system ().measurementSystem () == QLocale::ImperialUSSystem ? QPageSize::Letter : QPageSize::A4);
  QSizeF mm = def.size (QPageSize::Millimeter);
  s->paper_w = mm.width ();
  s->paper_h = mm.height ();
  s->top = s->bottom = s->left = s->right = 6.35;   /* GTK's default (1/4 inch) */
}
static void gtk_page_setup_class_init (GtkPageSetupClass *) {}

/* ---------- GtkPrintSettings ---------- */
struct _GtkPrintSettings { GObject parent_instance; QHash<QByteArray, QByteArray> *values; };
typedef struct { GObjectClass parent_class; } GtkPrintSettingsClass;
G_DEFINE_TYPE (GtkPrintSettings, gtk_print_settings, G_TYPE_OBJECT)
static void gtk_print_settings_init (GtkPrintSettings *s) { s->values = new QHash<QByteArray, QByteArray>; }
static void print_settings_finalize (GObject *o)
{
  delete ((GtkPrintSettings *) o)->values;
  G_OBJECT_CLASS (gtk_print_settings_parent_class)->finalize (o);
}
static void gtk_print_settings_class_init (GtkPrintSettingsClass *klass) { G_OBJECT_CLASS (klass)->finalize = print_settings_finalize; }

/* ---------- GtkPrintContext ---------- */
struct _GtkPrintContext {
  GObject parent_instance;
  cairo_t *cr;
  GtkPageSetup *setup;
  double width, height;   /* printable area in points */
  double dpi_x, dpi_y;
};
typedef struct { GObjectClass parent_class; } GtkPrintContextClass;
G_DEFINE_TYPE (GtkPrintContext, gtk_print_context, G_TYPE_OBJECT)
static void gtk_print_context_init (GtkPrintContext *) {}
static void print_context_finalize (GObject *o)
{
  auto *c = (GtkPrintContext *) o;
  if (c->cr) cairo_destroy (c->cr);
  if (c->setup) g_object_unref (c->setup);
  G_OBJECT_CLASS (gtk_print_context_parent_class)->finalize (o);
}
static void gtk_print_context_class_init (GtkPrintContextClass *klass) { G_OBJECT_CLASS (klass)->finalize = print_context_finalize; }

/* ---------- GtkPrintOperation ---------- */
struct _GtkPrintOperation {
  GObject parent_instance;
  GtkPageSetup *default_setup;
  GtkPrintSettings *settings;
  gchar *job_name, *export_filename;
  gint n_pages, current_page;
  gboolean full_page;
  GtkUnit unit;
  GtkPrintStatus status;
  GError *error;
  bool cancelled;
};
typedef struct { GObjectClass parent_class; } GtkPrintOperationClass;
G_DEFINE_TYPE (GtkPrintOperation, gtk_print_operation, G_TYPE_OBJECT)
static void gtk_print_operation_init (GtkPrintOperation *op) { op->n_pages = -1; op->current_page = -1; op->unit = GTK_UNIT_NONE; }
static void print_operation_finalize (GObject *o)
{
  auto *op = (GtkPrintOperation *) o;
  if (op->default_setup) g_object_unref (op->default_setup);
  if (op->settings) g_object_unref (op->settings);
  g_free (op->job_name);
  g_free (op->export_filename);
  g_clear_error (&op->error);
  G_OBJECT_CLASS (gtk_print_operation_parent_class)->finalize (o);
}
static void gtk_print_operation_class_init (GtkPrintOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = print_operation_finalize;
  GType t = G_TYPE_FROM_CLASS (klass);
  g_signal_new ("begin-print", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, gtk_print_context_get_type ());
  g_signal_new ("paginate", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_BOOLEAN, 1, gtk_print_context_get_type ());
  g_signal_new ("request-page-setup", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 3,
                gtk_print_context_get_type (), G_TYPE_INT, gtk_page_setup_get_type ());
  g_signal_new ("draw-page", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 2, gtk_print_context_get_type (), G_TYPE_INT);
  g_signal_new ("end-print", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, gtk_print_context_get_type ());
  g_signal_new ("done", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 1, G_TYPE_INT);
  g_signal_new ("status-changed", t, G_SIGNAL_RUN_LAST, 0, nullptr, nullptr, nullptr, G_TYPE_NONE, 0);
}

static void set_status (GtkPrintOperation *op, GtkPrintStatus s)
{
  op->status = s;
  g_signal_emit_by_name (op, "status-changed");
}

static GtkPageSetup *op_setup (GtkPrintOperation *op)
{
  if (!op->default_setup) op->default_setup = gtk_page_setup_new ();
  return op->default_setup;
}

/* QPrinter page layout from a GtkPageSetup */
static void apply_setup (QPrinter *printer, GtkPageSetup *s)
{
  printer->setPageSize (QPageSize (QSizeF (s->paper_w, s->paper_h), QPageSize::Millimeter));
  printer->setPageOrientation (s->orientation == GTK_PAGE_ORIENTATION_LANDSCAPE || s->orientation == GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE
                               ? QPageLayout::Landscape : QPageLayout::Portrait);
  printer->setFullPage (true);   /* margins are applied when drawing, as GTK does */
}

/* Renders all pages onto the printer. Returns false when cancelled. */
static bool render (GtkPrintOperation *op, QPrinter *printer)
{
  GtkPageSetup *setup = op_setup (op);
  bool landscape = setup->orientation == GTK_PAGE_ORIENTATION_LANDSCAPE || setup->orientation == GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE;
  double paper_w = landscape ? setup->paper_h : setup->paper_w, paper_h = landscape ? setup->paper_w : setup->paper_h;
  double ml = landscape ? setup->top : setup->left, mt = landscape ? setup->right : setup->top;
  double mr = landscape ? setup->bottom : setup->right, mb = landscape ? setup->left : setup->bottom;
  double pt = 72.0 / 25.4;
  double dpi = qBound (72, printer->resolution (), 600);

  auto *ctx = (GtkPrintContext *) g_object_new (gtk_print_context_get_type (), nullptr);
  ctx->setup = (GtkPageSetup *) g_object_ref (setup);
  ctx->dpi_x = ctx->dpi_y = 72;
  ctx->width = op->full_page ? paper_w * pt : (paper_w - ml - mr) * pt;
  ctx->height = op->full_page ? paper_h * pt : (paper_h - mt - mb) * pt;
  int img_w = (int) std::lround (paper_w / 25.4 * dpi), img_h = (int) std::lround (paper_h / 25.4 * dpi);
  /* a cairo context exists during begin-print too (programs measure text with it) */
  cairo_surface_t *probe = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);
  ctx->cr = cairo_create (probe);
  cairo_surface_destroy (probe);

  set_status (op, GTK_PRINT_STATUS_PREPARING);
  g_signal_emit_by_name (op, "begin-print", ctx);
  if (op->n_pages < 0) {
    gboolean done = FALSE;
    for (int guard = 0; !done && guard < 10000; guard++) {
      if (!g_signal_has_handler_pending (op, g_signal_lookup ("paginate", G_OBJECT_TYPE (op)), 0, FALSE)) break;
      g_signal_emit_by_name (op, "paginate", ctx, &done);
    }
  }
  int n = op->n_pages < 0 ? 1 : op->n_pages;
  int from = 0, to = n - 1;
  if (printer->fromPage () > 0) { from = qBound (0, printer->fromPage () - 1, n - 1); to = qBound (from, printer->toPage () - 1, n - 1); }
  if (op->current_page >= 0 && printer->printRange () == QPrinter::CurrentPage) from = to = qBound (0, op->current_page, n - 1);

  set_status (op, GTK_PRINT_STATUS_GENERATING_DATA);
  QPainter painter;
  bool ok = n > 0 && painter.begin (printer);
  QRectF target = printer->pageLayout ().fullRectPixels (printer->resolution ());
  for (int page = from; ok && page <= to && !op->cancelled; page++) {
    QImage img (img_w, img_h, QImage::Format_RGB32);
    img.fill (Qt::white);
    cairo_surface_t *s = cairo_image_surface_create_for_data (img.bits (), CAIRO_FORMAT_RGB24, img.width (), img.height (), img.bytesPerLine ());
    cairo_t *cr = cairo_create (s);
    cairo_scale (cr, dpi / 72.0, dpi / 72.0);
    if (!op->full_page) cairo_translate (cr, ml * pt, mt * pt);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_destroy (ctx->cr);
    ctx->cr = cr;
    g_signal_emit_by_name (op, "draw-page", ctx, page);
    cairo_surface_flush (s);
    if (page > from) printer->newPage ();
    painter.drawImage (QRectF (0, 0, target.width (), target.height ()), img);
    ctx->cr = cairo_reference (cr);
    cairo_destroy (cr);
    cairo_surface_destroy (s);
  }
  if (painter.isActive ()) painter.end ();
  g_signal_emit_by_name (op, "end-print", ctx);
  g_object_unref (ctx);
  set_status (op, op->cancelled ? GTK_PRINT_STATUS_FINISHED_ABORTED : GTK_PRINT_STATUS_FINISHED);
  return !op->cancelled;
}

extern "C" {

GQuark gtk_print_error_quark (void) { return g_quark_from_static_string ("gtk-print-error-quark"); }

GtkPageSetup *gtk_page_setup_new (void) { return (GtkPageSetup *) g_object_new (gtk_page_setup_get_type (), nullptr); }
GtkPageSetup *gtk_page_setup_copy (GtkPageSetup *o)
{
  GtkPageSetup *s = gtk_page_setup_new ();
  s->top = o->top; s->bottom = o->bottom; s->left = o->left; s->right = o->right;
  s->paper_w = o->paper_w; s->paper_h = o->paper_h; s->orientation = o->orientation;
  return s;
}
void gtk_page_setup_set_orientation (GtkPageSetup *s, GtkPageOrientation o) { s->orientation = o; }
GtkPageOrientation gtk_page_setup_get_orientation (GtkPageSetup *s) { return s->orientation; }
void gtk_page_setup_set_top_margin (GtkPageSetup *s, gdouble m, GtkUnit u) { s->top = to_mm (m, u); }
void gtk_page_setup_set_bottom_margin (GtkPageSetup *s, gdouble m, GtkUnit u) { s->bottom = to_mm (m, u); }
void gtk_page_setup_set_left_margin (GtkPageSetup *s, gdouble m, GtkUnit u) { s->left = to_mm (m, u); }
void gtk_page_setup_set_right_margin (GtkPageSetup *s, gdouble m, GtkUnit u) { s->right = to_mm (m, u); }
gdouble gtk_page_setup_get_top_margin (GtkPageSetup *s, GtkUnit u) { return from_mm (s->top, u); }
gdouble gtk_page_setup_get_bottom_margin (GtkPageSetup *s, GtkUnit u) { return from_mm (s->bottom, u); }
gdouble gtk_page_setup_get_left_margin (GtkPageSetup *s, GtkUnit u) { return from_mm (s->left, u); }
gdouble gtk_page_setup_get_right_margin (GtkPageSetup *s, GtkUnit u) { return from_mm (s->right, u); }
static bool setup_landscape (GtkPageSetup *s) { return s->orientation == GTK_PAGE_ORIENTATION_LANDSCAPE || s->orientation == GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE; }
gdouble gtk_page_setup_get_paper_width (GtkPageSetup *s, GtkUnit u) { return from_mm (setup_landscape (s) ? s->paper_h : s->paper_w, u); }
gdouble gtk_page_setup_get_paper_height (GtkPageSetup *s, GtkUnit u) { return from_mm (setup_landscape (s) ? s->paper_w : s->paper_h, u); }
gdouble gtk_page_setup_get_page_width (GtkPageSetup *s, GtkUnit u)
{
  return from_mm ((setup_landscape (s) ? s->paper_h : s->paper_w) - s->left - s->right, u);
}
gdouble gtk_page_setup_get_page_height (GtkPageSetup *s, GtkUnit u)
{
  return from_mm ((setup_landscape (s) ? s->paper_w : s->paper_h) - s->top - s->bottom, u);
}

GtkPageSetup *gtk_print_run_page_setup_dialog (GtkWindow *parent, GtkPageSetup *page_setup, GtkPrintSettings *)
{
  gtkqt_ensure_app ();
  GtkPageSetup *s = page_setup ? gtk_page_setup_copy (page_setup) : gtk_page_setup_new ();
  QPrinter printer;
  apply_setup (&printer, s);
  printer.setFullPage (false);
  printer.setPageMargins (QMarginsF (s->left, s->top, s->right, s->bottom), QPageLayout::Millimeter);
  QPageSetupDialog dlg (&printer, parent ? gtkqt_qw (GTK_WIDGET (parent)) : nullptr);
  if (dlg.exec () == QDialog::Accepted) {
    QPageLayout l = printer.pageLayout ();
    QSizeF mm = l.pageSize ().size (QPageSize::Millimeter);
    s->paper_w = mm.width (); s->paper_h = mm.height ();
    s->orientation = l.orientation () == QPageLayout::Landscape ? GTK_PAGE_ORIENTATION_LANDSCAPE : GTK_PAGE_ORIENTATION_PORTRAIT;
    QMarginsF m = l.margins (QPageLayout::Millimeter);
    s->left = m.left (); s->top = m.top (); s->right = m.right (); s->bottom = m.bottom ();
  }
  return s;
}

GtkPrintSettings *gtk_print_settings_new (void) { return (GtkPrintSettings *) g_object_new (gtk_print_settings_get_type (), nullptr); }
GtkPrintSettings *gtk_print_settings_copy (GtkPrintSettings *o)
{
  GtkPrintSettings *s = gtk_print_settings_new ();
  *s->values = *o->values;
  return s;
}
const gchar *gtk_print_settings_get (GtkPrintSettings *s, const gchar *key)
{
  auto it = s->values->constFind (QByteArray (key));
  return it == s->values->constEnd () ? nullptr : it->constData ();
}
void gtk_print_settings_set (GtkPrintSettings *s, const gchar *key, const gchar *value)
{
  if (value) s->values->insert (QByteArray (key), QByteArray (value));
  else s->values->remove (QByteArray (key));
}

cairo_t *gtk_print_context_get_cairo_context (GtkPrintContext *c) { return c->cr; }
GtkPageSetup *gtk_print_context_get_page_setup (GtkPrintContext *c) { return c->setup; }
gdouble gtk_print_context_get_width (GtkPrintContext *c) { return c->width; }
gdouble gtk_print_context_get_height (GtkPrintContext *c) { return c->height; }
gdouble gtk_print_context_get_dpi_x (GtkPrintContext *c) { return c->dpi_x; }
gdouble gtk_print_context_get_dpi_y (GtkPrintContext *c) { return c->dpi_y; }
PangoFontMap *gtk_print_context_get_pango_fontmap (GtkPrintContext *) { return pango_cairo_font_map_get_default (); }
PangoContext *gtk_print_context_create_pango_context (GtkPrintContext *c)
{
  PangoContext *p = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  cairo_font_options_t *fo = cairo_font_options_create ();
  cairo_font_options_set_hint_metrics (fo, CAIRO_HINT_METRICS_OFF);
  cairo_font_options_set_hint_style (fo, CAIRO_HINT_STYLE_NONE);
  pango_cairo_context_set_font_options (p, fo);
  cairo_font_options_destroy (fo);
  pango_cairo_context_set_resolution (p, c->dpi_y);
  return p;
}
PangoLayout *gtk_print_context_create_pango_layout (GtkPrintContext *c)
{
  PangoContext *p = gtk_print_context_create_pango_context (c);
  PangoLayout *l = pango_layout_new (p);
  g_object_unref (p);
  return l;
}
void gtk_print_context_set_cairo_context (GtkPrintContext *c, cairo_t *cr, double dpi_x, double dpi_y)
{
  if (c->cr) cairo_destroy (c->cr);
  c->cr = cr ? cairo_reference (cr) : nullptr;
  c->dpi_x = dpi_x;
  c->dpi_y = dpi_y;
}

GtkPrintOperation *gtk_print_operation_new (void) { return (GtkPrintOperation *) g_object_new (gtk_print_operation_get_type (), nullptr); }
void gtk_print_operation_set_default_page_setup (GtkPrintOperation *op, GtkPageSetup *s)
{
  if (s) g_object_ref (s);
  if (op->default_setup) g_object_unref (op->default_setup);
  op->default_setup = s;
}
GtkPageSetup *gtk_print_operation_get_default_page_setup (GtkPrintOperation *op) { return op->default_setup; }
void gtk_print_operation_set_print_settings (GtkPrintOperation *op, GtkPrintSettings *s)
{
  if (s) g_object_ref (s);
  if (op->settings) g_object_unref (op->settings);
  op->settings = s;
}
GtkPrintSettings *gtk_print_operation_get_print_settings (GtkPrintOperation *op)
{
  if (!op->settings) op->settings = gtk_print_settings_new ();
  return op->settings;
}
void gtk_print_operation_set_job_name (GtkPrintOperation *op, const gchar *name) { g_free (op->job_name); op->job_name = g_strdup (name); }
void gtk_print_operation_set_n_pages (GtkPrintOperation *op, gint n) { op->n_pages = n; }
void gtk_print_operation_set_current_page (GtkPrintOperation *op, gint p) { op->current_page = p; }
void gtk_print_operation_set_use_full_page (GtkPrintOperation *op, gboolean f) { op->full_page = f; }
void gtk_print_operation_set_unit (GtkPrintOperation *op, GtkUnit u) { op->unit = u; }
void gtk_print_operation_set_export_filename (GtkPrintOperation *op, const gchar *f) { g_free (op->export_filename); op->export_filename = g_strdup (f); }
void gtk_print_operation_set_show_progress (GtkPrintOperation *, gboolean) {}
void gtk_print_operation_set_allow_async (GtkPrintOperation *, gboolean) {}
void gtk_print_operation_set_embed_page_setup (GtkPrintOperation *, gboolean) {}
GtkPrintStatus gtk_print_operation_get_status (GtkPrintOperation *op) { return op->status; }
gboolean gtk_print_operation_is_finished (GtkPrintOperation *op)
{
  return op->status == GTK_PRINT_STATUS_FINISHED || op->status == GTK_PRINT_STATUS_FINISHED_ABORTED;
}
void gtk_print_operation_cancel (GtkPrintOperation *op) { op->cancelled = true; }
void gtk_print_operation_get_error (GtkPrintOperation *op, GError **error) { if (error && op->error) *error = g_error_copy (op->error); }

GtkPrintOperationResult gtk_print_operation_run (GtkPrintOperation *op, GtkPrintOperationAction action, GtkWindow *parent, GError **error)
{
  gtkqt_ensure_app ();
  QWidget *pw = parent ? gtkqt_qw (GTK_WIDGET (parent)) : nullptr;
  QPrinter printer (QPrinter::HighResolution);
  apply_setup (&printer, op_setup (op));
  if (op->job_name) printer.setDocName (QString::fromUtf8 (op->job_name));
  GtkPrintSettings *settings = gtk_print_operation_get_print_settings (op);
  if (const char *uri = gtk_print_settings_get (settings, "output-uri")) {
    gchar *file = g_filename_from_uri (uri, nullptr, nullptr);
    if (file) { printer.setOutputFileName (QFile::decodeName (file)); g_free (file); }
  }
  if (const char *name = gtk_print_settings_get (settings, "printer")) printer.setPrinterName (QString::fromUtf8 (name));
  /* test hook: GTKQT_PRINT_TO=<file.pdf> prints to that file without dialogs */
  QByteArray print_to = qgetenv ("GTKQT_PRINT_TO");
  if (!print_to.isEmpty ()) action = GTK_PRINT_OPERATION_ACTION_EXPORT;
  if (action == GTK_PRINT_OPERATION_ACTION_EXPORT) {
    QString out = !print_to.isEmpty () ? QString::fromLocal8Bit (print_to) : QFile::decodeName (op->export_filename ? op->export_filename : "output.pdf");
    printer.setOutputFormat (QPrinter::PdfFormat);
    printer.setOutputFileName (out);
  }

  bool ok = true;
  op->cancelled = false;
  if (action == GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG) {
    QPrintDialog dlg (&printer, pw);
    dlg.setOption (QAbstractPrintDialog::PrintCurrentPage, op->current_page >= 0);
    if (dlg.exec () != QDialog::Accepted) {
      g_signal_emit_by_name (op, "done", (int) GTK_PRINT_OPERATION_RESULT_CANCEL);
      return GTK_PRINT_OPERATION_RESULT_CANCEL;
    }
    ok = render (op, &printer);
  } else if (action == GTK_PRINT_OPERATION_ACTION_PREVIEW) {
    QPrintPreviewDialog dlg (&printer, pw);
    dlg.resize (800, 900);
    QObject::connect (&dlg, &QPrintPreviewDialog::paintRequested, &dlg, [op] (QPrinter *p) {
      op->n_pages = -1;
      render (op, p);
    });
    dlg.exec ();
  } else {
    ok = render (op, &printer);
  }
  if (!ok && op->cancelled) {
    g_signal_emit_by_name (op, "done", (int) GTK_PRINT_OPERATION_RESULT_CANCEL);
    return GTK_PRINT_OPERATION_RESULT_CANCEL;
  }
  if (printer.printerState () == QPrinter::Error) {
    g_set_error (&op->error, GTK_PRINT_ERROR, 0, "Printing failed");
    if (error) *error = g_error_copy (op->error);
    g_signal_emit_by_name (op, "done", (int) GTK_PRINT_OPERATION_RESULT_ERROR);
    return GTK_PRINT_OPERATION_RESULT_ERROR;
  }
  if (!printer.outputFileName ().isEmpty ())
    gtk_print_settings_set (settings, "output-uri", QUrl::fromLocalFile (printer.outputFileName ()).toEncoded ().constData ());
  if (!printer.printerName ().isEmpty ()) gtk_print_settings_set (settings, "printer", printer.printerName ().toUtf8 ().constData ());
  g_signal_emit_by_name (op, "done", (int) GTK_PRINT_OPERATION_RESULT_APPLY);
  return GTK_PRINT_OPERATION_RESULT_APPLY;
}

} /* extern "C" */
