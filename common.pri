QMAKE_CXXFLAGS -= -O
QMAKE_CXXFLAGS -= -O1
QMAKE_CXXFLAGS -= -O2
QMAKE_CXXFLAGS -= -std=c++0x
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -std=c++0x
QMAKE_CXXFLAGS_RELEASE *= -O3 -march=native #-flto -fwhole-program
QMAKE_LFLAGS -= -O1
QMAKE_LFLAGS -= -std=c++0x
QMAKE_LFLAGS *= -O3 #-flto -fwhole-program

if(true){
COMMIT_BRANCH = $$system(git rev-parse --abbrev-ref HEAD)
COMMIT_DATE = $$system(git show -s --pretty='%ci')
COMMIT_DATE = $$first(COMMIT_DATE)
COMMIT_HASH = $$system(git log --pretty=format:'%h' -n 1)
DEFINES += APP_VERSION=\\\"$$COMMIT_DATE-$$COMMIT_BRANCH-$$COMMIT_HASH\\\"
}
else {
DEFINES += APP_VERSION=\\\"123\\\"
}
CONFIG += c++11
CONFIG += no_keywords
CONFIG(release, debug|release): DEFINES+=NDEBUG

QMAKE_CXXFLAGS += -std=c++11 -Wextra -Winit-self -Wold-style-cast \
-Woverloaded-virtual -Wuninitialized -Winit-self -pedantic-errors -Wno-attributes#-Werror

LIBS += -lasync++ -lpteros -lpteros_analysis -ltng_io #-lyomm11

INCLUDEPATH += $$PWD
VPATH += $$PWD
SRC_DIR = $$PWD

DISTFILES += \
    .gitignore \
    vdWRadii.json

HEADERS +=

SOURCES +=

