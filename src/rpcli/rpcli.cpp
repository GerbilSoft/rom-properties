/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * rpcli.cpp: Command-line interface for properties.                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.rpcli.h"
#include "config.version.h"
#include "librpbase/config.librpbase.h"
#include "libromdata/config.libromdata.h"

// OS-specific security options.
#include "rpcli_secure.h"

// VT handling
#include "vt.hpp"

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

// librpbase
#include "libi18n/i18n.h"
#include "librpbase/RomData.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbase/img/RpPng.hpp"
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;

// librpfile
#include "librpfile/config.librpfile.h"
#include "librpfile/FileSystem.hpp"
#include "librpfile/RpFile.hpp"
using namespace LibRpFile;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using namespace LibRomData;

// librptexture
#include "librptexture/fileformat/FileFormat.hpp"
#include "librptexture/img/rp_image.hpp"
#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include "libwin32common/rp_versionhelpers.h"
#  include "librptexture/img/GdiplusHelper.hpp"
#endif /* _WIN32 */
using namespace LibRpTexture;

// librptext
#include "librptext/conversion.hpp"
using LibRpText::utf8_to_utf16;

#ifdef ENABLE_SIXEL
// Sixel
#include "rp_sixel.hpp"
#endif /* ENABLE_SIXEL */

#ifdef ENABLE_DECRYPTION
#  include "verifykeys.hpp"
#endif /* ENABLE_DECRYPTION */
#include "device.hpp"

// OS-specific userdirs
#ifdef _WIN32
#  include "libwin32common/userdirs.hpp"
#  define OS_NAMESPACE LibWin32Common
#  ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#    define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#  endif /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
#else
#  include "libunixcommon/userdirs.hpp"
#  define OS_NAMESPACE LibUnixCommon
#endif
#include "tcharx.h"

// C++ STL classes
#include <sstream>
using std::array;
using std::cout;
using std::cerr;
using std::locale;
using std::ofstream;
using std::ostringstream;
using std::shared_ptr;
using std::string;
using std::u16string;
using std::unique_ptr;
using std::vector;

#include "libi18n/config.libi18n.h"
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#include "libwin32common/DelayLoadHelper.h"

// DelayLoad: libromdata
// NOTE: Not using DELAYLOAD_TEST_FUNCTION_IMPL1 here due to the use of C++ functions.
#include "libromdata/config/ImageTypesConfig.hpp"
DELAYLOAD_FILTER_FUNCTION_IMPL(ImageTypesConfig_className);
static int DelayLoad_test_ImageTypesConfig_className(void) {
	static bool success = 0;
	if (!success) {
		__try {
			(void)ImageTypesConfig::className(0);
		} __except (DelayLoad_filter_ImageTypesConfig_className(GetExceptionCode())) {
			return -ENOTSUP;
		}
		success = 1;
	}
	return 0;
}

#  ifdef ENABLE_NLS
// DelayLoad: libi18n
#    include "libi18n/i18n.h"
DELAYLOAD_TEST_FUNCTION_IMPL1(libintl_textdomain, nullptr);
#  endif /* ENABLE_NLS */
#endif /* _MSC_VER */

// Mini-T2U8()
#ifdef _WIN32
#  include "librptext/wchar.hpp"
#  define T2U8c(tcs) (T2U8(tcs).c_str())
#else /* !_WIN32 */
#  define T2U8c(tcs) (tcs)
#endif /* _WIN32 */

#ifdef _WIN32
// Console code page restoration
// For UTF-8 console output on Windows 10.
static UINT old_console_output_cp = 0;
static void RestoreConsoleOutputCP(void)
{
	if (old_console_output_cp != 0) {
		SetConsoleOutputCP(old_console_output_cp);
	}
}
#endif /* _WIN32 */

/**
 * Print text to the console.
 *
 * On Windows, if a real console is in use, use WriteConsole().
 *
 * On other systems, or if we're not using a real console on Windows,
 * use regular stdio functions.
 *
 * @param ci ConsoleInfo_t
 * @param str String
 * @param newline If true, print a newline afterwards.
 */
static void ConsolePrint(ConsoleInfo_t *ci, const char *str, bool newline = false)
{
#ifdef _WIN32
	// Windows: If printing to console and UTF-8 is not enabled,
	// convert to UTF-16 and use WriteConsoleW().
	if (ci->is_console && ci->is_real_console) {
		fflush(ci->stream);
		// TODO: win32_write_to_console(): Take a handle.
		int ret = win32_write_to_console(ci, str);
		if (ret != 0) {
			// Failed to write to console.
			// Use stdio as a fallback.
			fputs(str, ci->stream);
		}
	} else
#endif /* _WIN32 */
	{
		// Regular stdio output.
		fputs(str, ci->stream);
	}

	if (newline) {
		// TODO: Use WriteConsole() for this if using a Win32 console?
		fputc('\n', ci->stream);
	}
}

