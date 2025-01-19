# Minimal version of libmspack from Xenia.
# We have to use our own version because the standard version
# doesn't export the LZX symbols directly.
# References:
# - Xenia commit: e706cf0d5413c31b7e80a50411cc88cc7c71af30
# - https://github.com/xenia-project/xenia/tree/master/third_party/mspack
# - https://github.com/xenia-project/xenia/blob/master/src/xenia/cpu/lzx.cc
# - https://github.com/xenia-project/xenia/blob/master/src/xenia/cpu/lzx.h
PROJECT(libmspack C)

# Configuration.
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
CHECK_INCLUDE_FILES(inttypes.h HAVE_INTTYPES_H)
CHECK_SYMBOL_EXISTS(fseeko "stdio.h" HAVE_FSEEKO)
CHECK_TYPE_SIZE(off_t SIZEOF_OFF_T)
CONFIGURE_FILE(config.h.in config.h)

# rom-properties: Hide symbols by default, since we don't want them
# leaking from the static library to the plugins.
CMAKE_POLICY(SET CMP0063 NEW)
SET(CMAKE_C_VISIBILITY_PRESET "hidden")
SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Sources.
SET(libmspack_SRCS
	lzxd.c
	system.c
	xenia_lzx.c
	)
# Headers.
SET(libmspack_H
	lzx.h
	mspack.h
	readbits.h
	readhuff.h
	system.h
	xenia_lzx.h
	)

######################
# Build the library. #
######################

ADD_LIBRARY(mspack STATIC ${libmspack_SRCS} ${libmspack_H})
TARGET_INCLUDE_DIRECTORIES(mspack
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	)
