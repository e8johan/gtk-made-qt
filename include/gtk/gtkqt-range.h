/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 * Adjustments and range widgets.
 */
#ifndef GTKQT_RANGE_H
#define GTKQT_RANGE_H

#include <gtk/gtkqt-controls.h>

G_BEGIN_DECLS

/* GtkAdjustment. The fields are public as in GTK 2; code that writes them
 * directly should then call gtk_adjustment_changed() (or emit "changed"),
 * exactly like with real GTK 2. */
struct _GtkAdjustment {
  GInitiallyUnowned parent_instance;
  gdouble lower;
  gdouble upper;
  gdouble value;
  gdouble step_increment;
  gdouble page_increment;
  gdouble page_size;
};
typedef struct _GtkAdjustmentClass {
  GInitiallyUnownedClass parent_class;
  void (*changed) (GtkAdjustment *adjustment);
  void (*value_changed) (GtkAdjustment *adjustment);
} GtkAdjustmentClass;
#define GTK_TYPE_ADJUSTMENT  (gtk_adjustment_get_type ())
#define GTK_ADJUSTMENT(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_ADJUSTMENT, GtkAdjustment))
#define GTK_IS_ADJUSTMENT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_ADJUSTMENT))
GType      gtk_adjustment_get_type (void) G_GNUC_CONST;
/* GTK 2 returns GtkObject*, GTK 3/4 GtkAdjustment*; both are pointers to the same object. */
GtkAdjustment *gtk_adjustment_new (gdouble value, gdouble lower, gdouble upper, gdouble step_increment,
                                   gdouble page_increment, gdouble page_size);
gdouble    gtk_adjustment_get_value (GtkAdjustment *adjustment);
void       gtk_adjustment_set_value (GtkAdjustment *adjustment, gdouble value);
void       gtk_adjustment_clamp_page (GtkAdjustment *adjustment, gdouble lower, gdouble upper);
void       gtk_adjustment_changed (GtkAdjustment *adjustment);
void       gtk_adjustment_value_changed (GtkAdjustment *adjustment);
void       gtk_adjustment_configure (GtkAdjustment *adjustment, gdouble value, gdouble lower, gdouble upper,
                                     gdouble step_increment, gdouble page_increment, gdouble page_size);
gdouble    gtk_adjustment_get_lower (GtkAdjustment *adjustment);
gdouble    gtk_adjustment_get_upper (GtkAdjustment *adjustment);
gdouble    gtk_adjustment_get_step_increment (GtkAdjustment *adjustment);
gdouble    gtk_adjustment_get_page_increment (GtkAdjustment *adjustment);
gdouble    gtk_adjustment_get_page_size (GtkAdjustment *adjustment);
void       gtk_adjustment_set_lower (GtkAdjustment *adjustment, gdouble lower);
void       gtk_adjustment_set_upper (GtkAdjustment *adjustment, gdouble upper);
void       gtk_adjustment_set_step_increment (GtkAdjustment *adjustment, gdouble step_increment);
void       gtk_adjustment_set_page_increment (GtkAdjustment *adjustment, gdouble page_increment);
void       gtk_adjustment_set_page_size (GtkAdjustment *adjustment, gdouble page_size);

