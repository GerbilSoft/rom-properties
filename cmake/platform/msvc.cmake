# Microsoft Visual C++
IF(MSVC_VERSION LESS 1600)
	MESSAGE(FATAL_ERROR "MSVC 2010 (10.0) or later is required.")
ENDIF()

# Disable useless warnings:
# - MSVC "logo" messages
# - C4355: 'this' used in base member initializer list (used for Qt Dpointer pattern)
# - MSVCRT "deprecated" functions
# - std::tr1 deprecation
# Increase some warnings to errors:
# - C4013: function undefined; this is allowed in C, but will
#   probably cause a linker error.
# - C4024: 'function': different types for formal and actual parameter n
# - C4047: 'function': 'parameter' differs in levels of indirection from 'argument'
SET(RP_C_FLAGS_COMMON "/nologo /wd4355 /wd4482 /we4013 /we4024 /we4047")
SET(RP_CXX_FLAGS_COMMON "${RP_C_FLAGS_COMMON} -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
# NOTE: /TSAWARE is automatically set for Windows 2000 and later. (as of at least Visual Studio .NET 2003)
# NOTE 2: /TSAWARE is not applicable for DLLs.
SET(RP_EXE_LINKER_FLAGS_COMMON "/NOLOGO /DYNAMICBASE /NXCOMPAT /LARGEADDRESSAWARE")
SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")
SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")

# Enable /EHsc if it isn't enabled already.
# Default in most cases; not enabled for MSVC 2019 on ARM or ARM64.
IF(NOT CMAKE_CXX_FLAGS MATCHES "/EHsc")
	SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /EHsc")
ENDIF(NOT CMAKE_CXX_FLAGS MATCHES "/EHsc")

# Test for MSVC-specific compiler flags.
# /utf-8 was added in MSVC 2015.
INCLUDE(CheckCCompilerFlag)
FOREACH(FLAG_TEST "/sdl" "/guard:cf" "/utf-8")
	# CMake doesn't like certain characters in variable names.
	STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST_VARNAME})
	IF(CFLAG_${FLAG_TEST_VARNAME})
		SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CFLAG_${FLAG_TEST_VARNAME})
	UNSET(CFLAG_${FLAG_TEST_VARNAME})
ENDFOREACH()

# "/guard:cf" must be added to linker flags in addition to CFLAGS.
CHECK_C_COMPILER_FLAG("/guard:cf" CFLAG__guard_cf)
IF(CFLAG_guard_cf)
	SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} /guard:cf")
	SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /guard:cf")
	SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} /guard:cf")
	SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_SHARED_LINKER_FLAGS_COMMON} /guard:cf")
	SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_MODULE_LINKER_FLAGS_COMMON} /guard:cf")
ENDIF(CFLAG_guard_cf)
UNSET(CFLAG_guard_cf)

# MSVC: C/C++ conformance settings
FOREACH(FLAG_TEST "/Zc:wchar_t" "/Zc:inline")
	# CMake doesn't like certain characters in variable names.
	STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST_VARNAME})
	IF(CFLAG_${FLAG_TEST_VARNAME})
		SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CFLAG_${FLAG_TEST_VARNAME})
	UNSET(CFLAG_${FLAG_TEST_VARNAME})
ENDFOREACH()

# MSVC: C++ conformance settings
INCLUDE(CheckCXXCompilerFlag)
FOREACH(FLAG_TEST "/Zc:__cplusplus" "/Zc:externC" "/Zc:noexceptTypes" "/Zc:rvalueCast" "/Zc:ternary")
	# CMake doesn't like certain characters in variable names.
	STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_CXX_COMPILER_FLAG("${FLAG_TEST}" CXXFLAG_${FLAG_TEST_VARNAME})
	IF(CXXFLAG_${FLAG_TEST_VARNAME})
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CXXFLAG_${FLAG_TEST_VARNAME})
	UNSET(CXXFLAG_${FLAG_TEST_VARNAME})
ENDFOREACH()

