# rom-properties file handling library
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rpfile LANGUAGES CXX)

# Check for C library functions.
IF(NOT WIN32)
	# Check for statx().
	SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=1")
	CHECK_SYMBOL_EXISTS(statx "sys/stat.h" HAVE_STATX)
	UNSET(CMAKE_REQUIRED_DEFINITIONS)

	# Check for an xattr header.
	INCLUDE(CheckIncludeFile)
	CHECK_INCLUDE_FILE("sys/xattr.h" HAVE_SYS_XATTR_H)
	CHECK_INCLUDE_FILE("sys/extattr.h" HAVE_SYS_EXTATTR_H)

	# Check for an xattr function.
	INCLUDE(CheckSymbolExists)
	IF(HAVE_SYS_XATTR_H)
		IF(APPLE)
			CHECK_SYMBOL_EXISTS(fsetxattr "sys/xattr.h" HAVE_FSETXATTR_MAC)
		ELSE(APPLE)
			CHECK_SYMBOL_EXISTS(fsetxattr "sys/xattr.h" HAVE_FSETXATTR_LINUX)
		ENDIF(APPLE)
	ELSEIF(HAVE_SYS_EXTATTR_H)
		CHECK_SYMBOL_EXISTS(extattr_set_fd "sys/extattr.h" HAVE_EXTATTR_SET_FD)
	ENDIF()
ENDIF(NOT WIN32)

# Sources.
SET(${PROJECT_NAME}_SRCS
	IRpFile.cpp
	MemFile.cpp
	VectorFile.cpp
	FileSystem_common.cpp
	RelatedFile.cpp
	DualFile.cpp
	scsi/RpFile_Kreon.cpp
	scsi/RpFile_scsi.cpp
	xattr/XAttrReader.cpp
	xattr/Ext2AttrData.c
	xattr/XfsAttrData.c
	)
# Headers.
SET(${PROJECT_NAME}_H
	DualFile.hpp
	IRpFile.hpp
	FileSystem.hpp
	MemFile.hpp
	RpFile.hpp
	RpFile_p.hpp
	RecursiveScan.hpp
	RelatedFile.hpp
	SubFile.hpp
	VectorFile.hpp
	scsi/ata_protocol.h
	scsi/scsi_protocol.h
	scsi/scsi_ata_cmds.h
	xattr/XAttrReader.hpp
	xattr/dos_attrs.h
	xattr/ext2_flags.h
	xattr/Ext2AttrData.h
	xattr/xfs_flags.h
	xattr/XfsAttrData.h
	)

# OS-specific implementations
IF(WIN32)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} scsi/RpFile_scsi_win32.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} scsi/RpFile_scsi_linux.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "Dragonfly")
	# FIXME: Not working properly. (SCSI ILLEGAL REQUEST errors)
	# NOTE: The file is still needed for rereadDeviceSizeOS().
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} scsi/RpFile_scsi_freebsd.cpp)

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
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} scsi/RpFile_scsi_netbsd.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSE()
	# Dummy implementation
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} scsi/RpFile_scsi_dummy.cpp)
ENDIF()

# XAttr functionality (OS-specific)
IF(WIN32)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} xattr/XAttrReader_win32.cpp)
ELSEIF(HAVE_SYS_XATTR_H OR HAVE_SYS_EXTATTR_H)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} xattr/XAttrReader_posix.cpp)
ELSE()
	# Dummy implementation
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} xattr/XAttrReader_dummy.cpp)
ENDIF()

IF(WIN32)
	SET(${PROJECT_NAME}_OS_SRCS
		win32/FileSystem_win32.cpp
		win32/IStreamWrapper.cpp
		win32/RecursiveScan_win32.cpp
		win32/RpFile_win32.cpp
		)
	SET(${PROJECT_NAME}_OS_H
		win32/IStreamWrapper.hpp
		)
ELSE(WIN32)
	SET(${PROJECT_NAME}_OS_SRCS
		FileSystem_posix.cpp
		RecursiveScan_posix.cpp
		RpFile_stdio.cpp
		)
ENDIF(WIN32)

# Write the config.h files.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

######################
# Build the library. #
######################

# NOTE: ${PROJECT_NAME} is the original target.
# ${PROJECT_NAME}-dll is the target with dllimport/dllexport for libromdata.
IF(RP_LIBROMDATA_IS_DLL AND WIN32)
	# On Win32, the dll target needs dllimport/dllexport.
	SET(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-dll)
ELSE()
	# On other platforms, the dll target will be an alias to the regular target.
	# That will be set up later, since the regular target doesn't exist yet.
	SET(TARGETS ${PROJECT_NAME})
ENDIF()

FOREACH(_target ${TARGETS})
	ADD_LIBRARY(${_target} STATIC
		${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
		${${PROJECT_NAME}_OS_SRCS} ${${PROJECT_NAME}_OS_H}
		)
	IF(ENABLE_PCH)
		TARGET_PRECOMPILE_HEADERS(${_target} PRIVATE
			"${RP_CONFIG_LIBC_H}" "${RP_COMPILER_COMPAT_H}"
			stdafx.h
			)
	ENDIF(ENABLE_PCH)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	# Make sure git_version.h is created before compiling this target.
	IF(TARGET git_version)
		ADD_DEPENDENCIES(${_target} git_version)
	ENDIF(TARGET git_version)

	# DLL targets need the appropriate macros.
	IF(${_target} MATCHES "-dll$")
		SET(_target_suffix "-dll")
		TARGET_COMPILE_DEFINITIONS(${_target}
			PUBLIC		RP_BUILDING_FOR_DLL=1
			PRIVATE		RP_BUILDING_LIBROMDATA=1
			)
	ELSE()
		UNSET(_target_suffix)
	ENDIF()

	# rpthreads is needed for atomic functions.
	TARGET_LINK_LIBRARIES(${_target} PRIVATE rpcpu${_target_suffix} rpthreads${_target_suffix})
	TARGET_LINK_LIBRARIES(${_target} PRIVATE cachecommon${_target_suffix})
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE i18n${_target_suffix})
	ENDIF(ENABLE_NLS)
	IF(WIN32)
		# xattr/XAttrReader_win32.cpp uses wchar.hpp
		TARGET_LINK_LIBRARIES(${_target} PRIVATE rptext${_target_suffix})
	ENDIF(WIN32)

	# Include paths:
	# - Public: Current source and binary directories.
	# - Private: Parent source and binary directories,
	#            and top-level binary directory for git_version.h.
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpfile
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpfile
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
		)

	IF(MSVC)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE delayimp)
	ENDIF(MSVC)

	# Other libraries.
	IF(ZLIB_FOUND)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${ZLIB_LIBRARIES})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${ZLIB_INCLUDE_DIRS})
		TARGET_COMPILE_DEFINITIONS(${_target} PRIVATE ${ZLIB_DEFINITIONS})
	ELSE(ZLIB_FOUND)
		MESSAGE(FATAL_ERROR "ZLIB_LIBRARIES has not been set by CheckZLIB.cmake.")
	ENDIF(ZLIB_FOUND)
	IF(SCSI_LIBRARY)
		# An extra library is needed for SCSI support.
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${SCSI_LIBRARY})
	ENDIF(SCSI_LIBRARY)
ENDFOREACH(_target)
UNSET(_target_suffix)

IF(NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
