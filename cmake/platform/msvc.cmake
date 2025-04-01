# Microsoft Visual C++
IF(MSVC_VERSION LESS 1800)
	MESSAGE(FATAL_ERROR "MSVC 2013 (12.0) or later is required.")
ENDIF()

# If an SDK version isn't specified by the user, set it to 10.0.
IF(NOT CMAKE_SYSTEM_VERSION)
	SET(CMAKE_SYSTEM_VERSION 10.0)
ENDIF(NOT CMAKE_SYSTEM_VERSION)

# Disable useless warnings:
# - MSVC "logo" messages
# - C4005: macro redefinition (libpng's intprefix.out.tf1 is breaking on this...)
# - C4091: 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
# - C4355: 'this' used in base member initializer list (used for Qt Dpointer pattern)
# - C4800: 'BOOL': forcing value to bool 'true' or 'false' (performance warning)
# - MSVCRT "deprecated" functions
# - std::tr1 deprecation
# Increase some warnings to errors:
# - C4013: function undefined; this is allowed in C, but will
#   probably cause a linker error.
# - C4024: 'function': different types for formal and actual parameter n
# - C4047: 'function': 'parameter' differs in levels of indirection from 'argument'
# - C4477: 'function' : format string 'string' requires an argument of type 'type', but variadic argument number has type 'type'
SET(RP_C_FLAGS_COMMON "/nologo /W3 /WX /wd4005 /wd4091 /wd4355 /wd4800 /we4013 /we4024 /we4047 /we4477")
SET(RP_CXX_FLAGS_COMMON "${RP_C_FLAGS_COMMON} -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
# NOTE: /TSAWARE is automatically set for Windows 2000 and later. (as of at least Visual Studio .NET 2003)
# NOTE 2: /TSAWARE is not applicable for DLLs.
SET(RP_EXE_LINKER_FLAGS_COMMON "/NOLOGO /DYNAMICBASE /NXCOMPAT /LARGEADDRESSAWARE")
SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")
SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON}")

# Add /EHsc if it isn't present already.
# Default in most cases; not enabled for MSVC 2019 on ARM or ARM64.
IF(NOT CMAKE_CXX_FLAGS MATCHES "/EHsc")
	SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /EHsc")
ENDIF(NOT CMAKE_CXX_FLAGS MATCHES "/EHsc")

# Add /MP for multi-processor compilation.
SET(RP_C_FLAGS_COMMON   "${RP_C_FLAGS_COMMON} /MP")
SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /MP")

# Test for MSVC-specific compiler flags.
# /utf-8 was added in MSVC 2015.
INCLUDE(CheckCCompilerFlag)
FOREACH(FLAG_TEST "/sdl" "/utf-8" "/guard:cf" "/guard:ehcont")
	# CMake doesn't like certain characters in variable names.
	STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST_VARNAME})
	IF(CFLAG_${FLAG_TEST_VARNAME})
		SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
		# "/guard:*" must be added to linker flags in addition to CFLAGS.
		IF(FLAG_TEST MATCHES "^/guard:")
			SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} ${FLAG_TEST}")
			SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_SHARED_LINKER_FLAGS_COMMON} ${FLAG_TEST}")
			SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_MODULE_LINKER_FLAGS_COMMON} ${FLAG_TEST}")
		ENDIF(FLAG_TEST MATCHES "^/guard:")
	ENDIF(CFLAG_${FLAG_TEST_VARNAME})
	UNSET(CFLAG_${FLAG_TEST_VARNAME})
ENDFOREACH()

# Enable /SAFESEH. (i386 only)
IF(_MSVC_C_ARCHITECTURE_FAMILY MATCHES "^([iI]?[xX3]86)$")
	SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} /SAFESEH")
	SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_SHARED_LINKER_FLAGS_COMMON} /SAFESEH")
	SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_MODULE_LINKER_FLAGS_COMMON} /SAFESEH")
ENDIF()

# MSVC 2019: Enable /CETCOMPAT.
# NOTE: i386/amd64 only. (last checked in MSVC 2022 [17.0])
# - LINK : fatal error LNK1246: '/CETCOMPAT' not compatible with 'ARM' target machine; link without '/CETCOMPAT'
# - LINK : fatal error LNK1246: '/CETCOMPAT' not compatible with 'ARM64' target machine; link without '/CETCOMPAT'
IF(MSVC_VERSION GREATER 1919 AND _MSVC_C_ARCHITECTURE_FAMILY MATCHES "^([iI]?[xX3]86)|([xX]64)$")
	SET(RP_EXE_LINKER_FLAGS_COMMON "${RP_EXE_LINKER_FLAGS_COMMON} /CETCOMPAT")
	SET(RP_SHARED_LINKER_FLAGS_COMMON "${RP_SHARED_LINKER_FLAGS_COMMON} /CETCOMPAT")
	SET(RP_MODULE_LINKER_FLAGS_COMMON "${RP_MODULE_LINKER_FLAGS_COMMON} /CETCOMPAT")
