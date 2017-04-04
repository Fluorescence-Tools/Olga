#-------------------------------------------------
#
# Project created by QtCreator 2017-04-04T16:16:01
#
#-------------------------------------------------

QT       -= core gui

TARGET = irmsd
TEMPLATE = lib

DEFINES += IRMSD_LIBRARY

SOURCES += \
    theobald_rmsd.cpp

HEADERS +=\
        irmsd_global.h \
    theobald_rmsd.h \
    util.h \
    msvccompat.h \
    sse_swizzle.h \
    center.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
