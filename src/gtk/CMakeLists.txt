PROJECT(gtk)

# GTK+ frontends for rom-properties.

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)

# CPU-specific and optimized sources.
# TODO: Move arch detection to platform.cmake.
STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch)
IF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")
	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(rom-properties-gtk_IFUNC_SRCS GdkImageConv_ifunc.cpp)
	ENDIF(UNIX AND NOT APPLE)

	# NOTE: SSSE3 flags are set in subprojects, not here.
	SET(rom-properties-gtk_SSSE3_SRCS GdkImageConv_ssse3.cpp)
ENDIF()
UNSET(arch)

# Sources and headers.
SET(rom-properties-gtk_SRCS
	RomDataView.cpp
	GdkImageConv.cpp
	CreateThumbnail.cpp
	)
SET(rom-properties-gtk_H
	RomDataView.hpp
	GdkImageConv.hpp
	)

# Common libraries required for both GTK+ 2.x and 3.x.
FIND_PACKAGE(GLib2 2.26.0)
FIND_PACKAGE(GObject2 2.26.0)
FIND_PACKAGE(GdkPixbuf2)
IF(NOT GLib2_FOUND OR NOT GObject2_FOUND OR NOT GdkPixbuf2_FOUND)
	# Cannot build any GTK+ frontends.
	SET(BUILD_GTK2 OFF CACHE "" INTERNAL FORCE)
	SET(BUILD_GTK3 OFF CACHE "" INTERNAL FORCE)
	SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
	SET(BUILD_GNOME OFF CACHE "" INTERNAL FORCE)
ENDIF(NOT GLib2_FOUND OR NOT GObject2_FOUND OR NOT GdkPixbuf2_FOUND)

# Build GTK+ 2.x frontends.
IF(BUILD_GTK2)
	# GTK2 minimum is 2.0.0, so no version check is needed.
	FIND_PACKAGE(GTK2 COMPONENTS gtk)
	IF(GTK2_FOUND)
		ADD_SUBDIRECTORY(xfce)
	ELSE(GTK2_FOUND)
		SET(BUILD_GTK2 OFF CACHE "" INTERNAL FORCE)
		SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
	ENDIF(GTK2_FOUND)
ELSE(BUILD_GTK2)
	# No GTK2 builds.
	SET(BUILD_XFCE OFF CACHE "" INTERNAL FORCE)
ENDIF(BUILD_GTK2)

# Build GTK+ 3.x frontends.
IF(BUILD_GTK3)
	# GTK3 minimum is 3.0.0, so no version check is needed.
	FIND_PACKAGE(GTK3)
	IF(GTK3_FOUND)
		ADD_SUBDIRECTORY(gnome)
	ELSE(GTK3_FOUND)
		SET(BUILD_GTK3 OFF CACHE "" INTERNAL FORCE)
		SET(BUILD_GNOME OFF CACHE "" INTERNAL FORCE)
	ENDIF(GTK3_FOUND)
ELSE(BUILD_GTK3)
	# No GTK3 builds.
	SET(BUILD_GNOME OFF CACHE "" INTERNAL FORCE)
ENDIF(BUILD_GTK3)
