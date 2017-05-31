TEMPLATE = subdirs

SUBDIRS = gui \
	  av2restraints \
    irmsd \
    screen-nox

 gui.depends = irmsd
