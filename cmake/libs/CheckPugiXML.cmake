# Check for PugiXML.
# If PugiXML isn't found, extlib/pugixml/ will be used instead.

IF(ENABLE_XML)

IF(NOT USE_INTERNAL_XML)
	IF(pugixml_LIBRARY MATCHES "^pugixml")
		# Internal PugiXML was previously in use.
		UNSET(XML_FOUND)
		UNSET(HAVE_XML)
		UNSET(pugixml_LIBRARY CACHE)
	ENDIF()

	# Check for PugiXML.
	FIND_PACKAGE(pugixml)
	IF(pugixml_FOUND)
		# Found system PugiXML.
		SET(HAVE_XML 1)
		# NOTE: PugiXML's CMake configuration files don't set pugixml_LIBRARY.
		SET(pugixml_LIBRARY pugixml::pugixml CACHE INTERNAL "PugiXML library" FORCE)
	ELSE()
		# System PugiXML was not found.
		MESSAGE(STATUS "Using the internal copy of PugiXML since a system version was not found.")
		SET(USE_INTERNAL_XML ON CACHE BOOL "Use the internal copy of PugiXML" FORCE)
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of PugiXML.")
ENDIF(NOT USE_INTERNAL_XML)

IF(USE_INTERNAL_XML)
	# Using the internal PugiXML library.
	# NOTE: The pugixml target has implicit include directories,
	# so we don't need to set the variables.
	SET(pugixml_FOUND 1)
	SET(HAVE_XML 1)
	IF(WIN32)
		# Using DLLs on Windows.
		# TODO: Build a dylib for Mac OS X.
		SET(USE_INTERNAL_XML_DLL ON)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_XML_DLL OFF)
	ENDIF()
	# FIXME: Verify this on v1.10 and older. (Ubuntu 16.04 has v1.7.)
	SET(pugixml_LIBRARY pugixml::pugixml CACHE INTERNAL "PugiXML library" FORCE)
	SET(pugixml_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extlib/pugixml/src")
ELSE(USE_INTERNAL_XML)
	SET(USE_INTERNAL_XML_DLL OFF)
ENDIF(USE_INTERNAL_XML)

ELSE(ENABLE_XML)
	# Disable PugiXML.
	UNSET(pugixml_FOUND)
	UNSET(HAVE_XML)
	UNSET(USE_INTERNAL_XML)
	UNSET(USE_INTERNAL_XML_DLL)
	UNSET(pugixml_LIBRARY)
ENDIF(ENABLE_XML)
