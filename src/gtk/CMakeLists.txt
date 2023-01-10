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

# Sources and headers
SET(${PROJECT_NAME}_SRCS
	RomDataView.cpp
	RomDataView_ops.cpp
	RomDataFormat.cpp
	DragImage.cpp
	CreateThumbnail.cpp
	PIMGTYPE.cpp
	rp-gtk-enums.c
	RpFile_gio.cpp
	MessageWidget.c
	RpGtk.cpp
	sort_funcs.c
	LanguageComboBox.cpp
	OptionsMenuButton.cpp
	is-supported.cpp
	AchSpritesheet.cpp
	ProxyForUrl.cpp
	config/stub-export.c
	config/ConfigDialog.cpp
	config/RpConfigTab.cpp
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
	RomDataFormat.hpp
	DragImage.hpp
	CreateThumbnail.hpp
	PIMGTYPE.hpp
	rp-gtk-enums.h
	RpFile_gio.hpp
	MessageWidget.h
	RpGtk.hpp
	sort_funcs.h
	glib-compat.h
	gtk-compat.h
	LanguageComboBox.hpp
	OptionsMenuButton.hpp
	check-uid.h
	is-supported.hpp
	AchSpritesheet.hpp
	ProxyForUrl.hpp
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
	)
SET(${PROJECT_NAME}_GTK4MIN_H
	LanguageComboBoxItem.h
	config/AchievementItem.h
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
IF(GLib2_FOUND AND GObject2_FOUND)
	# Check for version-specific image libraries.
	FIND_PACKAGE(GdkPixbuf2)
	IF(NOT GdkPixbuf2_FOUND)
		# Cannot build GTK2 frontends.
		SET(BUILD_GTK2 OFF CACHE INTERNAL "Build the GTK+ 2.x plugin." FORCE)
	ENDIF(NOT GdkPixbuf2_FOUND)
	FIND_PACKAGE(Cairo)
	IF(NOT Cairo_FOUND)
		# Cannot build GTK3 or GTK4 frontends.
		SET(BUILD_GTK3 OFF CACHE INTERNAL "Build the GTK+ 3.x plugin." FORCE)
		SET(BUILD_GTK4 OFF CACHE INTERNAL "Build the GTK 4.x plugin." FORCE)
	ENDIF(NOT Cairo_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND)
	# Cannot build any GTK+ frontends.
	SET(BUILD_GTK2 OFF CACHE INTERNAL "Build the GTK+ 2.x plugin." FORCE)
	SET(BUILD_GTK3 OFF CACHE INTERNAL "Build the GTK+ 3.x plugin." FORCE)
	SET(BUILD_GTK4 OFF CACHE INTERNAL "Build the GTK 4.x plugin." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND)

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

# Build GTK+ 2.x frontends.
IF(BUILD_GTK2)
	# GTK2 minimum is 2.0.0, so no version check is needed.
	FIND_PACKAGE(GTK2 ${REQUIRE_GTK2} COMPONENTS gtk)
	IF(GTK2_FOUND)
		ADD_SUBDIRECTORY(xfce)
	ELSE(GTK2_FOUND)
		SET(BUILD_GTK2 OFF CACHE INTERNAL "Build the GTK+ 2.x plugin." FORCE)
	ENDIF(GTK2_FOUND)
ENDIF(BUILD_GTK2)

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
