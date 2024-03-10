/***************************************************************************
 * ROM Properties Page shell extension. (librptexture/tests)               *
 * ImageDecoderLinearTest.cpp: Linear image decoding tests with SSSE3.     *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"
#include "common.h"

// librpcpu, librpbase
#include "librpcpu/byteswap_rp.h"
#include "aligned_malloc.h"

// librptexture
#include "librptexture/img/rp_image.hpp"
#include "librptexture/decoder/ImageDecoder_Linear.hpp"
#ifdef _WIN32
// rp_image backend registration.
#  include "librptexture/img/RpGdiplusBackend.hpp"
#endif /* _WIN32 */
using namespace LibRpTexture;

// C includes
#include <stdint.h>
#include <stdlib.h>

// C includes (C++ namespace)
#include <cstring>

// C++ includes
#include <memory>
#include <string>
using std::string;

// Uninitialized vector class
#include "uvector.h"

namespace LibRpTexture { namespace Tests {

struct ImageDecoderLinearTest_mode
{
	/**
	 * Source/destination pixels always use a format like:
	 * - 12 34 56 78 (32-bit)
	 * - 00 12 34 56 (24-bit)
	 * - 56 34 12 00 (24-bit, swapped)
	 * The 00 is the byte to ignore.
	 *
	 * Note that for 24-bit and 32-bit xRGB,
	 * the destination alpha will always be 0xFF.
	 */

	uint32_t src_pixel;			// Source pixel.
	ImageDecoder::PixelFormat src_pxf;	// Source pixel format.
	int stride;				// Source stride. (0 for default)
	uint32_t dest_pixel;			// Expected decoded ARGB32 pixel.
	uint8_t bpp;				// Bits per pixel.

	ImageDecoderLinearTest_mode(
		uint32_t src_pixel,
		ImageDecoder::PixelFormat src_pxf,
		int stride,
		uint32_t dest_pixel,
		uint8_t bpp)
		: src_pixel(src_pixel)
		, src_pxf(src_pxf)
		, stride(stride)
		, dest_pixel(dest_pixel)
		, bpp(bpp)
	{ }
};

class ImageDecoderLinearTest : public ::testing::TestWithParam<ImageDecoderLinearTest_mode>
{
	protected:
		ImageDecoderLinearTest()
			: ::testing::TestWithParam<ImageDecoderLinearTest_mode>()
			, m_img_buf(nullptr)
			, m_img_buf_len(0)
		{
#ifdef _WIN32
			// Register RpGdiplusBackend.
			// TODO: Static initializer somewhere?
			rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
#endif /* _WIN32 */
		}

		~ImageDecoderLinearTest() override
		{
			aligned_free(m_img_buf);
		}

		void SetUp(void) final;
		void TearDown(void) final;

	public:
		/**
		 * Validate the pixels of an rp_image.
		 * All pixels should match dest_pixel.
		 * @param img		[in] rp_image.
		 * @param dest_pixel	[in] dest_pixel.
		 */
		static void Validate_RpImage(
			const rp_image *img,
			const uint32_t dest_pixel);

		// Number of iterations for benchmarks.
		static const unsigned int BENCHMARK_ITERATIONS = 100000;

	public:
		// Temporary image buffer
		// 128x128 24-bit or 32-bit image data.
		// FIXME: Use an aligned Allocator with rp::uvector<>.
		//rp::uvector<uint8_t> m_img_buf;
		uint8_t *m_img_buf;
		size_t m_img_buf_len;

		// Image
		rp_image_ptr m_img;

	public:
		/**
		 * Convert ImageDecoder::PixelFormat to string.
		 * @param pxf ImageDecoder::PixelFormat
		 * @return String. ("Unknown" on error)
		 */
		static const char *pxfToString(ImageDecoder::PixelFormat pxf);

