# gcc (and other Unix-like compilers, e.g. MinGW)
IF(CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?[Cc]lang")
	# FIXME: May need later than 2.9.
	IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "2.9")
		MESSAGE(FATAL_ERROR "clang-2.9 or later is required.")
	ENDIF()
ELSEIF(CMAKE_COMPILER_IS_GNUCXX)
	IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.5")
		MESSAGE(FATAL_ERROR "gcc-4.5 or later is required.")
	ENDIF()
ENDIF()

# Compiler flag modules.
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCXXCompilerFlag)

SET(RP_C_FLAGS_COMMON "")
SET(RP_CXX_FLAGS_COMMON "")
SET(RP_EXE_LINKER_FLAGS_COMMON "")

# _GNU_SOURCE is needed for memmem() and statx().
ADD_DEFINITIONS(-D_GNU_SOURCE=1)

# Test for common CFLAGS and CXXFLAGS.
# NOTE: Not adding -Werror=format-nonliteral because there are some
# legitimate uses of non-literal format strings.
SET(CFLAGS_WARNINGS -Wall -Wextra -Wno-multichar -Werror=return-type -Wheader-hygiene -Wno-psabi)
SET(CFLAGS_WERROR_FORMAT -Werror=format -Werror=format-security -Werror=format-signedness -Werror=format-truncation -Werror=format-y2k)
SET(CFLAGS_OPTIONS -fstrict-aliasing -Werror=strict-aliasing -fno-common -fcf-protection -fno-math-errno)
IF(MINGW)
	# MinGW: Ignore warnings caused by casting from GetProcAddress().
	SET(CFLAGS_WARNINGS ${CFLAGS_WARNINGS} -Wno-cast-function-type)
ENDIF(MINGW)
FOREACH(FLAG_TEST ${CFLAGS_WARNINGS} ${CFLAGS_WERROR_FORMAT} ${CFLAGS_OPTIONS})
	# CMake doesn't like certain characters in variable names.
	STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST_VARNAME})
	IF(CFLAG_${FLAG_TEST_VARNAME})
		SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CFLAG_${FLAG_TEST_VARNAME})
	UNSET(CFLAG_${FLAG_TEST_VARNAME})

	CHECK_CXX_COMPILER_FLAG("${FLAG_TEST}" CXXFLAG_${FLAG_TEST_VARNAME})
	IF(CXXFLAG_${FLAG_TEST_VARNAME})
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CXXFLAG_${FLAG_TEST_VARNAME})
	UNSET(CXXFLAG_${FLAG_TEST_VARNAME})
ENDFOREACH(FLAG_TEST)

# Certain warnings should be errors. (C only)
SET(CFLAGS_WERROR_C_ONLY -Werror=implicit -Werror=implicit-function-declaration -Werror=incompatible-pointer-types -Werror=int-conversion)
FOREACH(FLAG_TEST ${CFLAGS_WERROR_C_ONLY})
	# CMake doesn't like certain characters in variable names.
	STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST_VARNAME})
	IF(CFLAG_${FLAG_TEST_VARNAME})
		SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CFLAG_${FLAG_TEST_VARNAME})
	UNSET(CFLAG_${FLAG_TEST_VARNAME})
ENDFOREACH(FLAG_TEST)

# Enable "suggest override" if available. (C++ only)
# NOTE: If gcc, only enable on 9.2 and later, since earlier versions
# will warn if a function is marked 'final' but not 'override'
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78010
IF(NOT CMAKE_COMPILER_IS_GNUCC OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.1))
	CHECK_CXX_COMPILER_FLAG("-Wsuggest-override" CXXFLAG_SUGGEST_OVERRIDE)
	IF(CXXFLAG_SUGGEST_OVERRIDE)
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} -Wsuggest-override -Wno-error=suggest-override")
	ENDIF(CXXFLAG_SUGGEST_OVERRIDE)
	UNSET(CXXFLAG_SUGGEST_OVERRIDE)
ENDIF(NOT CMAKE_COMPILER_IS_GNUCC OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.1))

