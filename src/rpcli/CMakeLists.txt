PROJECT(rom-properties-rpcli)

# Check for reentrant functions.
# NOTE: May be _gmtime32_s() or _gmtime64_s() on MSVC 2005+.
# The "inline" part will detect that.
INCLUDE(CheckSymbolExistsOrInline)
CHECK_SYMBOL_EXISTS_OR_INLINE(gmtime_r "time.h" "time_t tm; gmtime_r(&tm, NULL);" HAVE_GMTIME_R)
IF(NOT HAVE_GMTIME_R)
	CHECK_SYMBOL_EXISTS_OR_INLINE(gmtime_s "time.h" "time_t tm; gmtime_s(NULL, &tm);" HAVE_GMTIME_S)
ENDIF(NOT HAVE_GMTIME_R)
CHECK_SYMBOL_EXISTS_OR_INLINE(localtime_r "time.h" "time_t tm; localtime_r(&tm, NULL);" HAVE_LOCALTIME_R)
IF(NOT HAVE_LOCALTIME_R)
	CHECK_SYMBOL_EXISTS_OR_INLINE(localtime_s "time.h" "time_t tm; localtime_s(NULL, &tm);" HAVE_LOCALTIME_S)
ENDIF(NOT HAVE_LOCALTIME_R)

# Create the manifest file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/rpcli.exe.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/rpcli.exe.manifest" @ONLY)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.rpcli.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.rpcli.h")

# Sources and headers.
SET(rom-properties-rpcli_SRCS
	rpcli.cpp
	properties.cpp
	bmp.cpp
	)
SET(rom-properties-rpcli_H
	properties.hpp
	bmp.hpp
	)

IF(WIN32)
	SET(rom-properties-rpcli_RC resource.rc)
ENDIF(WIN32)

IF(ENABLE_DECRYPTION)
	SET(rom-properties-rpcli_CRYPTO_SRCS verifykeys.cpp)
	SET(rom-properties-rpcli_CRYPTO_H verifykeys.hpp)
ENDIF(ENABLE_DECRYPTION)

IF(MSVC)
	# Precompiled headers.
	# NOTE: Only enabling PCH for MSVC builds.
	INCLUDE(PrecompiledHeader)
	SET(rom-properties-rpcli_PCH_SRC stdafx.cpp)
	SET(rom-properties-rpcli_PCH_H stdafx.h)

	# Delay-load sources.
	SET(rom-properties-rpcli-DELAY_LOAD_SRC ../libwin32common/DelayLoadHelper.cpp)
	SET(rom-properties-rpcli-DELAY_LOAD_H ../libwin32common/DelayLoadHelper.hpp)
ENDIF(MSVC)

#########################
# Build the executable. #
#########################

IF(MSVC)
	# Delay-load extlibs.
	INCLUDE(../libwin32common/DelayLoadHelper.cmake)
	SET_DELAY_LOAD_FLAGS()
ENDIF(MSVC)

ADD_EXECUTABLE(rpcli
	${rom-properties-rpcli_PCH_SRC}
	${rom-properties-rpcli_PCH_H}
	${rom-properties-rpcli-DELAY_LOAD_SRC}
	${rom-properties-rpcli-DELAY_LOAD_H}
	${rom-properties-rpcli_SRCS}
	${rom-properties-rpcli_H}
	${rom-properties-rpcli_RC}
	${rom-properties-rpcli_CRYPTO_SRCS}
	${rom-properties-rpcli_CRYPTO_H}
	)
# NOTE: Only enabling PCH for MSVC.
IF(MSVC)
	ADD_PRECOMPILED_HEADER(rpcli ${rom-properties-rpcli_PCH_H}
		SOURCE_CXX ${rom-properties-rpcli_PCH_SRC}
		)
ENDIF(MSVC)
SET_TARGET_PROPERTIES(rpcli PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rpcli)
SET_WINDOWS_SUBSYSTEM(rpcli CONSOLE)
SET_WINDOWS_NO_MANIFEST(rpcli)
# FIXME: setargv isn't necessarily sorted...
# TODO: Implement our own setargv function?
SET_WINDOWS_ENTRYPOINT(rpcli wmain ON)
# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rpcli
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
TARGET_LINK_LIBRARIES(rpcli romdata rpbase)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rpcli i18n)
ENDIF(ENABLE_NLS)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_BINARY_DIR}\"")
ENDIF(MINGW)

# Delay-load some Windows system libraries.
# TODO: MinGW delay-load?
IF(MSVC)
	SET(RP_SYS_DELAY_LOAD_FLAGS "/DELAYLOAD:urlmon.dll /DELAYLOAD:wininet.dll")
	TARGET_LINK_LIBRARIES(rpcli delayimp)
ENDIF(MSVC)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS rpcli
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rpcli PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
