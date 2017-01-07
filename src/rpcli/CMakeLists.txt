PROJECT(rom-properties-rpcli)

# Sources and headers.
SET(rom-properties-rpcli_SRCS
	rpcli.cpp
	properties.cpp
	bmp.cpp
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
TARGET_INCLUDE_DIRECTORIES(rpcli
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
TARGET_LINK_LIBRARIES(rpcli romdata8)
