/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * ImageDecoderLinearTest.cpp: Linear image decoding tests with SSSE3.     *
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
#include "librpbase/img/ImageDecoder.hpp"

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

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRpBase { namespace Tests {

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
		{ }

		~ImageDecoderLinearTest()
		{
			aligned_free(m_img_buf);
		}

		void SetUp(void) final;

	public:
		/**
		 * Validate the pixels of an rp_image.
		 * All pixels should match dest_pixel.
		 * @param pImg		[in] rp_image.
		 * @param dest_pixel	[in] dest_pixel.
		 */
		static void Validate_RpImage(
			const rp_image *pImg,
			const uint32_t dest_pixel);

		// Number of iterations for benchmarks.
		static const unsigned int BENCHMARK_ITERATIONS = 100000;

	public:
		// Temporary image buffer.
		// 128x128 24-bit or 32-bit image data.
		// FIXME: Use an aligned Allocator with ao::uvector<>.
		//ao::uvector<uint8_t> m_img_buf;
		uint8_t *m_img_buf;
		size_t m_img_buf_len;

	public:
		/**
		 * Convert ImageDecoder::PixelFormat to string.
		 * @param pxf ImageDecoder::PixelFormat
		 * @return String. ("PXF_UNKNOWN" on error)
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
 * @return String. ("PXF_UNKNOWN" on error)
 */
const char *ImageDecoderLinearTest::pxfToString(ImageDecoder::PixelFormat pxf)
{
	#define CASE(pxf) \
		case ImageDecoder::pxf: \
			mode_str = #pxf; \
			break;

	// TODO: Use a string table instead of switch/case.
	const char *mode_str;
	switch (pxf) {
		case ImageDecoder::PXF_UNKNOWN:
		default:
			mode_str = "PXF_UNKNOWN";
			break;

		// 16-bit
		CASE(PXF_RGB565)
		CASE(PXF_BGR565)
		CASE(PXF_ARGB1555)
		CASE(PXF_ABGR1555)
		CASE(PXF_RGBA5551)
		CASE(PXF_BGRA5551)
		CASE(PXF_ARGB4444)
		CASE(PXF_ABGR4444)
		CASE(PXF_RGBA4444)
		CASE(PXF_BGRA4444)
		CASE(PXF_xRGB4444)
		CASE(PXF_xBGR4444)
		CASE(PXF_RGBx4444)
		CASE(PXF_BGRx4444)

		// Uncommon 16-bit formats.
		CASE(PXF_ARGB8332)

		// GameCube-specific 16-bit
		CASE(PXF_RGB5A3)
		CASE(PXF_IA8)

		// 15-bit
		CASE(PXF_RGB555)
		CASE(PXF_BGR555)
		CASE(PXF_BGR555_PS1)

		// 24-bit
		CASE(PXF_RGB888)
		CASE(PXF_BGR888)

		// 32-bit with alpha channel.
		CASE(PXF_ARGB8888)
		CASE(PXF_ABGR8888)
		CASE(PXF_RGBA8888)
		CASE(PXF_BGRA8888)
		// 32-bit with unused alpha channel.
		CASE(PXF_xRGB8888)
		CASE(PXF_xBGR8888)
		CASE(PXF_RGBx8888)
		CASE(PXF_BGRx8888)

		// Uncommon 32-bit formats.
		CASE(PXF_G16R16)
		CASE(PXF_A2R10G10B10)
		CASE(PXF_A2B10G10R10)

		// Uncommon 16-bit formats.
		CASE(PXF_RG88)
		CASE(PXF_GR88)

		// VTFEdit uses this as "ARGB8888".
		// TODO: Might be a VTFEdit bug. (Tested versions: 1.2.5, 1.3.3)
		CASE(PXF_RABG8888)

		// Luminance formats.
		CASE(PXF_L8)
		CASE(PXF_A4L4)
		CASE(PXF_L16)
		CASE(PXF_A8L8)

		// Alpha formats.
		CASE(PXF_A8)
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
					p[0] = (uint16_t)mode.src_pixel;
					p[1] = (uint16_t)mode.src_pixel;
					p[2] = (uint16_t)mode.src_pixel;
					p[3] = (uint16_t)mode.src_pixel;
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
 * Validate the pixels of an rp_image.
 * All pixels should match dest_pixel.
 * @param pImg		[in] rp_image.
 * @param dest_pixel	[in] dest_pixel.
 */
void ImageDecoderLinearTest::Validate_RpImage(
	const rp_image *pImg,
	const uint32_t dest_pixel)
{
	ASSERT_TRUE(pImg->width() == 128);
	ASSERT_TRUE(pImg->height() == 128);
	ASSERT_TRUE(pImg->format() == rp_image::FORMAT_ARGB32);

	const int width = pImg->height();
	const int height = pImg->height();
	for (int y = 0; y < height; y++) {
		const uint32_t *px = reinterpret_cast<const uint32_t*>(pImg->scanLine(y));
		for (int x = 0; x < width; x++, px++) {
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
	unique_ptr<rp_image> pImg;
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			pImg.reset(ImageDecoder::fromLinear24_cpp(mode.src_pxf, 128, 128,
				m_img_buf, (int)m_img_buf_len, mode.stride));
			break;

		case 32:
			// 32-bit image.
			pImg.reset(ImageDecoder::fromLinear32_cpp(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint32_t*>(m_img_buf),
				(int)m_img_buf_len, mode.stride));
			break;

		case 15:
		case 16:
			// 15/16-bit image.
			pImg.reset(ImageDecoder::fromLinear16_cpp(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint16_t*>(m_img_buf),
				(int)m_img_buf_len, mode.stride));
			break;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}

	ASSERT_TRUE(pImg.get() != nullptr);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(pImg.get(), mode.dest_pixel));
}

/**
 * Benchmark the ImageDecoder::fromLinear*() functions. (Standard version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_cpp_benchmark)
{
	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	unique_ptr<rp_image> pImg;
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear24_cpp(mode.src_pxf, 128, 128,
					m_img_buf, (int)m_img_buf_len, mode.stride));
			}
			break;

		case 32:
			// 32-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear32_cpp(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint32_t*>(m_img_buf),
					(int)m_img_buf_len, mode.stride));
			}
			break;

		case 15:
		case 16:
			// 15/16-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear16_cpp(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint16_t*>(m_img_buf),
					(int)m_img_buf_len, mode.stride));
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
	if (!RP_CPU_HasSSE2()) {
		fprintf(stderr, "*** SSE2 is not supported on this CPU. Skipping test.\n");
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	unique_ptr<rp_image> pImg;
	switch (mode.bpp) {
		case 24:
		case 32:
			// Not implemented...
			fprintf(stderr, "*** SSE2 decoding is not implemented for %u-bit color.\n", mode.bpp);
			return;

		case 15:
		case 16:
			// 15/16-bit image.
			pImg.reset(ImageDecoder::fromLinear16_sse2(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint16_t*>(m_img_buf),
				(int)m_img_buf_len, mode.stride));
			break;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}

	ASSERT_TRUE(pImg.get() != nullptr);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(pImg.get(), mode.dest_pixel));
}

/**
 * Benchmark the ImageDecoder::fromLinear*() functions. (SSE2-optimized version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_sse2_benchmark)
{
	if (!RP_CPU_HasSSE2()) {
		fprintf(stderr, "*** SSE2 is not supported on this CPU. Skipping test.\n");
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	unique_ptr<rp_image> pImg;
	switch (mode.bpp) {
		case 24:
		case 32:
			// Not implemented...
			fprintf(stderr, "*** SSE2 decoding is not implemented for %u-bit color.\n", mode.bpp);
			return;

		case 15:
		case 16:
			// 15/16-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear16_sse2(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint16_t*>(m_img_buf),
					(int)m_img_buf_len, mode.stride));
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
	if (!RP_CPU_HasSSSE3()) {
		fprintf(stderr, "*** SSSE3 is not supported on this CPU. Skipping test.\n");
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	unique_ptr<rp_image> pImg;
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			pImg.reset(ImageDecoder::fromLinear24_ssse3(mode.src_pxf, 128, 128,
				m_img_buf, (int)m_img_buf_len, mode.stride));
			break;

		case 32:
			// 32-bit image.
			pImg.reset(ImageDecoder::fromLinear32_ssse3(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint32_t*>(m_img_buf),
				(int)m_img_buf_len, mode.stride));
			break;

		case 15:
		case 16:
			// Not implemented...
			fprintf(stderr, "*** SSSE3 decoding is not implemented for %u-bit color.\n", mode.bpp);
			return;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}

	ASSERT_TRUE(pImg.get() != nullptr);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(pImg.get(), mode.dest_pixel));
}

/**
 * Benchmark the ImageDecoder::fromLinear*() functions. (SSSE3-optimized version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_ssse3_benchmark)
{
	if (!RP_CPU_HasSSSE3()) {
		fprintf(stderr, "*** SSSE3 is not supported on this CPU. Skipping test.\n");
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	unique_ptr<rp_image> pImg;
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear24_ssse3(mode.src_pxf, 128, 128,
					m_img_buf, (int)m_img_buf_len, mode.stride));
			}
			break;

		case 32:
			// 32-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear32_ssse3(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint32_t*>(m_img_buf),
					(int)m_img_buf_len, mode.stride));
			}
			break;

		case 15:
		case 16:
			// Not implemented...
			fprintf(stderr, "*** SSSE3 decoding is not implemented for %u-bit color.\n", mode.bpp);
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
	unique_ptr<rp_image> pImg;
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			pImg.reset(ImageDecoder::fromLinear24(mode.src_pxf, 128, 128,
				m_img_buf, (int)m_img_buf_len, mode.stride));
			break;

		case 32:
			// 32-bit image.
			pImg.reset(ImageDecoder::fromLinear32(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint32_t*>(m_img_buf),
				(int)m_img_buf_len, mode.stride));
			break;

		case 15:
		case 16:
			// 15/16-bit image.
			pImg.reset(ImageDecoder::fromLinear16(mode.src_pxf, 128, 128,
				reinterpret_cast<const uint16_t*>(m_img_buf),
				(int)m_img_buf_len, mode.stride));
			return;

		default:
			ASSERT_TRUE(false) << "Invalid bpp: " << mode.bpp;
			return;
	}

	ASSERT_TRUE(pImg.get() != nullptr);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(pImg.get(), mode.dest_pixel));
}

/**
 * Benchmark the ImageDecoder::fromLinear*() dispatch functions.
 */
TEST_P(ImageDecoderLinearTest, fromLinear_dispatch_benchmark)
{
	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	unique_ptr<rp_image> pImg;
	switch (mode.bpp) {
		case 24:
			// 24-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear24(mode.src_pxf, 128, 128,
					m_img_buf, (int)m_img_buf_len, mode.stride));
			}
			break;

		case 32:
			// 32-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear32(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint32_t*>(m_img_buf),
					(int)m_img_buf_len, mode.stride));
			}
			break;

		case 15:
		case 16:
			// 15/16-bit image.
			for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) {
				pImg.reset(ImageDecoder::fromLinear16(mode.src_pxf, 128, 128,
					reinterpret_cast<const uint16_t*>(m_img_buf),
					(int)m_img_buf_len, mode.stride));
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
INSTANTIATE_TEST_CASE_P(fromLinear32, ImageDecoderLinearTest,
	::testing::Values(
		// ARGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_ARGB8888,
			0,
			0x12345678,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PXF_ABGR8888,
			0,
			0x12345678,
			32),

		// xRGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_xRGB8888,
			0,
			0xFF345678,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PXF_xBGR8888,
			0,
			0xFF345678,
			32),

		// 30-bit RGB with 2-bit alpha (alpha == 00)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_A2R10G10B10,
			0,
			0x0048459E,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_A2B10G10R10,
			0,
			0x009E4548,
			32),

		// 30-bit RGB with 2-bit alpha (alpha == 10)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x92345678),
			ImageDecoder::PXF_A2R10G10B10,
			0,
			0xAA48459E,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x92345678),
			ImageDecoder::PXF_A2B10G10R10,
			0,
			0xAA9E4548,
			32),

		// PXF_RABG8888 (Valve VTF ARGB8888)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_RABG8888,
			0,
			0x34127856,
			32))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 32-bit tests. (custom stride)
