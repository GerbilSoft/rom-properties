/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * GcnFstPrint.cpp: GameCube/Wii FST printer.                              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "disc/GcnFst.hpp"
#include "FstPrint.hpp"
using LibRomData::GcnFst;

// C includes. (C++ namespace)
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// C++ includes.
#include <sstream>
using std::ostream;
using std::ostringstream;

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Syntax: %s fst.bin\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Open and read the FST file.
	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		printf("Error opening '%s': '%s'\n", argv[1], strerror(errno));
		return EXIT_FAILURE;
	}

	// Make sure the FST is less than 16 MB.
	fseeko(f, 0, SEEK_END);
	int64_t filesize = ftello(f);
	if (filesize > (16*1024*1024)) {
		printf("ERROR: FST is too big. (Maximum of 16 MB.)\n");
		return EXIT_FAILURE;
	}
	fseek(f, 0, SEEK_SET);

	// Read the FST into memory.
	uint8_t *fstData = reinterpret_cast<uint8_t*>(malloc(filesize));
	if (!fstData) {
		printf("ERROR: malloc(%u) failed.\n", (uint32_t)filesize);
		return EXIT_FAILURE;
	}
	size_t rd_size = fread(fstData, 1, filesize, f);
	if (rd_size != (size_t)filesize) {
		printf("ERROR: Read %u bytes, expected %u bytes.\n", (uint32_t)rd_size, (uint32_t)filesize);
		return EXIT_FAILURE;
	}

	// Parse the FST.
	// TODO: Offset shift.
	GcnFst *fst = new GcnFst(fstData, (uint32_t)filesize, 0);
	if (!fst) {
		printf("ERROR: new GcnFst() failed.\n");
		return EXIT_FAILURE;
	}

	// Print the FST to an ostringstream.
	ostringstream oss;
	LibRomData::fstPrint(fst, oss);
	printf("%s", oss.str().c_str());
}