struct ExtractParam {
	const TCHAR *filename;	// Target filename. Can be null due to argv[argc]
	int imageType;		// Image Type. -1 = iconAnimData, MUST be between -1 and IMG_INT_MAX
	int mipmapLevel;	// Mipmap level. (IMG_INT_IMAGE only) -1 = use default; 0 or higher = mipmap level

	ExtractParam(const TCHAR *filename, int imageType, int mipmapLevel = -1)
		: filename(filename)
		, imageType(imageType)
		, mipmapLevel(mipmapLevel)
	{}
};

/**
 * Extracts images from romdata
 * @param romData RomData containing the images
 * @param extract Vector of image extraction parameters
 */
static void ExtractImages(const RomData *romData, const vector<ExtractParam> &extract)
{
	const uint32_t supported = romData->supportedImageTypes();
	for (const ExtractParam &p : extract) {
		if (!p.filename) continue;
		bool found = false;
		
		if (p.imageType >= 0 && (supported & (1U << p.imageType))) {
			// normal image
			bool isMipmap = (unlikely(p.mipmapLevel >= 0));
			const RomData::ImageType imageType =
				static_cast<RomData::ImageType>(p.imageType);
			rp_image_const_ptr image;

			if (likely(!isMipmap)) {
				// normal image
				image = romData->image(imageType);
			} else {
				// mipmap level for IMG_INT_IMAGE
				image = romData->mipmap(p.mipmapLevel);
			}

			if (image && image->isValid()) {
				found = true;
				if (likely(!isMipmap)) {
					cerr << "-- " <<
						// tr: {0:s} == image type name, {1:s} == output filename
						fmt::format(FRUN(C_("rpcli", "Extracting {0:s} into '{1:s}'")),
							RomData::getImageTypeName(imageType),
							T2U8c(p.filename)) << '\n';
				} else {
					cerr << "-- " <<
						// tr: {0:d} == mipmap level, {1:s} == output filename
						fmt::format(FRUN(C_("rpcli", "Extracting mipmap level {0:d} into '{1:s}'")),
							p.mipmapLevel, T2U8c(p.filename)) << '\n';
				}
				cerr.flush();
				int errcode = RpPng::save(p.filename, image);
				if (errcode != 0) {
					// tr: {0:s} == filename, {1:s} == error message
					cerr << fmt::format(FRUN(C_("rpcli", "Couldn't create file '{0:s}': {1:s}")),
						T2U8c(p.filename), strerror(-errcode)) << '\n';
				} else {
					cerr << "   " << C_("rpcli", "Done") << '\n';
				}
				cerr.flush();
			}
		} else if (p.imageType == -1) {
			// iconAnimData image
			auto iconAnimData = romData->iconAnimData();
			if (iconAnimData && iconAnimData->count != 0 && iconAnimData->seq_count != 0) {
				found = true;
				cerr << "-- " << fmt::format(FRUN(C_("rpcli", "Extracting animated icon into '{:s}'")), T2U8c(p.filename)) << '\n';
				cerr.flush();
				int errcode = RpPng::save(p.filename, iconAnimData);
				if (errcode == -ENOTSUP) {
					cerr << "   " << C_("rpcli", "APNG not supported, extracting only the first frame") << '\n';
					cerr.flush();
					// falling back to outputting the first frame
					errcode = RpPng::save(p.filename, iconAnimData->frames[iconAnimData->seq_index[0]]);
				}
				if (errcode != 0) {
					cerr << "   " <<
						fmt::format(FRUN(C_("rpcli", "Couldn't create file '{0:s}': {1:s}")),
							T2U8c(p.filename), strerror(-errcode)) << '\n';
				} else {
					cerr << "   " << C_("rpcli", "Done") << '\n';
				}
				cerr.flush();
			}
		}

		if (!found) {
			// TODO: Return an error code?
			if (p.imageType == -1) {
				cerr << "-- " << C_("rpcli", "Animated icon not found") << '\n';
			} else if (p.mipmapLevel >= 0) {
				cerr << "-- " <<
					fmt::format(FRUN(C_("rpcli", "Mipmap level {:d} not found")), p.mipmapLevel) << '\n';
			} else {
				const RomData::ImageType imageType =
					static_cast<RomData::ImageType>(p.imageType);
				cerr << "-- " <<
					fmt::format(FRUN(C_("rpcli", "Image '{:s}' not found")),
						RomData::getImageTypeName(imageType)) << '\n';
			}
			cerr.flush();
		}
	}
}

