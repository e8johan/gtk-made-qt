#!/bin/sh
if [ ! -n "$1" ]
then
	test="test"
	echo "no test specified, running test"
else
	echo "running $1"
	test=$1
fi

# run MOC first
~/qt4-bin/bin/moc -i gtk/gtk_button.h -o gtk/moc_gtk_button.h
~/qt4-bin/bin/moc -i gtk/gtk_entry.h -o gtk/moc_gtk_entry.h
~/qt4-bin/bin/moc -i gtk/gtk_combo.h -o gtk/moc_gtk_combo.h
g++ "examples/$test.c" -o test -ggdb3 -I/home/w00t/gqt -I/home/w00t/qt4-bin/include -I/home/w00t/qt4-bin/include/QtGui -I/home/w00t/qt4-bin/include/QtUiTools -I/home/w00t/qt4-bin/include/QtCore -L/home/w00t/qt4-bin/lib -lQtCore -lQtGui -l QtUiTools -Wl,--rpath -Wl,/home/w00t/qt4-bin/lib `pkg-config --cflags --libs glib-2.0`
rm gtk/moc_gtk_button.h
rm gtk/moc_gtk_entry.h
rm gtk/moc_gtk_combo.h
./test -widgetcount
