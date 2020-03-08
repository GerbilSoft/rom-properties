/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * rpcli.cpp: Command-line interface for properties.                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.rpcli.h"

// OS-specific security options.
#include "librpsecure/os-secure.h"

// librpbase
#include "librpbase/config.librpbase.h"
#include "librpbase/byteswap.h"
#include "librpbase/RomData.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbase/file/FileSystem.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// libromdata
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/img/RpPng.hpp"
#include "librpbase/img/IconAnimData.hpp"
#include "libromdata/RomDataFactory.hpp"
using namespace LibRomData;

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

#ifdef _WIN32
// libwin32common
# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */

#include "properties.hpp"
#ifdef ENABLE_DECRYPTION
# include "verifykeys.hpp"
#endif /* ENABLE_DECRYPTION */
#include "device.hpp"

// OS-specific userdirs
#ifdef _WIN32
# include "libwin32common/userdirs.hpp"
# define OS_NAMESPACE LibWin32Common
#else
# include "libunixcommon/userdirs.hpp"
# define OS_NAMESPACE LibUnixCommon
#endif
#include "tcharx.h"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

// C++ includes.
#include <fstream>
#include <iostream>
#include <locale>
#include <string>
#include <vector>
using std::cout;
using std::cerr;
using std::endl;
using std::locale;
using std::ofstream;
using std::string;
using std::vector;

#include "libi18n/config.libi18n.h"
#if defined(_MSC_VER) && defined(ENABLE_NLS)
// MSVC: Exception handling for /DELAYLOAD.
#include "libwin32common/DelayLoadHelper.h"
// DelayLoad test implementation.
#include "libi18n/i18n.h"
DELAYLOAD_TEST_FUNCTION_IMPL1(textdomain, nullptr);
#endif /* defined(_MSC_VER) && defined(ENABLE_NLS) */

struct ExtractParam {
	const char* filename;	// Target filename. Can be null due to argv[argc]
	int image_type;		// Image Type. -1 = iconAnimData, MUST be between -1 and IMG_INT_MAX

	ExtractParam(const char *filename, int image_type)
		: filename(filename), image_type(image_type) { }
};

/**
* Extracts images from romdata
* @param romData RomData containing the images
* @param extract Vector of image extraction parameters
*/
static void ExtractImages(const RomData *romData, vector<ExtractParam>& extract) {
	int supported = romData->supportedImageTypes();
	for (auto it = extract.begin(); it != extract.end(); ++it) {
		if (!it->filename) continue;
		bool found = false;
		
		if (it->image_type >= 0 && supported & (1 << it->image_type)) {
			// normal image
			auto image = romData->image((RomData::ImageType)it->image_type);
			if (image && image->isValid()) {
				found = true;
				cerr << "-- " <<
					// tr: %1$s == image type name, %2$s == output filename
					rp_sprintf_p(C_("rpcli", "Extracting %1$s into '%2$s'"),
						RomData::getImageTypeName((RomData::ImageType)it->image_type),
						it->filename) << endl;
				int errcode = RpPng::save(it->filename, image);
				if (errcode != 0) {
					// tr: %1$s == filename, %2%s == error message
					cerr << rp_sprintf_p(C_("rpcli", "Couldn't create file '%1$s': %2$s"),
						it->filename, strerror(-errcode)) << endl;
				} else {
					cerr << "   " << C_("rpcli", "Done") << endl;
				}
			}
		}

		else if (it->image_type == -1) {
			// iconAnimData image
			auto iconAnimData = romData->iconAnimData();
			if (iconAnimData && iconAnimData->count != 0 && iconAnimData->seq_count != 0) {
				found = true;
				cerr << "-- " << rp_sprintf(C_("rpcli", "Extracting animated icon into '%s'"), it->filename) << endl;
				int errcode = RpPng::save(it->filename, iconAnimData);
				if (errcode == -ENOTSUP) {
					cerr << "   " << C_("rpcli", "APNG not supported, extracting only the first frame") << endl;
					// falling back to outputting the first frame
					errcode = RpPng::save(it->filename, iconAnimData->frames[iconAnimData->seq_index[0]]);
				}
				if (errcode != 0) {
					cerr << "   " <<
						rp_sprintf_p(C_("rpcli", "Couldn't create file '%1$s': %2$s"),
							it->filename, strerror(-errcode)) << endl;
				} else {
					cerr << "   " << C_("rpcli", "Done") << endl;
				}
			}
		}
		if (!found) {
			// TODO: Return an error code?
			if (it->image_type == -1) {
				cerr << "-- " << C_("rpcli", "Animated icon not found") << endl;
			} else {
				cerr << "-- " <<
					rp_sprintf(C_("rpcli", "Image '%s' not found"),
						RomData::getImageTypeName((RomData::ImageType)it->image_type)) << endl;
			}
		}
	}
}