/* GtkRange / GtkScale / GtkScrollbar */
GTKQT_SIMPLE_TYPE (GtkRange, GtkWidget)
GTKQT_SIMPLE_TYPE (GtkScale, GtkRange)
GTKQT_SIMPLE_TYPE (GtkHScale, GtkScale)
GTKQT_SIMPLE_TYPE (GtkVScale, GtkScale)
GTKQT_SIMPLE_TYPE (GtkScrollbar, GtkRange)
GTKQT_SIMPLE_TYPE (GtkHScrollbar, GtkScrollbar)
GTKQT_SIMPLE_TYPE (GtkVScrollbar, GtkScrollbar)
#define GTK_TYPE_RANGE      (gtk_range_get_type ())
#define GTK_RANGE(o)        (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_RANGE, GtkRange))
#define GTK_IS_RANGE(o)     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_RANGE))
#define GTK_TYPE_SCALE      (gtk_scale_get_type ())
#define GTK_SCALE(o)        (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_SCALE, GtkScale))
#define GTK_IS_SCALE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_SCALE))
#define GTK_TYPE_HSCALE     (gtk_hscale_get_type ())
#define GTK_HSCALE(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_HSCALE, GtkHScale))
#define GTK_IS_HSCALE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_HSCALE))
#define GTK_TYPE_VSCALE     (gtk_vscale_get_type ())
#define GTK_VSCALE(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_VSCALE, GtkVScale))
#define GTK_IS_VSCALE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_VSCALE))
#define GTK_TYPE_SCROLLBAR  (gtk_scrollbar_get_type ())
#define GTK_SCROLLBAR(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_SCROLLBAR, GtkScrollbar))
#define GTK_IS_SCROLLBAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_SCROLLBAR))
#define GTK_TYPE_HSCROLLBAR (gtk_hscrollbar_get_type ())
#define GTK_TYPE_VSCROLLBAR (gtk_vscrollbar_get_type ())
#define GTK_IS_HSCROLLBAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_HSCROLLBAR))
#define GTK_IS_VSCROLLBAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_VSCROLLBAR))
GType      gtk_range_get_type (void) G_GNUC_CONST;
GType      gtk_scale_get_type (void) G_GNUC_CONST;
GType      gtk_hscale_get_type (void) G_GNUC_CONST;
GType      gtk_vscale_get_type (void) G_GNUC_CONST;
GType      gtk_scrollbar_get_type (void) G_GNUC_CONST;
GType      gtk_hscrollbar_get_type (void) G_GNUC_CONST;
GType      gtk_vscrollbar_get_type (void) G_GNUC_CONST;
GtkAdjustment *gtk_range_get_adjustment (GtkRange *range);
void       gtk_range_set_adjustment (GtkRange *range, GtkAdjustment *adjustment);
gdouble    gtk_range_get_value (GtkRange *range);
void       gtk_range_set_value (GtkRange *range, gdouble value);
void       gtk_range_set_range (GtkRange *range, gdouble min, gdouble max);
void       gtk_range_set_increments (GtkRange *range, gdouble step, gdouble page);
void       gtk_range_set_inverted (GtkRange *range, gboolean setting);
void       gtk_range_set_update_policy (GtkRange *range, GtkUpdateType policy);   /* GTK 2 */
GtkWidget *gtk_scale_new (GtkOrientation orientation, GtkAdjustment *adjustment);
GtkWidget *gtk_scale_new_with_range (GtkOrientation orientation, gdouble min, gdouble max, gdouble step);
GtkWidget *gtk_hscale_new (GtkAdjustment *adjustment);
GtkWidget *gtk_vscale_new (GtkAdjustment *adjustment);
GtkWidget *gtk_hscale_new_with_range (gdouble min, gdouble max, gdouble step);
GtkWidget *gtk_vscale_new_with_range (gdouble min, gdouble max, gdouble step);
void       gtk_scale_set_digits (GtkScale *scale, gint digits);
gint       gtk_scale_get_digits (GtkScale *scale);
void       gtk_scale_set_draw_value (GtkScale *scale, gboolean draw_value);
gboolean   gtk_scale_get_draw_value (GtkScale *scale);
void       gtk_scale_set_value_pos (GtkScale *scale, GtkPositionType pos);
void       gtk_scale_set_has_origin (GtkScale *scale, gboolean has_origin);
void       gtk_scale_add_mark (GtkScale *scale, gdouble value, GtkPositionType position, const gchar *markup);
void       gtk_scale_clear_marks (GtkScale *scale);
GtkWidget *gtk_scrollbar_new (GtkOrientation orientation, GtkAdjustment *adjustment);
GtkWidget *gtk_hscrollbar_new (GtkAdjustment *adjustment);
GtkWidget *gtk_vscrollbar_new (GtkAdjustment *adjustment);

