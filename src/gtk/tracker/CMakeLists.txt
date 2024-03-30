# D-Bus Thumbnailer for rom-properties
PROJECT(extract-rom-properties LANGUAGES CXX)

# Find packages
FIND_PACKAGE(GLib2 ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GObject2 ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO-UNIX ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)
	# All required libraries were found.
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)
	# A required library was not found.
	# Disable the Tracker extractor.
	SET(BUILD_TRACKER_EXTRACTOR OFF CACHE INTERNAL "Build the Tracker extractor module." FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)

# Ensure we don't use functions not available in GLib 2.34.
ADD_DEFINITIONS(-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_34 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_34)

SET(${PROJECT_NAME}_SRCS
	rp-tracker.cpp
	tracker-mini.c
	)
SET(${PROJECT_NAME}_H
	tracker-mini.h
	tracker-mini-1.0.h
	tracker-mini-2.0.h
	)

IF(BUILD_TRACKER_EXTRACTOR)
	# Tracker extractor module
	# NOTE: Keeping the "lib" prefix by using SHARED instead of MODULE.
	ADD_LIBRARY(${PROJECT_NAME} SHARED
		${${PROJECT_NAME}_SRCS}
		${${PROJECT_NAME}_H}
		)
	DO_SPLIT_DEBUG(${PROJECT_NAME})
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE unixcommon rpsecure)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)
	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME}
		PRIVATE G_LOG_DOMAIN=\"${PROJECT_NAME}\"
		)
ENDIF(BUILD_TRACKER_EXTRACTOR)

#########################################
# Install the tracker extractor module. #
#########################################

IF(0) #BUILD_TRACKER_EXTRACTOR)
	# Rule files
	# - tracker-1.0: share/tracker/extract-rules/
	# - tracker-2.0: share/tracker-miners/extract-rules/
	SET(RULE_FILES
		rules/20-rp-audio.rule
		rules/20-rp-rom-images.rule
		rules/20-rp-textures.rule
		)
	INSTALL(FILES ${RULE_FILES}
		DESTINATION "share/tracker-miners/extract-rules"
		COMPONENT "plugin"
		)

	# Extractor module
	# - tracker-1.0: lib/[arch]/tracker-1.0/extract-modules/libextract-[module].so
	# - tracker-2.0: lib/[arch]/tracker-miners-2.0/extract-modules/libextract-[module].so
ENDIF(0) #BUILD_TRACKER_EXTRACTOR)
