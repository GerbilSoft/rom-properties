PROJECT(inih)

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

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