/**
 * Shows info about file
 * @param filename ROM filename
 * @param json Is program running in json mode?
 * @param extract Vector of image extraction parameters
 * @param lc Language code (0 for default)
 * @param flags ROMOutput flags (see OutputFlags)
 */
static void DoFile(const TCHAR *filename, bool json, const vector<ExtractParam> &extract,
	uint32_t lc = 0, unsigned int flags = 0)
{
	RomDataPtr romData;

	if (likely(!FileSystem::is_directory(filename))) {
		// File: Open the file and call RomDataFactory::create() with the opened file.

		// FIXME: Make T2U8c() unnecessary here.
		fputs("== ", stderr);
		fmt::print(stderr, FRUN(C_("rpcli", "Reading file '{:s}'...")), T2U8c(filename));
		fputc('\n', stderr);
		fflush(stderr);

		shared_ptr<RpFile> file = std::make_shared<RpFile>(filename, RpFile::FM_OPEN_READ_GZ);
		if (!file->isOpen()) {
			// TODO: Return an error code?
			fputs("-- ", stderr);
			fmt::print(stderr, FRUN(C_("rpcli", "Couldn't open file: {:s}")), strerror(file->lastError()));
			fputc('\n', stderr);
			fflush(stderr);
			if (json) {
				fmt::print(FSTR("{{\"error\":\"couldn't open file\",\"code\":{:d}}}\n"), file->lastError());
				fflush(stdout);
			}
			return;
		}

		romData = RomDataFactory::create(file);
	} else {
		// Directory: Call RomDataFactory::create() with the filename.

		// FIXME: Make T2U8c() unnecessary here.
		fputs("== ", stderr);
		fmt::print(stderr, FRUN(C_("rpcli", "Reading directory '{:s}'...")), T2U8c(filename));
		fputc('\n', stderr);
		fflush(stderr);

		romData = RomDataFactory::create(filename);
	}

	if (romData) {
		if (json) {
			fputs("-- ", stderr);
			fputs(C_("rpcli", "Outputting JSON data"), stderr);
			fputc('\n', stderr);
			fflush(stderr);

#ifdef _WIN32
			if (ci_stdout.is_console && ci_stdout.is_real_console) {
				// Windows: Using a real console on stdout.
				// Convert to UTF-16 and use WriteConsoleW().
				// NOTE: This is seemingly faster than even using UTF-8
				// output on Windows 10 1607.
				ostringstream oss;
				oss << JSONROMOutput(romData.get(), flags) << '\n';
				cout.flush();
				const string str = oss.str();
				// TODO: Error checking.
				win32_write_to_console(&ci_stdout, str.data(), static_cast<int>(str.size()));
			} else
#endif /* _WIN32 */
			{
				// Windows: Using stdout console with UTF-8 support,
				// or redirected to a file, or not using Windows.
				cout << JSONROMOutput(romData.get(), flags) << '\n';
			}
		} else {
#ifdef _WIN32
			if (ci_stdout.is_console && ci_stdout.is_real_console && !ci_stdout.supports_ansi) {
				// Windows: Using stdout console, but it doesn't support ANSI escapes.
				// NOTE: Console may support UTF-8, but since it doesn't support
				// ANSI escapes, we're better off using WriteConsoleW() anyway.
				// Support for ANSI escape sequences was added in Windows 10 1607.
				ostringstream oss;
				oss << ROMOutput(romData.get(), lc, flags) << '\n';
				cout.flush();
				// TODO: Error checking.
				win32_console_print_ansi_color(oss.str().c_str());
			} else
#endif /* _WIN32 */
			{
#ifdef ENABLE_SIXEL
				// If this is a tty, print the icon/banner using libsixel.
				if (ci_stdout.is_console) {
					print_sixel_icon_banner(romData);
				}
#endif /* ENABLE_SIXEL */
				// Windows: Using stdout console with UTF-8 and ANSI escape support,
				// or redirected to a file, or not using Windows.
				cout << ROMOutput(romData.get(), lc, flags) << '\n';
			}
		}
		cout.flush();
		ExtractImages(romData.get(), extract);
	} else {
		fputs("-- ", stderr);
		fputs(C_("rpcli", "ROM is not supported"), stderr);
		fputc('\n', stderr);
		fflush(stderr);

		if (json) {
			fputs("{\"error\":\"rom is not supported\"}\n", stdout);
			fflush(stdout);
		}
	}
}

/**
 * Print the system region information.
 */
