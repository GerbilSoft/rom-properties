PROJECT(src)

# Check for Large File Support.
# NOTE: This should have been done in platform.cmake, but we're
# including it here because the off_t/off64_t sizes are written
# to config.libc.h.
INCLUDE(CheckLargeFileSupport)
CHECK_LARGE_FILE_SUPPORT()

# Check for dirent.h. (for d_type.h)
CHECK_INCLUDE_FILE("dirent.h" HAVE_DIRENT_H)
# Check for sys/stat.h. (for d_type.h)
# Windows Universal CRT has this; older versions of MSVCRT do not.
CHECK_INCLUDE_FILE("sys/stat.h" HAVE_SYS_STAT_H)

# Check for reentrant time functions.
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

# Other time functions.
CHECK_SYMBOL_EXISTS_OR_INLINE(timegm "time.h" "struct tm tm; time_t x = timegm(&tm);" HAVE_TIMEGM)
IF(NOT HAVE_TIMEGM)
	# NOTE: MSVCRT's _mkgmtime64() has a range of [1970/01/01, 3000/12/31].
	# glibc and boost both support arbitrary ranges.
	CHECK_SYMBOL_EXISTS_OR_INLINE(_mkgmtime "time.h" "struct tm tm; time_t x = _mkgmtime(&tm);" HAVE__MKGMTIME)
	CHECK_SYMBOL_EXISTS_OR_INLINE(_mkgmtime32 "time.h" "struct tm tm; time_t x = _mkgmtime(&tm);" HAVE__MKGMTIME32)
	CHECK_SYMBOL_EXISTS_OR_INLINE(_mkgmtime64 "time.h" "struct tm tm; time_t x = _mkgmtime64(&tm);" HAVE__MKGMTIME64)
ENDIF(NOT HAVE_TIMEGM)
IF(NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME AND NOT HAVE__MKGMTIME32 AND NOT HAVE__MKGMTIME64)
	MESSAGE(FATAL_ERROR "timegm() or equivalent function not found.")
ENDIF(NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME AND NOT HAVE__MKGMTIME32 AND NOT HAVE__MKGMTIME64)

# Aligned malloc() functions.
# NOTE: MinGW-w64 only has _aligned_malloc().
# It doesn't even have inline function wrappers
# for the other functions.
CHECK_SYMBOL_EXISTS(_aligned_malloc	"malloc.h"		HAVE_MSVC_ALIGNED_MALLOC)
# FIXME: aligned_alloc isn't working on properly on Mac OS.
#CHECK_SYMBOL_EXISTS(aligned_alloc	"stdlib.h"		HAVE_ALIGNED_ALLOC)
CHECK_SYMBOL_EXISTS(posix_memalign	"stdlib.h"		HAVE_POSIX_MEMALIGN)
CHECK_SYMBOL_EXISTS(memalign		"stdlib.h;malloc.h"	HAVE_MEMALIGN)
IF(NOT HAVE_MSVC_ALIGNED_MALLOC AND NOT HAVE_ALIGNED_ALLOC AND NOT HAVE_POSIX_MEMALIGN AND NOT HAVE_MEMALIGN)
	MESSAGE(FATAL_ERROR "System does not have an aligned malloc() function.")
ENDIF()

# Write the libc configuration file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libc.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libc.h")

# C++11 compatibility header.
# NOTE: This must be included regardless of C++11 support in the compiler.
# gcc-4.6 supports some C++11, but is missing explicit virtual overrides.
SET(RP_CONFIG_LIBC_H "${CMAKE_CURRENT_BINARY_DIR}/config.libc.h")
SET(RP_COMPILER_COMPAT_H "${CMAKE_CURRENT_SOURCE_DIR}/compiler-compat.h")
IF(MSVC)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -FI${RP_CONFIG_LIBC_H} -FI${RP_COMPILER_COMPAT_H}")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -FI${RP_CONFIG_LIBC_H} -FI${RP_COMPILER_COMPAT_H}")
ELSE(MSVC)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -include ${RP_CONFIG_LIBC_H} -include ${RP_COMPILER_COMPAT_H}")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -include ${RP_CONFIG_LIBC_H} -include ${RP_COMPILER_COMPAT_H}")
ENDIF(MSVC)

# Check for hidden visibility flags.
INCLUDE(CheckHiddenVisibility)
CHECK_HIDDEN_VISIBILITY()

# Check for stdbool.h.
INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE("stdbool.h" HAVE_STDBOOL_H)

# glib-2.32 (released 2012/03/24) added GResource and GMenuModel.
# glib-2.34 (released 2012/09/23) added g_clear_object().
# glib-2.44 (released 2015/03/23) added GListModel, which is needed by GTK4.
# glib-2.66 (released 2020/09/10) is required by GTK 4.0.0.
# Ubuntu 14.04 has glib-2.40.
# NOTE: Only setting to 2.34.0 here. GTK4 build will need to check for 2.66.0.
SET(GLIB_MIN_VERSION 2.34.0)

# Write the version number to config.version.h.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.version.h")

