SET(HOST_SYSTEM i686-w64-mingw32)

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER	${HOST_SYSTEM}-gcc)
SET(CMAKE_CXX_COMPILER	${HOST_SYSTEM}-g++)
SET(CMAKE_RC_COMPILER	${HOST_SYSTEM}-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/${HOST_SYSTEM})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
