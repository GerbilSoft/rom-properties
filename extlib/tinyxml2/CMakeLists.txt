# rom-properties:
# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS       "${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS     "${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

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
#if(POLICY CMP0063)
#	cmake_policy(SET CMP0063 OLD)
#endif()

project(tinyxml2)
# rom-properties: Disabled.
#include(GNUInstallDirs)
#include(CTest)
#enable_testing()

#CMAKE_BUILD_TOOL

################################
# set lib version here

set(GENERIC_LIB_VERSION "6.0.0")
set(GENERIC_LIB_SOVERSION "6")

################################
# Add definitions

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

################################
# Add targets
# By Default shared libray is being built
# To build static libs also - Do cmake . -DBUILD_STATIC_LIBS:BOOL=ON
# User can choose not to build shared library by using cmake -DBUILD_SHARED_LIBS:BOOL=OFF
# To build only static libs use cmake . -DBUILD_SHARED_LIBS:BOOL=OFF -DBUILD_STATIC_LIBS:BOOL=ON
# To build the tests, use cmake . -DBUILD_TESTS:BOOL=ON
# To disable the building of the tests, use cmake . -DBUILD_TESTS:BOOL=OFF

# rom-properties: Don't use option() here; it's set by extlib.
IF(0)
option(BUILD_SHARED_LIBS "build as shared library" ON)
option(BUILD_STATIC_LIBS "build as static library" OFF)
option(BUILD_TESTS "build xmltest (deprecated: Use BUILD_TESTING)" ON)
ENDIF(0) # rom-properties

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# to distinguish between debug and release lib
set(CMAKE_DEBUG_POSTFIX "d")

if(BUILD_SHARED_LIBS)
# rom-properties: Add the custom .rc file in Windows DLL builds.
IF(WIN32)
	SET(TINYXML2_RC "tinyxml2.rc")
ENDIF(WIN32)
add_library(tinyxml2 SHARED tinyxml2.cpp tinyxml2.h ${TINYXML2_RC})

set_target_properties(tinyxml2 PROPERTIES
        #COMPILE_DEFINITIONS "TINYXML2_EXPORT"
	VERSION "${GENERIC_LIB_VERSION}"
	SOVERSION "${GENERIC_LIB_SOVERSION}")
# rom-properties: Set TINYXML2_IMPORT when linking to TinyXML2.
TARGET_COMPILE_DEFINITIONS(tinyxml2
	PRIVATE "TINYXML2_EXPORT"
	INTERFACE "TINYXML2_IMPORT")


if(DEFINED CMAKE_VERSION AND NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.11")
    target_include_directories(tinyxml2 PUBLIC 
                          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

    # rom-properties: This is already set in the platform configuration.
    IF(0)
    if(MSVC)
      target_compile_definitions(tinyxml2 PUBLIC -D_CRT_SECURE_NO_WARNINGS)
    endif(MSVC)
    ENDIF(0) # rom-properties
else()
    include_directories(${PROJECT_SOURCE_DIR})

    # rom-properties: This is already set in the platform configuration.
    IF(0)
    if(MSVC)
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif(MSVC)
    ENDIF(0) # rom-properties
endif()

# rom-properties: Disable target export.
IF(0)
# export targets for find_package config mode
export(TARGETS tinyxml2
      FILE ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Targets.cmake)
ENDIF(0) # rom-properties

# rom-properties:
# - Disabled installation of export files and import libraries.
# - Use the correct RUNTIME destination.
# - Install PDB files.
install(TARGETS tinyxml2
        #EXPORT ${CMAKE_PROJECT_NAME}Targets
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        #LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        #ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
	)

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
        #COMPILE_DEFINITONS "TINYXML2_EXPORT"
        VERSION "${GENERIC_LIB_VERSION}"
        SOVERSION "${GENERIC_LIB_SOVERSION}")
set_target_properties( tinyxml2_static PROPERTIES OUTPUT_NAME tinyxml2 )
# rom-properties: Set TINYXML2_IMPORT when linking to TinyXML2.
TARGET_COMPILE_DEFINITIONS(tinyxml2_static
	PRIVATE "TINYXML2_EXPORT"
	INTERFACE "TINYXML2_IMPORT")
# rom-properties: Set the MSVC debug path.
SET_MSVC_DEBUG_PATH(tinyxml2_static)

# rom-properties: This is already set in the platform configuration.
IF(0)
target_compile_definitions(tinyxml2_static PUBLIC -D_CRT_SECURE_NO_WARNINGS)
ENDIF(0) # rom-properties

if(DEFINED CMAKE_VERSION AND NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.11")
    target_include_directories(tinyxml2_static PUBLIC 
                          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

    # rom-properties: This is already set in the platform configuration.
    IF(0)
    if(MSVC)
      target_compile_definitions(tinyxml2_static PUBLIC -D_CRT_SECURE_NO_WARNINGS)
    endif(MSVC)
    ENDIF(0) # rom-properties
else()
    include_directories(${PROJECT_SOURCE_DIR})

    # rom-properties: This is already set in the platform configuration.
    IF(0)
    if(MSVC)
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif(MSVC)
    ENDIF(0) # rom-properties
endif()

# rom-properties: Disable target export.
IF(0)
# export targets for find_package config mode
export(TARGETS tinyxml2_static
      FILE ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Targets.cmake)
ENDIF(0) # rom-properties

# rom-properties: Don't install static libraries.
IF(0)
install(TARGETS tinyxml2_static
        EXPORT ${CMAKE_PROJECT_NAME}Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDIF(0) # rom-properties
endif()

# rom-properties: Don't build xmltest or install anything else.
IF(0)
if(BUILD_TESTING AND BUILD_TESTS)
  add_executable(xmltest xmltest.cpp)
  if(BUILD_SHARED_LIBS)
    add_dependencies(xmltest tinyxml2)
    target_link_libraries(xmltest tinyxml2)
  else(BUILD_STATIC_LIBS)
    add_dependencies(xmltest tinyxml2_static)
    target_link_libraries(xmltest tinyxml2_static)
  endif()

  # Copy test resources and create test output directory
  add_custom_command(TARGET xmltest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:xmltest>/resources
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:xmltest>/resources/out
    COMMENT "Configuring xmltest resources directory: ${CMAKE_BINARY_DIR}/resources"
  )

  add_test(NAME xmltest COMMAND xmltest)
endif()

install(FILES tinyxml2.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

foreach(p LIB INCLUDE)
	set(var CMAKE_INSTALL_${p}DIR)
	if(NOT IS_ABSOLUTE "${${var}}")
		set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
	endif()
endforeach()

configure_file(tinyxml2.pc.in tinyxml2.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tinyxml2.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
      IMMEDIATE @ONLY)

  add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
ENDIF(0) # rom-properties

# rom-properties: Don't install anything.
IF(0)
file(WRITE
    ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
    "include(\${CMAKE_CURRENT_LIST_DIR}/${CMAKE_PROJECT_NAME}Targets.cmake)\n")

install(FILES
        ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
        DESTINATION lib/cmake/${CMAKE_PROJECT_NAME})

install(EXPORT ${CMAKE_PROJECT_NAME}Targets
        DESTINATION lib/cmake/${CMAKE_PROJECT_NAME})
ENDIF(0) # rom-properties
