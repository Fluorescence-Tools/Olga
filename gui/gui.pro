! include( ../common.pri ) {
    error( "Couldn't find the common.pri file!" )
}

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = Olga
TEMPLATE = app

RESOURCES += \
    icons.qrc

FORMS += \
    mainwindow.ui

HEADERS += \
    mainwindow.h \
    Q_DebugStream.h \
    EvaluatorChi2Contribution.h

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    EvaluatorChi2Contribution.cpp

HEADERS += \
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
    EvaluatorsTreeModel.h \
    EvaluatorDelegate.h \
    CheckBoxList.h \
    EvaluatorWeightedResidual.h

SOURCES += \
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
    EvaluatorChi2.cpp \
    EvaluatorsTreeModel.cpp \
    EvaluatorDelegate.cpp \
    CheckBoxList.cpp \
    EvaluatorWeightedResidual.cpp
