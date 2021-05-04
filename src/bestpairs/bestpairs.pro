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

LIBS += -lpteros -lspdlog -lfmt
DEFINES += SPDLOG_FMT_EXTERNAL

COMMIT_DATE = $$system(git show -s --pretty='%ci')
COMMIT_DATE = $$first(COMMIT_DATE)
COMMIT_HASH = $$system(git log --pretty=format:'%h' -n 1)
COMMIT_BRANCH =$$system(git branch -a --contains $$COMMIT_HASH | grep -v HEAD | head -n1 | tr / \' \' | awk \'{print $NF}\')
DEFINES += APP_VERSION=\\\"$$COMMIT_DATE-$$COMMIT_BRANCH-$$COMMIT_HASH\\\"

SOURCES += \
        main.cpp

INCLUDEPATH += ../

HEADERS += \
    best_dist.h \
    chisqdist.hpp \
    spline.hpp \
    polynomial.hpp
