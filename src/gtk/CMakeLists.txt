# GTK+ UI frontends
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-gtk LANGUAGES C CXX)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)

# Disable deprecated GTK+ 2.x functionality that isn't available in GTK+ 3.x.
# Reference: https://developer.gnome.org/gtk3/stable/gtk-migrating-2-to-3.html
# NOTE: Debug builds only! Release builds should have these disabled because
# they could cause breakage with earlier versions of GTK+.
SET(GLIB_DEPRECATED_FLAGS "-DG_DISABLE_DEPRECATED")
SET(GDK_DEPRECATED_FLAGS "-DGDK_DISABLE_DEPRECATED")
SET(GTK_DEPRECATED_FLAGS "-DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE")
SET(GDK_PIXBUF_DEPRECATED_FLAGS "-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES")

SET(GTK_ALL_DEPRECATED_FLAGS "${GLIB_DEPRECATED_FLAGS} ${GDK_DEPRECATED_FLAGS} ${GTK_DEPRECATED_FLAGS} ${GDK_PIXBUF_DEPRECATED_FLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${GTK_ALL_DEPRECATED_FLAGS}")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${GTK_ALL_DEPRECATED_FLAGS}")

# Check if dlvsym() is available.
INCLUDE(CheckSymbolExists)
SET(OLD_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
SET(OLD_CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS})
SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${CMAKE_DL_LIBS})
SET(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_GNU_SOURCE)
CHECK_SYMBOL_EXISTS(dlvsym "dlfcn.h" HAVE_DLVSYM)
SET(CMAKE_REQUIRED_LIBRARIES ${OLD_CMAKE_REQUIRED_LIBRARIES})
SET(CMAKE_REQUIRED_DEFINITIONS ${OLD_CMAKE_REQUIRED_DEFINITIONS})
UNSET(OLD_CMAKE_REQUIRED_LIBRARIES)
UNSET(OLD_CMAKE_REQUIRED_DEFINITIONS)

# Generate GDBus bindings. (TODO: Split into a separate file?)
# _outvar: Variable for the generated source files
# _prefix: Prefix for the filenames
# _xml: D-Bus XML filename
FUNCTION(GDBUS_CODEGEN _outvar _prefix _xml)
	ADD_CUSTOM_COMMAND(
		OUTPUT "${_prefix}.c" "${_prefix}.h"
		COMMAND "${GDBUS_CODEGEN}"
			--generate-c-code "${_prefix}"
			"${_xml}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
		DEPENDS "${_xml}"
		VERBATIM
		)

	# Disable unused parameter warnings in generated D-Bus sources.
	INCLUDE(CheckCCompilerFlag)
	CHECK_C_COMPILER_FLAG("-Wno-unused-parameter" CFLAG_Wno_unused_parameter)
	IF(CFLAG_Wno_unused_parameter)
		SET_SOURCE_FILES_PROPERTIES("${CMAKE_CURRENT_BINARY_DIR}/${_prefix}.c"
			APPEND_STRING PROPERTIES COMPILE_FLAGS " -Wno-unused-parameter ")
	ENDIF(CFLAG_Wno_unused_parameter)

	SET(${_outvar} "${_prefix}.c" "${_prefix}.h" PARENT_SCOPE)
ENDFUNCTION(GDBUS_CODEGEN)

# D-Bus bindings for NetworkManager.
# NOTE: This has to be in this file, not the GTK+ CMakeLists.txt.
GDBUS_CODEGEN(NetworkManager_GDBus_SRCS NetworkManager "${CMAKE_CURRENT_SOURCE_DIR}/../dbus/org.freedesktop.NetworkManager.xml")
IF(ENABLE_ACHIEVEMENTS)
	GDBUS_CODEGEN(Notifications_GDBus_SRCS Notifications "${CMAKE_CURRENT_SOURCE_DIR}/../dbus/org.freedesktop.Notifications.xml")
ENDIF(ENABLE_ACHIEVEMENTS)

# Sources and headers (GLib common; no GTK dependencies)
SET(${PROJECT_NAME}_GLib_SRCS
	${NetworkManager_GDBus_SRCS}
	${Notifications_GDBus_SRCS}
	RomDataFormat.cpp
	RpFile_gio.cpp
	is-supported.cpp
	ProxyForUrl.cpp
	)
SET(${PROJECT_NAME}_GLib_H
	RomDataFormat.hpp
	RpFile_gio.hpp
	is-supported.hpp
	ProxyForUrl.hpp
	glib-compat.h
	)

