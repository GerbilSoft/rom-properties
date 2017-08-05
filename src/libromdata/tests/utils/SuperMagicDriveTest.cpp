/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * SuperMagicDriveTest.cpp: SuperMagicDrive class test.                    *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

// Google Test
#include "gtest/gtest.h"

// SuperMagicDrive
#include "libromdata/utils/SuperMagicDrive.hpp"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <memory>
using std::unique_ptr;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

// zlib
#define CHUNK 4096
#include <zlib.h>

namespace LibRomData { namespace Tests {

class SuperMagicDriveTest : public ::testing::Test
{
	protected:
		SuperMagicDriveTest() { }

	public:
		/**
		 * 16 KB plain binary data block.
		 */
		static const uint8_t bin_data_gz[11811];

		/**
		 * 16 KB SMD-interleaved data block.
		 */
		static const uint8_t smd_data_gz[403];

	public:
		// Uncompressed data buffers.
		static ao::uvector<uint8_t> m_bin_data;
		static ao::uvector<uint8_t> m_smd_data;

	private:
		/**
		 * Decompress a data block.
		 * @param vOut		[out] Output ao::uvector.
		 * @param pIn		[in] Input array.
		 * @param in_len	[in] Input length.
		 * @return 0 on success; non-zero on error.
		 */
		static int decompress(ao::uvector<uint8_t> &vOut, const uint8_t *pIn, unsigned int in_len);

	public:
		/**
		 * Decompress the data blocks.
		 * @return 0 on success; non-zero on error.
		 */
		static int decompress(void);
};

// Test data is in SuperMagicDriveTest_data.hpp.
#include "SuperMagicDriveTest_data.hpp"

// Uncompressed data buffers.
ao::uvector<uint8_t> SuperMagicDriveTest::m_bin_data;
ao::uvector<uint8_t> SuperMagicDriveTest::m_smd_data;

/**
 * Decompress a data block.
 * @param vOut		[out] Output ao::uvector.
 * @param pIn		[in] Input array.
 * @param in_len	[in] Input length.
 * @return 0 on success; non-zero on error.
 */
int SuperMagicDriveTest::decompress(ao::uvector<uint8_t> &vOut, const uint8_t *pIn, unsigned int in_len)
{
	// Based on zlib example code:
	// http://www.zlib.net/zlib_how.html
	int ret;
	z_stream strm;

	const unsigned int buf_siz = 16384;
	const unsigned int out_len = buf_siz + 64;
	vOut.resize(out_len);
	unsigned int out_pos = 0;

	// Allocate the zlib inflate state.
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = inflateInit2(&strm, 15+16);
	if (ret != Z_OK) {
		vOut.clear();
		return ret;
	}

	// Decompress the stream.
	unsigned int avail_out_before;
	unsigned int avail_out_after;
	// Set the input buffer values.
	// This doesn't need to be changed during decompression,
	// since we're reading everything in one go.
	strm.avail_in = in_len;
	strm.next_in = const_cast<Bytef*>(pIn);
	do {
		// Run inflate() on input until the output buffer is not full.
		do {
			avail_out_before = (out_len - out_pos);
			strm.avail_out = avail_out_before;
			strm.next_out = &vOut[out_pos];

			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);	// make sure the state isn't clobbered
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;
					// fall through
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
				case Z_STREAM_ERROR:
					// Error occurred while decoding the stream.
					inflateEnd(&strm);
					fprintf(stderr, "*** zlib error: %d\n", ret);
					vOut.clear();
					return ret;
				default:
					break;
			}

			// Increase the output position.
			avail_out_after = (avail_out_before - strm.avail_out);
			out_pos += avail_out_after;
		} while (strm.avail_out == 0 && avail_out_after > 0 && strm.avail_in > 0);
	} while (ret != Z_STREAM_END && avail_out_after > 0 && strm.avail_in > 0);

	// Close the stream.
	inflateEnd(&strm);

	// If we didn't actually finish reading the compressed data, something went wrong.
	if (ret != Z_STREAM_END) {
		vOut.clear();
		return Z_DATA_ERROR;
	}

	// VRAM data is 64 KB.
	if (out_pos != buf_siz) {
		vOut.clear();
		return Z_DATA_ERROR;
	}

	// Data was read successfully.
	vOut.resize(buf_siz);
	return 0;
}

/**
 * Decompress the data blocks.
 * @return 0 on success; non-zero on error.
 */
int SuperMagicDriveTest::decompress(void)
{
	int ret = decompress(m_bin_data, bin_data_gz, (unsigned int)sizeof(bin_data_gz));
	if (ret != 0)
		return ret;
	return decompress(m_smd_data, smd_data_gz, (unsigned int)sizeof(smd_data_gz));
}

/**
 * Test the standard SMD decoder.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_cpp_test)
{
	unique_ptr<uint8_t[]> buf(new uint8_t[SuperMagicDrive::SMD_BLOCK_SIZE]);
	SuperMagicDrive::decodeBlock_cpp(buf.get(), m_smd_data.data());
	EXPECT_EQ(0, memcmp(m_bin_data.data(), buf.get(), SuperMagicDrive::SMD_BLOCK_SIZE));
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: SuperMagicDrive tests.\n\n");
	fflush(nullptr);

	// Decompress the data blocks before running the tests.
	if (LibRomData::Tests::SuperMagicDriveTest::decompress() != 0) {
		fprintf(stderr, "*** FATAL ERROR: Could not decompress the test data.\n");
		return EXIT_FAILURE;
	}

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
