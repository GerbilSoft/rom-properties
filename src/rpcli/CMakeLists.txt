PROJECT(rom-properties-rpcli)

# Sources and headers.
SET(rom-properties-rpcli_SRCS
	rpcli.cpp
	properties.cpp
	bmp.cpp
	resource.rc
	)
SET(rom-properties-rpcli_H
	properties.hpp
	bmp.hpp
	)

#####################
# Build the plugin. #
#####################

ADD_EXECUTABLE(rpcli
		${rom-properties-rpcli_SRCS}
		${rom-properties-rpcli_H}
		)
SET_TARGET_PROPERTIES(rpcli PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rpcli)
SET_WINDOWS_SUBSYSTEM(rpcli CONSOLE)
SET_WINDOWS_NO_MANIFEST(rpcli)
# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rpcli
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
TARGET_LINK_LIBRARIES(rpcli romdata8)