/**
 * Shows info about file
 * @param filename ROM filename
 * @param json Is program running in json mode?
 * @param extract Vector of image extraction parameters
 * @param languageCode Language code. (0 for default)
 */
static void DoFile(const char *filename, bool json, vector<ExtractParam>& extract, uint32_t languageCode = 0)
{
	cerr << "== " << rp_sprintf(C_("rpcli", "Reading file '%s'..."), filename) << endl;
	RpFile *const file = new RpFile(filename, RpFile::FM_OPEN_READ_GZ);
	if (file->isOpen()) {
		RomData *romData = RomDataFactory::create(file);
		if (romData && romData->isValid()) {
			if (json) {
				cerr << "-- " << C_("rpcli", "Outputting JSON data") << endl;
				cout << JSONROMOutput(romData, languageCode) << endl;
			} else {
				cout << ROMOutput(romData, languageCode) << endl;
			}

			ExtractImages(romData, extract);
		} else {
			cerr << "-- " << C_("rpcli", "ROM is not supported") << endl;
			if (json) cout << "{\"error\":\"rom is not supported\"}" << endl;
		}

		if (romData) {
			romData->unref();
		}
	} else {
		cerr << "-- " << rp_sprintf(C_("rpcli", "Couldn't open file: %s"), strerror(file->lastError())) << endl;
		if (json) cout << "{\"error\":\"couldn't open file\",\"code\":" << file->lastError() << "}" << endl;
	}
	file->unref();
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
			buf += (char)(lc & 0xFF);
		}
	}
	cout << rp_sprintf(C_("rpcli", "System language code: %s"),
		(!buf.empty() ? buf.c_str() : C_("rpcli", "0 (this is a bug!)")));
	cout << endl;

	uint32_t cc = __swab32(SystemRegion::getCountryCode());
	buf.clear();
	if (cc != 0) {
		for (unsigned int i = 4; i > 0; i--, cc >>= 8) {
			if ((cc & 0xFF) == 0)
				continue;
			buf += (char)(cc & 0xFF);
		}
	}
	cout << rp_sprintf(C_("rpcli", "System country code: %s"),
		(!buf.empty() ? buf.c_str() : C_("rpcli", "0 (this is a bug!)")));
	cout << endl;

	// Extra line. (TODO: Only if multiple commands are specified.)
	cout << endl;
}

/**
 * Print pathname information.
 */
static void PrintPathnames(void)
{
	cout << rp_sprintf("User's home directory:   ") << OS_NAMESPACE::getHomeDirectory() << endl;
	cout << rp_sprintf("User's cache directory:  ") << OS_NAMESPACE::getCacheDirectory() << endl;
	cout << rp_sprintf("User's config directory: ") << OS_NAMESPACE::getConfigDirectory() << endl;
	cout << endl;
	cout << rp_sprintf("RP cache directory:      ") << FileSystem::getCacheDirectory() << endl;
	cout << rp_sprintf("RP config directory:     ") << FileSystem::getConfigDirectory() << endl;

	// Extra line. (TODO: Only if multiple commands are specified.)
	cout << endl;
}

#ifdef RP_OS_SCSI_SUPPORTED
/**
 * Run a SCSI INQUIRY command on a device.
 * @param filename Device filename
 * @param json Is program running in json mode?
 */
