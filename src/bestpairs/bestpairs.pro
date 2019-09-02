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

LIBS += -lpteros -lpteros_analysis -ltng_io

SOURCES += \
        main.cpp

INCLUDEPATH += ../

HEADERS += \
    best_dist.h \
    chisqdist.hpp \
    spline.hpp \
    polynomial.hpp
