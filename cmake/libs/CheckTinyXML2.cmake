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
		SET(USE_INTERNAL_XML ON CACHE STRING "Use the internal copy of TinyXML2." FORCE)
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
	IF(WIN32 OR APPLE)
		# Using DLLs on Windows and Mac OS X.
		# NOTE: libjpeg-turbo 1.5.1's CMakeLists only builds
		# a DLL version of turbojpeg, not libjpeg.
		SET(USE_INTERNAL_XML_DLL ON)
		SET(TinyXML2_LIBRARY tinyxml2 CACHE "TinyXML2 library." INTERNAL FORCE)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_XML_DLL OFF)
		SET(TinyXML2_LIBRARY tinyxml2_static CACHE "TinyXML2 library." INTERNAL FORCE)
	ENDIF()
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
