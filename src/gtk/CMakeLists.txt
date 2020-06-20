# GTK+ UI frontends
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
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

# CPU-specific and optimized sources.
IF(CPU_i386 OR CPU_amd64)
	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(rom-properties-gtk2_IFUNC_SRCS GdkImageConv_ifunc.cpp)
	ENDIF(UNIX AND NOT APPLE)

	# NOTE: SSSE3 flags are set in subprojects, not here.
	SET(rom-properties-gtk2_SSSE3_SRCS GdkImageConv_ssse3.cpp)
ENDIF(CPU_i386 OR CPU_amd64)

# Sources and headers.
SET(rom-properties-gtk_SRCS
	RomDataView.cpp
	DragImage.cpp
	CreateThumbnail.cpp
	PIMGTYPE.cpp
	rp-gtk-enums.c
	RpFile_gio.cpp
	)
SET(rom-properties-gtk_H
	RomDataView.hpp
	DragImage.hpp
	GdkImageConv.hpp
	PIMGTYPE.hpp
	rp-gtk-enums.h
	RpFile_gio.hpp
	)

# GTK2 sources and headers.
SET(rom-properties-gtk2_SRCS GdkImageConv.cpp)
SET(rom-properties-gtk2_H GdkImageConv.hpp)

# GTK3 sources and headers.
SET(rom-properties-gtk3_SRCS CairoImageConv.cpp)
SET(rom-properties-gtk3_H CairoImageConv.hpp)

# Common libraries required for both GTK+ 2.x and 3.x.
FIND_PACKAGE(GLib2 2.26.0)
FIND_PACKAGE(GObject2 2.26.0)
IF(GLib2_FOUND AND GObject2_FOUND)
	# Check for version-specific image libraries.
	FIND_PACKAGE(GdkPixbuf2)
	IF(NOT GdkPixbuf2_FOUND)
		# Cannot build GTK2 frontends.
		SET(BUILD_GTK2 OFF CACHE INTERNAL "Check for GTK+ 2.x." FORCE)
		SET(BUILD_XFCE OFF CACHE INTERNAL "Build the XFCE (GTK+ 2.x) plugin. (Thunar 1.7 and earlier)" FORCE)
	ENDIF(NOT GdkPixbuf2_FOUND)
	FIND_PACKAGE(Cairo)
	IF(NOT Cairo_FOUND)
		# Cannot build GTK3 frontends.
		SET(BUILD_GTK3 OFF CACHE INTERNAL "Check for GTK+ 3.x." FORCE)
		SET(BUILD_GNOME OFF CACHE INTERNAL "Build the GNOME (GTK+ 3.x) plugin." FORCE)
		SET(BUILD_XFCE3 OFF CACHE INTERNAL "Build the XFCE (GTK+ 3.x) plugin. (Thunar 1.8 and later)" FORCE)
	ENDIF(NOT Cairo_FOUND)
ELSE(GLib2_FOUND AND GObject2_FOUND)
	# Cannot build any GTK+ frontends.
	SET(BUILD_GTK2 OFF CACHE INTERNAL "Check for GTK+ 2.x." FORCE)
	SET(BUILD_GTK3 OFF CACHE INTERNAL "Check for GTK+ 3.x." FORCE)
	SET(BUILD_XFCE OFF CACHE INTERNAL "Build the XFCE (GTK+ 2.x) plugin. (Thunar 1.7 and earlier)" FORCE)
	SET(BUILD_XFCE3 OFF CACHE INTERNAL "Build the XFCE (GTK+ 3.x) plugin. (Thunar 1.8 and later)" FORCE)
	SET(BUILD_GNOME OFF CACHE INTERNAL "Build the GNOME (GTK+ 3.x) plugin." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND)

# Build GTK+ 2.x frontends.
IF(BUILD_GTK2)
	# GTK2 minimum is 2.0.0, so no version check is needed.
	FIND_PACKAGE(GTK2 ${REQUIRE_GTK2} COMPONENTS gtk)
	IF(GTK2_FOUND)
		ADD_SUBDIRECTORY(xfce)
	ELSE(GTK2_FOUND)
		SET(BUILD_GTK2 OFF CACHE INTERNAL "Check for GTK+ 2.x." FORCE)
		SET(BUILD_XFCE OFF CACHE INTERNAL "Build the XFCE (GTK+ 2.x) plugin. (Thunar 1.7 and earlier)" FORCE)
	ENDIF(GTK2_FOUND)
ELSE(BUILD_GTK2)
	# No GTK2 builds.
	SET(BUILD_XFCE OFF CACHE INTERNAL "Build the XFCE (GTK+ 2.x) plugin. (Thunar 1.7 and earlier)" FORCE)
ENDIF(BUILD_GTK2)

# Build GTK+ 3.x frontends.
IF(BUILD_GTK3)
	# GTK3 minimum is 3.10.0. (gtk_image_set_from_surface)
	FIND_PACKAGE(GTK3 3.10.0 ${REQUIRE_GTK3})
	IF(GTK3_FOUND)
		ADD_SUBDIRECTORY(gnome)
		ADD_SUBDIRECTORY(xfce3)

		IF(BUILD_GNOME)
			# Process the .thumbnailer file.
			INCLUDE(DirInstallPaths)
			CONFIGURE_FILE(rom-properties.thumbnailer.in rom-properties.thumbnailer)
			INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/rom-properties.thumbnailer"
				DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/thumbnailers"
				COMPONENT "plugin"
				)
		ENDIF(BUILD_GNOME)
	ELSE(GTK3_FOUND)
		SET(BUILD_GTK3 OFF CACHE INTERNAL "Check for GTK+ 3.x." FORCE)
		SET(BUILD_GNOME OFF CACHE INTERNAL "Build the GNOME (GTK+ 3.x) plugin." FORCE)
		SET(BUILD_XFCE3 OFF CACHE INTERNAL "Build the XFCE (GTK+ 3.x) plugin. (Thunar 1.8 and later)" FORCE)
	ENDIF(GTK3_FOUND)
ELSE(BUILD_GTK3)
	# No GTK3 builds.
	SET(BUILD_GNOME OFF CACHE INTERNAL "Build the GNOME (GTK+ 3.x) plugin." FORCE)
	SET(BUILD_XFCE3 OFF CACHE INTERNAL "Build the XFCE (GTK+ 3.x) plugin. (Thunar 1.8 and later)" FORCE)
ENDIF(BUILD_GTK3)

# Build the D-Bus thumbnailer if building an XFCE plugin.
IF(BUILD_XFCE OR BUILD_XFCE3)
	SET(BUILD_THUMBNAILER_DBUS ON CACHE INTERNAL "Build the D-Bus thumbnailer" FORCE)
	ADD_SUBDIRECTORY(thumbnailer-dbus)
ENDIF(BUILD_XFCE OR BUILD_XFCE3)