		/**
		 * Test case suffix generator.
		 * @param info Test parameter information.
		 * @return Test case suffix.
		 */
		static string test_case_suffix_generator(const ::testing::TestParamInfo<ImageDecoderLinearTest_mode> &info);
};

/**
 * Convert ImageDecoder::PixelFormat to string.
 * @param pxf ImageDecoder::PixelFormat
 * @return String. ("Unknown" on error)
 */
const char *ImageDecoderLinearTest::pxfToString(ImageDecoder::PixelFormat pxf)
{
	#define CASE(pxf) \
		case ImageDecoder::PixelFormat::pxf: \
			mode_str = #pxf; \
			break;

	// TODO: Use a string table instead of switch/case.
	const char *mode_str;
	switch (pxf) {
		case ImageDecoder::PixelFormat::Unknown:
		default:
			assert(!"Unknown PixelFormat");
			mode_str = "Unknown";
			break;

		// 16-bit
		CASE(RGB565)
		CASE(BGR565)
		CASE(ARGB1555)
		CASE(ABGR1555)
		CASE(RGBA5551)
		CASE(BGRA5551)
		CASE(ARGB4444)
		CASE(ABGR4444)
		CASE(RGBA4444)
		CASE(BGRA4444)
		CASE(xRGB4444)
		CASE(xBGR4444)
		CASE(RGBx4444)
		CASE(BGRx4444)

		// Uncommon 16-bit formats.
		CASE(ARGB8332)

		// GameCube-specific 16-bit
		CASE(RGB5A3)
		CASE(IA8)

		// PlayStation 2-specific 16-bit
		CASE(BGR5A3)

		// 15-bit
		CASE(RGB555)
		CASE(BGR555)
		CASE(BGR555_PS1)

		// 24-bit
		CASE(RGB888)
		CASE(BGR888)

		// 32-bit with alpha channel.
		CASE(ARGB8888)
		CASE(ABGR8888)
		CASE(RGBA8888)
		CASE(BGRA8888)
		// 32-bit with unused alpha channel.
		CASE(xRGB8888)
		CASE(xBGR8888)
		CASE(RGBx8888)
		CASE(BGRx8888)

		// PlayStation 2-specific 32-bit
		CASE(BGR888_ABGR7888)

		// Uncommon 32-bit formats
		CASE(G16R16)
		CASE(A2R10G10B10)
		CASE(A2B10G10R10)
		CASE(RGB9_E5)

		// Uncommon 16-bit formats
		CASE(RG88)
		CASE(GR88)

		// VTFEdit uses this as "ARGB8888".
		// TODO: Might be a VTFEdit bug. (Tested versions: 1.2.5, 1.3.3)
		CASE(RABG8888)

		// Luminance formats.
		CASE(L8)
		CASE(A4L4)
		CASE(L16)
		CASE(A8L8)

		// Alpha
		CASE(A8)

		// Other
		CASE(R8)
	}

	return mode_str;
}

/**
 * Formatting function for ImageDecoderLinearTest.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const ImageDecoderLinearTest_mode& mode)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "0x%08X", mode.dest_pixel);
	return os << ImageDecoderLinearTest::pxfToString(mode.src_pxf) << '_' << buf;
};

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string ImageDecoderLinearTest::test_case_suffix_generator(const ::testing::TestParamInfo<ImageDecoderLinearTest_mode> &info)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "0x%08X", info.param.dest_pixel);

	string ret;
	ret.reserve(64);
	ret = ImageDecoderLinearTest::pxfToString(info.param.src_pxf);
	ret += '_';
	ret += buf;
	return ret;
}

/**
 * SetUp() function.
 * Run before each test.
 */
void ImageDecoderLinearTest::SetUp(void)
{
	if (::testing::UnitTest::GetInstance()->current_test_info()->value_param() == nullptr) {
		// Not a parameterized test.
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Create the 128x128 image data buffer.
	switch (mode.bpp) {
		case 24: {
			// 24-bit color.
			const int stride = (mode.stride > 0 ? mode.stride : 128*3);
			ASSERT_GE(stride, 128*3);
			aligned_free(m_img_buf);
			m_img_buf_len = 128*stride;
			m_img_buf = static_cast<uint8_t*>(aligned_malloc(16, m_img_buf_len));
			ASSERT_TRUE(m_img_buf != nullptr);

			uint8_t bytes[3];
			if (!(mode.src_pixel & 0xFF)) {
				// MSB aligned.
				bytes[0] = (mode.src_pixel >>  8) & 0xFF;
				bytes[1] = (mode.src_pixel >> 16) & 0xFF;
				bytes[2] = (mode.src_pixel >> 24) & 0xFF;
			} else {
				// LSB aligned.
				bytes[0] = (mode.src_pixel      ) & 0xFF;
				bytes[1] = (mode.src_pixel >>  8) & 0xFF;
				bytes[2] = (mode.src_pixel >> 16) & 0xFF;
			}

			uint8_t *p = m_img_buf;
			const int stride_adj = stride - 128*3;
			for (unsigned int y = 128; y > 0; y--) {
				for (unsigned int x = 128; x > 0; x--, p += 3) {
					p[0] = bytes[0];
					p[1] = bytes[1];
					p[2] = bytes[2];
				}
				p += stride_adj;
			}
			break;
		}

		case 32: {
			// 32-bit color.
			const int stride = (mode.stride > 0 ? mode.stride : 128*4);
			ASSERT_GE(stride, 128*4);
			aligned_free(m_img_buf);
			m_img_buf_len = 128*stride;
			m_img_buf = static_cast<uint8_t*>(aligned_malloc(16, m_img_buf_len));
			ASSERT_TRUE(m_img_buf != nullptr);

			uint32_t *p = reinterpret_cast<uint32_t*>(m_img_buf);
			const int stride_adj = (stride / sizeof(*p)) - 128;
			for (unsigned int y = 128; y > 0; y--) {
				for (unsigned int x = 128; x > 0; x -= 4, p += 4) {
					p[0] = mode.src_pixel;
					p[1] = mode.src_pixel;
					p[2] = mode.src_pixel;
					p[3] = mode.src_pixel;
				}
				p += stride_adj;
			}
			break;
		}

		case 15:
		case 16: {
			// 15/16-bit color.
			const int stride = (mode.stride > 0 ? mode.stride : 128*2);
			ASSERT_GE(stride, 128*2);
			aligned_free(m_img_buf);
			m_img_buf_len = 128*stride;
			m_img_buf = static_cast<uint8_t*>(aligned_malloc(16, m_img_buf_len));
			ASSERT_TRUE(m_img_buf != nullptr);

			uint16_t *p = reinterpret_cast<uint16_t*>(m_img_buf);
			const int stride_adj = (stride / sizeof(*p)) - 128;
			for (unsigned int y = 128; y > 0; y--) {
				for (unsigned int x = 128; x > 0; x -= 4, p += 4) {
					p[0] = static_cast<uint16_t>(mode.src_pixel);
					p[1] = static_cast<uint16_t>(mode.src_pixel);
					p[2] = static_cast<uint16_t>(mode.src_pixel);
					p[3] = static_cast<uint16_t>(mode.src_pixel);
				}
				p += stride_adj;
			}
			break;
		}

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}
}

/**
 * TearDown() function.
 * Run after each test.
 */
void ImageDecoderLinearTest::TearDown(void)
{
	m_img.reset();
}

/**
 * Validate the pixels of an rp_image.
 * All pixels should match dest_pixel.
 * @param img		[in] rp_image.
 * @param dest_pixel	[in] dest_pixel.
 */
void ImageDecoderLinearTest::Validate_RpImage(
	const rp_image *img,
	const uint32_t dest_pixel)
{
	ASSERT_TRUE(img->width() == 128);
	ASSERT_TRUE(img->height() == 128);
	ASSERT_TRUE(img->format() == rp_image::Format::ARGB32);

	const int width = img->width();
	const int height = img->height();
	for (int y = 0; y < height; y++) {
		const uint32_t *px = static_cast<const uint32_t*>(img->scanLine(y));
		for (int x = 0; x < width; x++, px++) {
			if (dest_pixel != *px) {
				printf("ERR: (%d,%d): expected %08X, got %08X\n",
					x, y, dest_pixel, *px);
			}
			ASSERT_EQ(dest_pixel, *px);
		}
	}
}

/**
 * Test the ImageDecoder::fromLinear*() functions. (Standard version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_cpp_test)
{
	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			m_img = ImageDecoder::fromLinear24_cpp(mode.src_pxf, 128, 128,
				m_img_buf, m_img_buf_len, mode.stride);
			break;

		case 32:
			// 32-bit image.
			m_img = ImageDecoder::fromLinear32_cpp(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint32_t*>(m_img_buf),
				m_img_buf_len, mode.stride);
			break;

		case 15:
		case 16:
			// 15/16-bit image.
			m_img = ImageDecoder::fromLinear16_cpp(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint16_t*>(m_img_buf),
				m_img_buf_len, mode.stride);
			break;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}

	ASSERT_TRUE((bool)m_img);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(m_img.get(), mode.dest_pixel));
}

/**
 * Benchmark the ImageDecoder::fromLinear*() functions. (Standard version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_cpp_benchmark)
{
	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear24_cpp(mode.src_pxf, 128, 128,
					m_img_buf, m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		case 32:
			// 32-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear32_cpp(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint32_t*>(m_img_buf),
					m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		case 15:
		case 16:
			// 15/16-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear16_cpp(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint16_t*>(m_img_buf),
					m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}
}

#ifdef IMAGEDECODER_HAS_SSE2
/**
 * Test the ImageDecoder::fromLinear*() functions. (SSE2-optimized version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_sse2_test)
{
	if (!RP_CPU_HasSSE2() && !GTEST_FLAG_GET(brief)) {
		fputs("*** SSE2 is not supported on this CPU. Skipping test.\n", stderr);
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	switch (mode.bpp) {
		case 24:
		case 32:
			// Not implemented...
			if (!GTEST_FLAG_GET(brief)) {
				fprintf(stderr, "*** SSE2 decoding is not implemented for %u-bit color.\n", mode.bpp);
			}
			return;

		case 15:
		case 16:
			// 15/16-bit image.
			m_img = ImageDecoder::fromLinear16_sse2(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint16_t*>(m_img_buf),
				m_img_buf_len, mode.stride);
			break;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}

	ASSERT_TRUE((bool)m_img);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(m_img.get(), mode.dest_pixel));
}

/**
 * Benchmark the ImageDecoder::fromLinear*() functions. (SSE2-optimized version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_sse2_benchmark)
{
	if (!RP_CPU_HasSSE2() && !GTEST_FLAG_GET(brief)) {
		fputs("*** SSE2 is not supported on this CPU. Skipping test.\n", stderr);
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	switch (mode.bpp) {
		case 24:
		case 32:
			// Not implemented...
			if (!GTEST_FLAG_GET(brief)) {
				fprintf(stderr, "*** SSE2 decoding is not implemented for %u-bit color.\n", mode.bpp);
			}
			return;

		case 15:
		case 16:
			// 15/16-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear16_sse2(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint16_t*>(m_img_buf),
					m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}
}
#endif /* IMAGEDECODER_HAS_SSE2 */

#ifdef IMAGEDECODER_HAS_SSSE3
/**
 * Test the ImageDecoder::fromLinear*() functions. (SSSE3-optimized version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_ssse3_test)
{
	if (!RP_CPU_HasSSSE3() && !GTEST_FLAG_GET(brief)) {
		fprintf(stderr, "*** SSSE3 is not supported on this CPU. Skipping test.\n");
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			m_img = ImageDecoder::fromLinear24_ssse3(mode.src_pxf, 128, 128,
				m_img_buf, m_img_buf_len, mode.stride);
			break;

		case 32:
			// 32-bit image.
			m_img = ImageDecoder::fromLinear32_ssse3(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint32_t*>(m_img_buf),
				m_img_buf_len, mode.stride);
			break;

		case 15:
		case 16:
			// Not implemented...
			if (!GTEST_FLAG_GET(brief)) {
				fprintf(stderr, "*** SSSE3 decoding is not implemented for %u-bit color.\n", mode.bpp);
			}
			return;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}

	ASSERT_TRUE((bool)m_img);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(m_img.get(), mode.dest_pixel));
}

/**
 * Benchmark the ImageDecoder::fromLinear*() functions. (SSSE3-optimized version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_ssse3_benchmark)
{
	if (!RP_CPU_HasSSSE3() && !GTEST_FLAG_GET(brief)) {
		fprintf(stderr, "*** SSSE3 is not supported on this CPU. Skipping test.\n");
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear24_ssse3(mode.src_pxf, 128, 128,
					m_img_buf, m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		case 32:
			// 32-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear32_ssse3(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint32_t*>(m_img_buf),
					m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		case 15:
		case 16:
			// Not implemented...
			if (!GTEST_FLAG_GET(brief)) {
				fprintf(stderr, "*** SSSE3 decoding is not implemented for %u-bit color.\n", mode.bpp);
			}
			return;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}
}
#endif /* IMAGEDECODER_HAS_SSSE3 */

// NOTE: Add more instruction sets to the #ifdef if other optimizations are added.
#if defined(IMAGEDECODER_HAS_SSE2) || defined(IMAGEDECODER_HAS_SSSE3)
/**
 * Test the ImageDecoder::fromLinear*() dispatch functions.
 */
TEST_P(ImageDecoderLinearTest, fromLinear_dispatch_test)
{
	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			m_img = ImageDecoder::fromLinear24(mode.src_pxf, 128, 128,
				m_img_buf, m_img_buf_len, mode.stride);
			break;

		case 32:
			// 32-bit image.
			m_img = ImageDecoder::fromLinear32(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint32_t*>(m_img_buf),
				m_img_buf_len, mode.stride);
			break;

		case 15:
		case 16:
			// 15/16-bit image.
			m_img = ImageDecoder::fromLinear16(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint16_t*>(m_img_buf),
				m_img_buf_len, mode.stride);
			return;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}

	ASSERT_TRUE((bool)m_img);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(m_img.get(), mode.dest_pixel));
}

/**
 * Benchmark the ImageDecoder::fromLinear*() dispatch functions.
 */
TEST_P(ImageDecoderLinearTest, fromLinear_dispatch_benchmark)
{
	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear24(mode.src_pxf, 128, 128,
					m_img_buf, m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		case 32:
			// 32-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear32(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint32_t*>(m_img_buf),
					m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		case 15:
		case 16:
			// 15/16-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				m_img = ImageDecoder::fromLinear16(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint16_t*>(m_img_buf),
					m_img_buf_len, mode.stride);
				m_img.reset();
			}
			break;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}
}
#endif /* IMAGEDECODER_HAS_SSE2 || IMAGEDECODER_HAS_SSSE3 */