# Sources and headers
SET(${PROJECT_NAME}_SRCS
	RomDataView.cpp
	RomDataView_ops.cpp
	DragImage.cpp
	CreateThumbnail.cpp
	PIMGTYPE.cpp
	rp-gtk-enums.c
	MessageWidget.c
	RpGtk.cpp
	sort_funcs.c
	gtk-compat.c
	LanguageComboBox.cpp
	OptionsMenuButton.cpp
	AchSpritesheet.cpp
	config/stub-export.c
	config/ConfigDialog.cpp
	config/RpConfigTab.c
	config/ImageTypesTab.cpp
	config/SystemsTab.cpp
	config/OptionsTab.cpp
	config/CacheTab.cpp
	config/CacheCleaner.cpp
	config/AchievementsTab.cpp
	config/AboutTab.cpp
	config/UpdateChecker.cpp
	xattr/LinuxAttrView.c
	xattr/DosAttrView.c
	xattr/XAttrView.cpp
	)
SET(${PROJECT_NAME}_H
	RomDataView.hpp
	RomDataView_p.hpp
	DragImage.hpp
	CreateThumbnail.hpp
	PIMGTYPE.hpp
	rp-gtk-enums.h
	MessageWidget.h
	RpGtk.hpp
	sort_funcs.h
	gtk-compat.h
	LanguageComboBox.hpp
	OptionsMenuButton.hpp
	check-uid.h
	AchSpritesheet.hpp
	gtk-i18n.h
	config/ConfigDialog.hpp
	config/RpConfigTab.h
	config/ImageTypesTab.hpp
	config/SystemsTab.hpp
	config/OptionsTab.hpp
	config/CacheTab.hpp
	config/CacheCleaner.hpp
	config/AchievementsTab.hpp
	config/AboutTab.hpp
	config/UpdateChecker.hpp
	xattr/LinuxAttrView.h
	xattr/DosAttrView.h
	xattr/XAttrView.hpp
	)

# Sources and headers (GTK4+ only)
SET(${PROJECT_NAME}_GTK4MIN_SRCS
	LanguageComboBoxItem.c
	config/AchievementItem.c
	config/KeyStoreItem.c
	)
SET(${PROJECT_NAME}_GTK4MIN_H
	LanguageComboBoxItem.h
	config/AchievementItem.h
	config/KeyStoreItem.h
	)

IF(ENABLE_DECRYPTION)
	SET(${PROJECT_NAME}_SRCS
		${${PROJECT_NAME}_SRCS}
		config/KeyManagerTab.cpp
		config/KeyStoreGTK.cpp
		)
	SET(${PROJECT_NAME}_H
		${${PROJECT_NAME}_H}
		config/KeyManagerTab.hpp
		config/KeyManagerTab_p.hpp
		config/KeyStoreGTK.hpp
		)
ENDIF(ENABLE_DECRYPTION)

IF(ENABLE_ACHIEVEMENTS)
	# D-Bus notification for achievements
	SET(${PROJECT_NAME}-notify_SRCS AchGDBus.cpp)
	SET(${PROJECT_NAME}-notify_H    AchGDBus.hpp)
ENDIF(ENABLE_ACHIEVEMENTS)

# Common libraries required for both GTK+ 2.x and 3.x.
FIND_PACKAGE(GLib2 ${GLIB_MIN_VERSION})
FIND_PACKAGE(GObject2 ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO ${REQUIRE_GTK4} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO-UNIX ${REQUIRE_GTK4} ${GLIB_MIN_VERSION})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)
	# Check for version-specific image libraries.
	FIND_PACKAGE(GdkPixbuf2)
	IF(NOT GdkPixbuf2_FOUND)
		# Cannot build GTK2 frontends.
		SET(BUILD_GTK2 OFF)
		SET(BUILD_GTK2 OFF PARENT_SCOPE)
		SET(BUILD_XFCE OFF CACHE INTERNAL "Build the XFCE (GTK+ 2.x) plugin." FORCE)
	ENDIF(NOT GdkPixbuf2_FOUND)
	FIND_PACKAGE(Cairo)
	IF(NOT Cairo_FOUND)
		# Cannot build GTK3 or GTK4 frontends.
		SET(BUILD_GTK3 OFF CACHE INTERNAL "Build the GTK+ 3.x plugin." FORCE)
		SET(BUILD_GTK4 OFF CACHE INTERNAL "Build the GTK 4.x plugin." FORCE)
	ENDIF(NOT Cairo_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)
	# Cannot build any GTK+ frontends.
	SET(BUILD_GTK2 OFF)
	SET(BUILD_GTK2 OFF PARENT_SCOPE)
	SET(BUILD_XFCE OFF CACHE INTERNAL "Build the XFCE (GTK+ 2.x) plugin." FORCE)
	SET(BUILD_GTK3 OFF CACHE INTERNAL "Build the GTK+ 3.x plugin." FORCE)
	SET(BUILD_GTK4 OFF CACHE INTERNAL "Build the GTK 4.x plugin." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)

