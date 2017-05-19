# MSVC runtime selection macro.
# References:
# - http://stackoverflow.com/q/10113017
# - http://stackoverflow.com/questions/10113017/setting-the-msvc-runtime-in-cmake

# TODO: Add support for MinGW.

# Parameters:
# - _crt: static for static CRT; anything else for the DLL CRT.
# - ...: configurations to apply the CRT selection to (if not specified, all will be set)
# Specify target configurations as parameters.
# If no configurations are specified, all will be used.
macro(configure_msvc_runtime _crt)
  if(MSVC)
    # Default to statically-linked runtime.
    if("${_crt}" STREQUAL "")
      set(_crt "static")
    endif()
    # Set compiler options.
    set(list_var "${ARGN}")
    unset(variables)
    foreach(arg IN LISTS list_var)
      STRING(TOUPPER ${arg} arg)
      set(variables ${variables} CMAKE_C_FLAGS_${arg} CMAKE_CXX_FLAGS_${arg})
    endforeach()
    unset(list_var)
    if(NOT variables)
      set(variables
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
      )
    endif(NOT variables)
    if(${_crt} STREQUAL "static")
      message(STATUS
        "MSVC -> forcing use of statically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
      endforeach()
    else()
      message(STATUS
        "MSVC -> forcing use of dynamically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MT")
          string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
        endif()
      endforeach()
    endif()
  endif()
endmacro()
