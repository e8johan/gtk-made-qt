/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Printing (GTK 2.10+): GtkPrintOperation on QPrinter / QPrintDialog /
 * QPrintPreviewDialog. Pages are drawn with cairo, as in GTK.
 */
#ifndef GTKQT_PRINT_H
#define GTKQT_PRINT_H

#include <gtk/gtkqt-dialogs.h>

G_BEGIN_DECLS

typedef enum { GTK_UNIT_NONE, GTK_UNIT_POINTS, GTK_UNIT_INCH, GTK_UNIT_MM } GtkUnit;
#define GTK_UNIT_PIXEL GTK_UNIT_NONE
typedef enum { GTK_PAGE_ORIENTATION_PORTRAIT, GTK_PAGE_ORIENTATION_LANDSCAPE,
               GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT, GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE } GtkPageOrientation;
typedef enum { GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_PRINT_OPERATION_ACTION_PRINT,
               GTK_PRINT_OPERATION_ACTION_PREVIEW, GTK_PRINT_OPERATION_ACTION_EXPORT } GtkPrintOperationAction;
typedef enum { GTK_PRINT_OPERATION_RESULT_ERROR, GTK_PRINT_OPERATION_RESULT_APPLY,
               GTK_PRINT_OPERATION_RESULT_CANCEL, GTK_PRINT_OPERATION_RESULT_IN_PROGRESS } GtkPrintOperationResult;
typedef enum { GTK_PRINT_STATUS_INITIAL, GTK_PRINT_STATUS_PREPARING, GTK_PRINT_STATUS_GENERATING_DATA,
               GTK_PRINT_STATUS_SENDING_DATA, GTK_PRINT_STATUS_PENDING, GTK_PRINT_STATUS_PENDING_ISSUE,
               GTK_PRINT_STATUS_PRINTING, GTK_PRINT_STATUS_FINISHED, GTK_PRINT_STATUS_FINISHED_ABORTED } GtkPrintStatus;
#define GTK_PRINT_ERROR gtk_print_error_quark ()
GQuark gtk_print_error_quark (void);

typedef struct _GtkPrintOperation GtkPrintOperation;
typedef struct _GtkPrintContext GtkPrintContext;
typedef struct _GtkPrintSettings GtkPrintSettings;
typedef struct _GtkPageSetup GtkPageSetup;
typedef struct _GtkPaperSize GtkPaperSize;

#define GTK_TYPE_PRINT_OPERATION  (gtk_print_operation_get_type ())
#define GTK_PRINT_OPERATION(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_PRINT_OPERATION, GtkPrintOperation))
#define GTK_IS_PRINT_OPERATION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_PRINT_OPERATION))
#define GTK_TYPE_PRINT_CONTEXT    (gtk_print_context_get_type ())
#define GTK_TYPE_PRINT_SETTINGS   (gtk_print_settings_get_type ())
#define GTK_PRINT_SETTINGS(o)     ((GtkPrintSettings *) (o))
#define GTK_TYPE_PAGE_SETUP       (gtk_page_setup_get_type ())
#define GTK_PAGE_SETUP(o)         ((GtkPageSetup *) (o))
GType gtk_print_operation_get_type (void) G_GNUC_CONST;
GType gtk_print_context_get_type (void) G_GNUC_CONST;
GType gtk_print_settings_get_type (void) G_GNUC_CONST;
GType gtk_page_setup_get_type (void) G_GNUC_CONST;

