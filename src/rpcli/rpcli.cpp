/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * rpcli.cpp: Command-line interface for properties.                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
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

#include "stdafx.h"
#include "config.rpcli.h"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/RomData.hpp"
#include "librpbase/SystemRegion.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// libromdata
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/RpPng.hpp"
#include "librpbase/img/IconAnimData.hpp"
#include "libromdata/RomDataFactory.hpp"
using namespace LibRomData;

#include "bmp.hpp"
#include "properties.hpp"
#ifdef ENABLE_DECRYPTION
# include "verifykeys.hpp"
#endif /* ENABLE_DECRYPTION */

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>

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

struct ExtractParam {
	int image_type; // Image Type. -1 = iconAnimData, MUST be between -1 and IMG_INT_MAX
	bool is_bmp; // If true, extract as BMP, otherwise as PNG. n/a for iconAnimData
	const char* filename; // Target filename. Can be null due to argv[argc]
};

/**
* Extracts images from romdata
* @param romData RomData containing the images
* @param extract Vector of image extraction parameters
*/
static void ExtractImages(RomData *romData, std::vector<ExtractParam>& extract) {
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
				if (it->is_bmp) {
					if (rpbmp(it->filename, image)) {
						// TODO: Error code.
						// tr: Filename
						cerr << "   " << rp_sprintf(C_("rpcli", "Couldn't create file '%s'"), it->filename) << endl;
					} else {
						cerr << "   " << C_("rpcli", "Done") << endl;
					}
				} else {
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
*/
static void DoFile(const char *filename, bool json, std::vector<ExtractParam>& extract){
	cerr << "== " << rp_sprintf(C_("rpcli", "Reading file '%s'..."), filename) << endl;
	IRpFile *file = new RpFile(filename, RpFile::FM_OPEN_READ);
	if (file->isOpen()) {
		RomData *romData = RomDataFactory::create(file);
		if (romData && romData->isValid()) {
			if (json) {
				cerr << "-- " << C_("rpcli", "Outputting JSON data") << endl;
				cout << JSONROMOutput(romData) << endl;
			} else {
				cout << ROMOutput(romData) << endl;
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
	delete file;
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

int RP_C_API main(int argc, char *argv[])
{
	// Set the C and C++ locales.
	locale::global(locale(""));

	// Initialize i18n.
	rp_i18n_init();

	if(argc < 2){
#ifdef ENABLE_DECRYPTION
		cerr << C_("rpcli", "Usage: rpcli [-k] [-c] [-j] [[-x[b]N outfile]... filename]...") << endl;
		cerr << "  -k:   " << C_("rpcli", "Verify encryption keys in keys.conf.") << endl;
#else /* !ENABLE_DECRYPTION */
		cerr << C_("rpcli", "Usage: rpcli [-j] [[-x[b]N outfile]... filename]...") << endl;
#endif /* ENABLE_DECRYPTION */
		cerr << "  -c:   " << C_("rpcli", "Print system region information.") << endl;
		cerr << "  -j:   " << C_("rpcli", "Use JSON output format.") << endl;
		cerr << "  -xN:  " << C_("rpcli", "Extract image N to outfile in PNG format.") << endl;
		cerr << "  -xbN: " << C_("rpcli", "Extract image N to outfile in BMP format.") << endl;
		cerr << "  -a:   " << C_("rpcli", "Extract the animated icon to outfile in APNG format.") << endl;
		cerr << endl;
		cerr << C_("rpcli", "Examples:") << endl;
		cerr << "* rpcli s3.gen" << endl;
		cerr << "\t " << C_("rpcli", "displays info about s3.gen") << endl;
		cerr << "* rpcli -x0 icon.png pokeb2.nds" << endl;
		cerr << "\t " << C_("rpcli", "extracts icon from pokeb2.nds") << endl;
	}
	
	assert(RomData::IMG_INT_MIN == 0);
	// DoFile parameters
	bool json = false;
	std::vector<ExtractParam> extract;

	for (int i = 1; i < argc; i++) { // figure out the json mode in advance
		if (argv[i][0] == '-' && argv[i][1] == 'j') {
			json = true;
		}
	}
	if (json) cout << "[\n";
	bool first = true;
	int ret = 0;
	for(int i=1;i<argc;i++){
		if(argv[i][0] == '-'){
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
			case 'c': {
				// Print the system region information.
				PrintSystemRegion();
				break;
			}
			case 'x': {
				ExtractParam ep;
				ep.is_bmp = argv[i][2] == 'b';
				long num = atol(argv[i] + (ep.is_bmp ? 3 : 2));
				if (num<RomData::IMG_INT_MIN || num>RomData::IMG_INT_MAX) {
					cerr << rp_sprintf(C_("rpcli", "Warning: skipping unknown image type %ld"), num) << endl;
					i++; continue;
				}
				ep.image_type = num;
				ep.filename = argv[++i];
				extract.push_back(ep);
				break;
			}
			case 'a': {
				ExtractParam ep;
				ep.image_type = -1;
				ep.is_bmp = false;
				ep.filename = argv[++i];
				extract.push_back(ep);
				break;
			}
			case 'j': // do nothing
				break;
			default:
				cerr << rp_sprintf(C_("rpcli", "Warning: skipping unknown switch '%c'"), argv[i][1]) << endl;
				break;
			}
		}
		else{
			if (first) first = false;
			else if (json) cout << "," << endl;
			DoFile(argv[i], json, extract);
			extract.clear();
		}
	}
	if (json) cout << "]\n";
	return ret;
}
