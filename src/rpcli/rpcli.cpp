/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * rpcli.cpp: Command-line interface for properties.                       *
 *                                                                         *
 * Copyright (c) 2016-2017 by Egor.                                        *
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

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cassert>
#include <vector>

#include <libromdata/file/RpFile.hpp>
#include <libromdata/RomData.hpp>
#include <libromdata/RomDataFactory.hpp>
#include <libromdata/TextFuncs.hpp>
#include <libromdata/img/rp_image.hpp>
#include <libromdata/img/RpPng.hpp>
#include <libromdata/img/IconAnimData.hpp>
#include "bmp.hpp"
#include "properties.hpp"
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using namespace LibRomData;

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
void ExtractImages(RomData *romData, std::vector<ExtractParam>& extract) {
	int supported = romData->supportedImageTypes();
	for (auto it = extract.begin(); it != extract.end(); ++it) {
		if (!it->filename) continue;
		bool found = false;
		
		if (it->image_type >= 0 && supported & (1 << it->image_type)) {
			// normal image
			auto image = romData->image((RomData::ImageType)it->image_type);
			if (image && image->isValid()) {
				found = true;
				cerr << "-- Extracting " << RomData::getImageTypeName((RomData::ImageType)it->image_type) << " into '" << it->filename << "'" << endl;
				if (it->is_bmp) {
					if (rpbmp(it->filename, image)) {
						cerr << "   Couldn't create file " << it->filename << endl;
					}
					else cerr << "   Done" << endl;
				}
				else {
					int errcode = RpPng::save(it->filename, image);
					if (errcode != 0) {
						cerr << "   Couldn't create file " << it->filename << " : " << strerror(-errcode) << endl;
					}
					else cerr << "   Done" << endl;
				}
			}
		}

		else if (it->image_type == -1) {
			// iconAnimData image
			auto iconAnimData = romData->iconAnimData();
			if (iconAnimData && iconAnimData->count != 0 && iconAnimData->seq_count != 0) {
				found = true;
				cerr << "-- Extracting animated icon into " << it->filename << endl;
				int errcode = RpPng::save(it->filename, iconAnimData);
				if (errcode == -ENOTSUP) {
					cerr << "   APNG not supported, extracting only the first frame" << endl;
					// falling back to outputting the first frame
					errcode = RpPng::save(it->filename, iconAnimData->frames[iconAnimData->seq_index[0]]);
				}
				if (errcode != 0) {
					cerr << "   Couldn't create file " << it->filename << " : " << strerror(-errcode) << endl;
				}
				else cerr << "   Done" << endl;
			}
		}
		if (!found) {
			if (it->image_type == -1) {
				cerr << "-- Animated icon not found" << endl;
			}
			else {
				cerr << "-- Image '" << RomData::getImageTypeName((RomData::ImageType)it->image_type) << "' not found" << endl;
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
void DoFile(const char *filename, bool json, std::vector<ExtractParam>& extract){
	cerr << "== Reading file '" << filename << "'..." << endl;
	IRpFile *file = new RpFile(filename, RpFile::FM_OPEN_READ);	
	if (file->isOpen()) {
		RomData *romData = RomDataFactory::getInstance(file);
		if (romData) {
			if (romData->isValid()) {

				if (json) {
					cerr << "-- Outputting json data" << endl;
					cout << JSONROMOutput(romData) << endl;
				}
				else {
					cout << ROMOutput(romData) << endl;
				}

				ExtractImages(romData, extract);
			}else{
				cerr << "-- Rom is not supported" << endl;
				if (json) cout << "{\"error\":\"rom is not supported\"}" << endl;
			}			
			romData->close();
		}else {
			cerr << "-- Unknown error" << endl;
			if (json) cout << "{\"error\":\"unknown error\"}" << endl;
		}
	}else{
		cerr << "-- Couldn't open file... : " << strerror(file->lastError()) << endl;
		if (json) cout << "{\"error\":\"couldn't open file\"}" << endl;
	}
	delete file;
}

#ifdef _WIN32
static int real_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	if(argc<2){
		cerr << "Usage: rpcli [-j] [[-x[b]N outfile]... filename]..." << endl;
		cerr << "Examples:" << endl;
		cerr << "* rpcli s3.gen" << endl;
		cerr << "\t displays info about s3.gen" << endl;
		cerr << "* rpcli -x0 icon.png ~/pokeb2.nds" << endl;
		cerr << "\t extracts icon from ~/pokeb2.nds" << endl;
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
	if (json) cout << "[";
	bool first = true;
	for(int i=1;i<argc;i++){
		if(argv[i][0] == '-'){
			switch (argv[i][1]) {
			case 'x': {
				ExtractParam ep;
				ep.is_bmp = argv[i][2] == 'b';
				long num = atol(argv[i] + (ep.is_bmp ? 3 : 2));
				if (num<RomData::IMG_INT_MIN || num>RomData::IMG_INT_MAX) {
					cerr << "Warning: skipping unknown image type " << num << endl;
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
				cerr << "Warning: skipping unknown switch '" << argv[i][1] << "'" << endl;
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
	if (json) cout << "]";
	return 0;
}

#ifdef _WIN32
// UTF-16 main() wrapper for Windows.
#include <windows.h>
int wmain(int argc, wchar_t *argv[]) {
	// Convert the UTF-16 arguments to UTF-8.
	// NOTE: Using WideCharToMultiByte() directly in order to
	// avoid std::string overhead.
	char **u8argv = new char*[argc+1];
	u8argv[argc] = nullptr;
	for (int i = 0; i < argc; i++) {
		int cbMbs = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
		if (cbMbs <= 0) {
			// Invalid string. Make it an empty string anyway.
			u8argv[i] = strdup("");
			continue;
		}

		u8argv[i] = (char*)malloc(cbMbs);
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, u8argv[i], cbMbs, nullptr, nullptr);
	}

	// Run the program.
	int ret = real_main(argc, u8argv);

	// Free u8argv[].
	for (int i = argc-1; i >= 0; i--) {
		free(u8argv[i]);
	}
	delete[] u8argv;

	return ret;
}
#endif /* _WIN32 */
