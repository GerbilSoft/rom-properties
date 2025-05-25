# Note: CMake support is community-based. The maintainers do not use CMake
# internally.

cmake_minimum_required(VERSION 3.5...3.10)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif (POLICY CMP0077)

project(googletest-distribution)
set(GOOGLETEST_VERSION 1.12.1)

if(NOT CYGWIN AND NOT MSYS AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL QNX)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

enable_testing()

include(CMakeDependentOption)
include(GNUInstallDirs)

# rom-properties: Enable policy CMP0063 for visibility properties.
IF(POLICY CMP0063)
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

# rom-properties: Disable options.
IF(0)
#Note that googlemock target already builds googletest
option(BUILD_GMOCK "Builds the googlemock subproject" ON)
option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" ON)
ELSE(0) # rom-properties
SET(BUILD_GMOCK ON)
SET(INSTALL_GTEST OFF)
ENDIF(0) # rom-properties

# rom-properties: Set CMake debug/release postfix on MSVC.
IF(MSVC)
  SET(CMAKE_DEBUG_POSTFIX "d")
  SET(CMAKE_RELEASE_POSTFIX "")
ELSE(MSVC)
  SET(CMAKE_DEBUG_POSTFIX "")
  SET(CMAKE_RELEASE_POSTFIX "")
ENDIF(MSVC)

if(BUILD_GMOCK)
  add_subdirectory( googlemock )
else()
  add_subdirectory( googletest )
endif()
