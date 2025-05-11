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

// libgsvt for VT handling
#include "gsvt.h"

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
				gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
				gsvt_fputs("-- ", gsvt_stderr);
				if (likely(!isMipmap)) {
					// tr: {0:s} == image type name, {1:s} == output filename
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Extracting {0:s} into '{1:s}'")),
						RomData::getImageTypeName(imageType),
						T2U8c(p.filename)), gsvt_stderr);
				} else {
					// tr: {0:d} == mipmap level, {1:s} == output filename
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Extracting mipmap level {0:d} into '{1:s}'")),
						p.mipmapLevel, T2U8c(p.filename)), gsvt_stderr);
				}
				gsvt_text_color_reset(gsvt_stderr);
				gsvt_newline(gsvt_stderr);
				fflush(stderr);

				int errcode = RpPng::save(p.filename, image);
				if (errcode != 0) {
					// tr: {0:s} == filename, {1:s} == error message
					gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Couldn't create file '{0:s}': {1:s}")),
						T2U8c(p.filename), strerror(-errcode)), gsvt_stderr);
					gsvt_text_color_reset(gsvt_stderr);
					gsvt_newline(gsvt_stderr);
				} else {
					gsvt_fputs("   ", gsvt_stderr);
					gsvt_fputs(C_("rpcli", "Done"), gsvt_stderr);
					gsvt_newline(gsvt_stderr);
				}
				fflush(stderr);
			}
		} else if (p.imageType == -1) {
			// iconAnimData image
			auto iconAnimData = romData->iconAnimData();
			if (iconAnimData && iconAnimData->count != 0 && iconAnimData->seq_count != 0) {
				found = true;
				gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
				gsvt_fputs("-- ", gsvt_stderr);
				gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Extracting animated icon into '{:s}'")),
					T2U8c(p.filename)), gsvt_stderr);
				gsvt_text_color_reset(gsvt_stderr);
				gsvt_newline(gsvt_stderr);
				fflush(stderr);

				int errcode = RpPng::save(p.filename, iconAnimData);
				if (errcode == -ENOTSUP) {
					gsvt_text_color_set8(gsvt_stderr, 3, true);	// yellow
					gsvt_fputs("   ", gsvt_stderr);
					gsvt_fputs(C_("rpcli", "APNG is not supported, extracting only the first frame"), gsvt_stderr);
					gsvt_text_color_reset(gsvt_stderr);
					gsvt_newline(gsvt_stderr);
					fflush(stderr);
					// falling back to outputting the first frame
					errcode = RpPng::save(p.filename, iconAnimData->frames[iconAnimData->seq_index[0]]);
				}
				if (errcode != 0) {
					gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
					gsvt_fputs("   ", gsvt_stderr);
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Couldn't create file '{0:s}': {1:s}")),
						T2U8c(p.filename), strerror(-errcode)), gsvt_stderr);
					gsvt_text_color_reset(gsvt_stderr);
					gsvt_newline(gsvt_stderr);
				} else {
					gsvt_fputs("   ", gsvt_stderr);
					gsvt_fputs(C_("rpcli", "Done"), gsvt_stderr);
					gsvt_newline(gsvt_stderr);
				}
				fflush(stderr);
			}
		}

		if (!found) {
			// TODO: Return an error code?
			gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
			gsvt_fputs("-- ", gsvt_stderr);
			if (p.imageType == -1) {
				gsvt_fputs(C_("rpcli", "Animated icon not found"), gsvt_stderr);
			} else if (p.mipmapLevel >= 0) {
				gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Mipmap level {:d} not found")), p.mipmapLevel), gsvt_stderr);
			} else {
				const RomData::ImageType imageType =
					static_cast<RomData::ImageType>(p.imageType);
				gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Image '{:s}' not found")),
					RomData::getImageTypeName(imageType)), gsvt_stderr);
			}
			gsvt_text_color_reset(gsvt_stderr);
			gsvt_newline(gsvt_stderr);
			fflush(stderr);
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
		gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
		gsvt_fputs("== ", gsvt_stderr);
		gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Reading file '{:s}'...")), T2U8c(filename)), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		shared_ptr<RpFile> file = std::make_shared<RpFile>(filename, RpFile::FM_OPEN_READ_GZ);
		if (!file->isOpen()) {
			// TODO: Return an error code?
			gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
			gsvt_fputs("-- ", gsvt_stderr);
			gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Couldn't open file: {:s}")), strerror(file->lastError())), gsvt_stderr);
			gsvt_text_color_reset(gsvt_stderr);
			gsvt_newline(gsvt_stderr);
			fflush(stderr);
			if (json) {
				gsvt_fputs(fmt::format(FSTR("{{\"error\":\"couldn't open file\",\"code\":{:d}}}\n"), file->lastError()), gsvt_stdout);
				fflush(stdout);
			}
			return;
		}

		romData = RomDataFactory::create(file);
	} else {
		// Directory: Call RomDataFactory::create() with the filename.

		// FIXME: Make T2U8c() unnecessary here.
		gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
		gsvt_fputs("== ", gsvt_stderr);
		gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Reading directory '{:s}'...")), T2U8c(filename)), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		romData = RomDataFactory::create(filename);
	}

	if (romData) {
		if (json) {
			gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
			gsvt_fputs("-- ", gsvt_stderr);
			gsvt_fputs(C_("rpcli", "Outputting JSON data"), gsvt_stderr);
			gsvt_text_color_reset(gsvt_stderr);
			gsvt_newline(gsvt_stderr);
			fflush(stderr);

#ifdef _WIN32
			// Windows: Use gsvt_fwrite() for faster console output where applicable.
			// FIXME: gsvt_cout wrapper.
			// FIXME: gsvt_fwrite_raw() function to skip ANSI escape parsing.
			ostringstream oss;
			oss << JSONROMOutput(romData.get(), flags) << '\n';
			const string str = oss.str();
			// TODO: Error checking.
			gsvt_fwrite(str.data(), str.size(), gsvt_stdout);
#else /* !_WIN32 */
			// Not Windows: Write directly to cout.
			// FIXME: gsvt_cout wrapper.
			cout << JSONROMOutput(romData.get(), flags) << '\n';
#endif /* _WIN32 */
		} else {
#ifdef _WIN32
			// Windows: Use gsvt_fwrite() for faster console output where applicable.
			// FIXME: gsvt_cout wrapper.
			ostringstream oss;
			oss << ROMOutput(romData.get(), lc, flags) << '\n';
			const string str = oss.str();
			// TODO: Error checking.
			gsvt_fwrite(str.data(), str.size(), gsvt_stdout);
#else /* !_WIN32 */
#ifdef ENABLE_SIXEL
			// If this is a tty, print the icon/banner using libsixel.
			if (gsvt_is_console(gsvt_stdout)) {
				print_sixel_icon_banner(romData);
			}
#endif /* ENABLE_SIXEL */
			// Not Windows: Write directly to cout.
			// FIXME: gsvt_cout wrapper.
			cout << ROMOutput(romData.get(), lc, flags) << '\n';
#endif /* _WIN32 */
		}
		cout.flush();
		fflush(stdout);
		ExtractImages(romData.get(), extract);
	} else {
		gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
		gsvt_fputs("-- ", gsvt_stderr);
		gsvt_fputs(C_("rpcli", "ROM is not supported"), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		if (json) {
			gsvt_fputs(("{\"error\":\"rom is not supported\"}\n"), gsvt_stdout);
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
	gsvt_fputs(fmt::format(FRUN(C_("rpcli", "System language code: {:s}")),
		(!buf.empty() ? buf.c_str() : C_("rpcli", "0 (this is a bug!)"))), gsvt_stdout);
	gsvt_newline(gsvt_stdout);

	uint32_t cc = __swab32(SystemRegion::getCountryCode());
	buf.clear();
	if (cc != 0) {
		for (unsigned int i = 4; i > 0; i--, cc >>= 8) {
			if ((cc & 0xFF) == 0)
				continue;
			buf += static_cast<char>(cc & 0xFF);
		}
	}
	gsvt_fputs(fmt::format(FRUN(C_("rpcli", "System country code: {:s}")),
		(!buf.empty() ? buf.c_str() : C_("rpcli", "0 (this is a bug!)"))), gsvt_stdout);
	gsvt_newline(gsvt_stdout);

	// Extra line. (TODO: Only if multiple commands are specified.)
	gsvt_newline(gsvt_stdout);
	fflush(stdout);
}

/**
 * Print pathname information.
 */
static void PrintPathnames(void)
{
	// TODO: Localize these strings?
	// TODO: Only print an extra line if multiple commands are specified.
	gsvt_fputs(fmt::format(FSTR(
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
		FileSystem::getConfigDirectory()), gsvt_stdout);
	gsvt_newline(gsvt_stdout);

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
	gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
	gsvt_fputs("== ", gsvt_stderr);
	gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Opening device file '{:s}'...")), T2U8c(filename)), gsvt_stderr);
	gsvt_text_color_reset(gsvt_stderr);
	gsvt_newline(gsvt_stderr);
	fflush(stderr);

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ_GZ));
	if (!file->isOpen()) {
		// TODO: Return an error code?
		gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
		gsvt_fputs("-- ", gsvt_stderr);
		gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Couldn't open file: {:s}")), strerror(file->lastError())), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		if (json) {
			gsvt_fputs(fmt::format("{{\"error\":\"couldn't open file\",\"code\":{:d}}}\n", file->lastError()), gsvt_stdout);
			fflush(stdout);
		}
		return;
	}

	// TODO: Check for unsupported devices? (Only CD-ROM is supported.)
	if (!file->isDevice()) {
		// TODO: Return an error code?
		gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
		gsvt_fputs("-- ", gsvt_stderr);
		gsvt_fputs(C_("rpcli", "Not a device file"), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		if (json) {
			gsvt_fputs("{\"error\":\"not a device file\"}\n", gsvt_stdout);
			fflush(stdout);
		}
		return;
	}

	if (json) {
		gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
		gsvt_fputs("-- ", gsvt_stderr);
		gsvt_fputs(C_("rpcli", "Outputting JSON data"), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		// TODO: JSONScsiInquiry
		//cout << JSONScsiInquiry(file.get()) << '\n';
		//cout.flush();
	} else {
#ifdef _WIN32
		// Windows: Use gsvt_fwrite() for faster console output where applicable.
		// FIXME: gsvt_cout wrapper.
		// FIXME: gsvt_fwrite_raw() function to skip ANSI escape parsing.
		ostringstream oss;
		oss << ScsiInquiry(file.get()) << '\n';
		cout.flush();
		const string str = oss.str();
		// TODO: Error checking.
		gsvt_fwrite(str.data(), str.size(), gsvt_stdout);
#else /* !_WIN32 */
		// Not Windows: Write directly to cout.
		// FIXME: gsvt_cout wrapper.
		cout << ScsiInquiry(file.get()) << '\n';
		cout.flush();
#endif /* _WIN32 */
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
	gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
	gsvt_fputs("== ", gsvt_stderr);
	gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Opening device file '{:s}'...")), T2U8c(filename)), gsvt_stderr);
	gsvt_text_color_reset(gsvt_stderr);
	gsvt_newline(gsvt_stderr);
	fflush(stderr);

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ_GZ));
	if (!file->isOpen()) {
		// TODO: Return an error code?
		gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
		gsvt_fputs("-- ", gsvt_stderr);
		gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Couldn't open file: {:s}")), strerror(file->lastError())), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		if (json) {
			gsvt_fputs(fmt::format("{{\"error\":\"couldn't open file\",\"code\":{:d}}}\n", file->lastError()), gsvt_stdout);
			fflush(stdout);
		}
		return;
	}

	// TODO: Check for unsupported devices? (Only CD-ROM is supported.)
	if (!file->isDevice()) {
		// TODO: Return an error code?
		gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
		gsvt_fputs("-- ", gsvt_stderr);
		gsvt_fputs(C_("rpcli", "Not a device file"), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		if (json) {
			gsvt_fputs("{\"error\":\"Not a device file\"}\n", gsvt_stdout);
			fflush(stdout);
		}
		return;
	}

	if (json) {
		gsvt_text_color_set8(gsvt_stderr, 6, true);	// cyan
		gsvt_fputs("-- ", gsvt_stderr);
		gsvt_fputs(C_("rpcli", "Outputting JSON data"), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
		fflush(stderr);

		// TODO: JSONAtaIdentifyDevice
		//cout << JSONAtaIdentifyDevice(file.get(), packet) << '\n';
		//cout.flush();
	} else {
#ifdef _WIN32
		// Windows: Use gsvt_fwrite() for faster console output where applicable.
		// FIXME: gsvt_cout wrapper.
		// FIXME: gsvt_fwrite_raw() function to skip ANSI escape parsing.
		ostringstream oss;
		oss << AtaIdentifyDevice(file.get(), packet) << '\n';
		cout.flush();
		const string str = oss.str();
		// TODO: Error checking.
		gsvt_fwrite(str.data(), str.size(), gsvt_stdout);
#else /* !_WIN32 */
		// Not Windows: Write directly to cout.
		// FIXME: gsvt_cout wrapper.
		cout << AtaIdentifyDevice(file.get(), packet) << '\n';
		cout.flush();
#endif /* _WIN32 */
	}
}
#endif /* RP_OS_SCSI_SUPPORTED */

static void ShowUsage(void)
{
	// TODO: Use argv[0] instead of hard-coding 'rpcli'?
#ifdef ENABLE_DECRYPTION	
	const char *const s_usage = C_("rpcli", "Usage: rpcli [-k] [-c] [-p] [-j] [-l lang] [[-xN outfile]... [-mN outfile]... [-a apngoutfile] filename]...");
#else /* !ENABLE_DECRYPTION */
	const char *const s_usage = C_("rpcli", "Usage: rpcli [-c] [-p] [-j] [-l lang] [[-xN outfile]... [-mN outfile]... [-a apngoutfile] filename]...");
#endif /* ENABLE_DECRYPTION */
	gsvt_fputs(s_usage, gsvt_stderr);
	gsvt_newline(gsvt_stderr);

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
		gsvt_fputs(p.opt, gsvt_stderr);
		gsvt_fputs(pgettext_expr("rpcli", p.desc), gsvt_stderr);
		gsvt_newline(gsvt_stderr);
	}
	gsvt_newline(gsvt_stderr);

#ifdef RP_OS_SCSI_SUPPORTED
	// Commands for devices
	static const array<cmd_t, 3> cmds_dev = {{
		{"  -is: ", NOP_C_("rpcli", "Run a SCSI INQUIRY command.")},
		{"  -ia: ", NOP_C_("rpcli", "Run an ATA IDENTIFY DEVICE command.")},
		{"  -ip: ", NOP_C_("rpcli", "Run an ATA IDENTIFY PACKET DEVICE command.")},
	}};

	gsvt_fputs(C_("rpcli", "Special options for devices:"), gsvt_stderr);
	gsvt_newline(gsvt_stderr);
	for (const auto &p : cmds_dev) {
		gsvt_fputs(p.opt, gsvt_stderr);
		gsvt_fputs(pgettext_expr("rpcli", p.desc), gsvt_stderr);
		gsvt_newline(gsvt_stderr);
	}
	gsvt_newline(gsvt_stderr);
#endif /* RP_OS_SCSI_SUPPORTED */

	gsvt_fputs(C_("rpcli", "Examples:"), gsvt_stderr);
	gsvt_newline(gsvt_stderr);
	gsvt_fputs("* rpcli s3.gen\n", gsvt_stderr);
	gsvt_fputs("\t ", gsvt_stderr);
		gsvt_fputs(C_("rpcli", "displays info about s3.gen"), gsvt_stderr);
		gsvt_newline(gsvt_stderr);
	gsvt_fputs("* rpcli -x0 icon.png pokeb2.nds\n", gsvt_stderr);
	gsvt_fputs("\t ", gsvt_stderr);
		gsvt_fputs(C_("rpcli", "extracts icon from pokeb2.nds"), gsvt_stderr);
		gsvt_newline(gsvt_stderr);
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

	// Detect console information.
	gsvt_init();

#if defined(ENABLE_NLS) && defined(_MSC_VER)
	// Delay load verification: libgnuintl
	// TODO: Skip this if not linked with /DELAYLOAD?
	if (DelayLoad_test_libintl_textdomain() != 0) {
		// Delay load failed.
		gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
		gsvt_fputs("*** ERROR: " LIBGNUINTL_DLL " could not be loaded.", gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_fputs("\n\n"
			"This build of rom-properties has localization enabled,\n"
			"which requires the use of GNU gettext.\n\n"
			"Please redownload rom-properties " RP_VERSION_STRING " and copy the\n"
			LIBGNUINTL_DLL " file to the installation directory.\n", gsvt_stderr);
		fflush(stderr);
		return EXIT_FAILURE;
	}
#endif /* ENABLE_NLS && _MSC_VER */

#ifdef RP_LIBROMDATA_IS_DLL
#  ifdef _MSC_VER
#    define ROMDATA_PREFIX
#  else
#    define ROMDATA_PREFIX "lib"
#  endif
#  ifndef NDEBUG
#    define ROMDATA_SUFFIX "-" LIBROMDATA_SOVERSION_STR "d"
#  else
#    define ROMDATA_SUFFIX "-" LIBROMDATA_SOVERSION_STR
#  endif
#  ifdef _WIN32
#    define ROMDATA_EXT ".dll"
#  else
// TODO: macOS
#    define ROMDATA_EXT ".so"
#  endif
#  define ROMDATA_DLL ROMDATA_PREFIX "romdata" ROMDATA_SUFFIX ROMDATA_EXT

#  ifdef _MSC_VER
	// Delay load verification: romdata-X.dll
	// TODO: Skip this if not linked with /DELAYLOAD?
	if (DelayLoad_test_ImageTypesConfig_className() != 0) {
		// Delay load failed.
		gsvt_text_color_set8(gsvt_stderr, 1, true);	// red
		gsvt_fputs("*** ERROR: " ROMDATA_DLL " could not be loaded.", gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_fputs("\n\n"
			"Please redownload rom-properties " RP_VERSION_STRING " and copy the\n"
			ROMDATA_DLL " file to the installation directory.\n", gsvt_stderr);
		fflush(stderr);
		return EXIT_FAILURE;
	}
#  endif /* _MSC_VER */
#endif /* RP_LIBROMDATA_IS_DLL */

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
	// NOTE: Only checking ci_stdout here, since the actual data is printed to stdout.
	if (gsvt_is_console(gsvt_stdout)) {
#ifndef _WIN32
		// Non-Windows: ANSI color support is required.
		if (gsvt_supports_ansi(gsvt_stdout))
#endif /* !_WIN32 */
		{
			flags |= OF_Text_UseAnsiColor;
		}
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
		cout.flush();
		gsvt_fputs("[\n", gsvt_stdout);
		fflush(stdout);
	}

#ifdef _WIN32
	// Initialize GDI+.
	const ULONG_PTR gdipToken = GdiplusHelper::InitGDIPlus();
	assert(gdipToken != 0);
	if (gdipToken == 0) {
		gsvt_text_color_set8(gsvt_stderr, 6, true);	// red
		gsvt_fputs(C_("rpcli", "*** ERROR: GDI+ initialization failed."), gsvt_stderr);
		gsvt_text_color_reset(gsvt_stderr);
		gsvt_newline(gsvt_stderr);
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
			case _T('C'):
				// Force-enable ANSI escape sequences, even if it's not supported by the
				// console or we're redirected to a file.
				// TODO: Document this, and provide an option to force-disable ANSI.
				gsvt_force_color_on(gsvt_stdout);
				gsvt_force_color_on(gsvt_stderr);
				flags |= OF_Text_UseAnsiColor;
				break;

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
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Warning: ignoring invalid language code '{:s}'")), T2U8c(s_lang)), gsvt_stderr);
					gsvt_newline(gsvt_stderr);
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
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Warning: skipping invalid image type '{:s}'")), s_imgType), gsvt_stderr);
					gsvt_newline(gsvt_stderr);
					fflush(stderr);
					i++; continue;
				} else if (num < RomData::IMG_INT_MIN || num > RomData::IMG_INT_MAX) {
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Warning: skipping unknown image type {:d}")), num), gsvt_stderr);
					gsvt_newline(gsvt_stderr);
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
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Warning: skipping invalid mipmap level '{:s}'")), s_mipmapLevel), gsvt_stderr);
					gsvt_newline(gsvt_stderr);
					fflush(stderr);
					i++; continue;
				} else if (num < -1 || num > 1024) {
					gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Warning: skipping out-of-range mipmap level {:d}")), num), gsvt_stderr);
					gsvt_newline(gsvt_stderr);
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
							gsvt_fputs(C_("rpcli", "Warning: no inquiry request specified for '-i'"), gsvt_stderr);
						} else {
							// FIXME: Unicode character on Windows.
							gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Warning: skipping unknown inquiry request '{:c}'")),
								(char)argv[i][2]), gsvt_stderr);
						}
						gsvt_newline(gsvt_stderr);
						fflush(stderr);
						break;
				}
				break;
#endif /* RP_OS_SCSI_SUPPORTED */

			default:
				// FIXME: Unicode character on Windows.
				gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Warning: skipping unknown switch '{:c}'")), (char)argv[i][1]), gsvt_stderr);
				gsvt_newline(gsvt_stderr);
				fflush(stderr);
				break;
			}
		} else {
			if (first) {
				first = false;
			} else if (json) {
				cout.flush();
				gsvt_fputs(",\n", gsvt_stdout);
				fflush(stdout);
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
		cout.flush();
		gsvt_fputs("]\n", gsvt_stdout);
		fflush(stdout);
	}

#ifdef _WIN32
	// Shut down GDI+.
	GdiplusHelper::ShutdownGDIPlus(gdipToken);
#endif /* _WIN32 */

	return ret;
}