# Code coverage checking.
IF(ENABLE_COVERAGE)
	# Partially based on:
	# https://github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake
	# (commit 59f8ab8dded56b490dec388ac6ad449318de8779)
	IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
		# CMake 3.0.0 added code coverage support.
		IF("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 3)
			MESSAGE(FATAL_ERROR "clang-3.0.0 or later is required for code coverage testing.")
		ENDIF()
	ELSEIF(NOT CMAKE_COMPILER_IS_GNUCXX)
		MESSAGE(FATAL_ERROR "Code coverage testing is currently only supported on gcc and clang.")
	ENDIF()

	# Don't bother checking for the coverage options.
	# We're assuming they're always supported.
	SET(RP_C_FLAGS_COVERAGE "--coverage -fprofile-arcs -ftest-coverage -fprofile-update=atomic")
	SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${RP_C_FLAGS_COVERAGE}")
	SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${RP_C_FLAGS_COVERAGE}")
	SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${RP_C_FLAGS_COVERAGE}")

	# Create a code coverage target.
	FOREACH(_program gcov lcov genhtml)
		FIND_PROGRAM(${_program}_PATH ${_program})
		IF(NOT ${_program}_PATH)
			MESSAGE(FATAL_ERROR "${_program} not found; cannot enable code coverage testing.")
		ENDIF(NOT ${_program}_PATH)
		UNSET(${_program}_PATH)
	ENDFOREACH(_program)

	ADD_CUSTOM_TARGET(coverage
		WORKING_DIRECTORY "${CMAKE_BUILD_DIR}"
		COMMAND ${POSIX_SH} "${CMAKE_SOURCE_DIR}/scripts/lcov.sh" "${CMAKE_BUILD_TYPE}" "rom-properties" "coverage"
		)
ENDIF(ENABLE_COVERAGE)

# Test for common LDFLAGS.
# NOTE: CHECK_C_COMPILER_FLAG() doesn't seem to work, even with
# CMAKE_TRY_COMPILE_TARGET_TYPE. Check `ld --help` for the various
# parameters instead.
# NOTE 2: Emscripten doesn't like these LDFLAGS.
IF(NOT EMSCRIPTEN)
EXECUTE_PROCESS(COMMAND ${CMAKE_LINKER} --help
	OUTPUT_VARIABLE _ld_out
	ERROR_QUIET)

FOREACH(FLAG_TEST "--sort-common" "--as-needed" "--build-id" "-Bsymbolic-functions" "--no-undefined" "--no-allow-shlib-undefined")
	IF(NOT DEFINED LDFLAG_${FLAG_TEST})
		MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST}")
		IF(_ld_out MATCHES "${FLAG_TEST}")
			MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - yes")
			SET(LDFLAG_${FLAG_TEST} 1 CACHE INTERNAL "Linker supports ${FLAG_TEST}")
		ELSE()
			MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - no")
			SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports ${FLAG_TEST}")
		ENDIF()
	ENDIF()

	IF(LDFLAG_${FLAG_TEST})
		SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} -Wl,${FLAG_TEST}")
	ENDIF(LDFLAG_${FLAG_TEST})
ENDFOREACH()

# Special case for -O/-O1.
SET(FLAG_TEST "-O")
	IF(NOT DEFINED LDFLAG_${FLAG_TEST})
		MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST}")
		IF(_ld_out MATCHES "${FLAG_TEST}")
			MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - yes")
			SET(LDFLAG_${FLAG_TEST} 1 CACHE INTERNAL "Linker supports ${FLAG_TEST}")
		ELSE()
			MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - no")
			SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports ${FLAG_TEST}")
		ENDIF()
	ENDIF()

	IF(LDFLAG_${FLAG_TEST})
		SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} -Wl,-O1")
	ENDIF(LDFLAG_${FLAG_TEST})
UNSET(FLAG_TEST)

# Special case for --compress-debug-sections.
SET(FLAG_TEST "--compress-debug-sections")
	IF(NOT DEFINED LDFLAG_${FLAG_TEST})
		MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST}")
		IF(CMAKE_SYSTEM_NAME STREQUAL "NetBSD" OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
			# FIXME: Do an actual runtime test.
			# NetBSD/OpenBSD ld has the option, but it fails at runtime:
			# ld: error: --compress-debug-sections: zlib is not available
			MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - yes, but not usable")
			SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports ${FLAG_TEST}")
		ELSEIF(_ld_out MATCHES "${FLAG_TEST}")
			MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - yes")
			SET(LDFLAG_${FLAG_TEST} 1 CACHE INTERNAL "Linker supports ${FLAG_TEST}")
		ELSE()
			MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - no")
			SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports ${FLAG_TEST}")
		ENDIF()
	ENDIF()

	IF(LDFLAG_${FLAG_TEST})
		SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} -Wl,${FLAG_TEST}=zlib")
	ENDIF(LDFLAG_${FLAG_TEST})
UNSET(FLAG_TEST)

