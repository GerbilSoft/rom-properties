FUNCTION(CHECK_TIME_FUNCTIONS)
	# Check for 64-bit time support.
	INCLUDE(Check64BitTimeSupport)
	CHECK_64BIT_TIME_SUPPORT()

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
		SET(USING_INTERNAL_RP_TIMEGM 1 PARENT_SCOPE)
	ENDIF(TIME_T LESS 8 OR (NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME64))

	# If time_t is not 64-bit, use the internal rp_gmtime() function.
	# TODO: Add an internal rp_localtime() function?
	IF(TIME_T LESS 8)
		SET(USING_INTERNAL_RP_GMTIME 1 PARENT_SCOPE)
	ENDIF(TIME_T LESS 8)
ENDFUNCTION(CHECK_TIME_FUNCTIONS)
