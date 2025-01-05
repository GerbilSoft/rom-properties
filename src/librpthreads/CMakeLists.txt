# Threading wrapper library
PROJECT(rpthreads LANGUAGES C CXX)

# Find the system threading library.
# NOTE: If using Windows, always assume we're using Windows threads.
# CMake attempts to find pthreads, which causes conflicts if building
# with MSYS2/MinGW-w64.
IF(WIN32)
	SET(CMAKE_USE_WIN32_THREADS_INIT 1)
	SET(Threads_FOUND TRUE)
ELSE(WIN32)
	FIND_PACKAGE(Threads REQUIRED)
ENDIF(WIN32)

# Threading implementation.
SET(${PROJECT_NAME}_SRCS dummy.cpp)
SET(${PROJECT_NAME}_H
	Atomics.h
	Semaphore.hpp
	Mutex.hpp
	pthread_once.h
	)
IF(CMAKE_USE_WIN32_THREADS_INIT)
	SET(HAVE_WIN32_THREADS 1)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} pthread_once.c)
ELSEIF(CMAKE_USE_PTHREADS_INIT)
	SET(HAVE_PTHREADS 1)
ELSE()
	MESSAGE(FATAL_ERROR "No threading model is supported on this system.")
ENDIF()

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

######################
# Build the library. #
######################

# NOTE: ${PROJECT_NAME} is the original target.
# ${PROJECT_NAME}-dll is the target with dllimport/dllexport for libromdata.
IF(RP_LIBROMDATA_IS_DLL AND WIN32)
	# On Win32, the dll target needs dllimport/dllexport.
	SET(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-dll)
ELSE()
	# On other platforms, the dll target will be an alias to the regular target.
	# That will be set up later, since the regular target doesn't exist yet.
	SET(TARGETS ${PROJECT_NAME})
ENDIF()

FOREACH(_target ${TARGETS})
	# FIXME: May need a dummy file on Linux?
	ADD_LIBRARY(${_target} STATIC
		${${PROJECT_NAME}_SRCS}
		${${PROJECT_NAME}_H}
		)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds; enable PIC.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES
		EXCLUDE_FROM_ALL TRUE
		POSITION_INDEPENDENT_CODE TRUE
		)

	# DLL targets need the appropriate macros.
	IF(${_target} MATCHES "-dll$")
		TARGET_COMPILE_DEFINITIONS(${_target}
			PUBLIC		RP_BUILDING_FOR_DLL=1
			PRIVATE		RP_BUILDING_LIBROMDATA=1
			)
	ENDIF()

	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		)

	IF(CMAKE_THREAD_LIBS_INIT)
		TARGET_LINK_LIBRARIES(${_target} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
	ENDIF(CMAKE_THREAD_LIBS_INIT)
ENDFOREACH(_target)

IF(NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()
