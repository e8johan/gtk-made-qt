/* GTK 4: a drawing area with a draw function, a click gesture, a drag
 * gesture and a key controller. A tiny sketch pad: drag to draw, click to
 * stamp, press C to clear.
 * Build with real GTK:  gcc drawing.c $(pkg-config --cflags --libs gtk4) -lm
 */
#include <gtk/gtk.h>
#include <math.h>

typedef struct { double x, y; gboolean move; } Pt;
static GArray *path;
static double drag_x, drag_y;

static void draw (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  cairo_set_source_rgb (cr, 1, 1, 0.94);
  cairo_paint (cr);
  cairo_set_source_rgb (cr, 0.85, 0.85, 0.8);
  for (int x = 0; x < width; x += 20) { cairo_move_to (cr, x + 0.5, 0); cairo_line_to (cr, x + 0.5, height); }
  for (int y = 0; y < height; y += 20) { cairo_move_to (cr, 0, y + 0.5); cairo_line_to (cr, width, y + 0.5); }
  cairo_set_line_width (cr, 1);
  cairo_stroke (cr);

  cairo_set_source_rgb (cr, 0.1, 0.3, 0.7);
  cairo_set_line_width (cr, 3);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
  for (guint i = 0; i < path->len; i++) {
    Pt *p = &g_array_index (path, Pt, i);
    if (p->move) cairo_move_to (cr, p->x, p->y); else cairo_line_to (cr, p->x, p->y);
  }
  cairo_stroke (cr);
  (void) area; (void) data;
}

static void add_point (double x, double y, gboolean move)
{
  Pt p = { x, y, move };
  g_array_append_val (path, p);
}

static void drag_begin (GtkGestureDrag *g, double x, double y, gpointer area)
{
  drag_x = x; drag_y = y;
  add_point (x, y, TRUE);
  (void) g; (void) area;
}

static void drag_update (GtkGestureDrag *g, double dx, double dy, gpointer area)
{
  add_point (drag_x + dx, drag_y + dy, FALSE);
  gtk_widget_queue_draw (GTK_WIDGET (area));
  (void) g;
}

static void clicked (GtkGestureClick *g, int n_press, double x, double y, gpointer area)
{
  /* stamp a little star */
  for (int i = 0; i <= 10; i++) {
    double a = i * G_PI / 5 - G_PI / 2, r = i % 2 ? 6 : 15;
    add_point (x + r * cos (a), y + r * sin (a), i == 0);
  }
  gtk_widget_queue_draw (GTK_WIDGET (area));
  (void) g; (void) n_press;
}

static gboolean key_pressed (GtkEventControllerKey *c, guint keyval, guint keycode, GdkModifierType state, gpointer area)
{
  if (keyval == GDK_KEY_c || keyval == GDK_KEY_C) {
    g_array_set_size (path, 0);
    gtk_widget_queue_draw (GTK_WIDGET (area));
    return TRUE;
  }
  (void) c; (void) keycode; (void) state;
  return FALSE;
}

static void activate (GtkApplication *app, gpointer data)
{
  GtkWidget *win = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (win), "Sketch (GTK 4 controllers)");
  gtk_window_set_default_size (GTK_WINDOW (win), 400, 280);

  GtkWidget *area = gtk_drawing_area_new ();
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw, NULL, NULL);
  gtk_widget_set_focusable (area, TRUE);

  GtkGesture *drag = gtk_gesture_drag_new ();
  g_signal_connect (drag, "drag-begin", G_CALLBACK (drag_begin), area);
  g_signal_connect (drag, "drag-update", G_CALLBACK (drag_update), area);
  gtk_widget_add_controller (area, GTK_EVENT_CONTROLLER (drag));

  GtkGesture *click = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), GDK_BUTTON_SECONDARY);
  g_signal_connect (click, "pressed", G_CALLBACK (clicked), area);
  gtk_widget_add_controller (area, GTK_EVENT_CONTROLLER (click));

  GtkEventController *keys = gtk_event_controller_key_new ();
  g_signal_connect (keys, "key-pressed", G_CALLBACK (key_pressed), area);
  gtk_widget_add_controller (win, keys);

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *hint = gtk_label_new ("Drag to draw - right-click to stamp - C clears");
  gtk_widget_add_css_class (hint, "dim-label");
  gtk_widget_set_vexpand (area, TRUE);
  gtk_box_append (GTK_BOX (box), area);
  gtk_box_append (GTK_BOX (box), hint);
  gtk_window_set_child (GTK_WINDOW (win), box);

  /* something to look at straight away */
  for (int i = 0; i <= 60; i++) add_point (40 + i * 5, 120 + 50 * sin (i / 6.0), i == 0);
  clicked (NULL, 1, 330, 70, area);
  gtk_window_present (GTK_WINDOW (win));
  (void) data;
}

int main (int argc, char *argv[])
{
  path = g_array_new (FALSE, FALSE, sizeof (Pt));
  GtkApplication *app = gtk_application_new ("org.example.Sketch", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  g_array_free (path, TRUE);
  return status;
}
