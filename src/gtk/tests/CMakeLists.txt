# GTK UI frontend test suite
PROJECT(gtk-tests LANGUAGES C CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# GTK source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)

### sort_funcs.c testss

SET(SortFuncsTest_SRCS SortFuncsTest.cpp)

SET(SortFuncsTest_GTK3_SRCS ../gtk3/sort_funcs.c)
SET(SortFuncsTest_GTK3_H    ../gtk3/sort_funcs.h)

SET(SortFuncsTest_GTK4_SRCS ../gtk4/sort_funcs.c)
SET(SortFuncsTest_GTK4_H    ../gtk4/sort_funcs.h)

IF(BUILD_XFCE)
	ADD_EXECUTABLE(SortFuncsTest_gtk2
		${SortFuncsTest_SRCS}
		${SortFuncsTest_GTK3_SRCS}
		${SortFuncsTest_GTK3_H}
		)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk2 PRIVATE rptest)
	#TARGET_LINK_LIBRARIES(SortFuncsTest_gtk2 PRIVATE GdkPixbuf2::gdkpixbuf2)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk2 PUBLIC GTK2::gtk GTK2::gdk GTK2::pango)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk2 PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	LIST(APPEND GTK_TESTS SortFuncsTest_gtk2)
ENDIF(BUILD_XFCE)
IF(BUILD_GTK3)
	ADD_EXECUTABLE(SortFuncsTest_gtk3
		${SortFuncsTest_SRCS}
		${SortFuncsTest_GTK3_SRCS}
		${SortFuncsTest_GTK3_H}
		)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk3 PRIVATE rptest)
	#TARGET_LINK_LIBRARIES(SortFuncsTest_gtk3 PRIVATE Cairo::gobject)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk3 PUBLIC GTK3::gtk GTK3::gdk GTK3::pango GTK3::cairo)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk3 PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	LIST(APPEND GTK_TESTS SortFuncsTest_gtk3)
ENDIF(BUILD_GTK3)
IF(BUILD_GTK4)
	ADD_EXECUTABLE(SortFuncsTest_gtk4
		${SortFuncsTest_SRCS}
		# TODO: Test the GTK4 sorting functions.
		${SortFuncsTest_GTK3_SRCS}
		${SortFuncsTest_GTK3_H}
		)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk4 PRIVATE rptest)
	#TARGET_LINK_LIBRARIES(SortFuncsTest_gtk4 PRIVATE Cairo::gobject)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk4 PUBLIC GTK4::gtk GTK4::pango)
	TARGET_LINK_LIBRARIES(SortFuncsTest_gtk4 PUBLIC GLib2::gio-unix GLib2::gio GLib2::gobject GLib2::glib)
	LIST(APPEND GTK_TESTS SortFuncsTest_gtk4)
ENDIF(BUILD_GTK4)

FOREACH(_target ${GTK_TESTS})
	DO_SPLIT_DEBUG(${_target})
	SET_WINDOWS_SUBSYSTEM(${_target} CONSOLE)
	SET_WINDOWS_ENTRYPOINT(${_target} wmain OFF)
	ADD_TEST(NAME ${_target} COMMAND ${_target} --gtest_brief)
ENDFOREACH(_target)