INSTANTIATE_TEST_CASE_P(fromLinear32_stride640, ImageDecoderLinearTest,
	::testing::Values(
		// ARGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_ARGB8888,
			640,
			0x12345678,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PXF_ABGR8888,
			640,
			0x12345678,
			32),

		// xRGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_xRGB8888,
			640,
			0xFF345678,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PXF_xBGR8888,
			640,
			0xFF345678,
			32),

		// 30-bit RGB with 2-bit alpha (alpha == 00)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_A2R10G10B10,
			640,
			0x0048459E,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_A2B10G10R10,
			640,
			0x009E4548,
			32),

		// 30-bit RGB with 2-bit alpha (alpha == 10)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x92345678),
			ImageDecoder::PXF_A2R10G10B10,
			640,
			0xAA48459E,
			32),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x92345678),
			ImageDecoder::PXF_A2B10G10R10,
			640,
			0xAA9E4548,
			32),

		// PXF_RABG8888 (Valve VTF ARGB8888)
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_RABG8888,
			640,
			0x34127856,
			32))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 24-bit tests.
INSTANTIATE_TEST_CASE_P(fromLinear24, ImageDecoderLinearTest,
	::testing::Values(
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x123456),
			ImageDecoder::PXF_RGB888,
			0,
			0xFF123456,
			24),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x563412),
			ImageDecoder::PXF_BGR888,
			0,
			0xFF123456,
			24))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 24-bit tests. (custom stride)
