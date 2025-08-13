# rom-properties base library
PROJECT(rpbase LANGUAGES C CXX)

IF(NOT WIN32)
	# Non-Windows library checks.
	IF(ENABLE_DECRYPTION)
		INCLUDE(CheckNettle2or3)
		CHECK_NETTLE_2_OR_3()
	ENDIF(ENABLE_DECRYPTION)
ENDIF(NOT WIN32)

# ZLIB and libpng are checked in the top-level CMakeLists.txt.

# Check for unordered_map::reserve and unordered_set::reserve.
UNSET(CMAKE_REQUIRED_INCLUDES)
CHECK_CXX_SOURCE_COMPILES("#include <unordered_map>
int main(void) {
	std::unordered_map<int, int> testMap;
	testMap.reserve(4);
	return 0;
}" HAVE_UNORDERED_MAP_RESERVE)
CHECK_CXX_SOURCE_COMPILES("#include <unordered_set>
int main(void) {
	std::unordered_set<int> testSet;
	testSet.reserve(4);
	return 0;
}" HAVE_UNORDERED_SET_RESERVE)

# Sources
SET(${PROJECT_NAME}_SRCS
	RomData.cpp
	RomFields.cpp
	RomMetaData.cpp
	RomDataTestObject.cpp
	SystemRegion.cpp
	TextOut_common.cpp
	TextOut_text.cpp
	TextOut_json.cpp
	Achievements.cpp
	img/RpImageLoader.cpp
	img/RpPng.cpp
	img/RpPngWriter.cpp
	img/APNG_dlopen.cpp
	img/IconAnimHelper.cpp
	disc/IDiscReader.cpp
	disc/DiscReader.cpp
	disc/IPartition.cpp
	disc/PartitionFile.cpp
	disc/SparseDiscReader.cpp
	disc/CBCReader.cpp
	disc/IResourceReader.cpp
	crypto/KeyManager.cpp
	config/ConfReader.cpp
	config/Config.cpp
	config/AboutTabText.cpp
	)
# Headers
SET(${PROJECT_NAME}_H
	RomData.hpp
	RomData_decl.hpp
	RomData_p.hpp
	RomFields.hpp
	RomMetaData.hpp
	RomDataTestObject.hpp
	SystemRegion.hpp
	TextOut.hpp
	Achievements.hpp
	timeconv.h
	img/RpPng.hpp
	img/RpPngWriter.hpp
	img/APNG_dlopen.hpp
	disc/IDiscReader.hpp
	disc/DiscReader.hpp
	disc/IPartition.hpp
	disc/IFst.hpp
	disc/PartitionFile.hpp
	disc/SparseDiscReader.hpp
	disc/SparseDiscReader_p.hpp
	disc/CBCReader.hpp
	disc/IResourceReader.hpp
	disc/exe_res_structs.h
	crypto/KeyManager.hpp
	config/ConfReader.hpp
	config/Config.hpp
	config/AboutTabText.hpp

	cdrom_structs.h
	)

IF(NOT USE_INTERNAL_PNG OR (USE_INTERNAL_PNG AND USE_INTERNAL_PNG_DLL))
	SET(${PROJECT_NAME}_NEEDS_DL 1)
ENDIF(NOT USE_INTERNAL_PNG OR (USE_INTERNAL_PNG AND USE_INTERNAL_PNG_DLL))

IF(WIN32)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} img/RpJpeg_gdiplus.cpp)
	SET(${PROJECT_NAME}_H ${${PROJECT_NAME}_H} img/RpJpeg.hpp)
ELSEIF(JPEG_FOUND)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} img/RpJpeg.cpp)
	SET(${PROJECT_NAME}_H ${${PROJECT_NAME}_H} img/RpJpeg.hpp)
ENDIF()

# NOTE: Hash contains CRC32, which isn't cryptographic, but we're keeping
# Hash in ${PROJECT_NAME}_CRYPTO_SRCS for consistency.
SET(${PROJECT_NAME}_CRYPTO_H crypto/Hash.hpp)
IF(WIN32)
	SET(${PROJECT_NAME}_CRYPTO_SRCS crypto/HashCAPI.cpp)
ELSE(WIN32)
	SET(${PROJECT_NAME}_CRYPTO_SRCS crypto/HashNettle.cpp)
ENDIF(WIN32)
IF(ENABLE_DECRYPTION)
	SET(${PROJECT_NAME}_CRYPTO_SRCS ${${PROJECT_NAME}_CRYPTO_SRCS} crypto/AesCipherFactory.cpp)
	SET(${PROJECT_NAME}_CRYPTO_H    ${${PROJECT_NAME}_CRYPTO_H} crypto/IAesCipher.hpp)
	IF(WIN32)
		SET(${PROJECT_NAME}_CRYPTO_SRCS ${${PROJECT_NAME}_CRYPTO_SRCS}
			crypto/AesCAPI.cpp
			crypto/AesCAPI_NG.cpp
			)
		SET(${PROJECT_NAME}_CRYPTO_H ${${PROJECT_NAME}_CRYPTO_H}
			crypto/AesCAPI.hpp
			crypto/AesCAPI_NG.hpp
			)
	ELSE(WIN32)
		SET(${PROJECT_NAME}_CRYPTO_SRCS ${${PROJECT_NAME}_CRYPTO_SRCS}
			crypto/AesNettle.cpp
			)
		SET(${PROJECT_NAME}_CRYPTO_H ${${PROJECT_NAME}_CRYPTO_H}
			crypto/AesNettle.hpp
			)
	ENDIF(WIN32)
