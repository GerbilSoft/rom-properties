PROJECT(gtk)

# GTK+ frontends for rom-properties.

# TODO: Minimum GTK+ 2.x version?
FIND_PACKAGE(GTK2 COMPONENTS gtk)

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

IF(GTK2_FOUND)
	# Build GTK2 frontends.
	ADD_SUBDIRECTORY(xfce)
ENDIF(GTK2_FOUND)
