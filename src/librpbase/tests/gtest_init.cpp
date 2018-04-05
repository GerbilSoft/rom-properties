/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * gtest_init.c: Google Test initialization.                               *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "librpbase/config.librpbase.h"

// C++ includes.
#include <locale>
using std::locale;

#include "librpbase/common.h"

#ifdef _WIN32
#include "libwin32common/secoptions.h"

// rp_image backend registration.
#include "librpbase/img/RpGdiplusBackend.hpp"
#include "librpbase/img/rp_image.hpp"
using LibRpBase::RpGdiplusBackend;
using LibRpBase::rp_image;
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
#elif defined(HAVE__WPUTENV_S)
	_wputenv_s(L"LC_ALL", L"C");
#elif defined(HAVE__WPUTENV)
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
