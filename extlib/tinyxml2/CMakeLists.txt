PROJECT(tinyxml2 CXX)

ADD_LIBRARY(tinyxml2 tinyxml2.cpp tinyxml2.h)
TARGET_INCLUDE_DIRECTORIES(tinyxml2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
