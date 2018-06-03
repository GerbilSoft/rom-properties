/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * UnPremutiplyTest.cpp: Test un_premultiply().                            *
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

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
//#include "librpbase/uvector.h"
#include "librpbase/aligned_malloc.h"

#include "librpbase/img/rp_image.hpp"

// C includes.
#include <stdint.h>
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
using std::unique_ptr;
using std::string;

namespace LibRpBase { namespace Tests {

class UnPremultiplyTest : public ::testing::Test
{
	protected:
		UnPremultiplyTest()
			: m_img(new rp_image(512, 512, rp_image::FORMAT_ARGB32))
		{
			// Initialize the image with non-zero data.
			size_t sz = m_img->row_bytes() * (m_img->height() - 1);
			sz += (m_img->width() * sizeof(uint32_t));
			memset(m_img->bits(), 0x55, sz);
		}

		~UnPremultiplyTest()
		{
			delete m_img;
		}

	public:
		// Number of iterations for benchmarks.
		static const unsigned int BENCHMARK_ITERATIONS = 10000;

		// Image.
		rp_image *m_img;
};

// TODO: Add actual tests to verify that un-premultiply works.
// Currently we only have benchmark tests.

/**
 * Benchmark the ImageDecoder::un_premultiply() function. (Standard version)
 */
TEST_F(UnPremultiplyTest, un_premultiply_cpp_benchmark)
{
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
		m_img->un_premultiply_cpp();
	}
}

#ifdef RP_IMAGE_HAS_SSE41
/**
 * Benchmark the ImageDecoder::un_premultiply() function. (SSE4.1-optimized version)
 */
TEST_F(UnPremultiplyTest, un_premultiply_sse41_benchmark)
{
	if (!RP_CPU_HasSSE41()) {
		fprintf(stderr, "*** SSE4.1 is not supported on this CPU. Skipping test.\n");
		return;
	}

	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
		m_img->un_premultiply_sse41();
	}
}
#endif /* RP_IMAGE_HAS_SSE41 */

// NOTE: Add more instruction sets to the #ifdef if other optimizations are added.
#if defined(RP_IMAGE_HAS_SSE41)
/**
 * Benchmark the ImageDecoder::un_premultiply() dispatch function.
 */
TEST_F(UnPremultiplyTest, un_premultiply_dispatch_benchmark)
{
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
		m_img->un_premultiply();
	}
}
#endif /* RP_IMAGE_HAS_SSE41 */

} }

/**
 * Test suite main function.
 * Called by gtest_init.c.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRpBase test suite: rp_image::un_premultiply() tests.\n\n");
	fprintf(stderr, "Benchmark iterations: %u\n",
		LibRpBase::Tests::UnPremultiplyTest::BENCHMARK_ITERATIONS);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
