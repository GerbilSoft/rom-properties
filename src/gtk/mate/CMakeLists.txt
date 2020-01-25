PROJECT(rom-properties-mate)

# Mate (Caja) frontend for rom-properties.
FIND_PACKAGE(GLib2 ${REQUIRE_MATE} 2.26.0)
FIND_PACKAGE(GObject2 ${REQUIRE_MATE} 2.26.0)
FIND_PACKAGE(GIO ${REQUIRE_MATE} 2.26.0)
FIND_PACKAGE(Cairo ${REQUIRE_MATE})
# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_MATE})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# Find the Caja extension library.
	# GTK+ 3.x port was in MATE 1.18, so we're using
	# that as a mimum version.
	FIND_PACKAGE(LibCajaExtension ${REQUIRE_MATE} 1.18.0)
	IF(NOT LibCajaExtension_FOUND)
		# Caja extension library was not found.
		# Disable the MATE frontend.
		SET(BUILD_MATE OFF CACHE INTERNAL "Build the MATE (GTK+ 3.x) plugin." FORCE)
	ENDIF(NOT LibCajaExtension_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# GTK+ 3.x (or required dependencies) were not found.
	# Disable the MATE frontend.
	SET(BUILD_MATE OFF CACHE INTERNAL "Build the MATE (GTK+ 3.x) plugin." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-mate_SRCS  "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-mate_SRCS2 "${rom-properties-gtk3_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-mate_H     "${rom-properties-gtk_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-mate_H2    "${rom-properties-gtk3_H}")

# CPU-specific and optimized sources.
IF(rom-properties-gtk3_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-mate_IFUNC_SRCS "${rom-properties-gtk3_IFUNC_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-mate_SSSE3_SRCS "${rom-properties-gtk3_SSSE3_SRCS}")

	# Disable LTO on the IFUNC files if LTO is known to be broken.
	IF(GCC_5xx_LTO_ISSUES)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-mate_IFUNC_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
	ENDIF(GCC_5xx_LTO_ISSUES)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-mate_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Sources and headers. (MATE-specific)
SET(rom-properties-mate_SRCS
	${rom-properties-mate_SRCS} ${rom-properties-mate_SRCS2}
	${rom-properties-mate_IFUNC_SRCS}
	${rom-properties-mate_SSSE3_SRCS}
	../gnome/rom-properties-plugin.c
	../gnome/rom-properties-provider.cpp
	)
SET(rom-properties-mate_H
	${rom-properties-mate_H} ${rom-properties-mate_H2}
	../gnome/rom-properties-provider.hpp
	)

IF(BUILD_MATE)
	ADD_LIBRARY(rom-properties-mate MODULE
		${rom-properties-mate_SRCS}
		)
	SET_TARGET_PROPERTIES(rom-properties-mate PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-mate)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-mate
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
	TARGET_LINK_LIBRARIES(rom-properties-mate PRIVATE glibresources)
	TARGET_LINK_LIBRARIES(rom-properties-mate PRIVATE romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-mate PRIVATE i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-mate PUBLIC Mate::libcaja-extension)
	TARGET_LINK_LIBRARIES(rom-properties-mate PRIVATE Cairo::cairo)
	TARGET_LINK_LIBRARIES(rom-properties-mate PUBLIC Gtk3::gtk3 GLib2::gio GLib2::gobject GLib2::glib)
	TARGET_COMPILE_DEFINITIONS(rom-properties-mate
		PRIVATE G_LOG_DOMAIN=\"rom-properties-mate\"
			RP_UI_GTK3_MATE
		)
ENDIF(BUILD_MATE)

#######################
# Install the plugin. #
#######################

IF(BUILD_MATE)
	INSTALL(TARGETS rom-properties-mate
		LIBRARY DESTINATION "${LibCajaExtension_EXTENSION_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-mate PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${LibCajaExtension_EXTENSION_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_MATE)
