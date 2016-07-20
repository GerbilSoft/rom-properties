PROJECT(src)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

# C++11 compatibility header.
# NOTE: This must be included regardless of C++11 support in the compiler.
# gcc-4.6 supports some C++11, but is missing explicit virtual overrides.
IF(MSVC)
	SET(RP_CXX11_COMPAT_HEADER "-FI${CMAKE_CURRENT_SOURCE_DIR}/c++11-compat.h")
ELSE(MSVC)
	SET(RP_CXX11_COMPAT_HEADER "-include ${CMAKE_CURRENT_SOURCE_DIR}/c++11-compat.h")
ENDIF(MSVC)
SET(RP_CXXFLAG_CXX11_COMPAT ${RP_CXX11_COMPAT_HEADER})

# Add the C++11 compatibility header to CFLAGS/CXXFLAGS.
SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} ${RP_CXX11_COMPAT_HEADER}")
SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${RP_CXXFLAG_CXX11_COMPAT}")

# Source Code subdirectories.
ADD_SUBDIRECTORY(libromdata)
ADD_SUBDIRECTORY(kde)
