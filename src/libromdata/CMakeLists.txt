PROJECT(libromdata)
# CMake 2.8.11 adds TARGET_INCLUDE_DIRECTORIES() and Qt4::xxx targets, similar to Qt5.
# CMake 2.8.12 automates adding compile flags for Qt5, e.g. -fPIC on Linux.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

# On Win32, we use MultiByteToWideChar() and WideCharToMultiByte().
# On other systems, we use iconv(), which could be in libc or libiconv.
# Figure out which library has iconv().
IF(NOT WIN32)
	# First, check libc for iconv().
	INCLUDE(CheckLibraryExists)
	CHECK_LIBRARY_EXISTS(c iconv "" HAVE_ICONV_C)	# libc: iconv()
	IF(HAVE_ICONV_C)
		# iconv() found in libc.
		UNSET(ICONV_LIBRARY)
		SET(HAVE_ICONV 1)
	ELSE(HAVE_ICONV_C)
		# iconv() not found in libc.
		# Check in libiconv.
		CHECK_LIBRARY_EXISTS(iconv iconv "" HAVE_ICONV_LIBICONV)	# libiconv: iconv()
		IF(HAVE_ICONV_LIBICONV)
			# iconv() found in libiconv.
			SET(ICONV_LIBRARY "iconv")
			SET(HAVE_ICONV 1)
		ENDIF(HAVE_ICONV_LIBICONV)
	ENDIF(HAVE_ICONV_C)

	IF(NOT HAVE_ICONV)
		MESSAGE(FATAL_ERROR "iconv() not found, cannot continue.")
	ENDIF(NOT HAVE_ICONV)
ENDIF(NOT WIN32)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libromdata.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libromdata.h")

SET(libromdata_SRCS
	TextFuncs.cpp
	RomData.cpp
	RomFields.cpp
	MegaDrive.cpp
	MegaDrivePublishers.cpp
	GameCube.cpp
	NintendoPublishers.cpp
	RomDataFactory.cpp
	)
SET(libromdata_H
	byteorder.h
	byteswap.h
	RomData.hpp
	RomFields.hpp
	TextFuncs.hpp
	MegaDrive.hpp
	MegaDrivePublishers.hpp
	GameCube.hpp
	NintendoPublishers.hpp
	RomDataFactory.hpp
	)

######################
# Build the library. #
######################

# UTF-8 version.
ADD_LIBRARY(romdata8 STATIC ${libromdata_SRCS} ${libromdata_H})
TARGET_INCLUDE_DIRECTORIES(romdata8
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
TARGET_COMPILE_DEFINITIONS(romdata8 PUBLIC -DRP_UTF8)
SET_TARGET_PROPERTIES(romdata8 PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
        )

# UTF-16 version.
ADD_LIBRARY(romdata16 STATIC ${libromdata_SRCS} ${libromdata_H})
TARGET_INCLUDE_DIRECTORIES(romdata16
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
TARGET_COMPILE_DEFINITIONS(romdata16 PUBLIC -DRP_UTF16)
SET_TARGET_PROPERTIES(romdata16 PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
        )

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
