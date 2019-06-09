# Check for TinyXML2.
# If TinyXML2 isn't found, extlib/tinyxml2/ will be used instead.

IF(ENABLE_XML)

IF(NOT USE_INTERNAL_XML)
	IF(TinyXML2_LIBRARY MATCHES "^tinyxml2$" OR TinyXML2_LIBRARY MATCHES "^tinyxml2_static$")
		# Internal TinyXML2 was previously in use.
		UNSET(XML_FOUND)
		UNSET(HAVE_XML)
		UNSET(TinyXML2_LIBRARY CACHE)
	ENDIF()

	# Check for TinyXML2.
	FIND_PACKAGE(TinyXML2)
	IF(TinyXML2_FOUND)
		# Found system TinyXML2.
		SET(HAVE_XML 1)
	ELSE()
		# System TinyXML2 was not found.
		MESSAGE(STATUS "Using the internal copy of TinyXML2 since a system version was not found.")
		SET(USE_INTERNAL_XML ON CACHE BOOL "Use the internal copy of TinyXML2" FORCE)
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of TinyXML2.")
ENDIF(NOT USE_INTERNAL_XML)

IF(USE_INTERNAL_XML)
	# Using the internal TinyXML2 library.
	# NOTE: The tinyxml2 target has implicit include directories,
	# so we don't need to set the variables.
	SET(TinyXML2_FOUND 1)
	SET(HAVE_XML 1)
	IF(WIN32)
		# Using DLLs on Windows.
		# TODO: Build a dylib for Mac OS X.
		SET(USE_INTERNAL_XML_DLL ON)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_XML_DLL OFF)
	ENDIF()
	# TinyXML2 v7.0.0's CMakeLists.txt uses the same target for
	# both DLL and static library builds.
	SET(TinyXML2_LIBRARY tinyxml2 CACHE INTERNAL "TinyXML2 library" FORCE)
	SET(TinyXML2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extlib/tinyxml2")
ELSE(USE_INTERNAL_XML)
	SET(USE_INTERNAL_XML_DLL OFF)
ENDIF(USE_INTERNAL_XML)

ELSE(ENABLE_XML)
	# Disable TinyXML2.
	UNSET(TinyXML2_FOUND)
	UNSET(HAVE_XML)
	UNSET(USE_INTERNAL_XML)
	UNSET(USE_INTERNAL_XML_DLL)
	UNSET(TinyXML2_LIBRARY)
ENDIF(ENABLE_XML)
