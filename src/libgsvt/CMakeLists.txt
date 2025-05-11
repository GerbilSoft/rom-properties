# Virtual Terminal emulation for older Windows versions.
PROJECT(gsvt LANGUAGES C CXX)

######################
# Build the library. #
######################

IF(WIN32)
	SET(${PROJECT_NAME}_OS_SRCS gsvt_win32.c)
ELSE(WIN32)
	SET(${PROJECT_NAME}_OS_SRCS gsvt_posix.c)
ENDIF(WIN32)

SET(${PROJECT_NAME}_SRCS
	gsvtpp.cpp
	)
SET(${PROJECT_NAME}_H
	gsvt.h
	gsvtpp.hpp
	)

ADD_LIBRARY(${PROJECT_NAME} STATIC
	${${PROJECT_NAME}_OS_SRCS}
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}_H}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(${PROJECT_NAME})
# Exclude from ALL builds; enable PIC.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	POSITION_INDEPENDENT_CODE TRUE
	)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libgsvt
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libgsvt
	PRVIATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# ..
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# ..
	)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpthreads)