PROJECT(minizip C)

IF(WIN32)
	# Use extlib/zlib/.
	SET(ZLIB_LIBRARY zlibstatic)
ELSE(WIN32)
	FIND_LIBRARY(ZLIB REQUIRED)
ENDIF(WIN32)

# Sources.
SET(minizip_SRCS
	unzip.c
	zip.c
	ioapi.c
	)
SET(minizip_H
	crypt.h
	ioapi.h
	unzip.h
	zip.h

	# Custom headers.
	mz_of.h
	)

IF(WIN32)
	# Windows-specific I/O handler.
	SET(minizip_SRCS ${minizip_SRCS} iowin32.c)
	SET(minizip_H ${minizip_H} iowin32.h)
ENDIF(WIN32)

######################
# Build the library. #
######################

ADD_LIBRARY(minizip STATIC ${minizip_SRCS})
TARGET_LINK_LIBRARIES(minizip ${ZLIB_LIBRARY})
TARGET_INCLUDE_DIRECTORIES(minizip
	PRIVATE ${ZLIB_INCLUDE_DIR}
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	)
TARGET_COMPILE_DEFINITIONS(minizip PRIVATE ZLIB_CONST)

# Exclude from ALL builds.
SET_TARGET_PROPERTIES(minizip PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	)
