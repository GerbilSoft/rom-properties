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
#include "bmp.h"

// gzclose_r() and gzclose_w() were introduced in zlib-1.2.4.
#if (ZLIB_VER_MAJOR > 1) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR > 2) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR == 2 && ZLIB_VER_REVISION >= 4)
// zlib-1.2.4 or later
#else
#define gzclose_r(file) gzclose(file)
#define gzclose_w(file) gzclose(file)
#endif

// libromdata
#include "common.h"
#include "byteswap.h"
#include "uvector.h"
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
	rp_string bmp_gz_filename;	// Gzipped BMP image for comparison.

	// Expected rp_image parameters.
	const PNG_IHDR_t ihdr;		// FIXME: Making this const& causes problems.
	const BITMAPINFOHEADER bih;	// FIXME: Making this const& causes problems.
	rp_image::Format rp_format;

	// TODO: Verify PNG bit depth and color type.

	RpImageLoaderTest_mode(
		const rp_char *png_filename,
		const rp_char *bmp_gz_filename,
		const PNG_IHDR_t &ihdr,
		const BITMAPINFOHEADER &bih,
		rp_image::Format rp_format)
		: png_filename(png_filename)
		, bmp_gz_filename(bmp_gz_filename)
		, ihdr(ihdr)
		, bih(bih)
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
			, m_gzBmp(nullptr)
		{ }

		virtual void SetUp(void) override;
		virtual void TearDown(void) override;

	public:
		/**
		 * Load and verify an IHDR chunk.
		 * @param ihdr Destination IHDR chunk.
		 * @param ihdr_src Source IHDR chunk.
		 */
		static void Load_Verify_IHDR(PNG_IHDR_t *ihdr, const uint8_t *ihdr_src);

		/**
		 * Load and verify the headers from a bitmap file.
		 * @param pBfh Destination BITMAPFILEHEADER.
		 * @param pBih Destination BITMAPINFOHEADER.
		 * @param bmp_buf Bitmap buffer.
		 */
		static void Load_Verify_BMP_headers(
			BITMAPFILEHEADER *pBfh,
			BITMAPINFOHEADER *pBih,
			const ao::uvector<uint8_t> &bmp_buf);

		/**
		 * Compare an ARGB32 rp_image to a 24-bit RGB bitmap.
		 * @param img rp_image
		 * @param pBits Bitmap image data.
		 */
		static void Compare_ARGB32_BMP24(
			const rp_image *img,
			const uint8_t *pBits);

		/**
		 * Compare an ARGB32 rp_image to a 32-bit ARGB bitmap.
		 * @param img rp_image
		 * @param pBits Bitmap image data.
		 */
		static void Compare_ARGB32_BMP32(
			const rp_image *img,
			const uint8_t *pBits);

		/**
		 * Compare a CI8 rp_image to an 8-bit CI8 bitmap.
		 * @param img rp_image
		 * @param pBits Bitmap image data.
		 * @param pBmpPalette Bitmap palette.
		 */
		static void Compare_CI8_BMP8(
			const rp_image *img,
			const uint8_t *pBits,
			const uint32_t *pBmpPalette);

	public:
		// Image buffers.
		ao::uvector<uint8_t> m_png_buf;
		ao::uvector<uint8_t> m_bmp_buf;

		// gzip file handle for .bmp.gz.
		// Placed here so it can be freed by TearDown() if necessary.
		gzFile m_gzBmp;
};

/**
 * Formatting function for RpImageLoaderTest.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const RpImageLoaderTest_mode& mode) {
	return os << rp_string_to_utf8(mode.png_filename);
};

/**
 * SetUp() function.
 * Run before each test.
 */
