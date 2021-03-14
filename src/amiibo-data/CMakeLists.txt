# amiibo database data and compiler.
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(amiibo-data LANGUAGES CXX)

# TODO: Run amiiboc to generate the database file and install the database file.
SET(amiiboc_SRCS amiiboc.cpp)

#########################
# Build the executable. #
#########################

INCLUDE(SetMSVCDebugPath)

ADD_EXECUTABLE(amiiboc ${amiiboc_SRCS})
DO_SPLIT_DEBUG(amiiboc)
SET_WINDOWS_SUBSYSTEM(amiiboc CONSOLE)
#SET_WINDOWS_NO_MANIFEST(amiiboc)
# TODO: _tmain()?
SET_WINDOWS_ENTRYPOINT(amiiboc main OFF)

TARGET_LINK_LIBRARIES(amiiboc PRIVATE rpcpu)
TARGET_INCLUDE_DIRECTORIES(amiiboc
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# amiibo-data
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# amiibo-data
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(amiiboc git_version)
ENDIF(TARGET git_version)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}\"")
ENDIF(MINGW)
