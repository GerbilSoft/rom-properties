# KDE UI frontend test suite (KF6)
PROJECT(kf6-tests LANGUAGES CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../../..)

# KDE source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

INCLUDE(RP_FindQt6andKF6)
FIND_QT6_AND_KF6()

IF(BUILD_KF6)

### ListDataSortProxyModel tests

STRING(REGEX REPLACE "([^;]+)" "../\\1" ListDataSortProxyModelTest_kf6_SRCS "${ListDataSortProxyModelTest_SRCS}")

ADD_EXECUTABLE(ListDataSortProxyModelTest_kf6
	${ListDataSortProxyModelTest_kf6_SRCS}
	${ListDataSortProxyModelTest_kf6_H}
	)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf6 PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf6 PRIVATE rom-properties-kf6_LIB)
LIST(APPEND KF6_TESTS ListDataSortProxyModelTest_kf6)

### RomDataFormat tests

STRING(REGEX REPLACE "([^;]+)" "../\\1" RomDataFormatTest_kf6_SRCS "${RomDataFormatTest_SRCS}")

ADD_EXECUTABLE(RomDataFormatTest_kf6
	${RomDataFormatTest_kf6_SRCS}
	${RomDataFormatTest_kf6_H}
	)
TARGET_LINK_LIBRARIES(RomDataFormatTest_kf6 PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(RomDataFormatTest_kf6 PRIVATE rom-properties-kf6_LIB)
# Include directories required for stdafx.h.
TARGET_INCLUDE_DIRECTORIES(RomDataFormatTest_kf6
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../../../librptext"
	)
LIST(APPEND KF6_TESTS RomDataFormatTest_kf6)

### RomDataFormat tests

STRING(REGEX REPLACE "([^;]+)" "../\\1" RomDataViewTest_kf6_SRCS "${RomDataViewTest_SRCS}")

ADD_EXECUTABLE(RomDataViewTest_kf6 ${RomDataViewTest_kf6_SRCS})
SET(RomDataViewTest_kf6_XVFB ON)
TARGET_LINK_LIBRARIES(RomDataViewTest_kf6 PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(RomDataViewTest_kf6 PRIVATE rom-properties-kf6_LIB)
# Include directories required for stdafx.h.
TARGET_INCLUDE_DIRECTORIES(RomDataViewTest_kf6
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../../../librptext"
	)
LIST(APPEND KF6_TESTS RomDataViewTest_kf6)

###### Add tests to CTest ######

FOREACH(_target ${KF6_TESTS})
	DO_SPLIT_DEBUG(${_target})
	SET_WINDOWS_SUBSYSTEM(${_target} CONSOLE)
	SET_WINDOWS_ENTRYPOINT(${_target} wmain OFF)

	IF(USE_XVFB AND ${_target}_XVFB)
		# Use Xvfb for this test.
		#ADD_TEST(NAME ${_target} COMMAND ${XVFB_RUN} -a $<TARGET_FILE:${_target}> --gtest_brief)
		#SET_PROPERTY(TEST ${_target} PROPERTY ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;DISPLAY=;WAYLAND_DISPLAY=")
	ELSE(USE_XVFB AND ${_target}_XVFB)
		# Xvfb is not required.
		ADD_TEST(NAME ${_target} COMMAND ${_target} --gtest_brief)
	ENDIF(USE_XVFB AND ${_target}_XVFB)
ENDFOREACH(_target)

ENDIF(BUILD_KF6)
