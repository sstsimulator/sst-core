# -*- Makefile -*-
#
#

EXTRA_DIST = html

html-local: doxygen-html

dist-hook: doxygen-dist

maintainer-clean-local:
	@ $(RM) html

distclean-local:
	@ $(RM) html tagfile

if HAVE_DOXYGEN
doxygen-html:
	@ cd .. ; @DOXYGEN@ doc/Doxyfile

doxygen-dist: doxygen-html

else
doxygen-html:
	@ echo "Doxygen not installed, can not generate HTML output"
	@ exit 1

doxygen-dist:
	@ echo "Doxygen not installed.  Can not build distribution tarball."
	@ exit 1
endif
