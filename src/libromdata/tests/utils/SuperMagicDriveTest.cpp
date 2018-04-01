/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * SuperMagicDriveTest.cpp: SuperMagicDrive class test.                    *
 *                                                                         *
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

// Google Test
#include "gtest/gtest.h"

// SuperMagicDrive
#include "libromdata/utils/SuperMagicDrive.hpp"
#include "librpbase/aligned_malloc.h"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <memory>
using std::unique_ptr;

// zlib
#define CHUNK 4096
#include <zlib.h>

namespace LibRomData { namespace Tests {

class SuperMagicDriveTest : public ::testing::Test
{
	protected:
		SuperMagicDriveTest()
			: align_buf(nullptr)
		{ }

	public:
		// Output block size. (+64 for zlib)
		static const unsigned int OUT_BLOCK_UNZ_SIZE = SuperMagicDrive::SMD_BLOCK_SIZE+64;

		/**
		 * 16 KB plain binary data block.
		 */
		static const uint8_t bin_data_gz[11811];

		// Number of iterations for benchmarks.
		static const unsigned int BENCHMARK_ITERATIONS = 100000;

		/**
		 * 16 KB SMD-interleaved data block.
		 */
		static const uint8_t smd_data_gz[403];

	public:
		// Uncompressed data buffers.
		static uint8_t *m_bin_data;
		static uint8_t *m_smd_data;

	private:
		/**
		 * Decompress a data block.
		 * @param pOut		[out] Output buffer. (Must be 16 KB.)
		 * @param out_len	[in] Output buffer length.
		 * @param pIn		[in] Input array.
		 * @param in_len	[in] Input length.
		 * @return 0 on success; non-zero on error.
		 */
		static int decompress(uint8_t *RESTRICT pOut, unsigned int out_len, const uint8_t *RESTRICT pIn, unsigned int in_len);

	public:
		/**
		 * Decompress the data blocks.
		 * @return 0 on success; non-zero on error.
		 */
		static int decompress(void);

	public:
		void SetUp(void) final;
		void TearDown(void) final;

