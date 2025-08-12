/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * GcnFstPrint.cpp: GameCube/Wii FST printer.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "disc/GcnFst.hpp"
#include "FstPrint.hpp"
using LibRpBase::IFst;
using LibRomData::GcnFst;

// i18n
#include "libi18n/i18n.hpp"

// libgsvt for VT handling
#include "gsvtpp.hpp"

// C includes (C++ namespace)
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// C++ includes
#include <array>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
using std::array;
using std::locale;
using std::ostream;
using std::ostringstream;
using std::string;
using std::unique_ptr;

// libfmt
#include "rp-libfmt.h"

// librpsecure
#include "librpsecure/os-secure.h"

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include <io.h>
#  include "librptext/wchar.hpp"
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
#if defined(_WIN32) && defined(__MINGW32__)
	// FIXME: MinGW-w64 12.0.0 doesn't like setting the C++ locale to "".
	setlocale(LC_ALL, "");
#else /* !_WIN32 || !__MINGW32__ */
	locale::global(locale(""));
#endif /* _WIN32 && __MINGW32__ */
#ifdef _WIN32
	// NOTE: Revert LC_CTYPE to "C" to fix UTF-8 output.
	// (Needed for MSVC 2022; does nothing for MinGW-w64 11.0.0)
	setlocale(LC_CTYPE, "C");
#endif /* _WIN32 */

	// Detect console information.
	// NOTE: Technically not needed, since Gsvt::Console access
	// will call this for us...
	gsvt_init();

	// Initialize i18n.
	rp_i18n_init();

	if (argc < 2 || argc > 3) {
		Gsvt::StdErr.fputs(fmt::format(FRUN(C_("GcnFstPrint", "Syntax: {:s} fst.bin [offsetShift]")), argv[0]));
		Gsvt::StdErr.newline();
		Gsvt::StdErr.fputs(C_("GcnFstPrint", "offsetShift should be 0 for GameCube, 2 for Wii. (default is 0)"));
		Gsvt::StdErr.newline();
		return EXIT_FAILURE;
	}

	// Was an offsetShift specified?
	uint8_t offsetShift = 0;	// Default is GameCube.
	if (argc == 3) {
		char *endptr = nullptr;
		long ltmp = strtol(argv[2], &endptr, 10);
		if (*endptr != '\0' || (ltmp != 0 && ltmp != 2)) {
			Gsvt::StdErr.textColorSet8(1, true);	// red
			Gsvt::StdErr.fputs(fmt::format(FRUN(C_("GcnFstPrint", "Invalid offset shift '{:s}' specified.")), argv[2]));
			Gsvt::StdErr.textColorReset();
			Gsvt::StdErr.newline();
			Gsvt::StdErr.fputs(C_("GcnFstPrint", "offsetShift should be 0 for GameCube, 2 for Wii. (default is 0)"));
			Gsvt::StdErr.newline();
			return EXIT_FAILURE;
		}
		offsetShift = static_cast<uint8_t>(ltmp);
	}

	// Open and read the FST file.
	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		Gsvt::StdErr.textColorSet8(1, true);	// red
		// tr: {0:s} == filename, {1:s} == error message
		Gsvt::StdErr.fputs(fmt::format(FRUN(C_("GcnFstPrint", "Error opening '{0:s}': '{1:s}'")), argv[1], strerror(errno)));
		Gsvt::StdErr.textColorReset();
		Gsvt::StdErr.newline();
		return EXIT_FAILURE;
	}

	// Make sure the FST is less than 16 MB.
	fseeko(f, 0, SEEK_END);
	const off64_t fileSize_o = ftello(f);
	if (fileSize_o > (16*1024*1024)) {
		Gsvt::StdErr.textColorSet8(1, true);	// red
		Gsvt::StdErr.fputs(C_("GcnFstPrint", "ERROR: FST is too big. (Maximum of 16 MB.)"));
		Gsvt::StdErr.textColorReset();
		Gsvt::StdErr.newline();
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
		Gsvt::StdErr.textColorSet8(1, true);	// red
		// tr: {0:d} == number of bytes read, {1:d} == number of bytes expected to read
		Gsvt::StdErr.fputs(fmt::format(FRUN(C_("GcnFstPrint", "ERROR: Read {0:Ld} bytes, expected {1:Ld} bytes.")),
			rd_size, fileSize));
		Gsvt::StdErr.textColorReset();
		Gsvt::StdErr.newline();
		return EXIT_FAILURE;
	}

	// Check for NKit FST recovery data.
	// These FSTs have an extra header at the top, indicating what
	// disc the FST belongs to.
	unsigned int fst_start_offset = 0;
	static constexpr array<uint8_t, 10> root_dir_data = {{1,0,0,0,0,0,0,0,0,0}};
	if (fileSize >= 0x60) {
		if (!memcmp(&fstData[0x50], root_dir_data.data(), root_dir_data.size())) {
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
		Gsvt::StdErr.textColorSet8(1, true);	// red
		Gsvt::StdErr.fputs(fmt::format(FRUN(C_("GcnFstPrint", "*** ERROR: Could not parse '{:s}' as GcnFst.")), argv[1]));
		Gsvt::StdErr.textColorReset();
		Gsvt::StdErr.newline();
		return EXIT_FAILURE;
	}

	// Print the FST to an ostringstream.
	ostringstream oss;
	LibRomData::fstPrint(fst.get(), oss);
	Gsvt::StdOut.fputs(oss.str());

	if (fst->hasErrors()) {
		Gsvt::StdErr.newline();
		Gsvt::StdErr.textColorSet8(3, true);	// yellow
		Gsvt::StdErr.fputs(C_("GcnFstPrint", "*** WARNING: FST has errors and may be unusable."));
		Gsvt::StdErr.textColorReset();
		Gsvt::StdErr.newline();
	}

	// Cleanup.
	return 0;
}
