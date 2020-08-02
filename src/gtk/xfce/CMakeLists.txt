# GTK+ 2.x UI frontend for:
# - Thunar 1.6 (XFCE)
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-xfce LANGUAGES C CXX)

# Disable glib deprecation warnings.
# - GTypeDebugFlags is deprecated as of glib-2.36.
# - GTimeVal is deprecated as of glib-2.62.
# GTK+ 2.x uses both of these, and since it's unlikely that
# GTK+ 2.x will ever be updated to use the non-deprecated
# versions, simply remove the deprecation warning.
ADD_DEFINITIONS(-DGLIB_DISABLE_DEPRECATION_WARNINGS)

# Find packages.
FIND_PACKAGE(GLib2 ${REQUIRE_GTK2} 2.26.0)
FIND_PACKAGE(GObject2 ${REQUIRE_GTK2} 2.26.0)
FIND_PACKAGE(GIO ${REQUIRE_GTK2} 2.26.0)
FIND_PACKAGE(GdkPixbuf2 ${REQUIRE_GTK2})
# GTK2 minimum is 2.0.0, so no version check is needed.
FIND_PACKAGE(GTK2 ${REQUIRE_GTK2} COMPONENTS gtk)
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)
	# We're not going to have an explicit dependency on libnautilus-extension.
	# It will be dlopen()'d at runtime based on which
	# initialization function is called.

	# ThunarX2 (XFCE)
	FIND_PACKAGE(ThunarX2)
	IF(NOT ThunarX2_FOUND)
		MESSAGE(STATUS "ThunarX2 not found. Using the default extensions directory.")
		SET(ThunarX2_EXTENSIONS_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/thunarx-2")
	ENDIF(NOT ThunarX2_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)
	# GTK+ 2.x (or required dependencies) were not found.
	# Disable the XFCE frontend.
	SET(BUILD_GTK2 OFF CACHE INTERNAL "Build the XFCE (GTK+ 2.x) plugin. (Thunar 1.7 and earlier)" FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_SRCS  "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_SRCS2 "${rom-properties-gtk2_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_H     "${rom-properties-gtk_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_H2    "${rom-properties-gtk2_H}")

# CPU-specific and optimized sources.
IF(rom-properties-gtk2_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_IFUNC_SRCS "${rom-properties-gtk2_IFUNC_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-xfce_SSSE3_SRCS "${rom-properties-gtk2_SSSE3_SRCS}")

	# Disable LTO on the IFUNC files if LTO is known to be broken.
	IF(GCC_5xx_LTO_ISSUES)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-xfce_IFUNC_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
	ENDIF(GCC_5xx_LTO_ISSUES)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-xfce_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Sources and headers. (XFCE-specific)
SET(rom-properties-xfce_SRCS
	${rom-properties-xfce_SRCS} ${rom-properties-xfce_SRCS2}
	${rom-properties-xfce_IFUNC_SRCS}
	${rom-properties-xfce_SSSE3_SRCS}
	../gtk3/RpThunarPlugin.c
	../gtk3/RpThunarProvider.cpp
	../gtk3/is-supported.cpp
	)
SET(rom-properties-xfce_H
	${rom-properties-xfce_H} ${rom-properties-xfce_H2}
	../gtk3/RpThunarPlugin.h
	../gtk3/RpThunarProvider.hpp
	../gtk3/is-supported.hpp
	../gtk3/plugin-helper.h
	../gtk3/thunarx-mini.h
	)

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(rom-properties-xfce_PCH_C ../stdafx.c)
	SET(rom-properties-xfce_PCH_CXX ../stdafx.cpp)
	SET(rom-properties-xfce_PCH_SRC ${rom-properties-xfce_PCH_C} ${rom-properties-xfce_PCH_CXX})
	SET(rom-properties-xfce_PCH_H ../stdafx.h)
ENDIF(ENABLE_PCH)

IF(BUILD_GTK2)
	ADD_LIBRARY(rom-properties-xfce MODULE
		${rom-properties-xfce_PCH_SRC} ${rom-properties-xfce_PCH_H}
		${rom-properties-xfce_SRCS}
		)
	IF(ENABLE_PCH)
		ADD_PRECOMPILED_HEADER(rom-properties-xfce ${rom-properties-xfce_PCH_H}
			SOURCE_C ${rom-properties-xfce_PCH_C}
			SOURCE_CXX ${rom-properties-xfce_PCH_CXX}
			)
	ENDIF(ENABLE_PCH)
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

	TARGET_LINK_LIBRARIES(rom-properties-xfce PRIVATE glibresources)
	TARGET_LINK_LIBRARIES(rom-properties-xfce PRIVATE rpcpu romdata rpfile rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-xfce PRIVATE i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-xfce PRIVATE GdkPixbuf2::gdkpixbuf2)
	TARGET_LINK_LIBRARIES(rom-properties-xfce PUBLIC ${GTK2_LIBRARIES} GLib2::gio GLib2::gobject GLib2::glib)
	# TODO: Move GTK2_DEFINITIONS to TARGET_COMPILE_DEFINITIONS.
	# (Requires removing the "-D" switches.)
	ADD_DEFINITIONS(${GTK2_DEFINITIONS})
	TARGET_COMPILE_DEFINITIONS(rom-properties-xfce
		PRIVATE RP_UI_GTK2_XFCE
		)
ENDIF(BUILD_GTK2)

#######################
# Install the plugin. #
#######################

IF(BUILD_GTK2)
	# FIXME: ${ThunarX2_EXTENSIONS_DIR} always uses the system prefix.
	# This should be adjusted to ${CMAKE_INSTALL_PREFIX}.
	INSTALL(TARGETS rom-properties-xfce
		LIBRARY DESTINATION "${ThunarX2_EXTENSIONS_DIR}"
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
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_GTK2)