// Test cases.

// 32-bit tests.
INSTANTIATE_TEST_SUITE_P(fromLinear32, ImageDecoderLinearTest,
	::testing::Values(
		// ARGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::ARGB8888,
			0,
			0x12345678,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PixelFormat::ABGR8888,
			0,
			0x12345678,
			32),

		// xRGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::xRGB8888,
			0,
			0xFF345678,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PixelFormat::xBGR8888,
			0,
			0xFF345678,
			32),

		// 30-bit RGB with 2-bit alpha (alpha == 00)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::A2R10G10B10,
			0,
			0x0048459E,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::A2B10G10R10,
			0,
			0x009E4548,
			32),

		// 30-bit RGB with 2-bit alpha (alpha == 10)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x92345678),
			ImageDecoder::PixelFormat::A2R10G10B10,
			0,
			0xAA48459E,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x92345678),
			ImageDecoder::PixelFormat::A2B10G10R10,
			0,
			0xAA9E4548,
			32),

		// PixelFormat::RABG8888 (Valve VTF ARGB8888)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::RABG8888,
			0,
			0x34127856,
			32))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 32-bit tests. (custom stride)
INSTANTIATE_TEST_SUITE_P(fromLinear32_stride640, ImageDecoderLinearTest,
	::testing::Values(
		// ARGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::ARGB8888,
			640,
			0x12345678,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PixelFormat::ABGR8888,
			640,
			0x12345678,
			32),

		// xRGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::xRGB8888,
			640,
			0xFF345678,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PixelFormat::xBGR8888,
			640,
			0xFF345678,
			32),

		// 30-bit RGB with 2-bit alpha (alpha == 00)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::A2R10G10B10,
			640,
			0x0048459E,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::A2B10G10R10,
			640,
			0x009E4548,
			32),

		// 30-bit RGB with 2-bit alpha (alpha == 10)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x92345678),
			ImageDecoder::PixelFormat::A2R10G10B10,
			640,
			0xAA48459E,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x92345678),
			ImageDecoder::PixelFormat::A2B10G10R10,
			640,
			0xAA9E4548,
			32),

		// PixelFormat::RABG8888 (Valve VTF ARGB8888)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PixelFormat::RABG8888,
			640,
			0x34127856,
			32))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 24-bit tests.
