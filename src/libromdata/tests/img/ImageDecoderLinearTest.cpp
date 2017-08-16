/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * ImageDecoderLinearTest.cpp: Linear image decoding tests with SSSE3.     *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/uvector.h"
#include "librpbase/TextFuncs.hpp"

#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"
using namespace LibRpBase;

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

namespace LibRomData { namespace Tests {

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
	uint32_t dest_pixel;			// Expected decoded ARGB32 pixel.
	bool is24;				// If true, the source data is 24-bit.

	ImageDecoderLinearTest_mode(
		uint32_t src_pixel,
		ImageDecoder::PixelFormat src_pxf,
		uint32_t dest_pixel,
		bool is24)
		: src_pixel(src_pixel)
		, src_pxf(src_pxf)
		, dest_pixel(dest_pixel)
		, is24(is24)
	{ }
};

class ImageDecoderLinearTest : public ::testing::TestWithParam<ImageDecoderLinearTest_mode>
{
	protected:
		ImageDecoderLinearTest()
			: ::testing::TestWithParam<ImageDecoderLinearTest_mode>()
		{ }

		virtual void SetUp(void) override final;

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

	public:
		// Temporary image buffer.
		// 128x128 24-bit or 32-bit image data.
		ao::uvector<uint8_t> m_img_buf;

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
	return os << ImageDecoderLinearTest::pxfToString(mode.src_pxf);
};

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string ImageDecoderLinearTest::test_case_suffix_generator(const ::testing::TestParamInfo<ImageDecoderLinearTest_mode> &info)
{
	return ImageDecoderLinearTest::pxfToString(info.param.src_pxf);
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
	if (mode.is24) {
		// 24-bit color.
		m_img_buf.resize(128*128*3);
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

		uint8_t *p = m_img_buf.data();
		for (unsigned int count = 128*128; count > 0; count--, p += 3) {
			p[0] = bytes[0];
			p[1] = bytes[1];
			p[2] = bytes[2];
		}
	} else {
		// 32-bit color.
		m_img_buf.resize(128*128*4);
		uint32_t *p = reinterpret_cast<uint32_t*>(m_img_buf.data());
		for (unsigned int count = 128*128; count > 0; count -= 4, p += 4) {
			p[0] = mode.src_pixel;
			p[1] = mode.src_pixel;
			p[2] = mode.src_pixel;
			p[3] = mode.src_pixel;
		}
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
	if (mode.is24) {
		// 24-bit image.
		pImg.reset(ImageDecoder::fromLinear24_cpp(mode.src_pxf, 128, 128,
			m_img_buf.data(), m_img_buf.size()));
	} else {
		// 32-bit image.
		pImg.reset(ImageDecoder::fromLinear32_cpp(mode.src_pxf, 128, 128,
			reinterpret_cast<const uint32_t*>(m_img_buf.data()),
			m_img_buf.size()));
	}

	ASSERT_TRUE(pImg.get() != nullptr);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(pImg.get(), mode.dest_pixel));
}

#ifdef IMAGEDECODER_HAS_SSSE3
/**
 * Test the ImageDecoder::fromLinear*() functions. (SSSE3-optimized version)
 */
TEST_P(ImageDecoderLinearTest, fromLinear_ssse3_test)
{
	if (!RP_CPU_HasSSSE3()) {
		fprintf(stderr, "*** SSSE3 is not supported on this CPU. Skipping test.");
		return;
	}

	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	unique_ptr<rp_image> pImg;
	if (mode.is24) {
		// 24-bit image.
		pImg.reset(ImageDecoder::fromLinear24_ssse3(mode.src_pxf, 128, 128,
			m_img_buf.data(), m_img_buf.size()));
	} else {
		// 32-bit image.
		pImg.reset(ImageDecoder::fromLinear32_ssse3(mode.src_pxf, 128, 128,
			reinterpret_cast<const uint32_t*>(m_img_buf.data()),
			m_img_buf.size()));
	}

	ASSERT_TRUE(pImg.get() != nullptr);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(pImg.get(), mode.dest_pixel));
}
#endif /* IMAGEDECODER_HAS_SSSE3 */

// NOTE: Add more instruction sets to the #ifdef if other optimizations are added.
#ifdef IMAGEDECODER_HAS_SSSE3
/**
 * Test the ImageDecoder::fromLinear*() dispatch functions.
 */
TEST_P(ImageDecoderLinearTest, fromLinear_dispatch_test)
{
	// Parameterized test.
	const ImageDecoderLinearTest_mode &mode = GetParam();

	// Decode the image.
	unique_ptr<rp_image> pImg;
	if (mode.is24) {
		// 24-bit image.
		pImg.reset(ImageDecoder::fromLinear24(mode.src_pxf, 128, 128,
			m_img_buf.data(), m_img_buf.size()));
	} else {
		// 32-bit image.
		pImg.reset(ImageDecoder::fromLinear32(mode.src_pxf, 128, 128,
			reinterpret_cast<const uint32_t*>(m_img_buf.data()),
			m_img_buf.size()));
	}

	ASSERT_TRUE(pImg.get() != nullptr);

	// Validate the image.
	ASSERT_NO_FATAL_FAILURE(Validate_RpImage(pImg.get(), mode.dest_pixel));
}
#endif /* IMAGEDECODER_HAS_SSSE3 */

// Test cases.

// 32-bit tests.
INSTANTIATE_TEST_CASE_P(fromLinear32, ImageDecoderLinearTest,
	::testing::Values(
		// ARGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_ARGB8888,
			0x12345678,
			false),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PXF_ABGR8888,
			0x12345678,
			false),

		// xRGB
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12345678),
			ImageDecoder::PXF_xRGB8888,
			0xFF345678,
			false),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x12785634),
			ImageDecoder::PXF_xBGR8888,
			0xFF345678,
			false))
	, ImageDecoderLinearTest::test_case_suffix_generator);

// 24-bit tests.
INSTANTIATE_TEST_CASE_P(fromLinear24, ImageDecoderLinearTest,
	::testing::Values(
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x123456),
			ImageDecoder::PXF_RGB888,
			0xFF123456,
			true),
		ImageDecoderLinearTest_mode(
			le32_to_cpu(0x563412),
			ImageDecoder::PXF_BGR888,
			0xFF123456,
			true))
	, ImageDecoderLinearTest::test_case_suffix_generator);

} }
