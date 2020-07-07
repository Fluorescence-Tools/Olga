! include( ../common.pri ) {
    error( "Couldn't find the common.pri file!" )
}

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = Olga
TEMPLATE = app


DEPENDPATH += . ../irmsd
INCLUDEPATH += ../irmsd

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../irmsd/release/ -lirmsd
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../irmsd/debug/ -lirmsd
else:unix: LIBS += -L$$OUT_PWD/../irmsd/ -lirmsd


RESOURCES += icons.qrc

FORMS += \
    AtomSelectionTestDialog.ui \
    loadtrajectorydialog.ui \
    mainwindow.ui \
    BatchLPDialog.ui \
    BatchDistanceDialog.ui \
    GetInformativePairsDialog.ui \
    BatchFretEfficiencyDialog.ui

HEADERS += \
    AtomSelectionTestDialog.h \
    loadtrajectorydialog.h \
    mainwindow.h \
    Q_DebugStream.h \
    EvaluatorSphereAVOverlap.h \
    AV/fretAV.h \
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
    EvaluatorDistanceDistribution.h \
    EvaluatorMinDistance.h \
    EvaluatorChi2.h \
    EvaluatorsTreeModel.h \
    EvaluatorDelegate.h \
    CheckBoxList.h \
    EvaluatorWeightedResidual.h \
    EvaluatorChi2r.h \
    EvaluatorChi2Contribution.h \
    EvaluatorAvFile.h \
    EvaluatorFretEfficiency.h \
    EvaluatorAvVolume.h \
    BatchLPDialog.h \
    BatchDistanceDialog.h \
    GetInformativePairsDialog.h \
    BatchFretEfficiencyDialog.h \
    best_dist.h \
    split_string.h \
    chisqdist.hpp \
    spline.hpp \
    polynomial.hpp

SOURCES += \
    AtomSelectionTestDialog.cpp \
    loadtrajectorydialog.cpp \
    main.cpp \
    mainwindow.cpp \
    EvaluatorSphereAVOverlap.cpp \
    AV/fretAV.cpp \
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
    EvaluatorDistanceDistribution.cpp \
    EvaluatorMinDistance.cpp \
    EvaluatorChi2.cpp \
    EvaluatorsTreeModel.cpp \
    EvaluatorDelegate.cpp \
    CheckBoxList.cpp \
    EvaluatorWeightedResidual.cpp \
    EvaluatorChi2r.cpp \
    EvaluatorChi2Contribution.cpp \
    EvaluatorAvFile.cpp \
    EvaluatorFretEfficiency.cpp \
    EvaluatorAvVolume.cpp \
    BatchLPDialog.cpp \
    BatchDistanceDialog.cpp \
    GetInformativePairsDialog.cpp \
    BatchFretEfficiencyDialog.cpp
