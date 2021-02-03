# Find SECCOMP libraries and headers.
# If found, the following variables will be defined:
# - SECCOMP_FOUND: System has SECCOMP.
# - SECCOMP_INCLUDE_DIRS: SECCOMP include directories.
# - SECCOMP_LIBRARIES: SECCOMP libraries.
# - SECCOMP_DEFINITIONS: Compiler switches required for using SECCOMP.
#
# In addition, a target Seccomp::seccomp will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(SECCOMP
	libseccomp		# pkgconfig
	seccomp.h		# header
	seccomp			# library
	Seccomp::seccomp	# imported target
	)
