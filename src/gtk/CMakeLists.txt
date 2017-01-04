PROJECT(gtk)

# GTK+ frontends for rom-properties.

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)

# Sources and headers.
SET(rom-properties-gtk_SRCS
	RomDataView.cpp
	GdkImageConv.cpp
	)
SET(rom-properties-gtk_H
	RomDataView.hpp
	GdkImageConv.hpp
	)

# Common libraries required for both GTK+ 2.x and 3.x.
FIND_PACKAGE(GLib2)
FIND_PACKAGE(GdkPixbuf2)
IF(NOT GLIB2_FOUND OR NOT GDKPIXBUF2_FOUND)
	# Cannot build any GTK+ frontends.
	SET(BUILD_GTK2 OFF)
	SET(BUILD_GTK3 OFF)
ENDIF(NOT GLIB2_FOUND OR NOT GDKPIXBUF2_FOUND)

# Build GTK+ 2.x frontends.
IF(BUILD_GTK2)
	# TODO: Minimum GTK+ 2.x version?
	FIND_PACKAGE(GTK2 COMPONENTS gtk)
	IF(GTK2_FOUND)
		ADD_SUBDIRECTORY(xfce)
	ELSE(GTK2_FOUND)
		SET(BUILD_GTK2 OFF)
	ENDIF(GTK2_FOUND)
ENDIF(BUILD_GTK2)

# Build GTK+ 3.x frontends.
IF(BUILD_GTK3)
	# TODO: Minimum GTK+ 3.x version?
	FIND_PACKAGE(GTK3)
	IF(GTK3_FOUND)
		ADD_SUBDIRECTORY(gnome)
	ELSE(GTK3_FOUND)
		SET(BUILD_GTK3 OFF)
	ENDIF(GTK3_FOUND)
ENDIF(BUILD_GTK3)
