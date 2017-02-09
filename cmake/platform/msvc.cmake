# Microsoft Visual C++
IF(MSVC_VERSION LESS 1600)
	MESSAGE(FATAL_ERROR "MSVC 2010 (10.0) or later is required.")
ENDIF()

# Disable useless warnings:
# - MSVC "logo" messages
# - C4355: 'this' used in base member initializer list (used for Qt Dpointer pattern)
# - MSVCRT "deprecated" functions
SET(RP_C_FLAGS_COMMON "/nologo /wd4355 /wd4482 -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
SET(RP_CXX_FLAGS_COMMON "${RP_C_FLAGS_COMMON}")
# NOTE: /TSAWARE is automatically set for Windows 2000 and later. (as of at least Visual Studio .NET 2003)
# NOTE 2: /TSAWARE is not applicable for DLLs.
SET(RP_EXE_LINKER_FLAGS_COMMON "/NOLOGO /DYNAMICBASE /NXCOMPAT /LARGEADDRESSAWARE")
SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")
SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")

# Check what flag is needed for stack smashing protection.
INCLUDE(CheckStackProtectorCompilerFlag)
CHECK_STACK_PROTECTOR_COMPILER_FLAG(RP_STACK_CFLAG)
SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${RP_STACK_CFLAG}")
SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${RP_STACK_CFLAG}")
UNSET(RP_STACK_CFLAG)

# Test for "/sdl".
INCLUDE(CheckCCompilerFlag)
FOREACH(FLAG_TEST "-sdl")
	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST})
	IF(CFLAG_${FLAG_TEST})
		SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CFLAG_${FLAG_TEST})
	UNSET(CFLAG_${FLAG_TEST})
ENDFOREACH()

# Disable warning C4996 (deprecated), then re-enable it.
# Otherwise, it gets handled as an error due to /sdl.
SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} /wd4996 /w34996")
SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /wd4996 /w34996")

# Disable the RC and MASM "logo".
# FIXME: Setting CMAKE_RC_FLAGS causes msbuild to fail,
# since CMake already sets /NOLOGO there.
# - Also disabling /NOLOGO for MASM just in case.
#SET(CMAKE_RC_FLAGS "/NOLOGO")
#SET(CMAKE_ASM_MASM_FLAGS "/NOLOGO")

# FIXME: MSVC 2015's 32-bit masm has problems when using msbuild:
# - The default /W3 fails for seemingly no reason. /W0 fixes it.
# - Compilation fails due to no SAFESEH handlers in inffas32.asm.
# NOTE: We're enabling these for all MSVC platforms, not just 32-bit.
# NOTE 2: We need to cache this in order to prevent random build failures
# caused by an empty string being cached instead.
SET(CMAKE_ASM_MASM_FLAGS "/W0 /safeseh" CACHE STRING
     "Flags used by the assembler during all build types.")

# Check for link-time optimization.
IF(ENABLE_LTO)
	SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} -GL")
	SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} -GL")
	SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} -LTCG")
	SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_SHARED_LINKER_FLAGS_COMMON} -LTCG")
	SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_MODULE_LINKER_FLAGS_COMMON} -LTCG")
ENDIF(ENABLE_LTO)

# Check if CMAKE_SIZEOF_VOID_P is set correctly.
IF(NOT CMAKE_SIZEOF_VOID_P)
	# CMAKE_SIZEOF_VOID_P isn't set.
	# Set it based on CMAKE_SYSTEM_PROCESSOR.
	# FIXME: This won't work if we're cross-compiling, e.g. using
	# the x86_amd64 or amd64_x86 toolchains.
	STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch)
	IF(arch MATCHES "^x86_64$|^amd64$|^ia64$")
		SET(CMAKE_SIZEOF_VOID_P 8)
	ELSEIF(arch MATCHES "^(i.|x)86$")
		SET(CMAKE_SIZEOF_VOID_P 4)
	ELSE()
		# Assume other CPUs are 32-bit.
		SET(CMAKE_SIZEOF_VOID_P 4)
	ENDIF()
	UNSET(arch)
ENDIF(NOT CMAKE_SIZEOF_VOID_P)

# TODO: Code coverage checking for MSVC?
IF(ENABLE_COVERAGE)
	MESSAGE(FATAL_ERROR "Code coverage testing is currently only supported on gcc and clang.")
ENDIF(ENABLE_COVERAGE)

# Debug/release flags.
SET(RP_C_FLAGS_DEBUG			"-Zi")
SET(RP_CXX_FLAGS_DEBUG			"-Zi")
SET(RP_EXE_LINKER_FLAGS_DEBUG		 "/DEBUG /INCREMENTAL")
SET(RP_SHARED_LINKER_FLAGS_DEBUG 	"${RP_EXE_LINKER_FLAGS_DEBUG}")
SET(RP_MODULE_LINKER_FLAGS_DEBUG 	"${RP_EXE_LINKER_FLAGS_DEBUG}")
SET(RP_C_FLAGS_RELEASE			"-Zi")
SET(RP_CXX_FLAGS_RELEASE		"-Zi")
SET(RP_EXE_LINKER_FLAGS_RELEASE		"/DEBUG /INCREMENTAL:NO /OPT:ICF,REF")
SET(RP_SHARED_LINKER_FLAGS_RELEASE	"${RP_EXE_LINKER_FLAGS_RELEASE}")
SET(RP_MODULE_LINKER_FLAGS_RELEASE	"${RP_EXE_LINKER_FLAGS_RELEASE}")
