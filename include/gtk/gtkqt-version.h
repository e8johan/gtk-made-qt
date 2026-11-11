/*
 * GTK made Qt - the GTK C API implemented on top of Qt
 *
 * API level selection. The pkg-config files (gtk+-2.0.pc, gtk+-3.0.pc,
 * gtk4.pc) pass -DGTKQT_API_VERSION=2/3/4. The same library serves all
 * three; the API level only changes a few header-level details where the
 * GTK generations disagree (e.g. gtk_init() and gtk_window_new() lost their
 * arguments in GTK 4).
 */
#ifndef GTKQT_VERSION_H
#define GTKQT_VERSION_H

#ifndef GTKQT_API_VERSION
#define GTKQT_API_VERSION 3
#endif

#if GTKQT_API_VERSION != 2 && GTKQT_API_VERSION != 3 && GTKQT_API_VERSION != 4
#error "GTKQT_API_VERSION must be 2, 3 or 4"
#endif

#define GTK_MAJOR_VERSION GTKQT_API_VERSION
#if GTKQT_API_VERSION == 2
#define GTK_MINOR_VERSION 24
#define GTK_MICRO_VERSION 33
#elif GTKQT_API_VERSION == 3
#define GTK_MINOR_VERSION 24
#define GTK_MICRO_VERSION 41
#else
#define GTK_MINOR_VERSION 14
#define GTK_MICRO_VERSION 0
#endif

#define GTK_CHECK_VERSION(major, minor, micro) \
  (GTK_MAJOR_VERSION > (major) || \
   (GTK_MAJOR_VERSION == (major) && GTK_MINOR_VERSION > (minor)) || \
   (GTK_MAJOR_VERSION == (major) && GTK_MINOR_VERSION == (minor) && GTK_MICRO_VERSION >= (micro)))

/* True when compiling against this shim instead of real GTK. */
#define GTK_MADE_QT 1

#endif
