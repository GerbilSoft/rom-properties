/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * GcnFstPrint.cpp: GameCube/Wii FST printer.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "disc/GcnFst.hpp"
#include "FstPrint.hpp"
using LibRpBase::IFst;
using LibRomData::GcnFst;

// i18n
#include "libi18n/i18n.h"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <locale>
#include <memory>
#include <sstream>
#include <string>
using std::locale;
using std::ostream;
using std::ostringstream;
using std::string;
using std::unique_ptr;

// librpsecure
#include "librpsecure/os-secure.h"

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include <io.h>
#  include "librptext/wchar.hpp"
using std::u16string;
#endif /* _WIN32 */

int RP_C_API main(int argc, char *argv[])
{
	// Set OS-specific security options.
	// TODO: Non-Windows syscall stuff.
#ifdef _WIN32
	rp_secure_param_t param;
	param.bHighSec = FALSE;
	rp_secure_enable(param);
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
		offsetShift = static_cast<uint8_t>(ltmp);
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
	const off64_t fileSize_o = ftello(f);
	if (fileSize_o > (16*1024*1024)) {
		puts(C_("GcnFstPrint", "ERROR: FST is too big. (Maximum of 16 MB.)"));
		fclose(f);
		return EXIT_FAILURE;
	}
	fseeko(f, 0, SEEK_SET);

	// Read the FST into memory.
	const size_t fileSize = static_cast<size_t>(fileSize_o);
	unique_ptr<uint8_t[]> fstData(new uint8_t[fileSize]);
	size_t rd_size = fread(fstData.get(), 1, fileSize, f);
	fclose(f);
	if (rd_size != fileSize) {
		// tr: %1$u == number of bytes read, %2$u == number of bytes expected to read
		printf_p(C_("GcnFstPrint", "ERROR: Read %1$u bytes, expected %2$u bytes."),
			(unsigned int)rd_size, (unsigned int)fileSize);
		putchar('\n');
		return EXIT_FAILURE;
	}

	// Check for NKit FST recovery data.
	// These FSTs have an extra header at the top, indicating what
	// disc the FST belongs to.
	unsigned int fst_start_offset = 0;
	static const uint8_t root_dir_data[] = {1,0,0,0,0,0,0,0,0,0};
	if (fileSize >= 0x60) {
		if (!memcmp(&fstData[0x50], root_dir_data, sizeof(root_dir_data))) {
			// Found an NKit FST.
			fst_start_offset = 0x50;
		}
	}

	// Parse the FST.
	// TODO: Validate the FST and return an error if it doesn't
	// "look" like an FST?
	unique_ptr<IFst> fst(new GcnFst(&fstData[fst_start_offset],
		static_cast<uint32_t>(fileSize - fst_start_offset), offsetShift));
	if (!fst->isOpen()) {
		puts(C_("GcnFstPrint", "*** ERROR: Could not open a GcnFst."));
		return EXIT_FAILURE;
	}

	// Print the FST to an ostringstream.
	ostringstream oss;
	LibRomData::fstPrint(fst.get(), oss);
	string fst_str = oss.str();

#ifdef _WIN32
	// FIXME: isatty() might not work properly on Win8+ with MinGW.
	// Reference: https://lists.gnu.org/archive/html/bug-gnulib/2013-01/msg00007.html
	if (isatty(fileno(stdout))) {
		// Convert to wchar_t, then print it.
		_fputts(U82T_s(fst_str), stdout);
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
	return 0;
}
