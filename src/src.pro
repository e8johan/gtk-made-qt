TEMPLATE = lib

TARGET = gt
DESTDIR = ../lib
CONFIG += staticlib
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
    ../include/qgtkcallbackbridge.h \
    ../include/qgtkeventfilter.h \
    ../include/qgtkdeleteeventfilter.h \
    ../include/qgtkgeneraleventfilter.h

HEADERS += \
    ../include/gtk-made-qt.h \
    ../include/glibtypes.h
