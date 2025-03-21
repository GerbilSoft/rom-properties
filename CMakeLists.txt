# ROM Properties Page Shell Extension
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

# CMP0048: Set VERSION variables based on the project version specified in PROJECT().
# Introduced in CMake 3.0.
IF(POLICY CMP0048)
	CMAKE_POLICY(SET CMP0048 NEW)
ENDIF(POLICY CMP0048)

# CMP0063: Honor visibility properties for all target types,
# including static libraries and executables.
# Introduced in CMake 3.3.
IF(POLICY CMP0063)
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

# CMP0151: Use -isystem for AUTOMOC include directories.
# clang complains about `using namespace` statements in the
# generated moc_*.cpp files:
# In file included from src/kde/kf5/kfilemetadata_rom-properties-kf5_autogen/mocs_compilation.cpp:2:
# src/kde/kf5/kfilemetadata_rom-properties-kf5_autogen/BKZOEHIFDQ/moc_ExtractorPluginKF5.cpp:1082:17:
#     warning: using namespace directive in global context in header [-Wheader-hygiene]
#  1082 | using namespace RomPropertiesKF5;
#       |                 ^
# Introduced in CMake 3.27.
IF(POLICY CMP0151)
	CMAKE_POLICY(SET CMP0151 NEW)
ENDIF(POLICY CMP0151)

# Read the project version.
CONFIGURE_FILE(version.txt version.txt.tmp)
FILE(STRINGS version.txt RP_VERSION_FILE)
IF(NOT RP_VERSION_FILE)
	MESSAGE(FATAL_ERROR "version.txt not found; cannot determine project version.")
ENDIF(NOT RP_VERSION_FILE)
LIST(GET RP_VERSION_FILE 0 RP_VERSION)
UNSET(RP_VERSION_FILE)
PROJECT(rom-properties VERSION ${RP_VERSION})
IF(PROJECT_VERSION_TWEAK)
	SET(PROJECT_VERSION_DEVEL_PLUS "+")
ENDIF(PROJECT_VERSION_TWEAK)
SET(RP_DISPLAY_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}${PROJECT_VERSION_DEVEL_PLUS}")