void RpImageLoaderTest::SetUp(void)
{
	if (::testing::UnitTest::GetInstance()->current_test_info()->value_param() == nullptr) {
		// Not a parameterized test.
		return;
	}

	// Parameterized test.
	const RpImageLoaderTest_mode &mode = GetParam();

	// Open the PNG image file being tested.
	auto_ptr<IRpFile> file(new RpFile(mode.png_filename, RpFile::FM_OPEN_READ));
	ASSERT_TRUE(file.get() != nullptr);
	ASSERT_TRUE(file->isOpen());

	// Maximum image size.
	ASSERT_LE(file->fileSize(), MAX_IMAGE_FILESIZE) << "PNG test image is too big.";

	// Read the PNG image into memory.
	size_t pngSize = (size_t)file->fileSize();
	m_png_buf.resize(pngSize);
	ASSERT_EQ(pngSize, m_png_buf.size());
	size_t readSize = file->read(m_png_buf.data(), pngSize);
	ASSERT_EQ(pngSize, readSize) << "Error loading PNG image file: "
		<< rp_string_to_utf8(mode.png_filename);

	// Open the gzipped BMP image file being tested.
	file.reset(new RpFile(mode.bmp_gz_filename, RpFile::FM_OPEN_READ));
	ASSERT_TRUE(file.get() != nullptr);
	ASSERT_TRUE(file->isOpen());
	ASSERT_GT(file->fileSize(), 16);

	// Get the uncompressed image size from the end of the file.
	uint32_t bmpSize;
	file->seek(file->fileSize() - 4);
	file->read(&bmpSize, sizeof(bmpSize));
	bmpSize = le32_to_cpu(bmpSize);
	ASSERT_GT(bmpSize, sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER))
		<< "BMP test image is too small.";
	ASSERT_LE(bmpSize, MAX_IMAGE_FILESIZE)
		<< "BMP test image is too big.";

	// Read the BMP image into memory.
	m_bmp_buf.resize(bmpSize);
	ASSERT_EQ((size_t)bmpSize, m_bmp_buf.size());
	m_gzBmp = gzopen(rp_string_to_utf8(mode.bmp_gz_filename).c_str(), "r");
	ASSERT_TRUE(m_gzBmp != nullptr) << "gzopen() failed to open the BMP file:"
		<< rp_string_to_utf8(mode.png_filename);
	int sz = gzread(m_gzBmp, m_bmp_buf.data(), bmpSize);
	ASSERT_EQ(bmpSize, (uint32_t)sz) << "Error loading BMP image file: "
		<< rp_string_to_utf8(mode.png_filename);

	gzclose_r(m_gzBmp);
	m_gzBmp = nullptr;
}

/**
 * TearDown() function.
 * Run after each test.
 */
void RpImageLoaderTest::TearDown(void)
{
	if (m_gzBmp) {
		gzclose_r(m_gzBmp);
		m_gzBmp = nullptr;
	}
}

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
#if SYS_BYTEORDER != SYS_BIG_ENDIAN
	ihdr_full.chunk_size	= be32_to_cpu(ihdr_full.chunk_size);
	ihdr_full.data.width	= be32_to_cpu(ihdr_full.data.width);
	ihdr_full.data.height	= be32_to_cpu(ihdr_full.data.height);
	ihdr_full.crc32		= be32_to_cpu(ihdr_full.crc32);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */

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
 * Load and verify the headers from a bitmap file.
 * @param pBfh Destination BITMAPFILEHEADER.
 * @param pBih Destination BITMAPINFOHEADER.
 * @param bmp_buf Bitmap buffer.
 */
