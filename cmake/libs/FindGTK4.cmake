### MODIFIED version of cmake-3.23.3's FindGTK4.cmake.
### Updated for GTK4.

# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindGTK4
--------

Find the GTK4 widget libraries and several of its other optional components
like ``gtkmm``.

Specify one or more of the following components as you call this find
module.  See example below.

* ``gtk``
* ``gtkmm``

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets (subject to
component selection):

``GTK4::atk``, ``GTK4::atkmm``, ``GTK4::cairo``, ``GTK4::cairomm``,
``GTK4::gdk_pixbuf``, ``GTK4::gio``, ``GTK4::giomm``, ``GTK4::glib``,
``GTK4::glibmm``, ``GTK4::gmodule``, ``GTK4::gobject``, ``GTK4::graphene``,
``GTK4::gthread``, ``GTK4::gtk``, ``GTK4::gtkmm``, ``GTK4::harfbuzz``,
``GTK4::pango``, ``GTK4::pangocairo``, ``GTK4::pangoft2``, ``GTK4::pangomm``,
``GTK4::pangoxft``, ``GTK4::sigc``.

.. versionadded:: 3.16.7
  Added the ``GTK4::harfbuzz`` target.

Result Variables
^^^^^^^^^^^^^^^^

The following variables will be defined for your use

``GTK4_FOUND``
  Were all of your specified components found?
``GTK4_INCLUDE_DIRS``
  All include directories
``GTK4_LIBRARIES``
  All libraries
``GTK4_TARGETS``
  .. versionadded:: 3.5
    All imported targets
``GTK4_DEFINITIONS``
  Additional compiler flags
``GTK4_VERSION``
  The version of GTK4 found (x.y.z)
``GTK4_MAJOR_VERSION``
  The major version of GTK4
``GTK4_MINOR_VERSION``
  The minor version of GTK4
``GTK4_PATCH_VERSION``
  The patch version of GTK4

.. versionadded:: 3.5
  When ``GTK4_USE_IMPORTED_TARGETS`` is set to ``TRUE``, ``GTK4_LIBRARIES``
  will list imported targets instead of library paths.

Input Variables
^^^^^^^^^^^^^^^

Optional variables you can define prior to calling this module:

``GTK4_DEBUG``
  Enables verbose debugging of the module
``GTK4_ADDITIONAL_SUFFIXES``
  Allows defining additional directories to search for include files

Example Usage
^^^^^^^^^^^^^

Call :command:`find_package` once.  Here are some examples to pick from:

Require GTK 3.6 or later:

.. code-block:: cmake

  find_package(GTK4 3.6 REQUIRED gtk)

Search for GTK/GTKMM 3.8 or later:

.. code-block:: cmake

  find_package(GTK4 3.8 COMPONENTS gtk gtkmm)

Use the results:

.. code-block:: cmake

  if(GTK4_FOUND)
    include_directories(${GTK4_INCLUDE_DIRS})
    add_executable(mygui mygui.cc)
    target_link_libraries(mygui ${GTK4_LIBRARIES})
  endif()
#]=======================================================================]

# Version 1.6 (CMake 3.0)
#   * Create targets for each library
#   * Do not link libfreetype
# Version 1.5 (CMake 2.8.12)
#   * 14236: Detect gthread library
#            Detect pangocairo on windows
#            Detect pangocairo with gtk module instead of with gtkmm
#   * 14259: Use vc100 libraries with VS 11
#   * 14260: Export a GTK2_DEFINITIONS variable to set /vd2 when appropriate
#            (i.e. MSVC)
#   * Use the optimized/debug syntax for _LIBRARY and _LIBRARIES variables when
#     appropriate. A new set of _RELEASE variables was also added.
#   * Remove GTK2_SKIP_MARK_AS_ADVANCED option, as now the variables are
#     marked as advanced by SelectLibraryConfigurations
#   * Detect gmodule, pangoft2 and pangoxft libraries
# Version 1.4 (10/4/2012) (CMake 2.8.10)
#   * 12596: Missing paths for FindGTK2 on NetBSD
#   * 12049: Fixed detection of GTK include files in the lib folder on
#            multiarch systems.
# Version 1.3 (11/9/2010) (CMake 2.8.4)
#   * 11429: Add support for detecting GTK2 built with Visual Studio 10.
#            Thanks to Vincent Levesque for the patch.
# Version 1.2 (8/30/2010) (CMake 2.8.3)
#   * Merge patch for detecting gdk-pixbuf library (split off
#     from core GTK in 2.21).  Thanks to Vincent Untz for the patch
#     and Ricardo Cruz for the heads up.
# Version 1.1 (8/19/2010) (CMake 2.8.3)
#   * Add support for detecting GTK2 under macports (thanks to Gary Kramlich)
# Version 1.0 (8/12/2010) (CMake 2.8.3)
#   * Add support for detecting new pangommconfig.h header file
#     (Thanks to Sune Vuorela & the Debian Project for the patch)
#   * Add support for detecting fontconfig.h header
#   * Call find_package(Freetype) since it's required
#   * Add support for allowing users to add additional library directories
#     via the GTK2_ADDITIONAL_SUFFIXES variable (kind of a future-kludge in
#     case the GTK developers change versions on any of the directories in the
#     future).
# Version 0.8 (1/4/2010)
#   * Get module working under MacOSX fink by adding /sw/include, /sw/lib
#     to PATHS and the gobject library
# Version 0.7 (3/22/09)
#   * Checked into CMake CVS
#   * Added versioning support
#   * Module now defaults to searching for GTK if COMPONENTS not specified.
#   * Added HKCU prior to HKLM registry key and GTKMM specific environment
#      variable as per mailing list discussion.
#   * Added lib64 to include search path and a few other search paths where GTK
#      may be installed on Unix systems.
#   * Switched to lowercase CMake commands
#   * Prefaced internal variables with _GTK2 to prevent collision
#   * Changed internal macros to functions
#   * Enhanced documentation
# Version 0.6 (1/8/08)
#   Added GTK2_SKIP_MARK_AS_ADVANCED option
# Version 0.5 (12/19/08)
#   Second release to cmake mailing list