INSTANTIATE_TEST_SUITE_P(fromLinear24, ImageDecoderLinearTest,
	::testing::Values(
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x123456),
			ImageDecoder::PixelFormat::RGB888,
			0,
			0xFF123456,
			24),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x563412),
			ImageDecoder::PixelFormat::BGR888,
			0,
			0xFF123456,
			24))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 24-bit tests. (custom stride)
INSTANTIATE_TEST_SUITE_P(fromLinear24_stride512, ImageDecoderLinearTest,
	::testing::Values(
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x123456),
			ImageDecoder::PixelFormat::RGB888,
			512,
			0xFF123456,
			24),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x563412),
			ImageDecoder::PixelFormat::BGR888,
			512,
			0xFF123456,
			24))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 15/16-bit tests.
INSTANTIATE_TEST_SUITE_P(fromLinear16, ImageDecoderLinearTest,
	::testing::Values(
		/** 16-bit **/
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::RGB565,
			0,
			0xFF1045A5,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0xA222),
			ImageDecoder::PixelFormat::BGR565,
			0,
			0xFF1045A5,
			16),

		// ARGB4444
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::ARGB4444,
			0,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1432),
			ImageDecoder::PixelFormat::ABGR4444,
			0,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x2341),
			ImageDecoder::PixelFormat::RGBA4444,
			0,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x4321),
			ImageDecoder::PixelFormat::BGRA4444,
			0,
			0x11223344,
			16),

		// xRGB4444
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::xRGB4444,
			0,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1432),
			ImageDecoder::PixelFormat::xBGR4444,
			0,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x2341),
			ImageDecoder::PixelFormat::RGBx4444,
			0,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x4321),
			ImageDecoder::PixelFormat::BGRx4444,
			0,
			0xFF223344,
			16),

		// ARGB1555
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::ARGB1555,
			0,
			0x00218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x5224),
			ImageDecoder::PixelFormat::ABGR1555,
			0,
			0x00218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x9234),
			ImageDecoder::PixelFormat::ARGB1555,
			0,
			0xFF218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0xD224),
			ImageDecoder::PixelFormat::ABGR1555,
			0,
			0xFF218CA5,
			16),

		// RGBA1555
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x4320),
			ImageDecoder::PixelFormat::RGBA5551,
			0,
			0x00426384,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x8310),
			ImageDecoder::PixelFormat::BGRA5551,
			0,
			0x00426384,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x4321),
			ImageDecoder::PixelFormat::RGBA5551,
			0,
			0xFF426384,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x8311),
			ImageDecoder::PixelFormat::BGRA5551,
			0,
			0xFF426384,
			16),

		// RG88
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::RG88,
			0,
			0xFF123400,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x3412),
			ImageDecoder::PixelFormat::GR88,
			0,
			0xFF123400,
			16),

		/** 15-bit **/
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::RGB555,
			0,
			0xFF218CA5,
			15),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x5224),
			ImageDecoder::PixelFormat::BGR555,
			0,
			0xFF218CA5,
			15))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 15/16-bit tests. (custom stride)