void RpImageLoaderTest::Load_Verify_BMP_headers(
	BITMAPFILEHEADER *pBfh,
	BITMAPINFOHEADER *pBih,
	const ao::uvector<uint8_t> &bmp_buf)
{
	static_assert(sizeof(BITMAPFILEHEADER) == BITMAPFILEHEADER_SIZE,
		"BITMAPFILEHEADER size is incorrect. (should be 14 bytes)");
	static_assert(sizeof(BITMAPINFOHEADER) == BITMAPINFOHEADER_SIZE,
		"BITMAPINFOHEADER size is incorrect. (should be 40 bytes)");

	// Load the BITMAPFILEHEADER.
	memcpy(pBfh, bmp_buf.data(), sizeof(*pBfh));

	// Byteswap the values.
	pBfh->bfType		= be16_to_cpu(pBfh->bfType);
	pBfh->bfSize		= le32_to_cpu(pBfh->bfSize);
	pBfh->bfReserved1	= le16_to_cpu(pBfh->bfReserved1);
	pBfh->bfReserved2	= le16_to_cpu(pBfh->bfReserved2);
	pBfh->bfOffBits		= le32_to_cpu(pBfh->bfOffBits);

	// Check the magic number.
	ASSERT_EQ(BMP_magic, pBfh->bfType) <<
		"BITMAPFILEHEADER's magic number is incorrect.";
	// bfh.bfSize should be the size of the file.
	EXPECT_EQ(bmp_buf.size(), (size_t)pBfh->bfSize) <<
		"BITMAPFILEHEADER.bfSize does not match the BMP file size.";
	// bfOffBits should be less than the file size.
	ASSERT_LT((size_t)pBfh->bfOffBits, bmp_buf.size()) <<
		"BITMAPFILEHEADER.bfOffBits is past the end of the BMP file.";

	// Load the BITMAPINFOHEADER.
	memcpy(pBih, bmp_buf.data() + sizeof(BITMAPFILEHEADER), sizeof(*pBih));

#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	// Byteswap the values.
	// TODO: Update the byteswapping macros to handle signed ints.
	pBih->biSize		= le32_to_cpu(pBih->biSize);
	pBih->biWidth		= le32_to_cpu((uint32_t)pBih->biWidth);
	pBih->biHeight		= le32_to_cpu((uint32_t)pBih->biHeight);
	pBih->biPlanes		= le16_to_cpu(pBih->biPlanes);
	pBih->biBitCount	= le16_to_cpu(pBih->biBitCount);
	pBih->biCompression	= le32_to_cpu(pBih->biCompression);
	pBih->biSizeImage	= le32_to_cpu(pBih->biSizeImage);
	pBih->biXPelsPerMeter	= le32_to_cpu((uint32_t)pBih->biXPelsPerMeter);
	pBih->biYPelsPerMeter	= le32_to_cpu((uint32_t)pBih->biYPelsPerMeter);
	pBih->biClrUsed		= le32_to_cpu(pBih->biClrUsed);
	pBih->biClrImportant	= le32_to_cpu(pBih->biClrImportant);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

	// NOTE: The BITMAPINFOHEADER may be one of several types.
	// We only care about the BITMAPINFOHEADER section.
	ASSERT_NE(BITMAPCOREHEADER_SIZE, pBih->biSize) <<
		"Windows 2.0 and OS/2 1.x bitmaps are not supported.";
	ASSERT_NE(OS22XBITMAPHEADER_SIZE, pBih->biSize) <<
		"OS/2 2.x bitmaps are not supported.";
	ASSERT_NE(OS22XBITMAPHEADER_SHORT_SIZE, pBih->biSize) <<
		"OS/2 2.x bitmaps are not supported.";

	switch (pBih->biSize) {
		case BITMAPINFOHEADER_SIZE:
		case BITMAPV2INFOHEADER_SIZE:
		case BITMAPV3INFOHEADER_SIZE:
		case BITMAPV4HEADER_SIZE:
		case BITMAPV5HEADER_SIZE:
			// Supported size.
			break;

		default:
			// Assume anything larger than BITMAPINFOHEADER_SIZE is supported.
			ASSERT_GE(pBih->biSize, BITMAPINFOHEADER_SIZE) <<
				"Unsupported BITMAPINFOHEADER size.";
			break;
	}
}

/**
 * Compare an ARGB32 rp_image to a 24-bit RGB bitmap.
 * @param img rp_image
 * @param pBfh BITMAPFILEHEADER
 * @param pBih BITMAPINFOHEADER
 * @param pBits Bitmap image data.
 */
void RpImageLoaderTest::Compare_ARGB32_BMP24(
	const rp_image *img,
	const uint8_t *pBits)
{
	// BMP images are stored upside-down.
	// TODO: 8px row alignment?

	// To avoid calling EXPECT_EQ() for every single pixel,
	// XOR the two pixels together, then OR it here.
	// The eventual result should be 0 if the images are identical.
	uint32_t xor_result = 0;

	for (int y = img->height()-1; y >= 0; y--) {
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		// 24-bit:          RRRRRRRR GGGGGGGG BBBBBBBB
		// NOTE: We're reading the individual bytes from the BMP.
		// BMP uses little-endian, so the Blue channel is first.
		// FIXME: This may fail on aligned architectures.
		const uint32_t *pSrc = reinterpret_cast<const uint32_t*>(img->scanLine(y));
		for (int x = img->width()-1; x >= 0; x--, pSrc++, pBits += 3) {
			// Convert the source's 24-bit pixel to 32-bit.
			uint32_t bmp32 = pBits[0] | pBits[1] << 8 | pBits[2] << 16;
			// Pixel is fully opaque.
			bmp32 |= 0xFF000000;
			xor_result |= (*pSrc ^ bmp32);
		}
	}

	EXPECT_EQ(0U, xor_result) << "Comparison of ARGB32 rp_image to 24-bit RGB BMP failed.";
}

