# GTK UI frontend test suite
PROJECT(gtk-tests LANGUAGES C CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# GTK source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)

### RomDataFormat tests
SET(RomDataFormatTest_SRCS RomDataFormatTest.cpp)
ADD_EXECUTABLE(RomDataFormatTest_glib ${RomDataFormatTest_SRCS})
TARGET_LINK_LIBRARIES(RomDataFormatTest_glib PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(RomDataFormatTest_glib PUBLIC rom-properties-glib)
LIST(APPEND GTK_TESTS RomDataFormatTest_glib)

### sort_funcs.c tests

SET(SortFuncsTest_GTK_COMMON_SRCS ../sort_funcs_common.c)
SET(SortFuncsTest_GTK_COMMON_H    ../sort_funcs_common.h)

SET(SortFuncsTest_GTK3_SRCS
	SortFuncsTest_gtk3.cpp
	../gtk3/sort_funcs.c
	)
SET(SortFuncsTest_GTK3_H
	../gtk3/sort_funcs.h
	)

SET(SortFuncsTest_GTK4_SRCS
	SortFuncsTest_gtk4.cpp
	../rp-gtk-enums.c
	../ListDataItem.c
	../gtk4/sort_funcs.c
	)
SET(SortFuncsTest_GTK4_H
	../rp-gtk-enums.h
	../ListDataItem.h
	../gtk4/sort_funcs.h
	)

IF(BUILD_XFCE)
	ADD_EXECUTABLE(SortFuncsTest_gtk2
		${SortFuncsTest_GTK_COMMON_SRCS}
		${SortFuncsTest_GTK_COMMON_H}
		${SortFuncsTest_GTK3_SRCS}
		${SortFuncsTest_GTK3_H}
		)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk2 PRIVATE rptest)
	#TARGET_LINK_LIBRARIES(SortFuncsTest_gtk2 PRIVATE GdkPixbuf2::gdkpixbuf2)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk2 PUBLIC GTK2::gtk GTK2::gdk GTK2::pango)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk2 PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	LIST(APPEND GTK_TESTS SortFuncsTest_gtk2)

	ADD_EXECUTABLE(RomDataViewTest_gtk2 RomDataViewTest.cpp)
	TARGET_LINK_LIBRARIES(RomDataViewTest_gtk2 PRIVATE rptest romdata)
	TARGET_LINK_LIBRARIES(RomDataViewTest_gtk2 PUBLIC GTK2::gtk GTK2::gdk GTK2::pango)
	TARGET_LINK_LIBRARIES(RomDataViewTest_gtk2 PUBLIC rom-properties-xfce_LIB)
	LIST(APPEND GTK_TESTS RomDataViewTest_gtk2)
ENDIF(BUILD_XFCE)
IF(BUILD_GTK3)
	ADD_EXECUTABLE(SortFuncsTest_gtk3
		${SortFuncsTest_GTK_COMMON_SRCS}
		${SortFuncsTest_GTK_COMMON_H}
		${SortFuncsTest_GTK3_SRCS}
		${SortFuncsTest_GTK3_H}
		)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk3 PRIVATE rptest)
	#TARGET_LINK_LIBRARIES(SortFuncsTest_gtk3 PRIVATE Cairo::gobject)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk3 PUBLIC GTK3::gtk GTK3::gdk GTK3::pango GTK3::cairo)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk3 PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	LIST(APPEND GTK_TESTS SortFuncsTest_gtk3)

	ADD_EXECUTABLE(RomDataViewTest_gtk3 RomDataViewTest.cpp)
	TARGET_LINK_LIBRARIES(RomDataViewTest_gtk3 PRIVATE rptest romdata)
	TARGET_LINK_LIBRARIES(RomDataViewTest_gtk3 PUBLIC rom-properties-gtk3_LIB)
	LIST(APPEND GTK_TESTS RomDataViewTest_gtk3)
ENDIF(BUILD_GTK3)
IF(BUILD_GTK4)
	ADD_EXECUTABLE(SortFuncsTest_gtk4
		${SortFuncsTest_GTK_COMMON_SRCS}
		${SortFuncsTest_GTK_COMMON_H}
		${SortFuncsTest_GTK4_SRCS}
		${SortFuncsTest_GTK4_H}
		)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk4 PRIVATE rptest)
	#TARGET_LINK_LIBRARIES(SortFuncsTest_gtk4 PRIVATE Cairo::gobject)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk4 PUBLIC GTK4::gtk GTK4::pango)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk4 PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	LIST(APPEND GTK_TESTS SortFuncsTest_gtk4)

	ADD_EXECUTABLE(RomDataViewTest_gtk4 RomDataViewTest.cpp)
	TARGET_LINK_LIBRARIES(RomDataViewTest_gtk4 PRIVATE rptest romdata)
	TARGET_LINK_LIBRARIES(RomDataViewTest_gtk4 PUBLIC rom-properties-gtk4_LIB)
	LIST(APPEND GTK_TESTS RomDataViewTest_gtk4)
ENDIF(BUILD_GTK4)

FOREACH(_target ${GTK_TESTS})
	DO_SPLIT_DEBUG(${_target})
	SET_WINDOWS_SUBSYSTEM(${_target} CONSOLE)
	SET_WINDOWS_ENTRYPOINT(${_target} wmain OFF)
	ADD_TEST(NAME ${_target} COMMAND ${_target} --gtest_brief)
ENDFOREACH(_target)
