PROJECT(rom-properties-xfce)

# XFCE (Thunar) frontend for rom-properties.
FIND_PACKAGE(GLib2 2.26.0)
FIND_PACKAGE(GObject2 2.26.0)
FIND_PACKAGE(GdkPixbuf2)
FIND_PACKAGE(DBusGLib)
# GTK2 minimum is 2.0.0, so no version check is needed.
FIND_PACKAGE(GTK2 COMPONENTS gtk)
IF(GLib2_FOUND AND GObject2_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)
	# Find the ThunarX 2 library.
	FIND_PACKAGE(ThunarX2)
	IF(NOT ThunarX2_FOUND)
		# ThunarX 2 was not found.
		# Disable the XFCE frontend.
		SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
	ENDIF(NOT ThunarX2_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)
	# GTK+ 2.x (or required dependencies) were not found.
	# Disable the XFCE frontend.
	SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)

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

# D-Bus bindings for the thumbnailer.
ADD_CUSTOM_COMMAND(
	OUTPUT rp-thumbnail-server-bindings.h
	COMMAND "${GLIB_DBUS_BINDING_TOOL}" --prefix=rp_thumbnail --mode=glib-server
		"${CMAKE_CURRENT_SOURCE_DIR}/org.freedesktop.thumbnails.SpecializedThumbnailer1.xml"
		> rp-thumbnail-server-bindings.h
	DEPENDS org.freedesktop.thumbnails.SpecializedThumbnailer1.xml
	VERBATIM
	)

SET(rp-thumbnail-dbus_SRCS
	rp-thumbnail-dbus.cpp
	../../rp-stub/dll-search.c
	)
SET(rp-thumbnail-dbus_H
	rp-thumbnail-dbus.hpp
	${CMAKE_CURRENT_BINARY_DIR}/rp-thumbnail-server-bindings.h
	../../rp-stub/dll-search.h
	)

# Process the .service file.
INCLUDE(DirInstallPaths)
CONFIGURE_FILE(com.gerbilsoft.rom-properties.SpecializedThumbnailer1.service.in
	com.gerbilsoft.rom-properties.SpecializedThumbnailer1.service)

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
	TARGET_INCLUDE_DIRECTORIES(rom-properties-xfce PUBLIC ${GTK2_INCLUDE_DIRS})

	TARGET_LINK_LIBRARIES(rom-properties-xfce cachemgr8 romdata8 rpbase8)
	TARGET_LINK_LIBRARIES(rom-properties-xfce Xfce::thunarx-2)
	TARGET_LINK_LIBRARIES(rom-properties-xfce ${GTK2_LIBRARIES} GdkPixbuf2::gdkpixbuf2 GLib2::gobject GLib2::glib)
	ADD_DEFINITIONS(${GTK2_DEFINITIONS} -DG_LOG_DOMAIN=\"rom-properties-xfce\")

	# D-Bus thumbnailer.
	ADD_EXECUTABLE(rp-thumbnailer-dbus
		${rp-thumbnail-dbus_SRCS}
		${rp-thumbnail-dbus_H}
		)
	DO_SPLIT_DEBUG(rp-thumbnailer-dbus)
	TARGET_INCLUDE_DIRECTORIES(rp-thumbnailer-dbus
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
	TARGET_LINK_LIBRARIES(rp-thumbnailer-dbus cachemgr8 romdata8 rpbase8)
	TARGET_LINK_LIBRARIES(rp-thumbnailer-dbus GdkPixbuf2::gdkpixbuf2 GLib2::dbus GLib2::gobject GLib2::glib)
	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(rp-thumbnailer-dbus ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)
ENDIF(BUILD_XFCE)

#######################
# Install the plugin. #
#######################

IF(BUILD_XFCE)
	# FIXME: ${ThunarX2_EXTENSIONS_DIR} always uses the system prefix.
	# This should be adjusted to ${CMAKE_INSTALL_PREFIX}.
	INSTALL(TARGETS rom-properties-xfce
		LIBRARY DESTINATION "${ThunarX2_EXTENSIONS_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(TARGETS rp-thumbnailer-dbus
		RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
		COMPONENT "plugin"
		)
	# TODO: Use session_bus_services_dir from dbus-1.pc.
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/com.gerbilsoft.rom-properties.SpecializedThumbnailer1.service"
		DESTINATION "share/dbus-1/services"
		COMPONENT "plugin"
		)

	# NOTE: This file must be installed with a ".service" extension.
	# NOTE 2: On Ubuntu, this must be installed in share/xfce4/thumbnailers,
	# since Ubuntu ends up listing /usr/share/ in $XDG_DATA_DIRS twice,
	# which causes tumblerd to load /usr/share/thumbnailers/ twice and
	# fail an assertion.

	# Detect Ubuntu.
	# Reference: https://stackoverflow.com/questions/26919334/detect-underlying-platform-flavour-in-cmake
	FIND_PROGRAM(LSB_RELEASE lsb_release)
	IF(LSB_RELEASE)
		EXECUTE_PROCESS(COMMAND ${LSB_RELEASE} -is
			OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			)
	ENDIF(LSB_RELEASE)
	IF(LSB_RELEASE_ID_SHORT STREQUAL Ubuntu)
		SET(THUMBNAIL_DEST_DIR "share/xfce4/thumbnailers")
	ELSE()
		SET(THUMBNAIL_DEST_DIR "share/thumbnailers")
	ENDIF()
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/com.gerbilsoft.rom-properties.SpecializedThumbnailer1.thumbnailer"
		RENAME com.gerbilsoft.rom-properties.SpecializedThumbnailer1.service
		DESTINATION "${THUMBNAIL_DEST_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-xfce PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${ThunarX2_EXTENSIONS_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME rp-thumbnailer-dbus PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${DIR_INSTALL_EXE}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_XFCE)
