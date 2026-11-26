/*
 * CSS, GTK 3 API level, run headless: colours and fonts from a provider on
 * node names, type names, #names and .classes with state pseudo-classes,
 * @define-color and colour functions, specificity and provider priority,
 * a widget's own provider, restyling when classes change or a provider
 * goes away, @binding-set with -gtk-key-bindings, the key theme from
 * settings.ini, and the program's override_* winning over CSS.
 */
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CHECK(expr) do { if (!(expr)) { fprintf (stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); exit (1); } } while (0)

static void write_file (const char *path, const char *text)
{
  gchar *dir = g_path_get_dirname (path);
  g_mkdir_with_parents (dir, 0700);
  g_free (dir);
  CHECK (g_file_set_contents (path, text, -1, NULL));
}

static const char *css =
  "@define-color accent #3366cc;\n"
  "/* unknown properties and selectors are the theme's business */\n"
  "* { transition: none; border-radius: 3px; }\n"
  "label { color: @accent; }\n"
  "GtkLabel.warn, label.warn { background-color: rgb(255, 255, 0); color: shade(@accent, 0.5); }\n"
  "#big { font: bold 20px Serif; }\n"
  "#big:hover { color: red; }\n"
  "box > button label { color: alpha(#00ff00, 0.5); }\n"
  "entry:disabled { color: #808080; }\n"
  "entry:focus { color: red; }\n"          /* not tracked: ignored */
  "textview text { background-color: #000080; color: white; font-family: Monospace; font-size: 14pt; }\n"
  "textview text selection { background-color: mix(#ff0000, #0000ff, 0.5); }\n"
  "@binding-set my-entry-keys {\n"
  "  bind \"<ctrl>e\" { \"move-cursor\" (paragraph-ends, 1, 0) };\n"
  "  bind \"<ctrl>k\" { \"delete-from-cursor\" (paragraph-ends, 1) };\n"
  "}\n"
  "entry { -gtk-key-bindings: my-entry-keys; }\n";

static const char *key_theme_css =
  "@binding-set theme-keys { bind \"<ctrl>a\" { \"move-cursor\" (buffer-ends, -1, 0) }; }\n"
  "textview { -gtk-key-bindings: theme-keys; }\n";

static gboolean key (GtkWidget *w, guint keyval, guint mods)
{
  return gtk_bindings_activate (G_OBJECT (w), keyval, (GdkModifierType) mods);
}

static int is_rgb (GdkColor c, int r, int g, int b)
{
  return abs ((c.red >> 8) - r) <= 1 && abs ((c.green >> 8) - g) <= 1 && abs ((c.blue >> 8) - b) <= 1;
}