# CMAKE_PROJECT_VERSION was introduced in 3.12.
IF(NOT CMAKE_PROJECT_VERSION OR NOT CMAKE_PROJECT_VERSION_MAJOR)
	SET(CMAKE_PROJECT_VERSION ${PROJECT_VERSION})
	SET(CMAKE_PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
	SET(CMAKE_PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
	SET(CMAKE_PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH})
	SET(CMAKE_PROJECT_VERSION_TWEAK ${PROJECT_VERSION_TWEAK})
ENDIF(NOT CMAKE_PROJECT_VERSION OR NOT CMAKE_PROJECT_VERSION_MAJOR)

LIST(APPEND CMAKE_MODULE_PATH
	"${CMAKE_SOURCE_DIR}/cmake/macros"
	"${CMAKE_SOURCE_DIR}/cmake/libs"
	)
IF(UNIX AND NOT APPLE)
	LIST(APPEND CMAKE_MODULE_PATH
		"${CMAKE_SOURCE_DIR}/cmake/macros/GCR_CMake"
		)
ENDIF(UNIX AND NOT APPLE)

# Put all the binaries and libraries into a single directory.
# NOTE: CACHE INTERNAL is required in order to get this to work
# for KF5 for some reason. (and maybe that's why KDE4 did this
# layout by default?)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "Put all binaries in a single directory.")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE INTERNAL "Put all libraries in a single directory.")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE INTERNAL "Put all archives in a single directory.")

# Don't re-link executables if a shared library's implementation has
# changed but its headers haven't changed.
SET(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# Enable testing.
# TODO: INCLUDE(CTest) for more advanced testing after
# enough tests are added.
ENABLE_TESTING()

# Set the standards versions. (Minimum C11, C++14)
# C17 was added in CMake 3.21; for older versions, use C11.
# C++17 was added in CMake 3.8; for older versions, use C++14.
# C++20 was added in CMake 3.12; for older versions, use C++17.
# - C++20 is needed for std::span<>.
# NOTE: These aren't set as hard requirements, though if the compiler
# doesn't support them, code will either be less optimal or will fail
# to compile.
SET(CMAKE_C_STANDARD_REQUIRED OFF)
SET(CMAKE_C_EXTENSIONS ON)
SET(CMAKE_CXX_STANDARD_REQUIRED OFF)
SET(CMAKE_CXX_EXTENSIONS ON)
IF(CMAKE_VERSION VERSION_GREATER 3.20)		# >= 3.21
	SET(CMAKE_C_STANDARD 17)
	SET(CMAKE_CXX_STANDARD 20)
ELSEIF(CMAKE_VERSION VERSION_GREATER 3.11)	# >= 3.12
	SET(CMAKE_C_STANDARD 11)
	SET(CMAKE_CXX_STANDARD 20)
ELSEIF(CMAKE_VERSION VERSION_GREATER 3.7)	# >= 3.8
	SET(CMAKE_C_STANDARD 11)
	SET(CMAKE_CXX_STANDARD 17)
ELSE()
	SET(CMAKE_C_STANDARD 11)
	SET(CMAKE_CXX_STANDARD 14)
ENDIF()
IF(MSVC AND CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS 10.0.18362.0)
	# Windows SDK prior to 10.0.18362.0 has issues when compiling as either C11 or C17.
	# C:\Program Files (x86)\Windows Kits\8.1\Include\um\winbase.h(8816,5): warning C5105: macro expansion producing 'defined' has undefined behavior
	# C:\Program Files (x86)\Windows Kits\8.1\Include\um\oaidl.h(473,17): warning C5103: pasting '/' and '/' does not result in a valid preprocessing token
	# C:\Program Files (x86)\Windows Kits\8.1\Include\shared\wtypes.h(742,1): message : in expansion of macro '_VARIANT_BOOL'
	# C:\Program Files (x86)\Windows Kits\8.1\Include\um\oaidl.h(473,17): error C2059: syntax error: '/'
	# Reference: https://github.com/microsoft/vcpkg/issues/15035
	SET(CMAKE_C_STANDARD 99)
ENDIF(MSVC AND CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS 10.0.18362.0)

# Set default build options.
INCLUDE(cmake/options.cmake)
# Check for platform-specific functionality.
INCLUDE(cmake/platform.cmake NO_POLICY_SCOPE)

# Program information.
SET(DESCRIPTION "ROM Properties Page shell extension")
SET(PACKAGE_NAME "rom-properties")
SET(AUTHOR "David Korth")
IF(CMAKE_PROJECT_VERSION_PATCH)
	SET(VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}")
ELSE(CMAKE_PROJECT_VERSION_PATCH)
	SET(VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}")
ENDIF(CMAKE_PROJECT_VERSION_PATCH)
IF(CMAKE_PROJECT_VERSION_TWEAK)
	SET(VERSION_STRING "${VERSION_STRING}+")
ENDIF(CMAKE_PROJECT_VERSION_TWEAK)
SET(VERSION_STRING_WIN32 "${CMAKE_PROJECT_VERSION_MAJOR},${CMAKE_PROJECT_VERSION_MINOR},${CMAKE_PROJECT_VERSION_PATCH},${CMAKE_PROJECT_VERSION_TWEAK}")

# Split Debug function.
# Also sets the image version for Windows builds.
# TODO: Move to SplitDebugInformation.cmake?
FUNCTION(DO_SPLIT_DEBUG _target)
	GET_TARGET_PROPERTY(_target_type ${_target} TYPE)
	IF(TARGET ${_target} AND NOT _target_type STREQUAL "STATIC_LIBRARY")
		# Split debug information.
		INCLUDE(SetMSVCDebugPath)
		SET_MSVC_DEBUG_PATH(${_target})
		IF(SPLIT_DEBUG)
			INCLUDE(SplitDebugInformation)
			SPLIT_DEBUG_INFORMATION(${_target})
		ENDIF(SPLIT_DEBUG)
		IF(WIN32)
			# Set image version.
			# Subprojects can override ${PROJECT_VERSION_MAJOR} and ${PROJECT_VERSION_MINOR}.
			# FIXME: If minor version is e.g. "3", Windows interprets it as "03",
			# so "1.3" will actually be "1.03".
			INCLUDE(Win32ImageVersionLinkerFlags)
			IF(PROJECT_VERSION_MAJOR AND PROJECT_VERSION_MINOR)
				WIN32_IMAGE_VERSION_LINKER_FLAGS("${PROJECT_VERSION_MAJOR}" "${PROJECT_VERSION_MINOR}")
			ELSE()
				WIN32_IMAGE_VERSION_LINKER_FLAGS("${CMAKE_PROJECT_VERSION_MAJOR}" "${CMAKE_PROJECT_VERSION_MINOR}")
			ENDIF()
		ENDIF(WIN32)
	ENDIF(TARGET ${_target} AND NOT _target_type STREQUAL "STATIC_LIBRARY")
ENDFUNCTION(DO_SPLIT_DEBUG)

# Git version information.
FIND_PROGRAM(POSIX_SH sh)
IF(POSIX_SH)
	# sh is available.
	# Run the git version script.
	IF(WIN32)
		SET(ENV{SHELLOPTS} igncr)
	ENDIF(WIN32)
	ADD_CUSTOM_TARGET(git_version ALL
		${POSIX_SH} "${CMAKE_SOURCE_DIR}/git_version.sh"
		-s "${CMAKE_SOURCE_DIR}"
		-o "${CMAKE_BINARY_DIR}/git_version.h"
		VERBATIM
		)
ELSE(POSIX_SH)
	# sh isn't available.
	# Create a blank git_version.h.
	FILE(WRITE "${CMAKE_BINARY_DIR}/git_version.h"
		"/* dummy file; POSIX sh is not available */\n")
	# Dummy target for dependencies.
	ADD_CUSTOM_TARGET(git_version)
ENDIF(POSIX_SH)

# Make sure the file is deleted on `make clean`.
SET_PROPERTY(DIRECTORY APPEND
	PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/git_version.h")

# Check for required libraries.
INCLUDE(CheckZLIB)
INCLUDE(CheckPNG)
INCLUDE(CheckJPEG)
INCLUDE(CheckTinyXML2)
INCLUDE(CheckZSTD)
INCLUDE(CheckLZ4)
INCLUDE(CheckLZO)
INCLUDE(CheckLibfmt)

# Reference: https://cmake.org/Wiki/RecipeAddUninstallTarget
########### Add uninstall target ###############
CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

### Subdirectories. ###

IF(ENABLE_NLS)
	ADD_SUBDIRECTORY(locale)
ENDIF(ENABLE_NLS)

# Project subdirectories
ADD_SUBDIRECTORY(extlib)
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(xdg)
IF(INSTALL_DOC)
	ADD_SUBDIRECTORY(doc)
ENDIF(INSTALL_DOC)

### Build summary ###

UNSET(UI_FRONTENDS)
IF(BUILD_KDE4)
	SET(UI_FRONTENDS "${UI_FRONTENDS} KDE4")
ENDIF(BUILD_KDE4)
IF(BUILD_KF5)
	SET(UI_FRONTENDS "${UI_FRONTENDS} KF5")
ENDIF(BUILD_KF5)
IF(BUILD_KF6)
	SET(UI_FRONTENDS "${UI_FRONTENDS} KF6")
ENDIF(BUILD_KF6)
IF(BUILD_XFCE)
	# NOTE: The GTK+ 2.x plugin is XFCE only right now.
	SET(UI_FRONTENDS "${UI_FRONTENDS} XFCE")
ENDIF(BUILD_XFCE)
IF(BUILD_GTK3)
	SET(UI_FRONTENDS "${UI_FRONTENDS} GTK3")
ENDIF(BUILD_GTK3)
IF(BUILD_GTK4)
	SET(UI_FRONTENDS "${UI_FRONTENDS} GTK4")
ENDIF(BUILD_GTK4)
IF(BUILD_WIN32)
	SET(UI_FRONTENDS "${UI_FRONTENDS} Win32")
ENDIF(BUILD_WIN32)
STRING(STRIP "${UI_FRONTENDS}" UI_FRONTENDS)

IF(BUILD_CLI)
	SET(CLI_BUILD_MSG "Yes")
ELSE(BUILD_CLI)
	SET(CLI_BUILD_MSG "No")
ENDIF(BUILD_CLI)

IF(RP_LIBROMDATA_IS_DLL)
	IF(WIN32)
		SET(LIBROMDATA_LIB_TYPE "shared library (.dll)")
	ELSE(WIN32)
		SET(LIBROMDATA_LIB_TYPE "shared library (.so)")
	ENDIF(WIN32)
ELSE(RP_LIBROMDATA_IS_DLL)
	SET(LIBROMDATA_LIB_TYPE "static library")
ENDIF(RP_LIBROMDATA_IS_DLL)

IF(NOT SECURITY_MECHANISM)
	SET(SECURITY_MECHANISM "None")
ENDIF(NOT SECURITY_MECHANISM)

IF(ENABLE_DECRYPTION)
	SET(ENABLE_DECRYPTION_MSG "Enabled")
ELSE(ENABLE_DECRYPTION)
	SET(ENABLE_DECRYPTION_MSG "Disabled")
ENDIF(ENABLE_DECRYPTION)

IF(HAVE_PNG)
	IF(USE_INTERNAL_PNG)
		SET(HAVE_PNG_APNG_MSG "Yes (bundled)")
	ELSE(USE_INTERNAL_PNG)
		IF(HAVE_PNG_APNG)
			SET(HAVE_PNG_APNG_MSG "Yes (system)")
		ELSE(HAVE_PNG_APNG)
			SET(HAVE_PNG_APNG_MSG "No (system)")
		ENDIF(HAVE_PNG_APNG)
	ENDIF(USE_INTERNAL_PNG)
ELSE(HAVE_PNG)
	SET(HAVE_PNG_APNG_MSG "No (library not found?)")
ENDIF(HAVE_PNG)

IF(ENABLE_XML)
	IF(USE_INTERNAL_XML)
		SET(ENABLE_XML_MSG "Enabled (bundled)")
	ELSE(USE_INTERNAL_XML)
		SET(ENABLE_XML_MSG "Enabled (system)")
	ENDIF(USE_INTERNAL_XML)
ELSE(ENABLE_XML)
	SET(ENABLE_XML_MSG "Disabled")
ENDIF(ENABLE_XML)

IF(ENABLE_PVRTC)
	SET(ENABLE_PVRTC_MSG "Enabled")
ELSE(ENABLE_PVRTC)
	SET(ENABLE_PVRTC_MSG "Disabled")
ENDIF(ENABLE_PVRTC)

IF(ENABLE_ZSTD)
	IF(USE_INTERNAL_ZSTD)
		SET(ENABLE_ZSTD_MSG "Enabled (bundled)")
	ELSE(USE_INTERNAL_ZSTD)
		SET(ENABLE_ZSTD_MSG "Enabled (system)")
	ENDIF(USE_INTERNAL_ZSTD)
ELSE(ENABLE_ZSTD)
	SET(ENABLE_ZSTD_MSG "Disabled")
ENDIF(ENABLE_ZSTD)

IF(ENABLE_LZ4)
	IF(USE_INTERNAL_LZ4)
		SET(ENABLE_LZ4_MSG "Enabled (bundled)")
	ELSE(USE_INTERNAL_LZ4)
		SET(ENABLE_LZ4_MSG "Enabled (system)")
	ENDIF(USE_INTERNAL_LZ4)
ELSE(ENABLE_LZ4)
	SET(ENABLE_LZ4_MSG "Disabled")
ENDIF(ENABLE_LZ4)

IF(ENABLE_LZO)
	IF(USE_INTERNAL_LZO)
		SET(ENABLE_LZO_MSG "Enabled (bundled MiniLZO)")
	ELSE(USE_INTERNAL_LZO)
		SET(ENABLE_LZO_MSG "Enabled (system)")
	ENDIF(USE_INTERNAL_LZO)
ELSE(ENABLE_LZO)
	SET(ENABLE_LZO_MSG "Disabled")
ENDIF(ENABLE_LZO)


UNSET(EXTLIB_BUILD)
IF(USE_INTERNAL_ZLIB)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- zlib-ng\n")
ENDIF(USE_INTERNAL_ZLIB)
IF(USE_INTERNAL_PNG)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- libpng\n")
ENDIF(USE_INTERNAL_PNG)
IF(USE_INTERNAL_JPEG)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- libjpeg-turbo\n")
ENDIF(USE_INTERNAL_JPEG)
IF(USE_INTERNAL_ZSTD)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- zstd\n")
ENDIF(USE_INTERNAL_ZSTD)
IF(USE_INTERNAL_LZ4)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- lz4\n")
ENDIF(USE_INTERNAL_LZ4)
IF(USE_INTERNAL_LZO)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- MiniLZO\n")
ENDIF(USE_INTERNAL_LZO)
SET(EXTLIB_BUILD "${EXTLIB_BUILD}- minizip-ng\n")
SET(EXTLIB_BUILD "${EXTLIB_BUILD}- inih\n")
IF(USE_INTERNAL_XML)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- TinyXML2\n")
ENDIF(USE_INTERNAL_XML)
IF(BUILD_TESTING)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- Google Test\n")
ENDIF(BUILD_TESTING)
IF(USE_INTERNAL_FMT)
	SET(EXTLIB_BUILD "${EXTLIB_BUILD}- fmt\n")
ENDIF(USE_INTERNAL_FMT)

IF(BUILD_TRACKER_EXTRACTOR)
	SET(TRACKER_API_DISPLAY "${TRACKER_INSTALL_API_VERSION}")
ELSE(BUILD_TRACKER_EXTRACTOR)
	SET(TRACKER_API_DISPLAY "(none)")
ENDIF(BUILD_TRACKER_EXTRACTOR)

INCLUDE(DirInstallPaths)
MESSAGE(STATUS "

*** ROM Properties Page Shell Extension v${RP_DISPLAY_VERSION} ***

Build Summary:
- Target CPU architecture: ${TARGET_CPU_ARCH}
- Building these UI frontends: ${UI_FRONTENDS}
- Building command-line frontend: ${CLI_BUILD_MSG}
- GNOME Tracker API version: ${TRACKER_API_DISPLAY}
- libromdata is built as: ${LIBROMDATA_LIB_TYPE}
- Security mechanism: ${SECURITY_MECHANISM}
- Decryption functionality: ${ENABLE_DECRYPTION_MSG}
- APNG support: ${HAVE_PNG_APNG_MSG}
- XML parsing: ${ENABLE_XML_MSG}
- PVRTC decoder: ${ENABLE_PVRTC_MSG}
- ZSTD decompression: ${ENABLE_ZSTD_MSG}
- LZ4 decompression: ${ENABLE_LZ4_MSG}
- LZO decompression: ${ENABLE_LZO_MSG}

Building these third-party libraries from extlib:
${EXTLIB_BUILD}")

IF(NOT UI_FRONTENDS)
	MESSAGE(WARNING "*** No UI frontends are being compiled. ***")
	IF(NOT BUILD_CLI)
		MESSAGE(FATAL_ERROR "*** CLI frontend is not being compiled, either. ***")
	ENDIF(NOT BUILD_CLI)
ENDIF(NOT UI_FRONTENDS)

# TODO: Print build summary indicating what plugins will be built.
# (Some other project had something like this...)
# TODO: Fail if no plugins are being built.

### CPack ###

# CPack settings.
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${DESCRIPTION}")
SET(CPACK_PACKAGE_NAME "${PACKAGE_NAME}")
SET(CPACK_PACKAGE_VENDOR "${AUTHOR}")
SET(CPACK_PACKAGE_CONTACT "David Korth <gerbilsoft@gerbilsoft.com>")
SET(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})
SET(CPACK_PACKAGE_VERSION_TWEAK ${CMAKE_PROJECT_VERSION_TWEAK})
SET(CPACK_PACKAGE_VERSION ${VERSION_STRING})

# TODO: DESCRIPTION and WELCOME files.
#SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/doc/DESCRIPTION.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
SET(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
#SET(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_SOURCE_DIR}/doc/WELCOME.txt")

IF(INSTALL_DOC)
	# Additional document files for the root directory.
	# TODO: Convert from Unix line endings to Windows when
	# compiling for Windows?
	INCLUDE(DirInstallPaths)
	INSTALL(FILES NETWORK.md
		DESTINATION "${DIR_INSTALL_DOC_ROOT}"
		COMPONENT "doc")
ENDIF(INSTALL_DOC)

# CPack: Source package settings.
# NOTE: Double-escape is required because the unescaped
# string # is written to CPackSourceConfig.cmake, which
# is then unescaped.
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES
	"build.*/"
	"build.*\\\\.sh"
	"\\\\.git/"
	"\\\\.gitignore"
	"*\\\\.kate-swp"
	)

IF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.|x)86\$")
	SET(CPACK_PACKAGE_SYSTEM_PROCESSOR "i386")
ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64\$")
	SET(CPACK_PACKAGE_SYSTEM_PROCESSOR "amd64")
ELSE()
	SET(CPACK_PACKAGE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
ENDIF()

IF(APPLE)
	# TODO: Support for Mac OS X.
ELSEIF(WIN32)
	IF(MSVC AND CMAKE_CL_64)
		SET(WIN32_PACKAGE_SUFFIX "win64")
	ELSEIF(NOT MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		SET(WIN32_PACKAGE_SUFFIX "win64")
	ELSE()
		SET(WIN32_PACKAGE_SUFFIX "win32")
	ENDIF()

	SET(CPACK_GENERATOR "ZIP")
	SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${WIN32_PACKAGE_SUFFIX}")
	SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
ENDIF()

# Components.
SET(CPACK_COMPONENTS_ALL dll program debug doc i18n)
SET(CPACK_COMPONENT_DLL_DISPLAY_NAME "DLLs")
SET(CPACK_COMPONENT_PROGRAM_DISPLAY_NAME "Programs")
SET(CPACK_COMPONENT_DEBUG_DISPLAY_NAME "Debugging Symbols")
SET(CPACK_COMPONENT_DOC_DISPLAY_NAME "Documents")
SET(CPACK_COMPONENT_I18N_DISPLAY_NAME "Internationalization")

# Initialize CPack.
INCLUDE(CPack)