INSTANTIATE_TEST_SUITE_P(fromLinear16_384, ImageDecoderLinearTest,
	::testing::Values(
		/** 16-bit **/
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::RGB565,
			384,
			0xFF1045A5,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0xA222),
			ImageDecoder::PixelFormat::BGR565,
			384,
			0xFF1045A5,
			16),

		// ARGB4444
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::ARGB4444,
			384,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1432),
			ImageDecoder::PixelFormat::ABGR4444,
			384,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x2341),
			ImageDecoder::PixelFormat::RGBA4444,
			384,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x4321),
			ImageDecoder::PixelFormat::BGRA4444,
			384,
			0x11223344,
			16),

		// xRGB4444
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::xRGB4444,
			384,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1432),
			ImageDecoder::PixelFormat::xBGR4444,
			384,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x2341),
			ImageDecoder::PixelFormat::RGBx4444,
			384,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x4321),
			ImageDecoder::PixelFormat::BGRx4444,
			384,
			0xFF223344,
			16),

		// ARGB1555
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::ARGB1555,
			384,
			0x00218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x5224),
			ImageDecoder::PixelFormat::ABGR1555,
			384,
			0x00218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x9234),
			ImageDecoder::PixelFormat::ARGB1555,
			384,
			0xFF218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0xD224),
			ImageDecoder::PixelFormat::ABGR1555,
			384,
			0xFF218CA5,
			16),

		// RGBA1555
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x4320),
			ImageDecoder::PixelFormat::RGBA5551,
			384,
			0x00426384,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x8310),
			ImageDecoder::PixelFormat::BGRA5551,
			384,
			0x00426384,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x4321),
			ImageDecoder::PixelFormat::RGBA5551,
			384,
			0xFF426384,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x8311),
			ImageDecoder::PixelFormat::BGRA5551,
			384,
			0xFF426384,
			16),

		// RG88
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::RG88,
			384,
			0xFF123400,
			16),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x3412),
			ImageDecoder::PixelFormat::GR88,
			384,
			0xFF123400,
			16),

		/** 15-bit **/
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x1234),
			ImageDecoder::PixelFormat::RGB555,
			384,
			0xFF218CA5,
			15),
		ImageDecoderLinearTest_mode(
			le16_to_cpu(0x5224),
			ImageDecoder::PixelFormat::BGR555,
			384,
			0xFF218CA5,
			15))
	, ImageDecoderLinearTest::test_case_suffix_generator);

} }

/**
 * Test suite main function.
 * Called by gtest_init.cpp.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("LibRpTexture test suite: ImageDecoder::fromLinear*() tests.\n\n", stderr);
	fprintf(stderr, "Benchmark iterations: %u\n",
		LibRpTexture::Tests::ImageDecoderLinearTest::BENCHMARK_ITERATIONS);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
