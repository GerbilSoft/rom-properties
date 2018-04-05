PROJECT(rom-properties-kde4)

# Find Qt4.
SET(QT4_NO_LINK_QTMAIN 1)
FIND_PACKAGE(Qt4 4.6.0 COMPONENTS QtCore QtGui)
IF(QT4_FOUND)
	# Make sure KDE4's CMake files don't create an uninstall rule.
	SET(_kde4_uninstall_rule_created TRUE)

	# Find KDE4. (TODO: Version?)
	FIND_PACKAGE(KDE4)
	IF(NOT KDE4_FOUND)
		# KDE4 not found.
		SET(BUILD_KDE4 OFF CACHE "" INTERNAL FORCE)
	ENDIF(NOT KDE4_FOUND)

	# Get rid of the explicit C90 setting.
	STRING(REPLACE "-std=iso9899:1990" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
ELSE(QT4_FOUND)
	# Qt4 not found.
	SET(BUILD_KDE4 OFF CACHE "" INTERNAL FORCE)
ENDIF(QT4_FOUND)

# Sources and headers. (common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_H "${rom-properties-kde_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_UIS "${rom-properties-kde_UIS}")

# Sources and headers. (KDE4-specific)
SET(rom-properties-kde4_SRCS
	${rom-properties-kde4_SRCS}
	RomPropertiesDialogPluginFactoryKDE4.cpp
	)

IF(COMMAND QT4_WRAP_UI)
	# Wrap UI files.
	QT4_WRAP_UI(rom-properties-kde4_UIS_H ${rom-properties-kde4_UIS})
	ADD_CUSTOM_TARGET(kde4_generate_ui_h DEPENDS ${rom-properties-kde4_UIS_H})
ENDIF(COMMAND QT4_WRAP_UI)

#####################
# Build the plugin. #
#####################

IF(BUILD_KDE4)
	KDE4_ADD_PLUGIN(rom-properties-kde4
		${rom-properties-kde4_SRCS}
		${rom-properties-kde4_H}
		${rom-properties-kde4_UIS_H}
		)
	DO_SPLIT_DEBUG(rom-properties-kde4)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kde4
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kde4
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(rom-properties-kde4 cachemgr romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-kde4 i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-kde4 ${ZLIB_LIBRARY})
	TARGET_LINK_LIBRARIES(rom-properties-kde4 ${KDE4_KFILE_LIBRARY})
	TARGET_LINK_LIBRARIES(rom-properties-kde4 Qt4::QtGui Qt4::QtCore)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PUBLIC ${KDE4_INCLUDE_DIR})

	# Add zlib and png directories for the About tab.
	IF(HAVE_ZLIB)
		TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PRIVATE ${ZLIB_INCLUDE_DIRS})
	ENDIF(HAVE_ZLIB)
	IF(HAVE_PNG)
		TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PRIVATE ${PNG_INCLUDE_DIRS})
	ENDIF(HAVE_PNG)
ENDIF(BUILD_KDE4)

# Define -DQT_NO_DEBUG in release builds.
SET(CMAKE_C_FLAGS_RELEASE   "-DQT_NO_DEBUG ${CMAKE_C_FLAGS_RELEASE}")
SET(CMAKE_CXX_FLAGS_RELEASE "-DQT_NO_DEBUG ${CMAKE_CXX_FLAGS_RELEASE}")

#######################
# Install the plugin. #
#######################

IF(BUILD_KDE4)
	# FIXME: KDE4's installation directories have the prefix
	# hard-coded. This should be removed.
	INSTALL(TARGETS rom-properties-kde4
		LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom-properties-kde4.desktop"
		DESTINATION "${SERVICES_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca4.

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-kde4 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE4)