static void DoScsiInquiry(const char *filename, bool json)
{
	cerr << "== " << rp_sprintf(C_("rpcli", "Opening device file '%s'..."), filename) << endl;
	RpFile *const file = new RpFile(filename, RpFile::FM_OPEN_READ_GZ);
	if (file->isOpen()) {
		// TODO: Check for unsupported devices? (Only CD-ROM is supported.)
		if (file->isDevice()) {
			if (json) {
				cerr << "-- " << C_("rpcli", "Outputting JSON data") << endl;
				// TODO: JSONScsiInquiry
				//cout << JSONScsiInquiry(file) << endl;
			} else {
				cout << ScsiInquiry(file) << endl;
			}
		} else {
			cerr << "-- " << C_("rpcli", "Not a device file") << endl;
			if (json) cout << "{\"error\":\"Not a device file\"}" << endl;
		}
	} else {
		cerr << "-- " << rp_sprintf(C_("rpcli", "Couldn't open file: %s"), strerror(file->lastError())) << endl;
		if (json) cout << "{\"error\":\"couldn't open file\",\"code\":" << file->lastError() << "}" << endl;
	}
	file->unref();
}

/**
 * Run an ATA IDENTIFY DEVICE command on a device.
 * @param filename Device filename
 * @param json Is program running in json mode?
 */
static void DoAtaIdentifyDevice(const char *filename, bool json)
{
	cerr << "== " << rp_sprintf(C_("rpcli", "Opening device file '%s'..."), filename) << endl;
	RpFile *const file = new RpFile(filename, RpFile::FM_OPEN_READ_GZ);
	if (file->isOpen()) {
		// TODO: Check for unsupported devices? (Only CD-ROM is supported.)
		if (file->isDevice()) {
			if (json) {
				cerr << "-- " << C_("rpcli", "Outputting JSON data") << endl;
				// TODO: JSONAtaIdentifyDevice
				//cout << JSONAtaIdentifyDevice(file) << endl;
			} else {
				cout << AtaIdentifyDevice(file) << endl;
			}
		} else {
			cerr << "-- " << C_("rpcli", "Not a device file") << endl;
			if (json) cout << "{\"error\":\"Not a device file\"}" << endl;
		}
	} else {
		cerr << "-- " << rp_sprintf(C_("rpcli", "Couldn't open file: %s"), strerror(file->lastError())) << endl;
		if (json) cout << "{\"error\":\"couldn't open file\",\"code\":" << file->lastError() << "}" << endl;
	}
	file->unref();
}
#endif /* RP_OS_SCSI_SUPPORTED */

