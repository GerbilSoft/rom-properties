# Windows common code.
PROJECT(libwin32common)

### NOTE: DelayLoadHelper should NOT be compiled here.
# If it is, it won't be linked into the resulting executable,
# since MSVC will trim the object file. Compile it in the
# executable target directly.

# Sources and headers.
SET(libwin32common_SRCS
	wmain.c
	DialogBuilder.cpp
	RegKey.cpp
	WinUI.cpp
	w32err.c
	ComBase.cpp
	sdk/win_iid.cpp
	)
SET(libwin32common_H
	RpWin32_sdk.h
	secoptions.h
	DialogBuilder.hpp
	RegKey.hpp
	AutoGetDC.hpp
	WinUI.hpp
	w32err.h
	msvc_common.h
	w32time.h
	ComBase.hpp
	sdk/QITab.h
	sdk/IListView.hpp
	sdk/IOwnerDataCallback.hpp
	sdk/tsbase.h
	sdk/windowsx_ts.h
	sdk/commctrl_ts.h
	)

IF(MSVC)
	SET(libwin32common_SRCS ${libwin32common_SRCS} msvc_common.c)
ENDIF(MSVC)

######################
# Build the library. #
######################

ADD_LIBRARY(win32common STATIC
	${libwin32common_SRCS}
	${libwin32common_H}
	)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(win32common PROPERTIES EXCLUDE_FROM_ALL TRUE)