#=============================================================
# _GTK4_GET_VERSION
# Internal function to parse the version number in gtkversion.h
#   _OUT_major = Major version number
#   _OUT_minor = Minor version number
#   _OUT_micro = Micro version number
#   _gtkversion_hdr = Header file to parse
#=============================================================

include(SelectLibraryConfigurations)

function(_GTK4_GET_VERSION _OUT_major _OUT_minor _OUT_micro _gtkversion_hdr)
    file(STRINGS ${_gtkversion_hdr} _contents REGEX "#define GTK_M[A-Z]+_VERSION[ \t]+")
    if(_contents)
        string(REGEX REPLACE ".*#define GTK_MAJOR_VERSION[ \t]+\\(([0-9]+)\\).*" "\\1" ${_OUT_major} "${_contents}")
        string(REGEX REPLACE ".*#define GTK_MINOR_VERSION[ \t]+\\(([0-9]+)\\).*" "\\1" ${_OUT_minor} "${_contents}")
        string(REGEX REPLACE ".*#define GTK_MICRO_VERSION[ \t]+\\(([0-9]+)\\).*" "\\1" ${_OUT_micro} "${_contents}")

        if(NOT ${_OUT_major} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for GTK4_MAJOR_VERSION!")
        endif()
        if(NOT ${_OUT_minor} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for GTK4_MINOR_VERSION!")
        endif()
        if(NOT ${_OUT_micro} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for GTK4_MICRO_VERSION!")
        endif()

        set(${_OUT_major} ${${_OUT_major}} PARENT_SCOPE)
        set(${_OUT_minor} ${${_OUT_minor}} PARENT_SCOPE)
        set(${_OUT_micro} ${${_OUT_micro}} PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Include file ${_gtkversion_hdr} does not exist")
    endif()
endfunction()


#=============================================================
# _GTK4_SIGCXX_GET_VERSION
# Internal function to parse the version number in
# sigc++config.h
#   _OUT_major = Major version number
#   _OUT_minor = Minor version number
#   _OUT_micro = Micro version number
#   _sigcxxversion_hdr = Header file to parse
#=============================================================

function(_GTK4_SIGCXX_GET_VERSION _OUT_major _OUT_minor _OUT_micro _sigcxxversion_hdr)
    file(STRINGS ${_sigcxxversion_hdr} _contents REGEX "#define SIGCXX_M[A-Z]+_VERSION[ \t]+")
    if(_contents)
        string(REGEX REPLACE ".*#define SIGCXX_MAJOR_VERSION[ \t]+([0-9]+).*" "\\1" ${_OUT_major} "${_contents}")
        string(REGEX REPLACE ".*#define SIGCXX_MINOR_VERSION[ \t]+([0-9]+).*" "\\1" ${_OUT_minor} "${_contents}")
        string(REGEX REPLACE ".*#define SIGCXX_MICRO_VERSION[ \t]+([0-9]+).*" "\\1" ${_OUT_micro} "${_contents}")

        if(NOT ${_OUT_major} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for SIGCXX_MAJOR_VERSION!")
        endif()
        if(NOT ${_OUT_minor} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for SIGCXX_MINOR_VERSION!")
        endif()
        if(NOT ${_OUT_micro} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for SIGCXX_MICRO_VERSION!")
        endif()

        set(${_OUT_major} ${${_OUT_major}} PARENT_SCOPE)
        set(${_OUT_minor} ${${_OUT_minor}} PARENT_SCOPE)
        set(${_OUT_micro} ${${_OUT_micro}} PARENT_SCOPE)
    else()
        # The header does not have the version macros; assume it is ``0.0.0``.
        set(${_OUT_major} 0)
        set(${_OUT_minor} 0)
        set(${_OUT_micro} 0)
    endif()
endfunction()


#=============================================================
# _GTK4_FIND_INCLUDE_DIR
# Internal function to find the GTK include directories
#   _var = variable to set (_INCLUDE_DIR is appended)
#   _hdr = header file to look for
#=============================================================
function(_GTK4_FIND_INCLUDE_DIR _var _hdr)

    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "_GTK4_FIND_INCLUDE_DIR( ${_var} ${_hdr} )")
    endif()

    set(_gtk_packages
        # If these ever change, things will break.
        ${GTK4_ADDITIONAL_SUFFIXES}
        glibmm-2.4
        glib-2.0
        atk-1.0
        atkmm-1.6
        cairo
        cairomm-1.0
        gdk-pixbuf-2.0
        giomm-2.4
        graphene-1.0
        gtk-4.0
        gtkmm-4.0
        harfbuzz
        pango-1.0
        pangomm-2.48
        sigc++-3.0
    )

    #
    # NOTE: The following suffixes cause searching for header files in both of
    # these directories:
    #         /usr/include/<pkg>
    #         /usr/lib/<pkg>/include
    #

    set(_suffixes)
    foreach(_d ${_gtk_packages})
        list(APPEND _suffixes ${_d})
        list(APPEND _suffixes ${_d}/include) # for /usr/lib/gtk-2.0/include
    endforeach()

    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "include suffixes = ${_suffixes}")
    endif()

    if(CMAKE_LIBRARY_ARCHITECTURE)
      set(_gtk4_arch_dir /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE})
      if(GTK4_DEBUG)
        message(STATUS "Adding ${_gtk4_arch_dir} to search path for multiarch support")
      endif()
    endif()
    find_path(GTK4_${_var}_INCLUDE_DIR ${_hdr}
        PATHS
            ${_gtk4_arch_dir}
            /usr/local/libx32
            /usr/local/lib64
            /usr/local/lib
            /usr/libx32
            /usr/lib64
            /usr/lib
            /opt/gnome/include
            /opt/gnome/lib
            /opt/openwin/include
            /usr/openwin/lib
            /sw/lib
            /opt/local/lib
            /usr/pkg/lib
            /usr/pkg/include/glib
            $ENV{GTKMM_BASEPATH}/include
            $ENV{GTKMM_BASEPATH}/lib
            [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\4.0;Path]/include
            [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\4.0;Path]/lib
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\4.0;Path]/include
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\4.0;Path]/lib
        PATH_SUFFIXES
            ${_suffixes}
    )
    mark_as_advanced(GTK4_${_var}_INCLUDE_DIR)

    if(GTK4_${_var}_INCLUDE_DIR)
        set(GTK4_INCLUDE_DIRS ${GTK4_INCLUDE_DIRS} ${GTK4_${_var}_INCLUDE_DIR} PARENT_SCOPE)
    endif()

endfunction()

#=============================================================
# _GTK4_FIND_LIBRARY
# Internal function to find libraries packaged with GTK4
#   _var = library variable to create (_LIBRARY is appended)
#=============================================================
function(_GTK4_FIND_LIBRARY _var _lib _expand_vc)

    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "_GTK4_FIND_LIBRARY( ${_var} ${_lib} ${_expand_vc} )")
    endif()

    set(_library)
    set(_library_d)

    set(_library ${_lib})

    if(_expand_vc AND MSVC)
        # Add vc80/vc90/vc100 midfixes
        if(MSVC_TOOLSET_VERSION LESS 110)
            set(_library   ${_library}-vc${MSVC_TOOLSET_VERSION})
        else()
            # Up to gtkmm-win 2.22.0-2 there are no vc110 libraries but vc100 can be used
            set(_library ${_library}-vc100)
        endif()
        set(_library_d ${_library}-d)
    endif()

    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "After midfix addition = ${_library} and ${_library_d}")
    endif()

    set(_lib_list ${_library})
    set(_libd_list ${_library_d})

    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "library list = ${_lib_list} and library debug list = ${_libd_list}")
    endif()

    # For some silly reason the MSVC libraries use _ instead of .
    # in the version fields
    if(_expand_vc AND MSVC)
        set(_no_dots_lib_list)
        set(_no_dots_libd_list)
        foreach(_l ${_lib_list})
            string(REPLACE "." "_" _no_dots_library ${_l})
            list(APPEND _no_dots_lib_list ${_no_dots_library})
        endforeach()
        # And for debug
        set(_no_dots_libsd_list)
        foreach(_l ${_libd_list})
            string(REPLACE "." "_" _no_dots_libraryd ${_l})
            list(APPEND _no_dots_libd_list ${_no_dots_libraryd})
        endforeach()

        # Copy list back to original names
        set(_lib_list ${_no_dots_lib_list})
        set(_libd_list ${_no_dots_libd_list})
    endif()

    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "While searching for GTK4_${_var}_LIBRARY, our proposed library list is ${_lib_list}")
    endif()

    find_library(GTK4_${_var}_LIBRARY_RELEASE
        NAMES ${_lib_list}
        PATHS
            /opt/gnome/lib
            /usr/openwin/lib
            $ENV{GTKMM_BASEPATH}/lib
            [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\4.0;Path]/lib
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\4.0;Path]/lib
        )

    if(_expand_vc AND MSVC)
        if(GTK4_DEBUG)
            message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                           "While searching for GTK4_${_var}_LIBRARY_DEBUG our proposed library list is ${_libd_list}")
        endif()

        find_library(GTK4_${_var}_LIBRARY_DEBUG
            NAMES ${_libd_list}
            PATHS
            $ENV{GTKMM_BASEPATH}/lib
            [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\4.0;Path]/lib
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\4.0;Path]/lib
        )
    endif()

    select_library_configurations(GTK4_${_var})

    set(GTK4_${_var}_LIBRARY ${GTK4_${_var}_LIBRARY} PARENT_SCOPE)
    set(GTK4_${_var}_FOUND ${GTK4_${_var}_FOUND} PARENT_SCOPE)

    if(GTK4_${_var}_FOUND)
        set(GTK4_LIBRARIES ${GTK4_LIBRARIES} ${GTK4_${_var}_LIBRARY})
        set(GTK4_LIBRARIES ${GTK4_LIBRARIES} PARENT_SCOPE)
    endif()

    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "GTK4_${_var}_LIBRARY_RELEASE = \"${GTK4_${_var}_LIBRARY_RELEASE}\"")
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "GTK4_${_var}_LIBRARY_DEBUG   = \"${GTK4_${_var}_LIBRARY_DEBUG}\"")
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "GTK4_${_var}_LIBRARY         = \"${GTK4_${_var}_LIBRARY}\"")
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "GTK4_${_var}_FOUND           = \"${GTK4_${_var}_FOUND}\"")
    endif()

