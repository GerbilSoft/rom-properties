/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * RpImageLoaderTest.cpp: RpImageLoader class test.                        *
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

// Google Test
#include "gtest/gtest.h"

// libromdata
#include "TextFuncs.hpp"
#include "file/RpFile.hpp"
#include "img/rp_image.hpp"
#include "img/RpImageLoader.hpp"

// C++ includes.
#include <memory>
using std::auto_ptr;

namespace LibRomData { namespace Tests {

struct RpImageLoaderTest_mode
{
	rp_string png_filename;		// PNG image to test.

	// Expected rp_image parameters.
	int rp_img_w, rp_img_h;
	rp_image::Format rp_format;

	// TODO: Verify PNG bit depth and color type.

	RpImageLoaderTest_mode()
		: png_filename(_RP("")) { }

	RpImageLoaderTest_mode(const rp_char *png_filename,
		int rp_img_w, int rp_img_h,
		rp_image::Format rp_format)
		: png_filename(png_filename)
		, rp_img_w(rp_img_w)
		, rp_img_h(rp_img_h)
		, rp_format(rp_format)
	{ }
};

class RpImageLoaderTest : public ::testing::TestWithParam<RpImageLoaderTest_mode>
{
	protected:
		RpImageLoaderTest()
			: ::testing::TestWithParam<RpImageLoaderTest_mode>() { }
		virtual ~RpImageLoaderTest() { }
};

/**
 * Formatting function for VdpSpriteMaskingTest.
 * NOTE: This makes use of VdpSpriteMaskingTest::SpriteTestNames[],
 * so it must be defined after VdpSpriteMaskingTest is declared.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const RpImageLoaderTest_mode& mode) {
	return os << rp_string_to_utf8(mode.png_filename);
};

/**
 * Run an RpImageLoader test.
 */
TEST_P(RpImageLoaderTest, loadTest)
{
	const RpImageLoaderTest_mode &mode = GetParam();

	// TODO: Load the PNG image into memory and verify its parameters.

	// Open the PNG image file.
	auto_ptr<IRpFile> file(new RpFile(mode.png_filename, RpFile::FM_OPEN_READ));
	ASSERT_TRUE(file.get() != nullptr);
	ASSERT_TRUE(file->isOpen());

	// Load the PNG image.
	auto_ptr<rp_image> img(RpImageLoader::load(file.get()));
	ASSERT_TRUE(img.get() != nullptr);

	// Check the rp_image parameters.
	EXPECT_EQ(mode.rp_img_w, img->width());
	EXPECT_EQ(mode.rp_img_h, img->height());
	EXPECT_EQ(mode.rp_format, img->format());
}

// Test cases.

// gl_triangle PNG image tests.
INSTANTIATE_TEST_CASE_P(gl_triangle_png, RpImageLoaderTest,
	::testing::Values(
		RpImageLoaderTest_mode(_RP("gl_triangle.RGB24.png"),
			400, 352, rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_triangle.RGB24.tRNS.png"),
			400, 352, rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_triangle.ARGB32.png"),
			400, 352, rp_image::FORMAT_ARGB32)
		));

// gl_quad PNG image tests.
INSTANTIATE_TEST_CASE_P(gl_quad_png, RpImageLoaderTest,
	::testing::Values(
		RpImageLoaderTest_mode(_RP("gl_quad.RGB24.png"),
			480, 384, rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_quad.RGB24.tRNS.png"),
			480, 384, rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_quad.ARGB32.png"),
			480, 384, rp_image::FORMAT_ARGB32)
		));
} }

/**
 * Test suite main function.
 */
int main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: RpImageLoader tests.\n\n");
	fflush(nullptr);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
