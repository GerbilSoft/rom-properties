PROJECT(rom-properties-gnome)

# GNOME (Nautilus) frontend for rom-properties.
FIND_PACKAGE(GLib2 ${REQUIRE_GNOME} 2.26.0)
FIND_PACKAGE(GObject2 ${REQUIRE_GNOME} 2.26.0)
FIND_PACKAGE(GIO ${REQUIRE_GNOME} 2.26.0)
FIND_PACKAGE(Cairo ${REQUIRE_GNOME})
# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_GNOME})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# Find the Nautilus extension library.
	# TODO: Minimum version, and ensure it's 3, not 4?
	FIND_PACKAGE(LibNautilusExtension ${REQUIRE_GNOME} 3.0.0)
	IF(LibNautilusExtension_FOUND)
		# Check if we have nautilus-extension.h. (added in nautilus-3.30)
		SET(OLD_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
		LIST(APPEND CMAKE_REQUIRED_INCLUDES ${LibNautilusExtension_INCLUDE_DIRS})
		CHECK_INCLUDE_FILES("nautilus-extension.h" HAVE_NAUTILUS_EXTENSION_H)
		SET(CMAKE_REQUIRED_INCLUDES ${OLD_CMAKE_REQUIRED_INCLUDES})
		UNSET(OLD_CMAKE_REQUIRED_INCLUDES)
	ELSE(LibNautilusExtension_FOUND)
		# Nautilus extension library was not found.
		# Disable the GNOME frontend.
		SET(BUILD_GNOME OFF CACHE INTERNAL "Build the GNOME (GTK+ 3.x) plugin." FORCE)
	ENDIF(LibNautilusExtension_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# GTK+ 3.x (or required dependencies) were not found.
	# Disable the GNOME frontend.
	SET(BUILD_GNOME OFF CACHE INTERNAL "Build the GNOME (GTK+ 3.x) plugin." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_SRCS  "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_SRCS2 "${rom-properties-gtk3_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_H     "${rom-properties-gtk_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_H2    "${rom-properties-gtk3_H}")

# CPU-specific and optimized sources.
IF(rom-properties-gtk3_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_IFUNC_SRCS "${rom-properties-gtk3_IFUNC_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_SSSE3_SRCS "${rom-properties-gtk3_SSSE3_SRCS}")

	# Disable LTO on the IFUNC files if LTO is known to be broken.
	IF(GCC_5xx_LTO_ISSUES)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-gnome_IFUNC_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
	ENDIF(GCC_5xx_LTO_ISSUES)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-gnome_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.gnome.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.gnome.h")

# Sources and headers. (GNOME-specific)
SET(rom-properties-gnome_SRCS
	${rom-properties-gnome_SRCS} ${rom-properties-gnome_SRCS2}
	${rom-properties-gnome_IFUNC_SRCS}
	${rom-properties-gnome_SSSE3_SRCS}
	rom-properties-plugin.c
	rom-properties-provider.cpp
	)
SET(rom-properties-gnome_H
	${rom-properties-gnome_H} ${rom-properties-gnome_H2}
	rom-properties-provider.hpp
	)

IF(BUILD_GNOME)
	ADD_LIBRARY(rom-properties-gnome MODULE
		${rom-properties-gnome_SRCS}
		)
	SET_TARGET_PROPERTIES(rom-properties-gnome PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-gnome)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-gnome
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
	TARGET_LINK_LIBRARIES(rom-properties-gnome PRIVATE cachemgr romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-gnome PRIVATE i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-gnome PUBLIC Gnome::libnautilus-extension)
	TARGET_LINK_LIBRARIES(rom-properties-gnome PRIVATE Cairo::cairo)
	TARGET_LINK_LIBRARIES(rom-properties-gnome PUBLIC Gtk3::gtk3 GLib2::gio GLib2::gobject GLib2::glib)
	TARGET_COMPILE_DEFINITIONS(rom-properties-gnome
		PRIVATE G_LOG_DOMAIN=\"rom-properties-gnome\"
			RP_UI_GTK3_GNOME
		)
ENDIF(BUILD_GNOME)

#######################
# Install the plugin. #
#######################

IF(BUILD_GNOME)
	INSTALL(TARGETS rom-properties-gnome
		LIBRARY DESTINATION "${LibNautilusExtension_EXTENSION_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-gnome PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${LibNautilusExtension_EXTENSION_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_GNOME)
