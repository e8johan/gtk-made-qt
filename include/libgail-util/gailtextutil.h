/*
 * GTK made Qt - GTK 2's gail-util: text segments for ATK's AtkText, by
 * character, word, sentence and line. Programs get it with pkg-config
 * gail (which brings the real ATK).
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#ifndef GTKQT_GAIL_TEXT_UTIL_H
#define GTKQT_GAIL_TEXT_UTIL_H

#include <gtk/gtk.h>
#include <atk/atk.h>

G_BEGIN_DECLS

#define GAIL_TYPE_TEXT_UTIL           (gail_text_util_get_type ())
#define GAIL_TEXT_UTIL(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GAIL_TYPE_TEXT_UTIL, GailTextUtil))
#define GAIL_IS_TEXT_UTIL(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GAIL_TYPE_TEXT_UTIL))

typedef enum { GAIL_BEFORE_OFFSET, GAIL_AT_OFFSET, GAIL_AFTER_OFFSET } GailOffsetType;

typedef struct _GailTextUtil {
  GObject parent;
  GtkTextBuffer *buffer;
} GailTextUtil;
typedef struct _GailTextUtilClass { GObjectClass parent_class; } GailTextUtilClass;

GType         gail_text_util_get_type (void) G_GNUC_CONST;
GailTextUtil *gail_text_util_new (void);
void          gail_text_util_text_setup (GailTextUtil *textutil, const gchar *text);
void          gail_text_util_buffer_setup (GailTextUtil *textutil, GtkTextBuffer *buffer);
gchar        *gail_text_util_get_text (GailTextUtil *textutil, gpointer layout, GailOffsetType function,
                                       AtkTextBoundary boundary_type, gint offset, gint *start_offset, gint *end_offset);
gchar        *gail_text_util_get_substring (GailTextUtil *textutil, gint start_pos, gint end_pos);

G_END_DECLS

#endif