GtkPrintOperation *gtk_print_operation_new (void);
void   gtk_print_operation_set_default_page_setup (GtkPrintOperation *op, GtkPageSetup *default_page_setup);
GtkPageSetup *gtk_print_operation_get_default_page_setup (GtkPrintOperation *op);
void   gtk_print_operation_set_print_settings (GtkPrintOperation *op, GtkPrintSettings *print_settings);
GtkPrintSettings *gtk_print_operation_get_print_settings (GtkPrintOperation *op);
void   gtk_print_operation_set_job_name (GtkPrintOperation *op, const gchar *job_name);
void   gtk_print_operation_set_n_pages (GtkPrintOperation *op, gint n_pages);
void   gtk_print_operation_set_current_page (GtkPrintOperation *op, gint current_page);
void   gtk_print_operation_set_use_full_page (GtkPrintOperation *op, gboolean full_page);
void   gtk_print_operation_set_unit (GtkPrintOperation *op, GtkUnit unit);
void   gtk_print_operation_set_export_filename (GtkPrintOperation *op, const gchar *filename);
void   gtk_print_operation_set_show_progress (GtkPrintOperation *op, gboolean show_progress);
void   gtk_print_operation_set_allow_async (GtkPrintOperation *op, gboolean allow_async);
void   gtk_print_operation_set_embed_page_setup (GtkPrintOperation *op, gboolean embed);
GtkPrintOperationResult gtk_print_operation_run (GtkPrintOperation *op, GtkPrintOperationAction action,
                                                 GtkWindow *parent, GError **error);
void   gtk_print_operation_get_error (GtkPrintOperation *op, GError **error);
GtkPrintStatus gtk_print_operation_get_status (GtkPrintOperation *op);
gboolean gtk_print_operation_is_finished (GtkPrintOperation *op);
void   gtk_print_operation_cancel (GtkPrintOperation *op);

cairo_t      *gtk_print_context_get_cairo_context (GtkPrintContext *context);
GtkPageSetup *gtk_print_context_get_page_setup (GtkPrintContext *context);
gdouble       gtk_print_context_get_width (GtkPrintContext *context);
gdouble       gtk_print_context_get_height (GtkPrintContext *context);
gdouble       gtk_print_context_get_dpi_x (GtkPrintContext *context);
gdouble       gtk_print_context_get_dpi_y (GtkPrintContext *context);
PangoFontMap *gtk_print_context_get_pango_fontmap (GtkPrintContext *context);
PangoContext *gtk_print_context_create_pango_context (GtkPrintContext *context);
PangoLayout  *gtk_print_context_create_pango_layout (GtkPrintContext *context);
void          gtk_print_context_set_cairo_context (GtkPrintContext *context, cairo_t *cr, double dpi_x, double dpi_y);

GtkPrintSettings *gtk_print_settings_new (void);
GtkPrintSettings *gtk_print_settings_copy (GtkPrintSettings *other);
const gchar *gtk_print_settings_get (GtkPrintSettings *settings, const gchar *key);
void         gtk_print_settings_set (GtkPrintSettings *settings, const gchar *key, const gchar *value);

GtkPageSetup *gtk_page_setup_new (void);
GtkPageSetup *gtk_page_setup_copy (GtkPageSetup *other);
void    gtk_page_setup_set_orientation (GtkPageSetup *setup, GtkPageOrientation orientation);
GtkPageOrientation gtk_page_setup_get_orientation (GtkPageSetup *setup);
void    gtk_page_setup_set_top_margin (GtkPageSetup *setup, gdouble margin, GtkUnit unit);
void    gtk_page_setup_set_bottom_margin (GtkPageSetup *setup, gdouble margin, GtkUnit unit);
void    gtk_page_setup_set_left_margin (GtkPageSetup *setup, gdouble margin, GtkUnit unit);
void    gtk_page_setup_set_right_margin (GtkPageSetup *setup, gdouble margin, GtkUnit unit);
gdouble gtk_page_setup_get_top_margin (GtkPageSetup *setup, GtkUnit unit);
gdouble gtk_page_setup_get_bottom_margin (GtkPageSetup *setup, GtkUnit unit);
gdouble gtk_page_setup_get_left_margin (GtkPageSetup *setup, GtkUnit unit);
gdouble gtk_page_setup_get_right_margin (GtkPageSetup *setup, GtkUnit unit);
gdouble gtk_page_setup_get_paper_width (GtkPageSetup *setup, GtkUnit unit);
gdouble gtk_page_setup_get_paper_height (GtkPageSetup *setup, GtkUnit unit);
gdouble gtk_page_setup_get_page_width (GtkPageSetup *setup, GtkUnit unit);
gdouble gtk_page_setup_get_page_height (GtkPageSetup *setup, GtkUnit unit);
GtkPageSetup *gtk_print_run_page_setup_dialog (GtkWindow *parent, GtkPageSetup *page_setup, GtkPrintSettings *settings);

G_END_DECLS

#endif
