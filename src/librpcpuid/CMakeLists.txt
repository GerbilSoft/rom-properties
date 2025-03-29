# rom-properties CPU identification library
# NOTE: This is intended to be statically linked to every
# library or executable that needs it, NOT exported from
# libromdata.so.X / romdata-X.dll.
PROJECT(rpcpuid LANGUAGES C)

# Check for C headers
CHECK_INCLUDE_FILES("cpuid.h" HAVE_CPUID_H)

# Sources
SET(${PROJECT_NAME}_SRCS dummy.c)
SET(${PROJECT_NAME}_H cpu_dispatch.h)

# CPU-specific and optimized sources
INCLUDE(CPUInstructionSetFlags)
IF(CPU_i386 OR CPU_amd64)
	SET(${PROJECT_NAME}_CPU_SRCS cpuflags_x86.c)
	SET(${PROJECT_NAME}_CPU_H cpuflags_x86.h cpuid_x86.h)
ELSEIF(CPU_arm OR CPU_arm64)
	SET(${PROJECT_NAME}_CPU_SRCS cpuflags_arm.c)
	SET(${PROJECT_NAME}_CPU_H cpuflags_arm.h)
ENDIF()

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.librpcpuid.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.librpcpuid.h")

######################
# Build the library. #
######################

ADD_LIBRARY(${PROJECT_NAME} STATIC
	${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_OS_SRCS} ${${PROJECT_NAME}_OS_H}
	${${PROJECT_NAME}_CPU_SRCS} ${${PROJECT_NAME}_CPU_H}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(${PROJECT_NAME})
# Exclude from ALL builds; enable PIC.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	POSITION_INDEPENDENT_CODE TRUE
	)

# rpthreads is needed for atomic functions.
# FIXME: Not using it in the static rpcpuid library right now.
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpthreads)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpcpuid
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpcpuid
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)
