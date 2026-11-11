/* GTK 3: cairo drawing with the "draw" signal, mouse input with
 * "button-press-event", and an animation driven by g_timeout_add().
 * Build with real GTK:  gcc drawing.c $(pkg-config --cflags --libs gtk+-3.0) -lm
 */
#include <gtk/gtk.h>
#include <math.h>

static GArray *points;      /* GdkPoint-like pairs of doubles */
static double phase;

static gboolean on_draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  int w = gtk_widget_get_allocated_width (widget), h = gtk_widget_get_allocated_height (widget);
  cairo_pattern_t *bg = cairo_pattern_create_linear (0, 0, 0, h);
  cairo_pattern_add_color_stop_rgb (bg, 0, 0.12, 0.16, 0.30);
  cairo_pattern_add_color_stop_rgb (bg, 1, 0.02, 0.02, 0.08);
  cairo_set_source (cr, bg);
  cairo_paint (cr);
  cairo_pattern_destroy (bg);

  /* a sine wave that moves */
  cairo_set_line_width (cr, 2);
  cairo_set_source_rgb (cr, 0.4, 0.8, 1.0);
  for (int x = 0; x <= w; x += 4) {
    double y = h / 2.0 + sin (x / 30.0 + phase) * h / 5.0;
    if (x == 0) cairo_move_to (cr, x, y); else cairo_line_to (cr, x, y);
  }
  cairo_stroke (cr);

  /* clicked points */
  for (guint i = 0; i < points->len; i += 2) {
    double px = g_array_index (points, double, i), py = g_array_index (points, double, i + 1);
    cairo_arc (cr, px, py, 10, 0, 2 * G_PI);
    cairo_set_source_rgba (cr, 1.0, 0.6, 0.2, 0.85);
    cairo_fill (cr);
  }

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 16);
  cairo_move_to (cr, 12, 24);
  cairo_show_text (cr, "cairo on Qt - click to drop dots");
  (void) data;
  return FALSE;
}

static gboolean on_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  g_array_append_val (points, event->x);
  g_array_append_val (points, event->y);
  gtk_widget_queue_draw (widget);
  (void) data;
  return TRUE;
}

static gboolean tick (gpointer area)
{
  phase += 0.15;
  gtk_widget_queue_draw (GTK_WIDGET (area));
  return G_SOURCE_CONTINUE;
}

int main (int argc, char *argv[])
{
  gtk_init (&argc, &argv);
  points = g_array_new (FALSE, FALSE, sizeof (double));
  double seed[] = { 80, 150, 200, 110, 330, 170 };
  g_array_append_vals (points, seed, G_N_ELEMENTS (seed));

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Drawing");
  gtk_window_set_default_size (GTK_WINDOW (window), 420, 260);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  GtkWidget *area = gtk_drawing_area_new ();
  gtk_widget_add_events (area, GDK_BUTTON_PRESS_MASK);
  g_signal_connect (area, "draw", G_CALLBACK (on_draw), NULL);
  g_signal_connect (area, "button-press-event", G_CALLBACK (on_press), NULL);
  gtk_container_add (GTK_CONTAINER (window), area);
  g_timeout_add (40, tick, area);

  gtk_widget_show_all (window);
  gtk_main ();
  g_array_free (points, TRUE);
  return 0;
}
