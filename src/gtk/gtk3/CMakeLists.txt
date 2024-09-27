# GTK+ 3.x UI frontend for:
# - Nautilus 3.x (GNOME)
# - Caja 1.18 (MATE)
# - Nemo (Cinnamon)
# - Thunar 1.8 (XFCE)
PROJECT(rom-properties-gtk3 LANGUAGES C CXX)

FIND_PACKAGE(GLib2 ${REQUIRE_GTK3} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GObject2 ${REQUIRE_GTK3} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO ${REQUIRE_GTK3} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO-UNIX ${REQUIRE_GTK3} ${GLIB_MIN_VERSION})
FIND_PACKAGE(CairoGObject ${REQUIRE_GTK3})
# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_GTK3})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND CairoGObject_FOUND AND GTK3_FOUND)
	# We're not going to have an explicit dependency on libnautilus-extension.
	# It will be dlopen()'d at runtime based on which
	# initialization function is called.

	# Nautilus (GNOME)
	FIND_PACKAGE(LibNautilusExtension)	# v3
	IF(LibNautilusExtension_FOUND)
		# Found v3.
		SET(LibNautilusExtension3_EXTENSION_DIR "${LibNautilusExtension_EXTENSION_DIR}")
	ELSE(LibNautilusExtension_FOUND)
		# Check if v4 is installed.
		FIND_PACKAGE(LibNautilusExtension4)
		IF(LibNautilusExtension4_FOUND)
			# v4: Replace the version number with 3.0.
			STRING(REPLACE "extensions-4" "extensions-3.0" LibNautilusExtension3_EXTENSION_DIR "${LibNautilusExtension4_EXTENSION_DIR}")
		ELSE(LibNautilusExtension4_FOUND)
			# Neither v3 nor v4 were found.
			MESSAGE(STATUS "LibNautilusExtension not found. Using the default extensions directory.")
			SET(LibNautilusExtension3_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nautilus/extensions-3.0")
		ENDIF(LibNautilusExtension4_FOUND)
	ENDIF(LibNautilusExtension_FOUND)

	# Caja (MATE)
	FIND_PACKAGE(LibCajaExtension 1.18.0)
	IF(LibCajaExtension_FOUND)
		SET(LibCajaExtension3_EXTENSION_DIR "${LibCajaExtension_EXTENSION_DIR}")
	ELSE(LibCajaExtension_FOUND)
		MESSAGE(STATUS "LibCajaExtension not found. Using the default extensions directory.")
		SET(LibCajaExtension3_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/caja/extensions-2.0")
	ENDIF(LibCajaExtension_FOUND)

	# Nemo (Cinnamon)
	FIND_PACKAGE(LibNemoExtension)
	IF(LibNemoExtension_FOUND)
		SET(LibNemoExtension3_EXTENSION_DIR "${LibNemoExtension_EXTENSION_DIR}")
	ELSE(LibNemoExtension_FOUND)
		MESSAGE(STATUS "LibNemoExtension not found. Using the default extensions directory.")
		SET(LibNemoExtension3_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nemo/extensions-3.0")
	ENDIF(LibNemoExtension_FOUND)

	# ThunarX3 (XFCE)
	FIND_PACKAGE(ThunarX3)
	IF(NOT ThunarX3_FOUND)
		MESSAGE(STATUS "ThunarX3 not found. Using the default extensions directory.")
		SET(ThunarX3_EXTENSIONS_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/thunarx-3")
	ENDIF(NOT ThunarX3_FOUND)
ELSE()
	# GTK+ 3.x (or required dependencies) were not found.
	# Disable the GNOME frontend.
	SET(BUILD_GTK3 OFF CACHE INTERNAL "Build the GTK+ 3.x plugin." FORCE)
ENDIF()

# Ensure we don't use functions not available in GLib 2.34.
ADD_DEFINITIONS(-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_34 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_34)

# Sources and headers (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_SRCS "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_H    "${rom-properties-gtk_H}")

# Sources and headers (GTK3 maximum)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_GTK3MAX_SRCS "${rom-properties-gtk_GTK3MAX_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_GTK3MAX_H    "${rom-properties-gtk_GTK3MAX_H}")

# CairoImageConv (GTK+ 3.x)
SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} CairoImageConv.cpp)
SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    CairoImageConv.hpp)

IF(ENABLE_ACHIEVEMENTS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-gtk-notify_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-gtk-notify_H}")
	SET(${PROJECT_NAME}-notify_SRCS ${${PROJECT_NAME}-notify_SRCS})
ENDIF(ENABLE_ACHIEVEMENTS)

# Write the caja-extension file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.caja-extension.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.caja-extension")

# Sources and headers (GTK3-specific)
SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}_GTK3MAX_SRCS}
	${${PROJECT_NAME}-notify_SRCS}
	${${PROJECT_NAME}_IFUNC_SRCS}
	NautilusPlugin.cpp
	NautilusPropertyPageProvider.cpp
	NautilusMenuProvider.c
	NautilusExtraInterfaces.c
	ThunarPlugin.cpp
	ThunarPropertyPageProvider.cpp
	ThunarMenuProvider.c
	MenuProviderCommon.c
	dlopen-notes.c
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_GTK3MAX_H}
	${${PROJECT_NAME}-notify_H}
	../plugin-helper.h
	NautilusPlugin.hpp
	NautilusPropertyPageProvider.hpp
	NautilusMenuProvider.h
	NautilusExtraInterfaces.h
	ThunarPlugin.hpp
	ThunarPropertyPageProvider.hpp
	ThunarMenuProvider.h
	thunarx-mini.h
	nautilus-extension-mini.h
	mime-types.convert-to-png.h
	MenuProviderCommon.h
	)