ENDIF()

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

# MSVC: C/C++ conformance settings
IF(CMAKE_SYSTEM_VERSION VERSION_GREATER 9.9)
	FOREACH(FLAG_TEST "/permissive-")
		# CMake doesn't like certain characters in variable names.
		STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

		CHECK_C_COMPILER_FLAG("${FLAG_TEST}" CFLAG_${FLAG_TEST_VARNAME})
		IF(CFLAG_${FLAG_TEST_VARNAME})
			SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} ${FLAG_TEST}")
			SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
		ENDIF(CFLAG_${FLAG_TEST_VARNAME})
		UNSET(CFLAG_${FLAG_TEST_VARNAME})
	ENDFOREACH()
ENDIF()

# MSVC: C++ conformance settings
INCLUDE(CheckCXXCompilerFlag)
SET(CXX_CONFORMANCE_FLAGS "/Zc:__cplusplus" "/Zc:checkGwOdr" "/Zc:rvalueCast" "/Zc:templateScope" "/Zc:ternary")
IF(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# clang-cl enables certain conformance options by default,
	# and these cause warnings to be printed if specified.
	# Only enable these for original MSVC.
	SET(CXX_CONFORMANCE_FLAGS ${CXX_CONFORMANCE_FLAGS} "/Zc:externC" "/Zc:noexceptTypes" "/Zc:throwingNew")
ENDIF(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
FOREACH(FLAG_TEST ${CXX_CONFORMANCE_FLAGS})
	# CMake doesn't like certain characters in variable names.
	STRING(REGEX REPLACE "/|:|=" "_" FLAG_TEST_VARNAME "${FLAG_TEST}")

	CHECK_CXX_COMPILER_FLAG("${FLAG_TEST}" CXXFLAG_${FLAG_TEST_VARNAME})
	IF(CXXFLAG_${FLAG_TEST_VARNAME})
		SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} ${FLAG_TEST}")
	ENDIF(CXXFLAG_${FLAG_TEST_VARNAME})
	UNSET(CXXFLAG_${FLAG_TEST_VARNAME})
ENDFOREACH()

# Disable warning C4996 (deprecated), then re-enable it.
# Otherwise, it gets handled as an error due to /sdl.
SET(RP_C_FLAGS_COMMON "${RP_C_FLAGS_COMMON} /wd4996 /w34996")
SET(RP_CXX_FLAGS_COMMON "${RP_CXX_FLAGS_COMMON} /wd4996 /w34996")

# MSVC 2015 uses thread-safe statics by default.
# This doesn't work on Windows XP or Windows Server 2003, so disable it.
# NOTE: Only for i386 and amd64; enabling elsewhere because
# Windows XP and Windows Server 2003 weren't available for ARM.
IF(MSVC_VERSION GREATER 1899 AND _MSVC_C_ARCHITECTURE_FAMILY MATCHES "^([iI]?[xX3]86)|([xX]64)$")
	MESSAGE(STATUS "MSVC: Disabling thread-safe statics for Windows XP and Windows Server 2003 compatibility")
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

# MSVC needs a flag to automatically NULL-terminate strings in string tables.
# (It ignores the "\0" in string tables, too.)
SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /n")

# Dependent load flags (MSVC 2017+, Windows 10 1607+)
# For most executables and DLLs, this will be set to 0x800 (LOAD_LIBRARY_SEARCH_SYSTEM32),
# which disables loading DLLs from the current directory and application directory.
# For unit tests, we have to use 0xA00, which adds LOAD_LIBRARY_SEARCH_APPLICATION_DIR,
# because Google Test doesn't support delay-load due to exported data symbols.
#
# LINK : fatal error LNK1194: cannot delay-load 'gtestd.dll' due to import of data symbol
# '"__declspec(dllimport) class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > testing::FLAGS_gtest_death_test_style"'
# link without /DELAYLOAD:gtestd.dll
IF(MSVC_VERSION GREATER 1909)
	SET(RP_LINKER_FLAG_DEPENDENT_LOAD_FLAG_DEFAULT "/DEPENDENTLOADFLAG:0x800")
	SET(RP_LINKER_FLAG_DEPENDENT_LOAD_FLAG_GTEST   "/DEPENDENTLOADFLAG:0xA00")

	MACRO(SET_DEPENDENT_LOAD_FLAG_GTEST)
		FOREACH(_type EXE SHARED MODULE)
			STRING(REPLACE "${RP_LINKER_FLAG_DEPENDENT_LOAD_FLAG_DEFAULT}" "${RP_LINKER_FLAG_DEPENDENT_LOAD_FLAG_GTEST}" CMAKE_${_type}_LINKER_FLAGS "${CMAKE_${_type}_LINKER_FLAGS}")
		ENDFOREACH(_type)
	ENDMACRO(SET_DEPENDENT_LOAD_FLAG_GTEST)
ENDIF(MSVC_VERSION GREATER 1909)

FOREACH(_type EXE SHARED MODULE)
	SET(RP_${_type}_LINKER_FLAGS_COMMON "${RP_${_type}_LINKER_FLAGS_COMMON} ${RP_LINKER_FLAG_DEPENDENT_LOAD_FLAG_DEFAULT}")
ENDFOREACH(_type)

# TODO: Code coverage checking for MSVC?
IF(ENABLE_COVERAGE)
	MESSAGE(FATAL_ERROR "Code coverage testing is currently only supported on gcc and clang.")
ENDIF(ENABLE_COVERAGE)

### Debug/Release flags ###

SET(RP_C_FLAGS_DEBUG			"/Zi ${RP_C_FLAGS_DEBUG}")
SET(RP_CXX_FLAGS_DEBUG			"/Zi ${RP_CXX_FLAGS_DEBUG}")
SET(RP_EXE_LINKER_FLAGS_DEBUG		"/DEBUG /INCREMENTAL")
SET(RP_SHARED_LINKER_FLAGS_DEBUG 	"${RP_EXE_LINKER_FLAGS_DEBUG}")
SET(RP_MODULE_LINKER_FLAGS_DEBUG 	"${RP_EXE_LINKER_FLAGS_DEBUG}")

SET(RP_C_FLAGS_RELEASE			"/Zi")
SET(RP_CXX_FLAGS_RELEASE		"/Zi")
SET(RP_EXE_LINKER_FLAGS_RELEASE		"/DEBUG /INCREMENTAL:NO /OPT:ICF,REF")
SET(RP_SHARED_LINKER_FLAGS_RELEASE	"${RP_EXE_LINKER_FLAGS_RELEASE}")
SET(RP_MODULE_LINKER_FLAGS_RELEASE	"${RP_EXE_LINKER_FLAGS_RELEASE}")

SET(RP_C_FLAGS_RELWITHDEBINFO			"/Zi")
SET(RP_CXX_FLAGS_RELWITHDEBINFO			"/Zi")
SET(RP_EXE_LINKER_FLAGS_RELWITHDEBINFO		"/DEBUG /INCREMENTAL:NO /OPT:ICF,REF")
SET(RP_SHARED_LINKER_FLAGS_RELWITHDEBINFO	"${RP_EXE_LINKER_FLAGS_RELWITHDEBINFO}")
SET(RP_MODULE_LINKER_FLAGS_RELWITHDEBINFO	"${RP_EXE_LINKER_FLAGS_RELWITHDEBINFO}")

# Check for link-time optimization. (Release builds only.)
IF(ENABLE_LTO)
	SET(RP_C_FLAGS_RELEASE			"${RP_C_FLAGS_RELEASE} /GL")
	SET(RP_CXX_FLAGS_RELEASE		"${RP_CXX_FLAGS_RELEASE} /GL")
	SET(RP_EXE_LINKER_FLAGS_RELEASE		"${RP_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	SET(RP_SHARED_LINKER_FLAGS_RELEASE	"${RP_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
	SET(RP_MODULE_LINKER_FLAGS_RELEASE	"${RP_MODULE_LINKER_FLAGS_RELEASE} /LTCG")

	SET(RP_C_FLAGS_RELWITHDEBINFO			"${RP_C_FLAGS_RELWITHDEBINFO} /GL")
	SET(RP_CXX_FLAGS_RELWITHDEBINFO			"${RP_CXX_FLAGS_RELWITHDEBINFO} /GL")
	SET(RP_EXE_LINKER_FLAGS_RELWITHDEBINFO		"${RP_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
	SET(RP_SHARED_LINKER_FLAGS_RELWITHDEBINFO	"${RP_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
	SET(RP_MODULE_LINKER_FLAGS_RELWITHDEBINFO	"${RP_MODULE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
ENDIF(ENABLE_LTO)
