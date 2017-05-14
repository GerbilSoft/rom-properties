IF(BIICODE)
	ADD_BIICODE_TARGETS()
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    ENDIF()
    RETURN()
ENDIF(BIICODE)
# rom-properties: Disabled; use the main project policies.
#cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
#cmake_policy(VERSION 2.6)

project(tinyxml2)
# rom-properties: Disabled.
#include(GNUInstallDirs)
#enable_testing()

#CMAKE_BUILD_TOOL

################################
# set lib version here

set(GENERIC_LIB_VERSION "4.0.1")
set(GENERIC_LIB_SOVERSION "4")

# rom-properties: to distinguish between debug and release lib
set(CMAKE_DEBUG_POSTFIX "d")


################################
# Add common source

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/.")

################################
# Add custom target to copy all data

# rom-properties: Disabled.
IF(0)
set(TARGET_DATA_COPY DATA_COPY)
set(DATA_COPY_FILES)
if(NOT ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
	foreach(data dream.xml empty.xml utf8test.xml utf8testverify.xml)
		set(DATA_COPY_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/resources/${data})
		set(DATA_COPY_DEST ${CMAKE_CURRENT_BINARY_DIR}/resources/${data})
		add_custom_command(
			OUTPUT ${DATA_COPY_DEST}
			COMMAND ${CMAKE_COMMAND}
			ARGS -E copy ${DATA_COPY_SRC} ${DATA_COPY_DEST}
			DEPENDS ${DATA_COPY_SRC})
		list(APPEND DATA_COPY_FILES ${DATA_COPY_DEST})
	endforeach(data)
endif(NOT ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(${TARGET_DATA_COPY} DEPENDS ${DATA_COPY_FILES})
ENDIF(0) # rom-properties

################################
# Add definitions

# rom-properties: These are already set in the platform configuration.
IF(0)
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
ENDIF(0) # rom-properties

################################
# Add targets
# By Default shared libray is being built
# To build static libs also - Do cmake . -DBUILD_STATIC_LIBS:BOOL=ON
# User can choose not to build shared library by using cmake -BUILD_SHARED_LIBS:BOOL:OFF
# To build only static libs use cmake . -DBUILD_SHARED_LIBS:BOOL=OFF -DBUILD_STATIC_LIBS:BOOL=ON

# rom-properties: Don't use option() here; it's set by extlib.
IF(0)
option(BUILD_SHARED_LIBS "build as shared library" ON)
option(BUILD_STATIC_LIBS "build as static library" OFF)
ENDIF(0) # rom-properties

if(BUILD_SHARED_LIBS)
add_library(tinyxml2 SHARED tinyxml2.cpp tinyxml2.h)

set_target_properties(tinyxml2 PROPERTIES
	VERSION "${GENERIC_LIB_VERSION}"
	SOVERSION "${GENERIC_LIB_SOVERSION}")
# rom-properties: Set TINYXML2_IMPORT when linking to TinyXML2.
TARGET_COMPILE_DEFINITIONS(tinyxml2
	PRIVATE "TINYXML2_EXPORT"
	INTERFACE "TINYXML2_IMPORT")

if(DEFINED CMAKE_VERSION AND NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.11")
    target_include_directories(tinyxml2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/.")
endif()

# rom-properties:
# - Disabled installation of import libraries.
# - Use the correct RUNTIME destination.
# - Install PDB files.
#install(TARGETS tinyxml2
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS tinyxml2
	RUNTIME DESTINATION ${INSTALL_BIN_DIR}
	#LIBRARY DESTINATION ${INSTALL_LIB_DIR}
	#ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
	FRAMEWORK DESTINATION ${INSTALL_LIB_DIR})
# rom-properties: Split debug functionality; Windows subsystem.
DO_SPLIT_DEBUG(tinyxml2)
SET_WINDOWS_SUBSYSTEM(tinyxml2 WIN32)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	INCLUDE(DirInstallPaths)
	# TODO: Test on MinGW.
	# FIXME: Generator PDB expression doesn't have the debug postfix for some reason,
	# so cpack fails in debug builds if we get the PDB property.
	SET(PDB_FILENAME_D "$<TARGET_FILE_DIR:tinyxml2>/tinyxml2${CMAKE_DEBUG_POSTFIX}.pdb")
	SET(PDB_FILENAME_R "$<TARGET_FILE_DIR:tinyxml2>/tinyxml2${CMAKE_RELEASE_POSTFIX}.pdb")
	INSTALL(FILES "${PDB_FILENAME_D}" "${PDB_FILENAME_R}"
		DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
		COMPONENT "debug"
		OPTIONAL
		)
	UNSET(PDB_FILENAME_D)
	UNSET(PDB_FILENAME_R)
ENDIF(INSTALL_DEBUG)
endif()

if(BUILD_STATIC_LIBS)
add_library(tinyxml2_static STATIC tinyxml2.cpp tinyxml2.h)
set_target_properties(tinyxml2_static PROPERTIES
        VERSION "${GENERIC_LIB_VERSION}"
        SOVERSION "${GENERIC_LIB_SOVERSION}")
set_target_properties( tinyxml2_static PROPERTIES OUTPUT_NAME tinyxml2 )
# rom-properties: Set the MSVC debug path.
SET_MSVC_DEBUG_PATH(tinyxml2_static)

if(DEFINED CMAKE_VERSION AND NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.11")
    target_include_directories(tinyxml2_static INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/.")
endif()

# rom-properties: Don't install static libraries.
IF(0)
install(TARGETS tinyxml2_static
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDIF(0) # rom-properties
endif()

# rom-properties: Don't build xmltest or install anything else.
IF(0)
add_executable(xmltest xmltest.cpp)
if(BUILD_SHARED_LIBS)
   add_dependencies(xmltest tinyxml2)
   add_dependencies(xmltest ${TARGET_DATA_COPY})
   target_link_libraries(xmltest tinyxml2)
else(BUILD_STATIC_LIBS)
   add_dependencies(xmltest tinyxml2_static)
   add_dependencies(xmltest ${TARGET_DATA_COPY})
   target_link_libraries(xmltest tinyxml2_static)
endif()
install(TARGETS DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES tinyxml2.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

foreach(p LIB INCLUDE)
	set(var CMAKE_INSTALL_${p}DIR)
	if(NOT IS_ABSOLUTE "${${var}}")
		set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
	endif()
endforeach()

configure_file(tinyxml2.pc.in tinyxml2.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tinyxml2.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

#add_test(xmltest ${SAMPLE_NAME} COMMAND $<TARGET_FILE:${SAMPLE_NAME}>)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
ENDIF(0) # rom-properties
