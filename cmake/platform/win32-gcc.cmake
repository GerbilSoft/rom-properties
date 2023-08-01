# Win32-specific CFLAGS/CXXFLAGS.
# For MinGW compilers.

# Enable "secure" API functions: *_s()
ADD_DEFINITIONS(-DMINGW_HAS_SECURE_API)

# Subsystem and minimum Windows version:
# - If i386: 5.01
# - If amd64: 5.02
# - If arm or arm64: 6.02
# NOTE: MS_ENH_RSA_AES_PROV is only available starting with
# Windows XP. Because we're actually using some XP-specific
# functionality now, the minimum version is now Windows XP.
INCLUDE(CPUInstructionSetFlags)
IF(CPU_amd64)
	# amd64 (64-bit), Unicode Windows only.
	# (There is no amd64 ANSI Windows.)
	# Minimum target version is Windows Server 2003 / XP 64-bit.
	SET(RP_WIN32_SUBSYSTEM_VERSION "5.02")
ELSEIF(CPU_arm OR CPU_arm64)
	# ARM (32-bit or 64-bit), Unicode windows only. (MSVC)
	# (There is no ARM ANSI Windows.)
	# Minimum target version is Windows 8.
	SET(RP_WIN32_SUBSYSTEM_VERSION "6.02")
ELSEIF(CPU_i386)
	# 32-bit, Unicode Windows only.
	# Minimum target version is Windows XP.
	SET(RP_WIN32_SUBSYSTEM_VERSION "5.01")
ELSE()
	MESSAGE(FATAL_ERROR "Unsupported CPU.")
ENDIF()
# FIXME: Maybe we should use RP_LINKER_FLAGS_WIN32_EXE and RP_LINKER_FLAGS_CONSOLE_EXE.
# This is what's used in win32-msvc.cmake.
SET(CMAKE_CREATE_WIN32_EXE "-Wl,--subsystem,windows:${RP_WIN32_SUBSYSTEM_VERSION}")
SET(CMAKE_CREATE_CONSOLE_EXE "-Wl,--subsystem,console:${RP_WIN32_SUBSYSTEM_VERSION}")

SET(RP_EXE_LINKER_FLAGS_WIN32 "")
SET(RP_SHARED_LINKER_FLAGS_WIN32 "")
SET(RP_MODULE_LINKER_FLAGS_WIN32 "")

# Release build: Prefer static libraries.
IF(CMAKE_BUILD_TYPE MATCHES ^release)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(CMAKE_BUILD_TYPE MATCHES ^release)

# Test for common LDFLAGS.
# NOTE: CHECK_C_COMPILER_FLAG() doesn't seem to work, even with
# CMAKE_TRY_COMPILE_TARGET_TYPE. Check `ld --help` for the various
# parameters instead.

# NOTE: --tsaware is only valid for EXEs, not DLLs.
# TODO: Make static linkage a CMake option: --static-libgcc, --static-libstdc++
EXECUTE_PROCESS(COMMAND ${CMAKE_LINKER} --help
	OUTPUT_VARIABLE _ld_out
	ERROR_QUIET)

# NOTE: Newer ld shows things like "--[disable-]dynamicbase".
IF(CPU_i386 OR CPU_arm)
	# 32-bit only LDFLAGS
	FOREACH(FLAG_TEST "large-address-aware")
		IF(NOT DEFINED LDFLAG_${FLAG_TEST})
			MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST}")
			IF(_ld_out MATCHES "${FLAG_TEST}")
				MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST} - yes")
				SET(LDFLAG_${FLAG_TEST} 1 CACHE INTERNAL "Linker supports --${FLAG_TEST}")
			ELSE()
				MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST} - no")
				SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports --${FLAG_TEST}")
			ENDIF()
		ENDIF()

		IF(LDFLAG_${FLAG_TEST})
			SET(RP_EXE_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32} -Wl,--${FLAG_TEST}")
		ENDIF(LDFLAG_${FLAG_TEST})
	ENDFOREACH()
ELSEIF(CPU_amd64 OR CPU_arm64)
	# 64-bit only LDFLAGS
	FOREACH(FLAG_TEST "high-entropy-va")
		IF(NOT DEFINED LDFLAG_${FLAG_TEST})
			MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST}")
			IF(_ld_out MATCHES "${FLAG_TEST}")
				MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST} - yes")
				SET(LDFLAG_${FLAG_TEST} 1 CACHE INTERNAL "Linker supports --${FLAG_TEST}")
			ELSE()
				MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST} - no")
				SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports --${FLAG_TEST}")
			ENDIF()
		ENDIF()

		IF(LDFLAG_${FLAG_TEST})
			SET(RP_EXE_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32} -Wl,--${FLAG_TEST}")
		ENDIF(LDFLAG_${FLAG_TEST})
	ENDFOREACH()
