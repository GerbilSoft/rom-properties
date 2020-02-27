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
	// TODO: Non-Windows syscall stuff.
#ifdef _WIN32
	rp_secure_param_t param;
	param.bHighSec = FALSE;
	rp_secure_enable(param);

	// Register RpGdiplusBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
#endif /* _WIN32 */

	// TODO: setenv() wrapper in config.librpbase.h.in?
#if defined(HAVE_SETENV)
	setenv("LC_ALL", "C", true);
#elif defined(_UNICODE) && defined(HAVE__WPUTENV_S)
	_wputenv_s(L"LC_ALL", L"C");
#elif defined(_UNICODE) && defined(HAVE__WPUTENV)
	_wputenv(L"LC_ALL=C");
#elif defined(HAVE_PUTENV)
	putenv("LC_ALL=C");
#else
# error Could not find a usable putenv() or setenv() function.
#endif

	// Set the C and C++ locales.
	locale::global(locale("C"));

	// Call the actual main function.
	return gtest_main(argc, argv);
}
