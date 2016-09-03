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

// zlib and libpng
#include <zlib.h>
#include <png.h>
#include "png_chunks.h"

// libromdata
#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/RpFile.hpp"
#include "file/RpMemFile.hpp"
#include "img/rp_image.hpp"
#include "img/RpImageLoader.hpp"

// C includes.
#include <stdlib.h>

// C++ includes.
#include <memory>
using std::auto_ptr;

namespace LibRomData { namespace Tests {

struct RpImageLoaderTest_mode
{
	rp_string png_filename;		// PNG image to test.

	// Expected rp_image parameters.
	const PNG_IHDR_t ihdr;	// FIXME: Making this const& causes problems.
	rp_image::Format rp_format;

	// TODO: Verify PNG bit depth and color type.

	RpImageLoaderTest_mode(const rp_char *png_filename,
		const PNG_IHDR_t &ihdr,
		rp_image::Format rp_format)
		: png_filename(png_filename)
		, ihdr(ihdr)
		, rp_format(rp_format)
	{ }
};

// Maximum file size for PNG images.
static const int64_t MAX_IMAGE_FILESIZE = 1048576;

class RpImageLoaderTest : public ::testing::TestWithParam<RpImageLoaderTest_mode>
{
	protected:
		RpImageLoaderTest()
			: ::testing::TestWithParam<RpImageLoaderTest_mode>()
			, m_pngBuf(nullptr)
			, m_pngBuf_len(0)
		{ }

		virtual ~RpImageLoaderTest()
		{
			free(m_pngBuf);
		}

	public:
		/**
		 * Load and verify an IHDR chunk.
		 * @param ihdr Destination IHDR chunk.
		 * @param ihdr_src Source IHDR chunk.
		 */
		static void Load_Verify_IHDR(PNG_IHDR_t *ihdr, const uint8_t *ihdr_src);

	public:
		// PNG image buffer.
		uint8_t *m_pngBuf;
		size_t m_pngBuf_len;
};

/**
 * Formatting function for RpImageLoaderTest.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const RpImageLoaderTest_mode& mode) {
	return os << rp_string_to_utf8(mode.png_filename);
};

/**
 * Load and verify an IHDR chunk.
 * @param ihdr Destination IHDR chunk. (data only)
 * @param ihdr_src Source IHDR chunk. (full chunk)
 */
void RpImageLoaderTest::Load_Verify_IHDR(PNG_IHDR_t *ihdr, const uint8_t *ihdr_src)
{
	static_assert(sizeof(PNG_IHDR_t) == PNG_IHDR_t_SIZE,
		"PNG_IHDR_t size is incorrect. (should be 13 bytes)");
	static_assert(sizeof(PNG_IHDR_full_t) == PNG_IHDR_full_t_SIZE,
		"PNG_IHDR_full_t size is incorrect. (should be 25 bytes)");

	PNG_IHDR_full_t ihdr_full;
	memcpy(&ihdr_full, ihdr_src, sizeof(ihdr_full));

	// Calculate the CRC32.
	// This includes the chunk name and data section.
	const uint32_t chunk_crc = (uint32_t)(
		crc32(0, reinterpret_cast<const Bytef*>(&ihdr_full.chunk_name),
		      sizeof(ihdr_full.chunk_name) + sizeof(ihdr_full.data)));

	// Byteswap the values.
	ihdr_full.chunk_size = be32_to_cpu(ihdr_full.chunk_size);
	ihdr_full.data.width = be32_to_cpu(ihdr_full.data.width);
	ihdr_full.data.height = be32_to_cpu(ihdr_full.data.height);
	ihdr_full.crc32 = be32_to_cpu(ihdr_full.crc32);

	// Copy the data section of the IHDR chunk.
	memcpy(ihdr, &ihdr_full.data, sizeof(*ihdr));

	// Chunk size should be sizeof(*ihdr).
	// (12 == chunk_size, chunk_name, and crc32.)
	ASSERT_EQ(sizeof(*ihdr), ihdr_full.chunk_size) <<
		"IHDR chunk size is incorrect.";

	// Chunk name should be "IHDR".
	ASSERT_EQ(0, memcmp(PNG_IHDR_name, ihdr_full.chunk_name, sizeof(PNG_IHDR_name))) <<
		"IHDR chunk's name is incorrect.";

	// Validate the chunk's CRC32.
	// This does NOT include the chunk_size field.
	ASSERT_EQ(ihdr_full.crc32, chunk_crc) <<
		"IHDR CRC32 is incorrect.";
}

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
	ASSERT_TRUE(file->isOpen()) << "RpFile failed to open "
		<< rp_string_to_utf8(mode.png_filename);

