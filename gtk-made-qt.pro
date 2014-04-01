QT += widgets

# Examples, pick one
SOURCES += examples/helloworld.cpp
#SOURCES += examples/tablepacking.cpp
#SOURCES += examples/manual-menu.cpp
#SOURCES += examples/range-widgets.cpp # Not implemented

# GTK made Qt files
SOURCES += qgtkcallbackbridge.cpp \
    qgtkeventfilter.cpp \
    qgtkdeleteeventfilter.cpp \
    qgtkgeneraleventfilter.cpp
HEADERS += gtk-made-qt.h \
    qgtkcallbackbridge.h \
    glibtypes.h \
    qgtkeventfilter.h \
    qgtkdeleteeventfilter.h \
    qgtkgeneraleventfilter.h
