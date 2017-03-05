PROJECT(rom-properties-win32)

# Sources and headers.
SET(rom-properties-win32_SRCS
	DllMain.cpp
	RegKey.cpp
	RP_ExtractIcon.cpp
	RP_ExtractIcon_Register.cpp
	RP_ExtractIcon_Fallback.cpp
	RP_ComBase.cpp
	RP_ExtractImage.cpp
	RP_ExtractImage_Register.cpp
	RP_ExtractImage_Fallback.cpp
	RpImageWin32.cpp
	RP_ShellPropSheetExt.cpp
	RP_ShellPropSheetExt_Register.cpp
	#DialogBuilder.cpp
	RpFile_IStream.cpp
	RP_ThumbnailProvider.cpp
	RP_ThumbnailProvider_Register.cpp
	RP_ThumbnailProvider_Fallback.cpp
	resource.rc
	)
SET(rom-properties-win32_H
	RegKey.hpp
	RP_ExtractIcon.hpp
	RP_ExtractIcon_p.hpp
	RP_ClassFactory.hpp
	RP_ComBase.hpp
	RP_ExtractImage.hpp
	RP_ExtractImage_p.hpp
	RpImageWin32.hpp
	RP_ShellPropSheetExt.hpp
	#DialogBuilder.hpp
	RpFile_IStream.hpp
	RP_ThumbnailProvider.hpp
	RP_ThumbnailProvider_p.hpp
	resource.h
	)

# Precompiled headers.
# NOTE: Only enabling PCH for MSVC builds.
IF(MSVC)
	INCLUDE(PrecompiledHeader)
	SET(rom-properties-win32_PCH_SRC stdafx.cpp)
	SET(rom-properties-win32_PCH_H stdafx.h)
ENDIF(MSVC)

#####################
# Build the plugin. #
#####################

# FIXME: Build both Win32 and Win64 DLLs?
# NOTE: MODULE results in the .dll being placed in
# the 'lib' directory. Use SHARED instead.
# TODO: Disable building of import libraries.
ADD_LIBRARY(rom-properties SHARED
	${rom-properties-win32_PCH_SRC}
	${rom-properties-win32_PCH_H}
	${rom-properties-win32_SRCS}
	${rom-properties-win32_H}
	)
# NOTE: Only enabling PCH for MSVC.
IF(MSVC)
	ADD_PRECOMPILED_HEADER(rom-properties ${rom-properties-win32_PCH_H}
		SOURCE_CXX ${rom-properties-win32_PCH_SRC}
		)
ENDIF(MSVC)
SET_TARGET_PROPERTIES(rom-properties PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rom-properties)
SET_WINDOWS_SUBSYSTEM(rom-properties WIN32)
SET_WINDOWS_NO_MANIFEST(rom-properties)
# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rom-properties
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
TARGET_LINK_LIBRARIES(rom-properties romdata16 cachemgr16)
TARGET_LINK_LIBRARIES(rom-properties comctl32)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_BINARY_DIR}\"")
ENDIF(MINGW)

# Use the .def file for DLL exports.
IF(MSVC)
	SET(RP_DLL_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.def")
ELSEIF(MINGW)
	# NOTE: Enabling stdcall fixups so we can use
	# the same .def file for Win32 and Win64,
	# since Win64 doesn't use stdcall.
	SET(RP_DLL_FLAGS "-Wl,-enable-stdcall-fixup ${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.def")
ENDIF()

IF(RP_DLL_FLAGS)
	GET_TARGET_PROPERTY(RP_LINK_FLAGS rom-properties LINK_FLAGS)
	IF(RP_LINK_FLAGS)
		SET(RP_LINK_FLAGS "${RP_LINK_FLAGS} ${RP_DLL_FLAGS}")
	ELSE()
		SET(RP_LINK_FLAGS "${RP_DLL_FLAGS}")
	ENDIF()
	SET_TARGET_PROPERTIES(rom-properties PROPERTIES LINK_FLAGS "${RP_LINK_FLAGS}")
	UNSET(RP_LINK_FLAGS)
ENDIF(RP_DLL_FLAGS)

UNSET(RP_DLL_FLAGS)

# Delay-load ZLIB and LIBPNG.
# NOTE: Using CMAKE_SHARED_LINKER_FLAGS because zlib and libpng
# have different names in Debug and Release builds.
# FIXME: The rom-properties DLL directory is *not* in the
# DLL search path, so these delay loads fail.
# TODO: SetDllDirectory() temporarily for XP, and
# AddDllDirectory() on Windows Vista+?
#IF(USE_INTERNAL_ZLIB_DLL)
#	SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG   "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DELAYLOAD:zlib1d.dll")
#	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DELAYLOAD:zlib1.dll")
#ENDIF(USE_INTERNAL_ZLIB_DLL)
#IF(USE_INTERNAL_PNG_DLL)
	#SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG   "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DELAYLOAD:libpng16d.dll")
#	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DELAYLOAD:libpng16.dll")
#ENDIF(USE_INTERNAL_PNG_DLL)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS rom-properties
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "dll"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
