# D-Bus Thumbnailer for rom-properties
PROJECT(rp-thumbnailer-dbus LANGUAGES C)

# Find packages.
FIND_PACKAGE(GLib2 ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GObject2 ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GIO-UNIX ${REQUIRE_XFCE} ${GLIB_MIN_VERSION})
FIND_PACKAGE(GdkPixbuf2 ${REQUIRE_XFCE})
IF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)
	# All required libraries were found.
ELSE(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)
	# A required library was not found.
	# Disable the D-Bus Thumbnailer.
	SET(BUILD_THUMBNAILER_DBUS OFF CACHE INTERNAL "Build the D-Bus thumbnailer" FORCE)
ENDIF(GLib2_FOUND AND GObject2_FOUND AND GIO_FOUND AND GIO-UNIX_FOUND)

# Ensure we don't use functions not available in GLib 2.34.
ADD_DEFINITIONS(-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_34 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_34)

# D-Bus bindings for the thumbnailer.
SET(DBUS_XML_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.thumbnails.SpecializedThumbnailer1.xml")
ADD_CUSTOM_COMMAND(
	OUTPUT SpecializedThumbnailer1.c SpecializedThumbnailer1.h
	COMMAND "${GDBUS_CODEGEN}"
		--generate-c-code SpecializedThumbnailer1
		"${DBUS_XML_FILENAME}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	DEPENDS "${DBUS_XML_FILENAME}"
	VERBATIM
	)

# Disable unused parameter warnings in generated D-Bus sources.
INCLUDE(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-Wno-unused-parameter" CFLAG_Wno_unused_parameter)
IF(CFLAG_Wno_unused_parameter)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/SpecializedThumbnailer1.c
		APPEND_STRING PROPERTIES COMPILE_FLAGS " -Wno-unused-parameter ")
ENDIF(CFLAG_Wno_unused_parameter)

SET(${PROJECT_NAME}_SRCS
	rp-thumbnailer-dbus.c
	rp-thumbnailer-main.c
	rptsecure.c
	${CMAKE_CURRENT_BINARY_DIR}/SpecializedThumbnailer1.c
	)
SET(${PROJECT_NAME}_H
	rp-thumbnailer-dbus.h
	rptsecure.h
	${CMAKE_CURRENT_BINARY_DIR}/SpecializedThumbnailer1.h
	)

# Process the .service file.
INCLUDE(DirInstallPaths)
CONFIGURE_FILE(com.gerbilsoft.rom-properties.SpecializedThumbnailer1.service.in
	com.gerbilsoft.rom-properties.SpecializedThumbnailer1.service)

IF(BUILD_THUMBNAILER_DBUS)
	# D-Bus thumbnailer.
	ADD_EXECUTABLE(${PROJECT_NAME}
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
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} unixcommon rpsecure)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE G_LOG_DOMAIN=\"${PROJECT_NAME}\")

ENDIF(BUILD_THUMBNAILER_DBUS)

##########################################
# Install the thumbnailer D-Bus service. #
##########################################

IF(BUILD_THUMBNAILER_DBUS)
	# FIXME: ${ThunarX2_EXTENSIONS_DIR} always uses the system prefix.
	# This should be adjusted to ${CMAKE_INSTALL_PREFIX}.
	INSTALL(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
		COMPONENT "plugin"
		)
	# TODO: Use session_bus_services_dir from dbus-1.pc.
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/com.gerbilsoft.rom-properties.SpecializedThumbnailer1.service"
		DESTINATION "share/dbus-1/services"
		COMPONENT "plugin"
		)

	# NOTE: This file must be installed with a ".service" extension.
	# NOTE 2: On Ubuntu, this must be installed in share/xfce4/thumbnailers,
	# since Ubuntu ends up listing /usr/share/ in $XDG_DATA_DIRS twice,
	# which causes tumblerd to load /usr/share/thumbnailers/ twice and
	# fail an assertion.

	# Detect Ubuntu.
	# Reference: https://stackoverflow.com/questions/26919334/detect-underlying-platform-flavour-in-cmake
	FIND_PROGRAM(LSB_RELEASE lsb_release)
	IF(LSB_RELEASE)
		EXECUTE_PROCESS(COMMAND ${LSB_RELEASE} -is
			OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			)
	ENDIF(LSB_RELEASE)
	IF(LSB_RELEASE_ID_SHORT STREQUAL Ubuntu)
		SET(THUMBNAIL_DEST_DIR "share/xfce4/thumbnailers")
	ELSE()
		SET(THUMBNAIL_DEST_DIR "share/thumbnailers")
	ENDIF()

	# Process the .thumbnailer file.
	INCLUDE(ParseMimeTypes)
	PARSE_MIME_TYPES(MIMETYPES_THUMBONLY "${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types")
	CONFIGURE_FILE(com.gerbilsoft.rom-properties.SpecializedThumbnailer1.thumbnailer.in
	               com.gerbilsoft.rom-properties.SpecializedThumbnailer1.thumbnailer @ONLY)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/com.gerbilsoft.rom-properties.SpecializedThumbnailer1.thumbnailer"
		RENAME com.gerbilsoft.rom-properties.SpecializedThumbnailer1.service
		DESTINATION "${THUMBNAIL_DEST_DIR}"
		COMPONENT "plugin"
		)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_THUMBNAILER_DBUS)