int RP_C_API main(int argc, char *argv[])
{
	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = FALSE;
#elif defined(HAVE_SECCOMP)
	static const int syscall_wl[] = {
		// Syscalls used by rp-download.
		// TODO: Add more syscalls.
		// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
		// defined in earlier versions, including Ubuntu 14.04.
		SCMP_SYS(close),
		SCMP_SYS(dup),		// gzdopen()
		SCMP_SYS(fstat),
		SCMP_SYS(ftruncate),	// LibRpBase::RpFile::truncate() [from LibRpBase::RpPngWriterPrivate::init()]
		SCMP_SYS(futex),
		SCMP_SYS(ioctl),	// for devices; also afl-fuzz
		SCMP_SYS(lseek),
		SCMP_SYS(lstat),	// LibRpBase::FileSystem::is_symlink(), resolve_symlink()
		SCMP_SYS(mmap), SCMP_SYS(mmap2),
		SCMP_SYS(mprotect),	// dlopen()
		SCMP_SYS(munmap),
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#ifdef __SNR_openat2
		SCMP_SYS(openat2),	// Linux 5.6
#endif /* __SNR_openat2 */
		SCMP_SYS(readlink),	// realpath() [LibRpBase::FileSystem::resolve_symlink()]

		// KeyManager (keys.conf)
		SCMP_SYS(access),	// LibUnixCommon::isWritableDirectory()
		SCMP_SYS(stat),		// LibUnixCommon::isWritableDirectory()

#ifdef __SNR_statx
		SCMP_SYS(getcwd),	// called by glibc's statx()
		SCMP_SYS(statx),
#endif /* __SNR_statx */

		// glibc ncsd
		// TODO: Restrict connect() to AF_UNIX.
		SCMP_SYS(connect), SCMP_SYS(recvmsg), SCMP_SYS(sendto),

		// NOTE: The following syscalls are only made if either access() or stat() can't be run.
		// TODO: Can this happen in other situations?
		//SCMP_SYS(getuid),
		//SCMP_SYS(socket),	// ???

		-1	// End of whitelist
	};
	param.syscall_wl = syscall_wl;
#elif defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from ~/.config/rom-properties/ and ~/.cache/rom-properties/
	// - wpath: Write to ~/.cache/rom-properties/
	// - cpath: Create ~/.cache/rom-properties/ if it doesn't exist.
	// - getpw: Get user's home directory if HOME is empty.
	param.promises = "stdio rpath wpath cpath getpw";
#elif defined(HAVE_TAME)
	param.tame_flags = TAME_STDIO | TAME_RPATH | TAME_WPATH | TAME_CPATH | TAME_GETPW;
#else
	param.dummy = 0;
#endif
	rp_secure_enable(param);

	// Set the C and C++ locales.
	locale::global(locale(""));

#if defined(_MSC_VER) && defined(ENABLE_NLS)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_textdomain() != 0) {
		// Delay load failed.
		// TODO: Use a CMake macro for the soversion?
		_fputts(_T("*** ERROR: ") LIBGNUINTL_DLL _T(" could not be loaded.\n\n")
			_T("This build of rom-properties has localization enabled,\n")
			_T("which requires the use of GNU gettext.\n\n")
			_T("Please redownload rom-properties and copy the\n")
			LIBGNUINTL_DLL _T(" file to the installation directory.\n"),
			stderr);
		return EXIT_FAILURE;
	}
#endif /* defined(_MSC_VER) && defined(ENABLE_NLS) */

	// Initialize i18n.
	rp_i18n_init();

	if(argc < 2){
#ifdef ENABLE_DECRYPTION
		cerr << C_("rpcli", "Usage: rpcli [-k] [-c] [-p] [-j] [-l lang] [[-x[b]N outfile]... [-a apngoutfile] filename]...") << endl;
		cerr << "  -k:   " << C_("rpcli", "Verify encryption keys in keys.conf.") << endl;
#else /* !ENABLE_DECRYPTION */
		cerr << C_("rpcli", "Usage: rpcli [-c] [-p] [-j] [-l lang] [[-x[b]N outfile]... [-a apngoutfile] filename]...") << endl;
#endif /* ENABLE_DECRYPTION */
		cerr << "  -c:   " << C_("rpcli", "Print system region information.") << endl;
		cerr << "  -p:   " << C_("rpcli", "Print system path information.") << endl;
		cerr << "  -j:   " << C_("rpcli", "Use JSON output format.") << endl;
		cerr << "  -l:   " << C_("rpcli", "Retrieve the specified language from the ROM image.") << endl;
		cerr << "  -xN:  " << C_("rpcli", "Extract image N to outfile in PNG format.") << endl;
		cerr << "  -a:   " << C_("rpcli", "Extract the animated icon to outfile in APNG format.") << endl;
		cerr << endl;
#ifdef RP_OS_SCSI_SUPPORTED
		cerr << "Special options for devices:" << endl;
		cerr << "  -is:   " << C_("rpcli", "Run a SCSI INQUIRY command.") << endl;
		cerr << "  -ia:   " << C_("rpcli", "Run an ATA IDENTIFY DEVICE command.") << endl;
		cerr << endl;
#endif /* RP_OS_SCSI_SUPPORTED */
		cerr << C_("rpcli", "Examples:") << endl;
		cerr << "* rpcli s3.gen" << endl;
		cerr << "\t " << C_("rpcli", "displays info about s3.gen") << endl;
		cerr << "* rpcli -x0 icon.png pokeb2.nds" << endl;
		cerr << "\t " << C_("rpcli", "extracts icon from pokeb2.nds") << endl;
	}
	
	assert(RomData::IMG_INT_MIN == 0);
	// DoFile parameters
	bool json = false;
	vector<ExtractParam> extract;

	for (int i = 1; i < argc; i++) { // figure out the json mode in advance
		if (argv[i][0] == '-' && argv[i][1] == 'j') {
			json = true;
		}
	}
	if (json) cout << "[\n";

