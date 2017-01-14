#cmake_minimum_required(VERSION 2.4.4)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(zlib C)

set(VERSION "1.2.10")

# rom-properties:
# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# rom-properties: Disable zlib options.
IF(0)
option(ASM686 "Enable building i686 assembly implementation")
option(AMD64 "Enable building amd64 assembly implementation")
ENDIF(0)

# rom-properties: Disable zlib options.
IF(0)
set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Installation directory for executables")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "Installation directory for libraries")
set(INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "Installation directory for headers")
set(INSTALL_MAN_DIR "${CMAKE_INSTALL_PREFIX}/share/man" CACHE PATH "Installation directory for manual pages")
set(INSTALL_PKGCONFIG_DIR "${CMAKE_INSTALL_PREFIX}/share/pkgconfig" CACHE PATH "Installation directory for pkgconfig (.pc) files")
ENDIF(0)

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
enable_testing()

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)

#
# Check to see if we have large file support
#
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
if(HAVE_SYS_TYPES_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_TYPES_H)
endif()
if(HAVE_STDINT_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDINT_H)
endif()
if(HAVE_STDDEF_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDDEF_H)
endif()
check_type_size(off64_t OFF64_T)
if(HAVE_OFF64_T)
   add_definitions(-D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS) # clear variable

#
# Check for fseeko
#
check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
    add_definitions(-DNO_FSEEKO)
endif()

#
# Check for unistd.h
#
check_include_file(unistd.h Z_HAVE_UNISTD_H)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    # If we're doing an out of source build and the user has a zconf.h
    # in their source tree...
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h)
        message(STATUS "Renaming")
        message(STATUS "    ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h")
        message(STATUS "to 'zconf.h.included' because this file is included with zlib")
        message(STATUS "but CMake generates it automatically in the build directory.")
        file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.included)
  endif()
endif()

set(ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/zlib.pc.cmakein
		${ZLIB_PC} @ONLY)
configure_file(	${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein
		${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})


#============================================================================
# zlib
#============================================================================

set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
    zlib.h
)
set(ZLIB_PRIVATE_HDRS
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h
)
set(ZLIB_SRCS
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

if(NOT MINGW)
    set(ZLIB_DLL_SRCS
        win32/zlib1.rc # If present will override custom build rule below.
    )
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	if(ASM686)
		set(ZLIB_ASMS contrib/asm686/match.S)
	elseif (AMD64)
		set(ZLIB_ASMS contrib/amd64/amd64-match.S)
	endif ()

	if(ZLIB_ASMS)
		add_definitions(-DASMV)
		# rom-properties: Disable underlines only if not Windows.
		# NOTE: We have to force compilation as C.
		if(WIN32)
			set_source_files_properties(${ZLIB_ASMS} PROPERTIES LANGUAGE C COMPILE_FLAGS "")
		else()
			set_source_files_properties(${ZLIB_ASMS} PROPERTIES LANGUAGE C COMPILE_FLAGS -DNO_UNDERLINE)
		endif()
	endif()
endif()

if(MSVC)
	if(ASM686)
		ENABLE_LANGUAGE(ASM_MASM)
		set(ZLIB_ASMS
			contrib/masmx86/inffas32.asm
			contrib/masmx86/match686.asm
			)
	elseif (AMD64)
		ENABLE_LANGUAGE(ASM_MASM)
		set(ZLIB_ASMS
			contrib/masmx64/gvmat64.asm
			contrib/masmx64/inffasx64.asm
			)
		# rom-properties: MSVC amd64 also requires inffas8664.c.
		set(ZLIB_ASM_C_SRCS
			contrib/masmx64/inffas8664.c
			)
	endif()

	if(ZLIB_ASMS)
		add_definitions(-DASMV -DASMINF)
		set_source_files_properties(${ZLIB_ASMS} PROPERTIES LANGUAGE ASM_MASM)
	endif()
endif()

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})

if(MINGW)
    # This gets us DLL resource information when compiling on MinGW.
    if(NOT CMAKE_RC_COMPILER)
        set(CMAKE_RC_COMPILER windres.exe)
    endif()

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
                       COMMAND ${CMAKE_RC_COMPILER}
                            -D GCC_WINDRES
                            -I ${CMAKE_CURRENT_SOURCE_DIR}
                            -I ${CMAKE_CURRENT_BINARY_DIR}
                            -o ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
                            -i ${CMAKE_CURRENT_SOURCE_DIR}/win32/zlib1.rc)
    set(ZLIB_DLL_SRCS ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj)
endif(MINGW)

