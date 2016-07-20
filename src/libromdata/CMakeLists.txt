PROJECT(libromdata)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

# TODO: Check for iconv on non-Windows systems.

SET(libromdata_SRCS
	MegaDrive.cpp
	)
SET(libromdata_H
	byteorder.h
	byteswap.h
	MegaDrive.hpp
	)

######################
# Build the library. #
######################

ADD_LIBRARY(libromdata STATIC
	${libromdata_SRCS}
	${libromdata_H}
	)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
