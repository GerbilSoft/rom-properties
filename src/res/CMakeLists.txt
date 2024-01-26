# Resources
PROJECT(res)

IF(BUILD_XFCE OR BUILD_GTK3 OR BUILD_GTK4)

# GTK+ resource compilation.
FIND_PACKAGE(GLib2 ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO ${GLIB_MIN_VERSION})
IF(GLib2_FOUND AND GIO_FOUND)

# GLib resources.
INCLUDE(GlibCompileResourcesSupport)
SET(RESOURCE_LIST
	flags/flags-16x16.png
	flags/flags-24x24.png
	flags/flags-32x32.png
	ach/ach-16x16.png
	ach/ach-24x24.png
	ach/ach-32x32.png
	ach/ach-64x64.png
	ach/ach-gray-16x16.png
	ach/ach-gray-24x24.png
	ach/ach-gray-32x32.png
	ach/ach-gray-64x64.png
	)
COMPILE_GRESOURCES(GLIB_RESOURCE_SRC
	XML_OUT
	TYPE EMBED_C
	TARGET glibresources.c
	PREFIX "/com/gerbilsoft/rom-properties"
	NO_COMPRESS_ALL
	RESOURCES ${RESOURCE_LIST}
	)
COMPILE_GRESOURCES(GLIB_RESOURCE_H
	XML_OUT
	TYPE EMBED_H
	TARGET glibresources.h
	PREFIX "/com/gerbilsoft/rom-properties"
	NO_COMPRESS_ALL
	RESOURCES ${RESOURCE_LIST}
	)

ADD_LIBRARY(glibresources STATIC ${GLIB_RESOURCE_SRC} ${GLIB_RESOURCE_H})
SET_TARGET_PROPERTIES(glibresources PROPERTIES EXCLUDE_FROM_ALL TRUE)
TARGET_LINK_LIBRARIES(glibresources PRIVATE GLib2::glib GLib2::gio)
TARGET_INCLUDE_DIRECTORIES(glibresources
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

ENDIF(GLib2_FOUND AND GIO_FOUND)

ENDIF(BUILD_XFCE OR BUILD_GTK3 OR BUILD_GTK4)
