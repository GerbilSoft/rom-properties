# inih: Simple .INI file parser in C
PROJECT(inih VERSION 51 LANGUAGES C)

# rom-properties: Hide symbols by default, since we don't want them
# leaking from the static library to the plugins.
INCLUDE(CheckHiddenVisibility)
CHECK_HIDDEN_VISIBILITY()

# Sources.
SET(inih_SRCS ini.c)
SET(inih_H ini.h)

######################
# Build the library. #
######################

ADD_LIBRARY(inih STATIC ${inih_SRCS})
TARGET_INCLUDE_DIRECTORIES(inih
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	)
# inih is not exported by libromdata, so disable
# its symbol visibility definitions.
TARGET_COMPILE_DEFINITIONS(inih PUBLIC INI_API=)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
