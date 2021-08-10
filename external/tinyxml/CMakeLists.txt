add_library(libtinyxml OBJECT
	tinystr.cpp
	tinyxml.cpp
	tinyxmlerror.cpp
	tinyxmlparser.cpp
  )

target_compile_definitions(libtinyxml PUBLIC TIXML_USE_STL)
target_include_directories(libtinyxml
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/external
  PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)