# LibCanberra and related packages.
# TODO: Add an OPTION() to allow the user to disable this?
FIND_PACKAGE(GSound)
IF(GSound_FOUND)
	SET(HAVE_GSOUND 1)
ELSE(GSound_FOUND)
	FIND_PACKAGE(LibCanberra)
	IF(LibCanberra_FOUND)
		SET(HAVE_LIBCANBERRA 1)
		IF(BUILD_GTK2)
			FIND_PACKAGE(LibCanberraGtk2)
			IF(LibCanberraGtk2_FOUND)
				SET(HAVE_LIBCANBERRA_GTK2 1)
			ENDIF(LibCanberraGtk2_FOUND)
		ENDIF(BUILD_GTK2)
		IF(BUILD_GTK3)
			FIND_PACKAGE(LibCanberraGtk3)
			IF(LibCanberraGtk3_FOUND)
				SET(HAVE_LIBCANBERRA_GTK3 1)
			ENDIF(LibCanberraGtk3_FOUND)
		ENDIF(BUILD_GTK3)
	ENDIF(LibCanberra_FOUND)
ENDIF(GSound_FOUND)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.gtk.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.gtk.h")

IF(BUILD_GTK2 OR BUILD_GTK3 OR BUILD_GTK4)
	# GLib common library
	ADD_LIBRARY(rom-properties-glib STATIC
		${${PROJECT_NAME}_GLib_SRCS}
		${${PROJECT_NAME}_GLib_H}
		)
	SET_TARGET_PROPERTIES(rom-properties-glib PROPERTIES EXCLUDE_FROM_ALL TRUE)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-glib
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# gtk
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# gtk
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>		# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>		# src
		)
	TARGET_LINK_LIBRARIES(rom-properties-glib PUBLIC glibresources)
	TARGET_LINK_LIBRARIES(rom-properties-glib PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	TARGET_LINK_LIBRARIES(rom-properties-glib PRIVATE romdata)		# for config.librptext.h
ENDIF(BUILD_GTK2 OR BUILD_GTK3 OR BUILD_GTK4)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
# NOTE: This will be propagated to subprojects.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Build GTK+ 2.x frontends.
IF(BUILD_XFCE)
	# GTK2 minimum is 2.18.0. (gtk_widget_set_visible)
	FIND_PACKAGE(GTK2 2.18.0 ${REQUIRE_GTK2} COMPONENTS gtk)
	IF(GTK2_FOUND)
		ADD_SUBDIRECTORY(xfce)
	ELSE(GTK2_FOUND)
		SET(BUILD_GTK2 OFF PARENT_SCOPE)
		SET(BUILD_XFCE OFF CACHE INTERNAL "Build the GTK+ 2.x plugin." FORCE)
	ENDIF(GTK2_FOUND)
ENDIF(BUILD_XFCE)

# Build GTK+ 3.x frontends.
IF(BUILD_GTK3)
	# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
	FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_GTK3})
	IF(GTK3_FOUND)
		ADD_SUBDIRECTORY(gtk3)
	ELSE(GTK3_FOUND)
		SET(BUILD_GTK3 OFF CACHE INTERNAL "Build the GTK+ 3.x plugin." FORCE)
	ENDIF(GTK3_FOUND)
ENDIF(BUILD_GTK3)

# Build GTK 4.x frontends.
IF(BUILD_GTK4)
	FIND_PACKAGE(GTK4 ${REQUIRE_GTK4})
	IF(GTK4_FOUND)
		ADD_SUBDIRECTORY(gtk4)
	ELSE(GTK4_FOUND)
		SET(BUILD_GTK4 OFF CACHE INTERNAL "Build the GTK 4.x plugin." FORCE)
	ENDIF(GTK4_FOUND)
ENDIF(BUILD_GTK4)

IF(GTK3_FOUND OR GTK4_FOUND)
	# Process the .thumbnailer file.
	INCLUDE(DirInstallPaths)
	INCLUDE(ParseMimeTypes)
	PARSE_MIME_TYPES(MIMETYPES_THUMBONLY "${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types")
	CONFIGURE_FILE(rom-properties.thumbnailer.in rom-properties.thumbnailer @ONLY)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/rom-properties.thumbnailer"
		DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/thumbnailers"
		COMPONENT "plugin"
		)
ENDIF(GTK3_FOUND OR GTK4_FOUND)

# Build the D-Bus thumbnailer.
# Required for XFCE.
IF(BUILD_GTK2 OR BUILD_GTK3 OR BUILD_GTK4)
	SET(BUILD_THUMBNAILER_DBUS ON CACHE INTERNAL "Build the D-Bus thumbnailer." FORCE)
	ADD_SUBDIRECTORY(thumbnailer-dbus)
ENDIF(BUILD_GTK2 OR BUILD_GTK3 OR BUILD_GTK4)
