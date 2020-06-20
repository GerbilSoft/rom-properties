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

# gcc-5.4 and earlier have issues with LTO.
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
	SET(GCC_5xx_LTO_ISSUES ON)
ENDIF()

# Compiler flag modules.
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCXXCompilerFlag)

# Check what flag is needed for C11 and/or C99 support.
INCLUDE(CheckC11C99CompilerFlag)
CHECK_C11_C99_COMPILER_FLAG(RP_C11_CFLAG)

# Check what flag is needed for C++ 2011 support.
INCLUDE(CheckCXX11CompilerFlag)
CHECK_CXX11_COMPILER_FLAG(RP_CXX11_CXXFLAG)

SET(RP_C_FLAGS_COMMON "${RP_C11_CFLAG}")
SET(RP_CXX_FLAGS_COMMON "${RP_CXX11_CXXFLAG}")
SET(RP_EXE_LINKER_FLAGS_COMMON "")

# _GNU_SOURCE is needed for memmem() and statx().
ADD_DEFINITIONS(-D_GNU_SOURCE=1)

UNSET(RP_C11_CFLAG)
UNSET(RP_CXX11_CXXFLAG)

# Test for common CFLAGS and CXXFLAGS.
FOREACH(FLAG_TEST "-Wall" "-Wextra" "-Wno-multichar" "-fstrict-aliasing" "-fno-common")
	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST})
	IF(CFLAG_${FLAG_TEST})
		SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CFLAG_${FLAG_TEST})
	UNSET(CFLAG_${FLAG_TEST})

	CHECK_CXX_COMPILER_FLAG("${FLAG_TEST}" CXXFLAG_${FLAG_TEST})
	IF(CXXFLAG_${FLAG_TEST})
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CXXFLAG_${FLAG_TEST})
	UNSET(CXXFLAG_${FLAG_TEST})
ENDFOREACH()

# -Wimplicit-function-declaration should be an error. (C only)
CHECK_C_COMPILER_FLAG("-Werror=implicit-function-declaration" CFLAG_IMPLFUNC)
IF(CFLAG_IMPLFUNC)
	SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} -Werror=implicit-function-declaration")
ENDIF(CFLAG_IMPLFUNC)
UNSET(CFLAG_IMPLFUNC)

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
	SET(RP_C_FLAGS_COVERAGE "--coverage -fprofile-arcs -ftest-coverage")
	SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${RP_C_FLAGS_COVERAGE}")
	SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${RP_C_FLAGS_COVERAGE}")

	# Link gcov to all targets.
	SET(GCOV_LIBRARY "-lgcov")
	FOREACH(VAR "" C_ CXX_)
		IF(CMAKE_${VAR}STANDARD_LIBRARIES)
			SET(CMAKE_${VAR}STANDARD_LIBRARIES "${CMAKE_${VAR}STANDARD_LIBRARIES} ${GCOV_LIBRARY}")
		ELSE(CMAKE_${VAR}STANDARD_LIBRARIES)
			SET(CMAKE_${VAR}STANDARD_LIBRARIES "${GCOV_LIBRARY}")
		ENDIF(CMAKE_${VAR}STANDARD_LIBRARIES)
	ENDFOREACH(VAR)

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
EXECUTE_PROCESS(COMMAND ${CMAKE_LINKER} --help
	OUTPUT_VARIABLE _ld_out
	ERROR_QUIET)

FOREACH(FLAG_TEST "--sort-common" "--as-needed" "--build-id")
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

SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")
SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")

# Check for -Og.
# This flag was added in gcc-4.8, and enables optimizations that
# don't interfere with debugging.
CHECK_C_COMPILER_FLAG("-Og" CFLAG_OPTIMIZE_DEBUG)
IF (CFLAG_OPTIMIZE_DEBUG)
	SET(CFLAG_OPTIMIZE_DEBUG "-Og")
ELSE(CFLAG_OPTIMIZE_DEBUG)
	SET(CFLAG_OPTIMIZE_DEBUG "-O0")
ENDIF(CFLAG_OPTIMIZE_DEBUG)

# Debug/release flags.
SET(RP_C_FLAGS_DEBUG		"${CFLAG_OPTIMIZE_DEBUG} -ggdb -DDEBUG -D_DEBUG")
SET(RP_CXX_FLAGS_DEBUG		"${CFLAG_OPTIMIZE_DEBUG} -ggdb -DDEBUG -D_DEBUG")
SET(RP_C_FLAGS_RELEASE		"-O2 -ggdb -DNDEBUG")
SET(RP_CXX_FLAGS_RELEASE	"-O2 -ggdb -DNDEBUG")

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
