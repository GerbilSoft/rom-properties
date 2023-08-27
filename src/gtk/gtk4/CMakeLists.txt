# GTK4 UI frontend
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-gtk4 LANGUAGES C CXX)

# GTK4 requires glib-2.66.0.
SET(GLIB_MIN_VERSION 2.66.0)

FIND_PACKAGE(GLib2 ${REQUIRE_GTK4} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GObject2 ${REQUIRE_GTK4} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO ${REQUIRE_GTK4} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO-UNIX ${REQUIRE_GTK4} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GdkPixbuf2 ${REQUIRE_GTK})	# GTK4 doesn't support Cairo. (TODO: GdkTexture)
FIND_PACKAGE(GTK4 ${REQUIRE_GTK4})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND GdkPixbuf2_FOUND AND GTK4_FOUND)
	# We're not going to have an explicit dependency on libnautilus-extension.
	# It will be dlopen()'d at runtime based on which
	# initialization function is called.

	# Nautilus (GNOME)
	FIND_PACKAGE(LibNautilusExtension4)	# v4
	IF(LibNautilusExtension4_FOUND)
		# Found v4.
	ELSE(LibNautilusExtension4_FOUND)
		# Check if v3 is installed.
		FIND_PACKAGE(LibNautilusExtension)
		IF(LibNautilusExtension)
			# v3: Replace the version number with 4.
			STRING(REPLACE "extensions-3.0" "extensions-4.0" LibNautilusExtension4_EXTENSION_DIR "${LibNautilusExtension_EXTENSION_DIR}")
		ELSE(LibNautilusExtension)
			# Neither v3 nor v4 were found.
			MESSAGE(STATUS "LibNautilusExtension4 not found. Using the default extensions directory.")
			SET(LibNautilusExtension4_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nautilus/extensions-4")
		ENDIF(LibNautilusExtension)
	ENDIF(LibNautilusExtension4_FOUND)

	# Verify the GLib version.
	IF(GLib2_VERSION VERSION_LESS ${GLIB_MIN_VERSION})
		MESSAGE(WARNING "GLib2 minimum version for GTK4 is ${GLIB_MIN_VERSION}, but ${GLib2_VERSION} was found.\nDisabling GTK4 build.")
		SET(BUILD_GTK4 OFF CACHE INTERNAL "Build the GTK 4.x plugin." FORCE)
		SET(LibNautilusExtension4_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nautilus/extensions-4")
	ENDIF(GLib2_VERSION VERSION_LESS ${GLIB_MIN_VERSION})
ELSE()
	# GTK 4.x (or required dependencies) were not found.
	# Disable the GTK4 frontend.
	SET(BUILD_GTK4 OFF CACHE INTERNAL "Build the GTK 4.x plugin." FORCE)
ENDIF()

# Ensure we don't use functions not available in GLib 2.44.
# NOTE: GLib 2.44 added GListModel, which is needed by GTK4.
# NOTE: GTK 4.0.0 requires GLib 2.66, so we'll use that as a minimum here.
ADD_DEFINITIONS(-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_66 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_66)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_SRCS "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_H    "${rom-properties-gtk_H}")

STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_GTK4MIN_SRCS "${rom-properties-gtk_GTK4MIN_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_GTK4MIN_H    "${rom-properties-gtk_GTK4MIN_H}")

# GdkImageConv (GTK+ 2.x; TODO: Switch to GdkTexture)
SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../xfce/GdkImageConv.cpp)
SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    ../xfce/GdkImageConv.hpp)

IF(ENABLE_ACHIEVEMENTS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-gtk-notify_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-gtk-notify_H}")
	SET(${PROJECT_NAME}-notify_SRCS ${${PROJECT_NAME}-notify_SRCS})
ENDIF(ENABLE_ACHIEVEMENTS)

# CPU-specific and optimized sources.
INCLUDE(CPUInstructionSetFlags)
IF(CPU_i386 OR CPU_amd64)
	# IFUNC functionality
	INCLUDE(CheckIfuncSupport)
	CHECK_IFUNC_SUPPORT()
	IF(HAVE_IFUNC)
		SET(${PROJECT_NAME}_IFUNC_SRCS ../xfce/GdkImageConv_ifunc.cpp)
	ENDIF(HAVE_IFUNC)

	# NOTE: SSSE3 flags are set in subprojects, not here.
	SET(${PROJECT_NAME}_SSSE3_SRCS ../xfce/GdkImageConv_ssse3.cpp)

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

# Sources and headers. (GTK4-specific)
SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_GTK4MIN_SRCS} ${${PROJECT_NAME}_GTK4MIN_H}
	${${PROJECT_NAME}-notify_SRCS} ${${PROJECT_NAME}-notify_H}
	${${PROJECT_NAME}_IFUNC_SRCS}
	${${PROJECT_NAME}_SSSE3_SRCS}
	NautilusPlugin.cpp
	NautilusPropertiesModelProvider.cpp
	NautilusPropertiesModel.cpp
	../gtk3/NautilusMenuProvider.cpp
	../gtk3/MenuProviderCommon.c
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H} ${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_GTK4MIN_H}
	../plugin-helper.h
	NautilusPlugin.hpp
	NautilusPropertiesModelProvider.hpp
	NautilusPropertiesModel.hpp
	../gtk3/NautilusMenuProvider.hpp
	nautilus-extension-mini.h
	../gtk3/MenuProviderCommon.h
	)
IF(ENABLE_DECRYPTION)
	# FIXME: Editing keys in the GTK4 version causes crashes.
	#SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../config/KeyManagerTab_gtk4.cpp ../config/KeyStoreItem.c)
	#SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H} ../config/KeyStoreItem.h)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../config/KeyManagerTab_gtk3.cpp)
ENDIF(ENABLE_DECRYPTION)

IF(HAVE_GSOUND)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../MessageSound.cpp)
	SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    ../MessageSound.hpp)
ENDIF(HAVE_GSOUND)

IF(BUILD_GTK4)
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
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# gtk4
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# gtk4
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xfce>	# xfce (for GdkImageConv)
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xfce>	# xfce (for GdkImageConv)
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# gtk
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# gtk
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>		# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>		# src
		)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rom-properties-glib)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE GdkPixbuf2::gdkpixbuf2)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC GTK4::gtk GTK4::pango)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	IF(HAVE_GSOUND)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE GSound::gsound)
		TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE ENABLE_MESSAGESOUND)
	ENDIF(HAVE_GSOUND)

	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME}
		PRIVATE RP_UI_GTK4
		)
ENDIF(BUILD_GTK4)

#######################
# Install the plugin. #
#######################

# FIXME: Don't use .pc files for the extension libraries, since only
# one version (3 or 4) can be installed at a time. Use defaults only.
IF(BUILD_GTK4)
	# GNOME (Nautilus) extension is installed directly.
	INSTALL(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${LibNautilusExtension4_EXTENSION_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${LibNautilusExtension4_EXTENSION_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_GTK4)
