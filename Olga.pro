#-------------------------------------------------
#
# Project created by QtCreator 2014-02-27T12:04:31
#
#-------------------------------------------------
QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = Olga
TEMPLATE = app

#VERSION = YYYYMMDDx
#DEFINES += APP_VERSION=\\\"$$VERSION\\\"
COMMIT_BRANCH = $$system(git rev-parse --abbrev-ref HEAD)
COMMIT_DATE = $$system(git show -s --pretty='%ci')
COMMIT_DATE = $$first(COMMIT_DATE)
DEFINES += APP_VERSION=\\\"$$COMMIT_DATE-$$COMMIT_BRANCH\\\"
CONFIG += c++14

LIBS += -L$$(HOME)/opt/lib

QMAKE_CXXFLAGS += -std=c++14 -Wextra -Winit-self -Wold-style-cast -Woverloaded-virtual -Wuninitialized -Winit-self -pedantic-errors #-Werror
QMAKE_LFLAGS += -std=c++14
LIBS += -lasync++ -lpteros -lpteros_analysis -ltng_io -lboost_system -lboost_thread

#win32:{
#    QTPLUGIN += qsvg
#    INCLUDEPATH += /home/dimura/opt/Pteros-win/include
#    LIBS += -L"/home/dimura/opt/Pteros-win/lib"
#    LIBS += -L"/usr/x86_64-w64-mingw32/plugins/accessible"
#    LIBS += -L"/usr/x86_64-w64-mingw32/plugins/imageformats"
#    LIBS += -L"/usr/x86_64-w64-mingw32/plugins/platforms"
#}



RESOURCES += \
    icons.qrc

FORMS += \
    mainwindow.ui

HEADERS += \
    PositionTableModel.h \
    DistanceTableModel.h \
    DomainTableModel.h \
    mainwindow.h \
    DistanceDelegate.h \
    AV/av_routines.h \
    AV/Distance.h \
    AV/MolecularSystemDomain.h \
    AV/Position.h \
    AV/PositionSimulation.h \
    AV/PositionSimulationResult.h \
    combination.hpp \
    AbstractCalcResult.h \
    CalcResult.h \
    FrameDescriptor.h \
    MolecularTrajectory.h \
    TrajectoriesTreeModel.h \
    TrajectoriesTreeItem.h \
    AbstractEvaluator.h \
    TaskStorage.h \
    EvaluatorTrasformationMatrix.h \
    EvaluatorEulerAngle.h \
    PterosSystemLoader.h

SOURCES += \
    PositionTableModel.cpp \
    DistanceTableModel.cpp \
    DomainTableModel.cpp \
    main.cpp \
    mainwindow.cpp \
    DistanceDelegate.cpp \
    AV/av_routines.cpp \
    AV/Distance.cpp \
    AV/MolecularSystemDomain.cpp \
    AV/Position.cpp \
    AV/PositionSimulation.cpp \
    AV/PositionSimulationResult.cpp \
    AbstractCalcResult.cpp \
    CalcResult.cpp \
    FrameDescriptor.cpp \
    MolecularTrajectory.cpp \
    TrajectoriesTreeModel.cpp \
    TrajectoriesTreeItem.cpp \
    AbstractEvaluator.cpp \
    TaskStorage.cpp \
    EvaluatorTrasformationMatrix.cpp \
    EvaluatorEulerAngle.cpp \
    PterosSystemLoader.cpp

DISTFILES += \
    .gitignore

