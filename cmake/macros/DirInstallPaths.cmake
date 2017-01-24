# Directory install paths.
# Files are installed in different directories depending
# on platform, e.g. Unix-style for Linux and most other
# Unix systems.

# NOTE: DIR_INSTALL_DOC_ROOT is for documents that should
# be in the root of the Windows ZIP file. On other platforms,
# it's the same as DIR_INSTALL_DOC.

IF(NOT PACKAGE_NAME)
	MESSAGE(FATAL_ERROR "PACKAGE_NAME is not set.")
ENDIF(NOT PACKAGE_NAME)

# Architecture name for arch-specific paths.
STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch)
IF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")
	# i386/amd64. Check sizeof(void*) for the actual architecture,
	# since building 32-bit on 64-bit isn't considered "cross-compiling",
	# so CMAKE_SYSTEM_PROCESSOR might not be accurate.
	# NOTE: Checking CMAKE_CL_64 instead of sizeof(void*) for MSVC builds.
	IF(MSVC AND CMAKE_CL_64)
		SET(arch "amd64")
	ELSEIF(NOT MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		SET(arch "amd64")
	ELSE()
		SET(arch "i386")
	ENDIF()
ENDIF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")

INCLUDE(GNUInstallDirs)
IF(UNIX AND NOT APPLE)
	# Unix-style install paths.
	# NOTE: CMake doesn't use Debian-style multiarch for libexec,
	# so we have to check for that.
	IF(CMAKE_INSTALL_LIBDIR MATCHES .*gnu.*)
		SET(CMAKE_INSTALL_LIBEXECDIR "${CMAKE_INSTALL_LIBDIR}/libexec" CACHE "libexec override for Debian multi-arch" INTERNAL FORCE)
		SET(CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBDIR}/libexec" CACHE "libexec override for Debian multi-arch" INTERNAL FORCE)
	ENDIF()
	SET(DIR_INSTALL_EXE "${CMAKE_INSTALL_BINDIR}")
	SET(DIR_INSTALL_DLL "${CMAKE_INSTALL_LIBDIR}")
	SET(DIR_INSTALL_LIB "${CMAKE_INSTALL_LIBDIR}")
	SET(DIR_INSTALL_LIBEXEC "${CMAKE_INSTALL_LIBEXECDIR}")
	SET(DIR_INSTALL_TRANSLATIONS "share/${PACKAGE_NAME}/translations")
	SET(DIR_INSTALL_DOC "share/doc/${PACKAGE_NAME}")
	SET(DIR_INSTALL_DOC_ROOT "${DIR_INSTALL_DOC}")
	SET(DIR_INSTALL_EXE_DEBUG "lib/debug/${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_EXE}")
	SET(DIR_INSTALL_DLL_DEBUG "lib/debug/${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_DLL}")
	SET(DIR_INSTALL_LIB_DEBUG "lib/debug/${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIB}")
	SET(DIR_INSTALL_LIBEXEC_DEBUG "lib/debug/${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIBEXEC}")
ELSEIF(APPLE)
	# Mac OS X-style install paths.
	# Install should be relative to the application bundle.
	# TODO: Not supported...
	MESSAGE(STATUS "WARNING: Mac OS X is not officially supported yet.")
ELSEIF(WIN32)
	# Win32-style install paths.
	# Files are installed relative to root, since the
	# program is run out of its own directory.
	SET(DIR_INSTALL_EXE "${arch}")
	SET(DIR_INSTALL_DLL "${arch}")
	SET(DIR_INSTALL_LIB "${arch}")
	SET(DIR_INSTALL_LIBEXEC "${arch}")
	SET(DIR_INSTALL_TRANSLATIONS "translations")
	SET(DIR_INSTALL_DOC "doc")
	SET(DIR_INSTALL_DOC_ROOT ".")
	SET(DIR_INSTALL_EXE_DEBUG "debug")
	# Installing debug symbols for DLLs in the
	# same directory as the DLL.
	SET(DIR_INSTALL_EXE_DEBUG "${DIR_INSTALL_EXE}")
	SET(DIR_INSTALL_DLL_DEBUG "${DIR_INSTALL_DLL}")
	SET(DIR_INSTALL_LIB_DEBUG "${DIR_INSTALL_LIB}")
	SET(DIR_INSTALL_LIBEXEC_DEBUG "${DIR_INSTALL_LIBEXEC}")
ELSE()
	MESSAGE(WARNING "Installation paths have not been set up for this system.")
ENDIF()

UNSET(arch)
