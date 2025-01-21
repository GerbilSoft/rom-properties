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
STRING(REGEX REPLACE "([^;]+)" "../\\1" ListDataSortProxyModelTest_kf6_H    "${ListDataSortProxyModelTest_H}")

ADD_EXECUTABLE(ListDataSortProxyModelTest_kf6
	${ListDataSortProxyModelTest_kf6_SRCS}
	${ListDataSortProxyModelTest_kf6_H}
	)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf6 PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf6 PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core)
IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf6 PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)
LIST(APPEND KF6_TESTS ListDataSortProxyModelTest_kf6)

### RomDataFormat tests

STRING(REGEX REPLACE "([^;]+)" "../\\1" RomDataFormatTest_kf6_SRCS "${RomDataFormatTest_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" RomDataFormatTest_kf6_H    "${RomDataFormatTest_H}")

ADD_EXECUTABLE(RomDataFormatTest_kf6
	${RomDataFormatTest_kf6_SRCS}
	${RomDataFormatTest_kf6_H}
	)
TARGET_LINK_LIBRARIES(RomDataFormatTest_kf6 PRIVATE rptest romdata)
# NOTE: QtWidgets is still needed for stdafx.h.
TARGET_LINK_LIBRARIES(RomDataFormatTest_kf6 PUBLIC Qt6::Widgets Qt6::Core)
IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(RomDataFormatTest_kf6 PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)
# Include directories required for stdafx.h.
TARGET_INCLUDE_DIRECTORIES(RomDataFormatTest_kf6
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../../../librptext"
	)
LIST(APPEND KF5_TESTS RomDataFormatTest_kf6)

###### Add tests to CTest ######

FOREACH(_target ${KF6_TESTS})
	DO_SPLIT_DEBUG(${_target})
	SET_WINDOWS_SUBSYSTEM(${_target} CONSOLE)
	SET_WINDOWS_ENTRYPOINT(${_target} wmain OFF)
	ADD_TEST(NAME ${_target} COMMAND ${_target} --gtest_brief)
ENDFOREACH(_target)

ENDIF(BUILD_KF6)
