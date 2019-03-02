# Anything that must be linked against the shared C library on Windows must
# be built in this subdirectory, because CMake doesn't allow us to override
# the compiler flags for each build type except at directory scope.  Note
# to CMake developers:  Add a COMPILE_FLAGS_<CONFIG> target property, or
# better yet, provide a friendly way of configuring a Windows target to use the
# static C library.

# rom-properties: This is set in the top-level CMake build system.
IF(0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)
ENDIF(0) # rom-properties

if(MSVC)
  # Build all configurations against shared C library
  foreach(var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    if(${var} MATCHES "/MT")
      string(REGEX REPLACE "/MT" "/MD" ${var} "${${var}}")
    endif()
  endforeach()
endif()

foreach(src ${JPEG_SOURCES})
  set(JPEG_SRCS ${JPEG_SRCS} ../${src})
endforeach()

if(WITH_SIMD AND MSVC_IDE)
  # This tells CMake that the "source" files haven't been generated yet
  set_source_files_properties(${SIMD_OBJS} PROPERTIES GENERATED 1)
endif()

if(WIN32)
  if(WITH_MEM_SRCDST)
    set(DEFFILE ../win/jpeg${SO_MAJOR_VERSION}-memsrcdst.def)
  else()
    set(DEFFILE ../win/jpeg${SO_MAJOR_VERSION}.def)
  endif()
  # rom-properties: Add the custom .rc file in Windows DLL builds.
  SET(LIBJPEG_TURBO_RC "../win/libjpeg-turbo.rc")
endif()
add_library(jpeg SHARED ${JPEG_SRCS} ${DEFFILE} $<TARGET_OBJECTS:simd>
  ${SIMD_OBJS} ${LIBJPEG_TURBO_RC})

set_target_properties(jpeg PROPERTIES SOVERSION ${SO_MAJOR_VERSION}
  VERSION ${SO_MAJOR_VERSION}.${SO_AGE}.${SO_MINOR_VERSION})
if(APPLE AND (NOT CMAKE_OSX_DEPLOYMENT_TARGET OR
              CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER 10.4))
  if(NOT CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG)
    set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-rpath,")
  endif()
  set_target_properties(jpeg PROPERTIES MACOSX_RPATH 1)
endif()
if(MAPFLAG)
  set_target_properties(jpeg PROPERTIES
    LINK_FLAGS "${MAPFLAG}${CMAKE_CURRENT_BINARY_DIR}/../libjpeg.map")
endif()
# rom-properties: Set POSTFIX instead of using RUNTIME_OUTPUT_NAME.
if(MSVC)
  #set_target_properties(jpeg PROPERTIES
  #  RUNTIME_OUTPUT_NAME jpeg${SO_MAJOR_VERSION})
  SET_TARGET_PROPERTIES(jpeg PROPERTIES DEBUG_POSTFIX "${SO_MAJOR_VERSION}${CMAKE_DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(jpeg PROPERTIES RELEASE_POSTFIX "${SO_MAJOR_VERSION}${CMAKE_RELEASE_POSTFIX}")
  # The jsimd_*.c file is built using /MT, so this prevents a linker warning.
  set_target_properties(jpeg PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:LIBCMTD")
elseif(MINGW OR CYGWIN)
  #set_target_properties(jpeg PROPERTIES SUFFIX -${SO_MAJOR_VERSION}.dll)
  SET_TARGET_PROPERTIES(jpeg PROPERTIES DEBUG_POSTFIX "-${SO_MAJOR_VERSION}${CMAKE_DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(jpeg PROPERTIES RELEASE_POSTFIX "-${SO_MAJOR_VERSION}${CMAKE_RELEASE_POSTFIX}")
endif()
# rom-properties: DO_SPLIT_DEBUG(), SET_WINDOWS_SUBSYSTEM()
DO_SPLIT_DEBUG(jpeg)
SET_WINDOWS_SUBSYSTEM(jpeg WIN32)

# rom-properties: Disable examples and tests.
IF(0)
if(WIN32)
  set(USE_SETMODE "-DUSE_SETMODE")
endif()
if(WITH_12BIT)
  set(COMPILE_FLAGS "-DGIF_SUPPORTED -DPPM_SUPPORTED ${USE_SETMODE}")
else()
  set(COMPILE_FLAGS "-DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED -DTARGA_SUPPORTED ${USE_SETMODE}")
  set(CJPEG_BMP_SOURCES ../rdbmp.c ../rdtarga.c)
  set(DJPEG_BMP_SOURCES ../wrbmp.c ../wrtarga.c)
endif()

add_executable(cjpeg ../cjpeg.c ../cdjpeg.c ../rdgif.c ../rdppm.c
  ../rdswitch.c ${CJPEG_BMP_SOURCES})
set_property(TARGET cjpeg PROPERTY COMPILE_FLAGS ${COMPILE_FLAGS})
target_link_libraries(cjpeg jpeg)

add_executable(djpeg ../djpeg.c ../cdjpeg.c ../rdcolmap.c ../rdswitch.c
  ../wrgif.c ../wrppm.c ${DJPEG_BMP_SOURCES})
set_property(TARGET djpeg PROPERTY COMPILE_FLAGS ${COMPILE_FLAGS})
target_link_libraries(djpeg jpeg)

add_executable(jpegtran ../jpegtran.c ../cdjpeg.c ../rdswitch.c ../transupp.c)
target_link_libraries(jpegtran jpeg)
set_property(TARGET jpegtran PROPERTY COMPILE_FLAGS "${USE_SETMODE}")

add_executable(jcstest ../jcstest.c)
target_link_libraries(jcstest jpeg)

ENDIF(0) # rom-properties

# rom-properties: Use our own INSTALL().
IF(0)
install(TARGETS jpeg cjpeg djpeg jpegtran
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(NOT CMAKE_VERSION VERSION_LESS "3.1" AND MSVC AND
  CMAKE_C_LINKER_SUPPORTS_PDB)
  install(FILES "$<TARGET_PDB_FILE:jpeg>"
    DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()
ENDIF(0) # rom-properties

# rom-properties: Use our own INSTALL().
INCLUDE(DirInstallPaths)
INSTALL(TARGETS jpeg
	RUNTIME DESTINATION ${DIR_INSTALL_DLL}
	#LIBRARY DESTINATION ${DIR_INSTALL_LIB}
	#ARCHIVE DESTINATION ${DIR_INSTALL_LIB}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	# TODO: Test on MinGW.
	# FIXME: Generator PDB expression doesn't have the debug postfix for some reason,
	# so cpack fails in debug builds if we get the PDB property.
	SET(PDB_FILENAME_D "$<TARGET_FILE_DIR:jpeg>/jpeg${SO_MAJOR_VERSION}${CMAKE_DEBUG_POSTFIX}.pdb")
	SET(PDB_FILENAME_R "$<TARGET_FILE_DIR:jpeg>/jpeg${SO_MAJOR_VERSION}${CMAKE_RELEASE_POSTFIX}.pdb")
	INSTALL(FILES "${PDB_FILENAME_D}" "${PDB_FILENAME_R}"
		DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
		COMPONENT "debug"
		OPTIONAL
		)
	UNSET(PDB_FILENAME_D)
	UNSET(PDB_FILENAME_R)
ENDIF(INSTALL_DEBUG)
