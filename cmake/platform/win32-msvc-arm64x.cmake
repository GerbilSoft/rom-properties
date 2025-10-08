# Reference: https://learn.microsoft.com/en-us/windows/arm/arm64x-build
# NOTE: We're still using MSVC 17.6.5 for Windows 7 compatibility,
# so we can't use /LINKREPROFULLPATHRSP.

# directory where the link_repro directories for each arm64x target will be created during arm64 build.
# NOTE: Expecting a directory tree setup similar to package.cmd.
IF(BUILD_AS_ARM64X STREQUAL "ARM64")
	# ARM64: Repros should be here.
	SET(arm64ReproDir "${CMAKE_BINARY_DIR}/repros")
ELSEIF(BUILD_AS_ARM64X STREQUAL "ARM64EC")
	# ARM64EC: Repros are in the build.arm64/ directory.
	CMAKE_PATH(GET CMAKE_BINARY_DIR PARENT_PATH PKG_WINDOWS_BINARY_DIR)
	SET(arm64ReproDir "${PKG_WINDOWS_BINARY_DIR}/build.arm64/repros")
ENDIF()

# This function globs the linker input files that was copied into a repro_directory for each target during arm64 build. Then it passes the arm64 libs, objs and def file to the linker using /machine:arm64x to combine them with the arm64ec counterparts and create an arm64x binary.

function(set_arm64_dependencies n)
	set(ARM64_LIBS)
	set(ARM64_OBJS)
	set(ARM64_DEF)
	set(REPRO_PATH "${arm64ReproDir}/${n}")
	if(NOT EXISTS "${REPRO_PATH}")
		set(REPRO_PATH "${arm64ReproDir}/${n}_temp")
	endif()
	file(GLOB ARM64_OBJS "${REPRO_PATH}/*.obj")
	file(GLOB ARM64_DEF "${REPRO_PATH}/*.def")
	file(GLOB ARM64_LIBS "${REPRO_PATH}/*.LIB")

	if(NOT "${ARM64_DEF}" STREQUAL "")
		set(ARM64_DEF "/defArm64Native:${ARM64_DEF}")
	endif()
	target_sources(${n} PRIVATE ${ARM64_OBJS})
	target_link_options(${n} PRIVATE /machine:arm64x "${ARM64_DEF}" "${ARM64_LIBS}")
endfunction()

# During the arm64 build, pass the /link_repro flag to linker so it knows to copy into a directory, all the file inputs needed by the linker for arm64 build (objs, def files, libs).
# extra logic added to deal with rebuilds and avoiding overwriting directories.
if(BUILD_AS_ARM64X STREQUAL "ARM64")
	foreach (n ${ARM64X_TARGETS})
		add_custom_target(mkdirs_${n} ALL COMMAND cmd /c (if exist \"${arm64ReproDir}/${n}_temp/\" rmdir /s /q \"${arm64ReproDir}/${n}_temp\") && mkdir \"${arm64ReproDir}/${n}_temp\" )
		add_dependencies(${n} mkdirs_${n})
		target_link_options(${n} PRIVATE "/LINKREPRO:${arm64ReproDir}/${n}_temp")
		add_custom_target(${n}_checkRepro ALL COMMAND cmd /c if exist \"${n}_temp/*.obj\" if exist \"${n}\" rmdir /s /q \"${n}\" 2>nul && if not exist \"${n}\" ren \"${n}_temp\" \"${n}\" WORKING_DIRECTORY ${arm64ReproDir})
		add_dependencies(${n}_checkRepro ${n})
	endforeach()

# During the ARM64EC build, modify the link step appropriately to produce an arm64x binary
elseif(BUILD_AS_ARM64X STREQUAL "ARM64EC")
	foreach (n ${ARM64X_TARGETS})
		set_arm64_dependencies(${n})
	endforeach()
endif()
