# rom-properties: Disable CMake version requirement.
#cmake_minimum_required(VERSION 3.15)
project(tinyxml2 VERSION 11.0.0)

IF(0) # rom-properties: Disable tests.
include(CTest)
option(tinyxml2_BUILD_TESTING "Build tests for tinyxml2" "${BUILD_TESTING}")
ENDIF(0)

##
## Honor tinyxml2_SHARED_LIBS to match install interface
##

if (DEFINED tinyxml2_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${tinyxml2_SHARED_LIBS}")
endif ()

##
## Main library build
##

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

# rom-properties: to distinguish between debug and release lib
set(CMAKE_DEBUG_POSTFIX "d")

# rom-properties: Add the custom .rc file in Windows DLL builds.
# FIXME: Don't add it if building both DLL and static.
IF(WIN32 AND BUILD_SHARED_LIBS)
    SET(TINYXML2_RC "tinyxml2.rc")
ENDIF(WIN32 AND BUILD_SHARED_LIBS)
add_library(tinyxml2 tinyxml2.cpp tinyxml2.h ${TINYXML2_RC})
add_library(tinyxml2::tinyxml2 ALIAS tinyxml2)

IF(NOT (WIN32 OR APPLE) AND NOT BUILD_SHARED_LIBS)
    # rom-properties: Hide all symbols so they're not exported by libromdata.
    target_compile_definitions(tinyxml2 PUBLIC -DTINYXML2_NO_GCC_EXPORT)
ENDIF(NOT (WIN32 OR APPLE) AND NOT BUILD_SHARED_LIBS)

# Uncomment the following line to require C++11 (or greater) to use tinyxml2
# target_compile_features(tinyxml2 PUBLIC cxx_std_11)
target_include_directories(tinyxml2 PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_compile_definitions(
    tinyxml2
    PUBLIC $<$<CONFIG:Debug>:TINYXML2_DEBUG>
    INTERFACE $<$<BOOL:${BUILD_SHARED_LIBS}>:TINYXML2_IMPORT>
    # rom-properties: This is already set in the platform configuration.
    #PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    #PUBLIC _FILE_OFFSET_BITS=64
)

set_target_properties(
    tinyxml2
    PROPERTIES
    DEFINE_SYMBOL "TINYXML2_EXPORT"
    VERSION "${tinyxml2_VERSION}"
    SOVERSION "${tinyxml2_VERSION_MAJOR}"
)

# rom-properties: Make sure we have a versioned DLL on Windows.
IF(WIN32)
	# Append the SOVERSION and "d" for debug.
	SET(CMAKE_DEBUG_POSTFIX "d")
	SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES DEBUG_POSTFIX "-${tinyxml2_VERSION_MAJOR}${CMAKE_DEBUG_POSTFIX}")
	SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES RELEASE_POSTFIX "-${tinyxml2_VERSION_MAJOR}${CMAKE_RELEASE_POSTFIX}")
ENDIF(WIN32)

# rom-properties: Set TINYXML2_IMPORT when linking to TinyXML2.
TARGET_COMPILE_DEFINITIONS(tinyxml2
	PRIVATE "TINYXML2_EXPORT"
	INTERFACE "TINYXML2_IMPORT")

if (tinyxml2_BUILD_TESTING)
    add_executable(xmltest xmltest.cpp)
    target_link_libraries(xmltest PRIVATE tinyxml2::tinyxml2)

    add_test(
        NAME xmltest
        COMMAND xmltest
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    set_tests_properties(xmltest PROPERTIES PASS_REGULAR_EXPRESSION ", Fail 0")
endif ()

##
## Installation
##

## Standard modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

## Custom settings
# rom-properties: Disabled.
#option(tinyxml2_INSTALL_PKGCONFIG "Create and install pkgconfig files" ON)
set(tinyxml2_INSTALL_PKGCONFIG "Create and install pkgconfig files" OFF)

set(tinyxml2_INSTALL_PKGCONFIGDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    CACHE PATH "Directory for pkgconfig files")

set(tinyxml2_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/tinyxml2"
    CACHE STRING "Path to tinyxml2 CMake files")

## CMake targets and export scripts

# rom-properties:
# - Only install on Windows.
#  - TODO: Install .dylib/.so into macOS bundle.
# - Disabled installation of export files and import libraries.
# - Use the correct RUNTIME destination.
# - Install PDB files.
IF(WIN32) # rom-properties
install(
    TARGETS tinyxml2 EXPORT tinyxml2-targets
    RUNTIME COMPONENT tinyxml2_runtime
            DESTINATION ${INSTALL_BIN_DIR}
    #LIBRARY COMPONENT tinyxml2_runtime
    #NAMELINK_COMPONENT tinyxml2_development
    #ARCHIVE COMPONENT tinyxml2_development
    #INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
ENDIF(WIN32) # rom-properties

# rom-properties: Split debug functionality; Windows subsystem.
DO_SPLIT_DEBUG(tinyxml2)
SET_WINDOWS_SUBSYSTEM(tinyxml2 WIN32)
IF(WIN32 AND BUILD_SHARED_LIBS AND INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	INCLUDE(DirInstallPaths)
	# TODO: Test on MinGW.
	# FIXME: Generator PDB expression doesn't have the debug postfix for some reason,
	# so cpack fails in debug builds if we get the PDB property.
	SET(PDB_FILENAME_D "$<TARGET_FILE_DIR:tinyxml2>/tinyxml2-${tinyxml2_VERSION_MAJOR}${CMAKE_DEBUG_POSTFIX}.pdb")
	SET(PDB_FILENAME_R "$<TARGET_FILE_DIR:tinyxml2>/tinyxml2-${tinyxml2_VERSION_MAJOR}${CMAKE_RELEASE_POSTFIX}.pdb")
	INSTALL(FILES "${PDB_FILENAME_D}" "${PDB_FILENAME_R}"
		DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
		COMPONENT "debug"
		OPTIONAL
		)
	UNSET(PDB_FILENAME_D)
	UNSET(PDB_FILENAME_R)
ENDIF(WIN32 AND BUILD_SHARED_LIBS AND INSTALL_DEBUG)

IF(0) # rom-properties
# Type-specific targets

if (BUILD_SHARED_LIBS)
    set(type shared)
else ()
    set(type static)
endif ()

install(
    EXPORT tinyxml2-targets
    DESTINATION "${tinyxml2_INSTALL_CMAKEDIR}"
    NAMESPACE tinyxml2::
    FILE tinyxml2-${type}-targets.cmake
    COMPONENT tinyxml2_development
)

# Auto-generated version compatibility file
write_basic_package_version_file(
    tinyxml2-config-version.cmake
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tinyxml2-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/tinyxml2-config-version.cmake"
    DESTINATION "${tinyxml2_INSTALL_CMAKEDIR}"
    COMPONENT tinyxml2_development
)

## Headers

install(
    FILES tinyxml2.h
    TYPE INCLUDE
    COMPONENT tinyxml2_development
)

## pkg-config

if (tinyxml2_INSTALL_PKGCONFIG)
    configure_file(cmake/tinyxml2.pc.in tinyxml2.pc.gen @ONLY)
    file(GENERATE OUTPUT tinyxml2.pc INPUT "${CMAKE_CURRENT_BINARY_DIR}/tinyxml2.pc.gen")
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/tinyxml2.pc"
        DESTINATION "${tinyxml2_INSTALL_PKGCONFIGDIR}"
        COMPONENT tinyxml2_development
    )
endif ()
ENDIF(0) # rom-properties
