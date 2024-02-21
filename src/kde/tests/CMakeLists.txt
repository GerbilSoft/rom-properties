# KDE UI frontend test suite
PROJECT(kde-tests LANGUAGES CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# KDE source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)

# Find supported Qt/KDE versions.
IF(BUILD_KDE4)
	INCLUDE(RP_FindQt4andKDE4)
	FIND_QT4_AND_KDE4()
ENDIF(BUILD_KDE4)
IF(BUILD_KF5)
	INCLUDE(RP_FindQt5andKF5)
	FIND_QT5_AND_KF5()
ENDIF(BUILD_KF5)
IF(BUILD_KF6)
	INCLUDE(RP_FindQt6andKF6)
	FIND_QT6_AND_KF6()
ENDIF(BUILD_KF6)

### ListDataSortProxyModel test

SET(ListDataSortProxyModelTest_SRCS
	ListDataSortProxyModelTest.cpp
	../ListDataModel.cpp
	../ListDataSortProxyModel.cpp
	../RomDataFormat.cpp
	../RpQImageBackend.cpp
	../RpQUrl.cpp
	../RpQt.cpp
	)
SET(ListDataSortProxyModelTest_H
	../ListDataModel.hpp
	../ListDataSortProxyModel.hpp
	../RomDataFormat.hpp
	../RpQImageBackend.hpp
	../RpQUrl.hpp
	../RpQt.hpp
	)

IF(BUILD_KDE4)
	ADD_EXECUTABLE(ListDataSortProxyModelTest_kde4
		${ListDataSortProxyModelTest_SRCS}
		${ListDataSortProxyModelTest_H}
		)
	TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kde4 PRIVATE rptest romdata)
	TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kde4 PUBLIC Qt4::QtGui Qt4::QtCore)
	LIST(APPEND KDE_TESTS ListDataSortProxyModelTest_kde4)
ENDIF(BUILD_KDE4)
IF(BUILD_KF5)
	ADD_EXECUTABLE(ListDataSortProxyModelTest_kf5
		${ListDataSortProxyModelTest_SRCS}
		${ListDataSortProxyModelTest_H}
		)
	TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf5 PRIVATE rptest romdata)
	TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf5 PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core)
	LIST(APPEND KDE_TESTS ListDataSortProxyModelTest_kf5)
ENDIF(BUILD_KF5)
IF(BUILD_KF6)
	ADD_EXECUTABLE(ListDataSortProxyModelTest_kf6
		${ListDataSortProxyModelTest_SRCS}
		${ListDataSortProxyModelTest_H}
		)
	TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf6 PRIVATE rptest romdata)
	TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest_kf6 PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core)
	LIST(APPEND KDE_TESTS ListDataSortProxyModelTest_kf6)
ENDIF(BUILD_KF6)

FOREACH(_target ${KDE_TESTS})
	DO_SPLIT_DEBUG(${_target})
	SET_WINDOWS_SUBSYSTEM(${_target} CONSOLE)
	SET_WINDOWS_ENTRYPOINT(${_target} wmain OFF)
	ADD_TEST(NAME ${_target} COMMAND ${_target} --gtest_brief)
ENDFOREACH(_target)
