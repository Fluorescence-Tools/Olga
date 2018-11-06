# Olga
Software for FRET-based screening of conformations and experiment planning

[**Download (Windows x64)**][1]

# Installation
Windows version of Olga software can be downloaded from the release section of this repository. In order to install Olga software on Windows it is enough to extract the [Olga_win64_*.zip][1] archive. Olga can then be started from `Olga.exe` executable file.

# Documentation
Documentation and usage examples are available in the [doc](/doc/) folder of this repository. The documentaion covers several common use cases:

 * [FRET-screening with organic dyes (e.g. Alexa and Cy5)](/doc/screening%20tutorial/screening%20tutorial.md)
 * [Selection of informative FRET pairs (experiment planning)](/doc/screening%20tutorial/FRET%20pair%20selection%20tutorial.md)
 * [FRET-screening with fluorescent protein dyes](/doc/screening%20with%20fluorescent%20proteins/Screening%20and%20AV%20saving.md#tutorial-screening-of-structural-models-and-generation-of-accessible-volumes)

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

[1]: https://github.com/Fluorescence-Tools/Olga/releases/download/master-20171130/Olga_master-20171130_win64_3dd9922.zip
[2]: https://doi.org/10.1038/nmeth.2222