	// Maximum image size.
	ASSERT_LE(file->fileSize(), MAX_IMAGE_FILESIZE) << "Test image is too big.";

	// Read the PNG image into memory.
	size_t pngSize = (size_t)file->fileSize();
	m_pngBuf = (uint8_t*)malloc(pngSize);
	ASSERT_TRUE(m_pngBuf != nullptr) << "malloc(" << pngSize << ") failed.";
	size_t readSize = file->read(m_pngBuf, pngSize);
	ASSERT_EQ(pngSize, readSize) << "Error reading image "
		<< rp_string_to_utf8(mode.png_filename);
	m_pngBuf_len = pngSize;

	// Verify the PNG image's magic number.
	ASSERT_EQ(0, memcmp(PNG_magic, m_pngBuf, sizeof(PNG_magic))) <<
		"PNG magic number is incorrect.";

	// Load and verify the IHDR.
	// This should be located immediately after the magic number.
	PNG_IHDR_t ihdr;
	ASSERT_NO_FATAL_FAILURE(Load_Verify_IHDR(&ihdr, &m_pngBuf[8]));

	// Check if the IHDR values are correct.
	EXPECT_EQ(mode.ihdr.width, ihdr.width);
	EXPECT_EQ(mode.ihdr.height, ihdr.height);
	EXPECT_EQ(mode.ihdr.bit_depth, ihdr.bit_depth);
	EXPECT_EQ(mode.ihdr.color_type, ihdr.color_type);
	EXPECT_EQ(mode.ihdr.compression_method, ihdr.compression_method);
	EXPECT_EQ(mode.ihdr.filter_method, ihdr.filter_method);
	EXPECT_EQ(mode.ihdr.interlace_method, ihdr.interlace_method);

	// Create an RpMemFile.
	// This also deletes the previous IRpFile object.
	file.reset(new RpMemFile(m_pngBuf, m_pngBuf_len));
	ASSERT_TRUE(file.get() != nullptr);
	ASSERT_TRUE(file->isOpen());

	// Load the PNG image from memory.
	auto_ptr<rp_image> img(RpImageLoader::load(file.get()));
	ASSERT_TRUE(img.get() != nullptr) << "RpImageLoader failed to load the image.";

	// Check the rp_image parameters.
	EXPECT_EQ((int)mode.ihdr.width, img->width()) << "rp_image width is incorrect.";
	EXPECT_EQ((int)mode.ihdr.height, img->height()) << "rp_image height is incorrect.";
	EXPECT_EQ(mode.rp_format, img->format()) << "rp_image format is incorrect.";
}

// Test cases.

// gl_triangle PNG image tests.
INSTANTIATE_TEST_CASE_P(gl_triangle_png, RpImageLoaderTest,
	::testing::Values(
		RpImageLoaderTest_mode(_RP("gl_triangle.RGB24.png"),
			{400, 352, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_triangle.RGB24.tRNS.png"),
			{400, 352, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_triangle.ARGB32.png"),
			{400, 352, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_triangle.gray.png"),
			{400, 352, 8, PNG_COLOR_TYPE_GRAY, 0, 0, 0},
			rp_image::FORMAT_CI8)
		));

// gl_quad PNG image tests.
INSTANTIATE_TEST_CASE_P(gl_quad_png, RpImageLoaderTest,
	::testing::Values(
		RpImageLoaderTest_mode(_RP("gl_quad.RGB24.png"),
			{480, 384, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_quad.RGB24.tRNS.png"),
			{480, 384, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_quad.ARGB32.png"),
			{480, 384, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(_RP("gl_quad.gray.png"),
			{480, 384, 8, PNG_COLOR_TYPE_GRAY, 0, 0, 0},
			rp_image::FORMAT_CI8)
		));
} }

/**
 * Test suite main function.
 */
int main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: RpImageLoader tests.\n\n");
	fflush(nullptr);

	// Make sure the CRC32 table is initialized.
	get_crc_table();

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
