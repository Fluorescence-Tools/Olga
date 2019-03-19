! include( ../common.pri ) {
    error( "Couldn't find the common.pri file!" )
}

QT += core
QT -= gui
LIBS   -= -lQtGui

TARGET = screen-nox
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../
VPATH += ../

SOURCES += \
    main.cpp \
    AV/fretAV.cpp \
    AV/Distance.cpp \
    AV/MolecularSystemDomain.cpp \
    AV/Position.cpp \
    AV/PositionSimulation.cpp \
    AV/PositionSimulationResult.cpp \
    TaskStorage.cpp \
    EvaluatorAvFile.cpp \
    EvaluatorAvVolume.cpp \
    EvaluatorChi2.cpp \
    EvaluatorChi2Contribution.cpp \
    EvaluatorChi2r.cpp \
    EvaluatorDistance.cpp \
    EvaluatorDistanceDistribution.cpp \
    EvaluatorEulerAngle.cpp \
    EvaluatorFretEfficiency.cpp \
    EvaluatorMinDistance.cpp \
    EvaluatorPositionSimulation.cpp \
    EvaluatorSphereAVOverlap.cpp \
    EvaluatorTrasformationMatrix.cpp \
    EvaluatorWeightedResidual.cpp \
    FrameDescriptor.cpp \
    PterosSystemLoader.cpp \
    AbstractCalcResult.cpp \
    CalcResult.cpp \
    AbstractEvaluator.cpp

HEADERS += \
    AV/fretAV.h \
    AV/Distance.h \
    AV/MolecularSystemDomain.h \
    AV/Position.h \
    AV/PositionSimulation.h \
    AV/PositionSimulationResult.h \
    TaskStorage.h \
    EvaluatorAvFile.h \
    EvaluatorAvVolume.h \
    EvaluatorChi2.h \
    EvaluatorChi2Contribution.h \
    EvaluatorChi2r.h \
    EvaluatorDistance.h \
    EvaluatorDistanceDistribution.h \
    EvaluatorEulerAngle.h \
    EvaluatorFretEfficiency.h \
    EvaluatorMinDistance.h \
    EvaluatorPositionSimulation.h \
    EvaluatorSphereAVOverlap.h \
    EvaluatorTrasformationMatrix.h \
    EvaluatorWeightedResidual.h \
    FrameDescriptor.h \
    PterosSystemLoader.h \
    AbstractCalcResult.h \
    CalcResult.h \
    AbstractEvaluator.h
