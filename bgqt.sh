#!/bin/sh
if [ ! -n "$1" ]
then
	_test="test"
	echo "no test specified, running test"
else
	echo "running $1"
	_test=$1
fi

#various directories. Change if you use another path
_Here=$(pwd)
_QtInclude=/usr/include/qt
_QtLib=/usr/lib
_QtBin=/usr/lib/qt/bin
_Glib2Include=/usr/include/glib-2.0
_Glib2LibInclude=/usr/lib/glib-2.0/include

#prepare moc headers - TODO: figure out how to do this in qmake
"${_QtBin}"/moc -i include/gtk/gtk_button.h -o include/gtk/moc_gtk_button.h
"${_QtBin}"/moc -i include/gtk/gtk_entry.h -o include/gtk/moc_gtk_entry.h
"${_QtBin}"/moc -i include/gtk/gtk_combo.h -o include/gtk/moc_gtk_combo.h

# build library
qmake-qt5 gtk-made-qt.pro
make

# compile
g++ -fPIC "examples/${_test}.c" -o ${_test} -ggdb3 -DUSE_GLIB2 -I"${_Here}"/include -I"${_Glib2LibInclude}" -I"${_Glib2Include}" -I"${_QtInclude}" -I"${_QtInclude}"/QtGui -I"${_QtInclude}"/QtWidgets -I"${_QtInclude}"/QtUiTools -I"${_QtInclude}"/QtCore -L"${_QtLib}" -L"${_Here}"/lib -lgt -lQt5Core -lQt5Gui -lQt5Widgets -Wl,--rpath -Wl,"${_Here}"/lib

./"${_test}" -widgetcount



