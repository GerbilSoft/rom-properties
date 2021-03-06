# CMake support for LZ4
#
# To the extent possible under law, the author(s) have dedicated all
# copyright and related and neighboring rights to this software to
# the public domain worldwide. This software is distributed without
# any warranty.
#
# For details, see <http://creativecommons.org/publicdomain/zero/1.0/>.
#
# LZ4's CMake support is maintained by Evan Nemerson; when filing
# bugs please mention @nemequ to make sure I see it.

set(LZ4_TOP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# rom-properties: Always disabled.
#option(LZ4_BUILD_LEGACY_LZ4C "Build lz4c progam with legacy argument support" ON)
set(LZ4_BUILD_LEGACY_LZ4C OFF)

# Parse version information
file(STRINGS "${LZ4_TOP_SOURCE_DIR}/lib/lz4.h" LZ4_VERSION_MAJOR REGEX "^#define LZ4_VERSION_MAJOR +([0-9]+) +.*$")
string(REGEX REPLACE "^#define LZ4_VERSION_MAJOR +([0-9]+) +.*$" "\\1" LZ4_VERSION_MAJOR "${LZ4_VERSION_MAJOR}")
file(STRINGS "${LZ4_TOP_SOURCE_DIR}/lib/lz4.h" LZ4_VERSION_MINOR REGEX "^#define LZ4_VERSION_MINOR +([0-9]+) +.*$")
string(REGEX REPLACE "^#define LZ4_VERSION_MINOR +([0-9]+) +.*$" "\\1" LZ4_VERSION_MINOR "${LZ4_VERSION_MINOR}")
file(STRINGS "${LZ4_TOP_SOURCE_DIR}/lib/lz4.h" LZ4_VERSION_RELEASE REGEX "^#define LZ4_VERSION_RELEASE +([0-9]+) +.*$")
string(REGEX REPLACE "^#define LZ4_VERSION_RELEASE +([0-9]+) +.*$" "\\1" LZ4_VERSION_RELEASE "${LZ4_VERSION_RELEASE}")
set(LZ4_VERSION_STRING "${LZ4_VERSION_MAJOR}.${LZ4_VERSION_MINOR}.${LZ4_VERSION_RELEASE}")
mark_as_advanced(LZ4_VERSION_STRING LZ4_VERSION_MAJOR LZ4_VERSION_MINOR LZ4_VERSION_RELEASE)

if("${CMAKE_VERSION}" VERSION_LESS "3.0")
  project(LZ4 C)
else()
  cmake_policy (SET CMP0048 NEW)
  project(LZ4
    VERSION ${LZ4_VERSION_STRING}
    LANGUAGES C)
endif()

cmake_minimum_required (VERSION 2.8.6)

# If LZ4 is being bundled in another project, we don't want to
# install anything.  However, we want to let people override this, so
# we'll use the LZ4_BUNDLED_MODE variable to let them do that; just
# set it to OFF in your project before you add_subdirectory(lz4/contrib/cmake_unofficial).
get_directory_property(LZ4_PARENT_DIRECTORY PARENT_DIRECTORY)
if("${LZ4_BUNDLED_MODE}" STREQUAL "")
  # Bundled mode hasn't been set one way or the other, set the default
  # depending on whether or not we are the top-level project.
  if("${LZ4_PARENT_DIRECTORY}" STREQUAL "")
    set(LZ4_BUNDLED_MODE OFF)
  else()
    set(LZ4_BUNDLED_MODE ON)
  endif()
endif()
mark_as_advanced(LZ4_BUNDLED_MODE)

# CPack
if(NOT LZ4_BUNDLED_MODE AND NOT CPack_CMake_INCLUDED)
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LZ4 compression library")
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${LZ4_TOP_SOURCE_DIR}/README.md")
  set(CPACK_RESOURCE_FILE_LICENSE "${LZ4_TOP_SOURCE_DIR}/LICENSE")
  set(CPACK_PACKAGE_VERSION_MAJOR ${LZ4_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${LZ4_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${LZ4_VERSION_RELEASE})
  include(CPack)
endif(NOT LZ4_BUNDLED_MODE AND NOT CPack_CMake_INCLUDED)

# Allow people to choose whether to build shared or static libraries
# via the BUILD_SHARED_LIBS option unless we are in bundled mode, in
# which case we always use static libraries.
# rom-properties: Disable this; we'll control it ourserlves in extlib.
#include(CMakeDependentOption)
#CMAKE_DEPENDENT_OPTION(BUILD_SHARED_LIBS "Build shared libraries" ON "NOT LZ4_BUNDLED_MODE" OFF)
#CMAKE_DEPENDENT_OPTION(BUILD_STATIC_LIBS "Build static libraries" OFF "BUILD_SHARED_LIBS" ON)

if(NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
  message(FATAL_ERROR "Both BUILD_SHARED_LIBS and BUILD_STATIC_LIBS have been disabled")
endif()

set(LZ4_LIB_SOURCE_DIR "${LZ4_TOP_SOURCE_DIR}/lib")
set(LZ4_PROG_SOURCE_DIR "${LZ4_TOP_SOURCE_DIR}/programs")

include_directories("${LZ4_LIB_SOURCE_DIR}")

# CLI sources
set(LZ4_SOURCES
  "${LZ4_LIB_SOURCE_DIR}/lz4.c"
  "${LZ4_LIB_SOURCE_DIR}/lz4hc.c"
  "${LZ4_LIB_SOURCE_DIR}/lz4.h"
  "${LZ4_LIB_SOURCE_DIR}/lz4hc.h"
  "${LZ4_LIB_SOURCE_DIR}/lz4frame.c"
  "${LZ4_LIB_SOURCE_DIR}/lz4frame.h"
  "${LZ4_LIB_SOURCE_DIR}/xxhash.c")
set(LZ4_CLI_SOURCES
  "${LZ4_PROG_SOURCE_DIR}/bench.c"
  "${LZ4_PROG_SOURCE_DIR}/lz4cli.c"
  "${LZ4_PROG_SOURCE_DIR}/lz4io.c"
  "${LZ4_PROG_SOURCE_DIR}/datagen.c")

# Whether to use position independent code for the static library.  If
# we're building a shared library this is ignored and PIC is always
# used.
# rom-properties: Always enable this.
#option(LZ4_POSITION_INDEPENDENT_LIB "Use position independent code for static library (if applicable)" ON)
set(LZ4_POSITION_INDEPENDENT_LIB ON)

# rom-properties
IF(MSVC)
	# FIXME: MSVC 2019 is complaining of C2373 type redefinitions
	# if we don't remove "/Gz", even though we have ZSTDAPI_CALL.
	STRING(REPLACE "/Gz" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

	# Append 'd' to the DLL filename for debug builds.
	SET(CMAKE_DEBUG_POSTFIX "d")
ENDIF(MSVC)

# liblz4
set(LZ4_LIBRARIES_BUILT)
if(BUILD_SHARED_LIBS)
  IF(WIN32)
    # rom-properties: Add the Win32 resource file.
    SET(LZ4_RC_FILE "${LZ4_TOP_SOURCE_DIR}/visual/VS2017/liblz4-dll/liblz4-dll.rc")
  ENDIF(WIN32)
  add_library(lz4_shared SHARED ${LZ4_SOURCES} ${LZ4_RC_FILE})
  set_target_properties(lz4_shared PROPERTIES
    OUTPUT_NAME lz4
    SOVERSION "${LZ4_VERSION_MAJOR}"
    VERSION "${LZ4_VERSION_STRING}")
  list(APPEND LZ4_LIBRARIES_BUILT lz4_shared)
  # rom-properties: Add the include directory, split debug, etc.
  # Also set LZ4_DLL_EXPORT to ensure the import library is built,
  # and LZ4_DLL_IMPORT to optimize DLL imports.
  target_include_directories(lz4_shared PUBLIC "${LZ4_LIB_SOURCE_DIR}")
  IF(WIN32)
    target_compile_definitions(lz4_shared
	PRIVATE -DLZ4_DLL_EXPORT=1
	INTERFACE -DLZ4_DLL_IMPORT=1
	)
  ENDIF(WIN32)
  DO_SPLIT_DEBUG(lz4_shared)
  SET_WINDOWS_SUBSYSTEM(lz4_shared WIN32)
endif()
if(BUILD_STATIC_LIBS)
  add_library(lz4_static STATIC ${LZ4_SOURCES})
  set_target_properties(lz4_static PROPERTIES
    OUTPUT_NAME lz4
    POSITION_INDEPENDENT_CODE ${LZ4_POSITION_INDEPENDENT_LIB})
  list(APPEND LZ4_LIBRARIES_BUILT lz4_static)
  # rom-properties: Add the include directory.
  target_include_directories(lz4_static PUBLIC "${LZ4_LIB_SOURCE_DIR}")
endif()

# link to shared whenever possible, to static otherwise
if(BUILD_SHARED_LIBS)
  set(LZ4_LINK_LIBRARY lz4_shared)
else()
  set(LZ4_LINK_LIBRARY lz4_static)
endif()

# lz4
set(LZ4_PROGRAMS_BUILT lz4cli)
add_executable(lz4cli ${LZ4_CLI_SOURCES})
set_target_properties(lz4cli PROPERTIES OUTPUT_NAME lz4)
target_link_libraries(lz4cli ${LZ4_LINK_LIBRARY})

# lz4c
if (LZ4_BUILD_LEGACY_LZ4C)
  list(APPEND LZ4_PROGRAMS_BUILT lz4c)
  add_executable(lz4c ${LZ4_CLI_SOURCES})
  set_target_properties(lz4c PROPERTIES COMPILE_DEFINITIONS "ENABLE_LZ4C_LEGACY_OPTIONS")
  target_link_libraries(lz4c ${LZ4_LINK_LIBRARY})
endif()

# Extra warning flags
include (CheckCCompilerFlag)
foreach (flag
    # GCC-style
    -Wall
    -Wextra
    -Wundef
    -Wcast-qual
    -Wcast-align
    -Wshadow
    -Wswitch-enum
    -Wdeclaration-after-statement
    -Wstrict-prototypes
    -Wpointer-arith

    # MSVC-style
    /W4)
  # Because https://gcc.gnu.org/wiki/FAQ#wnowarning
  string(REGEX REPLACE "\\-Wno\\-(.+)" "-W\\1" flag_to_test "${flag}")
  string(REGEX REPLACE "[^a-zA-Z0-9]+" "_" test_name "CFLAG_${flag_to_test}")

  check_c_compiler_flag("${ADD_COMPILER_FLAGS_PREPEND} ${flag_to_test}" ${test_name})

  if(${test_name})
    set(CMAKE_C_FLAGS "${flag} ${CMAKE_C_FLAGS}")
  endif()

  unset(test_name)
  unset(flag_to_test)
endforeach (flag)

if(NOT LZ4_BUNDLED_MODE)
  include(GNUInstallDirs)

  install(TARGETS ${LZ4_PROGRAMS_BUILT}
    BUNDLE	DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  install(TARGETS ${LZ4_LIBRARIES_BUILT}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  install(FILES
    "${LZ4_LIB_SOURCE_DIR}/lz4.h"
    "${LZ4_LIB_SOURCE_DIR}/lz4frame.h"
    "${LZ4_LIB_SOURCE_DIR}/lz4hc.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(FILES "${LZ4_PROG_SOURCE_DIR}/lz4.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/liblz4.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

  # install lz4cat and unlz4 symlinks on *nix
  if(UNIX)
    install(CODE "
      foreach(f lz4cat unlz4)
        set(dest \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/\${f}\")
        message(STATUS \"Symlinking: \${dest} -> lz4\")
        execute_process(
          COMMAND \"${CMAKE_COMMAND}\" -E create_symlink lz4 \"\${dest}\")
      endforeach()
    ")

    # create manpage aliases
    foreach(f lz4cat unlz4)
      file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${f}.1" ".so man1/lz4.1\n")
      install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${f}.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
    endforeach()
  endif(UNIX)
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

# pkg-config
set(PREFIX "${CMAKE_INSTALL_PREFIX}")

if("${CMAKE_INSTALL_FULL_LIBDIR}" STREQUAL "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  set(LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
else()
  set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
endif()

if("${CMAKE_INSTALL_FULL_INCLUDEDIR}" STREQUAL "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
  set(INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
else()
  set(INCLUDEDIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
endif()

# for liblz4.pc substitution
set(VERSION ${LZ4_VERSION_STRING})
configure_file(${LZ4_LIB_SOURCE_DIR}/liblz4.pc.in liblz4.pc @ONLY)