int main (int argc, char **argv)
{
  gchar *dir = g_dir_make_tmp ("gtkqt-css-XXXXXX", NULL);
  CHECK (dir);
  gchar *cfg = g_build_filename (dir, "config", NULL), *data = g_build_filename (dir, "data", NULL);
  gchar *p = g_build_filename (cfg, "gtk-3.0", "settings.ini", NULL);
  write_file (p, "[Settings]\ngtk-key-theme-name = TestKeys\ngtk-theme-name = Raleigh\n");
  g_free (p);
  p = g_build_filename (data, "themes", "TestKeys", "gtk-3.0", "gtk-keys.css", NULL);
  write_file (p, key_theme_css);
  g_free (p);
  g_setenv ("XDG_CONFIG_HOME", cfg, TRUE);
  g_setenv ("XDG_DATA_HOME", data, TRUE);

  gtk_init (&argc, &argv);

  GtkCssProvider *prov = gtk_css_provider_new ();
  CHECK (gtk_css_provider_load_from_data (prov, css, -1, NULL));
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (), GTK_STYLE_PROVIDER (prov),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (win), box);
  GtkWidget *plain = gtk_label_new ("plain");
  GtkWidget *warn = gtk_label_new ("warn");
  gtk_style_context_add_class (gtk_widget_get_style_context (warn), "warn");
  GtkWidget *big = gtk_label_new ("big");
  gtk_widget_set_name (big, "big");
  GtkWidget *own = gtk_label_new ("own");
  GdkRGBA mine = { 1, 0, 1, 1 };
  gtk_widget_override_color (own, GTK_STATE_FLAG_NORMAL, &mine);
  GtkWidget *button = gtk_button_new_with_label ("button");
  GtkWidget *entry = gtk_entry_new ();
  GtkWidget *off = gtk_entry_new ();
  GtkWidget *text = gtk_text_view_new ();
  GtkWidget *widget_css = gtk_label_new ("widget css");
  gtk_box_pack_start (GTK_BOX (box), plain, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), warn, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), big, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), own, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), off, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), text, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), widget_css, FALSE, FALSE, 0);
  gtk_widget_show_all (win);
  while (gtk_events_pending ()) gtk_main_iteration ();

  /* node name, @define-color */
  CHECK (is_rgb (gtk_widget_get_style (plain)->fg[GTK_STATE_NORMAL], 0x33, 0x66, 0xcc));
  /* a class is more specific than the node; shade() */
  GtkStyle *s = gtk_widget_get_style (warn);
  CHECK (is_rgb (s->bg[GTK_STATE_NORMAL], 255, 255, 0));
  CHECK (s->fg[GTK_STATE_NORMAL].blue < 0xcc00 && s->fg[GTK_STATE_NORMAL].blue > 0x4000);
  /* #name: the font; :hover goes to the PRELIGHT state */
  s = gtk_widget_get_style (big);
  CHECK (s->font_desc && !strcmp (pango_font_description_get_family (s->font_desc), "Serif"));
  CHECK (pango_font_description_get_weight (s->font_desc) == PANGO_WEIGHT_BOLD);
  CHECK (pango_font_description_get_size_is_absolute (s->font_desc));
  CHECK (pango_font_description_get_size (s->font_desc) == 20 * PANGO_SCALE);
  CHECK (is_rgb (s->fg[GTK_STATE_PRELIGHT], 255, 0, 0));
  CHECK (is_rgb (s->fg[GTK_STATE_NORMAL], 0x33, 0x66, 0xcc));
  /* the program's colour wins */
  CHECK (is_rgb (gtk_widget_get_style (own)->fg[GTK_STATE_NORMAL], 255, 0, 255));
  /* :disabled is the INSENSITIVE state; :focus is not tracked */
  s = gtk_widget_get_style (entry);
  CHECK (is_rgb (s->fg[GTK_STATE_INSENSITIVE], 0x80, 0x80, 0x80));
  CHECK (!is_rgb (s->fg[GTK_STATE_NORMAL], 255, 0, 0));
  /* "textview text": the text view's base, text and font; "selection": SELECTED */
  s = gtk_widget_get_style (text);
  CHECK (is_rgb (s->base[GTK_STATE_NORMAL], 0, 0, 0x80));
  CHECK (is_rgb (s->text[GTK_STATE_NORMAL], 255, 255, 255));
  CHECK (is_rgb (s->base[GTK_STATE_SELECTED], 0x80, 0, 0x80));
  CHECK (!strcmp (pango_font_description_get_family (s->font_desc), "Monospace"));
  CHECK (pango_font_description_get_size (s->font_desc) == 14 * PANGO_SCALE);

  /* classes change at run time */
  gtk_style_context_remove_class (gtk_widget_get_style_context (warn), "warn");
  CHECK (is_rgb (gtk_widget_get_style (warn)->fg[GTK_STATE_NORMAL], 0x33, 0x66, 0xcc));
  gtk_style_context_add_class (gtk_widget_get_style_context (plain), "warn");
  CHECK (is_rgb (gtk_widget_get_style (plain)->bg[GTK_STATE_NORMAL], 255, 255, 0));

  /* a widget's own provider applies to that widget, above the screen's */
  GtkCssProvider *wp = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (wp, "label { color: #112233; }", -1, NULL);
  gtk_style_context_add_provider (gtk_widget_get_style_context (widget_css), GTK_STYLE_PROVIDER (wp),
                                  GTK_STYLE_PROVIDER_PRIORITY_USER);
  CHECK (is_rgb (gtk_widget_get_style (widget_css)->fg[GTK_STATE_NORMAL], 0x11, 0x22, 0x33));
  CHECK (is_rgb (gtk_widget_get_style (warn)->fg[GTK_STATE_NORMAL], 0x33, 0x66, 0xcc));

  /* a higher-priority provider wins over a more specific rule */
  GtkCssProvider *user = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (user, "* { color: #445566; }", -1, NULL);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (), GTK_STYLE_PROVIDER (user),
                                             GTK_STYLE_PROVIDER_PRIORITY_USER);
  CHECK (is_rgb (gtk_widget_get_style (big)->fg[GTK_STATE_NORMAL], 0x44, 0x55, 0x66));
  gtk_style_context_remove_provider_for_screen (gdk_screen_get_default (), GTK_STYLE_PROVIDER (user));
  CHECK (is_rgb (gtk_widget_get_style (big)->fg[GTK_STATE_NORMAL], 0x33, 0x66, 0xcc));

  /* reloading a provider replaces its rules; what is gone is reset */
  GdkColor before = gtk_widget_get_style (button)->bg[GTK_STATE_NORMAL];
  GtkCssProvider *tmp = gtk_css_provider_new ();
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (), GTK_STYLE_PROVIDER (tmp),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data (tmp, "button { background-color: #ff8800; }", -1, NULL);
  CHECK (is_rgb (gtk_widget_get_style (button)->bg[GTK_STATE_NORMAL], 0xff, 0x88, 0));
  gtk_css_provider_load_from_data (tmp, "", -1, NULL);
  GdkColor after = gtk_widget_get_style (button)->bg[GTK_STATE_NORMAL];
  CHECK (after.red == before.red && after.green == before.green && after.blue == before.blue);

  GdkRGBA looked;
  CHECK (gtk_style_context_lookup_color (gtk_widget_get_style_context (plain), "accent", &looked));
  CHECK (looked.blue > 0.79 && looked.blue < 0.81);

  /* @binding-set + -gtk-key-bindings */
  gtk_entry_set_text (GTK_ENTRY (entry), "hello world");
  gtk_editable_set_position (GTK_EDITABLE (entry), 0);
  CHECK (key (entry, GDK_KEY_e, GDK_CONTROL_MASK));
  CHECK (gtk_editable_get_position (GTK_EDITABLE (entry)) == 11);
  gtk_editable_set_position (GTK_EDITABLE (entry), 5);
  CHECK (key (entry, GDK_KEY_k, GDK_CONTROL_MASK));
  CHECK (!strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), "hello"));
  /* the key theme from settings.ini */
  GtkTextBuffer *b = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  gtk_text_buffer_set_text (b, "one\ntwo", -1);
  GtkTextIter it;
  gtk_text_buffer_get_end_iter (b, &it);
  gtk_text_buffer_place_cursor (b, &it);
  CHECK (key (text, GDK_KEY_a, GDK_CONTROL_MASK));
  gtk_text_buffer_get_iter_at_mark (b, &it, gtk_text_buffer_get_insert (b));
  CHECK (gtk_text_iter_get_offset (&it) == 0);
  /* switching the key theme off drops its bindings */
  g_object_set (gtk_settings_get_default (), "gtk-key-theme-name", "Default", NULL);
  gtk_text_buffer_get_end_iter (b, &it);
  gtk_text_buffer_place_cursor (b, &it);
  CHECK (!key (text, GDK_KEY_a, GDK_CONTROL_MASK));
  gtk_text_buffer_get_iter_at_mark (b, &it, gtk_text_buffer_get_insert (b));
  CHECK (gtk_text_iter_get_offset (&it) == 7);

  gtk_widget_destroy (win);
  printf ("css: all checks passed\n");
  return 0;
}
