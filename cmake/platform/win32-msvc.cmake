# Win32-specific CFLAGS/CXXFLAGS.
# For Microsoft Visual C++ compilers.

# If ENABLE_OLDWINCOMPAT, check for incompatible build settings.
# If found, show a warning and disable ENABLE_OLDWINCOMPAT.
IF(ENABLE_OLDWINCOMPAT)
	IF(MSVC_VERSION LESS 1600)
		# ENABLE_OLDWINCOMPAT is not useful on MSVC 2005 or earlier.
		# - MSVC 2005 is the last version to support Win9x.
		# - MSVC 2008 is the last version to support Win2000.
		MESSAGE(WARNING "ENABLE_OLDWINCOMPAT is not useful on MSVC 2005 or earlier.")
		SET(ENABLE_OLDWINCOMPAT OFF CACHE INTERNAL "Enable compatibility with Windows 2000 with MSVC 2010+. (forces static CRT)" FORCE)
	ELSEIF(CMAKE_CL_64)
		MESSAGE(WARNING "ENABLE_OLDWINCOMPAT is not useful for 64-bit builds.")
		SET(ENABLE_OLDWINCOMPAT OFF CACHE INTERNAL "Enable compatibility with Windows 2000 with MSVC 2010+. (forces static CRT)" FORCE)
	ELSE()
		MESSAGE(STATUS "Enabling compatibility with Windows 2000.")
		MESSAGE(STATUS "Static CRT will be used for all targets.")
	ENDIF()
ENDIF(ENABLE_OLDWINCOMPAT)

# NOTE: This program is Unicode only on Windows.
# No ANSI support.

# Subsystem and minimum Windows version:
# - If i386: 5.01
# - If amd64: 5.02
# - If arm or arm64: 6.02
# ROM Properties does NOT support ANSI Windows.
# MSVC 2010's minimum supported target OS is XP SP2.
# MSVC 2012 and later has a minimum subsystem value of 5.01.
# NOTE: CMake sets /subsystem:windows or /subsystem:console itself,
# but with the MSBUILD generator, that's *before* this one, which
# results in console programs being linked as Windows.
# Use the SET_WINDOWS_SUBSYSTEM(_target _subsystem) function defined in
# platform.cmake. That macro will only do anything if building with
# MSVC, since the variables won't be defined anywhere else.
# NOTE: MS_ENH_RSA_AES_PROV is only available starting with
# Windows XP. Because we're actually using some XP-specific
# functionality now, the minimum version is now Windows XP.
INCLUDE(CPUInstructionSetFlags)
IF(CPU_amd64)
	# amd64 (64-bit), Unicode Windows only. (MSVC)
	# (There is no amd64 ANSI Windows.)
	# Minimum target version is Windows Server 2003 / XP 64-bit.
	SET(RP_WIN32_SUBSYSTEM_VERSION "5.02")
ELSEIF(CPU_arm OR CPU_arm64)
	# ARM (32-bit or 64-bit), Unicode windows only. (MSVC)
	# (There is no ARM ANSI Windows.)
	# Minimum target version is Windows 8.
	SET(RP_WIN32_SUBSYSTEM_VERSION "6.02")
ELSEIF(CPU_i386)
	# i386 (32-bit), Unicode Windows only.
	# Minimum target version is Windows XP,
	# unless ENABLE_OLDWINCOMPAT is set.
	# NOTE: MSVC 2012+ doesn't support lower than 5.01.
	# TODO: Win9x support.
	IF(ENABLE_OLDWINCOMPAT)
		SET(RP_WIN32_SUBSYSTEM_VERSION "5.00")
	ELSE(ENABLE_OLDWINCOMPAT)
		SET(RP_WIN32_SUBSYSTEM_VERSION "5.01")
	ENDIF(ENABLE_OLDWINCOMPAT)
ELSE()
	MESSAGE(FATAL_ERROR "Unsupported CPU.")
ENDIF()
SET(RP_LINKER_FLAGS_WIN32_EXE "/SUBSYSTEM:WINDOWS,${RP_WIN32_SUBSYSTEM_VERSION}")
SET(RP_LINKER_FLAGS_CONSOLE_EXE "/SUBSYSTEM:CONSOLE,${RP_WIN32_SUBSYSTEM_VERSION}")
UNSET(RP_WIN32_SUBSYSTEM_VERSION)

# Release build: Prefer static libraries.
IF(CMAKE_BUILD_TYPE MATCHES ^release)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(CMAKE_BUILD_TYPE MATCHES ^release)

# Append the CFLAGS and LDFLAGS.
SET(RP_C_FLAGS_COMMON	"${RP_C_FLAGS_COMMON} ${RP_C_FLAGS_WIN32}")
SET(RP_CXX_FLAGS_COMMON	"${RP_CXX_FLAGS_COMMON} ${RP_C_FLAGS_WIN32} ${RP_CXX_FLAGS_WIN32}")

IF(ENABLE_OLDWINCOMPAT)
	# Always use the static CRT when building for old Windows compatibility.
	SET(RP_C_FLAGS_DEBUG			"${RP_C_FLAGS_DEBUG} /MTd")
	SET(RP_C_FLAGS_RELEASE			"${RP_C_FLAGS_RELEASE} /MT")
	SET(RP_CXX_FLAGS_DEBUG			"${RP_CXX_FLAGS_DEBUG} /MTd")
	SET(RP_CXX_FLAGS_RELEASE		"${RP_CXX_FLAGS_RELEASE} /MT")

	# Remove kernel32.lib from the default libraries list.
	SET(RP_EXE_LINKER_FLAGS_COMMON		"${RP_EXE_LINKER_FLAGS_COMMON} /NODEFAULTLIB:kernel32.lib")
	SET(RP_SHARED_LINKER_FLAGS_COMMON	"${RP_SHARED_LINKER_FLAGS_COMMON} /NODEFAULTLIB:kernel32.lib")
	SET(RP_MODULE_LINKER_FLAGS_COMMON	"${RP_MODULE_LINKER_FLAGS_COMMON} /NODEFAULTLIB:kernel32.lib")

	# Remove kernel32.lib from CMAKE_<lang>_STANDARD_LIBRARIES_INIT.
	STRING(REPLACE "kernel32.lib" "" CSTDLIBS "${CMAKE_C_STANDARD_LIBRARIES}")
	STRING(REPLACE "kernel32.lib" "" CXXSTDLIBS "${CMAKE_CXX_STANDARD_LIBRARIES}")
	SET(CMAKE_C_STANDARD_LIBRARIES "${CSTDLIBS}" CACHE INTERNAL "C standard libraries" FORCE)
	SET(CMAKE_CXX_STANDARD_LIBRARIES "${CXXSTDLIBS}" CACHE INTERNAL "C++ standard libraries" FORCE)
	UNSET(CSTDLIBS)
	UNSET(CXXSTDLIBS)

	# Link everything to liboldwincompat.
	# Add kernel32.lib after oldwincompat to fix linking issues.
	LINK_LIBRARIES(oldwincompat)
	LINK_LIBRARIES(kernel32.lib)
ENDIF(ENABLE_OLDWINCOMPAT)

# Unset temporary variables.
UNSET(RP_C_FLAGS_WIN32)
