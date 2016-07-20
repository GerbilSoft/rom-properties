# Microsoft Visual C++

# CMake has a bunch of defaults, including /Od for debug and /O2 for release.
# Remove some default CFLAGS/CXXFLAGS.
STRING(REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
STRING(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Disable useless warnings:
# - MSVC "logo" messages
# - C4355: 'this' used in base member initializer list (used for Qt Dpointer pattern)
# - MSVCRT "deprecated" functions
SET(GENS_C_FLAGS_COMMON "-nologo -wd4355 -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
SET(GENS_CXX_FLAGS_COMMON "${GENS_C_FLAGS_COMMON}")
# NOTE: -tsaware is automatically set for Windows 2000 and later. (as of at least Visual Studio .NET 2003)
SET(GENS_EXE_LINKER_FLAGS_COMMON "-nologo /manifest:no -dynamicbase -nxcompat -largeaddressaware")
SET(GENS_SHARED_LINKER_FLAGS_COMMON "${GENS_EXE_LINKER_FLAGS_COMMON}")

# Disable C++ RTTI and asynchronous exceptions.
SET(GENS_CXX_FLAGS_COMMON "${GENS_CXX_FLAGS_COMMON} -GR- -EHsc")

# Disable the RC and MASM "logo".
SET(CMAKE_RC_FLAGS "-nologo")
SET(CMAKE_ASM_MASM_FLAGS "-nologo")

# Check for link-time optimization.
IF(ENABLE_LTO)
	SET(GENS_C_FLAGS_COMMON "${GENS_C_FLAGS_COMMON} -GL")
	SET(GENS_CXX_FLAGS_COMMON "${GENS_CXX_FLAGS_COMMON} -GL")
	SET(GENS_EXE_LINKER_FLAGS_COMMON "${GENS_EXE_LINKER_FLAGS_COMMON} -LTCG")
	SET(GENS_SHARED_LINKER_FLAGS_COMMON "${GENS_SHARED_LINKER_FLAGS_COMMON} -LTCG")
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

# Debug/release flags.
SET(GENS_C_FLAGS_DEBUG			"-Zi")
SET(GENS_CXX_FLAGS_DEBUG		"-Zi")
SET(GENS_EXE_LINKER_FLAGS_DEBUG		 "-debug -incremental")
SET(GENS_SHARED_LINKER_FLAGS_DEBUG 	"${GENS_EXE_LINKER_FLAGS_DEBUG}")
SET(GENS_C_FLAGS_RELEASE		"-Zi")
SET(GENS_CXX_FLAGS_RELEASE		"-Zi")
SET(GENS_EXE_LINKER_FLAGS_RELEASE	"-debug -incremental:no -opt:icf,ref")
SET(GENS_SHARED_LINKER_FLAGS_RELEASE	"${GENS_EXE_LINKER_FLAGS_RELEASE}")