/* GtkSpinButton */
typedef struct _GtkSpinButton { GtkEntry parent_instance; GtkAdjustment *adjustment; guint digits; gfloat climb_rate; } GtkSpinButton;
typedef struct _GtkSpinButtonClass { GtkEntryClass parent_class; } GtkSpinButtonClass;
#define GTK_TYPE_SPIN_BUTTON (gtk_spin_button_get_type ())
#define GTK_SPIN_BUTTON(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_SPIN_BUTTON, GtkSpinButton))
#define GTK_IS_SPIN_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_SPIN_BUTTON))
GType      gtk_spin_button_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_spin_button_new (GtkAdjustment *adjustment, gdouble climb_rate, guint digits);
GtkWidget *gtk_spin_button_new_with_range (gdouble min, gdouble max, gdouble step);
void       gtk_spin_button_set_adjustment (GtkSpinButton *spin_button, GtkAdjustment *adjustment);
GtkAdjustment *gtk_spin_button_get_adjustment (GtkSpinButton *spin_button);
void       gtk_spin_button_set_digits (GtkSpinButton *spin_button, guint digits);
void       gtk_spin_button_set_increments (GtkSpinButton *spin_button, gdouble step, gdouble page);
void       gtk_spin_button_set_range (GtkSpinButton *spin_button, gdouble min, gdouble max);
gdouble    gtk_spin_button_get_value (GtkSpinButton *spin_button);
gint       gtk_spin_button_get_value_as_int (GtkSpinButton *spin_button);
void       gtk_spin_button_set_value (GtkSpinButton *spin_button, gdouble value);
void       gtk_spin_button_set_numeric (GtkSpinButton *spin_button, gboolean numeric);
void       gtk_spin_button_set_wrap (GtkSpinButton *spin_button, gboolean wrap);
void       gtk_spin_button_set_update_policy (GtkSpinButton *spin_button, GtkSpinButtonUpdatePolicy policy);
void       gtk_spin_button_update (GtkSpinButton *spin_button);

/* GtkProgressBar */
typedef struct _GtkProgressBar {
  GtkWidget parent_instance;
  /* GTK 1 GtkProgress fields */
  GtkAdjustment *adjustment;
  gchar *format;
  gfloat x_align, y_align;
  guint show_text : 1, activity_mode : 1;
} GtkProgressBar;
typedef struct _GtkProgressBarClass { GtkWidgetClass parent_class; } GtkProgressBarClass;
#define GTK_TYPE_PROGRESS_BAR (gtk_progress_bar_get_type ())
#define GTK_PROGRESS_BAR(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_PROGRESS_BAR, GtkProgressBar))
#define GTK_IS_PROGRESS_BAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_PROGRESS_BAR))
typedef enum { GTK_PROGRESS_LEFT_TO_RIGHT, GTK_PROGRESS_RIGHT_TO_LEFT,
               GTK_PROGRESS_BOTTOM_TO_TOP, GTK_PROGRESS_TOP_TO_BOTTOM } GtkProgressBarOrientation;
GType      gtk_progress_bar_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_progress_bar_new (void);
void       gtk_progress_bar_set_fraction (GtkProgressBar *pbar, gdouble fraction);
gdouble    gtk_progress_bar_get_fraction (GtkProgressBar *pbar);
void       gtk_progress_bar_pulse (GtkProgressBar *pbar);
void       gtk_progress_bar_set_pulse_step (GtkProgressBar *pbar, gdouble fraction);
void       gtk_progress_bar_set_text (GtkProgressBar *pbar, const gchar *text);
void       gtk_progress_bar_set_show_text (GtkProgressBar *pbar, gboolean show_text);
void       gtk_progress_bar_set_inverted (GtkProgressBar *pbar, gboolean inverted);
void       gtk_progress_bar_set_orientation (GtkProgressBar *pbar, GtkProgressBarOrientation orientation);

/* GtkLevelBar */
GTKQT_SIMPLE_TYPE (GtkLevelBar, GtkWidget)
#define GTK_TYPE_LEVEL_BAR (gtk_level_bar_get_type ())
#define GTK_LEVEL_BAR(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_LEVEL_BAR, GtkLevelBar))
#define GTK_IS_LEVEL_BAR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_LEVEL_BAR))
GType      gtk_level_bar_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_level_bar_new (void);
GtkWidget *gtk_level_bar_new_for_interval (gdouble min_value, gdouble max_value);
void       gtk_level_bar_set_value (GtkLevelBar *self, gdouble value);
gdouble    gtk_level_bar_get_value (GtkLevelBar *self);

G_END_DECLS

#endif
