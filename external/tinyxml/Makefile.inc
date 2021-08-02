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

dist-hook:
	rm -rf `find $(distdir)/tinyxml/docs -name .svn`

# vim:ft=make
