# CMake support for LZ4
#
# To the extent possible under law, the author(s) have dedicated all
# copyright and related and neighboring rights to this software to
# the public domain worldwide. This software is distributed without
# any warranty.
#
# For details, see <http://creativecommons.org/publicdomain/zero/1.0/>.


# Use range version specification for policy control while maintaining 
# compatibility with older CMake versions
cmake_minimum_required(VERSION 3.5...4.0.2)

set(LZ4_TOP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

#-----------------------------------------------------------------------------
# VERSION EXTRACTION - Parse version information from header file
#-----------------------------------------------------------------------------
function(parse_lz4_version VERSION_TYPE)
    file(STRINGS "${LZ4_TOP_SOURCE_DIR}/lib/lz4.h" version_line REGEX "^#define LZ4_VERSION_${VERSION_TYPE} +([0-9]+).*$")
    string(REGEX REPLACE "^#define LZ4_VERSION_${VERSION_TYPE} +([0-9]+).*$" "\\1" version_number "${version_line}")
    set(LZ4_VERSION_${VERSION_TYPE} ${version_number} PARENT_SCOPE)
endfunction()

foreach(version_type IN ITEMS MAJOR MINOR RELEASE)
    parse_lz4_version(${version_type})
endforeach()

set(LZ4_VERSION_STRING "${LZ4_VERSION_MAJOR}.${LZ4_VERSION_MINOR}.${LZ4_VERSION_RELEASE}")
mark_as_advanced(LZ4_VERSION_STRING LZ4_VERSION_MAJOR LZ4_VERSION_MINOR LZ4_VERSION_RELEASE)

message(STATUS "Creating build script for LZ4 version: ${LZ4_VERSION_STRING}")

project(LZ4 VERSION ${LZ4_VERSION_STRING} LANGUAGES C)

#-----------------------------------------------------------------------------
# DEFAULT BUILD TYPE - Set Release as default when no build type is specified
#-----------------------------------------------------------------------------
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#-----------------------------------------------------------------------------
# BUILD OPTIONS - Configure build targets and features
#-----------------------------------------------------------------------------
# Determine if LZ4 is being built as part of another project.
# If LZ4 is bundled in another project, we don't want to install anything,
# nor build the lz4 program by default.
# Default behavior can be overridden by setting the LZ4_BUNDLED_MODE variable.
if(NOT DEFINED LZ4_BUNDLED_MODE)
  get_directory_property(LZ4_IS_SUBPROJECT PARENT_DIRECTORY)
  if(LZ4_IS_SUBPROJECT)
    set(LZ4_BUNDLED_MODE ON)
  else()
    set(LZ4_BUNDLED_MODE OFF)
  endif()
endif()
mark_as_advanced(LZ4_BUNDLED_MODE)

if(LZ4_BUNDLED_MODE)
  set(LZ4_BUILD_CLI_DEFAULT OFF)
else()
  set(LZ4_BUILD_CLI_DEFAULT ON)
endif()
IF(0) # rom-properties: Disable options; set them in extlib/CMakeLists.txt.
option(LZ4_BUILD_CLI "Build lz4 program" ${LZ4_BUILD_CLI_DEFAULT})
option(LZ4_BUILD_LEGACY_LZ4C "Build lz4c program with legacy argument support" OFF)
option(LZ4_BUILD_MULTITHREAD "Enable multithreading in lz4 programs" ON)
# By default, the shared library supports multithreaded compression (see LZ4F_preferences_t.nbWorkers),
# while the static library doesn't, so that programs linking it don't need to link threads.
option(LZ4_SHARED_MULTITHREAD "Enable multithreading support in shared library" ON)
option(LZ4_STATIC_MULTITHREAD "Enable multithreading support in static library (programs linking it must then link threads)" OFF)
ENDIF(0) # rom-properties


#-----------------------------------------------------------------------------
# PACKAGING - CPack configuration
#-----------------------------------------------------------------------------
if(NOT LZ4_BUNDLED_MODE AND NOT CPack_CMake_INCLUDED)
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LZ4 compression library")
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${LZ4_TOP_SOURCE_DIR}/README.md")
  set(CPACK_RESOURCE_FILE_LICENSE "${LZ4_TOP_SOURCE_DIR}/LICENSE")
  set(CPACK_PACKAGE_VERSION_MAJOR ${LZ4_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${LZ4_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${LZ4_VERSION_RELEASE})
  include(CPack)
endif(NOT LZ4_BUNDLED_MODE AND NOT CPack_CMake_INCLUDED)

#-----------------------------------------------------------------------------
# LIBRARY TYPE CONFIGURATION - Static vs Shared libraries
#-----------------------------------------------------------------------------
# Allow people to choose whether to build shared or static libraries
# via the BUILD_SHARED_LIBS option unless we are in bundled mode, in
# which case we always use static libraries.
IF(0) # rom-properties: Disable this; we'll control it ourserlves in extlib.
include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(BUILD_SHARED_LIBS "Build shared libraries" ON "NOT LZ4_BUNDLED_MODE" OFF)
CMAKE_DEPENDENT_OPTION(BUILD_STATIC_LIBS "Build static libraries" OFF "BUILD_SHARED_LIBS" ON)
ENDIF(0) # rom-properties

if(NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
  message(FATAL_ERROR "Both BUILD_SHARED_LIBS and BUILD_STATIC_LIBS have been disabled")
endif(NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)

#-----------------------------------------------------------------------------
# SOURCE FILES & DIRECTORIES - Path setup and source file collection
#-----------------------------------------------------------------------------
set(LZ4_LIB_SOURCE_DIR "${LZ4_TOP_SOURCE_DIR}/lib")
set(LZ4_PROG_SOURCE_DIR "${LZ4_TOP_SOURCE_DIR}/programs")

include_directories("${LZ4_LIB_SOURCE_DIR}")

# CLI sources
file(GLOB LZ4_SOURCES
     "${LZ4_LIB_SOURCE_DIR}/*.c")
file(GLOB LZ4_CLI_SOURCES
     "${LZ4_PROG_SOURCE_DIR}/*.c")
list(APPEND LZ4_CLI_SOURCES ${LZ4_SOURCES}) # LZ4_CLI always use liblz4 sources directly.

#-----------------------------------------------------------------------------
# POSITION INDEPENDENT CODE - PIC settings for static libraries
#-----------------------------------------------------------------------------
# Whether to use position independent code for the static library.  If
# we're building a shared library this is ignored and PIC is always
# used.
if(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE OR CMAKE_POSITION_INDEPENDENT_CODE)
  set(LZ4_POSITION_INDEPENDENT_LIB_DEFAULT ON)
else()
  set(LZ4_POSITION_INDEPENDENT_LIB_DEFAULT OFF)
endif(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE OR CMAKE_POSITION_INDEPENDENT_CODE)

IF(0) # rom-properties: Always enable PIC.
option(LZ4_POSITION_INDEPENDENT_LIB "Use position independent code for static library (if applicable)" ${LZ4_POSITION_INDEPENDENT_LIB_DEFAULT})
ENDIF(0) # rom-properties
set(LZ4_POSITION_INDEPENDENT_LIB ON)

# rom-properties
IF(MSVC)
	# Append 'd' to the DLL filename for debug builds.
	SET(CMAKE_DEBUG_POSTFIX "d")
ENDIF(MSVC)

#-----------------------------------------------------------------------------
# TARGETS - Library and executable targets
#-----------------------------------------------------------------------------
# liblz4
include(GNUInstallDirs)
set(LZ4_LIBRARIES_BUILT)
if(BUILD_SHARED_LIBS)
  IF(WIN32)
    # rom-properties: Add the Win32 resource file.
    SET(LZ4_RC_FILE "${LZ4_TOP_SOURCE_DIR}/build/VS2022/liblz4-dll/liblz4-dll.rc")
    SET(LZ4_SOURCES ${LZ4_SOURCES} ${LZ4_RC_FILE})
  ENDIF(WIN32)
  add_library(lz4_shared SHARED ${LZ4_SOURCES})
  target_include_directories(lz4_shared
    PUBLIC $<BUILD_INTERFACE:${LZ4_LIB_SOURCE_DIR}>
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  set_target_properties(lz4_shared PROPERTIES
    OUTPUT_NAME lz4
    SOVERSION "${LZ4_VERSION_MAJOR}"
    VERSION "${LZ4_VERSION_STRING}")
  if(MSVC)
    # rom-properties: Add LZ4_DLL_IMPORT=1 in interface flags.
    target_compile_definitions(lz4_shared
	PRIVATE -DLZ4_DLL_EXPORT=1
	INTERFACE -DLZ4_DLL_IMPORT=1
	)
  endif()
  list(APPEND LZ4_LIBRARIES_BUILT lz4_shared)
  # rom-properties: Add the include directory, split debug, etc.
  target_include_directories(lz4_shared PUBLIC "${LZ4_LIB_SOURCE_DIR}")
  DO_SPLIT_DEBUG(lz4_shared)
  SET_WINDOWS_SUBSYSTEM(lz4_shared WIN32)
endif()
if(BUILD_STATIC_LIBS)
  # rom-properties: Hide all symbols so they're not exported by libromdata.
  CMAKE_POLICY(SET CMP0063 NEW)
  SET(CMAKE_C_VISIBILITY_PRESET "hidden")
  SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

  set(STATIC_LIB_NAME lz4)
  if(MSVC AND BUILD_SHARED_LIBS)
    set(STATIC_LIB_NAME lz4_static)
  endif(MSVC AND BUILD_SHARED_LIBS)
  add_library(lz4_static STATIC ${LZ4_SOURCES})
  target_include_directories(lz4_static
    PUBLIC $<BUILD_INTERFACE:${LZ4_LIB_SOURCE_DIR}>
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  set_target_properties(lz4_static PROPERTIES
    OUTPUT_NAME ${STATIC_LIB_NAME}
    POSITION_INDEPENDENT_CODE ${LZ4_POSITION_INDEPENDENT_LIB})
  list(APPEND LZ4_LIBRARIES_BUILT lz4_static)
  # rom-properties: Hide all symbols so they're not exported by libromdata.
  target_compile_definitions(lz4_static PUBLIC LZ4LIB_VISIBILITY=)
endif()

# Add unified target.
add_library(lz4 INTERFACE)
list(APPEND LZ4_LIBRARIES_BUILT lz4)
if(BUILD_SHARED_LIBS)
  target_link_libraries(lz4 INTERFACE lz4_shared)
  set(LZ4_LINK_LIBRARY lz4_shared) # rom-properties
else()
  target_link_libraries(lz4 INTERFACE lz4_static)
  set(LZ4_LINK_LIBRARY lz4_static) # rom-properties
endif(BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# DEBUG CONFIGURATION - Configurable LZ4_DEBUG level
#-----------------------------------------------------------------------------
# LZ4_DEBUG levels:
# 0 - Disable everything (default for Release)
# 1 - Enable assert() statements  
# 2-8 - Enable debug traces with increasing verbosity
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(LZ4_DEBUG_LEVEL_DEFAULT 1)
else()
  set(LZ4_DEBUG_LEVEL_DEFAULT 0)
endif()

set(LZ4_DEBUG_LEVEL ${LZ4_DEBUG_LEVEL_DEFAULT} CACHE STRING 
    "LZ4 debug level: 0=disabled, 1=assert(), 2-8=debug traces with increasing verbosity")
set_property(CACHE LZ4_DEBUG_LEVEL PROPERTY STRINGS "0" "1" "2" "3" "4" "5" "6" "7" "8")

# Apply LZ4_DEBUG configuration if level > 0
if(LZ4_DEBUG_LEVEL GREATER 0)
  if(MSVC)
    add_definitions(/DLZ4_DEBUG=${LZ4_DEBUG_LEVEL})
  else()
    add_definitions(-DLZ4_DEBUG=${LZ4_DEBUG_LEVEL})
  endif()
endif()

#-----------------------------------------------------------------------------
# NAMESPACE CONFIGURATION - xxHash namespace settings
#-----------------------------------------------------------------------------
# xxhash namespace
if(BUILD_SHARED_LIBS)
  target_compile_definitions(lz4_shared PRIVATE
    XXH_NAMESPACE=LZ4_)
endif(BUILD_SHARED_LIBS)
if(BUILD_STATIC_LIBS)
  target_compile_definitions(lz4_static PRIVATE
    XXH_NAMESPACE=LZ4_)
endif(BUILD_STATIC_LIBS)

#-----------------------------------------------------------------------------
# LIBRARY MULTITHREADING - requires pthread, except on Windows
#-----------------------------------------------------------------------------
set(LZ4_LIB_MT_TARGETS)
if(BUILD_SHARED_LIBS AND LZ4_SHARED_MULTITHREAD)
  list(APPEND LZ4_LIB_MT_TARGETS lz4_shared)
endif()
if(BUILD_STATIC_LIBS AND LZ4_STATIC_MULTITHREAD)
  list(APPEND LZ4_LIB_MT_TARGETS lz4_static)
endif()
set(LZ4_STATIC_NEEDS_THREADS OFF)   # used by lz4Config.cmake and liblz4.pc
if(LZ4_LIB_MT_TARGETS)
  if(WIN32)
    set(LZ4_LIB_HAVE_THREADS ON)    # Windows threads : no additional library
  else()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads)
    if(CMAKE_USE_PTHREADS_INIT)
      set(LZ4_LIB_HAVE_THREADS ON)
      foreach(target ${LZ4_LIB_MT_TARGETS})
        target_link_libraries(${target} PRIVATE Threads::Threads)
      endforeach()
      if(BUILD_STATIC_LIBS AND LZ4_STATIC_MULTITHREAD)
        set(LZ4_STATIC_NEEDS_THREADS ON)
      endif()
    else()
      set(LZ4_LIB_HAVE_THREADS OFF)
      message(WARNING "pthread not found : liblz4 is built without multithreading support")
    endif()
  endif()
  if(LZ4_LIB_HAVE_THREADS)
    foreach(target ${LZ4_LIB_MT_TARGETS})
      target_compile_definitions(${target} PRIVATE LZ4F_MULTITHREAD=1)
    endforeach()
    message(STATUS "liblz4 multithreading support : ${LZ4_LIB_MT_TARGETS}")
  endif()
endif()

#-----------------------------------------------------------------------------
# CLI EXECUTABLES - Configuring command-line programs
#-----------------------------------------------------------------------------
# lz4
if(LZ4_BUILD_CLI)
  set(LZ4_PROGRAMS_BUILT lz4cli)
  add_executable(lz4cli ${LZ4_CLI_SOURCES})
  set_target_properties(lz4cli PROPERTIES OUTPUT_NAME lz4)
endif(LZ4_BUILD_CLI)

# lz4c
if(LZ4_BUILD_LEGACY_LZ4C)
  list(APPEND LZ4_PROGRAMS_BUILT lz4c)
  add_executable(lz4c ${LZ4_CLI_SOURCES})
  set_target_properties(lz4c PROPERTIES COMPILE_DEFINITIONS "ENABLE_LZ4C_LEGACY_OPTIONS")
endif(LZ4_BUILD_LEGACY_LZ4C)

# multithreading : requires pthread, except on Windows (see programs/lz4conf.h)
if(LZ4_PROGRAMS_BUILT)
  if(NOT LZ4_BUILD_MULTITHREAD)
    set_property(TARGET ${LZ4_PROGRAMS_BUILT} APPEND PROPERTY COMPILE_DEFINITIONS LZ4IO_MULTITHREAD=0)
  elseif(WIN32)
    # liblz4 sources are compiled into programs : frames are compressed by its worker threads
    set_property(TARGET ${LZ4_PROGRAMS_BUILT} APPEND PROPERTY COMPILE_DEFINITIONS LZ4F_MULTITHREAD=1)
  else()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads)
    if(CMAKE_USE_PTHREADS_INIT)
      set_property(TARGET ${LZ4_PROGRAMS_BUILT} APPEND PROPERTY COMPILE_DEFINITIONS LZ4IO_MULTITHREAD=1 LZ4F_MULTITHREAD=1)
      foreach(prog ${LZ4_PROGRAMS_BUILT})
        target_link_libraries(${prog} PRIVATE Threads::Threads)
      endforeach()
    else()
      message(WARNING "pthread not found : lz4 programs are built without multithreading support")
    endif()
  endif()
endif()

#-----------------------------------------------------------------------------
# COMPILER FLAGS - Configure warning flags and compiler-specific options
#-----------------------------------------------------------------------------
# Extra warning flags
if(MSVC)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W4")
else()
  include(CheckCCompilerFlag)
  foreach(flag
      # GCC-style
      -pedantic-errors
      -Wall
      -Wextra
      -Wundef
      -Wcast-qual
      -Wcast-align
      -Wshadow
      -Wswitch-enum
      -Wdeclaration-after-statement
      -Wstrict-prototypes
      -Wpointer-arith)

    # Because https://gcc.gnu.org/wiki/FAQ#wnowarning
    string(REGEX REPLACE "\\-Wno\\-(.+)" "-W\\1" flag_to_test "${flag}")
    string(REGEX REPLACE "[^a-zA-Z0-9]+" "_" test_name "CFLAG_${flag_to_test}")

    check_c_compiler_flag("${ADD_COMPILER_FLAGS_PREPEND} ${flag_to_test}" ${test_name})

    if(${test_name})
      set(CMAKE_C_FLAGS_DEBUG "${flag} ${CMAKE_C_FLAGS_DEBUG}")
    endif()

    unset(test_name)
    unset(flag_to_test)
  endforeach(flag)
endif(MSVC)

# On x86 and x86-64, keep branches within 32-byte boundaries,
# which mitigates the JCC erratum of Intel cpus of the Skylake family :
# lz4 is faster on these cpus (measured on Coffee Lake : compression up to +12%, decompression up to +31%),
# and unchanged on others (AMD Zen 4 and Zen 5, Intel Lunar Lake).
# The option depends on the compiler : MSVC, clang, or gcc (assembler option, binutils >= 2.34).
IF(0) # rom-properties: Disable the option; set it in extlib/CMakeLists.txt.
option(LZ4_JCC_ERRATUM "Mitigate the JCC erratum of Intel Skylake family cpus (x86 only)" ON)
ENDIF(0) # rom-properties
if(LZ4_JCC_ERRATUM AND CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|AMD64|amd64|i[3-6]86|x86)$")
  include(CheckCCompilerFlag)
  foreach(flag /QIntel-jcc-erratum -mbranches-within-32B-boundaries -Wa,-mbranches-within-32B-boundaries)
    string(REGEX REPLACE "[^a-zA-Z0-9]+" "_" test_name "CFLAG_JCC_${flag}")
    check_c_compiler_flag("${flag}" ${test_name})
    if(${test_name})
      string(APPEND CMAKE_C_FLAGS " ${flag}")
      break()
    endif()
  endforeach(flag)
endif()


#-----------------------------------------------------------------------------
# INSTALLATION - Install targets, headers, and documentation
#-----------------------------------------------------------------------------
if(NOT LZ4_BUNDLED_MODE)
  install(TARGETS ${LZ4_PROGRAMS_BUILT}
    BUNDLE	DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  install(TARGETS ${LZ4_LIBRARIES_BUILT}
    EXPORT lz4Targets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  install(FILES
    "${LZ4_LIB_SOURCE_DIR}/lz4.h"
    "${LZ4_LIB_SOURCE_DIR}/lz4hc.h"
    "${LZ4_LIB_SOURCE_DIR}/lz4frame.h"
    "${LZ4_LIB_SOURCE_DIR}/lz4file.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(FILES "${LZ4_PROG_SOURCE_DIR}/lz4.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/liblz4.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

#-----------------------------------------------------------------------------
# CMAKE PACKAGE CONFIG - Configure CMake package for find_package support
#-----------------------------------------------------------------------------
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/lz4ConfigVersion.cmake"
    VERSION ${LZ4_VERSION_STRING}
    COMPATIBILITY SameMajorVersion)

  set(LZ4_PKG_INSTALLDIR "${CMAKE_INSTALL_LIBDIR}/cmake/lz4")
  configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/lz4Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lz4Config.cmake"
    INSTALL_DESTINATION ${LZ4_PKG_INSTALLDIR})
  export(EXPORT lz4Targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/lz4Targets.cmake
    NAMESPACE LZ4::)

  install(EXPORT lz4Targets
    FILE lz4Targets.cmake
    NAMESPACE LZ4::
    DESTINATION ${LZ4_PKG_INSTALLDIR})
  install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/lz4Config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/lz4ConfigVersion.cmake
    DESTINATION ${LZ4_PKG_INSTALLDIR})

#-----------------------------------------------------------------------------
# SYMLINKS - Create and install Unix symlinks and manpage aliases
#-----------------------------------------------------------------------------
  # Install lz4cat and unlz4 symlinks on Unix systems
  if(UNIX AND LZ4_BUILD_CLI)
    foreach(cli_tool IN ITEMS lz4cat unlz4)
      # Create a custom target for the symlink creation
      add_custom_target("create_${cli_tool}_symlink" ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
          $<TARGET_FILE_NAME:lz4cli> ${cli_tool}
        DEPENDS lz4cli
        COMMENT "Creating symlink for ${cli_tool}"
        VERBATIM)

      # Install the symlink into the binary installation directory
      install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${cli_tool}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        RENAME ${cli_tool})
    endforeach()

    # create manpage aliases
    foreach(f lz4cat unlz4)
      file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${f}.1" ".so man1/lz4.1\n")
      install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${f}.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
    endforeach()
  endif(UNIX AND LZ4_BUILD_CLI)
endif(NOT LZ4_BUNDLED_MODE)

# rom-properties: Install the LZ4 shared library in the package.
IF(NOT SKIP_INSTALL_LIBRARIES)
	INSTALL(TARGETS ${LZ4_LINK_LIBRARY}
		#EXPORT lz4
		RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
		#ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
		FRAMEWORK DESTINATION "${INSTALL_LIB_DIR}")

	IF(INSTALL_DEBUG AND BUILD_SHARED_LIBS)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		# TODO: Test on MinGW.
		# FIXME: Generator PDB expression doesn't have the debug postfix for some reason,
		# so cpack fails in debug builds if we get the PDB property.
		INCLUDE(DirInstallPaths)
		SET(PDB_FILENAME_D "$<TARGET_FILE_DIR:${LZ4_LINK_LIBRARY}>/lz4${CMAKE_DEBUG_POSTFIX}.pdb")
		SET(PDB_FILENAME_R "$<TARGET_FILE_DIR:${LZ4_LINK_LIBRARY}>/lz4${CMAKE_RELEASE_POSTFIX}.pdb")
		INSTALL(FILES "${PDB_FILENAME_D}" "${PDB_FILENAME_R}"
			DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
			COMPONENT "debug"
			OPTIONAL
			)
		UNSET(PDB_FILENAME_D)
		UNSET(PDB_FILENAME_R)
	ENDIF(INSTALL_DEBUG AND BUILD_SHARED_LIBS)
ENDIF(NOT SKIP_INSTALL_LIBRARIES)

#-----------------------------------------------------------------------------
# PKG-CONFIG - Generate and install pkg-config file
#-----------------------------------------------------------------------------
# pkg-config
set(PREFIX "${CMAKE_INSTALL_PREFIX}")

if("${CMAKE_INSTALL_FULL_LIBDIR}" STREQUAL "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  set(LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
else()
  set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
endif("${CMAKE_INSTALL_FULL_LIBDIR}" STREQUAL "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

if("${CMAKE_INSTALL_FULL_INCLUDEDIR}" STREQUAL "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
  set(INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
else()
  set(INCLUDEDIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
endif("${CMAKE_INSTALL_FULL_INCLUDEDIR}" STREQUAL "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")

# for liblz4.pc substitution
set(VERSION ${LZ4_VERSION_STRING})
configure_file(${LZ4_LIB_SOURCE_DIR}/liblz4.pc.in liblz4.pc @ONLY)
if(LZ4_STATIC_NEEDS_THREADS)
  # static linking (pkg-config --static) must then link threads
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/liblz4.pc" "Libs.private: -pthread\n")
endif()
