# Win32-specific CFLAGS/CXXFLAGS.
# For MinGW compilers.

# Basic platform flags for gcc:
# - wchar_t is short.
# - Enable MSVC 2005 compatibility. (In MinGW-w64 v4.0.2, this enables 64-bit time_t.)
SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -D__MINGW_USE_VC2005_COMPAT")

# NOTE: This program is Unicode only on Windows.
# No ANSI support.

# Subsystem and minimum Windows version:
# - If 32-bit: 5.00
# - If 64-bit: 5.02
# TODO: Does CMAKE_CREATE_*_EXE also affect DLLs?
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	# 64-bit, Unicode Windows only.
	# (There is no 64-bit ANSI Windows.)
	SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -D_WIN32_WINNT=0x0502")
	SET(CMAKE_CREATE_WIN32_EXE "-Wl,--subsystem,windows:5.02")
	SET(CMAKE_CREATE_CONSOLE_EXE "-Wl,--subsystem,console:5.02")
ELSE()
	# 32-bit, Unicode Windows only.
	SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -D_WIN32_WINNT=0x0500")
	SET(CMAKE_CREATE_WIN32_EXE "-Wl,--subsystem,windows:5.00")
	SET(CMAKE_CREATE_CONSOLE_EXE "-Wl,--subsystem,console:5.00")
ENDIF()

SET(RP_EXE_LINKER_FLAGS_WIN32 "")
SET(RP_SHARED_LINKER_FLAGS_WIN32 "")

# Release build: Prefer static libraries.
IF(CMAKE_BUILD_TYPE MATCHES ^release)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(CMAKE_BUILD_TYPE MATCHES ^release)

# Test for various linker flags.
# NOTE: Technically, --tsaware is only valid for EXEs, not DLLs,
# but we should mark DLLs anyway.
# TODO: Make static linkage a CMake option: --static-libgcc, --static-libstdc++
FOREACH(FLAG_TEST "-Wl,--large-address-aware" "-Wl,--nxcompat" "-Wl,--tsaware")
	# CMake doesn't like "+" characters in variable names.
	STRING(REPLACE "+" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	IF(LDFLAG_${FLAG_TEST_VARNAME})
		SET(RP_EXE_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32} ${FLAG_TEST}")
	ENDIF(LDFLAG_${FLAG_TEST_VARNAME})
	UNSET(LDFLAG_${FLAG_TEST_VARNAME})
	UNSET(FLAG_TEST_VARNAME)
ENDFOREACH()
SET(RP_SHARED_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32}")

# Test for dynamicbase (ASLR) support.
# Simply enabling --dynamicbase won't work; we also need to
# tell `ld` to generate the .reloc section. Also, there's
# a bug in `ld` where if it generates the .reloc section,
# it conveniently forgets the entry point.
# Reference: https://lists.libav.org/pipermail/libav-devel/2014-October/063871.html
INCLUDE(CheckSystemX8632)
CHECK_SYSTEM_X86_32(MINGW_IS_CPU_X86_32)
# TODO: Does ARM Windows have a leading underscore?
IF(MINGW_IS_CPU_X86_32)
	SET(ENTRY_POINT "_mainCRTStartup")
ELSE(MINGW_IS_CPU_X86_32)
	SET(ENTRY_POINT "mainCRTStartup")
ENDIF(MINGW_IS_CPU_X86_32)

FOREACH(FLAG_TEST "-Wl,--dynamicbase,--pic-executable")
	# CMake doesn't like "+" characters in variable names.
	STRING(REPLACE "+" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" LDFLAG_${FLAG_TEST_VARNAME})
	IF(LDFLAG_${FLAG_TEST_VARNAME})
		# Entry point is only set for EXEs.
		# GNU `ld` always has the -e option.
		SET(RP_EXE_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32} ${FLAG_TEST} -Wl,-e,${ENTRY_POINT}")
		SET(RP_SHARED_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32} ${FLAG_TEST}")
	ENDIF(LDFLAG_${FLAG_TEST_VARNAME})
	UNSET(LDFLAG_${FLAG_TEST_VARNAME})
	UNSET(FLAG_TEST_VARNAME)
ENDFOREACH()

# Enable windres support on MinGW.
# http://www.cmake.org/Bug/view.php?id=4068
SET(CMAKE_RC_COMPILER_INIT windres)
ENABLE_LANGUAGE(RC)

# NOTE: Setting CMAKE_RC_OUTPUT_EXTENSION doesn't seem to work.
# Force windres to output COFF, even though it'll use the .res extension.
SET(CMAKE_RC_OUTPUT_EXTENSION .obj)
SET(CMAKE_RC_COMPILE_OBJECT
	"<CMAKE_RC_COMPILER> --output-format=coff <FLAGS> <DEFINES> -o <OBJECT> <SOURCE>")

# Append the CFLAGS and LDFLAGS.
SET(RP_C_FLAGS_COMMON			"${RP_C_FLAGS_COMMON} ${RP_C_FLAGS_WIN32}")
SET(RP_CXX_FLAGS_COMMON			"${RP_CXX_FLAGS_COMMON} ${RP_C_FLAGS_WIN32} ${RP_CXX_FLAGS_WIN32}")
SET(RP_EXE_LINKER_FLAGS_COMMON		"${RP_EXE_LINKER_FLAGS_COMMON} ${RP_EXE_LINKER_FLAGS_WIN32}")
SET(RP_SHARED_LINKER_FLAGS_COMMON	"${RP_SHARED_LINKER_FLAGS_COMMON} ${RP_SHARED_LINKER_FLAGS_WIN32}")

# Unset temporary variables.
UNSET(RP_C_FLAGS_WIN32)
UNSET(RP_EXE_LINKER_FLAGS_WIN32)
