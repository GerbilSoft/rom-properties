#cmake_minimum_required(VERSION 2.8.8)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(googletest-distribution)
set(GOOGLETEST_VERSION 1.9.0)

enable_testing()

include(CMakeDependentOption)
include(GNUInstallDirs)

# rom-properties: Enable policy CMP0063 for visibility properties.
IF(NOT CMAKE_VERSION VERSION_LESS 3.3.0)
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(NOT CMAKE_VERSION VERSION_LESS 3.3.0)

# rom-properties: Disable options.
IF(0)
#Note that googlemock target already builds googletest
option(BUILD_GMOCK "Builds the googlemock subproject" ON)
option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" ON)
ELSE(0) # rom-properties
SET(BUILD_GMOCK ON)
SET(INSTALL_GTEST OFF)
ENDIF(0) # rom-properties

if(BUILD_GMOCK)
  add_subdirectory( googlemock )
else()
  add_subdirectory( googletest )
endif()
