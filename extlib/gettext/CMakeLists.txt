# rom-properties: Remove "-Werror=undef".
string(REPLACE "-Werror=undef" "" CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}")
string(REPLACE "-Werror=undef" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

ADD_SUBDIRECTORY(gettext-runtime)