	public:
		// Temporary aligned memory buffer.
		// Automatically freed in teardown().
		uint8_t *align_buf;
};

// Test data is in SuperMagicDriveTest_data.hpp.
#include "SuperMagicDriveTest_data.hpp"

// Uncompressed data buffers.
uint8_t *SuperMagicDriveTest::m_bin_data = nullptr;
uint8_t *SuperMagicDriveTest::m_smd_data = nullptr;

/**
 * SetUp() function.
 * Run before each test.
 */
void SuperMagicDriveTest::SetUp(void)
{
	align_buf = static_cast<uint8_t*>(aligned_malloc(16, SuperMagicDrive::SMD_BLOCK_SIZE));
	ASSERT_TRUE(align_buf != nullptr);
}

/**
 * TearDown() function.
 * Run after each test.
 */
void SuperMagicDriveTest::TearDown(void)
{
	aligned_free(align_buf);
	align_buf = nullptr;
}

/**
 * Decompress a data block.
 * @param pOut		[out] Output buffer. (Must be 16 KB.)
 * @param out_len	[in] Output buffer length.
 * @param pIn		[in] Input array.
 * @param in_len	[in] Input length.
 * @return 0 on success; non-zero on error.
 */
int SuperMagicDriveTest::decompress(uint8_t *pOut, unsigned int out_len, const uint8_t *pIn, unsigned int in_len)
{
	// Based on zlib example code:
	// http://www.zlib.net/zlib_how.html
	int ret;
	z_stream strm;

	const unsigned int buf_siz = SuperMagicDrive::SMD_BLOCK_SIZE;
	assert(out_len >= OUT_BLOCK_UNZ_SIZE);
	if (out_len < OUT_BLOCK_UNZ_SIZE) {
		// Output buffer is too small.
		return Z_MEM_ERROR;
	}
	unsigned int out_pos = 0;

	// Allocate the zlib inflate state.
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = inflateInit2(&strm, 15+16);
	if (ret != Z_OK) {
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
			strm.next_out = &pOut[out_pos];

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
		return Z_DATA_ERROR;
	}

	// VRAM data is 64 KB.
	if (out_pos != buf_siz) {
		return Z_DATA_ERROR;
	}

	// Data was read successfully.
	return 0;
}

/**
 * Decompress the data blocks.
 * @return 0 on success; non-zero on error.
 */
int SuperMagicDriveTest::decompress(void)
{
	m_bin_data = static_cast<uint8_t*>(aligned_malloc(16, OUT_BLOCK_UNZ_SIZE));
	int ret = decompress(m_bin_data, OUT_BLOCK_UNZ_SIZE, bin_data_gz, (unsigned int)sizeof(bin_data_gz));
	if (ret != 0) {
		aligned_free(m_bin_data);
		m_bin_data = nullptr;
		return ret;
	}

	m_smd_data = static_cast<uint8_t*>(aligned_malloc(16, OUT_BLOCK_UNZ_SIZE));
	ret = decompress(m_smd_data, OUT_BLOCK_UNZ_SIZE, smd_data_gz, (unsigned int)sizeof(smd_data_gz));
	if (ret != 0) {
		aligned_free(m_smd_data);
		m_smd_data = nullptr;
	}
	return ret;
}

/**
 * Test the standard SMD decoder.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_cpp_test)
{
	SuperMagicDrive::decodeBlock_cpp(align_buf, m_smd_data);
	EXPECT_EQ(0, memcmp(m_bin_data, align_buf, SuperMagicDrive::SMD_BLOCK_SIZE));
}

/**
 * Benchmark the standard SMD decoder.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_cpp_benchmark)
{
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
		SuperMagicDrive::decodeBlock_cpp(align_buf, m_smd_data);
	}
}

#ifdef SMD_HAS_MMX
/**
 * Test the MMX-optimized SMD decoder.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_mmx_test)
{
	if (!RP_CPU_HasMMX()) {
		fprintf(stderr, "*** MMX is not supported on this CPU. Skipping test.");
		return;
	}

	SuperMagicDrive::decodeBlock_mmx(align_buf, m_smd_data);
	EXPECT_EQ(0, memcmp(m_bin_data, align_buf, SuperMagicDrive::SMD_BLOCK_SIZE));
}

/**
 * Benchmark the MMX-optimized SMD decoder.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_mmx_benchmark)
{
	if (!RP_CPU_HasMMX()) {
		fprintf(stderr, "*** MMX is not supported on this CPU. Skipping test.");
		return;
	}

	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
		SuperMagicDrive::decodeBlock_mmx(align_buf, m_smd_data);
	}
}
#endif /* SMD_HAS_MMX */

#ifdef SMD_HAS_SSE2
/**
 * Test the SSE2-optimized SMD decoder.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_sse2_test)
{
	if (!RP_CPU_HasSSE2()) {
		fprintf(stderr, "*** SSE2 is not supported on this CPU. Skipping test.");
		return;
	}

	SuperMagicDrive::decodeBlock_sse2(align_buf, m_smd_data);
	EXPECT_EQ(0, memcmp(m_bin_data, align_buf, SuperMagicDrive::SMD_BLOCK_SIZE));
}

/**
 * Benchmark the SSE2-optimized SMD decoder.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_sse2_benchmark)
{
	if (!RP_CPU_HasSSE2()) {
		fprintf(stderr, "*** SSE2 is not supported on this CPU. Skipping test.");
		return;
	}

	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
		SuperMagicDrive::decodeBlock_sse2(align_buf, m_smd_data);
	}
}
#endif /* SMD_HAS_SSE2 */

// NOTE: Add more instruction sets to the #ifdef if other optimizations are added.
#if defined(SMD_HAS_MMX) || defined(SMD_HAS_SSE2)
/**
 * Test the decodeBlock() dispatch function.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_dispatch_test)
{
	SuperMagicDrive::decodeBlock(align_buf, m_smd_data);
	EXPECT_EQ(0, memcmp(m_bin_data, align_buf, SuperMagicDrive::SMD_BLOCK_SIZE));
}

/**
 * Benchmark the decodeBlock() dispatch function.
 */
TEST_F(SuperMagicDriveTest, decodeBlock_dispatch_benchmark)
{
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
		SuperMagicDrive::decodeBlock(align_buf, m_smd_data);
	}
}
#endif /* SMD_HAS_MMX || SMD_HAS_SSE2 */

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: SuperMagicDrive tests.\n\n");
	fprintf(stderr, "Benchmark iterations: %u\n", LibRomData::Tests::SuperMagicDriveTest::BENCHMARK_ITERATIONS);
	fflush(nullptr);

	// Decompress the data blocks before running the tests.
	if (LibRomData::Tests::SuperMagicDriveTest::decompress() != 0) {
		fprintf(stderr, "*** FATAL ERROR: Could not decompress the test data.\n");
		return EXIT_FAILURE;
	}

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();

	// Free the allocated data blocks.
	aligned_free(LibRomData::Tests::SuperMagicDriveTest::m_bin_data);
	aligned_free(LibRomData::Tests::SuperMagicDriveTest::m_smd_data);
	return ret;
}