static void PrintSystemRegion(void)
{
	string buf;
	buf.reserve(8);

	uint32_t lc = __swab32(SystemRegion::getLanguageCode());
	if (lc != 0) {
		for (unsigned int i = 4; i > 0; i--, lc >>= 8) {
			if ((lc & 0xFF) == 0)
				continue;
			buf += static_cast<char>(lc & 0xFF);
		}
	}
	fmt::print(FRUN(C_("rpcli", "System language code: {:s}")),
		(!buf.empty() ? buf.c_str() : C_("rpcli", "0 (this is a bug!)")));
	putchar('\n');

	uint32_t cc = __swab32(SystemRegion::getCountryCode());
	buf.clear();
	if (cc != 0) {
		for (unsigned int i = 4; i > 0; i--, cc >>= 8) {
			if ((cc & 0xFF) == 0)
				continue;
			buf += static_cast<char>(cc & 0xFF);
		}
	}
	fmt::print(FRUN(C_("rpcli", "System country code: {:s}")),
		(!buf.empty() ? buf.c_str() : C_("rpcli", "0 (this is a bug!)")));
	putchar('\n');

	// Extra line. (TODO: Only if multiple commands are specified.)
	putchar('\n');
	fflush(stdout);
}

/**
 * Print pathname information.
 */
static void PrintPathnames(void)
{
	// TODO: Localize these strings?
	fmt::print(FSTR(
		"User's home directory:   {:s}\n"
		"User's cache directory:  {:s}\n"
		"User's config directory: {:s}\n"
		"\n"
		"RP cache directory:      {:s}\n"
		"RP config directory:     {:s}\n"),
		OS_NAMESPACE::getHomeDirectory(),
		OS_NAMESPACE::getCacheDirectory(),
		OS_NAMESPACE::getConfigDirectory(),
		FileSystem::getCacheDirectory(),
		FileSystem::getConfigDirectory());

	// Extra line. (TODO: Only if multiple commands are specified.)
	putchar('\n');
	fflush(stdout);
}

#ifdef RP_OS_SCSI_SUPPORTED
/**
 * Run a SCSI INQUIRY command on a device.
 * @param filename Device filename
 * @param json Is program running in json mode?
 */
static void DoScsiInquiry(const TCHAR *filename, bool json)
{
	// FIXME: Make T2U8c() unnecessary here.
	fputs("== ", stderr);
	fmt::print(stderr, FRUN(C_("rpcli", "Opening device file '{:s}'...")), T2U8c(filename));
	fputc('\n', stderr);
	fflush(stderr);

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ_GZ));
	if (!file->isOpen()) {
		// TODO: Return an error code?
		fputs("-- ", stderr);
		fmt::print(stderr, FRUN(C_("rpcli", "Couldn't open file: {:s}")), strerror(file->lastError()));
		fputc('\n', stderr);
		fflush(stderr);

		if (json) {
			fmt::print("{{\"error\":\"couldn't open file\",\"code\":{:d}}}\n", file->lastError());
			fflush(stdout);
		}
		return;
	}

	// TODO: Check for unsupported devices? (Only CD-ROM is supported.)
	if (!file->isDevice()) {
		// TODO: Return an error code?
		fputs("-- ", stderr);
		fputs(C_("rpcli", "Not a device file"), stderr);
		fputc('\n', stderr);
		fflush(stderr);

		if (json) {
			fputs("{\"error\":\"not a device file\"}\n", stdout);
			fflush(stdout);
		}
		return;
	}

	if (json) {
		fputs("-- ", stderr);
		fputs(C_("rpcli", "Outputting JSON data"), stderr);
		fputc('\n', stderr);
		fflush(stderr);

		// TODO: JSONScsiInquiry
		//cout << JSONScsiInquiry(file.get()) << '\n';
		//cout.flush();
	} else {
#ifdef _WIN32
		if (ci_stdout.is_console && ci_stdout.is_real_console) {
			// Windows: Using a real console on stdout.
			// Convert to UTF-16 and use WriteConsoleW().
			// NOTE: This is seemingly faster than even using UTF-8
			// output on Windows 10 1607.
			ostringstream oss;
			oss << ScsiInquiry(file.get()) << '\n';
			cout.flush();
			const string str = oss.str();
			// TODO: Error checking.
			win32_write_to_console(&ci_stdout, str.data(), static_cast<int>(str.size()));
		} else
#endif /* _WIN32 */
		{
			cout << ScsiInquiry(file.get()) << '\n';
			cout.flush();
		}
	}
}

/**
 * Run an ATA IDENTIFY DEVICE command on a device.
 * @param filename Device filename
 * @param json Is program running in json mode?
 * @param packet If true, use ATA IDENTIFY PACKET.
 */
