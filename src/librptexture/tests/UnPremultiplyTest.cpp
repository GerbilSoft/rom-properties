/***************************************************************************
 * ROM Properties Page shell extension. (librptexture/tests)               *
 * UnPremutiplyTest.cpp: Test un_premultiply().                            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"
#include "common.h"

// librptexture
#include "librptexture/ImageSizeCalc.hpp"
#include "librptexture/img/rp_image.hpp"
#ifdef _WIN32
// rp_image backend registration.
#  include "librptexture/img/RpGdiplusBackend.hpp"
#endif /* _WIN32 */
using namespace LibRpTexture;

// C includes.
#include <stdint.h>
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cstring>

// C++ includes.
#include <string>
using std::string;

namespace LibRpTexture { namespace Tests {

class UnPremultiplyTest : public ::testing::Test
{
	protected:
		UnPremultiplyTest()
			: m_img(new rp_image(512, 512, rp_image::Format::ARGB32))
		{
#ifdef _WIN32
			// Register RpGdiplusBackend.
			// TODO: Static initializer somewhere?
			rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
#endif /* _WIN32 */

			// Initialize the image with non-zero data.
			size_t sz = ImageSizeCalc::T_calcImageSize(m_img->row_bytes(), (m_img->height() - 1));
			sz += (m_img->width() * sizeof(uint32_t));
			memset(m_img->bits(), 0x55, sz);
		}

		~UnPremultiplyTest() override
		{
			m_img->unref();
		}

	public:
		// Number of iterations for benchmarks.
		static const unsigned int BENCHMARK_ITERATIONS = 1000;

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
		fputs("*** SSE4.1 is not supported on this CPU. Skipping test.\n", stderr);
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

/**
 * Benchmark the ImageDecoder::premultiply() function. (Standard version)
 */
TEST_F(UnPremultiplyTest, premultiply_cpp)
{
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
		m_img->premultiply();
	}
}

} }

/**
 * Test suite main function.
 * Called by gtest_init.cpp.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("LibRpTexture test suite: rp_image::un_premultiply() tests.\n\n", stderr);
	fprintf(stderr, "Benchmark iterations: %u\n",
		LibRpTexture::Tests::UnPremultiplyTest::BENCHMARK_ITERATIONS);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
