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
IF(0)
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
cmake_policy(VERSION 2.6)
if(POLICY CMP0063)
	cmake_policy(SET CMP0063 OLD)
endif()
ENDIF(0) # rom-properties

project(tinyxml2)
# rom-properties: Disabled.
IF(0)
include(GNUInstallDirs)
include(CTest)
#enable_testing()
ENDIF(0) # rom-properties

#CMAKE_BUILD_TOOL

################################
# set lib version here

set(GENERIC_LIB_VERSION "7.0.1")
set(GENERIC_LIB_SOVERSION "7")

################################
# Add definitions

################################
# Add targets
# By Default shared library is being built
# To build static libs also - Do cmake . -DBUILD_STATIC_LIBS:BOOL=ON
# User can choose not to build shared library by using cmake -DBUILD_SHARED_LIBS:BOOL=OFF
# To build only static libs use cmake . -DBUILD_SHARED_LIBS:BOOL=OFF -DBUILD_STATIC_LIBS:BOOL=ON
# To build the tests, use cmake . -DBUILD_TESTS:BOOL=ON
# To disable the building of the tests, use cmake . -DBUILD_TESTS:BOOL=OFF

# rom-properties: Don't use option() here; it's set by extlib.
IF(0)
option(BUILD_SHARED_LIBS "build as shared library" ON)
option(BUILD_TESTS "build xmltest (deprecated: Use BUILD_TESTING)" ON)
ENDIF(0) # rom-properties

# To allow using tinyxml in another shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# to distinguish between debug and release lib
set(CMAKE_DEBUG_POSTFIX "d")

# rom-properties: Add the custom .rc file in Windows DLL builds.
# FIXME: Don't add it if building both DLL and static.
IF(WIN32 AND BUILD_SHARED_LIBS)
	SET(TINYXML2_RC "tinyxml2.rc")
ENDIF(WIN32 AND BUILD_SHARED_LIBS)
add_library(tinyxml2 tinyxml2.cpp tinyxml2.h ${TINYXML2_RC})

set_target_properties(tinyxml2 PROPERTIES
        COMPILE_DEFINITIONS "TINYXML2_EXPORT"
	VERSION "${GENERIC_LIB_VERSION}"
	SOVERSION "${GENERIC_LIB_SOVERSION}")
# rom-properties: Set TINYXML2_IMPORT when linking to TinyXML2.
TARGET_COMPILE_DEFINITIONS(tinyxml2
	PRIVATE "TINYXML2_EXPORT"
	INTERFACE "TINYXML2_IMPORT")

target_compile_definitions(tinyxml2 PUBLIC $<$<CONFIG:Debug>:TINYXML2_DEBUG>)

if(DEFINED CMAKE_VERSION AND NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.11")
    target_include_directories(tinyxml2 PUBLIC 
                          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

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
# - Only install on Windows.
#  - TODO: Install .dylib/.so into macOS bundle.
# - Disabled installation of export files and import libraries.
# - Use the correct RUNTIME destination.
# - Install PDB files.
IF(WIN32)
	install(TARGETS tinyxml2
		#EXPORT ${CMAKE_PROJECT_NAME}Targets
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		#LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		#ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)
ENDIF(WIN32)

# rom-properties: Split debug functionality; Windows subsystem.
DO_SPLIT_DEBUG(tinyxml2)
SET_WINDOWS_SUBSYSTEM(tinyxml2 WIN32)
IF(WIN32 AND BUILD_SHARED_LIBS AND INSTALL_DEBUG)
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
ENDIF(WIN32 AND BUILD_SHARED_LIBS AND INSTALL_DEBUG)

# rom-properties: Don't build xmltest or install anything else.
IF(0)
if(BUILD_TESTING AND BUILD_TESTS)
  add_executable(xmltest xmltest.cpp)
  add_dependencies(xmltest tinyxml2)
  target_link_libraries(xmltest tinyxml2)

  # Copy test resources and create test output directory
  add_custom_command(TARGET xmltest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:xmltest>/resources
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:xmltest>/resources/out
    COMMENT "Configuring xmltest resources directory: ${CMAKE_CURRENT_BINARY_DIR}/resources"
  )

  add_test(NAME xmltest COMMAND xmltest WORKING_DIRECTORY $<TARGET_FILE_DIR:xmltest>)
endif()

install(FILES tinyxml2.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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

include(CMakePackageConfigHelpers)
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
configure_package_config_file(
  "Config.cmake.in"
  "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}"
)
install(FILES
        ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME})

install(EXPORT ${CMAKE_PROJECT_NAME}Targets NAMESPACE tinyxml2::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME})
ENDIF(0) # rom-properties
