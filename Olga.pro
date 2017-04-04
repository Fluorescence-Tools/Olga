TEMPLATE = subdirs

SUBDIRS = gui \
	  av2restraints \
    irmsd

 gui.depends = irmsd
