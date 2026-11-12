/*
 * GTK made Qt - scripted input for headless testing.
 *
 * GTKQT_SCRIPT="wait 500; type hello; key Ctrl+S; ..." runs these steps one
 * after another once the event loop runs:
 *   wait <ms>                  pause
 *   key <keys>                 key presses to the focused widget (QKeySequence
 *                              syntax: Return, Ctrl+Q, Shift+Tab); a key bound
 *                              as an accelerator triggers it
 *   type <text>                type text into the focused widget
 *   menu <path>                activate a menu item: File/Save As...
 *   click <widget> [1|2|3] [x y]   press and release a mouse button
 *   focus <widget>             give a widget the keyboard focus
 *   file <name>                accept the open file dialog with that file
 *   accept | reject            close the active dialog (OK / Cancel)
 *   shot <file.png>            save the visible windows
 *   dump                       print the widget tree
 *   quit                       exit(0)
 * <widget> is a widget name (gtk_widget_set_name), a button or label text,
 * or a kind: textview, entry, treeview, window.
 * Licensed under the LGPL 2.1 or later; see COPYING.
 */
#include "gtkqt-private.h"

#include <QtWidgets>
#include <cstdio>

static QString plain (QString s) { return s.remove ('&').remove (QStringLiteral ("...")).remove (QChar (0x2026)).trimmed (); }

static QList<QWidget *> visible_windows ()
{
  QList<QWidget *> l;
  if (QWidget *m = QApplication::activeModalWidget ()) l << m;
  for (QWidget *t : QApplication::topLevelWidgets ())
    if (t->isVisible () && !qobject_cast<QMenu *> (t) && !l.contains (t)) l << t;
  return l;
}

static QWidget *find_widget (const QString &spec)
{
  for (QWidget *top : visible_windows ()) {
    QList<QWidget *> all = top->findChildren<QWidget *> ();
    all.prepend (top);
    for (QWidget *w : all) {
      if (!w->isVisible ()) continue;
      if (w->objectName () == spec) return w;
      if (spec == "textview" && qobject_cast<QTextEdit *> (w)) return w;
      if (spec == "entry" && qobject_cast<QLineEdit *> (w)) return w;
      if (spec == "treeview" && qobject_cast<QAbstractItemView *> (w)) return w;
      if (spec == "window" && w == top) return w;
      if (auto *b = qobject_cast<QAbstractButton *> (w); b && plain (b->text ()) == plain (spec)) return w;
      if (auto *lb = qobject_cast<QLabel *> (w); lb && plain (lb->text ()) == plain (spec)) return w;
    }
  }
  return nullptr;
}

static QWidget *focus_target ()
{
  if (QWidget *p = QApplication::activePopupWidget ()) return p;
  if (QWidget *f = QApplication::focusWidget (); f && f->isVisible ()) return f;
  /* no active window (offscreen platform after a dialog closed): the
   * window's remembered focus child */
  QList<QWidget *> w = visible_windows ();
  if (QWidget *a = QApplication::activeWindow (); a && a->isVisible ()) w.prepend (a);
  for (QWidget *t : w) if (QWidget *f = t->focusWidget (); f && f->isVisible ()) { t->activateWindow (); f->setFocus (); return f; }
  return w.isEmpty () ? nullptr : w.first ();
}

static void send_key (QWidget *t, int key, Qt::KeyboardModifiers mods, const QString &text)
{
  QKeyEvent press (QEvent::KeyPress, key, mods, text);
  QKeyEvent release (QEvent::KeyRelease, key, mods, text);
  QApplication::sendEvent (t, &press);
  QApplication::sendEvent (t, &release);
}

static bool trigger_shortcut (const QKeySequence &seq)
{
  QWidget *win = QApplication::activeModalWidget ();
  if (!win) { QWidget *f = focus_target (); win = f ? f->window () : nullptr; }
  QList<QAction *> acts;
  if (win) {
    acts << win->actions ();
    for (QWidget *w : win->findChildren<QWidget *> ()) acts << w->actions ();
  }
  for (QAction *a : acts)
    if (a->isEnabled () && a->shortcuts ().contains (seq)) { a->trigger (); return true; }
  return false;
}

