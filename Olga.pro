#-------------------------------------------------
#
# Project created by QtCreator 2014-02-27T12:04:31
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Olga

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11 #-pthread -Wl,--no-as-needed
QMAKE_LFLAGS += -std=c++11 #-pthread -Wl,--no-as-needed

INCLUDEPATH += ../include

unix:{
INCLUDEPATH += /usr/include/eigen3 /home/dimura/opt/Pteros/include
LIBS += -L"/home/dimura/opt/Pteros/lib"
}

win32:{
INCLUDEPATH += C:/boost64/include/boost-1_56 ../include/Eigen C:/Pteros/include/
LIBS += -L"C:/Pteros/lib"
}

LIBS += -lpteros -lpteros_analysis -ltng_io

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
    DistanceDelegate.h \
    AV/av_routines.h \
    AV/Distance.h \
    AV/MolecularSystemDomain.h \
    AV/Position.h \
    AV/PositionSimulation.h \
    AV/PositionSimulationResult.h

SOURCES += \
    PositionTableModel.cpp \
    SystemsTableModel.cpp \
    DistanceTableModel.cpp \
    DomainTableModel.cpp \
    main.cpp \
    mainwindow.cpp \
    MolecularSystem.cpp \
    DistanceDelegate.cpp \
    AV/av_routines.cpp \
    AV/Distance.cpp \
    AV/MolecularSystemDomain.cpp \
    AV/Position.cpp \
    AV/PositionSimulation.cpp \
    AV/PositionSimulationResult.cpp

