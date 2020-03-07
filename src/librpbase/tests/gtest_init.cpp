/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * gtest_init.c: Google Test initialization.                               *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "librpbase/config.librpbase.h"

// C includes. (C++ namespace)
#include <cstdlib>

// C++ includes.
#include <locale>
using std::locale;

#include "librpbase/common.h"
#include "tcharx.h"

// librpsecure
#include "librpsecure/os-secure.h"

#ifdef _WIN32
// rp_image backend registration.
#include "librptexture/img/RpGdiplusBackend.hpp"
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::RpGdiplusBackend;
using LibRpTexture::rp_image;
#endif /* _WIN32 */

extern "C" int gtest_main(int argc, TCHAR *argv[]);

int RP_C_API _tmain(int argc, TCHAR *argv[])
{
	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = FALSE;
#elif defined(HAVE_SECCOMP)
	static const int syscall_wl[] = {
		// Syscalls used by rp-download.
		// TODO: Add more syscalls.
		// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
		// defined in earlier versions, including Ubuntu 14.04.
		SCMP_SYS(fstat),	// __GI___fxstat() [printf()]
		SCMP_SYS(futex),	// iconv_open()
		SCMP_SYS(mmap),		// iconv_open()
		SCMP_SYS(mmap2),	// iconv_open() [might only be needed on i386...]
		SCMP_SYS(mprotect),	// iconv_open()
		SCMP_SYS(munmap),	// free() [in some cases]
		SCMP_SYS(lseek),
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#ifdef __SNR_openat2
		SCMP_SYS(openat2),	// Linux 5.6
#endif /* __SNR_openat2 */

		// Google Test
		SCMP_SYS(getcwd),	// testing::internal::FilePath::GetCurrentDir()
					// - testing::internal::UnitTestImpl::AddTestInfo()
		SCMP_SYS(ioctl),	// testing::internal::posix::IsATTY()

		// MiniZip
		SCMP_SYS(close),	// mktime() [mz_zip_dosdate_to_time_t()]
		SCMP_SYS(stat),		// mktime() [mz_zip_dosdate_to_time_t()]

		-1	// End of whitelist
	};
	param.syscall_wl = syscall_wl;
#elif defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read test cases.
	param.promises = "stdio rpath";
#elif defined(HAVE_TAME)
	param.tame_flags = TAME_STDIO | TAME_RPATH;
#else
	param.dummy = 0;
#endif
	rp_secure_enable(param);

#ifdef _WIN32
	// Register RpGdiplusBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
#endif /* _WIN32 */

	// Set the C and C++ locales.
	// NOTE: The variable needs to be static char[] because
	// POSIX putenv() takes `char*` and the buffer becomes
	// part of the environment.
	static TCHAR lc_all_env[] = _T("LC_ALL=C");
	_tputenv(lc_all_env);
	locale::global(locale("C"));

	// Call the actual main function.
	return gtest_main(argc, argv);
}