endfunction()


function(_GTK4_ADD_TARGET_DEPENDS_INTERNAL _var _property)
    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "_GTK4_ADD_TARGET_DEPENDS_INTERNAL( ${_var} ${_property} )")
    endif()

    string(TOLOWER "${_var}" _basename)

    if (TARGET GTK4::${_basename})
        foreach(_depend ${ARGN})
            set(_valid_depends)
            if (TARGET GTK4::${_depend})
                list(APPEND _valid_depends GTK4::${_depend})
            endif()
            if (_valid_depends)
                set_property(TARGET GTK4::${_basename} APPEND PROPERTY ${_property} "${_valid_depends}")
            endif()
            set(_valid_depends)
        endforeach()
    endif()
endfunction()

function(_GTK4_ADD_TARGET_DEPENDS _var)
    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "_GTK4_ADD_TARGET_DEPENDS( ${_var} )")
    endif()

    string(TOLOWER "${_var}" _basename)

    if(TARGET GTK4::${_basename})
        get_target_property(_configs GTK4::${_basename} IMPORTED_CONFIGURATIONS)
        _GTK4_ADD_TARGET_DEPENDS_INTERNAL(${_var} INTERFACE_LINK_LIBRARIES ${ARGN})
        foreach(_config ${_configs})
            _GTK4_ADD_TARGET_DEPENDS_INTERNAL(${_var} IMPORTED_LINK_INTERFACE_LIBRARIES_${_config} ${ARGN})
        endforeach()
    endif()
