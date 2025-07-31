# KDE UI frontend test suite (KF5)
PROJECT(kf5-tests LANGUAGES CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../../..)

# KDE source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

INCLUDE(RP_FindQt5andKF5)
FIND_QT5_AND_KF5()

IF(BUILD_KF5)

### ListDataSortProxyModel tests

STRING(REGEX REPLACE "([^;]+)" "../\\1" ListDataSortProxyModelTest_kf5_SRCS "${ListDataSortProxyModelTest_SRCS}")

ADD_EXECUTABLE(ListDataSortProxyModelTest_kf5
	${ListDataSortProxyModelTest_kf5_SRCS}
	${ListDataSortProxyModelTest_kf5_H}
	)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf5 PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf5 PRIVATE rom-properties-kf5_LIB)
LIST(APPEND KF5_TESTS ListDataSortProxyModelTest_kf5)

### RomDataFormat tests

STRING(REGEX REPLACE "([^;]+)" "../\\1" RomDataFormatTest_kf5_SRCS "${RomDataFormatTest_SRCS}")

ADD_EXECUTABLE(RomDataFormatTest_kf5
	${RomDataFormatTest_kf5_SRCS}
	${RomDataFormatTest_kf5_H}
	)
TARGET_LINK_LIBRARIES(RomDataFormatTest_kf5 PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(RomDataFormatTest_kf5 PRIVATE rom-properties-kf5_LIB)
# Include directories required for stdafx.h.
TARGET_INCLUDE_DIRECTORIES(RomDataFormatTest_kf5
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../../../librptext"
	)
LIST(APPEND KF5_TESTS RomDataFormatTest_kf5)

### RomDataView tests

STRING(REGEX REPLACE "([^;]+)" "../\\1" RomDataViewTest_kf5_SRCS "${RomDataViewTest_SRCS}")

ADD_EXECUTABLE(RomDataViewTest_kf5
	${RomDataViewTest_kf5_SRCS}
	${RomDataViewTest_kf5_H}
	)
TARGET_LINK_LIBRARIES(RomDataViewTest_kf5 PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(RomDataViewTest_kf5 PRIVATE rom-properties-kf5_LIB)
# Include directories required for stdafx.h.
TARGET_INCLUDE_DIRECTORIES(RomDataViewTest_kf5
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../../../librptext"
	)
LIST(APPEND KF5_TESTS RomDataViewTest_kf5)

###### Add tests to CTest ######

FOREACH(_target ${KF5_TESTS})
	DO_SPLIT_DEBUG(${_target})
	SET_WINDOWS_SUBSYSTEM(${_target} CONSOLE)
	SET_WINDOWS_ENTRYPOINT(${_target} wmain OFF)
	ADD_TEST(NAME ${_target} COMMAND ${_target} --gtest_brief)
ENDFOREACH(_target)

ENDIF(BUILD_KF5)
