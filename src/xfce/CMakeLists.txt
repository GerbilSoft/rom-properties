PROJECT(rom-properties-xfce)

# XFCE frontend for rom-properties.

# TODO: Minimum GTK+ 2.x version?
FIND_PACKAGE(GTK2 COMPONENTS gtk)
IF(GTK2_FOUND)
	# Find the ThunarX 2 library.
	FIND_PACKAGE(ThunarX2)
	IF(THUNARX2_FOUND)
	ELSE(THUNARX2_FOUND)
		# ThunarX 2 not found. Disable the XFCE frontend.
		SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
	ENDIF(THUNARX2_FOUND)
ELSE(GTK2_FOUND)
	# GTK+ 2.x not found. Disable the XFCE frontend.
	SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
ENDIF(GTK2_FOUND)

# Sources and headers.
SET(rom-properties-xfce_SRCS
	rom-properties-plugin.c
	rom-properties-provider.cpp
	rom-properties-page.cpp
	)
SET(rom-properties-xfce_H
	rom-properties-provider.hpp
	rom-properties-page.hpp
	)

IF(BUILD_XFCE)
	ADD_LIBRARY(rom-properties-xfce MODULE
		${rom-properties-xfce_SRCS}
		)
	SET_TARGET_PROPERTIES(rom-properties-xfce PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-xfce)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-xfce
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>
		)
	TARGET_LINK_LIBRARIES(rom-properties-xfce romdata8 cachemgr8)

	TARGET_INCLUDE_DIRECTORIES(rom-properties-xfce PUBLIC ${GTK2_INCLUDE_DIRS})
	TARGET_LINK_LIBRARIES(rom-properties-xfce Xfce::thunarx-2 ${GTK2_LIBRARIES})
	ADD_DEFINITIONS(${GTK2_DEFINITIONS} -DG_LOG_DOMAIN=\"rom-properties-xfce\")
ENDIF(BUILD_XFCE)