#ifdef RP_OS_SCSI_SUPPORTED
	bool inq_scsi = false;
	bool inq_ata = false;
#endif /* RP_OS_SCSI_SUPPORTED */
	uint32_t languageCode = 0;
	bool first = true;
	int ret = 0;
	for (int i = 1; i < argc; i++){
		if (argv[i][0] == '-'){
			switch (argv[i][1]) {
#ifdef ENABLE_DECRYPTION
			case 'k': {
				// Verify encryption keys.
				static bool hasVerifiedKeys = false;
				if (!hasVerifiedKeys) {
					hasVerifiedKeys = true;
					ret = VerifyKeys();
				}
				break;
			}
#endif /* ENABLE_DECRYPTION */
			case 'c':
				// Print the system region information.
				PrintSystemRegion();
				break;
			case 'p':
				// Print pathnames.
				PrintPathnames();
				break;
			case 'l': {
				// Language code.
				// NOTE: Actual language may be immediately after 'l',
				// or it might be a completely separate argument.
				// NOTE 2: Allowing multiple language codes, since the
				// language code affects files specified *after* it.
				const char *s_lang;
				if (argv[i][2] == '\0') {
					// Separate argument.
					s_lang = argv[i+1];
					i++;
				} else {
					// Same argument.
					s_lang = &argv[i][2];
				}

				// Parse the language code.
				uint32_t lc = 0;
				int pos;
				for (pos = 0; pos < 4 && s_lang[pos] != '\0'; pos++) {
					lc <<= 8;
					lc |= (uint8_t)s_lang[pos];
				}
				if (pos == 4 && s_lang[pos] != '\0') {
					// Invalid language code.
					cerr << rp_sprintf(C_("rpcli", "Warning: ignoring invalid language code '%s'"), s_lang) << endl;
					break;
				}

				// Language code set.
				languageCode = lc;
				break;
			}
			case 'x': {
				long num = atol(argv[i] + 2);
				if (num<RomData::IMG_INT_MIN || num>RomData::IMG_INT_MAX) {
					cerr << rp_sprintf(C_("rpcli", "Warning: skipping unknown image type %ld"), num) << endl;
					i++; continue;
				}
				extract.emplace_back(ExtractParam(argv[++i], num));
				break;
			}
			case 'a':
				extract.emplace_back(ExtractParam(argv[++i], -1));
				break;
			case 'j': // do nothing
				break;
#ifdef RP_OS_SCSI_SUPPORTED
			case 'i':
				// TODO: Check if a SCSI implementation is available for this OS?
				// These commands take precedence over the usual rpcli functionality.
				if (argv[i][2] == 's') {
					// SCSI INQUIRY command.
					inq_scsi = true;
				} else if (argv[i][2] == 'a') {
					// ATA IDENTIFY DEVICE command.
					inq_ata = true;
				}
				break;
#endif /* RP_OS_SCSI_SUPPORTED */
			default:
				cerr << rp_sprintf(C_("rpcli", "Warning: skipping unknown switch '%c'"), argv[i][1]) << endl;
				break;
			}
		} else {
			if (first) first = false;
			else if (json) cout << "," << endl;

			// TODO: Return codes?
#ifdef RP_OS_SCSI_SUPPORTED
			if (inq_scsi) {
				// SCSI INQUIRY command.
				DoScsiInquiry(argv[i], json);
			} else if (inq_ata) {
				// ATA IDENTIFY DEVICE command.
				DoAtaIdentifyDevice(argv[i], json);
			} else
#endif /* RP_OS_SCSI_SUPPORTED */
			{
				// Regular file.
				DoFile(argv[i], json, extract, languageCode);
			}

#ifdef RP_OS_SCSI_SUPPORTED
			inq_scsi = false;
			inq_ata = false;
#endif /* RP_OS_SCSI_SUPPORTED */
			extract.clear();
		}
	}
	if (json) cout << "]\n";
	return ret;
}
