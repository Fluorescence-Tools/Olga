#-------------------------------------------------
#
# Project created by QtCreator 2014-02-27T12:04:31
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Olga

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11 -pthread -Wl,--no-as-needed
QMAKE_LFLAGS += -std=c++11 -pthread -Wl,--no-as-needed


INCLUDEPATH += ../include ../include/AV

unix:{
INCLUDEPATH += /usr/include/eigen3
}

win32:{
INCLUDEPATH += ..\ext-qt-5.2.1-x64-mingw482r2-sjlj\include ..\ext-qt-5.2.1-x64-mingw482r2-sjlj\include\Eigen
LIBS += -L"..\ext-qt-5.2.1-x64-mingw482r2-sjlj\dlls"
}

LIBS += -lBALL

RESOURCES += \
    icons.qrc

FORMS += \
    mainwindow.ui

HEADERS += \
    PositionTableModel.h \
    SystemsTableModel.h \
    DistanceTableModel.h \
    DomainTableModel.h \
    mainwindow.h \
    MolecularSystem.h \
    ../include/AV/av_routines.h \
    DistanceDelegate.h

SOURCES += \
    PositionTableModel.cpp \
    SystemsTableModel.cpp \
    DistanceTableModel.cpp \
    DomainTableModel.cpp \
    main.cpp \
    mainwindow.cpp \
    MolecularSystem.cpp \
    ../include/AV/Distance.cpp \
    ../include/AV/MolecularSystemDomain.cpp \
    ../include/AV/Position.cpp \
    ../include/AV/PositionSimulation.cpp \
    ../include/AV/PositionSimulationResult.cpp \
    ../include/AV/av_routines.cpp \
    DistanceDelegate.cpp

