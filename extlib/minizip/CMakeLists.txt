PROJECT(minizip C)

IF(NOT ZLIB_LIBRARY)
	MESSAGE(FATAL_ERROR "ZLIB_LIBRARY has not been set by CheckZLIB.cmake.")
ENDIF(NOT ZLIB_LIBRARY)

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

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