INSTANTIATE_TEST_CASE_P(fromLinear24_stride512, ImageDecoderLinearTest,
	::testing::Values(
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x123456),
			ImageDecoder::PXF_RGB888,
			512,
			0xFF123456,
			24),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x563412),
			ImageDecoder::PXF_BGR888,
			512,
			0xFF123456,
			24))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 15/16-bit tests.
INSTANTIATE_TEST_CASE_P(fromLinear16, ImageDecoderLinearTest,
	::testing::Values(
		/** 16-bit **/
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_RGB565,
			0,
			0xFF1045A5,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0xA222),
			ImageDecoder::PXF_BGR565,
			0,
			0xFF1045A5,
			16),

		// ARGB4444
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_ARGB4444,
			0,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1432),
			ImageDecoder::PXF_ABGR4444,
			0,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x2341),
			ImageDecoder::PXF_RGBA4444,
			0,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x4321),
			ImageDecoder::PXF_BGRA4444,
			0,
			0x11223344,
			16),

		// xRGB4444
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_xRGB4444,
			0,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1432),
			ImageDecoder::PXF_xBGR4444,
			0,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x2341),
			ImageDecoder::PXF_RGBx4444,
			0,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x4321),
			ImageDecoder::PXF_BGRx4444,
			0,
			0xFF223344,
			16),

		// ARGB1555
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_ARGB1555,
			0,
			0x00218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x5224),
			ImageDecoder::PXF_ABGR1555,
			0,
			0x00218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x9234),
			ImageDecoder::PXF_ARGB1555,
			0,
			0xFF218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0xD224),
			ImageDecoder::PXF_ABGR1555,
			0,
			0xFF218CA5,
			16),

		// RGBA1555
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x4320),
			ImageDecoder::PXF_RGBA5551,
			0,
			0x00426384,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x8310),
			ImageDecoder::PXF_BGRA5551,
			0,
			0x00426384,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x4321),
			ImageDecoder::PXF_RGBA5551,
			0,
			0xFF426384,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x8311),
			ImageDecoder::PXF_BGRA5551,
			0,
			0xFF426384,
			16),

		// RG88
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_RG88,
			0,
			0xFF123400,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x3412),
			ImageDecoder::PXF_GR88,
			0,
			0xFF123400,
			16),

		/** 15-bit **/
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_RGB555,
			0,
			0xFF218CA5,
			15),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x5224),
			ImageDecoder::PXF_BGR555,
			0,
			0xFF218CA5,
			15))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 15/16-bit tests. (custom stride)
