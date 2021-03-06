! include( ../common.pri ) {
    error( "Couldn't find the common.pri file!" )
}

QT += core
QT -= gui
LIBS   -= -lQtGui

TEMPLATE = app
CONFIG += console

SOURCES += \
    main.cpp

HEADERS += \
    AV/fretAV.h \
    AV/Distance.h \
    AV/MolecularSystemDomain.h \
    EvaluatorSphereAVOverlap.h \
    AV/Position.h \
    AV/PositionSimulation.h \
    AV/PositionSimulationResult.h \
    combination.hpp \
    AbstractCalcResult.h \
    CalcResult.h \
    FrameDescriptor.h \
    MolecularTrajectory.h \
    AbstractEvaluator.h \
    TaskStorage.h \
    EvaluatorTrasformationMatrix.h \
    EvaluatorEulerAngle.h \
    EvaluatorPositionSimulation.h \
    EvaluatorDistance.h \
    EvaluatorDistanceDistribution.h \
    EvaluatorMinDistance.h \
    EvaluatorChi2.h \
    EvaluatorChi2Contribution.h \
    EvaluatorWeightedResidual.h \
    EvaluatorChi2r.h \
    EvaluatorAvFile.h \
    EvaluatorFretEfficiency.h \
    EvaluatorAvVolume.h \
    split_string.h

SOURCES += \
    AV/fretAV.cpp \
    AV/Distance.cpp \
    AV/MolecularSystemDomain.cpp \
    EvaluatorSphereAVOverlap.cpp \
    AV/Position.cpp \
    AV/PositionSimulation.cpp \
    AV/PositionSimulationResult.cpp \
    AbstractCalcResult.cpp \
    CalcResult.cpp \
    FrameDescriptor.cpp \
    MolecularTrajectory.cpp \
    AbstractEvaluator.cpp \
    TaskStorage.cpp \
    EvaluatorTrasformationMatrix.cpp \
    EvaluatorEulerAngle.cpp \
    PterosSystemLoader.cpp \
    EvaluatorPositionSimulation.cpp \
    EvaluatorDistance.cpp \
    EvaluatorDistanceDistribution.cpp \
    EvaluatorMinDistance.cpp \
    EvaluatorChi2.cpp \
    EvaluatorChi2Contribution.cpp \
    EvaluatorWeightedResidual.cpp \
    EvaluatorChi2r.cpp \
    EvaluatorAvFile.cpp \
    EvaluatorFretEfficiency.cpp \
    EvaluatorAvVolume.cpp