endfunction()

function(_GTK4_ADD_TARGET_INCLUDE_DIRS _var)
    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "_GTK4_ADD_TARGET_INCLUDE_DIRS( ${_var} )")
    endif()

    string(TOLOWER "${_var}" _basename)

    if(TARGET GTK4::${_basename})
        foreach(_include ${ARGN})
            set_property(TARGET GTK4::${_basename} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${_include}")
        endforeach()
    endif()
endfunction()

#=============================================================
# _GTK4_ADD_TARGET
# Internal function to create targets for GTK4
#   _var = target to create
#=============================================================
function(_GTK4_ADD_TARGET _var)
    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "_GTK4_ADD_TARGET( ${_var} )")
    endif()

    string(TOLOWER "${_var}" _basename)

    cmake_parse_arguments(_${_var} "" "" "GTK4_DEPENDS;GTK4_OPTIONAL_DEPENDS;OPTIONAL_INCLUDES" ${ARGN})

    if(GTK4_${_var}_FOUND)
        if(NOT TARGET GTK4::${_basename})
            # Do not create the target if dependencies are missing
            foreach(_dep ${_${_var}_GTK4_DEPENDS})
                if(NOT TARGET GTK4::${_dep})
                    return()
                endif()
            endforeach()

            add_library(GTK4::${_basename} UNKNOWN IMPORTED)

            if(GTK4_${_var}_LIBRARY_RELEASE)
                set_property(TARGET GTK4::${_basename} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
                set_property(TARGET GTK4::${_basename}        PROPERTY IMPORTED_LOCATION_RELEASE "${GTK4_${_var}_LIBRARY_RELEASE}" )
            endif()

            if(GTK4_${_var}_LIBRARY_DEBUG)
                set_property(TARGET GTK4::${_basename} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
                set_property(TARGET GTK4::${_basename}        PROPERTY IMPORTED_LOCATION_DEBUG "${GTK4_${_var}_LIBRARY_DEBUG}" )
            endif()

            if(GTK4_${_var}_INCLUDE_DIR)
                set_property(TARGET GTK4::${_basename} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GTK4_${_var}_INCLUDE_DIR}")
            endif()

            if(GTK4_${_var}CONFIG_INCLUDE_DIR AND NOT "x${GTK4_${_var}CONFIG_INCLUDE_DIR}" STREQUAL "x${GTK4_${_var}_INCLUDE_DIR}")
                set_property(TARGET GTK4::${_basename} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GTK4_${_var}CONFIG_INCLUDE_DIR}")
            endif()

            # FIXME: GTK4_DEFINITIONS might have -pthread, which can sometimes cause
            # -D-pthread to be added, resulting in build failures.
            STRING(REPLACE "-pthread" "" GTK4_DEFINITIONS "${GTK4_DEFINITIONS}")
            if(GTK4_DEFINITIONS)
                set_property(TARGET GTK4::${_basename} PROPERTY INTERFACE_COMPILE_DEFINITIONS "${GTK4_DEFINITIONS}")
            endif()

            if(_${_var}_GTK4_DEPENDS)
                _GTK4_ADD_TARGET_DEPENDS(${_var} ${_${_var}_GTK4_DEPENDS} ${_${_var}_GTK4_OPTIONAL_DEPENDS})
            endif()

            if(_${_var}_OPTIONAL_INCLUDES)
                foreach(_D ${_${_var}_OPTIONAL_INCLUDES})
                    if(_D)
                        _GTK4_ADD_TARGET_INCLUDE_DIRS(${_var} ${_D})
                    endif()
                endforeach()
            endif()
        endif()

        set(GTK4_TARGETS ${GTK4_TARGETS} GTK4::${_basename})
        set(GTK4_TARGETS ${GTK4_TARGETS} PARENT_SCOPE)

        if(GTK4_USE_IMPORTED_TARGETS)
            set(GTK4_${_var}_LIBRARY GTK4::${_basename} PARENT_SCOPE)
        endif()

    endif()
endfunction()



#=============================================================

#
# main()
#

set(GTK4_FOUND)
set(GTK4_INCLUDE_DIRS)
set(GTK4_LIBRARIES)
set(GTK4_TARGETS)
set(GTK4_DEFINITIONS)

if(NOT GTK4_FIND_COMPONENTS)
    # Assume they only want GTK
    set(GTK4_FIND_COMPONENTS gtk)
endif()

#
# If specified, enforce version number
#
if(GTK4_FIND_VERSION)
    set(GTK4_FAILED_VERSION_CHECK true)
    if(GTK4_DEBUG)
        message(STATUS "[FindGTK4.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "Searching for version ${GTK4_FIND_VERSION}")
    endif()
    _GTK4_FIND_INCLUDE_DIR(GTK gtk/gtk.h)
    if(GTK4_GTK_INCLUDE_DIR)
        _GTK4_GET_VERSION(GTK4_MAJOR_VERSION
                          GTK4_MINOR_VERSION
                          GTK4_PATCH_VERSION
                          ${GTK4_GTK_INCLUDE_DIR}/gtk/gtkversion.h)
        set(GTK4_VERSION
            ${GTK4_MAJOR_VERSION}.${GTK4_MINOR_VERSION}.${GTK4_PATCH_VERSION})
        if(GTK4_FIND_VERSION_EXACT)
            if(GTK4_VERSION VERSION_EQUAL GTK4_FIND_VERSION)
                set(GTK4_FAILED_VERSION_CHECK false)
            endif()
        else()
            if(GTK4_VERSION VERSION_EQUAL   GTK4_FIND_VERSION OR
               GTK4_VERSION VERSION_GREATER GTK4_FIND_VERSION)
                set(GTK4_FAILED_VERSION_CHECK false)
            endif()
        endif()
    else()
        # If we can't find the GTK include dir, we can't do version checking
        if(GTK4_FIND_REQUIRED AND NOT GTK4_FIND_QUIETLY)
            message(FATAL_ERROR "Could not find GTK4 include directory")
        endif()
        return()
    endif()

    if(GTK4_FAILED_VERSION_CHECK)
        if(GTK4_FIND_REQUIRED AND NOT GTK4_FIND_QUIETLY)
            if(GTK4_FIND_VERSION_EXACT)
                message(FATAL_ERROR "GTK4 version check failed.  Version ${GTK4_VERSION} was found, version ${GTK4_FIND_VERSION} is needed exactly.")
            else()
                message(FATAL_ERROR "GTK4 version check failed.  Version ${GTK4_VERSION} was found, at least version ${GTK4_FIND_VERSION} is required")
            endif()
        endif()

        # If the version check fails, exit out of the module here
        return()
    endif()
endif()

#
# On MSVC, according to https://wiki.gnome.org/gtkmm/MSWindows, the /vd2 flag needs to be
# passed to the compiler in order to use gtkmm
#
if(MSVC)
    foreach(_GTK4_component ${GTK4_FIND_COMPONENTS})
        if(_GTK4_component STREQUAL "gtkmm")
            set(GTK4_DEFINITIONS "/vd2")
        endif()
    endforeach()
endif()

#
# Find all components
#

find_package(Freetype QUIET)
if(FREETYPE_INCLUDE_DIR_ft2build AND FREETYPE_INCLUDE_DIR_freetype2)
    list(APPEND GTK4_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR_ft2build} ${FREETYPE_INCLUDE_DIR_freetype2})
endif()

foreach(_GTK4_component ${GTK4_FIND_COMPONENTS})
    if(_GTK4_component STREQUAL "gtk")
        # Left for compatibility with previous versions.
        _GTK4_FIND_INCLUDE_DIR(FONTCONFIG fontconfig/fontconfig.h)
        _GTK4_FIND_INCLUDE_DIR(X11 X11/Xlib.h)

        _GTK4_FIND_INCLUDE_DIR(GLIB glib.h)
        _GTK4_FIND_INCLUDE_DIR(GLIBCONFIG glibconfig.h)
        _GTK4_FIND_LIBRARY    (GLIB glib-2.0 false)
        _GTK4_ADD_TARGET      (GLIB)

        _GTK4_FIND_INCLUDE_DIR(GOBJECT glib-object.h)
        _GTK4_FIND_LIBRARY    (GOBJECT gobject-2.0 false)
        _GTK4_ADD_TARGET      (GOBJECT GTK4_DEPENDS glib)

        _GTK4_FIND_INCLUDE_DIR(ATK atk/atk.h)
        _GTK4_FIND_LIBRARY    (ATK atk-1.0 false)
        _GTK4_ADD_TARGET      (ATK GTK4_DEPENDS gobject glib)

        _GTK4_FIND_LIBRARY    (GIO gio-2.0 false)
        _GTK4_ADD_TARGET      (GIO GTK4_DEPENDS gobject glib)

        _GTK4_FIND_LIBRARY    (GTHREAD gthread-2.0 false)
        _GTK4_ADD_TARGET      (GTHREAD GTK4_DEPENDS glib)

        _GTK4_FIND_LIBRARY    (GMODULE gmodule-2.0 false)
        _GTK4_ADD_TARGET      (GMODULE GTK4_DEPENDS glib)

        _GTK4_FIND_INCLUDE_DIR(GDK_PIXBUF gdk-pixbuf/gdk-pixbuf.h)
        _GTK4_FIND_LIBRARY    (GDK_PIXBUF gdk_pixbuf-2.0 false)
        _GTK4_ADD_TARGET      (GDK_PIXBUF GTK4_DEPENDS gobject glib)

        _GTK4_FIND_INCLUDE_DIR(CAIRO cairo.h)
        _GTK4_FIND_LIBRARY    (CAIRO cairo false)
        _GTK4_ADD_TARGET      (CAIRO)

        _GTK4_FIND_INCLUDE_DIR(HARFBUZZ hb.h)
        _GTK4_FIND_LIBRARY    (HARFBUZZ harfbuzz false)
        _GTK4_ADD_TARGET      (HARFBUZZ)

        _GTK4_FIND_INCLUDE_DIR(PANGO pango/pango.h)
        _GTK4_FIND_LIBRARY    (PANGO pango-1.0 false)
        _GTK4_ADD_TARGET      (PANGO GTK4_DEPENDS gobject glib
                                     GTK4_OPTIONAL_DEPENDS harfbuzz)

        _GTK4_FIND_LIBRARY    (PANGOCAIRO pangocairo-1.0 false)
        _GTK4_ADD_TARGET      (PANGOCAIRO GTK4_DEPENDS pango cairo gobject glib)

        _GTK4_FIND_LIBRARY    (PANGOFT2 pangoft2-1.0 false)
        _GTK4_ADD_TARGET      (PANGOFT2 GTK4_DEPENDS pango gobject glib
                                        OPTIONAL_INCLUDES ${FREETYPE_INCLUDE_DIR_ft2build} ${FREETYPE_INCLUDE_DIR_freetype2}
                                                          ${GTK4_FONTCONFIG_INCLUDE_DIR}
                                                          ${GTK4_X11_INCLUDE_DIR})

        _GTK4_FIND_LIBRARY    (PANGOXFT pangoxft-1.0 false)
        _GTK4_ADD_TARGET      (PANGOXFT GTK4_DEPENDS pangoft2 pango gobject glib
                                        OPTIONAL_INCLUDES ${FREETYPE_INCLUDE_DIR_ft2build} ${FREETYPE_INCLUDE_DIR_freetype2}
                                                          ${GTK4_FONTCONFIG_INCLUDE_DIR}
                                                          ${GTK4_X11_INCLUDE_DIR})

        _GTK4_FIND_INCLUDE_DIR(GRAPHENE graphene.h)
        _GTK4_FIND_INCLUDE_DIR(GRAPHENECONFIG graphene-config.h)
        _GTK4_FIND_LIBRARY    (GRAPHENE graphene-1.0 false)
        _GTK4_ADD_TARGET (GRAPHENE GTK4_DEPENDS gobject glib)

        _GTK4_FIND_INCLUDE_DIR(GTK gtk/gtk.h)
        _GTK4_FIND_LIBRARY    (GTK gtk-4 false)
        _GTK4_ADD_TARGET (GTK GTK4_DEPENDS atk pangoft2 pango gdk_pixbuf gthread gobject glib graphene
                              GTK4_OPTIONAL_DEPENDS gio pangocairo cairo)

    elseif(_GTK4_component STREQUAL "gtkmm")

        _GTK4_FIND_INCLUDE_DIR(SIGC++ sigc++/sigc++.h)
        _GTK4_FIND_INCLUDE_DIR(SIGC++CONFIG sigc++config.h)
        _GTK4_FIND_LIBRARY    (SIGC++ sigc-3 true)
        _GTK4_ADD_TARGET      (SIGC++)
        # Since sigc++ 2.5.1 c++11 support is required
        if(GTK4_SIGC++CONFIG_INCLUDE_DIR)
            _GTK4_SIGCXX_GET_VERSION(GTK4_SIGC++_VERSION_MAJOR
                                     GTK4_SIGC++_VERSION_MINOR
                                     GTK4_SIGC++_VERSION_MICRO
                                     ${GTK4_SIGC++CONFIG_INCLUDE_DIR}/sigc++config.h)
            if(NOT ${GTK4_SIGC++_VERSION_MAJOR}.${GTK4_SIGC++_VERSION_MINOR}.${GTK4_SIGC++_VERSION_MICRO} VERSION_LESS 2.5.1)
                # These are the features needed by clients in order to include the
                # project headers:
                set_property(TARGET GTK4::sigc++
                             PROPERTY INTERFACE_COMPILE_FEATURES cxx_alias_templates
                                                                 cxx_auto_type
                                                                 cxx_decltype
                                                                 cxx_deleted_functions
                                                                 cxx_noexcept
                                                                 cxx_nullptr
                                                                 cxx_right_angle_brackets
                                                                 cxx_rvalue_references
                                                                 cxx_variadic_templates)
            endif()
        endif()

        _GTK4_FIND_INCLUDE_DIR(GLIBMM glibmm.h)
        _GTK4_FIND_INCLUDE_DIR(GLIBMMCONFIG glibmmconfig.h)
        _GTK4_FIND_LIBRARY    (GLIBMM glibmm-4.0 true)
        _GTK4_ADD_TARGET      (GLIBMM GTK4_DEPENDS gobject sigc++ glib)

        _GTK4_FIND_INCLUDE_DIR(GIOMM giomm.h)
        _GTK4_FIND_INCLUDE_DIR(GIOMMCONFIG giommconfig.h)
        _GTK4_FIND_LIBRARY    (GIOMM giomm-4.0 true)
        _GTK4_ADD_TARGET      (GIOMM GTK4_DEPENDS gio glibmm gobject sigc++ glib)

        _GTK4_FIND_INCLUDE_DIR(ATKMM atkmm.h)
        _GTK4_FIND_INCLUDE_DIR(ATKMMCONFIG atkmmconfig.h)
        _GTK4_FIND_LIBRARY    (ATKMM atkmm-1.6 true)
        _GTK4_ADD_TARGET      (ATKMM GTK4_DEPENDS atk glibmm gobject sigc++ glib)

        _GTK4_FIND_INCLUDE_DIR(CAIROMM cairomm/cairomm.h)
        _GTK4_FIND_INCLUDE_DIR(CAIROMMCONFIG cairommconfig.h)
        _GTK4_FIND_LIBRARY    (CAIROMM cairomm-1.0 true)
        _GTK4_ADD_TARGET      (CAIROMM GTK4_DEPENDS cairo sigc++
                                       OPTIONAL_INCLUDES ${FREETYPE_INCLUDE_DIR_ft2build} ${FREETYPE_INCLUDE_DIR_freetype2}
                                                         ${GTK4_FONTCONFIG_INCLUDE_DIR}
                                                         ${GTK4_X11_INCLUDE_DIR})

        _GTK4_FIND_INCLUDE_DIR(PANGOMM pangomm.h)
        _GTK4_FIND_INCLUDE_DIR(PANGOMMCONFIG pangommconfig.h)
        _GTK4_FIND_LIBRARY    (PANGOMM pangomm-2.48 true)
        _GTK4_ADD_TARGET      (PANGOMM GTK4_DEPENDS glibmm sigc++ pango gobject glib
                                       GTK4_OPTIONAL_DEPENDS cairomm pangocairo cairo
                                       OPTIONAL_INCLUDES ${FREETYPE_INCLUDE_DIR_ft2build} ${FREETYPE_INCLUDE_DIR_freetype2}
                                                         ${GTK4_FONTCONFIG_INCLUDE_DIR}
                                                         ${GTK4_X11_INCLUDE_DIR})

        _GTK4_FIND_INCLUDE_DIR(GTKMM gtkmm.h)
        _GTK4_FIND_INCLUDE_DIR(GTKMMCONFIG gtkmmconfig.h)
        _GTK4_FIND_LIBRARY    (GTKMM gtkmm-4.0 true)
        _GTK4_ADD_TARGET      (GTKMM GTK4_DEPENDS atkmm gdkmm pangomm gtk glibmm sigc++ gdk atk pangoft2 gdk_pixbuf pango gthread gobject glib
                                     GTK4_OPTIONAL_DEPENDS giomm cairomm gio pangocairo cairo
                                     OPTIONAL_INCLUDES ${FREETYPE_INCLUDE_DIR_ft2build} ${FREETYPE_INCLUDE_DIR_freetype2}
                                                       ${GTK4_FONTCONFIG_INCLUDE_DIR}
                                                       ${GTK4_X11_INCLUDE_DIR})

    else()
        message(FATAL_ERROR "Unknown GTK4 component ${_component}")
    endif()
endforeach()

#
# Solve for the GTK4 version if we haven't already
#
if(NOT GTK4_FIND_VERSION AND GTK4_GTK_INCLUDE_DIR)
    _GTK4_GET_VERSION(GTK4_MAJOR_VERSION
                      GTK4_MINOR_VERSION
                      GTK4_PATCH_VERSION
                      ${GTK4_GTK_INCLUDE_DIR}/gtk/gtkversion.h)
    set(GTK4_VERSION ${GTK4_MAJOR_VERSION}.${GTK4_MINOR_VERSION}.${GTK4_PATCH_VERSION})
endif()

#
# Try to enforce components
#

set(_GTK4_did_we_find_everything true)  # This gets set to GTK4_FOUND

include(FindPackageHandleStandardArgs)

foreach(_GTK4_component ${GTK4_FIND_COMPONENTS})
    string(TOUPPER ${_GTK4_component} _COMPONENT_UPPER)

    set(GTK4_${_COMPONENT_UPPER}_FIND_QUIETLY ${GTK4_FIND_QUIETLY})

    set(FPHSA_NAME_MISMATCHED 1)
    if(_GTK4_component STREQUAL "gtk")
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTK4_${_COMPONENT_UPPER} "Some or all of the gtk libraries were not found."
            GTK4_GTK_LIBRARY
            GTK4_GTK_INCLUDE_DIR

            GTK4_GLIB_INCLUDE_DIR
            GTK4_GLIBCONFIG_INCLUDE_DIR
            GTK4_GLIB_LIBRARY
        )
    elseif(_GTK4_component STREQUAL "gtkmm")
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTK4_${_COMPONENT_UPPER} "Some or all of the gtkmm libraries were not found."
            GTK4_GTKMM_LIBRARY
            GTK4_GTKMM_INCLUDE_DIR
            GTK4_GTKMMCONFIG_INCLUDE_DIR

            GTK4_GLIBMM_INCLUDE_DIR
            GTK4_GLIBMMCONFIG_INCLUDE_DIR
            GTK4_GLIBMM_LIBRARY

            FREETYPE_INCLUDE_DIR_ft2build
            FREETYPE_INCLUDE_DIR_freetype2
        )
    endif()
    unset(FPHSA_NAME_MISMATCHED)

    if(NOT GTK4_${_COMPONENT_UPPER}_FOUND)
        set(_GTK4_did_we_find_everything false)
    endif()
endforeach()

if(GTK4_USE_IMPORTED_TARGETS)
    set(GTK4_LIBRARIES ${GTK4_TARGETS})
endif()


if(_GTK4_did_we_find_everything AND NOT GTK4_VERSION_CHECK_FAILED)
    set(GTK4_FOUND true)
else()
    # Unset our variables.
    set(GTK4_FOUND false)
    set(GTK4_VERSION)
    set(GTK4_VERSION_MAJOR)
    set(GTK4_VERSION_MINOR)
    set(GTK4_VERSION_PATCH)
    set(GTK4_INCLUDE_DIRS)
    set(GTK4_LIBRARIES)
    set(GTK4_TARGETS)
    set(GTK4_DEFINITIONS)
endif()

if(GTK4_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES GTK4_INCLUDE_DIRS)
endif()
