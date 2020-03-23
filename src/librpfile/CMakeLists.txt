PROJECT(librpfile)

# Check for C library functions.
IF(NOT WIN32)
	# Check for statx().
	SET(OLD_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
	SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=1")
	CHECK_SYMBOL_EXISTS(statx "sys/stat.h" HAVE_STATX)
	SET(CMAKE_REQUIRED_DEFINITIONS "${OLD_CMAKE_REQUIRED_DEFINITIONS}")
	UNSET(OLD_CMAKE_REQUIRED_DEFINITIONS)
ENDIF(NOT WIN32)

# Sources.
SET(librpfile_SRCS
	IRpFile.cpp
	RpMemFile.cpp
	RpVectorFile.cpp
	FileSystem_common.cpp
	RelatedFile.cpp
	scsi/RpFile_Kreon.cpp
	scsi/RpFile_scsi.cpp
	)
# Headers.
SET(librpfile_H
	IRpFile.hpp
	RpFile.hpp
	RpFile_p.hpp
	RpMemFile.hpp
	RpVectorFile.hpp
	FileSystem.hpp
	RelatedFile.hpp
	scsi/ata_protocol.h
	scsi/scsi_protocol.h
	scsi/scsi_ata_cmds.h
	)

# SCSI implementation for Kreon disc drive support.
IF(WIN32)
	SET(librpfile_SRCS ${librpfile_SRCS} scsi/RpFile_scsi_win32.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(librpfile_SRCS ${librpfile_SRCS} scsi/RpFile_scsi_linux.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "Dragonfly")
	# FIXME: Not working properly. (SCSI ILLEGAL REQUEST errors)
	# NOTE: The file is still needed for rereadDeviceSizeOS().
	SET(librpfile_SRCS ${librpfile_SRCS} scsi/RpFile_scsi_freebsd.cpp)

	IF(0) # FIXME
	# TODO: Check for header files. CHECK_INCLUDE_FILE() doesn't work
	# properly because these header files use FILE*, so we need to
	# #include <stdio.h>.
	INCLUDE(CheckIncludeFile)
	INCLUDE(CheckLibraryExists)
	#CHECK_INCLUDE_FILE("camlib.h" HAVE_CAMLIB_H)
	#CHECK_INCLUDE_FILE("cam/scsi/scsi_message.h" HAVE_CAM_SCSI_SCSI_MESSAGE_H)
	#CHECK_INCLUDE_FILE("cam/scsi/scsi_pass.h" HAVE_CAM_SCSI_SCSI_PASS_H)
	CHECK_LIBRARY_EXISTS(cam "cam_open_pass" "" HAVE_LIBCAM)
	IF(HAVE_LIBCAM)
		SET(SCSI_LIBRARY cam)
		SET(RP_OS_SCSI_SUPPORTED 1)
	ENDIF()
	ENDIF(0) # FIXME
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "NetBSD" OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
	SET(librpfile_SRCS ${librpfile_SRCS} scsi/RpFile_scsi_netbsd.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSE()
	# Dummy implementation for RpFile::rereadDeviceSizeOS().
	SET(librpfile_SRCS ${librpfile_SRCS} scsi/RpFile_scsi_dummy.cpp)
ENDIF()

IF(WIN32)
	SET(librpfile_OS_SRCS
		win32/FileSystem_win32.cpp
		win32/RpFile_win32.cpp
		win32/IStreamWrapper.cpp
		Win32/RpFile_windres.cpp
		)
	SET(librpfile_OS_H
		win32/IStreamWrapper.hpp
		Win32/RpFile_windres.hpp
		)
ELSE(WIN32)
	SET(librpfile_OS_SRCS
		FileSystem_posix.cpp
		RpFile_stdio.cpp
		)
ENDIF(WIN32)

# Write the config.h files.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.librpfile.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.librpfile.h")

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(librpfile_PCH_CXX stdafx.cpp)
	SET(librpfile_PCH_SRC ${librpfile_PCH_CXX})
	SET(librpfile_PCH_H stdafx.h)
ENDIF(ENABLE_PCH)

######################
# Build the library. #
######################

ADD_LIBRARY(rpfile STATIC
	${librpfile_PCH_SRC} ${librpfile_PCH_H}
	${librpfile_SRCS} ${librpfile_H}
	${librpfile_OS_SRCS} ${librpfile_OS_H}
	)
IF(ENABLE_PCH)
	ADD_PRECOMPILED_HEADER(rpfile ${librpfile_PCH_H}
		SOURCE_CXX ${librpfile_PCH_CXX}
		)
ENDIF(ENABLE_PCH)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(rpfile)

# rpthreads is needed for atomic functions.
TARGET_LINK_LIBRARIES(rpfile PRIVATE rpcpu rpthreads)
IF(WIN32)
	# for MiniU82T
	TARGET_LINK_LIBRARIES(rpfile PRIVATE win32common)
ENDIF(WIN32)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rpfile
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpfile
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpfile
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)
IF(NOT ZLIB_LIBRARY)
	MESSAGE(FATAL_ERROR "ZLIB_LIBRARY has not been set by CheckZLIB.cmake.")
ENDIF(NOT ZLIB_LIBRARY)
TARGET_LINK_LIBRARIES(rpfile PRIVATE ${ZLIB_LIBRARY})
IF(MSVC)
	TARGET_LINK_LIBRARIES(rpfile PRIVATE delayimp)
ENDIF(MSVC)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(rpfile PROPERTIES EXCLUDE_FROM_ALL TRUE)
# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(rpfile git_version)
ENDIF(TARGET git_version)

# Other libraries.
TARGET_LINK_LIBRARIES(rpfile PRIVATE rptexture inih rpthreads cachecommon)
IF(ZLIB_FOUND)
	TARGET_LINK_LIBRARIES(rpfile PRIVATE ${ZLIB_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(rpfile PRIVATE ${ZLIB_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(rpfile PRIVATE ${ZLIB_DEFINITIONS})
ENDIF(ZLIB_FOUND)
#IF(WIN32)
#	# libwin32common
#	TARGET_LINK_LIBRARIES(rpfile PRIVATE win32common)
#ELSE(WIN32)
#	# libunixcommon
#	TARGET_LINK_LIBRARIES(rpfile PRIVATE unixcommon)
#ENDIF(WIN32)
IF(SCSI_LIBRARY)
	# An extra library is needed for SCSI support.
	TARGET_LINK_LIBRARIES(rpfile PRIVATE ${SCSI_LIBRARY})
ENDIF(SCSI_LIBRARY)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
