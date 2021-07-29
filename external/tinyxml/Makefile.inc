# -*- Makefile -*-
#
#

sst_xml_sources = \
	$(top_srcdir)/external/tinyxml/tinystr.cpp \
	$(top_srcdir)/external/tinyxml/tinyxml.cpp \
	$(top_srcdir)/external/tinyxml/tinyxmlerror.cpp \
	$(top_srcdir)/external/tinyxml/tinyxmlparser.cpp \
	$(top_srcdir)/external/tinyxml/tinystr.h \
	$(top_srcdir)/external/tinyxml/tinyxml.h

EXTRA_DIST += \
	$(top_srcdir)/external/tinyxml/Makefile.orig \
	$(top_srcdir)/external/tinyxml/changes.txt \
	$(top_srcdir)/external/tinyxml/demotest.xml \
	$(top_srcdir)/external/tinyxml/echo.dsp \
	$(top_srcdir)/external/tinyxml/readme.txt \
	$(top_srcdir)/external/tinyxml/test5.xml \
	$(top_srcdir)/external/tinyxml/test6.xml \
	$(top_srcdir)/external/tinyxml/test7.xml \
	$(top_srcdir)/external/tinyxml/textfile.txt \
	$(top_srcdir)/external/tinyxml/tinyXmlTest.dsp \
	$(top_srcdir)/external/tinyxml/tinyXmlTest.vcproj \
	$(top_srcdir)/external/tinyxml/tinyXmlTestSTL.dsp \
	$(top_srcdir)/external/tinyxml/tinyXmlTestSTL.vcproj \
	$(top_srcdir)/external/tinyxml/tinyxml.dsw \
	$(top_srcdir)/external/tinyxml/tinyxml.sln \
	$(top_srcdir)/external/tinyxml/tinyxmlSTL.dsp \
	$(top_srcdir)/external/tinyxml/tinyxmlSTL.vcproj \
	$(top_srcdir)/external/tinyxml/tinyxml_lib.dsp \
	$(top_srcdir)/external/tinyxml/tinyxml_lib.vcproj \
	$(top_srcdir)/external/tinyxml/utf8test.gif \
	$(top_srcdir)/external/tinyxml/utf8test.xml \
	$(top_srcdir)/external/tinyxml/utf8testout.xml \
	$(top_srcdir)/external/tinyxml/utf8testverify.xml \
	$(top_srcdir)/external/tinyxml/xmltest.cpp \
	$(top_srcdir)/external/tinyxml/docs

dist-hook:
	rm -rf `find $(distdir)/tinyxml/docs -name .svn`

# vim:ft=make