# "/Zc:throwingNew" is always enabled on clang-cl, and causes
# warnings to be printed if it's specified.
# NOTE: "/Zc:throwingNew" was added in MSVC 2015.
IF(NOT CMAKE_CXX_COMPILER_ID STREQUAL Clang)
	INCLUDE(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("/Zc:throwingNew" CXXFLAG_Zc_throwingNew)
	IF(CXXFLAG_Zc_throwingNew)
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /Zc:throwingNew")
	ENDIF(CXXFLAG_Zc_throwingNew)
	UNSET(CXXFLAG_Zc_throwingNew)
ENDIF(NOT CMAKE_CXX_COMPILER_ID STREQUAL Clang)

# Disable warning C4996 (deprecated), then re-enable it.
# Otherwise, it gets handled as an error due to /sdl.
SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} /wd4996 /w34996")
SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /wd4996 /w34996")

# MSVC 2015 uses thread-safe statics by default.
# This doesn't work on XP, so disable it.
IF(MSVC_VERSION GREATER 1899)
	SET(RP_C_FLAGS_COMMON   "${RP_C_FLAGS_COMMON} /Zc:threadSafeInit-")
	SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /Zc:threadSafeInit-")
ENDIF()

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

# Check if CMAKE_SIZEOF_VOID_P is set correctly.
IF(NOT CMAKE_SIZEOF_VOID_P)
	# CMAKE_SIZEOF_VOID_P isn't set.
	# Set it based on CMAKE_SYSTEM_PROCESSOR.
	# FIXME: This won't work if we're cross-compiling, e.g. using
	# the x86_amd64 or amd64_x86 toolchains.
	IF(CMAKE_CL_64)
		SET(CMAKE_SIZEOF_VOID_P 8)
	ELSE()
		SET(CMAKE_SIZEOF_VOID_P 4)
	ENDIF()
ENDIF(NOT CMAKE_SIZEOF_VOID_P)

# TODO: Code coverage checking for MSVC?
IF(ENABLE_COVERAGE)
	MESSAGE(FATAL_ERROR "Code coverage testing is currently only supported on gcc and clang.")
ENDIF(ENABLE_COVERAGE)

# Debug/release flags.
SET(RP_C_FLAGS_DEBUG			"/Zi")
SET(RP_CXX_FLAGS_DEBUG			"/Zi")
SET(RP_EXE_LINKER_FLAGS_DEBUG		 "/DEBUG /INCREMENTAL")
SET(RP_SHARED_LINKER_FLAGS_DEBUG 	"${RP_EXE_LINKER_FLAGS_DEBUG}")
SET(RP_MODULE_LINKER_FLAGS_DEBUG 	"${RP_EXE_LINKER_FLAGS_DEBUG}")
SET(RP_C_FLAGS_RELEASE			"/Zi")
SET(RP_CXX_FLAGS_RELEASE		"/Zi")
SET(RP_EXE_LINKER_FLAGS_RELEASE		"/DEBUG /INCREMENTAL:NO /OPT:ICF,REF")
SET(RP_SHARED_LINKER_FLAGS_RELEASE	"${RP_EXE_LINKER_FLAGS_RELEASE}")
SET(RP_MODULE_LINKER_FLAGS_RELEASE	"${RP_EXE_LINKER_FLAGS_RELEASE}")

# Check for link-time optimization. (Release builds only.)
IF(ENABLE_LTO)
	SET(RP_C_FLAGS_RELEASE			"${RP_C_FLAGS_RELEASE} /GL")
	SET(RP_CXX_FLAGS_RELEASE		"${RP_CXX_FLAGS_RELEASE} /GL")
	SET(RP_EXE_LINKER_FLAGS_RELEASE		"${RP_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	SET(RP_SHARED_LINKER_FLAGS_RELEASE	"${RP_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
	SET(RP_MODULE_LINKER_FLAGS_RELEASE	"${RP_MODULE_LINKER_FLAGS_RELEASE} /LTCG")
ENDIF(ENABLE_LTO)
