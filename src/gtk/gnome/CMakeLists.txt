PROJECT(rom-properties-gnome)

# GNOME (Nautilus) frontend for rom-properties.
FIND_PACKAGE(GLib2 2.26.0)
FIND_PACKAGE(GObject2 2.26.0)
FIND_PACKAGE(GdkPixbuf2)
# GTK3 minimum is 3.0.0, so no version check is needed.
FIND_PACKAGE(GTK3)
IF(GLib2_FOUND AND GObject2_FOUND AND GdkPixbuf2_FOUND AND GTK3_FOUND)
	# Find the Nautilus extension library.
	# TODO: Minimum version, and ensure it's 3, not 4?
	FIND_PACKAGE(LibNautilusExtension 3.0.0)
	IF(NOT LibNautilusExtension_FOUND)
		# Nautilus extension library was not found.
		# Disable the GNOME frontend.
		SET(BUILD_GNOME OFF CACHE "" INTERNAL FORCE)
	ENDIF(NOT LibNautilusExtension_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GdkPixbuf2_FOUND AND GTK3_FOUND)
	# GTK+ 3.x (or required dependencies) were not found.
	# Disable the GNOME frontend.
	SET(BUILD_GNOME OFF CACHE "" INTERNAL FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GdkPixbuf2_FOUND AND GTK3_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_SRCS "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_H "${rom-properties-gtk_H}")

# CPU-specific and optimized sources.
IF(rom-properties-gtk_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_IFUNC_SRCS "${rom-properties-gtk_IFUNC_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gnome_SSSE3_SRCS "${rom-properties-gtk_SSSE3_SRCS}")

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		FOREACH(ssse3_file ${rom-properties-gnome_SSSE3_SRCS})
			SET_SOURCE_FILES_PROPERTIES(${ssse3_file}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
		ENDFOREACH(ssse3_file ${rom-properties-gnome_SSSE3_SRCS})
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.gnome.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.gnome.h")

# Sources and headers. (GNOME-specific)
SET(rom-properties-gnome_SRCS
	${rom-properties-gnome_SRCS}
	${rom-properties-gnome_IFUNC_SRCS}
	${rom-properties-gnome_SSSE3_SRCS}
	rom-properties-plugin.c
	rom-properties-provider.cpp
	)
SET(rom-properties-gnome_H
	${rom-properties-gnome_H}
	rom-properties-provider.hpp
	)

# Process the .thumbnailer file.
INCLUDE(DirInstallPaths)
CONFIGURE_FILE(rom-properties.thumbnailer.in rom-properties.thumbnailer)

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
	TARGET_LINK_LIBRARIES(rom-properties-gnome cachemgr romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-gnome i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-gnome Gnome::libnautilus-extension)
	TARGET_LINK_LIBRARIES(rom-properties-gnome Gtk3::gtk3 GdkPixbuf2::gdkpixbuf2 GLib2::gobject GLib2::glib)
	ADD_DEFINITIONS(-DG_LOG_DOMAIN=\"rom-properties-gnome\")
ENDIF(BUILD_GNOME)

#######################
# Install the plugin. #
#######################

IF(BUILD_GNOME)
	INSTALL(TARGETS rom-properties-gnome
		LIBRARY DESTINATION "${LibNautilusExtension_EXTENSION_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/rom-properties.thumbnailer"
		DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/thumbnailers"
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
