# inih: Simple .INI file parser in C
PROJECT(inih VERSION 51 LANGUAGES C)

CMAKE_POLICY(SET CMP0063 NEW)
SET(CMAKE_C_VISIBILITY_PRESET "hidden")
SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

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
