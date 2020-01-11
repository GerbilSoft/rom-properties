PROJECT(rom-properties-cinnamon)

# Cinnamon (Nemo) frontend for rom-properties.
FIND_PACKAGE(GLib2 ${REQUIRE_CINNAMON} 2.26.0)
FIND_PACKAGE(GObject2 ${REQUIRE_CINNAMON} 2.26.0)
FIND_PACKAGE(GIO ${REQUIRE_CINNAMON} 2.26.0)
FIND_PACKAGE(Cairo)
# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_CINNAMON})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# Find the Nemo extension library.
	# Nemo was forked from Nautilus 3.x, so all versions of Nemo
	# use GTK+ 3.x.
	FIND_PACKAGE(LibNemoExtension ${REQUIRE_CINNAMON})
	IF(NOT LibNemoExtension_FOUND)
		# Nemo extension library was not found.
		# Disable the Cinnamon frontend.
		SET(BUILD_CINNAMON OFF CACHE INTERNAL "Build the Cinnamon (GTK+ 3.x) plugin." FORCE)
	ENDIF(NOT LibNemoExtension_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)
	# GTK+ 3.x (or required dependencies) were not found.
	# Disable the Cinnamon frontend.
	SET(BUILD_CINNAMON OFF CACHE INTERNAL "Build the Cinnamon (GTK+ 3.x) plugin." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND Cairo_FOUND AND GTK3_FOUND)

# Sources and headers. (GTK+ common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-cinnamon_SRCS  "${rom-properties-gtk_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-cinnamon_SRCS2 "${rom-properties-gtk3_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-cinnamon_H     "${rom-properties-gtk_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-cinnamon_H2    "${rom-properties-gtk3_H}")

# CPU-specific and optimized sources.
IF(rom-properties-gtk3_SSSE3_SRCS)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-cinnamon_IFUNC_SRCS "${rom-properties-gtk3_IFUNC_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-cinnamon_SSSE3_SRCS "${rom-properties-gtk3_SSSE3_SRCS}")

	# Disable LTO on the IFUNC files if LTO is known to be broken.
	IF(GCC_5xx_LTO_ISSUES)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-cinnamon_IFUNC_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
	ENDIF(GCC_5xx_LTO_ISSUES)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${rom-properties-cinnamon_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Sources and headers. (Cinnamon-specific)
SET(rom-properties-cinnamon_SRCS
	${rom-properties-cinnamon_SRCS} ${rom-properties-cinnamon_SRCS2}
	${rom-properties-cinnamon_IFUNC_SRCS}
	${rom-properties-cinnamon_SSSE3_SRCS}
	../gnome/rom-properties-plugin.c
	../gnome/rom-properties-provider.cpp
	)
SET(rom-properties-cinnamon_H
	${rom-properties-cinnamon_H} ${rom-properties-cinnamon_H2}
	../gnome/rom-properties-provider.hpp
	)

IF(BUILD_CINNAMON)
	ADD_LIBRARY(rom-properties-cinnamon MODULE
		${rom-properties-cinnamon_SRCS}
		)
	SET_TARGET_PROPERTIES(rom-properties-cinnamon PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-cinnamon)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-cinnamon
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
	TARGET_LINK_LIBRARIES(rom-properties-cinnamon PRIVATE cachemgr romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-cinnamon PRIVATE i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-cinnamon PUBLIC Cinnamon::libnemo-extension)
	TARGET_LINK_LIBRARIES(rom-properties-cinnamon PRIVATE Cairo::cairo)
	TARGET_LINK_LIBRARIES(rom-properties-cinnamon PUBLIC Gtk3::gtk3 GLib2::gio GLib2::gobject GLib2::glib)
	TARGET_COMPILE_DEFINITIONS(rom-properties-cinnamon
		PRIVATE G_LOG_DOMAIN=\"rom-properties-cinnamon\"
			RP_UI_GTK3_CINNAMON
		)
ENDIF(BUILD_CINNAMON)

#######################
# Install the plugin. #
#######################

IF(BUILD_CINNAMON)
	INSTALL(TARGETS rom-properties-cinnamon
		LIBRARY DESTINATION "${LibNemoExtension_EXTENSION_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-cinnamon PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${LibNemoExtension_EXTENSION_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_CINNAMON)
