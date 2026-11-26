/*
 * gtkrc, GTK 2 API level, run headless: the user's ~/.gtkrc-2.0 (settings,
 * styles attached by class, widget and widget_class paths, style
 * inheritance), the key theme named by gtk-key-theme-name, the program's
 * own gtk_rc_parse_string (as Leafpad's Emacs mode does it), and the
 * program's gtk_widget_modify_* winning over rc styles.
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

static const char *user_rc =
  "# the user's file\n"
  "gtk-key-theme-name = \"TestKeys\"\n"
  "gtk-theme-name = \"Raleigh\"\n"              /* the look stays Qt's */
  "include \"/usr/share/themes/Raleigh/gtk-2.0/gtkrc\"\n"
  "style \"red\" {\n"
  "  fg[NORMAL] = \"#ff0000\"\n"
  "  fg[INSENSITIVE] = { 0.5, 0.5, 0.5 }\n"
  "  font_name = \"Monospace 15\"\n"
  "  engine \"pixmap\" { image { function = BOX } }\n"
  "  GtkWidget::focus-line-width = 2\n"
  "}\n"
  "style \"green\" = \"red\" { fg[NORMAL] = \"#00ff00\" }\n"
  "style \"blue-text\" { color[\"mine\"] = \"#0000ff\"\n base[NORMAL] = @mine\n text[NORMAL] = shade (0.5, \"#ffffff\") }\n"
  "class \"GtkLabel\" style \"red\"\n"
  "widget \"*.special\" style \"green\"\n"
  "widget_class \"*<GtkBox>.GtkTextView\" style \"blue-text\"\n";

static const char *key_theme =
  "binding \"test-keys\" {\n"
  "  bind \"<ctrl>e\" { \"move-cursor\" (paragraph-ends, 1, 0) }\n"
  "  bind \"<ctrl>a\" { \"move-cursor\" (paragraph-ends, -1, 0) }\n"
  "  bind \"<ctrl>k\" { \"delete-from-cursor\" (paragraph-ends, 1) }\n"
  "  bind \"<alt>space\" { \"delete-from-cursor\" (whitespace, 1)\n"
  "                      \"insert-at-cursor\" (\" \") }\n"
  "  bind \"<alt>f\" { \"move-cursor\" (words, 1, 0) }\n"
  "  bind \"<shift><alt>f\" { \"move-cursor\" (words, 1, 1) }\n"
  "  bind \"<ctrl>w\" { \"delete-from-cursor\" (word-ends, -1) }\n"
  "  bind \"<ctrl>h\" { }\n"
  "}\n"
  "class \"GtkEntry\" binding \"test-keys\"\n"
  "class \"GtkTextView\" binding \"test-keys\"\n";

static int deletes, inserts;
static void on_delete_text (GtkEditable *e, gint s, gint en, gpointer d) { deletes++; }

static int offset_of_cursor (GtkTextBuffer *b)
{
  GtkTextIter it;
  gtk_text_buffer_get_iter_at_mark (b, &it, gtk_text_buffer_get_insert (b));
  return gtk_text_iter_get_offset (&it);
}

static gchar *buffer_text (GtkTextBuffer *b)
{
  GtkTextIter s, e;
  gtk_text_buffer_get_bounds (b, &s, &e);
  return gtk_text_buffer_get_text (b, &s, &e, FALSE);
}

static void set_cursor (GtkTextBuffer *b, int off)
{
  GtkTextIter it;
  gtk_text_buffer_get_iter_at_offset (b, &it, off);
  gtk_text_buffer_place_cursor (b, &it);
}

static gboolean key (GtkWidget *w, guint keyval, guint mods)
{
  return gtk_bindings_activate (G_OBJECT (w), keyval, (GdkModifierType) mods);
}

