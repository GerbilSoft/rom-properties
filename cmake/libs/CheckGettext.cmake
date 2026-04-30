# Check for gettext.
# On Windows, we always use our own precompiled gettext.
# On other platforms, we always use the system gettext.

FUNCTION(CHECK_GETTEXT)
	# Intl == runtime library
	# Gettext == compile-time tools
	IF(WIN32)
		# On Windows, we'll use precompiled executables to build the
		# gettext message catalogs, and gettext-runtime sources to
		# build libgnuintl.
		# NOTE: DirInstallPaths sets ${TARGET_CPU_ARCH}.
		INCLUDE(DirInstallPaths)
		SET(gettext_ARCH_DIR ${TARGET_CPU_ARCH})
		IF(gettext_ARCH_DIR STREQUAL arm64ec)
			# arm64ec: Use "arm64" as the directory, since we're using ARM64X.
			SET(gettext_ARCH_DIR arm64)
		ENDIF(gettext_ARCH_DIR STREQUAL arm64ec)

		SET(gettext_ROOT "${CMAKE_SOURCE_DIR}/extlib/gettext.win32")
		#SET(gettext_BIN "${gettext_ROOT}/bin.${gettext_ARCH_DIR}")
		# Use i386 (32-bit) build-time executables on all systems.
		SET(gettext_BIN "${gettext_ROOT}/bin.i386")
		SET(Intl_LIBRARY gnuintl CACHE INTERNAL "libintl library." FORCE)
		SET(USING_INTERNAL_Intl 1 PARENT_SCOPE)

		# Executables
		# NOTE: If cross-compiling on Linux for Windows, use the native tools.
		# On MSVC, CMAKE_CROSSCOMPLING may be set if building for arm/arm64/arm64ec.
		IF(MSVC)
			SET(GETTEXT_MSGFMT_EXECUTABLE "${gettext_BIN}/msgfmt.exe" CACHE INTERNAL "msgfmt executable" FORCE)
			SET(GETTEXT_MSGMERGE_EXECUTABLE "${gettext_BIN}/msgmerge.exe" CACHE INTERNAL "msgmerge executable" FORCE)
			SET(GETTEXT_XGETTEXT_EXECUTABLE "${gettext_BIN}/xgettext.exe" CACHE INTERNAL "xgettext executable" FORCE)
			SET(GETTEXT_MSGINIT_EXECUTABLE "${gettext_BIN}/msginit.exe" CACHE INTERNAL "msginit executable" FORCE)
		ELSE(MSVC)
			FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
			FIND_PROGRAM(GETTEXT_MSGMERGE_EXECUTABLE msgmerge)
			FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
			FIND_PROGRAM(GETTEXT_MSGINIT_EXECUTABLE msginit)
		ENDIF(MSVC)
	ELSE(WIN32)
		FIND_PACKAGE(Intl REQUIRED)
		FIND_PACKAGE(Gettext REQUIRED)
		FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
		FIND_PROGRAM(GETTEXT_MSGINIT_EXECUTABLE msginit)
	ENDIF(WIN32)
	SET(HAVE_GETTEXT 1 PARENT_SCOPE)
ENDFUNCTION(CHECK_GETTEXT)