# rom-properties: Added ${ZLIB_ASM_C_SRCS} for 64-bit MSVC builds.
add_library(zlib SHARED ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_ASM_C_SRCS} ${ZLIB_DLL_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
add_library(zlibstatic STATIC ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_ASM_C_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
set_target_properties(zlib PROPERTIES SOVERSION 1)
add_definitions(${ZLIB_DEFINITIONS})

# rom-properties: Set target definitions.
target_compile_definitions(zlib PUBLIC -DZLIB_CONST)
target_compile_definitions(zlibstatic PUBLIC -DZLIB_CONST)

# rom-properties: Set include directories.
TARGET_INCLUDE_DIRECTORIES(zlib
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	)
TARGET_INCLUDE_DIRECTORIES(zlibstatic
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	)

# rom-properties: Split debug functionality.
# NOTE: Do NOT use DO_SPLIT_DEBUG on static library targets,
# since it results in the entire thing being split out on Linux.
DO_SPLIT_DEBUG(zlib)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(zlibstatic)

if(NOT CYGWIN)
    # This property causes shared libraries on Linux to have the full version
    # encoded into their final filename.  We disable this on Cygwin because
    # it causes cygz-${ZLIB_FULL_VERSION}.dll to be created when cygz.dll
    # seems to be the default.
    #
    # This has no effect with MSVC, on that platform the version info for
    # the DLL comes from the resource file win32/zlib1.rc
    set_target_properties(zlib PROPERTIES VERSION ${ZLIB_FULL_VERSION})
endif()

if(UNIX)
    # On unix-like platforms the library is almost always called libz
   set_target_properties(zlib zlibstatic PROPERTIES OUTPUT_NAME z)
   if(NOT APPLE)
     set_target_properties(zlib PROPERTIES LINK_FLAGS "-Wl,--version-script,\"${CMAKE_CURRENT_SOURCE_DIR}/zlib.map\"")
   endif()
elseif(BUILD_SHARED_LIBS AND WIN32)
    # Creates zlib1.dll when building shared library version
    # rom-properties: MSVC builds: Append to POSTFIX instead of setting SUFFIX.
    #set_target_properties(zlib PROPERTIES SUFFIX "1.dll")
    SET_TARGET_PROPERTIES(zlib PROPERTIES DEBUG_POSTFIX "1${CMAKE_DEBUG_POSTFIX}")
    SET_TARGET_PROPERTIES(zlib PROPERTIES RELEASE_POSTFIX "1${CMAKE_RELEASE_POSTFIX}")
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
    # rom-properties:
    # - Removed zlibstatic from INSTALL().
    # - Disabled installation of import libraries.
    # - Install PDB files.
    install(TARGETS zlib
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}" )
        #ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        #LIBRARY DESTINATION "${INSTALL_LIB_DIR}" )
    IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	INCLUDE(DirInstallPaths)
	GET_TARGET_PROPERTY(DEBUG_FILENAME zlib PDB)
	IF(WIN32)
		# TODO: Test on MinGW.
		SET(PDB_FILENAME_D "$<TARGET_FILE_DIR:zlib>/zlib1${CMAKE_DEBUG_POSTFIX}.pdb")
		SET(PDB_FILENAME_R "$<TARGET_FILE_DIR:zlib>/zlib1${CMAKE_RELEASE_POSTFIX}.pdb")
		INSTALL(FILES "${PDB_FILENAME_D}" "${PDB_FILENAME_R}"
			DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
			COMPONENT "debug"
			OPTIONAL
			)
		UNSET(PDB_FILENAME_D)
		UNSET(PDB_FILENAME_R)
	ELSE(WIN32)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
			COMPONENT "debug"
			)
	ENDIF(WIN32)
	UNSET(DEBUG_FILENAME)
     ENDIF(INSTALL_DEBUG)
endif()
if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL )
    install(FILES ${ZLIB_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")
endif()
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
    install(FILES zlib.3 DESTINATION "${INSTALL_MAN_DIR}/man3")
endif()
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
    install(FILES ${ZLIB_PC} DESTINATION "${INSTALL_PKGCONFIG_DIR}")
endif()

#============================================================================
# Example binaries
# (disabled for rom-properties)
#============================================================================
set(SKIP_EXAMPLES 1)

if(NOT SKIP_EXAMPLES)
add_executable(example test/example.c)
target_link_libraries(example zlib)
add_test(example example)

add_executable(minigzip test/minigzip.c)
target_link_libraries(minigzip zlib)

if(HAVE_OFF64_T)
    add_executable(example64 test/example.c)
    target_link_libraries(example64 zlib)
    set_target_properties(example64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
    add_test(example64 example64)

    add_executable(minigzip64 test/minigzip.c)
    target_link_libraries(minigzip64 zlib)
    set_target_properties(minigzip64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
endif()
endif(NOT SKIP_EXAMPLES)