ENDIF()

# CPU-independent LDFLAGS
FOREACH(FLAG_TEST "dynamicbase" "nxcompat")
	IF(NOT DEFINED LDFLAG_${FLAG_TEST})
		MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST}")
		IF(_ld_out MATCHES "${FLAG_TEST}")
			MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST} - yes")
			SET(LDFLAG_${FLAG_TEST} 1 CACHE INTERNAL "Linker supports --${FLAG_TEST}")
		ELSE()
			MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST} - no")
			SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports --${FLAG_TEST}")
		ENDIF()
	ENDIF()

	IF(LDFLAG_${FLAG_TEST})
		SET(RP_EXE_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32} -Wl,--${FLAG_TEST}")
	ENDIF(LDFLAG_${FLAG_TEST})
ENDFOREACH()
SET(RP_SHARED_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32}")
SET(RP_MODULE_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32}")

# EXE-only flags.
FOREACH(FLAG_TEST "tsaware")
	IF(NOT DEFINED LDFLAG_${FLAG_TEST})
		MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST}")
		IF(_ld_out MATCHES "${FLAG_TEST}")
			MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST} - yes")
			SET(LDFLAG_${FLAG_TEST} 1 CACHE INTERNAL "Linker supports --${FLAG_TEST}")
		ELSE()
			MESSAGE(STATUS "Checking if ld supports --${FLAG_TEST} - no")
			SET(LDFLAG_${FLAG_TEST} "" CACHE INTERNAL "Linker supports --${FLAG_TEST}")
		ENDIF()
	ENDIF()

	IF(LDFLAG_${FLAG_TEST})
		SET(RP_EXE_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32} -Wl,--${FLAG_TEST}")
	ENDIF(LDFLAG_${FLAG_TEST})
ENDFOREACH()

# Test for dynamicbase (ASLR) support.
# Simply enabling --dynamicbase won't work; we also need to
# tell `ld` to generate the .reloc section. Also, there's
# a bug in `ld` where if it generates the .reloc section,
# it conveniently forgets the entry point.
# Reference: https://lists.libav.org/pipermail/libav-devel/2014-October/063871.html
# NOTE: Entry point is set using SET_WINDOWS_ENTRYPOINT()
# in platform.cmake due to ANSI/Unicode differences.
FOREACH(FLAG_TEST "-Wl,--dynamicbase,--pic-executable")
	# CMake doesn't like "+" characters in variable names.
	STRING(REPLACE "+" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" LDFLAG_${FLAG_TEST_VARNAME})
	IF(LDFLAG_${FLAG_TEST_VARNAME})
		# Entry point is only set for EXEs.
		# GNU `ld` always has the -e option.
		SET(RP_EXE_LINKER_FLAGS_WIN32 "${RP_EXE_LINKER_FLAGS_WIN32} ${FLAG_TEST}")
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
	"<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> <INCLUDES> -o <OBJECT> <SOURCE>")

# Append the CFLAGS and LDFLAGS.
SET(RP_C_FLAGS_COMMON			"${RP_C_FLAGS_COMMON} ${RP_C_FLAGS_WIN32}")
SET(RP_CXX_FLAGS_COMMON			"${RP_CXX_FLAGS_COMMON} ${RP_C_FLAGS_WIN32} ${RP_CXX_FLAGS_WIN32}")
SET(RP_EXE_LINKER_FLAGS_COMMON		"${RP_EXE_LINKER_FLAGS_COMMON} ${RP_EXE_LINKER_FLAGS_WIN32}")
SET(RP_SHARED_LINKER_FLAGS_COMMON	"${RP_SHARED_LINKER_FLAGS_COMMON} ${RP_SHARED_LINKER_FLAGS_WIN32}")
SET(RP_MODULE_LINKER_FLAGS_COMMON	"${RP_MODULE_LINKER_FLAGS_COMMON} ${RP_MODULE_LINKER_FLAGS_WIN32}")

# Unset temporary variables.
UNSET(RP_C_FLAGS_WIN32)
UNSET(RP_EXE_LINKER_FLAGS_WIN32)
UNSET(RP_SHARED_LINKER_FLAGS_WIN32)
UNSET(RP_MODULE_LINKER_FLAGS_WIN32)
