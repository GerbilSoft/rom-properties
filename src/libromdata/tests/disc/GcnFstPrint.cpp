/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * GcnFstPrint.cpp: GameCube/Wii FST printer.                              *
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

#include "disc/GcnFst.hpp"
#include "FstPrint.hpp"
using LibRomData::GcnFst;

// i18n
#include "libi18n/i18n.h"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cstdio>
#include <cstring>

// C++ includes.
#include <locale>
#include <sstream>
#include <string>
using std::locale;
using std::ostream;
using std::ostringstream;
using std::string;

#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
# include "libwin32common/secoptions.h"
# include <io.h>
# include "librpbase/TextFuncs.hpp"
using std::u16string;
#endif /* _WIN32 */

int RP_C_API main(int argc, char *argv[])
{
#ifdef _WIN32
	// Set Win32 security options.
	secoptions_init();
#endif /* _WIN32 */

	// Set the C and C++ locales.
	locale::global(locale(""));

	// Initialize i18n.
	rp_i18n_init();

	if (argc < 2 || argc > 3) {
		printf(C_("GcnFstPrint", "Syntax: %s fst.bin [offsetShift]"), argv[0]);
		putchar('\n');
		puts(C_("GcnFstPrint", "offsetShift should be 0 for GameCube, 2 for Wii. (default is 0)"));
		return EXIT_FAILURE;
	}

	// Was an offsetShift specified?
	uint8_t offsetShift = 0;	// Default is GameCube.
	if (argc == 3) {
		char *endptr = nullptr;
		long ltmp = strtol(argv[2], &endptr, 10);
		if (*endptr != '\0' || (ltmp != 0 && ltmp != 2)) {
			printf(C_("GcnFstPrint", "Invalid offset shift '%s' specified."), argv[2]);
			putchar('\n');
			puts(C_("GcnFstPrint", "offsetShift should be 0 for GameCube, 2 for Wii. (default is 0)"));
			return EXIT_FAILURE;
		}
		offsetShift = (uint8_t)ltmp;
	}

	// Open and read the FST file.
	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		// tr: %1$s == filename, %2$s == error message
		printf_p(C_("GcnFstPrint", "Error opening '%1$s': '%2$s'"), argv[1], strerror(errno));
		putchar('\n');
		return EXIT_FAILURE;
	}

	// Make sure the FST is less than 16 MB.
	fseeko(f, 0, SEEK_END);
	int64_t filesize = ftello(f);
	if (filesize > (16*1024*1024)) {
		puts(C_("GcnFstPrint", "ERROR: FST is too big. (Maximum of 16 MB.)"));
		fclose(f);
		return EXIT_FAILURE;
	}
	fseek(f, 0, SEEK_SET);

	// Read the FST into memory.
	uint8_t *fstData = static_cast<uint8_t*>(malloc(filesize));
	if (!fstData) {
		printf(C_("GcnFstPrint", "ERROR: malloc(%u) failed."), (unsigned int)filesize);
		putchar('\n');
		fclose(f);
		return EXIT_FAILURE;
	}
	size_t rd_size = fread(fstData, 1, filesize, f);
	fclose(f);
	if (rd_size != (size_t)filesize) {
		// tr: %1$u == number of bytes read, %2$u == number of bytes expected to read
		printf_p(C_("GcnFstPrint", "ERROR: Read %1$u bytes, expected %2$u bytes."),
			(unsigned int)rd_size, (unsigned int)filesize);
		putchar('\n');
		free(fstData);
		return EXIT_FAILURE;
	}

	// Parse the FST.
	// TODO: Validate the FST and return an error if it doesn't
	// "look" like an FST?
	GcnFst *fst = new GcnFst(fstData, (uint32_t)filesize, offsetShift);
	if (!fst->isOpen()) {
		puts(C_("GcnFstPrint", "*** ERROR: Could not open a GcnFst."));
		free(fstData);
		return EXIT_FAILURE;
	}

	// Print the FST to an ostringstream.
	ostringstream oss;
	LibRomData::fstPrint(fst, oss);
	string fst_str = oss.str();

#ifdef _WIN32
	// FIXME: isatty() might not work properly on Win8+ with MinGW.
	// Reference: https://lists.gnu.org/archive/html/bug-gnulib/2013-01/msg00007.html
	if (isatty(fileno(stdout))) {
		// Convert to wchar_t, then print it.
		fputws(U82W_s(fst_str), stdout);
	} else {
		// Writing to file. Print the original UTF-8.
		fputs(fst_str.c_str(), stdout);
	}
#else /* !_WIN32 */
	// Print the FST.
	fputs(fst_str.c_str(), stdout);
#endif

	if (fst->hasErrors()) {
		putchar('\n');
		puts(C_("GcnFstPrint", "*** WARNING: FST has errors and may be unusable."));
	}

	// Cleanup.
	delete fst;
	free(fstData);
	return 0;
}
