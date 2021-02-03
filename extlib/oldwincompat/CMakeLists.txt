# Compatibility library for older versions of Windows.
# References:
# - https://stackoverflow.com/questions/19516796/visual-studio-2012-win32-project-targeting-windows-2000/53548116
# - https://stackoverflow.com/a/53548116
PROJECT(oldwincompat LANGUAGES C)

IF(NOT MSVC)
	MESSSAGE(FATAL_ERROR "liboldwincompat only supports MSVC right now.")
ENDIF(NOT MSVC)
IF(CMAKE_CL_64)
	MESSSAGE(FATAL_ERROR "liboldwincompat does not, and never will, support 64-bit.")
ENDIF(CMAKE_CL_64)

# Sources.
ENABLE_LANGUAGE(ASM_MASM)
SET(liboldwincompat_SRCS
	win2kstubs.asm
	win9xstubs.c
	)

######################
# Build the library. #
######################

# NOTE: win32-msvc.cmake adds oldwincompat to LINK_LIBRARIES().
# To prevent warnings, we'll need to disable policy CMP0038.
IF(POLICY CMP0038)
	CMAKE_POLICY(SET CMP0038 OLD)
ENDIF(POLICY CMP0038)

ADD_LIBRARY(oldwincompat STATIC ${liboldwincompat_SRCS})
