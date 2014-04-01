TEMPLATE = lib

TARGET = gt
DESTDIR = ../lib
VERSION = 0.1.0

QT += widgets

INCLUDEPATH += ../include

# GTK made Qt files
SOURCES += \
    qgtkcallbackbridge.cpp \
    qgtkeventfilter.cpp \
    qgtkdeleteeventfilter.cpp \
    qgtkgeneraleventfilter.cpp

HEADERS += \
    qgtkcallbackbridge.h \
    qgtkeventfilter.h \
    qgtkdeleteeventfilter.h \
    qgtkgeneraleventfilter.h

HEADERS += \
    ../include/gtk-made-qt.h \
    ../include/glibtypes.h