static void DoAtaIdentifyDevice(const TCHAR *filename, bool json, bool packet)
{
	// FIXME: Make T2U8c() unnecessary here.
	fputs("== ", stderr);
	fmt::print(stderr, FRUN(C_("rpcli", "Opening device file '{:s}'...")), T2U8c(filename));
	fputc('\n', stderr);
	fflush(stderr);

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ_GZ));
	if (!file->isOpen()) {
		// TODO: Return an error code?
		fputs("-- ", stderr);
		fmt::print(stderr, FRUN(C_("rpcli", "Couldn't open file: {:s}")), strerror(file->lastError()));
		fputc('\n', stderr);
		fflush(stderr);

		if (json) {
			fmt::print("{{\"error\":\"couldn't open file\",\"code\":{:d}}}\n", file->lastError());
			fflush(stdout);
		}
		return;
	}

	// TODO: Check for unsupported devices? (Only CD-ROM is supported.)
	if (!file->isDevice()) {
		// TODO: Return an error code?
		fputs("-- ", stderr);
		fputs(C_("rpcli", "Not a device file"), stderr);
		fputc('\n', stderr);
		fflush(stderr);

		if (json) {
			fputs("{\"error\":\"Not a device file\"}\n", stdout);
			fflush(stdout);
		}
		return;
	}

	if (json) {
		fputs("-- ", stderr);
		fputs(C_("rpcli", "Outputting JSON data"), stderr);
		fputc('\n', stderr);
		fflush(stderr);

		// TODO: JSONAtaIdentifyDevice
		//cout << JSONAtaIdentifyDevice(file.get(), packet) << '\n';
		//cout.flush();
	} else {
#ifdef _WIN32
		if (ci_stdout.is_console && ci_stdout.is_real_console) {
			// Windows: Using a real console on stdout.
			// Convert to UTF-16 and use WriteConsoleW().
			// NOTE: This is seemingly faster than even using UTF-8
			// output on Windows 10 1607.
			ostringstream oss;
			oss << AtaIdentifyDevice(file.get()) << '\n';
			cout.flush();
			const string str = oss.str();
			// TODO: Error checking.
			win32_write_to_console(&ci_stdout, str.data(), static_cast<int>(str.size()));
		} else
#endif /* _WIN32 */
		{
			cout << AtaIdentifyDevice(file.get(), packet) << '\n';
			cout.flush();
		}
	}
}
#endif /* RP_OS_SCSI_SUPPORTED */