INSTANTIATE_TEST_CASE_P(fromLinear16_384, ImageDecoderLinearTest,
	::testing::Values(
		/** 16-bit **/
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_RGB565,
			384,
			0xFF1045A5,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0xA222),
			ImageDecoder::PXF_BGR565,
			384,
			0xFF1045A5,
			16),

		// ARGB4444
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_ARGB4444,
			384,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1432),
			ImageDecoder::PXF_ABGR4444,
			384,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x2341),
			ImageDecoder::PXF_RGBA4444,
			384,
			0x11223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x4321),
			ImageDecoder::PXF_BGRA4444,
			384,
			0x11223344,
			16),

		// xRGB4444
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_xRGB4444,
			384,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1432),
			ImageDecoder::PXF_xBGR4444,
			384,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x2341),
			ImageDecoder::PXF_RGBx4444,
			384,
			0xFF223344,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x4321),
			ImageDecoder::PXF_BGRx4444,
			384,
			0xFF223344,
			16),

		// ARGB1555
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_ARGB1555,
			384,
			0x00218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x5224),
			ImageDecoder::PXF_ABGR1555,
			384,
			0x00218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x9234),
			ImageDecoder::PXF_ARGB1555,
			384,
			0xFF218CA5,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0xD224),
			ImageDecoder::PXF_ABGR1555,
			384,
			0xFF218CA5,
			16),

		// RGBA1555
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x4320),
			ImageDecoder::PXF_RGBA5551,
			384,
			0x00426384,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x8310),
			ImageDecoder::PXF_BGRA5551,
			384,
			0x00426384,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x4321),
			ImageDecoder::PXF_RGBA5551,
			384,
			0xFF426384,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x8311),
			ImageDecoder::PXF_BGRA5551,
			384,
			0xFF426384,
			16),

		// RG88
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_RG88,
			384,
			0xFF123400,
			16),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x3412),
			ImageDecoder::PXF_GR88,
			384,
			0xFF123400,
			16),

		/** 15-bit **/
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x1234),
			ImageDecoder::PXF_RGB555,
			384,
			0xFF218CA5,
			15),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x5224),
			ImageDecoder::PXF_BGR555,
			384,
			0xFF218CA5,
			15))
	, ImageDecoderLinearTest::test_case_suffix_generator);

} }

/**
 * Test suite main function.
 * Called by gtest_init.c.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRpBase test suite: ImageDecoder::fromLinear*() tests.\n\n");
	fprintf(stderr, "Benchmark iterations: %u\n",
		LibRpBase::Tests::ImageDecoderLinearTest::BENCHMARK_ITERATIONS);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
