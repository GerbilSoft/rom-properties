# -*- cmake -*-
#
# unice68
project(unice68 C)

# rom-properties: Hide symbols by default, since we don't want them
# leaking from the static library to the plugins.
INCLUDE(CheckHiddenVisibility)
CHECK_HIDDEN_VISIBILITY()

# ======================================================================
# cmake setup

# rom-properties: Disabled cmake_minimum_required().
#cmake_minimum_required(VERSION 3.3)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

# rom-properties:
# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Disable certain warning flags.
IF(NOT MSVC)
	FOREACH(FLAG_TEST "-Wno-empty-body" "-Wno-shift-negative-value")
		CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST})
		IF(CFLAG_${FLAG_TEST})
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG_TEST}")
		ENDIF(CFLAG_${FLAG_TEST})
		UNSET(CFLAG_${FLAG_TEST})
	ENDFOREACH()
ENDIF(NOT MSVC)

# ======================================================================
# Modules

include(CheckIncludeFile)
include(CheckFunctionExists)
# rom-properties: Disabled GNUInstallDirs.
#include(GNUInstallDirs)

# ======================================================================
# Options

# rom-properties: Disabled options. Use hard-coded values instead.
IF(0)
option(
  BUILD_UNICE68_CLI
  "Build unice68 command line executable"
  ON)

option(
  BUILD_SHARED_LIBS
  "Prefer shared library over static"
  ON)

option(
  VERBOSE_MAKEFILE
  "Generate verbose Makefile"
  OFF)
set(CMAKE_VERBOSE_MAKEFILE ${VERBOSE_MAKEFILE})
ENDIF(0) # rom-properties

# ======================================================================
# Project

# rom-properties: Disabled this.
IF(0)
execute_process(
  COMMAND ./vcversion.sh 2.0.0
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  TIMEOUT 15
  RESULT_VARIABLE VC_RES
  OUTPUT_VARIABLE VC_OUT
  ERROR_QUIET)
if (NOT "0" STREQUAL "${VC_RES}")
  message(
    FATAL_ERROR
    "Failed to get revision from source control:\n${VC_RES}")
endif (NOT "0" STREQUAL "${VC_RES}")
string(STRIP "${VC_OUT}" VC_OUT)

project(
  unice68
  VERSION ${VC_OUT}
  LANGUAGES C)

set(PACKAGE "${PROJECT_NAME}")
set(VERSION "${PROJECT_VERSION}")
set(PACKAGE_BUGREPORT "http://sourceforge.net/projects/sc68")
set(PACKAGE_URL "http://sc68.atari.org")
set(PACKAGE_DESC "Ice packer/depacker program and library")
set(PACKAGE_SHORTDESC "${PACKAGE_DESC}")
ENDIF(0)

# ======================================================================
# Configs
#
# $$$ Shouldn't those file be by configuration instead of by build ?

set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
set(datarootdir "\${prefix}/${CMAKE_INSTALL_DATAROOTDIR}")
set(datadir "\${datarootdir}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
set(libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")

add_definitions(-DHAVE_CONFIG_H)

# ======================================================================
# Target library: unice68

CHECK_INCLUDE_FILE(assert.h HAVE_ASSERT_H)
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)

add_library(unice68_lib
  unice68_pack.c unice68_unpack.c unice68_version.c)

set_target_properties(
  unice68_lib PROPERTIES
  OUTPUT_NAME unice68
  C_STANDARD 99
  # rom-properties: Disabled... (CMP0063)
  #C_VISIBILITY_PRESET hidden
  )

# rom-properties: Target include directories.
target_include_directories(unice68_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# rom-properties: Disabled installation.
IF(0)
install(
  TARGETS unice68_lib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )

install(
  FILES unice68.h DESTINATION include
  )

install(
  FILES ${PROJECT_BINARY_DIR}/unice68.pc DESTINATION lib/pkgconfig
  )
ENDIF(0) # rom-properties

# ======================================================================
# Target binary: unice68

if (BUILD_UNICE68_CLI)
  
  CHECK_INCLUDE_FILE (libgen.h HAVE_LIBGEN_H)
  CHECK_INCLUDE_FILE (unistd.h HAVE_UNISTD_H)
  CHECK_INCLUDE_FILE (fcntl.h HAVE_FCNTL_H)
  CHECK_INCLUDE_FILE (io.h HAVE_IO_H)

  check_function_exists (basename HAVE_BASENAME)
  check_function_exists (setmode HAVE_SETMODE)
  check_function_exists (_setmode HAVE__SETMODE)
  check_function_exists (fileno HAVE_FILENO)
  check_function_exists (_fileno HAVE__FILENO)

  # rom-properties
  if (MSVC)
    set(CMAKE_REQUIRED_FLAGS "${OLD_CMAKE_REQUIRED_FLAGS}")
    unset(OLD_CMAKE_REQUIRED_FLAGS)
  endif (MSVC)

  add_executable(unice68_exe unice68.c)
  target_link_libraries(unice68_exe unice68_lib)
  set_target_properties(
    unice68_exe PROPERTIES
    OUTPUT_NAME unice68
    C_STANDARD 99
    COMPILE_DEFINITIONS "HAVE_CONFIG_H")
  # rom-properties: Disable installation.
  IF(0)
  install(
    TARGETS unice68_exe
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
  ENDIF(0) # rom-properties
  
endif (BUILD_UNICE68_CLI)

# ======================================================================
# rom-properties: Write config files *after* checking includes and functions.

configure_file (
  "${PROJECT_SOURCE_DIR}/cm_config.h.in"
  "${PROJECT_BINARY_DIR}/config.unice68.h"
  @ONLY)

configure_file (
  "${PROJECT_SOURCE_DIR}/unice68.pc.in"
  "${PROJECT_BINARY_DIR}/unice68.pc"
  @ONLY)
