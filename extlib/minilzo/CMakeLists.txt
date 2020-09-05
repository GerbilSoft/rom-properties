# MiniLZO
PROJECT(minilzo LANGUAGES C)

IF(MSVC)
	# Append 'd' to the DLL filename for debug builds.
	SET(CMAKE_DEBUG_POSTFIX "d")
ENDIF(MSVC)

IF(WIN32)
	# rom-properties: Add the Win32 resource file.
	SET(LZO_RC_FILE minilzo-dll.rc)
ENDIF(WIN32)

ADD_LIBRARY(minilzo SHARED
	minilzo.c minilzo.h
	lzoconf.h lzodefs.h
	${LZO_RC_FILE}
	)
TARGET_INCLUDE_DIRECTORIES(minilzo
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	)
DO_SPLIT_DEBUG(minilzo)
SET_WINDOWS_SUBSYSTEM(minilzo WIN32)
SET_WINDOWS_SUBSYSTEM(minilzo WIN32)

# Set dllexport/dllimport on Windows to ensure the import library is
# created and to optimize DLL linkage.
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(minilzo
		PRIVATE "-D__LZO_EXPORT1=__declspec(dllexport)"
		INTERFACE "-D__LZO_EXPORT1=__declspec(dllimport)"
		)
ENDIF(WIN32)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
