PROJECT(gtk)

# GTK+ frontends for rom-properties.

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)

# Sources and headers.
SET(rom-properties-gtk_SRCS
	RomDataView.cpp
	GdkImageConv.cpp
	CreateThumbnail.cpp
	)
SET(rom-properties-gtk_H
	RomDataView.hpp
	GdkImageConv.hpp
	CreateThumbnail.hpp
	)

# Common libraries required for both GTK+ 2.x and 3.x.
FIND_PACKAGE(GLib2 2.26.0)
FIND_PACKAGE(GObject2 2.26.0)
FIND_PACKAGE(GdkPixbuf2)
IF(NOT GLib2_FOUND OR NOT GObject2_FOUND OR NOT GdkPixbuf2_FOUND)
	# Cannot build any GTK+ frontends.
	SET(BUILD_GTK2 OFF)
	SET(BUILD_GTK3 OFF)
ENDIF(NOT GLib2_FOUND OR NOT GObject2_FOUND OR NOT GdkPixbuf2_FOUND)

# Build GTK+ 2.x frontends.
IF(BUILD_GTK2)
	# GTK2 minimum is 2.0.0, so no version check is needed.
	FIND_PACKAGE(GTK2 COMPONENTS gtk)
	IF(GTK2_FOUND)
		ADD_SUBDIRECTORY(xfce)
	ELSE(GTK2_FOUND)
		SET(BUILD_GTK2 OFF)
	ENDIF(GTK2_FOUND)
ENDIF(BUILD_GTK2)

# Build GTK+ 3.x frontends.
IF(BUILD_GTK3)
	# GTK3 minimum is 3.0.0, so no version check is needed.
	FIND_PACKAGE(GTK3)
	IF(GTK3_FOUND)
		ADD_SUBDIRECTORY(gnome)
	ELSE(GTK3_FOUND)
		SET(BUILD_GTK3 OFF)
	ENDIF(GTK3_FOUND)
ENDIF(BUILD_GTK3)
