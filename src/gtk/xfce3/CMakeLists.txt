PROJECT(rom-properties-xfce3)

# XFCE (Thunar) (GTK+ 3.x) frontend for rom-properties.

# Find packages.
FIND_PACKAGE(GLib2 ${REQUIRE_XFCE3} 2.26.0)
FIND_PACKAGE(GObject2 ${REQUIRE_XFCE3} 2.26.0)
FIND_PACKAGE(Cairo)
# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_XFCE3})
IF(GLib2_FOUND AND GObject2_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# Find the ThunarX 3 library.
	FIND_PACKAGE(ThunarX3 ${REQUIRE_XFCE3})
	IF(NOT ThunarX3_FOUND)
		# ThunarX 3 was not found.
		# Disable the XFCE frontend.
		SET(BUILD_XFCE3 OFF CACHE INTERNAL "Build the XFCE (GTK+ 3.x) plugin. (Thunar 1.8 and later)" FORCE)
	ENDIF(NOT ThunarX3_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# GTK+ 3.x (or required dependencies) were not found.
	# Disable the XFCE frontend.
	SET(BUILD_XFCE3 OFF CACHE INTERNAL "Build the XFCE (GTK+ 3.x) plugin. (Thunar 1.8 and later)" FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND Cairo_FOUND AND GTK3_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce3_SRCS  "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce3_SRCS2 "${rom-properties-gtk3_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce3_H     "${rom-properties-gtk_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce3_H2    "${rom-properties-gtk3_H}")

# CPU-specific and optimized sources.
IF(rom-properties-gtk3_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce3_IFUNC_SRCS "${rom-properties-gtk3_IFUNC_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce3_SSSE3_SRCS "${rom-properties-gtk3_SSSE3_SRCS}")

	# Disable LTO on the IFUNC files if LTO is known to be broken.
	IF(GCC_5xx_LTO_ISSUES)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-xfce3_IFUNC_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
	ENDIF(GCC_5xx_LTO_ISSUES)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-xfce3_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Sources and headers. (XFCE-specific)
SET(rom-properties-xfce3_SRCS
	${rom-properties-xfce3_SRCS} ${rom-properties-xfce3_SRCS2}
	${rom-properties-xfce3_IFUNC_SRCS}
	${rom-properties-xfce3_SSSE3_SRCS}
	../xfce/rom-properties-plugin.c
	../xfce/rom-properties-provider.cpp
	../xfce/rom-properties-page.cpp
	)
SET(rom-properties-xfce3_H
	${rom-properties-xfce3_H} ${rom-properties-xfce3_H2}
	../xfce/rom-properties-provider.hpp
	../xfce/rom-properties-page.hpp
	)

IF(BUILD_XFCE3)
	ADD_LIBRARY(rom-properties-xfce3 MODULE
		${rom-properties-xfce3_SRCS}
		)
	SET_TARGET_PROPERTIES(rom-properties-xfce3 PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-xfce3)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-xfce3
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-xfce3 PUBLIC ${GTK3_INCLUDE_DIRS})

	TARGET_LINK_LIBRARIES(rom-properties-xfce3 PRIVATE cachemgr romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-xfce3 PRIVATE i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-xfce3 PUBLIC Xfce::thunarx-3)
	TARGET_LINK_LIBRARIES(rom-properties-xfce3 PRIVATE Cairo::cairo)
	TARGET_LINK_LIBRARIES(rom-properties-xfce3 PUBLIC Gtk3::gtk3 GLib2::gobject GLib2::glib)
	TARGET_COMPILE_DEFINITIONS(rom-properties-xfce3
		PRIVATE G_LOG_DOMAIN=\"rom-properties-xfce3\"
		)
ENDIF(BUILD_XFCE3)

#######################
# Install the plugin. #
#######################

IF(BUILD_XFCE3)
	# FIXME: ${ThunarX3_EXTENSIONS_DIR} always uses the system prefix.
	# This should be adjusted to ${CMAKE_INSTALL_PREFIX}.
	INSTALL(TARGETS rom-properties-xfce3
		LIBRARY DESTINATION "${ThunarX3_EXTENSIONS_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-xfce3 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${ThunarX3_EXTENSIONS_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_XFCE3)
