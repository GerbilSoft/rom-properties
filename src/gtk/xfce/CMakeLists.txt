PROJECT(rom-properties-xfce)

# XFCE (Thunar) frontend for rom-properties.

# Disable deprecated GTK+ 2.x functionality that isn't available in GTK+ 3.x.
# Reference: https://developer.gnome.org/gtk3/stable/gtk-migrating-2-to-3.html
ADD_DEFINITIONS(-DGTK_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE)

# Find packages.
FIND_PACKAGE(GLib2 2.26.0)
FIND_PACKAGE(GObject2 2.26.0)
FIND_PACKAGE(GIO 2.26.0)
FIND_PACKAGE(GIO-UNIX 2.26.0)
FIND_PACKAGE(GdkPixbuf2)
# GTK2 minimum is 2.0.0, so no version check is needed.
FIND_PACKAGE(GTK2 COMPONENTS gtk)
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)
	# Find the ThunarX 2 library.
	FIND_PACKAGE(ThunarX2)
	IF(NOT ThunarX2_FOUND)
		# ThunarX 2 was not found.
		# Disable the XFCE frontend.
		SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
	ENDIF(NOT ThunarX2_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)
	# GTK+ 2.x (or required dependencies) were not found.
	# Disable the XFCE frontend.
	SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_SRCS "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_H "${rom-properties-gtk_H}")

# CPU-specific and optimized sources.
IF(rom-properties-gtk_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_IFUNC_SRCS "${rom-properties-gtk_IFUNC_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_SSSE3_SRCS "${rom-properties-gtk_SSSE3_SRCS}")

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		FOREACH(ssse3_file ${rom-properties-xfce_SSSE3_SRCS})
			SET_SOURCE_FILES_PROPERTIES(${ssse3_file}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
		ENDFOREACH(ssse3_file ${rom-properties-xfce_SSSE3_SRCS})
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Sources and headers. (XFCE-specific)
SET(rom-properties-xfce_SRCS
	${rom-properties-xfce_SRCS}
	${rom-properties-xfce_IFUNC_SRCS}
	${rom-properties-xfce_SSSE3_SRCS}
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
	OUTPUT SpecializedThumbnailer1.c SpecializedThumbnailer1.h
	COMMAND "${GDBUS_CODEGEN}"
		--generate-c-code SpecializedThumbnailer1
		"${CMAKE_CURRENT_SOURCE_DIR}/org.freedesktop.thumbnails.SpecializedThumbnailer1.xml"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	DEPENDS org.freedesktop.thumbnails.SpecializedThumbnailer1.xml
	VERBATIM
	)

SET(rp-thumbnailer-dbus_SRCS
	rp-thumbnailer-dbus.cpp
	rp-thumbnailer-main.cpp
	${CMAKE_CURRENT_BINARY_DIR}/SpecializedThumbnailer1.c
	)
SET(rp-thumbnailer-dbus_H
	rp-thumbnailer-dbus.hpp
	${CMAKE_CURRENT_BINARY_DIR}/SpecializedThumbnailer1.h
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

	TARGET_LINK_LIBRARIES(rom-properties-xfce cachemgr romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-xfce i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-xfce Xfce::thunarx-2)
	TARGET_LINK_LIBRARIES(rom-properties-xfce ${GTK2_LIBRARIES} GdkPixbuf2::gdkpixbuf2 GLib2::gobject GLib2::glib)
	# TODO: Move GTK2_DEFINITIONS to TARGET_COMPILE_DEFINITIONS.
	# (Requires removing the "-D" switches.)
	ADD_DEFINITIONS(${GTK2_DEFINITIONS})
	TARGET_COMPILE_DEFINITIONS(rom-properties-xfce
		PRIVATE G_LOG_DOMAIN=\"rom-properties-xfce\"
		)

	# D-Bus thumbnailer.
	ADD_EXECUTABLE(rp-thumbnailer-dbus
		${rp-thumbnailer-dbus_SRCS}
		${rp-thumbnailer-dbus_H}
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
	TARGET_LINK_LIBRARIES(rp-thumbnailer-dbus unixcommon)
	TARGET_LINK_LIBRARIES(rp-thumbnailer-dbus GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(rp-thumbnailer-dbus ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)
	TARGET_COMPILE_DEFINITIONS(rp-thumbnailer-dbus
		PRIVATE G_LOG_DOMAIN=\"rp-thumbnailer-dbus\"
		)
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
