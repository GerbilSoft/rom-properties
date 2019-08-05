/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * gtest_init.c: Google Test initialization.                               *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "librpbase/config.librpbase.h"

// C++ includes.
#include <locale>
using std::locale;

#include "librpbase/common.h"

#ifdef _WIN32
#include "libwin32common/secoptions.h"

// rp_image backend registration.
#include "librptexture/img/RpGdiplusBackend.hpp"
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::RpGdiplusBackend;
using LibRpTexture::rp_image;
#endif /* _WIN32 */

extern "C" int gtest_main(int argc, char *argv[]);

int RP_C_API main(int argc, char *argv[])
{
#ifdef _WIN32
	// Set Win32 security options.
	secoptions_init();

	// Register RpGdiplusBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
#endif

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
