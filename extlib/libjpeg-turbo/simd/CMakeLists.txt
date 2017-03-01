# rom-properties: Use FIND_PROGRAM() for nasm.
#if(NOT DEFINED NASM)
#  set(NASM nasm CACHE FILEPATH "Path to NASM/YASM executable")
#endif()
IF(WIN32)
	# Search for NASM in a few places:
	# - %LOCALAPPDATA% (if set)
	# - %ProgramFiles(x86)%/NASM (if set)
	# - %ProgramFiles%/NASM

	IF(DEFINED ENV{LOCALAPPDATA})
		# Use LOCALAPPDATA.
		# TODO: Determine LOCALAPPDATA on XP?
		# NASM only uses it on Win7 (and probably Vista), though...
		SET(LocalAppData "$ENV{LOCALAPPDATA}")
	ENDIF(DEFINED ENV{LOCALAPPDATA})

	# NOTE: Due to CMP0053, we can't directly reference
	# the "ProgramFiles(x86)" variable.
	IF(DEFINED ENV{ProgramW6432})
		# 64-bit Windows.
		# Use %ProgramFiles(x86)% for 32-bit,
		# and %ProgramW6432% for 64-bit.
		SET(MYENV "ProgramFiles(x86)")
		SET(ProgramFilesX86 "$ENV{${MYENV}}")
		SET(ProgramFiles "$ENV{ProgramFilesW6432}")
	ELSE(DEFINED ENV{ProgramW6432})
		# 32-bit Windows. Use %ProgramFiles% directly.
		SET(ProgramFiles "$ENV{ProgramFiles}")
	ENDIF(DEFINED ENV{ProgramW6432})

	FIND_PROGRAM(NASM nasm
		PATHS	"${LocalAppData}/NASM"
			"${ProgramFiles}/NASM"
			"${ProgramFilesX86}/NASM"
		)
ELSE(WIN32)
	# Not Windows. Just search for nasm directly.
	FIND_PROGRAM(NASM nasm)
ENDIF(WIN32)
IF(NOT NASM)
	MESSAGE(FATAL_ERROR "NASM is required for libjpeg-turbo SIMD functions.")
ENDIF(NOT NASM)

# rom-properties: Replace ${CMAKE_SOURCE_DIR} with ${TOPSRCDIR},
# since libjpeg-turbo isn't the base project.
SET(TOPSRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(SIMD_X86_64)
  set(NAFLAGS -fwin64 -DWIN64 -D__x86_64__)
else()
  if(BORLAND)
    set(NAFLAGS -fobj -DOBJ32)
  else()
    set(NAFLAGS -fwin32 -DWIN32)
  endif()
endif()
set(NAFLAGS ${NAFLAGS} -I${TOPSRCDIR}/win/ -I${CMAKE_CURRENT_SOURCE_DIR}/)

# This only works if building from the command line.  There is currently no way
# to set a variable's value based on the build type when using the MSVC IDE.
if(CMAKE_BUILD_TYPE STREQUAL "Debug"
  OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(NAFLAGS ${NAFLAGS} -g)
endif()

if(SIMD_X86_64)
  set(SIMD_BASENAMES jfdctflt-sse-64 jccolor-sse2-64 jcgray-sse2-64
    jchuff-sse2-64 jcsample-sse2-64 jdcolor-sse2-64 jdmerge-sse2-64
    jdsample-sse2-64 jfdctfst-sse2-64 jfdctint-sse2-64 jidctflt-sse2-64
    jidctfst-sse2-64 jidctint-sse2-64 jidctred-sse2-64 jquantf-sse2-64
    jquanti-sse2-64)
  message(STATUS "Building x86_64 SIMD extensions")
else()
  set(SIMD_BASENAMES jsimdcpu jfdctflt-3dn jidctflt-3dn jquant-3dn jccolor-mmx
    jcgray-mmx jcsample-mmx jdcolor-mmx jdmerge-mmx jdsample-mmx jfdctfst-mmx
    jfdctint-mmx jidctfst-mmx jidctint-mmx jidctred-mmx jquant-mmx jfdctflt-sse
    jidctflt-sse jquant-sse jccolor-sse2 jcgray-sse2 jchuff-sse2 jcsample-sse2
    jdcolor-sse2 jdmerge-sse2 jdsample-sse2 jfdctfst-sse2 jfdctint-sse2
    jidctflt-sse2 jidctfst-sse2 jidctint-sse2 jidctred-sse2 jquantf-sse2
    jquanti-sse2)
  message(STATUS "Building i386 SIMD extensions")
endif()

if(MSVC_IDE)
  set(OBJDIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
else()
  set(OBJDIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

file(GLOB INC_FILES *.inc)

foreach(file ${SIMD_BASENAMES})
  set(DEPFILE "")
  set(SIMD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${file}.asm)
  if(${file} MATCHES jccolor)
    set(DEPFILE ${file})
    string(REGEX REPLACE "jccolor" "jccolext" DEPFILE ${DEPFILE})
    set(DEPFILE ${CMAKE_CURRENT_SOURCE_DIR}/${DEPFILE}.asm)
  endif()
  if(${file} MATCHES jcgray)
    set(DEPFILE ${file})
    string(REGEX REPLACE "jcgray" "jcgryext" DEPFILE ${DEPFILE})
    set(DEPFILE ${CMAKE_CURRENT_SOURCE_DIR}/${DEPFILE}.asm)
  endif()
  if(${file} MATCHES jdcolor)
    set(DEPFILE ${file})
    string(REGEX REPLACE "jdcolor" "jdcolext" DEPFILE ${DEPFILE})
    set(DEPFILE ${CMAKE_CURRENT_SOURCE_DIR}/${DEPFILE}.asm)
  endif()
  if(${file} MATCHES jdmerge)
    set(DEPFILE ${file})
    string(REGEX REPLACE "jdmerge" "jdmrgext" DEPFILE ${DEPFILE})
    set(DEPFILE ${CMAKE_CURRENT_SOURCE_DIR}/${DEPFILE}.asm)
  endif()
  set(SIMD_OBJ ${OBJDIR}/${file}.obj)
  add_custom_command(OUTPUT ${SIMD_OBJ}
    DEPENDS ${SIMD_SRC} ${DEPFILE} ${INC_FILES}
    COMMAND ${NASM} ${NAFLAGS} ${SIMD_SRC} -o${SIMD_OBJ})
  set(SIMD_OBJS ${SIMD_OBJS} ${SIMD_OBJ})
endforeach()

set(SIMD_OBJS ${SIMD_OBJS} PARENT_SCOPE)
add_custom_target(simd DEPENDS ${SIMD_OBJS})