static void ShowUsage(void)
{
	// TODO: Use argv[0] instead of hard-coding 'rpcli'?
#ifdef ENABLE_DECRYPTION	
	const char *const s_usage = C_("rpcli", "Usage: rpcli [-k] [-c] [-p] [-j] [-l lang] [[-xN outfile]... [-mN outfile]... [-a apngoutfile] filename]...");
	fputc('\n', stderr);
#else /* !ENABLE_DECRYPTION */
	const char *const s_usage = C_("rpcli", "Usage: rpcli [-c] [-p] [-j] [-l lang] [[-xN outfile]... [-mN outfile]... [-a apngoutfile] filename]...");
#endif /* ENABLE_DECRYPTION */
	ConsolePrint(&ci_stderr, s_usage, true);

	struct cmd_t {
		char opt[8];	// TODO: Automatic padding?
		const char *desc;
	};

	// Normal commands
#ifdef ENABLE_DECRYPTION
	static const array<cmd_t, 9> cmds = {{
		{"  -k:  ", NOP_C_("rpcli", "Verify encryption keys in keys.conf.")},
#else /* !ENABLE_DECRYPTION */
	static const array<cmd_t, 8> cmds = {{
#endif /* ENABLE_DECRYPTION */
		{"  -c:  ", NOP_C_("rpcli", "Print system region information.")},
		{"  -p:  ", NOP_C_("rpcli", "Print system path information.")},
		{"  -d:  ", NOP_C_("rpcli", "Skip ListData fields with more than 10 items. [text only]")},
		{"  -j:  ", NOP_C_("rpcli", "Use JSON output format.")},
		{"  -l:  ", NOP_C_("rpcli", "Retrieve the specified language from the ROM image.")},
		{"  -xN: ", NOP_C_("rpcli", "Extract image N to outfile in PNG format.")},
		{"  -mN: ", NOP_C_("rpcli", "Extract mipmap level N to outfile in PNG format.")},
		{"  -a:  ", NOP_C_("rpcli", "Extract the animated icon to outfile in APNG format.")},
	}};

	for (const auto &p : cmds) {
		ConsolePrint(&ci_stderr, p.opt);
		ConsolePrint(&ci_stderr, pgettext_expr("rpcli", p.desc), true);
	}
	fputc('\n', stderr);

#ifdef RP_OS_SCSI_SUPPORTED
	// Commands for devices
	static const array<cmd_t, 3> cmds_dev = {{
		{"  -is: ", NOP_C_("rpcli", "Run a SCSI INQUIRY command.")},
		{"  -ia: ", NOP_C_("rpcli", "Run an ATA IDENTIFY DEVICE command.")},
		{"  -ip: ", NOP_C_("rpcli", "Run an ATA IDENTIFY PACKET DEVICE command.")},
	}};

	ConsolePrint(&ci_stderr, C_("rpcli", "Special options for devices:"), true);
	for (const auto &p : cmds_dev) {
		ConsolePrint(&ci_stderr, p.opt);
		ConsolePrint(&ci_stderr, pgettext_expr("rpcli", p.desc), true);
	}
	fputc('\n', stderr);
#endif /* RP_OS_SCSI_SUPPORTED */

	ConsolePrint(&ci_stderr, C_("rpcli", "Examples:"), true);
	ConsolePrint(&ci_stderr, "* rpcli s3.gen\n");
	ConsolePrint(&ci_stderr, "\t ");
		ConsolePrint(&ci_stderr, C_("rpcli", "displays info about s3.gen"), true);
	ConsolePrint(&ci_stderr, "* rpcli -x0 icon.png pokeb2.nds\n");
	ConsolePrint(&ci_stderr, "\t ");
		ConsolePrint(&ci_stderr, C_("rpcli", "extracts icon from pokeb2.nds"), true);
	fflush(stderr);
}

int RP_C_API _tmain(int argc, TCHAR *argv[])
{
	// Enable security options.
	rpcli_do_security_options();

#ifdef __GLIBC__
	// Reduce /etc/localtime stat() calls.
	// References:
	// - https://lwn.net/Articles/944499/
	// - https://gitlab.com/procps-ng/procps/-/merge_requests/119
	setenv("TZ", ":/etc/localtime", 0);
#endif /* __GLIBC__ */

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

#ifdef RP_LIBROMDATA_IS_DLL
#  ifdef _MSC_VER
#    define ROMDATA_PREFIX
#  else
#    define ROMDATA_PREFIX _T("lib")
#  endif
#  ifndef NDEBUG
#    define ROMDATA_SUFFIX _T("-") _T(LIBROMDATA_SOVERSION_STR) _T("d")
#  else
#    define ROMDATA_SUFFIX _T("-") _T(LIBROMDATA_SOVERSION_STR)
#  endif
#  ifdef _WIN32
#    define ROMDATA_EXT _T(".dll")
#  else
// TODO: macOS
#    define ROMDATA_EXT _T(".so")
#  endif
#  define ROMDATA_DLL ROMDATA_PREFIX _T("romdata") ROMDATA_SUFFIX ROMDATA_EXT

#  ifdef _MSC_VER
	// Delay load verification: romdata-X.dll
	// TODO: Skip this if not linked with /DELAYLOAD?
	if (DelayLoad_test_ImageTypesConfig_className() != 0) {
		// Delay load failed.
		_fputts(_T("*** ERROR: ") ROMDATA_DLL _T(" could not be loaded.\n\n")
			_T("Please redownload rom-properties ") _T(RP_VERSION_STRING) _T(" and copy the\n")
			ROMDATA_DLL _T(" file to the installation directory.\n"),
			stderr);
		return EXIT_FAILURE;
	}
#  endif /* _MSC_VER */
#endif /* RP_LIBROMDATA_IS_DLL */

#if defined(ENABLE_NLS) && defined(_MSC_VER)
	// Delay load verification: libgnuintl
	// TODO: Skip this if not linked with /DELAYLOAD?
	if (DelayLoad_test_libintl_textdomain() != 0) {
		// Delay load failed.
		_fputts(_T("*** ERROR: ") LIBGNUINTL_DLL _T(" could not be loaded.\n\n")
			_T("This build of rom-properties has localization enabled,\n")
			_T("which requires the use of GNU gettext.\n\n")
			_T("Please redownload rom-properties ") _T(RP_VERSION_STRING) _T(" and copy the\n")
			LIBGNUINTL_DLL _T(" file to the installation directory.\n"),
			stderr);
		return EXIT_FAILURE;
	}
#endif /* ENABLE_NLS && _MSC_VER */

#ifdef _WIN32
	// Enable UTF-8 console output on Windows 10.
	// For older Windows, which doesn't support ANSI escape sequences,
	// WriteConsoleW() will be used for Unicode output instead.
	// TODO: Require Windows 10 1607 or later for ANSI escape sequences?
	if (IsWindows10OrGreater()) {
		old_console_output_cp = GetConsoleOutputCP();
		atexit(RestoreConsoleOutputCP);
		SetConsoleOutputCP(CP_UTF8);
	}
#endif /* _WIN32 */

	// Detect console information.
	init_vt();

	// Initialize i18n.
	rp_i18n_init();

	if(argc < 2){
		ShowUsage();

		// Since we didn't do anything, return a failure code.
		return EXIT_FAILURE;
	}
	
	static_assert(RomData::IMG_INT_MIN == 0, "RomData::IMG_INT_MIN must be 0!");

	unsigned int flags = 0;	// OutputFlags
	// DoFile parameters
	bool json = false;
	vector<ExtractParam> extract;

	// TODO: Add a command line option to override color output.
	// NOTE: Only checking ci_stdout here, since actual data is printed on stdout.
	if (ci_stdout.is_console) {
		flags |= OF_Text_UseAnsiColor;
	}

	for (int i = 1; i < argc; i++) { // figure out the json mode in advance
		if (argv[i][0] == _T('-')) {
			if (argv[i][1] == _T('j')) {
				json = true;
			} else if (argv[i][1] == _T('J')) {
				json = true;
				flags |= OF_JSON_NoPrettyPrint;
			}
		}
	}
	if (json) {
		cout << "[\n";
		cout.flush();
	}

#ifdef _WIN32
	// Initialize GDI+.
	const ULONG_PTR gdipToken = GdiplusHelper::InitGDIPlus();
	assert(gdipToken != 0);
	if (gdipToken == 0) {
		fputs("*** ERROR: GDI+ initialization failed.", stderr);
		fputc('\n', stderr);
		fflush(stderr);
		return -EIO;
	}
#endif /* _WIN32 */

#ifdef RP_OS_SCSI_SUPPORTED
	bool inq_scsi = false;
	bool inq_ata = false;
	bool inq_ata_packet = false;
#endif /* RP_OS_SCSI_SUPPORTED */
	uint32_t lc = 0;
	bool first = true;
	int ret = 0;
	for (int i = 1; i < argc; i++){
		if (argv[i][0] == _T('-')){
			switch (argv[i][1]) {
#ifdef ENABLE_DECRYPTION
			case _T('k'): {
				// Verify encryption keys.
				static bool hasVerifiedKeys = false;
				if (!hasVerifiedKeys) {
					hasVerifiedKeys = true;
					ret = VerifyKeys();
				}
				break;
			}
#endif /* ENABLE_DECRYPTION */

			case _T('c'):
				// Print the system region information.
				PrintSystemRegion();
				break;

			case _T('p'):
				// Print pathnames.
				PrintPathnames();
				break;

			case _T('l'): {
				// Language code.
				// NOTE: Actual language may be immediately after 'l',
				// or it might be a completely separate argument.
				// NOTE 2: Allowing multiple language codes, since the
				// language code affects files specified *after* it.
				const TCHAR *s_lang;
				if (argv[i][2] == _T('\0')) {
					// Separate argument.
					s_lang = argv[i+1];
					i++;
				} else {
					// Same argument.
					s_lang = &argv[i][2];
				}

				// Parse the language code.
				uint32_t new_lc = 0;
				int pos;
				for (pos = 0; pos < 4 && s_lang[pos] != _T('\0'); pos++) {
					new_lc <<= 8;
					new_lc |= static_cast<uint8_t>(s_lang[pos]);
				}
				if (pos == 4 && s_lang[pos] != _T('\0')) {
					// Invalid language code.
					fmt::print(stderr, FRUN(C_("rpcli", "Warning: ignoring invalid language code '{:s}'")), T2U8c(s_lang));
					fputc('\n', stderr);
					fflush(stderr);
					break;
				}

				// Language code set.
				lc = new_lc;
				break;
			}

			case _T('K'): {
				// Skip internal images. (NOTE: Not documented.)
				flags |= LibRpBase::OF_SkipInternalImages;
				break;
			}

			case _T('d'): {
				// Skip RFT_LISTDATA with more than 10 items. (Text only)
				flags |= LibRpBase::OF_SkipListDataMoreThan10;
				break;
			}

			case _T('x'): {
				const TCHAR *const ts_imgType = argv[i] + 2;
				TCHAR *endptr = nullptr;
				const long num = _tcstol(ts_imgType, &endptr, 10);
				if (*endptr != '\0') {
#ifdef _WIN32
					// fmt::print() doesn't allow mixing narrow and wide strings.
					const string s_imgType = T2U8(ts_imgType);
#else /* !_WIN32 */
					const char *const s_imgType = ts_imgType;
#endif /* _WIN32 */
					fmt::print(stderr, FRUN(C_("rpcli", "Warning: skipping invalid image type '{:s}'")), s_imgType);
					fputc('\n', stderr);
					fflush(stderr);
					i++; continue;
				} else if (num < RomData::IMG_INT_MIN || num > RomData::IMG_INT_MAX) {
					fmt::print(stderr, FRUN(C_("rpcli", "Warning: skipping unknown image type {:d}")), num);
					fputc('\n', stderr);
					fflush(stderr);
					i++; continue;
				}
				extract.emplace_back(argv[++i], num);
				break;
			}

			case _T('m'): {
				const TCHAR *const ts_mipmapLevel = argv[i] + 2;
				TCHAR *endptr = nullptr;
				const long num = _tcstol(ts_mipmapLevel, &endptr, 10);
				if (*endptr != '\0') {
#ifdef _WIN32
					// fmt::print() doesn't allow mixing narrow and wide strings.
					const string s_mipmapLevel = T2U8(ts_mipmapLevel);
#else /* !_WIN32 */
					const char *const s_mipmapLevel = ts_mipmapLevel;
#endif /* _WIN32 */
					fmt::print(stderr, FRUN(C_("rpcli", "Warning: skipping invalid mipmap level '{:s}'")), s_mipmapLevel);
					fputc('\n', stderr);
					fflush(stderr);
					i++; continue;
				} else if (num < -1 || num > 1024) {
					fmt::print(stderr, FRUN(C_("rpcli", "Warning: skipping out-of-range mipmap level {:d}")), num);
					fputc('\n', stderr);
					fflush(stderr);
					i++; continue;
				}
				extract.emplace_back(argv[++i], RomData::IMG_INT_IMAGE, num);
				break;
			}

			case _T('a'):
				extract.emplace_back(argv[++i], -1);
				break;

			case _T('j'): // do nothing
			case _T('J'): // still do nothing
				break;

#ifdef RP_OS_SCSI_SUPPORTED
			case _T('i'):
				// These commands take precedence over the usual rpcli functionality.
				switch (argv[i][2]) {
					case _T('s'):
						// SCSI INQUIRY command.
						inq_scsi = true;
						break;
					case _T('a'):
						// ATA IDENTIFY DEVICE command.
						inq_ata = true;
						break;
					case _T('p'):
						// ATA IDENTIFY PACKET DEVICE command.
						inq_ata_packet = true;
						break;
					default:
						if (argv[i][2] == _T('\0')) {
							fputs(C_("rpcli", "Warning: no inquiry request specified for '-i'"), stderr);
							fputc('\n', stderr);
						} else {
							// FIXME: Unicode character on Windows.
							fmt::print(stderr, FRUN(C_("rpcli", "Warning: skipping unknown inquiry request '{:c}'")), (char)argv[i][2]);
							fputc('\n', stderr);
						}
						fflush(stderr);
						break;
				}
				break;
#endif /* RP_OS_SCSI_SUPPORTED */

			default:
				// FIXME: Unicode character on Windows.
				fmt::print(stderr, FRUN(C_("rpcli", "Warning: skipping unknown switch '{:c}'")), (char)argv[i][1]);
				fputc('\n', stderr);
				fflush(stderr);
				break;
			}
		} else {
			if (first) {
				first = false;
			} else if (json) {
				cout << ",\n";
				cout.flush();
			}

			// TODO: Return codes?
#ifdef RP_OS_SCSI_SUPPORTED
			if (inq_scsi) {
				// SCSI INQUIRY command.
				DoScsiInquiry(argv[i], json);
			} else if (inq_ata) {
				// ATA IDENTIFY DEVICE command.
				DoAtaIdentifyDevice(argv[i], json, false);
			} else if (inq_ata_packet) {
				// ATA IDENTIFY PACKET DEVICE command.
				DoAtaIdentifyDevice(argv[i], json, true);
			} else
#endif /* RP_OS_SCSI_SUPPORTED */
			{
				// Regular file.
				DoFile(argv[i], json, extract, lc, flags);
			}

#ifdef RP_OS_SCSI_SUPPORTED
			inq_scsi = false;
			inq_ata = false;
			inq_ata_packet = false;
#endif /* RP_OS_SCSI_SUPPORTED */
			extract.clear();
		}
	}
	if (json) {
		cout << "]\n";
		cout.flush();
	}

#ifdef _WIN32
	// Shut down GDI+.
	GdiplusHelper::ShutdownGDIPlus(gdipToken);
#endif /* _WIN32 */

	return ret;
}
