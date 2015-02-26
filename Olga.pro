#-------------------------------------------------
#
# Project created by QtCreator 2014-02-27T12:04:31
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Olga

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++1y #-pthread -Wl,--no-as-needed
QMAKE_LFLAGS += -std=c++1y #-pthread -Wl,--no-as-needed

INCLUDEPATH += /usr/include/eigen3

unix:{
INCLUDEPATH += /home/dimura/opt/Pteros/include
LIBS += -L"/home/dimura/opt/Pteros/lib"
}

win32:{
QTPLUGIN += qsvg
INCLUDEPATH += /home/dimura/opt/Pteros-win/include
LIBS += -L"/home/dimura/opt/Pteros-win/lib"
LIBS += -L"/usr/x86_64-w64-mingw32/plugins/accessible"
LIBS += -L"/usr/x86_64-w64-mingw32/plugins/imageformats"
LIBS += -L"/usr/x86_64-w64-mingw32/plugins/platforms"
}

LIBS += -lpteros -lpteros_analysis -ltng_io -lboost_system -lboost_thread

RESOURCES += \
    icons.qrc

FORMS += \
    mainwindow.ui

HEADERS += \
    PositionTableModel.h \
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
    AV/PositionSimulationResult.h \
    combination.hpp \
    AbstractCalcResult.h \
    CalcResult.h \
    AbstractCalculator.h \
    FrameDescriptor.h \
    MolecularTrajectory.h \
    TrajectoriesTreeModel.h \
    TrajectoriesTreeItem.h \
    CalculatorTrasformationMatrix.h \
    ThreadPool.h \
    CalculatorEulerAngle.h \
    CalculatorPositionSimulation.h \
    CalculatorDistance.h

SOURCES += \
    PositionTableModel.cpp \
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
    AV/PositionSimulationResult.cpp \
    AbstractCalcResult.cpp \
    CalcResult.cpp \
    AbstractCalculator.cpp \
    FrameDescriptor.cpp \
    MolecularTrajectory.cpp \
    TrajectoriesTreeModel.cpp \
    TrajectoriesTreeItem.cpp \
    CalculatorTrasformationMatrix.cpp \
    ThreadPool.cpp \
    CalculatorEulerAngle.cpp \
    CalculatorPositionSimulation.cpp \
    CalculatorDistance.cpp