IF(NOT WIN32)
	# Bsymbolic-functions doesn't make sense on Windows.
	FOREACH(FLAG_TEST "-Bsymbolic-functions")
		IF(NOT DEFINED LDFLAG_${FLAG_TEST})
			MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST}")
			IF(_ld_out MATCHES "${FLAG_TEST}")
				MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - yes")
				SET(LDFLAG_${FLAG_TEST} 1 CACHE INTERNAL "Linker supports ${FLAG_TEST}")
			ELSE()
				MESSAGE(STATUS "Checking if ld supports ${FLAG_TEST} - no")
				SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports ${FLAG_TEST}")
			ENDIF()
		ENDIF()

		IF(LDFLAG_${FLAG_TEST})
			SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} -Wl,${FLAG_TEST}")
		ENDIF(LDFLAG_${FLAG_TEST})
	ENDFOREACH()
ENDIF(NOT WIN32)

# Check for -Wl,-z,pack-relative-relocs.
# Requires binutils-2.38 and glibc-2.36. (TODO: Check other Unix systems.)
IF(UNIX AND NOT APPLE)
	IF(NOT DEFINED HAVE_DT_RELR)
		MESSAGE(STATUS "Checking if the system supports DT_RELR")
		IF(_ld_out MATCHES "-z pack-relative-relocs")
			# NOTE: ${CMAKE_MODULE_PATH} has two directories, macros/ and libs/,
			# so we have to configure this manually.
			SET(DT_RELR_SOURCE_PATH "${CMAKE_SOURCE_DIR}/cmake/platform")

			# TODO: Better cross-compile handling.
			TRY_RUN(TMP_DT_RELR_RUN TMP_DT_RELR_COMPILE
				"${CMAKE_CURRENT_BINARY_DIR}"
				"${DT_RELR_SOURCE_PATH}/DT_RELR_Test.c")
			IF(TMP_DT_RELR_COMPILE AND (TMP_DT_RELR_RUN EQUAL 0))
				SET(TMP_HAVE_DT_RELR TRUE)
				MESSAGE(STATUS "Checking if the system supports DT_RELR - yes")
			ELSE()
				SET(TMP_HAVE_DT_RELR FALSE)
				MESSAGE(STATUS "Checking if the system supports DT_RELR - no, needs glibc-2.36 or later")
			ENDIF()
		ELSE(_ld_out MATCHES "-z pack-relative-relocs")
			SET(TMP_HAVE_DT_RELR FALSE)
			MESSAGE(STATUS "Checking if the system supports DT_RELR - no, needs binutils-2.38 or later")
		ENDIF(_ld_out MATCHES "-z pack-relative-relocs")
		SET(HAVE_DT_RELR ${TMP_HAVE_DT_RELR} CACHE INTERNAL "System supports DT_RELR")
		UNSET(TMP_HAVE_DT_RELR)
	ENDIF(NOT DEFINED HAVE_DT_RELR)

	IF(HAVE_DT_RELR)
		SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} -Wl,-z,pack-relative-relocs")
	ENDIF(HAVE_DT_RELR)
ENDIF(UNIX AND NOT APPLE)

SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")
SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")

# Special case: On non-Linux systems, remove "--no-undefined" and
# "--no-allow-shlib-undefined" from SHARED and MODULE linker flags.
# On FreeBSD 13.2, `environ` and `__progname` are intentionally undefined,
# so this *always* fails when building a shared library.
IF(NOT CMAKE_SYSTEM MATCHES "Linux")
	FOREACH(FLAG_REMOVE "--no-undefined" "--no-allow-shlib-undefined")
		STRING(REPLACE "-Wl,${FLAG_REMOVE}" "" RP_SHARED_LINKER_FLAGS_COMMON "${RP_SHARED_LINKER_FLAGS_COMMON}")
		STRING(REPLACE "-Wl,${FLAG_REMOVE}" "" RP_MODULE_LINKER_FLAGS_COMMON "${RP_MODULE_LINKER_FLAGS_COMMON}")
	ENDFOREACH(FLAG_REMOVE)
ENDIF(NOT CMAKE_SYSTEM MATCHES "Linux")
ENDIF(NOT EMSCRIPTEN)

# Debug builds: Check for -Og.
# This flag was added in gcc-4.8, and enables optimizations that
# don't interfere with debugging.
CHECK_C_COMPILER_FLAG("-Og" CFLAG_OPTIMIZE_DEBUG)
IF (CFLAG_OPTIMIZE_DEBUG)
	SET(CFLAG_OPTIMIZE_DEBUG "-Og")
ELSE(CFLAG_OPTIMIZE_DEBUG)
	SET(CFLAG_OPTIMIZE_DEBUG "-O0")
ENDIF(CFLAG_OPTIMIZE_DEBUG)

