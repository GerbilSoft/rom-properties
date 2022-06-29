# GTK+ 3.x UI frontend for:
# - Nautilus 3.x (GNOME)
# - Caja 1.18 (MATE)
# - Nemo (Cinnamon)
# - Thunar 1.8 (XFCE)
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-gtk3 LANGUAGES C CXX)

FIND_PACKAGE(GLib2 ${REQUIRE_GTK3} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GObject2 ${REQUIRE_GTK3} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO ${REQUIRE_GTK3} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO-UNIX ${REQUIRE_GTK3} ${GLIB_MIN_VERSION})
FIND_PACKAGE(Cairo ${REQUIRE_GTK3})
# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_GTK3})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# We're not going to have an explicit dependency on libnautilus-extension.
	# It will be dlopen()'d at runtime based on which
	# initialization function is called.

	# Nautilus (GNOME)
	FIND_PACKAGE(LibNautilusExtension 3.0.0)
	IF(NOT LibNautilusExtension_FOUND)
		MESSAGE(STATUS "LibNautilusExtension not found. Using the default extensions directory.")
		SET(LibNautilusExtension_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nautilus/extensions-3.0")
	ENDIF(NOT LibNautilusExtension_FOUND)

	# Caja (MATE)
	FIND_PACKAGE(LibCajaExtension 1.18.0)
	IF(NOT LibCajaExtension_FOUND)
		MESSAGE(STATUS "LibCajaExtension not found. Using the default extensions directory.")
		SET(LibCajaExtension_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/caja/extensions-2.0")
	ENDIF(NOT LibCajaExtension_FOUND)

	# Nemo (Cinnamon)
	FIND_PACKAGE(LibNemoExtension)
	IF(NOT LibNemoExtension_FOUND)
		MESSAGE(STATUS "LibNemoExtension not found. Using the default extensions directory.")
		SET(LibNemoExtension_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nemo/extensions-3.0")
	ENDIF(NOT LibNemoExtension_FOUND)

	# ThunarX3 (XFCE)
	FIND_PACKAGE(ThunarX3)
	IF(NOT ThunarX3_FOUND)
		MESSAGE(STATUS "ThunarX3 not found. Using the default extensions directory.")
		SET(ThunarX3_EXTENSIONS_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/thunarx-3")
	ENDIF(NOT ThunarX3_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# GTK+ 3.x (or required dependencies) were not found.
	# Disable the GNOME frontend.
	SET(BUILD_GTK3 OFF CACHE INTERNAL "Build the GTK+ 3.x plugin." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND Cairo_FOUND AND GTK3_FOUND)

# Ensure we don't use functions not available in GLib 2.34.
ADD_DEFINITIONS(-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_34 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_34)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_CSRCS "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_CH    "${rom-properties-gtk_H}")

# CairoImageConv (GTK+ 3.x)
SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} CairoImageConv.cpp)
SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    CairoImageConv.hpp)

IF(ENABLE_ACHIEVEMENTS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-gtk-notify_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-gtk-notify_H}")

	# D-Bus bindings for notifications.
	# NOTE: This has to be in this file, not the GTK+ CMakeLists.txt.
	SET(DBUS_XML_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.Notifications.xml")
	ADD_CUSTOM_COMMAND(
		OUTPUT Notifications.c Notifications.h
		COMMAND "${GDBUS_CODEGEN}"
			--generate-c-code Notifications
			"${DBUS_XML_FILENAME}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
		DEPENDS "${DBUS_XML_FILENAME}"
		VERBATIM
		)

	SET(${PROJECT_NAME}-notify_SRCS
		${${PROJECT_NAME}-notify_SRCS}
		${CMAKE_CURRENT_BINARY_DIR}/Notifications.c
		)

	# Disable unused parameter warnings in generated D-Bus sources.
	INCLUDE(CheckCCompilerFlag)
	CHECK_C_COMPILER_FLAG("-Wno-unused-parameter" CFLAG_Wno_unused_parameter)
	IF(CFLAG_Wno_unused_parameter)
		SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/Notifications.c
			APPEND_STRING PROPERTIES COMPILE_FLAGS " -Wno-unused-parameter ")
	ENDIF(CFLAG_Wno_unused_parameter)
ENDIF(ENABLE_ACHIEVEMENTS)

# CPU-specific and optimized sources.
IF(${PROJECT_NAME}_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_SSSE3_SRCS "${${PROJECT_NAME}_SSSE3_SRCS}")

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Sources and headers. (GTK3-specific)
SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_CSRCS} ${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}-notify_SRCS} ${${PROJECT_NAME}-notify_H}
	${${PROJECT_NAME}_IFUNC_SRCS}
	${${PROJECT_NAME}_SSSE3_SRCS}
	RpNautilusPlugin.cpp
	RpNautilusProvider.cpp
	RpThunarPlugin.cpp
	RpThunarProvider.cpp
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_CH} ${${PROJECT_NAME}_H}
	RpNautilusPlugin.hpp
	RpNautilusProvider.hpp
	RpThunarPlugin.hpp
	RpThunarProvider.hpp
	plugin-helper.h
	thunarx-mini.h
	)

IF(HAVE_GSOUND OR (HAVE_LIBCANBERRA AND HAVE_LIBCANBERRA_GTK3))
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../MessageSound.cpp)
	SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    ../MessageSound.hpp)
ENDIF(HAVE_GSOUND OR (HAVE_LIBCANBERRA AND HAVE_LIBCANBERRA_GTK3))

IF(BUILD_GTK3)
	ADD_LIBRARY(${PROJECT_NAME} MODULE ${${PROJECT_NAME}_SRCS})
	IF(ENABLE_PCH)
		TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME} PRIVATE
			"${RP_CONFIG_LIBC_H}" "${RP_CXX11_COMPAT_H}"
			../stdafx.h
			)
	ENDIF(ENABLE_PCH)
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(${PROJECT_NAME})
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# gtk3
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# gtk3
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# gtk
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# gtk
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>		# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>		# src
		)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE glibresources)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE Cairo::cairo)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Gtk3::gtk3 GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	IF(HAVE_GSOUND)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE GSound::gsound)
		TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE ENABLE_MESSAGESOUND)
	ELSEIF(HAVE_LIBCANBERRA AND HAVE_LIBCANBERRA_GTK3)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE LibCanberra::gtk3 LibCanberra::libcanberra)
		TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE ENABLE_MESSAGESOUND)
	ENDIF()

	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE RP_UI_GTK3)
ENDIF(BUILD_GTK3)

#######################
# Install the plugin. #
#######################

IF(BUILD_GTK3)
	# GNOME (Nautilus) extension is installed directly.
	INSTALL(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${LibNautilusExtension_EXTENSION_DIR}"
		COMPONENT "plugin"
		)

	# Symlinks are created for Caja, Nemo, and Thunar.
	# Reference: https://stackoverflow.com/questions/34578362/how-can-i-package-a-symlink-with-cpack
	ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ln -sf "${LibNautilusExtension_EXTENSION_DIR}/${PROJECT_NAME}.so" ${PROJECT_NAME}.symlink.so
		WORKING_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
		VERBATIM
		)

	# Install symlinks.
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.so"
		DESTINATION "${LibCajaExtension_EXTENSION_DIR}"
		RENAME "${PROJECT_NAME}.so"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.so"
		DESTINATION "${LibNemoExtension_EXTENSION_DIR}"
		RENAME "${PROJECT_NAME}.so"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.so"
		DESTINATION "${ThunarX3_EXTENSIONS_DIR}"
		RENAME "${PROJECT_NAME}.so"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${LibNautilusExtension_EXTENSION_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_GTK3)
