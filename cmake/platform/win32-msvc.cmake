# Win32-specific CFLAGS/CXXFLAGS.
# For Microsoft Visual C++ compilers.

# Basic platform flags for MSVC:
# - wchar_t should be a distinct type. (MSVC 2002+)
IF(MSVC_VERSION GREATER 1200)
	SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -Zc:wchar_t")
ENDIF()

# NOTE: This program is Unicode only on Windows.
# No ANSI support.

# Subsystem and minimum Windows version:
# - If 32-bit: 5.00
# - If 64-bit: 5.02
# NOTE: MSVC 2012 and later has a minimum subsystem value of 5.01.
# NOTE: CMake sets /subsystem:windows or /subsystem:console itself,
# but with the MSBUILD generator, that's *before* this one, which
# results in console programs being linked as Windows.
# Use the SET_WINDOWS_SUBSYSTEM(_target _subsystem) function defined in
# platform.cmake. That macro will only do anything if building with
# MSVC, since the variables won't be defined anywhere else.
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	# 64-bit, Unicode Windows only.
	# (There is no 64-bit ANSI Windows.)
	SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -DWINVER=0x0502 -D_WIN32_WINNT=0x0502 -D_WIN32_IE=0x0600")
	SET(RP_LINKER_FLAGS_WIN32_EXE "/SUBSYSTEM:WINDOWS,5.02")
	SET(RP_LINKER_FLAGS_CONSOLE_EXE "/SUBSYSTEM:CONSOLE,5.02")
ELSE()
	# 32-bit, Unicode Windows only.
	IF(MSVC_VERSION GREATER 1600)
		# MSVC 2012 or later.
		# Minimum target version is Windows XP.
		SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -D_WIN32_IE=0x0600")
		SET(RP_LINKER_FLAGS_WIN32_EXE "/SUBSYSTEM:WINDOWS,5.01")
		SET(RP_LINKER_FLAGS_CONSOLE_EXE "/SUBSYSTEM:CONSOLE,5.01")
	ELSE()
		# MSVC 2010 or earlier.
		# Minimum target version is Windows 2000.
		SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0500")
		SET(RP_LINKER_FLAGS_WIN32_EXE "/SUBSYSTEM:WINDOWS,5.00")
		SET(RP_LINKER_FLAGS_CONSOLE_EXE "/SUBSYSTEM:CONSOLE,5.00")
	ENDIF()
ENDIF()

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
