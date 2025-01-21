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
STRING(REGEX REPLACE "([^;]+)" "../\\1" ListDataSortProxyModelTest_kf5_H    "${ListDataSortProxyModelTest_H}")

ADD_EXECUTABLE(ListDataSortProxyModelTest_kf5
	${ListDataSortProxyModelTest_kf5_SRCS}
	${ListDataSortProxyModelTest_kf5_H}
	)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf5 PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf5 PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core)
IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf5 PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)
LIST(APPEND KF5_TESTS ListDataSortProxyModelTest_kf5)

### RomDataFormat tests

STRING(REGEX REPLACE "([^;]+)" "../\\1" RomDataFormatTest_kf5_SRCS "${RomDataFormatTest_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" RomDataFormatTest_kf5_H    "${RomDataFormatTest_H}")

ADD_EXECUTABLE(RomDataFormatTest_kf5
	${RomDataFormatTest_kf5_SRCS}
	${RomDataFormatTest_kf5_H}
	)
TARGET_LINK_LIBRARIES(RomDataFormatTest_kf5 PRIVATE rptest romdata)
# NOTE: QtWidgets is still needed for stdafx.h.
TARGET_LINK_LIBRARIES(RomDataFormatTest_kf5 PUBLIC Qt5::Widgets Qt5::Core)
IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(RomDataFormatTest_kf5 PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)
# Include directories required for stdafx.h.
TARGET_INCLUDE_DIRECTORIES(RomDataFormatTest_kf5
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../../../librptext"
	)
LIST(APPEND KF5_TESTS RomDataFormatTest_kf5)

###### Add tests to CTest ######

FOREACH(_target ${KF5_TESTS})
	DO_SPLIT_DEBUG(${_target})
	SET_WINDOWS_SUBSYSTEM(${_target} CONSOLE)
	SET_WINDOWS_ENTRYPOINT(${_target} wmain OFF)
	ADD_TEST(NAME ${_target} COMMAND ${_target} --gtest_brief)
ENDFOREACH(_target)

ENDIF(BUILD_KF5)