/**
 * Compare an ARGB32 rp_image to a 32-bit RGB bitmap.
 * @param img rp_image
 * @param pBfh BITMAPFILEHEADER
 * @param pBih BITMAPINFOHEADER
 * @param pBits Bitmap image data.
 */
void RpImageLoaderTest::Compare_ARGB32_BMP32(
	const rp_image *img,
	const uint8_t *pBits)
{
	// BMP images are stored upside-down.
	// TODO: 8px row alignment?
	// FIXME: This may fail on aligned architectures.
	const uint32_t *pBmp32 = reinterpret_cast<const uint32_t*>(pBits);

	// To avoid calling EXPECT_EQ() for every single pixel,
	// XOR the two pixels together, then OR it here.
	// The eventual result should be 0 if the images are identical.
	uint32_t xor_result = 0;

	for (int y = img->height()-1; y >= 0; y--) {
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		// BMP uses little-endian, so byteswapping is needed.
		const uint32_t *pSrc = reinterpret_cast<const uint32_t*>(img->scanLine(y));
		for (int x = img->width()-1; x >= 0; x--, pSrc++, pBmp32++) {
			const uint32_t pxBmp = le32_to_cpu(*pBmp32);
			xor_result |= (*pSrc ^ pxBmp);
		}
	}

	EXPECT_EQ(0U, xor_result) << "Comparison of ARGB32 rp_image to 32-bit ARGB BMP failed.";
}

/**
 * Compare a CI8 rp_image to an 8-bit CI8 bitmap.
 * @param img rp_image
 * @param pBits Bitmap image data.
 * @param pBmpPalette Bitmap palette.
 */
void RpImageLoaderTest::Compare_CI8_BMP8(
	const rp_image *img,
	const uint8_t *pBits,
	const uint32_t *pBmpPalette)
{
	// BMP images are stored upside-down.
	// TODO: 8px row alignment?

	// To avoid calling EXPECT_EQ() for every single pixel,
	// XOR the two pixels together, then OR it here.
	// The eventual result should be 0 if the images are identical.
	uint32_t xor_result = 0;

	// Check the palette.
	const uint32_t *pSrcPalette = reinterpret_cast<const uint32_t*>(img->palette());
	for (int i = img->palette_len()-1; i >= 0; i--, pSrcPalette++, pBmpPalette++) {
		// NOTE: The alpha channel in the BMP palette is ignored.
		// It's always 0. Assume all colors are fully opaque.
		const uint32_t bmp_color = le32_to_cpu(*pBmpPalette) | 0xFF000000;
		xor_result |= (*pSrcPalette ^ bmp_color);
	}
	EXPECT_EQ(0U, xor_result) << "CI8 rp_image's palette doesn't match CI8 BMP.";

	// Check the image data.
	xor_result = 0;
	const int width = img->width();
	for (int y = img->height()-1; y >= 0; y--, pBits += width) {
		// Do a full memcmp() instead of xoring bytes, since
		// each pixel is a single byte.
		const uint8_t *pSrc = reinterpret_cast<const uint8_t*>(img->scanLine(y));
		xor_result |= memcmp(pSrc, pBits, width);
	}
	EXPECT_EQ(0U, xor_result) << "CI8 rp_image's pixel data doesn't match CI8 BMP.";
}

/**
 * Run an RpImageLoader test.
 */
