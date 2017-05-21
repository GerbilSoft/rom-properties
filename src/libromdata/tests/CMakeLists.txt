PROJECT(libromdata-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

IF(ENABLE_DECRYPTION)
	# CtrKeyScrambler test.
	ADD_EXECUTABLE(CtrKeyScramblerTest
		../../librpbase/tests/gtest_init.cpp
		CtrKeyScramblerTest.cpp
		)
	TARGET_LINK_LIBRARIES(CtrKeyScramblerTest rpbase8 romdata8)
	TARGET_LINK_LIBRARIES(CtrKeyScramblerTest gtest)
	DO_SPLIT_DEBUG(CtrKeyScramblerTest)
	SET_WINDOWS_SUBSYSTEM(CtrKeyScramblerTest CONSOLE)
	ADD_TEST(NAME CtrKeyScramblerTest COMMAND CtrKeyScramblerTest)
ENDIF(ENABLE_DECRYPTION)

# GcnFstPrint. (Not a test, but a useful program.)
ADD_EXECUTABLE(GcnFstPrint
	../../librpbase/tests/gtest_init.cpp
	disc/FstPrint.cpp
	disc/FstPrint.hpp
	disc/GcnFstPrint.cpp
	)
TARGET_LINK_LIBRARIES(GcnFstPrint rpbase8 romdata8)
DO_SPLIT_DEBUG(GcnFstPrint)
SET_WINDOWS_SUBSYSTEM(GcnFstPrint CONSOLE)

# GcnFstTest.
ADD_EXECUTABLE(GcnFstTest
	../../librpbase/tests/gtest_init.cpp
	disc/FstPrint.cpp
	disc/FstPrint.hpp
	disc/GcnFstTest.cpp
	)
TARGET_LINK_LIBRARIES(GcnFstTest rpbase8 romdata8)
TARGET_LINK_LIBRARIES(GcnFstTest gtest)
TARGET_LINK_LIBRARIES(GcnFstTest minizip ${ZLIB_LIBRARY})
DO_SPLIT_DEBUG(GcnFstTest)
SET_WINDOWS_SUBSYSTEM(GcnFstTest CONSOLE)
ADD_TEST(NAME GcnFstTest COMMAND GcnFstTest)

# Copy the reference FSTs to:
# - bin/fst_data/ (TODO: Subdirectory?)
# - ${CMAKE_CURRENT_BINARY_DIR}/fst_data/
# NOTE: Although the test executable is in bin/, CTest still
# uses ${CMAKE_CURRENT_BINARY_DIR} as the working directory.
# Hence, we have to copy the files to both places.
FILE(GLOB test_fsts RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/disc/fst_data" disc/fst_data/*.zip)
FOREACH(test_fst ${test_fsts})
	ADD_CUSTOM_COMMAND(TARGET GcnFstTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/disc/fst_data/${test_fst}"
			"$<TARGET_FILE_DIR:GcnFstTest>/fst_data/${test_fst}"
		)
	ADD_CUSTOM_COMMAND(TARGET GcnFstTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/disc/fst_data/${test_fst}"
			"${CMAKE_CURRENT_BINARY_DIR}/fst_data/${test_fst}"
		)
ENDFOREACH(test_fst test_fsts)
