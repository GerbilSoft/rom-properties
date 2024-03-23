#!/bin/sh

### glib-mkenums script
###
### We can't easily integrate glib-mkenums into the build system, since
### the root GTK+ directory doesn't actually build any targets.
###
### This script will be run whenever enums are updated, and the resulting
### files will be committed to git.
###
### References:
### - https://arosenfeld.wordpress.com/2010/08/11/glib-mkenums/
### - https://github.com/Kurento/kms-cmake-utils/blob/master/CMake/FindGLIB-MKENUMS.cmake
### - https://github.com/Kurento/kms-cmake-utils/blob/master/CMake/GLibHelpers.cmake
### - https://developer-old.gnome.org/gobject/stable/glib-mkenums.html

name=rp-gtk-enums
name_upper=RP_GTK_ENUMS
shortname=RP

set -ev

ENUM_HEADER_FILES="RomDataView.hpp config/CacheCleaner.hpp ListDataItem.h"

# Header
glib-mkenums \
	--template rp-gtk-enums.template.h \
	${ENUM_HEADER_FILES} > "${name}.h"

# Source
glib-mkenums \
	--template rp-gtk-enums.template.c \
	${ENUM_HEADER_FILES} > "${name}.c"
