# Win32-specific CFLAGS/CXXFLAGS.
# For Microsoft Visual C++ compilers.

# Basic platform flags for MSVC:
# - wchar_t should be a distinct type. (MSVC 2002+)
IF(MSVC_VERSION GREATER 1200)
	SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -Zc:wchar_t")
ENDIF()

# NOTE: This program is Unicode only on Windows.
# No ANSI support.

# Minimum Windows version for the SDK is Windows XP.
SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -D_WIN32_IE=0x0600")

# Subsystem and minimum Windows version:
# - If 32-bit: 5.01
# - If 64-bit: 5.02
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
IF(MSVC AND CMAKE_CL_64)
	# 64-bit, Unicode Windows only. (MSVC)
	# (There is no 64-bit ANSI Windows.)
	# Minimum target version is Windows Server 2003 / XP 64-bit.
	SET(RP_WIN32_SUBSYSTEM_VERSION "5.02")
ELSEIF(NOT MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	# 64-bit, Unicode Windows only. (MinGW)
	# (There is no 64-bit ANSI Windows.)
	# Minimum target version is Windows Server 2003 / XP 64-bit.
	SET(RP_WIN32_SUBSYSTEM_VERSION "5.02")
ELSE()
	# 32-bit, Unicode Windows only.
	# Minimum target version is Windows XP.
	SET(RP_WIN32_SUBSYSTEM_VERSION "5.01")
ENDIF()
SET(RP_LINKER_FLAGS_WIN32_EXE "/SUBSYSTEM:WINDOWS,${RP_WIN32_SUBSYSTEM_VERSION}")
SET(RP_LINKER_FLAGS_CONSOLE_EXE "/SUBSYSTEM:CONSOLE,${RP_WIN32_SUBSYSTEM_VERSION}")
UNSET(RP_WIN32_SUBSYSTEM_VERSION)

# Release build: Prefer static libraries.
IF(CMAKE_BUILD_TYPE MATCHES ^release)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(CMAKE_BUILD_TYPE MATCHES ^release)

# Append the CFLAGS and LDFLAGS.
SET(RP_C_FLAGS_COMMON			"${RP_C_FLAGS_COMMON} ${RP_C_FLAGS_WIN32}")
SET(RP_CXX_FLAGS_COMMON			"${RP_CXX_FLAGS_COMMON} ${RP_C_FLAGS_WIN32} ${RP_CXX_FLAGS_WIN32}")
SET(RP_EXE_LINKER_FLAGS_COMMON		"${RP_EXE_LINKER_FLAGS_COMMON} ${RP_EXE_LINKER_FLAGS_WIN32}")
SET(RP_SHARED_LINKER_FLAGS_COMMON	"${RP_SHARED_LINKER_FLAGS_COMMON} ${RP_EXE_LINKER_FLAGS_WIN32}")
SET(RP_MODULE_LINKER_FLAGS_COMMON	"${RP_MODULE_LINKER_FLAGS_COMMON} ${RP_EXE_LINKER_FLAGS_WIN32}")

# Unset temporary variables.
UNSET(RP_C_FLAGS_WIN32)
UNSET(RP_EXE_LINKER_FLAGS_WIN32)