# Release builds: Check for -ftree-vectorize.
# On i386, also add -mstackrealign to ensure proper stack alignment.
CHECK_C_COMPILER_FLAG("-ftree-vectorize" CFLAG_OPTIMIZE_FTREE_VECTORIZE)
IF(CFLAG_OPTIMIZE_FTREE_VECTORIZE)
	IF(arch MATCHES "^(i.|x)86$" AND NOT CMAKE_CL_64 AND ("${CMAKE_SIZEOF_VOID_P}" EQUAL 4))
		# i386: "-mstackrealign" is required.
		CHECK_C_COMPILER_FLAG("-mstackrealign" CFLAG_OPTIMIZE_MSTACKREALIGN)
		IF(CFLAG_OPTIMIZE_MSTACK_REALIGN)
			SET(CFLAGS_VECTORIZE "-ftree-vectorize -mstackrealign")
		ENDIF(CFLAG_OPTIMIZE_MSTACKREALIGN)
	ELSE()
		# Not i386. Add "-ftree-vectorize" without "-mstackrealign".
		SET(CFLAGS_VECTORIZE "-ftree-vectorize")
	ENDIF()
ENDIF(CFLAG_OPTIMIZE_FTREE_VECTORIZE)

# Add "-Werror" *after* checking for everything else.
SET(RP_C_FLAGS_COMMON   "${RP_C_FLAGS_COMMON} -Werror")
SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} -Werror")
SET(CFLAGS_WNO_ERROR -Wno-error=unknown-pragmas -Wno-error=address -Wno-error=attributes -Wno-error=unused-parameter -Wno-error=unused-but-set-variable -Wno-error=ignored-qualifiers -Wno-error=missing-field-initializers -Wno-error=unused-variable -Wno-error=unused-function -Wno-error=type-limits -Wno-error=empty-body -Wno-error=address-of-packed-member -Wno-error=shift-negative-value -Wno-error=clobbered -Wno-error=overloaded-virtual -Wno-error=header-hygiene -Wno-error=cast-align -Wno-error=stringop-overread)
FOREACH(FLAG_TEST ${CFLAGS_WNO_ERROR})
	# CMake doesn't like certain characters in variable names.
	STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST_VARNAME})
	IF(CFLAG_${FLAG_TEST_VARNAME})
		SET(RP_C_FLAGS_COMMON   "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CFLAG_${FLAG_TEST_VARNAME})
	UNSET(CFLAG_${FLAG_TEST_VARNAME})
ENDFOREACH(FLAG_TEST)

### Debug/Release flags ###

SET(RP_C_FLAGS_DEBUG		"${CFLAG_OPTIMIZE_DEBUG} -ggdb -DDEBUG -D_DEBUG")
SET(RP_CXX_FLAGS_DEBUG		"${CFLAG_OPTIMIZE_DEBUG} -ggdb -DDEBUG -D_DEBUG")

SET(RP_C_FLAGS_RELEASE		"-O2 -DNDEBUG ${CFLAGS_VECTORIZE}")
SET(RP_CXX_FLAGS_RELEASE	"-O2 -DNDEBUG ${CFLAGS_VECTORIZE}")

SET(RP_C_FLAGS_RELWITHDEBINFO	"-O2 -ggdb -DNDEBUG ${CFLAGS_VECTORIZE}")
SET(RP_CXX_FLAGS_RELWITHDEBINFO	"-O2 -ggdb -DNDEBUG ${CFLAGS_VECTORIZE}")

# Enable C++ assertions. (libstdc++ / libc++)
# Also enable C++ debug mode. (libstdc++ only)
# TODO: Only enable GLIBCXX for libstdc++, and LIBCPP for libc++.
SET(RP_CXX_FLAGS_DEBUG "${RP_CXX_FLAGS_DEBUG} -D_GLIBCXX_ASSERTIONS -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_ENABLE_ASSERTIONS -D_LIBCPP_ENABLE_HARDENED_MODE")

# Unset temporary variables.
UNSET(CFLAG_OPTIMIZE_DEBUG)

# Check for link-time optimization. (Release builds only.)
# FIXME: Running CMake with -DENABLE_LTO=ON, then -DENABLE_LTO=OFF
# doesn't seem to work right... (The flags aren't removed properly.)
# [Kubuntu 17.04, cmake-3.7.2]
IF(ENABLE_LTO)
	# We need two wrappers in order for LTO to work properly:
	# - gcc-ar: static library archiver
	# - gcc-ranlib: static library indexer
	# Without these wrappers, all sorts of undefined refernce errors
	# occur in gcc-4.9 due to "slim" LTO objects, and possibly
	# earlier versions for various reasons.
	MESSAGE(STATUS "Checking if the gcc LTO wrappers are available:")

	# gcc-5.4 and earlier have issues with LTO.
	IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
	   CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
		MESSAGE(STATUS "Checking if the gcc LTO wrappers are available: too old")
		MESSAGE(FATAL_ERROR "gcc 6.1 or later is required for LTO.")
	ENDIF()

	IF("${CMAKE_AR}" MATCHES "gcc-ar$")
		# Already using the gcc-ar wrapper.
		SET(GCC_WRAPPER_AR "${CMAKE_AR}")
	ELSE()
		# Replace ar with gcc-ar.
		STRING(REGEX REPLACE "ar$" "gcc-ar" GCC_WRAPPER_AR "${CMAKE_AR}")
	ENDIF()
	IF("${CMAKE_RANLIB}" MATCHES "gcc-ranlib$")
		# Already using the gcc-ranlib wrapper.
		SET(GCC_WRAPPER_RANLIB "${CMAKE_RANLIB}")
	ELSE()
		# Replace ranlib with gcc-ranlib.
		STRING(REGEX REPLACE "ranlib$" "gcc-ranlib" GCC_WRAPPER_RANLIB "${CMAKE_RANLIB}")
	ENDIF()

	IF(EXISTS "${GCC_WRAPPER_AR}" AND EXISTS "${GCC_WRAPPER_RANLIB}")
		# Found gcc binutils wrappers.
		SET(CMAKE_AR "${GCC_WRAPPER_AR}")
		SET(CMAKE_RANLIB "${GCC_WRAPPER_RANLIB}")
		MESSAGE(STATUS "Checking if the gcc LTO wrappers are available: yes")

		# Check for the LTO compiler flag.
		CHECK_C_COMPILER_FLAG("-flto" CFLAG_LTO)
		IF(CFLAG_LTO)
			SET(RP_C_FLAGS_RELEASE   "${RP_C_FLAGS_RELEASE} -flto")
			SET(RP_CXX_FLAGS_RELEASE "${RP_CXX_FLAGS_RELEASE} -flto")
			SET(RP_EXE_LINKER_FLAGS_RELEASE    "${RP_EXE_LINKER_FLAGS_RELEASE} -flto -fuse-linker-plugin")
			SET(RP_SHARED_LINKER_FLAGS_RELEASE "${RP_SHARED_LINKER_FLAGS_RELEASE} -flto -fuse-linker-plugin")
			SET(RP_MODULE_LINKER_FLAGS_RELEASE "${RP_MODULE_LINKER_FLAGS_RELEASE} -flto -fuse-linker-plugin")

			SET(RP_C_FLAGS_RELWITHDEBINFO   "${RP_C_FLAGS_RELWITHDEBINFO} -flto")
			SET(RP_CXX_FLAGS_RELWITHDEBINFO "${RP_CXX_FLAGS_RELWITHDEBINFO} -flto")
			SET(RP_EXE_LINKER_FLAGS_RELWITHDEBINFO    "${RP_EXE_LINKER_FLAGS_RELWITHDEBINFO} -flto -fuse-linker-plugin")
			SET(RP_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${RP_SHARED_LINKER_FLAGS_RELWITHDEBINFO} -flto -fuse-linker-plugin")
			SET(RP_MODULE_LINKER_FLAGS_RELWITHDEBINFO "${RP_MODULE_LINKER_FLAGS_RELWITHDEBINFO} -flto -fuse-linker-plugin")
		ELSE(CFLAG_LTO)
			MESSAGE(FATAL_ERROR "LTO optimization requested but -flto is not supported.")
		ENDIF(CFLAG_LTO)
		UNSET(CFLAG_LTO)
	ELSE()
		MESSAGE(STATUS "Checking if the gcc LTO wrappers are available: no")
		MESSAGE(FATAL_ERROR "gcc binutils wrappers not found; cannot enable LTO.")
	ENDIF()
ENDIF(ENABLE_LTO)

# Use thin archives on Linux.
# References:
# - https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=a5967db9af51a84f5e181600954714a9e4c69f1f
# - https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=9a6cfca4f4130444cb02536a4fdf7b6e285c713e
# - https://bugs.webkit.org/show_bug.cgi?id=108330
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> qcTP <TARGET> <LINK_FLAGS> <OBJECTS>")
	SET(CMAKE_C_ARCHIVE_APPEND   "<CMAKE_AR> qTP  <TARGET> <LINK_FLAGS> <OBJECTS>")
	SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qcTP <TARGET> <LINK_FLAGS> <OBJECTS>")
	SET(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> qTP  <TARGET> <LINK_FLAGS> <OBJECTS>")
ENDIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
