# Windows common code
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(win32common LANGUAGES C CXX)

### NOTE: DelayLoadHelper should NOT be compiled here.
# If it is, it won't be linked into the resulting executable,
# since MSVC will trim the object file. Compile it in the
# executable target directly.

# Check for unordered_set::reserve.
SET(OLD_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
CHECK_CXX_SOURCE_COMPILES("#include <unordered_set>
int main(void) {
	std::unordered_set<int> testSet;
	testSet.reserve(4);
	return 0;
}" HAVE_UNORDERED_SET_RESERVE)
SET(CMAKE_REQUIRED_INCLUDES ${OLD_CMAKE_REQUIRED_INCLUDES})
UNSET(OLD_CMAKE_REQUIRED_INCLUDES)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

# Sources and headers.
SET(${PROJECT_NAME}_SRCS
	w32err.cpp
	ComBase.cpp
	propsys_xp.c
	userdirs.cpp
	sdk/GUID_fn.c
	)
SET(${PROJECT_NAME}_H
	RpWin32_sdk.h
	w32err.hpp
	w32time.h
	ComBase.hpp
	propsys_xp.h
	userdirs.hpp
	sdk/QITab.h
	sdk/IListView.hpp
	sdk/IOwnerDataCallback.hpp
	sdk/tsbase.h
	sdk/windowsx_ts.h
	sdk/commctrl_ts.h
	sdk/GUID_fn.h
	)

######################
# Build the library. #
######################

# NOTE: ${PROJECT_NAME} is the original target.
# ${PROJECT_NAME}-dll is the target with dllimport/dllexport for libromdata.
IF(RP_LIBROMDATA_IS_DLL AND WIN32)
	# On Win32, the dll target needs dllimport/dllexport.
	SET(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-dll)
ELSE()
	# On other platforms, the dll target will be an alias to the regular target.
	# That will be set up later, since the regular target doesn't exist yet.
	SET(TARGETS ${PROJECT_NAME})
ENDIF()

FOREACH(_target ${TARGETS})
	ADD_LIBRARY(${_target} STATIC
		${${PROJECT_NAME}_SRCS}
		${${PROJECT_NAME}_H}
		)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)

	# DLL targets need the appropriate macros.
	IF(${_target} MATCHES "-dll$")
		SET(_target_suffix "-dll")
		TARGET_COMPILE_DEFINITIONS(${_target}
			PUBLIC		RP_BUILDING_FOR_DLL=1
			PRIVATE		RP_BUILDING_LIBROMDATA=1
			)
	ELSE()
		UNSET(_target_suffix)
	ENDIF()

	TARGET_LINK_LIBRARIES(${_target} PRIVATE rpthreads${_target_suffix})
	TARGET_LINK_LIBRARIES(${_target} PRIVATE rptext${_target_suffix})
	TARGET_LINK_LIBRARIES(${_target} PRIVATE uuid ole32 oleaut32 advapi32 shell32)
	# for DefSubclassProc and related
	TARGET_LINK_LIBRARIES(${_target} PUBLIC comctl32)

	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libwin32common
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libwin32common
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		)
ENDFOREACH(_target)
UNSET(_target_suffix)

IF(NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()

###################
# wmain.c library #
###################
ADD_LIBRARY(wmain STATIC wmain.c)
SET_MSVC_DEBUG_PATH(wmain)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(wmain PROPERTIES EXCLUDE_FROM_ALL TRUE)
TARGET_INCLUDE_DIRECTORIES(wmain
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
	)