TEST_P(RpImageLoaderTest, loadTest)
{
	const RpImageLoaderTest_mode &mode = GetParam();

	// Make sure the PNG image was actually loaded.
	ASSERT_GT(m_png_buf.size(), sizeof(PNG_magic) + sizeof(PNG_IHDR_full_t)) <<
		"PNG image is too small.";

	// Verify the PNG image's magic number.
	ASSERT_EQ(0, memcmp(PNG_magic, m_png_buf.data(), sizeof(PNG_magic))) <<
		"PNG image's magic number is incorrect.";

	// Load and verify the IHDR.
	// This should be located immediately after the magic number.
	PNG_IHDR_t ihdr;
	ASSERT_NO_FATAL_FAILURE(Load_Verify_IHDR(&ihdr, m_png_buf.data() + 8));

	// Check if the IHDR values are correct.
	EXPECT_EQ(mode.ihdr.width,		ihdr.width);
	EXPECT_EQ(mode.ihdr.height,		ihdr.height);
	EXPECT_EQ(mode.ihdr.bit_depth,		ihdr.bit_depth);
	EXPECT_EQ(mode.ihdr.color_type,		ihdr.color_type);
	EXPECT_EQ(mode.ihdr.compression_method,	ihdr.compression_method);
	EXPECT_EQ(mode.ihdr.filter_method,	ihdr.filter_method);
	EXPECT_EQ(mode.ihdr.interlace_method,	ihdr.interlace_method);

	// Create an RpMemFile.
	auto_ptr<IRpFile> png_mem_file(new RpMemFile(m_png_buf.data(), m_png_buf.size()));
	ASSERT_TRUE(png_mem_file.get() != nullptr);
	ASSERT_TRUE(png_mem_file->isOpen());

	// Load the PNG image from memory.
	auto_ptr<rp_image> img(RpImageLoader::load(png_mem_file.get()));
	ASSERT_TRUE(img.get() != nullptr) << "RpImageLoader failed to load the image.";

	// Check the rp_image parameters.
	EXPECT_EQ((int)mode.ihdr.width, img->width()) << "rp_image width is incorrect.";
	EXPECT_EQ((int)mode.ihdr.height, img->height()) << "rp_image height is incorrect.";
	EXPECT_EQ(mode.rp_format, img->format()) << "rp_image format is incorrect.";

	// Load and verify the bitmap headers.
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	ASSERT_NO_FATAL_FAILURE(Load_Verify_BMP_headers(&bfh, &bih, m_bmp_buf));

	// Check if the BITMAPINFOHEADER values are correct.
	// BITMAPINFOHEADER.biSize is checked by Load_Verify_BMP_headers().
	// NOTE: The PelsPerMeter fields are ignored. The test BMP images
	// have them set to 3936 (~100 dpi).
	EXPECT_EQ(mode.bih.biWidth,		bih.biWidth);
	EXPECT_EQ(mode.bih.biHeight,		bih.biHeight);
	EXPECT_EQ(mode.bih.biPlanes,		bih.biPlanes);
	EXPECT_EQ(mode.bih.biBitCount,		bih.biBitCount);
	EXPECT_EQ(mode.bih.biCompression,	bih.biCompression);
	EXPECT_EQ(mode.bih.biSizeImage,		bih.biSizeImage);
	//EXPECT_EQ(mode.bih.biXPelsPerMeter,	bih.biXPelsPerMeter);
	//EXPECT_EQ(mode.bih.biYPelsPerMeter,	bih.biYPelsPerMeter);
	EXPECT_EQ(mode.bih.biClrUsed,		bih.biClrUsed);
	EXPECT_EQ(mode.bih.biClrImportant,	bih.biClrImportant);

	// Compare the image data.
	const uint8_t *pBits = m_bmp_buf.data() + bfh.bfOffBits;
	if (img->format() == rp_image::FORMAT_ARGB32) {
		if (bih.biBitCount == 24 && bih.biCompression == BI_RGB) {
			// Comparing an ARGB32 rp_image to a 24-bit RGB bitmap.
			ASSERT_NO_FATAL_FAILURE(Compare_ARGB32_BMP24(img.get(), pBits));
		} else if (bih.biBitCount == 32 && bih.biCompression == BI_BITFIELDS) {
			// Comparing an ARGB32 rp_image to a 32-bit ARGB bitmap.
			// TODO: Check the bitfield masks?
			ASSERT_NO_FATAL_FAILURE(Compare_ARGB32_BMP32(img.get(), pBits));
		} else {
			// Unsupported comparison.
			ASSERT_TRUE(false) << "Image format comparison isn't supported.";
		}
	} else if (img->format() == rp_image::FORMAT_CI8) {
		if (bih.biBitCount == 8 && bih.biCompression == BI_RGB) {
			// 256-color image. Get the palette.
			ASSERT_EQ(img->palette_len(), (int)bih.biClrUsed)
				<< "BMP palette length does not match rp_image palette length.";
			// NOTE: Palette has 32-bit entries, but the alpha channel is ignored.
			// FIXME: This may fail on aligned architectures.
			const uint32_t *pBmpPalette = reinterpret_cast<const uint32_t*>(
				m_bmp_buf.data() + sizeof(BITMAPFILEHEADER) + bih.biSize);
			ASSERT_NO_FATAL_FAILURE(Compare_CI8_BMP8(img.get(), pBits, pBmpPalette));
		}
	} else {
		// Unsupported comparison.
		ASSERT_TRUE(false) << "Image format comparison isn't supported.";
	}
}

