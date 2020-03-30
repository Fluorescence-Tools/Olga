TEMPLATE = subdirs

SUBDIRS = gui \
          av2restraints \
          irmsd \
          screen-nox  \
          bestpairs

gui.subdir = src/gui
av2restraints.subdir = src/av2restraints
irmsd.subdir = src/irmsd
screen-nox.subdir = src/screen-nox
bestpairs.subdir = src/bestpairs
gui.depends = irmsd
bestpairs.depends = irmsd

DISTFILES += \
    .gitignore \
    .gitlab-ci.yml \
    src/pack_mingw.sh \
    src/pack_ubuntu.sh \
    .clang-format \
    src/qt_c_code-style_linux-kernel.xml \
    src/vdWRadii.json \
    src/weighting_function.csv \
    README.md \
    LICENSE
