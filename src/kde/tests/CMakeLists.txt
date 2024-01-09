# KDE UI frontend test suite
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(kde-tests LANGUAGES CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# KDE source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)

# NOTE: Tests are currently only built for a single Qt version.
# Using Qt5 for now.
# TODO: Build for all selected Qt versions?

# Find Qt5 and KF5.
INCLUDE(RP_FindQt5andKF5)
FIND_QT5_AND_KF5()

IF(BUILD_KF5)

# ListDataSortProxyModel test
ADD_EXECUTABLE(ListDataSortProxyModelTest
	ListDataSortProxyModelTest.cpp
	../ListDataModel.cpp
	../ListDataModel.hpp
	../ListDataSortProxyModel.cpp
	../ListDataSortProxyModel.hpp
	../RomDataFormat.cpp
	../RomDataFormat.hpp
	../RpQImageBackend.cpp
	../RpQImageBackend.hpp
	../RpQUrl.cpp
	../RpQUrl.hpp
	../RpQt.cpp
	../RpQt.hpp
	)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core)
#TARGET_INCLUDE_DIRECTORIES(ListDataSortProxyModelTest PRIVATE ${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
#TARGET_COMPILE_DEFINITIONS(ListDataSortProxyModelTest PRIVATE ${ZLIB_DEFINITIONS} ${PNG_DEFINITIONS})
DO_SPLIT_DEBUG(ListDataSortProxyModelTest)
SET_WINDOWS_SUBSYSTEM(ListDataSortProxyModelTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(ListDataSortProxyModelTest wmain OFF)
ADD_TEST(NAME ListDataSortProxyModelTest COMMAND ListDataSortProxyModelTest --gtest_brief)

ENDIF(BUILD_KF5)
