# Check for gettext.
# On Windows, we always use our own precompiled gettext.
# On other platforms, we always use the system gettext.

FUNCTION(CHECK_GETTEXT)
	# Intl == runtime library
	# Gettext == compile-time tools
	IF(WIN32)
		# Use our own precompiled version for Windows.
		# NOTE: DirInstallPaths sets ${TARGET_CPU_ARCH}.
		INCLUDE(DirInstallPaths)
		SET(gettext_ROOT "${CMAKE_SOURCE_DIR}/extlib/gettext.win32")
		SET(gettext_BIN "${gettext_ROOT}/bin.${TARGET_CPU_ARCH}")
		SET(gettext_LIB "${gettext_ROOT}/lib.${TARGET_CPU_ARCH}")
		# Use 32-bit executables on all systems.
		SET(gettext_BIN "${gettext_ROOT}/bin.i386")

		SET(Intl_INCLUDE_DIR "${gettext_ROOT}/include" CACHE INTERNAL "libintl include directory." FORCE)
		IF(MSVC)
			# MSVC: Link to the import library
			SET(Intl_LIBRARY "${gettext_LIB}/libgnuintl-8.lib" CACHE INTERNAL "libintl libraries" FORCE)
		ELSE(MSVC)
			# MinGW: Link to the import library (TODO: Link to the DLL?)
			SET(Intl_LIBRARY "${gettext_LIB}/libgnuintl.dll.a" CACHE INTERNAL "libintl libraries" FORCE)
		ENDIF(MSVC)

		# Executables
		# NOTE: If cross-compiling on Linux for Windows, use the native tools.
		IF(CMAKE_CROSSCOMPILING)
			FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
			FIND_PROGRAM(GETTEXT_MSGMERGE_EXECUTABLE msgmerge)
			FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
			FIND_PROGRAM(GETTEXT_MSGINIT_EXECUTABLE msginit)
		ELSE(CMAKE_CROSSCOMPILING)
			SET(GETTEXT_MSGFMT_EXECUTABLE "${gettext_BIN}/msgfmt.exe" CACHE INTERNAL "msgfmt executable" FORCE)
			SET(GETTEXT_MSGMERGE_EXECUTABLE "${gettext_BIN}/msgmerge.exe" CACHE INTERNAL "msgmerge executable" FORCE)
			SET(GETTEXT_XGETTEXT_EXECUTABLE "${gettext_BIN}/xgettext.exe" CACHE INTERNAL "xgettext executable" FORCE)
			SET(GETTEXT_MSGINIT_EXECUTABLE "${gettext_BIN}/msginit.exe" CACHE INTERNAL "msginit executable" FORCE)
		ENDIF(CMAKE_CROSSCOMPILING)

		IF(NOT TARGET libgnuintl_dll_target)
			# Destination directory.
			# If CMAKE_CFG_INTDIR is set, a Debug or Release subdirectory is being used.
			IF(CMAKE_CFG_INTDIR)
				SET(DLL_DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
			ELSE(CMAKE_CFG_INTDIR)
				SET(DLL_DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
			ENDIF(CMAKE_CFG_INTDIR)

			# Copy and install the DLL.
			SET(LIBGNUINTL_DLL "${gettext_LIB}/libgnuintl-8.dll")

			ADD_CUSTOM_TARGET(libgnuintl_dll_target ALL
				DEPENDS libgnuintl_dll_command
				)
			ADD_CUSTOM_COMMAND(OUTPUT libgnuintl_dll_command
				COMMAND ${CMAKE_COMMAND}
				ARGS -E copy_if_different
					"${LIBGNUINTL_DLL}" "${DLL_DESTDIR}/libgnuintl-8.dll"
				DEPENDS libgnuintl_always_rebuild
				)
			ADD_CUSTOM_COMMAND(OUTPUT libgnuintl_always_rebuild
				COMMAND ${CMAKE_COMMAND}
				ARGS -E echo
				)

			INSTALL(FILES "${LIBGNUINTL_DLL}"
				DESTINATION "${DIR_INSTALL_DLL}"
				COMPONENT "dll"
				)
		ENDIF(NOT TARGET libgnuintl_dll_target)

		UNSET(DLL_DESTDIR)
	ELSE(WIN32)
		FIND_PACKAGE(Intl REQUIRED)
		FIND_PACKAGE(Gettext REQUIRED)
		FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
		FIND_PROGRAM(GETTEXT_MSGINIT_EXECUTABLE msginit)
	ENDIF(WIN32)
	SET(HAVE_GETTEXT 1 PARENT_SCOPE)
ENDFUNCTION(CHECK_GETTEXT)
