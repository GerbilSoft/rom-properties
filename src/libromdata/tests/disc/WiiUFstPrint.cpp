/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * WiiUFstPrint.cpp: Wii U FST printer                                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: Using GcnFstPrint for (most) localization contexts.

#include "disc/WiiUFst.hpp"
#include "FstPrint.hpp"
using LibRpBase::IFst;
using LibRomData::WiiUFst;

// i18n
#include "libi18n/i18n.h"

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
		Gsvt::StdErr.fputs(fmt::format(FRUN(C_("WiiUFstPrint", "Syntax: {:s} fst.bin")), argv[0]));
		Gsvt::StdErr.newline();
		return EXIT_FAILURE;
	}

	// Open and read the FST file.
	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		// tr: {0:s} == filename, {1:s} == error message
		Gsvt::StdErr.fputs(fmt::format(FRUN(C_("GcnFstPrint", "Error opening '{0:s}': '{1:s}'")), argv[1], strerror(errno)));
		Gsvt::StdErr.newline();
		return EXIT_FAILURE;
	}

	// Make sure the FST is less than 16 MB.
	fseeko(f, 0, SEEK_END);
	const off64_t fileSize_o = ftello(f);
	if (fileSize_o > (16*1024*1024)) {
		Gsvt::StdErr.fputs(C_("GcnFstPrint", "ERROR: FST is too big. (Maximum of 16 MB.)"));
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
		// tr: {0:Ld} == number of bytes read, {1:Ld} == number of bytes expected to read
		Gsvt::StdErr.fputs(fmt::format(FRUN(C_("GcnFstPrint", "ERROR: Read {0:Ld} bytes, expected {1:Ld} bytes.")),
			rd_size, fileSize));
		Gsvt::StdErr.newline();
		return EXIT_FAILURE;
	}

	// Parse the FST.
	// TODO: Validate the FST and return an error if it doesn't
	// "look" like an FST?
	unique_ptr<IFst> fst(new WiiUFst(fstData.get(), static_cast<uint32_t>(fileSize)));
	if (!fst->isOpen()) {
		Gsvt::StdErr.fputs(fmt::format(FRUN(C_("WiiUFstPrint", "*** ERROR: Could not parse '{:s}' as WiiUFst.")), argv[1]));
		Gsvt::StdErr.newline();
		return EXIT_FAILURE;
	}

	// Print the FST to an ostringstream.
	ostringstream oss;
	LibRomData::fstPrint(fst.get(), oss, true);
	const string fst_str = oss.str();
	Gsvt::StdOut.fputs(oss.str());

	if (fst->hasErrors()) {
		Gsvt::StdErr.newline();
		Gsvt::StdErr.fputs(C_("WiiUFstPrint", "*** WARNING: FST has errors and may be unusable."));
		Gsvt::StdErr.newline();
	}

	// Cleanup.
	return 0;
}
