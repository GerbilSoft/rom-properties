# GTK+ 3.x UI frontend for:
# - Nautilus 3.x (GNOME)
# - Caja 1.18 (MATE)
# - Nemo (Cinnamon)
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-gtk3 LANGUAGES C CXX)

FIND_PACKAGE(GLib2 ${REQUIRE_GTK3} 2.26.0)
FIND_PACKAGE(GObject2 ${REQUIRE_GTK3} 2.26.0)
FIND_PACKAGE(GIO ${REQUIRE_GTK3} 2.26.0)
FIND_PACKAGE(Cairo ${REQUIRE_GTK3})
# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_GTK3})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# We're not going to have an explicit dependency on libnautilus-extension.
	# It will be dlopen()'d at runtime based on which
	# initialization function is called.

	# Get the various extensions directories.
	FIND_PACKAGE(LibNautilusExtension 3.0.0)
	IF(NOT LibNautilusExtension_FOUND)
		MESSAGE(STATUS "LibNautilusExtension not found. Using default extensions directory.")
	ENDIF(NOT LibNautilusExtension_FOUND)
	FIND_PACKAGE(LibCajaExtension 1.18.0)
	IF(NOT LibCajaExtension_FOUND)
		MESSAGE(STATUS "LibCajaExtension not found. Using default extensions directory.")
	ENDIF(NOT LibCajaExtension_FOUND)
	FIND_PACKAGE(LibNemoExtension)
	IF(NOT LibNemoExtension_FOUND)
		MESSAGE(STATUS "LibNemoExtension not found. Using default extensions directory.")
	ENDIF(NOT LibNemoExtension_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# GTK+ 3.x (or required dependencies) were not found.
	# Disable the GNOME frontend.
	SET(BUILD_GTK3 OFF CACHE INTERNAL "Build the GTK+ 3.x plugin. (GNOME, MATE, and Cinnamon)" FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gtk3_CSRCS  "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gtk3_CSRCS2 "${rom-properties-gtk3_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gtk3_CH     "${rom-properties-gtk_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gtk3_CH2    "${rom-properties-gtk3_H}")

# CPU-specific and optimized sources.
IF(rom-properties-gtk3_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gtk3_IFUNC_SRCS "${rom-properties-gtk3_IFUNC_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-gtk3_SSSE3_SRCS "${rom-properties-gtk3_SSSE3_SRCS}")

	# Disable LTO on the IFUNC files if LTO is known to be broken.
	IF(GCC_5xx_LTO_ISSUES)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-gtk3_IFUNC_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
	ENDIF(GCC_5xx_LTO_ISSUES)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-gtk3_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Sources and headers. (GTK3-specific)
SET(rom-properties-gtk3_SRCS
	${rom-properties-gtk3_CSRCS} ${rom-properties-gtk3_CSRCS2}
	${rom-properties-gtk3_IFUNC_SRCS}
	${rom-properties-gtk3_SSSE3_SRCS}
	RpNautilusPlugin.c
	RpNautilusProvider.cpp
	)
SET(rom-properties-gtk3_H
	${rom-properties-gtk3_CH} ${rom-properties-gtk3_CH2}
	RpNautilusPlugin.h
	RpNautilusProvider.hpp
	)

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(rom-properties-gtk3_PCH_C ../stdafx.c)
	SET(rom-properties-gtk3_PCH_CXX ../stdafx.cpp)
	SET(rom-properties-gtk3_PCH_SRC ${rom-properties-gtk3_PCH_C} ${rom-properties-gtk3_PCH_CXX})
	SET(rom-properties-gtk3_PCH_H ../stdafx.h)
ENDIF(ENABLE_PCH)

IF(BUILD_GTK3)
	ADD_LIBRARY(rom-properties-gtk3 MODULE
		${rom-properties-gtk3_PCH_SRC} ${rom-properties-gtk3_PCH_H}
		${rom-properties-gtk3_SRCS}
		)
	IF(ENABLE_PCH)
		ADD_PRECOMPILED_HEADER(rom-properties-gtk3 ${rom-properties-gtk3_PCH_H}
			SOURCE_C ${rom-properties-gtk3_PCH_C}
			SOURCE_CXX ${rom-properties-gtk3_PCH_CXX}
			)
	ENDIF(ENABLE_PCH)
	SET_TARGET_PROPERTIES(rom-properties-gtk3 PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-gtk3)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-gtk3
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
	TARGET_LINK_LIBRARIES(rom-properties-gtk3 PRIVATE glibresources)
	TARGET_LINK_LIBRARIES(rom-properties-gtk3 PRIVATE rpcpu romdata rpfile rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-gtk3 PRIVATE i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-gtk3 PRIVATE Cairo::cairo)
	TARGET_LINK_LIBRARIES(rom-properties-gtk3 PUBLIC Gtk3::gtk3 GLib2::gio GLib2::gobject GLib2::glib)

	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(rom-properties-gtk3 PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	TARGET_COMPILE_DEFINITIONS(rom-properties-gtk3
		PRIVATE RP_UI_GTK3_GNOME
		)
ENDIF(BUILD_GTK3)

#######################
# Install the plugin. #
#######################

IF(BUILD_GTK3)
	# Check the extension directories.
	# Default paths will be used if they aren't found.
	IF(NOT LibNautilusExtension_EXTENSION_DIR)
		SET(LibNautilusExtension_EXTENSION_DIR "${CMAKE_INSTALL_LIBDIR}/nautilus/extensions-3.0")
	ENDIF(NOT LibNautilusExtension_EXTENSION_DIR)
	IF(NOT LibCajaExtension_EXTENSION_DIR)
		SET(LibCajaExtension_EXTENSION_DIR "${CMAKE_INSTALL_LIBDIR}/caja/extensions-2.0")
	ENDIF(NOT LibCajaExtension_EXTENSION_DIR)
	IF(NOT LibNemoExtension_EXTENSION_DIR)
		SET(LibNemoExtension_EXTENSION_DIR "${CMAKE_INSTALL_LIBDIR}/nemo/extensions-3.0")
	ENDIF(NOT LibNemoExtension_EXTENSION_DIR)

	# GNOME (Nautilus) extension is installed directly.
	INSTALL(TARGETS rom-properties-gtk3
		LIBRARY DESTINATION "${LibNautilusExtension_EXTENSION_DIR}"
		COMPONENT "plugin"
		)

	# Symlinks are created for MATE (Caja) and Cinnamon (Nemo).
	# Reference: https://stackoverflow.com/questions/34578362/how-can-i-package-a-symlink-with-cpack
	ADD_CUSTOM_COMMAND(TARGET rom-properties-gtk3 POST_BUILD
		COMMAND ln -sf "${LibNautilusExtension_EXTENSION_DIR}/rom-properties-gtk3.so" rom-properties-gtk3.symlink.so
		WORKING_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
		VERBATIM
		)

	# Install symlinks.
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/rom-properties-gtk3.symlink.so"
		DESTINATION "${LibCajaExtension_EXTENSION_DIR}"
		RENAME "rom-properties-gtk3.so"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/rom-properties-gtk3.symlink.so"
		DESTINATION "${LibNemoExtension_EXTENSION_DIR}"
		RENAME "rom-properties-gtk3.so"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-gtk3 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${LibNautilusExtension_EXTENSION_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_GTK3)
