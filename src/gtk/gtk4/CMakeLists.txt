# GTK+ 4.x UI frontend
# TODO: Not supported by any file browsers yet...
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-gtk4 LANGUAGES C CXX)

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
	INCLUDE(LibNautilusExtensionDirs)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND GdkPixbuf2_FOUND AND GTK4_FOUND)
	# GTK 4.x (or required dependencies) were not found.
	# Disable the GTK4 frontend.
	SET(BUILD_GTK4 OFF CACHE INTERNAL "Build the GTK 4.x plugin." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND AND GdkPixbuf2_FOUND AND GTK4_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_CSRCS "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_CH    "${rom-properties-gtk_H}")

# GdkImageConv (GTK+ 2.x; TODO: Switch to GdkTexture)
SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../xfce/GdkImageConv.cpp)
SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    ../xfce/GdkImageConv.hpp)

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
INCLUDE(CPUInstructionSetFlags)
IF(CPU_i386 OR CPU_amd64)
	# IFUNC functionality
	INCLUDE(CheckIfuncSupport)
	CHECK_IFUNC_SUPPORT()
	IF(HAVE_IFUNC)
		SET(${PROJECT_NAME}_IFUNC_SRCS ../xfce/GdkImageConv_ifunc.cpp)

		# Disable LTO on the IFUNC files if LTO is known to be broken.
		IF(GCC_5xx_LTO_ISSUES)
			SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_IFUNC_SRCS}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
		ENDIF(GCC_5xx_LTO_ISSUES)
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
	${${PROJECT_NAME}_CSRCS} ${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}-notify_SRCS} ${${PROJECT_NAME}-notify_H}
	${${PROJECT_NAME}_IFUNC_SRCS}
	${${PROJECT_NAME}_SSSE3_SRCS}
	../gtk3/RpNautilusPlugin.cpp
	../gtk3/RpNautilusProvider.cpp
	#../gtk3/RpThunarPlugin.cpp	# No plans for GTK4 Thunar yet...
	#../gtk3/RpThunarProvider.cpp	# No plans for GTK4 Thunar yet...
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_CH} ${${PROJECT_NAME}_H}
	../gtk3/RpNautilusPlugin.hpp
	../gtk3/RpNautilusProvider.hpp
	#../gtk3/RpThunarPlugin.hpp	# No plans for GTK4 Thunar yet...
	#../gtk3/RpThunarProvider.hpp	# No plans for GTK4 Thunar yet...
	../gtk3/plugin-helper.h
	../gtk3/thunarx-mini.h
	)

IF(HAVE_GSOUND)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} ../MessageSound.cpp)
	SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    ../MessageSound.hpp)
ENDIF(HAVE_GSOUND)

IF(BUILD_GTK4)
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
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../gtk3>	# gtk3
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../gtk3>	# gtk3
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xfce>	# xfce (for GdkImageConv)
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xfce>	# xfce (for GdkImageConv)
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# gtk
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# gtk
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>		# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>		# src
		)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE glibresources)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE GdkPixbuf2::gdkpixbuf2)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Gtk4::gtk4 GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
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
IF(0) #IF(BUILD_GTK4)
	# GNOME (Nautilus) extension is installed directly.
	INSTALL(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${LibNautilusExtension4_EXTENSION_DIR}"
		COMPONENT "plugin"
		)

	# Symlinks are created for Caja, Nemo, and Thunar.
	# Reference: https://stackoverflow.com/questions/34578362/how-can-i-package-a-symlink-with-cpack
	ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ln -sf "${LibNautilusExtension4_EXTENSION_DIR}/${PROJECT_NAME}.so" ${PROJECT_NAME}.symlink.so
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
		DESTINATION "${ThunarX4_EXTENSIONS_DIR}"
		RENAME "${PROJECT_NAME}.so"
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
ENDIF(0) #ENDIF(BUILD_GTK4)
