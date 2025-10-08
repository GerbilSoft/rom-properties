# Win32-specific CFLAGS/CXXFLAGS.
# For Microsoft Visual C++ compilers.

# NOTE: This program is Unicode only on Windows.
# No ANSI support.

# Subsystem and minimum Windows version:
# - If i386: 5.01
# - If amd64: 5.02
# - If arm: 6.02
# - If arm64 or arm64ec: 10.00
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
ELSEIF(CPU_arm)
	# ARM (32-bit), Unicode windows only. (MSVC)
	# (There is no ARM ANSI Windows.)
	# Minimum target version is Windows 8.
	SET(RP_WIN32_SUBSYSTEM_VERSION "6.02")
ELSEIF(CPU_arm64 OR CPU_arm64ec)
	# ARM (64-bit), Unicode windows only. (MSVC)
	# Includes both Standard and Emulation Compatible.
	# (There is no ARM ANSI Windows.)
	# Minimum target version is Windows 10.
	SET(RP_WIN32_SUBSYSTEM_VERSION "10.00")
ELSEIF(CPU_i386)
	# i386 (32-bit), Unicode Windows only.
	# Minimum target version is Windows XP.
	SET(RP_WIN32_SUBSYSTEM_VERSION "5.01")
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

# Unset temporary variables.
UNSET(RP_C_FLAGS_WIN32)
