#cmake_minimum_required(VERSION 2.6.2)

project( googletest-distribution )

enable_testing()

option(BUILD_GTEST "Builds the googletest subproject" OFF)

#Note that googlemock target already builds googletest
option(BUILD_GMOCK "Builds the googlemock subproject" ON)

# rom-properties: Enable policy CMP0063 for visibility properties.
IF(NOT CMAKE_VERSION VERSION_LESS 3.3.0)
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(NOT CMAKE_VERSION VERSION_LESS 3.3.0)

if(BUILD_GMOCK)
  add_subdirectory( googlemock )
elseif(BUILD_GTEST)
  add_subdirectory( googletest )
endif()