# Write stdboolx.h.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/stdboolx.h.in" "${CMAKE_CURRENT_BINARY_DIR}/stdboolx.h")

# Build certain libraries as shared libraries on Linux and Windows.
STRING(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UCASE)
IF(CMAKE_C_COMPILER MATCHES afl)
	# Don't enable libromdata.so when building with afl/afl++.
	# afl-fuzz crashes when trying to load libromdata.so for some reason.
	MESSAGE(WARNING "Disabling libromdata.so due to afl")
ELSEIF(CMAKE_C_FLAGS MATCHES "-fsanitize" OR CMAKE_CXX_FLAGS MATCHES "-fsanitize")
	# Don't enable libromdata.so when using a sanitizer.
	# The sanitizer requires typeinfo, which isn't exported for some classes.
	MESSAGE(WARNING "Disabling libromdata.so due to -fsanitize")
ELSEIF(CMAKE_C_FLAGS_${BUILD_TYPE_UCASE} MATCHES "-fsanitize" OR CMAKE_CXX_FLAGS_${BUILD_TYPE_UCASE} MATCHES "-fsanitize")
	# Don't enable libromdata.so when using a sanitizer.
	# The sanitizer requires typeinfo, which isn't exported for some classes.
	MESSAGE(WARNING "Disabling libromdata.so due to -fsanitize")
ELSEIF(EMSCRIPTEN)
	# Emscripten doesn't support shared libraries.
	MESSAGE(WARNING "Disabling libromdata.so due to Emscripten")
ELSEIF(WIN32 OR (UNIX AND NOT APPLE))
	SET(RP_LIBROMDATA_IS_DLL 1)
	SET(RP_LIBROMDATA_IS_DLL 1 PARENT_SCOPE)
	ADD_DEFINITIONS(-DRP_LIBROMDATA_IS_DLL=1)
ELSE()
	MESSAGE(WARNING "Disabling libromdata.so due to unsupported OS")
ENDIF()
UNSET(BUILD_TYPE_UCASE)

# FIXME: gcc-5's libgcc.a doesn't get linked in for some reason,
# resulting in the following error:
# /usr/bin/ld: ../../../bin/TextFuncsTest: hidden symbol `__cpu_indicator_init'
# in /usr/lib/gcc/i686-linux-gnu/5/libgcc.a(cpuinfo.o) is referenced by DSO
# Reference: https://github.com/baldurk/renderdoc/issues/365
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
	LINK_LIBRARIES(-lgcc)
ENDIF()

# Source Code subdirectories.
ADD_SUBDIRECTORY(librpsecure)
ADD_SUBDIRECTORY(libi18n)
ADD_SUBDIRECTORY(librpthreads)
ADD_SUBDIRECTORY(libcachecommon)
ADD_SUBDIRECTORY(rp-download)
ADD_SUBDIRECTORY(librpcpuid)
ADD_SUBDIRECTORY(librpbyteswap)
ADD_SUBDIRECTORY(librpfile)
ADD_SUBDIRECTORY(librptext)
ADD_SUBDIRECTORY(librpbase)
ADD_SUBDIRECTORY(librptexture)
ADD_SUBDIRECTORY(amiibo-data)
ADD_SUBDIRECTORY(libromdata)

IF(BUILD_CLI)
	ADD_SUBDIRECTORY(rpcli)
ENDIF(BUILD_CLI)

IF(UNIX AND NOT APPLE)
	IF(BUILD_KDE4 OR BUILD_KF5 OR BUILD_KF6)
		ADD_SUBDIRECTORY(kde)
	ENDIF(BUILD_KDE4 OR BUILD_KF5 OR BUILD_KF6)
	IF(BUILD_XFCE OR BUILD_GTK3)
		ADD_SUBDIRECTORY(res)
		ADD_SUBDIRECTORY(gtk)
	ENDIF(BUILD_XFCE OR BUILD_GTK3)
	ADD_SUBDIRECTORY(rp-stub)
ELSEIF(WIN32)
	IF(BUILD_WIN32)
		ADD_SUBDIRECTORY(win32)

		# NOTE: Only building svrplus for i386 for compatibility reasons.
		# All supported versions of Windows support running i386 executables.
		# IsWow64Process2() and/or IsWow64Process() can be used to determine
		# the actual host architecture.
		INCLUDE(CPUInstructionSetFlags)
		IF(CPU_i386)
			ADD_SUBDIRECTORY(svrplus)
		ENDIF(CPU_i386)
	ENDIF(BUILD_WIN32)
ENDIF()

# NOTE: libunixcommon must be *after* the UI frontends.
# Otherwise, the UI frontend checks won't work right, and on
# Unix systems, the qtpaths check will fail if Qt5 isn't
# installed and KF5 isn't explicitly disabled.
IF(WIN32)
	ADD_SUBDIRECTORY(libwin32common)
	ADD_SUBDIRECTORY(libwin32ui)
	ADD_SUBDIRECTORY(libwin32darkmode)
ELSE(WIN32)
	ADD_SUBDIRECTORY(libunixcommon)
ENDIF(WIN32)
