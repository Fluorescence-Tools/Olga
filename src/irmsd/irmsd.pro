QT       -= core gui

TARGET = irmsd
TEMPLATE = lib

win32: QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE2

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
