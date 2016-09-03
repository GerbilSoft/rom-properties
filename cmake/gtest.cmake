# Macros for enabling unit testing in Gens/GS II.

# Enable testing.
INCLUDE(CTest)

# Google Test is bundled in extlib.
# We're always using the bundled version.
IF(BUILD_TESTING)
	SET(GTEST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extlib/googletest/googletest/include")
	SET(GTEST_LIBRARY gtest)
	SET(GMOCK_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extlib/googletest/googlemock/include")
	SET(GMOCK_LIBRARY gmock)
ENDIF(BUILD_TESTING)
