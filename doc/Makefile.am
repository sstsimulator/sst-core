# -*- Makefile -*-
#
#

EXTRA_DIST = html

html-local: doxygen-html

dist-hook: doxygen-dist

maintainer-clean-local:
	@ $(RM) -rf html

distclean-local:
	@ $(RM) -rf html tagfile

if HAVE_DOXYGEN
doxygen-html:
	@ pushd $(top_srcdir) ; @DOXYGEN@ $(abs_builddir)/Doxyfile ; popd

doxygen-dist: doxygen-html

else
doxygen-html:
	@ echo "Doxygen not installed, can not generate HTML output"
	@ exit 1

doxygen-dist:
	@ echo "Doxygen not installed.  Can not build distribution tarball."
	@ exit 1
endif