static QAction *find_menu_action (const QStringList &path)
{
  for (QWidget *top : visible_windows ())
    for (QMenuBar *bar : top->findChildren<QMenuBar *> ()) {
      QList<QAction *> level = bar->actions ();
      QAction *found = nullptr;
      for (const QString &part : path) {
        found = nullptr;
        for (QAction *a : level)
          if (plain (a->text ()) == plain (part)) { found = a; break; }
        if (!found) break;
        if (found->menu ()) {
          Q_EMIT found->menu ()->aboutToShow ();   /* programs update items here */
          level = found->menu ()->actions ();
        }
      }
      if (found) return found;
    }
  return nullptr;
}

static void run_steps (QStringList steps);

static void next (const QStringList &steps, int ms = 50)
{
  QTimer::singleShot (ms, qApp, [steps] { run_steps (steps); });
}

static void run_steps (QStringList steps)
{
  while (!steps.isEmpty ()) {
    QString step = steps.takeFirst ().trimmed ();
    if (step.isEmpty ()) continue;
    QString cmd = step.section (' ', 0, 0), rest = step.section (' ', 1);
    QStringList a = rest.split (' ', Qt::SkipEmptyParts);
    if (cmd == "wait") { next (steps, a.value (0).toInt ()); return; }
    if (cmd == "key") {
      QKeySequence seq (rest, QKeySequence::PortableText);
      if (!trigger_shortcut (seq)) {
        QWidget *t = focus_target ();
        for (int i = 0; t && i < seq.count (); i++) {
          QKeyCombination kc = seq[i];
          int k = kc.key ();
          Qt::KeyboardModifiers m = kc.keyboardModifiers ();
          QString text;
          if (k >= 0x20 && k < 0x7f && !(m & (Qt::ControlModifier | Qt::AltModifier)))
            text = (m & Qt::ShiftModifier) ? QChar (k).toUpper () : QChar (k).toLower ();
          else if (k == Qt::Key_Return || k == Qt::Key_Enter) text = "\r";
          else if (k == Qt::Key_Tab) text = "\t";
          else if (k == Qt::Key_Backspace) text = "\b";
          send_key (t, k, m, text);
        }
      }
    } else if (cmd == "type") {
      QWidget *t = focus_target ();
      for (QChar c : rest) if (t) send_key (t, c.toUpper ().unicode (), c.isUpper () ? Qt::ShiftModifier : Qt::NoModifier, QString (c));
    } else if (cmd == "menu") {
      QAction *act = find_menu_action (rest.split ('/'));
      if (!act) fprintf (stderr, "script: no menu item: %s\n", qPrintable (rest));
      else {
        QStringList later = steps;
        /* the item may open a modal dialog: run the rest of the script from inside it */
        QTimer::singleShot (0, qApp, [act] { act->trigger (); });
        next (later, 300);
        return;
      }
    } else if (cmd == "click" || cmd == "focus") {
      QWidget *t = find_widget (a.value (0));
      if (!t) { fprintf (stderr, "script: no widget: %s\n", qPrintable (a.value (0))); continue; }
      if (cmd == "focus") { t->activateWindow (); t->setFocus (Qt::OtherFocusReason); }
      else if (auto *b = qobject_cast<QAbstractButton *> (t); b && a.size () < 2) {
        QTimer::singleShot (0, b, [b] { b->click (); });
        next (steps, 300);
        return;
      } else {
        int bn = a.size () > 1 ? a[1].toInt () : 1;
        Qt::MouseButton button = bn == 3 ? Qt::RightButton : bn == 2 ? Qt::MiddleButton : Qt::LeftButton;
        QWidget *target = t;
        if (auto *sa = qobject_cast<QAbstractScrollArea *> (t)) target = sa->viewport ();
        QPointF pos = a.size () > 3 ? QPointF (a[2].toInt (), a[3].toInt ()) : QPointF (target->width () / 2.0, target->height () / 2.0);
        QMouseEvent press (QEvent::MouseButtonPress, pos, target->mapToGlobal (pos), button, button, Qt::NoModifier);
        QMouseEvent release (QEvent::MouseButtonRelease, pos, target->mapToGlobal (pos), button, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent (target, &press);
        QApplication::sendEvent (target, &release);
      }
    } else if (cmd == "file") {
      QFileDialog *fd = nullptr;
      for (QWidget *w : visible_windows ()) if ((fd = qobject_cast<QFileDialog *> (w))) break;
      if (!fd) { fprintf (stderr, "script: no file dialog\n"); continue; }
      QFileInfo fi (rest);
      fd->setDirectory (fi.absolutePath ());
      fd->selectFile (fi.absoluteFilePath ());
      QTimer::singleShot (0, fd, [fd] { static_cast<QDialog *> (fd)->accept (); });
      next (steps, 300);
      return;
    } else if (cmd == "accept" || cmd == "reject") {
      bool acc = cmd == "accept";
      QWidget *m = QApplication::activeModalWidget ();
      if (!m)
        for (QWidget *w : visible_windows ()) {
          GtkWidget *gw = gtkqt_from_qobject (w);
          if (qobject_cast<QDialog *> (w) || (gw && GTK_IS_DIALOG (gw))) { m = w; if (w->isModal ()) break; }
        }
      GtkWidget *g = m ? gtkqt_from_qobject (m) : nullptr;
      if (g && GTK_IS_DIALOG (g) && !m->property ("gtkqt-self-drawn").toBool ()) {
        /* press the button with an accepting (or cancelling) response, as a user would */
        static const int ok_ids[] = { GTK_RESPONSE_OK, GTK_RESPONSE_ACCEPT, GTK_RESPONSE_YES, GTK_RESPONSE_APPLY, GTK_RESPONSE_CLOSE };
        static const int no_ids[] = { GTK_RESPONSE_CANCEL, GTK_RESPONSE_REJECT, GTK_RESPONSE_NO, GTK_RESPONSE_CLOSE };
        GtkWidget *button = nullptr;
        int id = acc ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL;
        for (int r : acc ? QList<int> (std::begin (ok_ids), std::end (ok_ids)) : QList<int> (std::begin (no_ids), std::end (no_ids)))
          if ((button = gtk_dialog_get_widget_for_response (GTK_DIALOG (g), r))) { id = r; break; }
        g_object_ref (g);
        if (button) g_object_ref (button);
        QTimer::singleShot (0, qApp, [g, button, id] {
          if (button) { gtk_button_clicked (GTK_BUTTON (button)); g_object_unref (button); }
          else gtk_dialog_response (GTK_DIALOG (g), id);
          g_object_unref (g);
        });
      } else if (auto *d = qobject_cast<QDialog *> (m)) {
        QTimer::singleShot (0, d, [d, acc] { if (acc) d->accept (); else d->reject (); });
      } else {
        fprintf (stderr, "script: no dialog to %s\n", qPrintable (cmd));
      }
      next (steps, 300);
      return;
    } else if (cmd == "shot") {
      int n = 0;
      for (QWidget *tl : visible_windows ()) {
        QString name = rest;
        if (n > 0) { int dot = name.lastIndexOf ('.'); name.insert (dot < 0 ? name.size () : dot, QStringLiteral ("-%1").arg (n)); }
        tl->grab ().save (name);
        n++;
      }
    } else if (cmd == "dump") {
      for (QWidget *top : visible_windows ()) {
        std::function<void (QWidget *, int)> dump = [&] (QWidget *w, int depth) {
          if (!w->isVisible ()) return;
          GtkWidget *g = gtkqt_from_qobject (w);
          QString text;
          if (auto *b = qobject_cast<QAbstractButton *> (w)) text = b->text ();
          else if (auto *l = qobject_cast<QLabel *> (w)) text = l->text ();
          printf ("%*s%s %s%s%s %dx%d+%d+%d\n", depth * 2, "", w->metaObject ()->className (), g ? G_OBJECT_TYPE_NAME (g) : "",
                  text.isEmpty () ? "" : " ", qPrintable (text), w->width (), w->height (), w->x (), w->y ());
          for (QObject *c : w->children ()) if (auto *cw = qobject_cast<QWidget *> (c); cw && !cw->isWindow ()) dump (cw, depth + 1);
        };
        dump (top, 0);
      }
      fflush (stdout);
    } else if (cmd == "quit") {
      fflush (stdout);
      fflush (stderr);
      _exit (0);
    } else {
      fprintf (stderr, "script: unknown step: %s\n", qPrintable (step));
    }
    next (steps);
    return;
  }
}

void gtkqt_script_start ()
{
  QString script = qEnvironmentVariable ("GTKQT_SCRIPT");
  if (script.isEmpty ()) return;
  QStringList steps = script.split (';');
  QTimer::singleShot (0, qApp, [steps] { run_steps (steps); });
}
