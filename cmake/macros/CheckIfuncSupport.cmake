# Check for IFUNC support.
# Requires ELF. Currently supported by gcc, but not clang.

FUNCTION(CHECK_IFUNC_SUPPORT)
	# NOTE: ${CMAKE_MODULE_PATH} has two directories, macros/ and libs/,
	# so we have to configure this manually.
	SET(IFUNC_SOURCE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

	# NOTE: If -fsanitize is specified, disable IFUNC.
	# Otherwise, SIGSEGV will happen.
	IF(CMAKE_C_FLAGS MATCHES "-fsanitize" OR CMAKE_CXX_FLAGS MATCHES "-fsanitize")
		SET(IS_SANITIZE 1)
	ELSEIF(CMAKE_C_FLAGS_${BUILD_TYPE_UCASE} MATCHES "-fsanitize" OR CMAKE_CXX_FLAGS_${BUILD_TYPE_UCASE} MATCHES "-fsanitize")
		SET(IS_SANITIZE 1)
	ENDIF()

	# Check for IFUNC support.
	MESSAGE(STATUS "Checking if IFUNC is supported")
	IF(WIN32 OR APPLE)
		# No IFUNC for Windows or Mac OS X.
		# Shortcut here to avoid needing a cross-compiled test program.
		MESSAGE(STATUS "Checking if IFUNC is supported - no")
		SET(TMP_HAVE_IFUNC FALSE)
	ELSEIF(IS_SANITIZE)
		MESSAGE(STATUS "Checking if IFUNC is supported - disabled due to -fsanitize")
		SET(TMP_HAVE_IFUNC FALSE)
	ELSE()
		TRY_RUN(TMP_HAVE_IFUNC_RUN TMP_HAVE_IFUNC_COMPILE
			"${CMAKE_BINARY_DIR}"
			"${IFUNC_SOURCE_PATH}/IfuncTest.c")
		IF(TMP_HAVE_IFUNC_COMPILE AND (TMP_HAVE_IFUNC_RUN EQUAL 0))
			MESSAGE(STATUS "Checking if IFUNC is supported - yes")
			SET(TMP_HAVE_IFUNC TRUE)
		ELSE()
			MESSAGE(STATUS "Checking if IFUNC is supported - no")
			SET(TMP_HAVE_IFUNC FALSE)
		ENDIF()
	ENDIF()

	SET(HAVE_IFUNC ${TMP_HAVE_IFUNC} CACHE INTERNAL "Is IFUNC supported?")
ENDFUNCTION(CHECK_IFUNC_SUPPORT)
