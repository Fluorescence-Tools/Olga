#-------------------------------------------------
#
# Project created by QtCreator 2014-02-27T12:04:31
#
#-------------------------------------------------
QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = Olga
TEMPLATE = app

QMAKE_CXXFLAGS -= -O
QMAKE_CXXFLAGS -= -O1
QMAKE_CXXFLAGS -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3 -march=native -flto -fwhole-program
QMAKE_LFLAGS -= -O1
QMAKE_LFLAGS *= -O3 -flto -fwhole-program

COMMIT_BRANCH = $$system(git rev-parse --abbrev-ref HEAD)
COMMIT_DATE = $$system(git show -s --pretty='%ci')
COMMIT_DATE = $$first(COMMIT_DATE)
COMMIT_HASH = $$system(git log --pretty=format:'%h' -n 1)
DEFINES += APP_VERSION=\\\"$$COMMIT_DATE-$$COMMIT_BRANCH-$$COMMIT_HASH\\\"
CONFIG += c++14
CONFIG += no_keywords

QMAKE_CXXFLAGS += -std=c++14 -Wextra -Winit-self -Wold-style-cast \
-Woverloaded-virtual -Wuninitialized -Winit-self -pedantic-errors -Wno-attributes#-Werror
LIBS += -lasync++ -lpteros -lpteros_analysis -ltng_io

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
    PterosSystemLoader.h \
    EvaluatorPositionSimulation.h \
    EvaluatorDistance.h \
    EvaluatorChi2.h \
    Q_DebugStream.h

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
    PterosSystemLoader.cpp \
    EvaluatorPositionSimulation.cpp \
    EvaluatorDistance.cpp \
    EvaluatorChi2.cpp

DISTFILES += \
    .gitignore \
    vdWRadii.json

