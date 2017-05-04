# Win32-specific CFLAGS/CXXFLAGS.

# Basic platform flags:
# - Enable strict type checking in the Windows headers.
# - Define WIN32_LEAN_AND_MEAN to reduce the number of Windows headers included.
# - Define NOMINMAX to disable the MIN() and MAX() macros.
SET(RP_C_FLAGS_WIN32 "-DSTRICT -DWIN32_LEAN_AND_MEAN -DNOMINMAX")

# NOTE: This program only supports Unicode on Windows.
# No support for ANSI Windows, i.e. Win9x.
SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -DUNICODE -D_UNICODE")

# Minimum Windows version for the SDK is Windows XP.
SET(RP_C_FLAGS_WIN32 "${RP_C_FLAGS_WIN32} -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -D_WIN32_IE=0x0600")

# Enable secure template overloads for C++.
# References:
# - MinGW's _mingw_secapi.h
# - http://msdn.microsoft.com/en-us/library/ms175759%28v=VS.100%29.aspx
SET(RP_CXX_FLAGS_WIN32 "-D_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=1")
SET(RP_CXX_FLAGS_WIN32 "${RP_CXX_FLAGS_WIN32} -D_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY=1")
SET(RP_CXX_FLAGS_WIN32 "${RP_CXX_FLAGS_WIN32} -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1")
SET(RP_CXX_FLAGS_WIN32 "${RP_CXX_FLAGS_WIN32} -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1")
SET(RP_CXX_FLAGS_WIN32 "${RP_CXX_FLAGS_WIN32} -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY=1")

# Compiler-specific Win32 flags.
IF(MSVC)
	INCLUDE(cmake/platform/win32-msvc.cmake)
ELSE(MSVC)
	INCLUDE(cmake/platform/win32-gcc.cmake)
ENDIF(MSVC)
