# boost-time64 (64-bit time functions from boost-1.90.0)
PROJECT(boost-time64 LANGUAGES C)

# Check for 64-bit time support.
INCLUDE(Check64BitTimeSupport)
CHECK_64BIT_TIME_SUPPORT()

# Check the size of time_t.
INCLUDE(CheckTypeSize)
SET(CMAKE_REQUIRED_INCLUDES "time.h")
IF(TMP_TIME64_DEFINITIONS)
	SET(CMAKE_REQUIRED_DEFINITIONS ${LFS_DEFINITIONS} ${TMP_TIME64_DEFINITIONS})
ENDIF(TMP_TIME64_DEFINITIONS)
CHECK_TYPE_SIZE("time_t" TIME_T)
IF(NOT TIME_T)
	MESSAGE(FATAL_ERROR "Cannot determine the size of time_t!")
ENDIF(NOT TIME_T)

# Check for reentrant time functions.
# NOTE: May be _gmtime32_s() or _gmtime64_s() on MSVC 2005+.
# The "inline" part will detect that.
INCLUDE(CheckSymbolExistsOrInline)
SET(CMAKE_REQUIRED_DEFINITIONS -D_POSIX_C_SOURCE=1 -D_BSD_SOURCE=1)
CHECK_SYMBOL_EXISTS_OR_INLINE(gmtime_r "time.h" "time_t tm; gmtime_r(&tm, NULL);" HAVE_GMTIME_R)
UNSET(CMAKE_REQUIRED_DEFINITIONS)
IF(NOT HAVE_GMTIME_R)
	CHECK_SYMBOL_EXISTS_OR_INLINE(gmtime_s "time.h" "time_t tm; gmtime_s(NULL, &tm);" HAVE_GMTIME_S)
ENDIF(NOT HAVE_GMTIME_R)
SET(CMAKE_REQUIRED_DEFINITIONS -D_POSIX_C_SOURCE=1 -D_BSD_SOURCE=1)
CHECK_SYMBOL_EXISTS_OR_INLINE(localtime_r "time.h" "time_t tm; localtime_r(&tm, NULL);" HAVE_LOCALTIME_R)
UNSET(CMAKE_REQUIRED_DEFINITIONS)
IF(NOT HAVE_LOCALTIME_R)
	CHECK_SYMBOL_EXISTS_OR_INLINE(localtime_s "time.h" "time_t tm; localtime_s(NULL, &tm);" HAVE_LOCALTIME_S)
ENDIF(NOT HAVE_LOCALTIME_R)

# Other time functions.
SET(CMAKE_REQUIRED_DEFINITIONS -D_DEFAULT_SOURCE=1 -D_BSD_SOURCE=1)
CHECK_SYMBOL_EXISTS_OR_INLINE(timegm "time.h" "struct tm tm; time_t x = timegm(&tm);" HAVE_TIMEGM)
UNSET(CMAKE_REQUIRED_DEFINITIONS)
IF(NOT HAVE_TIMEGM)
	# NOTE: MSVCRT's _mkgmtime64() has a range of [1970/01/01, 3000/12/31].
	# glibc and boost both support arbitrary ranges.
	CHECK_SYMBOL_EXISTS_OR_INLINE(_mkgmtime64 "time.h" "struct tm tm; time_t x = _mkgmtime64(&tm);" HAVE__MKGMTIME64)
	IF(MSVC AND NOT HAVE__MKGMTIME64)
		MESSAGE(FATAL_ERROR "MSVC should always have _mkgmtime64()!")
	ENDIF(MSVC AND NOT HAVE__MKGMTIME64)
ENDIF(NOT HAVE_TIMEGM)
IF(TIME_T LESS 8 OR (NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME64))
	# System does not have timegm() or _mkgmtime64(), or time_t is not 64-bit.
	# Use the internal rp_timegm() function.
	SET(USING_INTERNAL_RP_TIMEGM 1)
	SET(USING_INTERNAL_RP_TIMEGM 1 PARENT_SCOPE)
ENDIF(TIME_T LESS 8 OR (NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME64))

# If time_t is not 64-bit, use the internal rp_gmtime() function.
# TODO: Add an internal rp_localtime() function?
IF(TIME_T LESS 8)
	SET(USING_INTERNAL_RP_GMTIME 1)
	SET(USING_INTERNAL_RP_GMTIME 1 PARENT_SCOPE)
ENDIF(TIME_T LESS 8)

# Write the boost-time64 configuration file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.boost-time64.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.boost-time64.h")

# Hide all symbols so they're not exported by libromdata.
CMAKE_POLICY(SET CMP0063 NEW)
SET(CMAKE_C_VISIBILITY_PRESET "hidden")
SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

ADD_LIBRARY(${PROJECT_NAME} STATIC time64_funcs.c)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	)
