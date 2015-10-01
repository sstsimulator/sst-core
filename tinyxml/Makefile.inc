# -*- Makefile -*-
#
#

sst_xml_sources = \
	tinyxml/tinystr.cpp \
	tinyxml/tinyxml.cpp \
	tinyxml/tinyxmlerror.cpp \
	tinyxml/tinyxmlparser.cpp \
	tinyxml/tinystr.h \
	tinyxml/tinyxml.h

EXTRA_DIST += \
	tinyxml/Makefile.orig \
	tinyxml/changes.txt \
	tinyxml/demotest.xml \
	tinyxml/echo.dsp \
	tinyxml/readme.txt \
	tinyxml/test5.xml \
	tinyxml/test6.xml \
	tinyxml/test7.xml \
	tinyxml/textfile.txt \
	tinyxml/tinyXmlTest.dsp \
	tinyxml/tinyXmlTest.vcproj \
	tinyxml/tinyXmlTestSTL.dsp \
	tinyxml/tinyXmlTestSTL.vcproj \
	tinyxml/tinyxml.dsw \
	tinyxml/tinyxml.sln \
	tinyxml/tinyxmlSTL.dsp \
	tinyxml/tinyxmlSTL.vcproj \
	tinyxml/tinyxml_lib.dsp \
	tinyxml/tinyxml_lib.vcproj \
	tinyxml/utf8test.gif \
	tinyxml/utf8test.xml \
	tinyxml/utf8testout.xml \
	tinyxml/utf8testverify.xml \
	tinyxml/xmltest.cpp \
	tinyxml/docs

dist-hook:
	rm -rf `find $(distdir)/tinyxml/docs -name .svn`

# vim:ft=make