ENDIF(ENABLE_DECRYPTION)

# CPU-specific and optimized sources.
INCLUDE(CPUInstructionSetFlags)
IF(CPU_i386 OR CPU_amd64)
	IF(JPEG_FOUND AND NOT WIN32)
		SET(${PROJECT_NAME}_SSSE3_SRCS
			${${PROJECT_NAME}_SSSE3_SRCS}
			img/RpJpeg_ssse3.cpp
			)
		SET(${PROJECT_NAME}_SSSE3_H
			${${PROJECT_NAME}_SSSE3_H}
			img/RpJpeg_ssse3.hpp
			)
	ENDIF(JPEG_FOUND AND NOT WIN32)

	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Write the config.h file.
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
		${${PROJECT_NAME}_CRYPTO_SRCS} ${${PROJECT_NAME}_CRYPTO_H}
		${${PROJECT_NAME}_SSSE3_SRCS} ${${PROJECT_NAME}_SSSE3_H}
		)
	IF(ENABLE_PCH)
		TARGET_PRECOMPILE_HEADERS(${_target} PRIVATE
			"${RP_CONFIG_LIBC_H}" "${RP_COMPILER_COMPAT_H}"
			stdafx.h
			)
	ENDIF(ENABLE_PCH)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds; enable PIC.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES
		EXCLUDE_FROM_ALL TRUE
		POSITION_INDEPENDENT_CODE TRUE
		)
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

	# Include paths:
	# - Public: Current source and binary directories.
	# - Private: Parent source and binary directories,
	#            and top-level binary directory for git_version.h.
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpbase
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpbase
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
			${RAPIDJSON_INCLUDE_DIRS}				# rapidjson
		)
	# rapidjson: Enable std::string support.
	ADD_DEFINITIONS(-DRAPIDJSON_HAS_STDSTRING)
	IF(CPU_amd64)
		# rapidjson: Enable SSE2 support in amd64 builds.
		# NOTE: rapidjson does not have dynamic dispatch,
		# so we can't enable SSE2 in i386 or SSE 4.2 at all
		# because it'll crash if the machine doesn't support it.
		ADD_DEFINITIONS(-DRAPIDJSON_SSE2)
	ENDIF(CPU_amd64)

	IF(MSVC)
		# Delay-load implementation.
		TARGET_LINK_LIBRARIES(${_target} PRIVATE delayimp)
	ENDIF(MSVC)

	# Link in libdl if it's required for dlopen()
	# and we have a component that uses it.
	IF(${PROJECT_NAME}_NEEDS_DL AND CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(${PROJECT_NAME}_NEEDS_DL AND CMAKE_DL_LIBS)

	# Other libraries
	TARGET_LINK_LIBRARIES(${_target} PRIVATE
		rpbyteswap${_target_suffix} rpfile${_target_suffix} rptext${_target_suffix}
		rptexture${_target_suffix} cachecommon${_target_suffix})
	TARGET_LINK_LIBRARIES(${_target} PRIVATE rpcpuid)	# for CPU dispatch
	TARGET_LINK_LIBRARIES(${_target} PRIVATE inih)

	IF(ZLIB_FOUND)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${ZLIB_LIBRARIES})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${ZLIB_INCLUDE_DIRS})
		TARGET_COMPILE_DEFINITIONS(${_target} PRIVATE ${ZLIB_DEFINITIONS})
	ELSE(ZLIB_FOUND)
		MESSAGE(FATAL_ERROR "ZLIB_FOUND has not been set by CheckZLIB.cmake.")
	ENDIF(ZLIB_FOUND)
	IF(PNG_FOUND)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${PNG_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${PNG_INCLUDE_DIRS})
		TARGET_COMPILE_DEFINITIONS(${_target} PRIVATE ${PNG_DEFINITIONS})
	ENDIF(PNG_FOUND)
	IF(JPEG_FOUND)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${JPEG_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${JPEG_INCLUDE_DIRS})
	ENDIF(JPEG_FOUND)
	IF(NETTLE_FOUND)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${NETTLE_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${NETTLE_INCLUDE_DIRS})
	ENDIF(NETTLE_FOUND)
	IF(Fmt_FOUND)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${Fmt_LIBRARY})
	ENDIF(Fmt_FOUND)

	IF(WIN32)
		# libwin32common
		TARGET_LINK_LIBRARIES(${_target} PRIVATE win32common${_target_suffix})
		# Windows system libraries
		TARGET_LINK_LIBRARIES(${_target} PRIVATE gdiplus)
		IF(ENABLE_DECRYPTION)
			TARGET_LINK_LIBRARIES(${_target} PRIVATE advapi32)
		ENDIF(ENABLE_DECRYPTION)
	ELSE(WIN32)
		# libunixcommon
		TARGET_LINK_LIBRARIES(${_target} PRIVATE unixcommon${_target_suffix})
	ENDIF(WIN32)

	IF(SCSI_LIBRARY)
		# An extra library is needed for SCSI support.
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${SCSI_LIBRARY})
	ENDIF(SCSI_LIBRARY)

	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE i18n${_target_suffix})
	ENDIF(ENABLE_NLS)
ENDFOREACH(_target)
UNSET(_target_suffix)

IF(NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
