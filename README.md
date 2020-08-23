# Olga
Software for FRET-based screening of conformations and experiment planning with a graphical user interface

[**Download (Windows/Linux)**][1]

# General description
Olga is a program for the simulation of small probes flexibly coupled to biomolecules and analysis of inter-label distance distributions. Olga software calculates the spatial distribution of flexible labels around attachment points. Typically, these labels are fluorescent dyes. Olga can also calculate FRET observables (apparent distances, FRET Efficiencies, etc).
The software uses uses a coarse-grained approach to simulate the spatial distribution of probes around their attachment points. In this coarse-grained approach, software determines the sterically accessible volume of the probe considering the linker length and the spatial dimensions of the probe. The linker, which connects the probe to the biomolecule, is approximated by a tube, and the probe itself is approximated by soft sphere. Details are provided in the publications [![DOI for citing FPS](https://img.shields.io/badge/DOI-10.1038%2Fnmeth.2222-blue.svg)](https://doi.org/10.1038/nmeth.2222)[![DOI for citing FPS](https://img.shields.io/badge/DOI-10.1021%2Fja105725e-blue.svg)](https://doi.org/10.1021/ja105725e).

![AV clouds](/doc/screening%20tutorial/DNA_AVs.jpg)

# Installation
Windows version of Olga software can be downloaded from the release section of this repository. In order to install Olga software on Windows it is enough to extract the [Olga_win64_*.zip][1] archive. Olga can then be started from `Olga.exe` executable file.

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

If you have used LabelLib in a scientific publication, we would appreciate citations to the following papers:

[FPS toolkit][2], the predecessor of the Olga software [![DOI for citing FPS](https://img.shields.io/badge/DOI-10.1038%2Fnmeth.2222-blue.svg)][2]:

> Kalinin, S., Peulen, T., Sindbert, S., Rothwell, P.J., Berger, S., Restle, T., Goody, R.S., Gohlke, H. and Seidel, C.A., 2012. A toolkit and benchmark study for FRET-restrained high-precision structural modeling. Nature methods, 9(12), pp.1218-1225.

See the [page of C. Seidel research group](http://www.mpc.hhu.de/) for more fluorescence-related software and additional information.

# See also

[LabelLib][3] - a C++/Python library for the simulation of small probes flexibly coupled to biomolecules

[1]: https://github.com/Fluorescence-Tools/Olga/releases/latest
[2]: https://doi.org/10.1038/nmeth.2222
[3]: https://github.com/Fluorescence-Tools/LabelLib
