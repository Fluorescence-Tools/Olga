TEMPLATE = subdirs

SUBDIRS = gui \
          av2restraints \
          irmsd \
          screen-nox 
gui.subdir = src/gui
av2restraints.subdir = src/av2restraints
irmsd.subdir = src/irmsd
screen-nox.subdir = src/screen-nox
gui.depends = irmsd

DISTFILES += \
    .gitignore \
    .gitlab-ci.yml \
    .clang-format \
    src/vdWRadii.json \
    src/weighting_function.csv \
    README.md \
    LICENSE