int main (int argc, char **argv)
{
  gchar *home = g_dir_make_tmp ("gtkqt-rc-XXXXXX", NULL);
  CHECK (home);
  gchar *p = g_build_filename (home, ".gtkrc-2.0", NULL);
  write_file (p, user_rc);
  g_free (p);
  p = g_build_filename (home, ".themes", "TestKeys", "gtk-2.0-key", "gtkrc", NULL);
  write_file (p, key_theme);
  g_free (p);
  g_setenv ("HOME", home, TRUE);
  g_unsetenv ("GTK2_RC_FILES");
  g_unsetenv ("GTK_RC_FILES");

  gtk_init (&argc, &argv);

  /* settings from the rc file, but not the theme */
  gchar *kt = NULL, *theme = NULL;
  g_object_get (gtk_settings_get_default (), "gtk-key-theme-name", &kt, "gtk-theme-name", &theme, NULL);
  CHECK (kt && !strcmp (kt, "TestKeys"));
  CHECK (theme && strcmp (theme, "Raleigh"));
  g_free (kt);
  g_free (theme);

  /* the program's own binding, as Leafpad's Emacs mode adds it */
  gtk_rc_parse_string ("binding \"test-keys\" {\n bind \"<ctrl>w\" { \"cut-clipboard\" () }\n}\n");

  GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *box = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (win), box);
  GtkWidget *plain = gtk_label_new ("plain");
  GtkWidget *special = gtk_label_new ("special");
  gtk_widget_set_name (special, "special");
  GtkWidget *own = gtk_label_new ("own colour");
  GdkColor magenta = { 0, 0xffff, 0, 0xffff };
  gtk_widget_modify_fg (own, GTK_STATE_NORMAL, &magenta);
  GtkWidget *button = gtk_button_new_with_label ("button");
  GtkWidget *entry = gtk_entry_new ();
  GtkWidget *text = gtk_text_view_new ();
  gtk_box_pack_start (GTK_BOX (box), plain, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), special, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), own, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), text, TRUE, TRUE, 0);
  gtk_widget_show_all (win);
  while (gtk_events_pending ()) gtk_main_iteration ();

  /* styles: class, then widget path; a derived style keeps its parent's font */
  GtkStyle *s = gtk_widget_get_style (plain);
  CHECK (s->fg[GTK_STATE_NORMAL].red == 0xffff && s->fg[GTK_STATE_NORMAL].green == 0);
  CHECK (s->fg[GTK_STATE_INSENSITIVE].red == 0x8000);
  CHECK (s->font_desc && !strcmp (pango_font_description_get_family (s->font_desc), "Monospace"));
  CHECK (pango_font_description_get_size (s->font_desc) == 15 * PANGO_SCALE);
  s = gtk_widget_get_style (special);
  CHECK (s->fg[GTK_STATE_NORMAL].green == 0xffff && s->fg[GTK_STATE_NORMAL].red == 0);
  CHECK (pango_font_description_get_size (s->font_desc) == 15 * PANGO_SCALE);
  /* the program's colour wins; the rc font still applies */
  s = gtk_widget_get_style (own);
  CHECK (s->fg[GTK_STATE_NORMAL].red == 0xffff && s->fg[GTK_STATE_NORMAL].blue == 0xffff);
  CHECK (pango_font_description_get_size (s->font_desc) == 15 * PANGO_SCALE);
  /* widget_class with <GtkBox> (a GtkVBox is one), symbolic and shaded colours */
  s = gtk_widget_get_style (text);
  CHECK (s->base[GTK_STATE_NORMAL].blue == 0xffff && s->base[GTK_STATE_NORMAL].red == 0);
  CHECK (s->text[GTK_STATE_NORMAL].red > 0x7000 && s->text[GTK_STATE_NORMAL].red < 0x9000);
  /* the button matched nothing */
  s = gtk_widget_get_style (button);
  CHECK (s->fg[GTK_STATE_NORMAL].red != 0xffff || s->fg[GTK_STATE_NORMAL].green != 0);

  /* a style given up by the program falls back to the rc one */
  gtk_widget_modify_fg (own, GTK_STATE_NORMAL, NULL);
  s = gtk_widget_get_style (own);
  CHECK (s->fg[GTK_STATE_NORMAL].red == 0xffff && s->fg[GTK_STATE_NORMAL].blue == 0);

  /* renaming a widget restyles it */
  gtk_widget_set_name (plain, "special");
  CHECK (gtk_widget_get_style (plain)->fg[GTK_STATE_NORMAL].green == 0xffff);

  /* key bindings from the key theme on the text view */
  GtkTextBuffer *b = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  gtk_text_buffer_set_text (b, "hello world\nsecond   line", -1);
  set_cursor (b, 0);
  CHECK (key (text, GDK_e, GDK_CONTROL_MASK));
  CHECK (offset_of_cursor (b) == 11);
  CHECK (key (text, GDK_a, GDK_CONTROL_MASK));
  CHECK (offset_of_cursor (b) == 0);
  CHECK (key (text, GDK_f, GDK_MOD1_MASK));
  CHECK (offset_of_cursor (b) == 5);
  CHECK (key (text, GDK_f, GDK_MOD1_MASK | GDK_SHIFT_MASK));   /* extends: " world" */
  GtkTextIter ss, se;
  CHECK (gtk_text_buffer_get_selection_bounds (b, &ss, &se));
  CHECK (gtk_text_iter_get_offset (&ss) == 5 && gtk_text_iter_get_offset (&se) == 11);
  /* the program's <ctrl>w replaced the theme's: cut */
  CHECK (key (text, GDK_w, GDK_CONTROL_MASK));
  gchar *t = buffer_text (b);
  CHECK (!strcmp (t, "hello\nsecond   line"));
  g_free (t);
  gchar *clip = gtk_clipboard_wait_for_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
  CHECK (clip && !strcmp (clip, " world"));
  g_free (clip);
  /* C-k at the end of a line takes the line break */
  set_cursor (b, 5);
  CHECK (key (text, GDK_k, GDK_CONTROL_MASK));
  t = buffer_text (b);
  CHECK (!strcmp (t, "hellosecond   line"));
  g_free (t);
  /* C-k elsewhere: to the end of the line */
  set_cursor (b, 5);
  CHECK (key (text, GDK_k, GDK_CONTROL_MASK));
  t = buffer_text (b);
  CHECK (!strcmp (t, "hello"));
  g_free (t);
  /* two signals for one key: M-space squeezes white space to one */
  gtk_text_buffer_set_text (b, "a    b", -1);
  set_cursor (b, 3);
  CHECK (key (text, GDK_space, GDK_MOD1_MASK));
  t = buffer_text (b);
  CHECK (!strcmp (t, "a b"));
  g_free (t);
  /* bound to nothing: consumed */
  CHECK (key (text, GDK_h, GDK_CONTROL_MASK));
  /* not bound */
  CHECK (!key (text, GDK_q, GDK_CONTROL_MASK));

  /* the entry: the same set, changes through "delete-text" */
  g_signal_connect (entry, "delete-text", G_CALLBACK (on_delete_text), NULL);
  gtk_entry_set_text (GTK_ENTRY (entry), "one two three");
  gtk_editable_set_position (GTK_EDITABLE (entry), 0);
  CHECK (key (entry, GDK_e, GDK_CONTROL_MASK));
  CHECK (gtk_editable_get_position (GTK_EDITABLE (entry)) == 13);
  CHECK (key (entry, GDK_a, GDK_CONTROL_MASK));
  CHECK (key (entry, GDK_f, GDK_MOD1_MASK));
  CHECK (gtk_editable_get_position (GTK_EDITABLE (entry)) == 3);
  deletes = 0;
  CHECK (key (entry, GDK_k, GDK_CONTROL_MASK));
  CHECK (!strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), "one"));
  CHECK (deletes == 1);
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, 2);
  CHECK (key (entry, GDK_w, GDK_CONTROL_MASK));
  CHECK (!strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), "e"));
  clip = gtk_clipboard_wait_for_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
  CHECK (clip && !strcmp (clip, "on"));
  g_free (clip);

  /* class bindings from code still apply below the rc ones */
  GtkBindingSet *cls = gtk_binding_set_by_class (GTK_ENTRY_GET_CLASS (entry));
  gtk_binding_entry_add_signal (cls, GDK_e, GDK_CONTROL_MASK, "move-cursor", 3, GTK_TYPE_MOVEMENT_STEP,
                                GTK_MOVEMENT_BUFFER_ENDS, G_TYPE_INT, -1, G_TYPE_BOOLEAN, FALSE);
  gtk_binding_entry_add_signal (cls, GDK_b, GDK_CONTROL_MASK, "move-cursor", 3, GTK_TYPE_MOVEMENT_STEP,
                                GTK_MOVEMENT_BUFFER_ENDS, G_TYPE_INT, -1, G_TYPE_BOOLEAN, FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry), "abc");
  gtk_editable_set_position (GTK_EDITABLE (entry), 1);
  CHECK (key (entry, GDK_e, GDK_CONTROL_MASK));             /* the rc set wins */
  CHECK (gtk_editable_get_position (GTK_EDITABLE (entry)) == 3);
  CHECK (key (entry, GDK_b, GDK_CONTROL_MASK));             /* only in the class set */
  CHECK (gtk_editable_get_position (GTK_EDITABLE (entry)) == 0);

  /* gtk_binding_set_add_path from code, on a widget path */
  GtkBindingSet *mine = gtk_binding_set_new ("mine");
  gtk_binding_entry_add_signal (mine, GDK_F5, 0, "insert-at-cursor", 1, G_TYPE_STRING, "!");
  gtk_binding_set_add_path (mine, GTK_PATH_WIDGET, "*.GtkEntry", GTK_PATH_PRIO_APPLICATION);
  CHECK (key (entry, GDK_F5, 0));
  CHECK (!strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), "!abc"));

  /* a key theme set later is read then */
  p = g_build_filename (home, ".themes", "Later", "gtk-2.0-key", "gtkrc", NULL);
  write_file (p, "binding \"later\" { bind \"F6\" { \"insert-at-cursor\" (\"6\") } }\nclass \"GtkEntry\" binding \"later\"\n");
  g_free (p);
  g_object_set (gtk_settings_get_default (), "gtk-key-theme-name", "Later", NULL);
  CHECK (key (entry, GDK_F6, 0));
  CHECK (strstr (gtk_entry_get_text (GTK_ENTRY (entry)), "6"));

  gtk_widget_destroy (win);
  printf ("rc: all checks passed\n");
  return 0;
}
