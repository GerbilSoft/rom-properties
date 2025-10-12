# Detours
# Copyright (c) Microsoft Corporation
# License: MIT
PROJECT(Detours LANGUAGES CXX)

# Hide all symbols so they're not exported by any plugins that use this librrary.
CMAKE_POLICY(SET CMP0063 NEW)
SET(CMAKE_C_VISIBILITY_PRESET "hidden")
SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

INCLUDE(CPUInstructionSetFlags)
IF(MSVC AND CPU_ia64)
	# The original Makefile disables warning C4163 on Itanium due to
	# the intrinsic rdtebex not being available.
	# (Using newer Windows headers with an older compiler.)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4163")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4163")
ENDIF(MSVC AND CPU_ia64)

# Sources
SET(${PROJECT_NAME}_SRCS
	src/detours.cpp
	src/modules.cpp
	src/disasm.cpp
	src/image.cpp
	src/creatwth.cpp
	src/disolx86.cpp
	src/disolx64.cpp
	src/disolia64.cpp
	src/disolarm.cpp
	src/disolarm64.cpp
	)

# Headers
SET(${PROJECT_NAME}_H
	src/detours.h
	src/detver.h
	)

ADD_LIBRARY(${PROJECT_NAME} STATIC
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}_H}
	)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/src
	)
