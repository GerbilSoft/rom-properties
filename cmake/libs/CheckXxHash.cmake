# Check for libxxhash.
# If USE_XXHASH = ON or AUTO, check for the library.
# If not found, fail if ON, or switch to OFF if AUTO.
IF(USE_XXHASH)
	IF(USE_XXHASH STREQUAL "ON")
		SET(XXHASH_REQUIRED "REQUIRED")
	ELSE()
		UNSET(XXHASH_REQUIRED)
	ENDIF()

	FIND_PACKAGE(XxHash ${XXHASH_REQUIRED})
	IF(NOT XxHash_FOUND)
		SET(USE_XXHASH OFF CACHE STRING "Use xxHash for std::hash<>. (System library only!)")
	ENDIF(NOT XxHash_FOUND)

	SET(HAVE_XXHASH ${XxHash_FOUND})
ENDIF(USE_XXHASH)
