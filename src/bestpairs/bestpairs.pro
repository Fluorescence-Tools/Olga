TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
QT += core
QT -= gui

DEPENDPATH += . ../irmsd
INCLUDEPATH += ../irmsd
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../irmsd/release/ -lirmsd
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../irmsd/debug/ -lirmsd
else:unix: LIBS += -L$$OUT_PWD/../irmsd/ -lirmsd

win32: QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE3

LIBS += -lpteros
!system("grep Ubuntu /etc/os-release -q"): LIBS += -lspdlog -lfmt

SOURCES += \
        main.cpp

INCLUDEPATH += ../

HEADERS += \
    best_dist.h \
    chisqdist.hpp \
    spline.hpp \
    polynomial.hpp
