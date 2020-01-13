# Windows common code.
PROJECT(libwin32common)

### NOTE: DelayLoadHelper should NOT be compiled here.
# If it is, it won't be linked into the resulting executable,
# since MSVC will trim the object file. Compile it in the
# executable target directly.

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libwin32common.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libwin32common.h")

# Sources and headers.
SET(libwin32common_SRCS
	wmain.c
	DialogBuilder.cpp
	RegKey.cpp
	WinUI.cpp
	w32err.c
	ComBase.cpp
	propsys_xp.c
	sdk/win_iid.c
	sdk/GUID_fn.c
	)
SET(libwin32common_H
	RpWin32_sdk.h
	secoptions.h
	DialogBuilder.hpp
	RegKey.hpp
	AutoGetDC.hpp
	WinUI.hpp
	w32err.h
	w32time.h
	ComBase.hpp
	propsys_xp.h
	WTSSessionNotification.hpp
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

ADD_LIBRARY(win32common STATIC
	${libwin32common_SRCS}
	${libwin32common_H}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(win32common)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(win32common PROPERTIES EXCLUDE_FROM_ALL TRUE)
TARGET_LINK_LIBRARIES(win32common PRIVATE rpthreads)

TARGET_INCLUDE_DIRECTORIES(win32common
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libwin32common
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libwin32common
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
	)
