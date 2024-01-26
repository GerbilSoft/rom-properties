# GTK+ 2.x UI frontend for:
# - Thunar 1.6 (XFCE)
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
FIND_PACKAGE(GLib2 ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GObject2 ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO-UNIX ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GdkPixbuf2 ${REQUIRE_XFCE})
# GTK2 minimum is 2.18.0. (gtk_widget_set_visible)
FIND_PACKAGE(GTK2 2.18.0 ${REQUIRE_XFCE} COMPONENTS gtk)
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND GdkPixbuf2_FOUND AND GTK2_FOUND)
	# We're not going to have an explicit dependency on libnautilus-extension.
	# It will be dlopen()'d at runtime based on which
	# initialization function is called.

	# ThunarX2 (XFCE)
	FIND_PACKAGE(ThunarX2)
	IF(NOT ThunarX2_FOUND)
		MESSAGE(STATUS "ThunarX2 not found. Using the default extensions directory.")
		SET(ThunarX2_EXTENSIONS_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/thunarx-2")
	ENDIF(NOT ThunarX2_FOUND)
ELSE()
	# GTK+ 2.x (or required dependencies) were not found.
	# Disable the XFCE frontend.
	SET(BUILD_XFCE OFF)
	SET(BUILD_XFCE OFF PARENT_SCOPE)
	SET(BUILD_XFCE OFF CACHE INTERNAL "Build the XFCE (GTK+ 2.x) plugin. (Thunar 1.7 and earlier)" FORCE)
ENDIF()

# Ensure we don't use functions not available in GLib 2.34.
ADD_DEFINITIONS(-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_34 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_34)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_SRCS "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_H    "${rom-properties-gtk_H}")

# GdkImageConv (GTK+ 2.x)
SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} GdkImageConv.cpp)
SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    GdkImageConv.hpp)

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
		SET(${PROJECT_NAME}_IFUNC_SRCS GdkImageConv_ifunc.cpp)
	ENDIF(HAVE_IFUNC)

	# NOTE: SSSE3 flags are set in subprojects, not here.
	SET(${PROJECT_NAME}_SSSE3_SRCS GdkImageConv_ssse3.cpp)

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

# Sources and headers. (XFCE-specific)
SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}-notify_SRCS} ${${PROJECT_NAME}-notify_H}
	${${PROJECT_NAME}_IFUNC_SRCS}
	${${PROJECT_NAME}_SSSE3_SRCS}
	../gtk3/ThunarPlugin.cpp
	../gtk3/ThunarMenuProvider.cpp
	../gtk3/ThunarPropertyPageProvider.cpp
	../gtk3/MenuProviderCommon.c
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H} ${${PROJECT_NAME}_H}
	../plugin-helper.h
	../gtk3/ThunarPlugin.hpp
	../gtk3/ThunarMenuProvider.hpp
	../gtk3/ThunarPropertyPageProvider.hpp
	../gtk3/thunarx-mini.h
	../gtk3/MenuProviderCommon.h
	)
IF(ENABLE_DECRYPTION)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../config/KeyManagerTab_gtk3.cpp)
ENDIF(ENABLE_DECRYPTION)

IF(HAVE_GSOUND OR (HAVE_LIBCANBERRA AND HAVE_LIBCANBERRA_GTK2))
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../MessageSound.cpp)
	SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    ../MessageSound.hpp)
ENDIF(HAVE_GSOUND OR (HAVE_LIBCANBERRA AND HAVE_LIBCANBERRA_GTK2))

IF(BUILD_XFCE)
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
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# xfce
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# xfce
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# gtk
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# gtk
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>		# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>		# src
		)
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${GTK2_INCLUDE_DIRS})

	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rom-properties-glib)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE GdkPixbuf2::gdkpixbuf2)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC GTK2::gtk GTK2::gdk GTK2::pango)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	IF(HAVE_GSOUND)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE GSound::gsound)
		TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE ENABLE_MESSAGESOUND)
	ELSEIF(HAVE_LIBCANBERRA AND HAVE_LIBCANBERRA_GTK2)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE LibCanberra::gtk2 LibCanberra::libcanberra)
		TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE ENABLE_MESSAGESOUND)
	ENDIF()

	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	# TODO: Move GTK2_DEFINITIONS to TARGET_COMPILE_DEFINITIONS.
	# (Requires removing the "-D" switches.)
	ADD_DEFINITIONS(${GTK2_DEFINITIONS})
	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE RP_UI_XFCE)
ENDIF(BUILD_XFCE)

#######################
# Install the plugin. #
#######################

IF(BUILD_XFCE)
	# FIXME: ${ThunarX2_EXTENSIONS_DIR} always uses the system prefix.
	# This should be adjusted to ${CMAKE_INSTALL_PREFIX}.
	INSTALL(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${ThunarX2_EXTENSIONS_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${ThunarX2_EXTENSIONS_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_XFCE)
