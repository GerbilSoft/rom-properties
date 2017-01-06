/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * rpcli.cpp: Command-line interface for properties.                       *
 *                                                                         *
 * Copyright (c) 2016 by Egor.                                             *
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

#include <libromdata/file/RpFile.hpp>
#include <libromdata/RomData.hpp>
#include <libromdata/RomDataFactory.hpp>
#include <libromdata/TextFuncs.hpp>
#include <libromdata/img/rp_image.hpp>
#include <libromdata/img/RpPng.hpp>
#include "bmp.hpp"
#include "properties.hpp"
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using namespace LibRomData;

/**
* Shows info about file
* @param filename ROM filename
* @param extract Bitmask: which ImageTypes should be extracted
* @param outnames Target filenames for extracting ImageTypes
* @param json Is program running in json mode?
* @param bmp Bitmask: should an image be extracted as png (0) or bmp (1)?
* @param iconanim Target filename for extracting IconAnimData. nullptr = don't extract
*/
void DoFile(const char *filename, uint32_t extract, const char *outnames[], bool json, uint32_t bmp, const char *iconanim){
	cerr << "== Reading file '" << filename << "'..." << endl;
	IRpFile *file = new RpFile(filename, RpFile::FM_OPEN_READ);	
	if (file && file->isOpen()) {
		RomData *romData = RomDataFactory::getInstance(file);
		if (romData) {
			if(romData->isValid()){

				if (json) {
					cerr << "-- Outputting json data" << endl;
					cout << JSONROMOutput(romData) << endl;
				}
				else {
					cerr << ROMOutput(romData) << endl;
				}
				
				int supported = romData->supportedImageTypes();
				for(int i=RomData::IMG_INT_MIN; i<=RomData::IMG_INT_MAX; i++){
					if(supported & extract & (1<<i)){
						auto image = romData->image((RomData::ImageType)i);
						cerr << "-- Extracting " << RomData::getImageTypeName((RomData::ImageType)i) << " into '" << outnames[i] << "'" << endl;
						if (bmp & (1 << i)) {
							if (rpbmp(outnames[i], image)) {
								cerr << "-- Couldn't create file " << outnames[i] << endl;
							}
						}
						else {
							int errcode;
							if (errcode = RpPng::save(outnames[i], image)) {
								cerr << "-- Couldn't create file " << outnames[i] << " : " << strerror(-errcode) << endl;
							}
						}				
					}
				}
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
int main(int argc,char **argv){
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
	const char* outnames[RomData::IMG_INT_MAX+1] = {0};
	uint32_t extract = 0, bmp = 0;
	bool json = false;
	const char* iconanim = nullptr;

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
				bool isbmp = argv[i][2] == 'b';
				long num = atol(argv[i] + (isbmp ? 3 : 2));
				if (num<RomData::IMG_INT_MIN || num>RomData::IMG_INT_MAX) {
					cerr << "Warning: skipping unknown image type " << num << endl;
					i++; continue;
				}
				outnames[num] = argv[++i];
				extract |= 1 << num;
				if (isbmp) bmp |= 1 << num;
				break;
			}
			case 'a':
				iconanim = argv[++i];
				break;
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
			DoFile(argv[i], extract, outnames, json, bmp, iconanim);
			extract = bmp = 0;
			iconanim = nullptr;
		}
	}
	if (json) cout << "]";
	return 0;
}