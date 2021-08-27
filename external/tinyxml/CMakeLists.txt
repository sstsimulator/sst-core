add_library(tinyxml OBJECT
	tinystr.cpp
	tinyxml.cpp
	tinyxmlerror.cpp
	tinyxmlparser.cpp
  )

target_compile_definitions(tinyxml PUBLIC TIXML_USE_STL)
target_include_directories(tinyxml
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${sst-core_SOURCE_DIR}/../external
  PRIVATE
  ${sst-core_SOURCE_DIR}/../src # For warning suppression
)
