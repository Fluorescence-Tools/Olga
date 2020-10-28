# Olga
Software for FRET-based screening of conformations and experiment planning with a graphical user interface

[**Download (Windows/Linux)**][release]

# General description
Olga  is a program for FRET-assisted structural modelling [![Dimura et all 2020](https://img.shields.io/badge/DOI-10.1038%2Fs41467--020--19023--1-blue)][Dimura2020]. Olga allows to compare conformational models to FRET distances and plan efficient FRET-experiments.
In order to calculate FRET distances, Olga simulates the behaviour of the label and its linker using the Accessible Volume (AV) model[![Kalinin2012](https://img.shields.io/badge/DOI-10.1038%2Fnmeth.2222-blue.svg)][Kalinin2012] [![Linker](https://img.shields.io/badge/DOI-10.1021%2Fja105725e-blue.svg)](https://doi.org/10.1021/ja105725e). AV model simulates a small probe, flexibly coupled to a biomolecule. Typically, this label is a fluorescent dye. AV represents the sterically accessible volume of the probe considering the linker length and the spatial dimensions of the probe. The linker, which connects the probe to the biomolecule, is approximated by a tube, and the probe itself is approximated by a sphere.
Olga uses AV to calculate the spatial distributions of flexible labels around attachment points, correspinding AV to AV distance distributions and FRET observables (apparent distances, FRET Efficiencies, etc).
In order to plan an efficient FRET experiment, Olga requires an initial conformational ensemble that represents the uncertainty of the structural model (prior). Given the initial ensemble, it finds a set of informative FRET pairs which would reduce the uncertainty in the initial ensemble as much as possible.

![AV clouds](/doc/screening%20tutorial/DNA_AVs.jpg)

# Installation
Windows version of Olga software can be downloaded from the release section of this repository. In order to install Olga software on Windows it is enough to extract the [Olga_win64_*.zip][release] archive. Olga can then be started from `Olga.exe` executable file.

# Documentation
Documentation and usage examples are available in the [doc](/doc/) folder of this repository. The documentation covers several common use cases:

 * [FRET-screening with organic dyes (e.g. Alexa and Cy5)](/doc/screening%20tutorial/screening%20tutorial.md)
 * [Selection of informative FRET pairs (experiment planning)](/doc/FRET%20pair%20selection%20tutorial/FRET%20pair%20selection%20tutorial.md)
 * [FRET-screening with fluorescent protein dyes](/doc/screening%20with%20fluorescent%20proteins/Screening%20and%20AV%20saving.md#tutorial-screening-of-structural-models-and-generation-of-accessible-volumes)

Exhaustive list of all parameters and their descriptions can be found at [doc/JSON Types and Parameters.docx](/doc/JSON%20Types%20and%20Parameters.docx)

# Dependencies

 * cmake
 * boost
 * qt5
 * [pteros](http://pteros.sourceforge.net/)
 * [libcuckoo](https://github.com/efficient/libcuckoo)
 * [readerwriterqueue](https://github.com/cameron314/readerwriterqueue)
 * [async++](https://github.com/Amanieu/asyncplusplus)

# Citation

If you used Olga in a scientific publication, we would appreciate citations to the following papers [![Dimura et all 2020](https://img.shields.io/badge/DOI-10.1038%2Fs41467--020--19023--1-blue)][Dimura2020]:

> Dimura, M., Peulen, T., Sanabria, H., Rodnin, D., Hemmen, K., Hanke, C., Seidel, C. A. M., and Gohlke, H. Automated and optimally FRET-assisted structural modeling. Nat. Commun. 11 (2020); doi: 10.1038/s41467-020-19023-1

[FPS toolkit][Kalinin2012], the predecessor of the Olga software [![FPS](https://img.shields.io/badge/DOI-10.1038%2Fnmeth.2222-blue.svg)][Kalinin2012]:

> Kalinin, S., Peulen, T., Sindbert, S., Rothwell, P.J., Berger, S., Restle, T., Goody, R.S., Gohlke, H. and Seidel, C.A., 2012. A toolkit and benchmark study for FRET-restrained high-precision structural modeling. Nature methods, 9(12), pp.1218-1225.

# See also

[LabelLib][LabelLib] - a C++/Python library for the simulation of small probes flexibly coupled to biomolecules.

Page of [Claus Seidel's research group](http://www.mpc.hhu.de/) for more fluorescence-related software and additional information.


[Dimura2020]: https://doi.org/10.1038/s41467-020-19023-1
[release]: https://github.com/Fluorescence-Tools/Olga/releases/latest
[Kalinin2012]: https://doi.org/10.1038/nmeth.2222
[LabelLib]: https://github.com/Fluorescence-Tools/LabelLib