// Test cases.

// NOTE: 32-bit ARGB bitmaps use BI_BITFIELDS.
// 24-bit RGB bitmaps use BI_RGB.
// 256-color bitmaps use BI_RGB, unless they're RLE-compressed,
// in which case they use BI_RLE8.

// TODO: Test PNG_COLOR_TYPE_GRAY_ALPHA, paletted images
// with alphatransparency, and gray/paletted images using
// 1-, 2-, 4-, and 8 bits per channel.

// gl_triangle PNG image tests.
INSTANTIATE_TEST_CASE_P(gl_triangle_png, RpImageLoaderTest,
	::testing::Values(
		RpImageLoaderTest_mode(
			_RP("gl_triangle.RGB24.png"),
			_RP("gl_triangle.RGB24.bmp.gz"),
			{400, 352, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0},
			{sizeof(BITMAPINFOHEADER),
				400, 352, 1, 24, BI_RGB, 400*352*(24/8),
				3936, 3936, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(
			_RP("gl_triangle.RGB24.tRNS.png"),
			_RP("gl_triangle.RGB24.tRNS.bmp.gz"),
			{400, 352, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0},
			{sizeof(BITMAPINFOHEADER),
				400, 352, 1, 32, BI_BITFIELDS, 400*352*(32/8),
				3936, 3936, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(
			_RP("gl_triangle.ARGB32.png"),
			_RP("gl_triangle.ARGB32.bmp.gz"),
			{400, 352, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0, 0},
			{sizeof(BITMAPINFOHEADER),
				400, 352, 1, 32, BI_BITFIELDS, 400*352*(32/8),
				3936, 3936, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(
			_RP("gl_triangle.gray.png"),
			_RP("gl_triangle.gray.bmp.gz"),
			{400, 352, 8, PNG_COLOR_TYPE_GRAY, 0, 0, 0},
			{sizeof(BITMAPINFOHEADER),
				400, 352, 1, 8, BI_RGB, 400*352,
				3936, 3936, 256, 256},
			rp_image::FORMAT_CI8)
		));

// gl_quad PNG image tests.
INSTANTIATE_TEST_CASE_P(gl_quad_png, RpImageLoaderTest,
	::testing::Values(
		RpImageLoaderTest_mode(
			_RP("gl_quad.RGB24.png"),
			_RP("gl_quad.RGB24.bmp.gz"),
			{480, 384, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0},
			{sizeof(BITMAPINFOHEADER),
				480, 384, 1, 24, BI_RGB, 480*384*(24/8),
				3936, 3936, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(
			_RP("gl_quad.RGB24.tRNS.png"),
			_RP("gl_quad.RGB24.tRNS.bmp.gz"),
			{480, 384, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0},
			{sizeof(BITMAPINFOHEADER),
				480, 384, 1, 32, BI_BITFIELDS, 480*384*(32/8),
				3936, 3936, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(
			_RP("gl_quad.ARGB32.png"),
			_RP("gl_quad.ARGB32.bmp.gz"),
			{480, 384, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0, 0},
			{sizeof(BITMAPINFOHEADER),
				480, 384, 1, 32, BI_BITFIELDS, 480*384*(32/8),
				3936, 3936, 0, 0},
			rp_image::FORMAT_ARGB32),
		RpImageLoaderTest_mode(
			_RP("gl_quad.gray.png"),
			_RP("gl_quad.gray.bmp.gz"),
			{480, 384, 8, PNG_COLOR_TYPE_GRAY, 0, 0, 0},
			{sizeof(BITMAPINFOHEADER),
				480, 384, 1, 8, BI_RGB, 480*384,
				3936, 3936, 256, 256},
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
