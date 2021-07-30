# -*- Makefile -*-
#
#

# This is going to be included by the makefile in src/sst/core/ 
# so our relative path is src/sst/core/../../../external/tinyxml
# so I'm going to prepend with ../../../

sst_xml_sources = \
	../../../external/tinyxml/tinystr.cpp \
	../../../external/tinyxml/tinyxml.cpp \
	../../../external/tinyxml/tinyxmlerror.cpp \
	../../../external/tinyxml/tinyxmlparser.cpp \
	../../../external/tinyxml/tinystr.h \
	../../../external/tinyxml/tinyxml.h

EXTRA_DIST += \
	../../../external/tinyxml/Makefile.orig \
	../../../external/tinyxml/changes.txt \
	../../../external/tinyxml/demotest.xml \
	../../../external/tinyxml/echo.dsp \
	../../../external/tinyxml/readme.txt \
	../../../external/tinyxml/test5.xml \
	../../../external/tinyxml/test6.xml \
	../../../external/tinyxml/test7.xml \
	../../../external/tinyxml/textfile.txt \
	../../../external/tinyxml/tinyXmlTest.dsp \
	../../../external/tinyxml/tinyXmlTest.vcproj \
	../../../external/tinyxml/tinyXmlTestSTL.dsp \
	../../../external/tinyxml/tinyXmlTestSTL.vcproj \
	../../../external/tinyxml/tinyxml.dsw \
	../../../external/tinyxml/tinyxml.sln \
	../../../external/tinyxml/tinyxmlSTL.dsp \
	../../../external/tinyxml/tinyxmlSTL.vcproj \
	../../../external/tinyxml/tinyxml_lib.dsp \
	../../../external/tinyxml/tinyxml_lib.vcproj \
	../../../external/tinyxml/utf8test.gif \
	../../../external/tinyxml/utf8test.xml \
	../../../external/tinyxml/utf8testout.xml \
	../../../external/tinyxml/utf8testverify.xml \
	../../../external/tinyxml/xmltest.cpp \
	../../../external/tinyxml/docs

dist-hook:
	rm -rf `find $(distdir)/tinyxml/docs -name .svn`

# vim:ft=make
