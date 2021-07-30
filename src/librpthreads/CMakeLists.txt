# Threading wrapper library
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
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

# FIXME: May need a dummy file on Linux?
ADD_LIBRARY(${PROJECT_NAME} STATIC
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}_H}
	)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	)

IF(CMAKE_THREAD_LIBS_INIT)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
ENDIF(CMAKE_THREAD_LIBS_INIT)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
