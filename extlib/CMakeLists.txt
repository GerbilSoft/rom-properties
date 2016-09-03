PROJECT(extlib)

# Google Test
IF(BUILD_TESTING)
	# Reference: http://stackoverflow.com/questions/12540970/how-to-make-gtest-build-mdd-instead-of-mtd-by-default-using-cmake
	SET(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
	ADD_SUBDIRECTORY(googletest)

	INCLUDE(SetMSVCDebugPath)
	FOREACH(_target
		gtest gtest_main gtest_no_exception gtest_main_no_exception
		gtest_main_no_rtti gtest_dll gtest_main_use_own_tuple
		gmock gmock_main gmock_main_no_exception
		gmock_main_no_rtti gmock_main_use_own_tuple)
		IF(TARGET ${_target})
			# Set the MSVC debug path.
			SET_MSVC_DEBUG_PATH(${_target})

			# Exclude from ALL and DEFAULT builds.
			SET_TARGET_PROPERTIES(${_target} PROPERTIES
				EXCLUDE_FROM_ALL TRUE
				EXCLUDE_FROM_DEFAULT_BUILD TRUE
				)
		ENDIF(TARGET ${_target})
	ENDFOREACH()
ENDIF(BUILD_TESTING)
