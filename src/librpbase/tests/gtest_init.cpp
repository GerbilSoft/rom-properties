/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * gtest_init.c: Google Test initialization.                               *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

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

	// Set the C and C++ locales.
	locale::global(locale(""));

	// Call the actual main function.
	return gtest_main(argc, argv);
}
