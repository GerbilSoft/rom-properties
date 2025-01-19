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

IF(BUILD_SHARED_LIBS)
	SET(minilzo_LIB_TYPE SHARED)
ELSE(BUILD_SHARED_LIBS)
	SET(minilzo_LIB_TYPE STATIC)

	# rom-properties: Hide all symbols so they're not exported by libromdata.
	CMAKE_POLICY(SET CMP0063 NEW)
	SET(CMAKE_C_VISIBILITY_PRESET "hidden")
	SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
ENDIF(BUILD_SHARED_LIBS)

ADD_LIBRARY(minilzo ${minilzo_LIB_TYPE}
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

IF(NOT SKIP_INSTALL_LIBRARIES)
	INSTALL(TARGETS minilzo
		#EXPORT minilzo
		RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
		#ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
		FRAMEWORK DESTINATION "${INSTALL_LIB_DIR}")

	IF(INSTALL_DEBUG AND BUILD_SHARED_LIBS)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		# TODO: Test on MinGW.
		# FIXME: Generator PDB expression doesn't have the debug postfix for some reason,
		# so cpack fails in debug builds if we get the PDB property.
		INCLUDE(DirInstallPaths)
		SET(PDB_FILENAME_D "$<TARGET_FILE_DIR:minilzo>/minilzo${CMAKE_DEBUG_POSTFIX}.pdb")
		SET(PDB_FILENAME_R "$<TARGET_FILE_DIR:minilzo>/minilzo${CMAKE_RELEASE_POSTFIX}.pdb")
		INSTALL(FILES "${PDB_FILENAME_D}" "${PDB_FILENAME_R}"
			DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
			COMPONENT "debug"
			OPTIONAL
			)
		UNSET(PDB_FILENAME_D)
		UNSET(PDB_FILENAME_R)
	ENDIF(INSTALL_DEBUG AND BUILD_SHARED_LIBS)
ENDIF(NOT SKIP_INSTALL_LIBRARIES)