IF(ENABLE_DECRYPTION)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../config/KeyManagerTab_gtk3.cpp)
ENDIF(ENABLE_DECRYPTION)

IF(HAVE_GSOUND OR (HAVE_LIBCANBERRA AND HAVE_LIBCANBERRA_GTK3))
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../MessageSound.cpp)
	SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    ../MessageSound.hpp)
ENDIF(HAVE_GSOUND OR (HAVE_LIBCANBERRA AND HAVE_LIBCANBERRA_GTK3))

IF(BUILD_GTK3)
	ADD_LIBRARY(${PROJECT_NAME} MODULE ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H})
	IF(ENABLE_PCH)
		TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME} PRIVATE
			"${RP_CONFIG_LIBC_H}" "${RP_COMPILER_COMPAT_H}"
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
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rom-properties-glib)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE Cairo::gobject)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC GTK3::gtk GTK3::gdk GTK3::pango GTK3::cairo)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
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
		LIBRARY DESTINATION "${LibNautilusExtension3_EXTENSION_DIR}"
		COMPONENT "plugin"
		)

	# Symlinks are created for Caja, Nemo, and Thunar.
	# Reference: https://stackoverflow.com/questions/34578362/how-can-i-package-a-symlink-with-cpack
	ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ln -sf "${LibNautilusExtension3_EXTENSION_DIR}/${PROJECT_NAME}.so" ${PROJECT_NAME}.symlink.so
		WORKING_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
		VERBATIM
		)

	# Install symlinks.
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.so"
		DESTINATION "${LibCajaExtension3_EXTENSION_DIR}"
		RENAME "${PROJECT_NAME}.so"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.so"
		DESTINATION "${LibNemoExtension3_EXTENSION_DIR}"
		RENAME "${PROJECT_NAME}.so"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.so"
		DESTINATION "${ThunarX3_EXTENSIONS_DIR}"
		RENAME "${PROJECT_NAME}.so"
		COMPONENT "plugin"
		)

	# Install the Caja extension description file.
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.caja-extension"
		DESTINATION "${LibCajaExtension_EXTENSION_DESC_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${LibNautilusExtension3_EXTENSION_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_GTK3)
