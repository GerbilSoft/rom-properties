PROJECT(rom-properties-win32)

# Sources and headers.
SET(rom-properties-win32_SRCS
	DllMain.cpp
	RegKey.cpp
	RP_ExtractIcon.cpp
	RP_ComBase.cpp
	RP_ExtractImage.cpp
	RpImageWin32.cpp
	RP_ShellPropSheetExt.cpp
	#DialogBuilder.cpp
	RpFile_IStream.cpp
	RP_ThumbnailProvider.cpp
	resource.rc
	)
SET(rom-properties-win32_H
	RegKey.hpp
	RP_ExtractIcon.hpp
	RP_ClassFactory.hpp
	RP_ComBase.hpp
	RP_ExtractImage.hpp
	RpImageWin32.hpp
	RP_ShellPropSheetExt.hpp
	#DialogBuilder.hpp
	RpFile_IStream.hpp
	RP_ThumbnailProvider.hpp
	resource.h
	)

#####################
# Build the plugin. #
#####################

# FIXME: Build both Win32 and Win64 DLLs?
ADD_LIBRARY(rom-properties MODULE
		${rom-properties-win32_SRCS}
		${rom-properties-win32_H}
		)
SET_TARGET_PROPERTIES(rom-properties PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rom-properties)
SET_WINDOWS_SUBSYSTEM(rom-properties WIN32)
SET_WINDOWS_NO_MANIFEST(rom-properties)
TARGET_INCLUDE_DIRECTORIES(rom-properties
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
TARGET_LINK_LIBRARIES(rom-properties romdata16 cachemgr16)
TARGET_LINK_LIBRARIES(rom-properties comctl32)

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
ENDIF(RP_DLL_FLAGS)

UNSET(RP_DLL_FLAGS)
UNSET(RP_LINK_FLAGS)
