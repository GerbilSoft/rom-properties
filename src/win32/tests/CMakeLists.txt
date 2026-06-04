# Win32 UI frontend test suite
PROJECT(win32-tests LANGUAGES CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# Win32 source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)

# MSVC: Set /DEPENDENTLOADFLAG:0xA00 for unit tests.
IF(COMMAND SET_DEPENDENT_LOAD_FLAG_GTEST)
	SET_DEPENDENT_LOAD_FLAG_GTEST()
ENDIF(COMMAND SET_DEPENDENT_LOAD_FLAG_GTEST)

### RomDataFormat tests

SET(RomDataFormatTest_SRCS
	RomDataFormatTest.cpp
	../RomDataFormat.cpp
	)
SET(RomDataFormatTest_H
	../RomDataFormat.hpp
	)
ADD_EXECUTABLE(RomDataFormatTest_win32
	${RomDataFormatTest_SRCS}
	${RomDataFormatTest_H}
	)
TARGET_LINK_LIBRARIES(RomDataFormatTest_win32 PRIVATE rptest)
LIST(APPEND WIN32_TESTS RomDataFormatTest_win32)

### Run the tests

FOREACH(_target ${WIN32_TESTS})
	DO_SPLIT_DEBUG(${_target})
	SET_WINDOWS_SUBSYSTEM(${_target} CONSOLE)
	SET_WINDOWS_ENTRYPOINT(${_target} wmain ON)
	ADD_TEST(NAME ${_target} COMMAND ${_target} --gtest_brief)
ENDFOREACH(_target)
