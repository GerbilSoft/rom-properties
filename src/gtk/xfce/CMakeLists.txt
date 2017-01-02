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

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_SRCS "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_H "${rom-properties-gtk_H}")

# Sources and headers. (XFCE-specific)
SET(rom-properties-xfce_SRCS
	${rom-properties-xfce_SRCS}
	rom-properties-plugin.c
	rom-properties-provider.cpp
	rom-properties-page.cpp
	)
SET(rom-properties-xfce_H
	${rom-properties-xfce_H}
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
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
	TARGET_LINK_LIBRARIES(rom-properties-xfce romdata8 cachemgr8)

	TARGET_INCLUDE_DIRECTORIES(rom-properties-xfce PUBLIC ${GTK2_INCLUDE_DIRS})
	TARGET_LINK_LIBRARIES(rom-properties-xfce Xfce::thunarx-2 ${GTK2_LIBRARIES})
	ADD_DEFINITIONS(${GTK2_DEFINITIONS} -DG_LOG_DOMAIN=\"rom-properties-xfce\")
ENDIF(BUILD_XFCE)

#######################
# Install the plugin. #
#######################

IF(BUILD_XFCE)
	# FIXME: ${THUNARX2_EXTENSIONS_DIR} always uses the system prefix.
	# This should be adjusted to ${CMAKE_INSTALL_PREFIX}.
	INSTALL(TARGETS rom-properties-xfce
		LIBRARY DESTINATION "${THUNARX2_EXTENSIONS_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-xfce PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${THUNARX2_EXTENSIONS_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_XFCE)
