/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * RpPngFormatTest.cpp: RpImageLoader PNG format test.                     *
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

// zlib and libpng
#include <zlib.h>
#include "png_chunks.h"
#include "bmp.h"

#ifdef HAVE_PNG
#include <png.h>
#endif /* HAVE_PNG */

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
#include "file/FileSystem.hpp"
#include "img/rp_image.hpp"
#include "img/RpImageLoader.hpp"

// C includes.
#include <cstdlib>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
using std::string;
using std::unique_ptr;

namespace LibRomData { namespace Tests {

// tRNS chunk for CI8 paletted images.
// BMP format doesn't support alpha values
// in the color table.
struct tRNS_CI8_t
{
	uint8_t alpha[256];
};

struct RpPngFormatTest_mode
{
	rp_string png_filename;		// PNG image to test.
	rp_string bmp_gz_filename;	// Gzipped BMP image for comparison.

	// Expected PNG and BMP parameters.
	PNG_IHDR_t ihdr;		// FIXME: Making this const& causes problems.
	BITMAPINFOHEADER bih;		// FIXME: Making this const& causes problems.
	tRNS_CI8_t bmp_tRNS;		// FIXME: Making this const& causes problems.
	rp_image::Format rp_format;	// Expected rp_image format.

	bool has_bmp_tRNS;		// Set if bmp_tRNS is specified in the constructor.

	// TODO: Verify PNG bit depth and color type.

	RpPngFormatTest_mode(
		const rp_char *png_filename,
		const rp_char *bmp_gz_filename,
		const PNG_IHDR_t &ihdr,
		const BITMAPINFOHEADER &bih,
		const tRNS_CI8_t &bmp_tRNS,
		rp_image::Format rp_format)
		: png_filename(png_filename)
		, bmp_gz_filename(bmp_gz_filename)
		, ihdr(ihdr)
		, bih(bih)
		, bmp_tRNS(bmp_tRNS)
		, rp_format(rp_format)
		, has_bmp_tRNS(true)
	{ }

	RpPngFormatTest_mode(
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
		, has_bmp_tRNS(false)
	{
		// No tRNS chunk for the BMP image.
		memset(bmp_tRNS.alpha, 0xFF, sizeof(bmp_tRNS));
	}

	// May be required for MSVC 2010?
	RpPngFormatTest_mode(const RpPngFormatTest_mode &other)
		: png_filename(other.png_filename)
		, bmp_gz_filename(other.bmp_gz_filename)
		, ihdr(other.ihdr)
		, bih(other.bih)
		, rp_format(other.rp_format)
		, has_bmp_tRNS(other.has_bmp_tRNS)
	{
		memcpy(bmp_tRNS.alpha, other.bmp_tRNS.alpha, sizeof(bmp_tRNS));
	}

	// Required for MSVC 2010.
	RpPngFormatTest_mode &operator=(const RpPngFormatTest_mode &other)
	{
		png_filename = other.png_filename;
		bmp_gz_filename = other.bmp_gz_filename;
		ihdr = other.ihdr;
		bih = other.bih;
		memcpy(bmp_tRNS.alpha, other.bmp_tRNS.alpha, sizeof(bmp_tRNS));
		rp_format = other.rp_format;
		has_bmp_tRNS = other.has_bmp_tRNS;
		return *this;
	}
};

// Maximum file size for images.
static const int64_t MAX_PNG_IMAGE_FILESIZE =    512*1024;
static const int64_t MAX_BMP_IMAGE_FILESIZE = 2*1024*1024;

class RpPngFormatTest : public ::testing::TestWithParam<RpPngFormatTest_mode>
{
	protected:
		RpPngFormatTest()
			: ::testing::TestWithParam<RpPngFormatTest_mode>()
			, m_gzBmp(nullptr)
		{ }

		virtual void SetUp(void) override final;
		virtual void TearDown(void) override final;

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
		 * Compare an ARGB32 rp_image to an ARGB32 bitmap.
		 * @param img rp_image
		 * @param pBits Bitmap image data.
		 */
		static void Compare_ARGB32_BMP32(
			const rp_image *img,
			const uint8_t *pBits);

		/**
		 * Compare an rp_image's palette to a BMP palette.
		 * @param img rp_image.
		 * @param pBmpPalette Bitmap palette.
		 * @param pBmpAlpha BMP tRNS chunk from the test parameter. (If nullptr, no tRNS.)
		 * @param bmpColorTableSize BMP color table size. (256 for CI8, 2 for monochrome.)
		 * @param biClrUsed Number of used colors. (If negative, check all 256 colors.)
		 */
		static void Compare_Palettes(
			const rp_image *img,
			const uint32_t *pBmpPalette,
			const tRNS_CI8_t *pBmpAlpha = nullptr,
			int bmpColorTableSize = 256,
			int biClrUsed = -1);

		/**
		 * Compare a CI8 rp_image to an 8-bit CI8 bitmap.
		 * @param img rp_image
		 * @param pBits Bitmap image data.
		 * @param pBmpPalette Bitmap palette.
		 * @param pBmpAlpha BMP tRNS chunk from the test parameter. (If nullptr, no tRNS.)
		 * @param biClrUsed Number of used colors. (If negative, check the entire palette.)
		 */
		static void Compare_CI8_BMP8(
			const rp_image *img,
			const uint8_t *pBits,
			const uint32_t *pBmpPalette,
			const tRNS_CI8_t *pBmpAlpha = nullptr,
			int biClrUsed = -1);

		/**
		 * Compare an ARGB32 rp_image to an 8-bit CI8 bitmap.
		 * NOTE: This should only happen if GDI+ decoded
		 * a grayscale image to ARGB32, which seems to happen
		 * on wine-1.9.18 and AppVeyor for some reason.
		 * @param img rp_image
		 * @param pBits Bitmap image data.
		 * @param pBmpPalette Bitmap palette.
		 */
		static void Compare_ARGB32_BMP8(
			const rp_image *img,
			const uint8_t *pBits,
			const uint32_t *pBmpPalette);

		/**
		 * Compare a CI8 rp_image to an ARGB32 bitmap.
		 * wine-1.9.18 loads xterm-256color.CI8.tRNS.png as CI8
		 * instead of as ARGB32 for some reason.
		 * @param img rp_image
		 * @param pBits Bitmap image data.
		 */
		static void Compare_CI8_BMP32(
			const rp_image *img,
			const uint8_t *pBits);

		/**
		 * Compare a CI8 rp_image to a monochrome bitmap.
		 * @param img rp_image
		 * @param pBits Bitmap image data.
		 * @param pBmpPalette Bitmap palette.
		 * @param pBmpAlpha BMP tRNS chunk from the test parameter. (If nullptr, no tRNS.)
		 * @param biClrUsed Number of used colors. (If negative, check the entire palette.)
		 */
		static void Compare_CI8_BMP1(
			const rp_image *img,
			const uint8_t *pBits,
			const uint32_t *pBmpPalette,
			const tRNS_CI8_t *pBmpAlpha = nullptr,
			int biClrUsed = -1);

	public:
		// Image buffers.
		ao::uvector<uint8_t> m_png_buf;
		ao::uvector<uint8_t> m_bmp_buf;

		// gzip file handle for .bmp.gz.
		// Placed here so it can be freed by TearDown() if necessary.
		gzFile m_gzBmp;

	public:
		/** Test case parameters. **/

		/**
		 * Test case suffix generator.
		 * @param info Test parameter information.
		 * @return Test case suffix.
		 */
		static string test_case_suffix_generator(const ::testing::TestParamInfo<RpPngFormatTest_mode> &info);
};

/**
 * Formatting function for RpPngFormatTest.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const RpPngFormatTest_mode& mode)
{
	return os << rp_string_to_utf8(mode.png_filename);
};

/**
 * SetUp() function.
 * Run before each test.
 */
void RpPngFormatTest::SetUp(void)
{
	if (::testing::UnitTest::GetInstance()->current_test_info()->value_param() == nullptr) {
		// Not a parameterized test.
		return;
	}

	// Parameterized test.
	const RpPngFormatTest_mode &mode = GetParam();

	// Open the PNG image file being tested.
	rp_string path = _RP("png_data");
	path += _RP_CHR(DIR_SEP_CHR);
	path += mode.png_filename;
	unique_ptr<IRpFile> file(new RpFile(path, RpFile::FM_OPEN_READ));
	ASSERT_TRUE(file.get() != nullptr);
	ASSERT_TRUE(file->isOpen());

	// Maximum image size.
	ASSERT_LE(file->size(), MAX_PNG_IMAGE_FILESIZE) << "PNG test image is too big.";

	// Read the PNG image into memory.
	const size_t pngSize = (size_t)file->size();
	m_png_buf.resize(pngSize);
	ASSERT_EQ(pngSize, m_png_buf.size());
	size_t readSize = file->read(m_png_buf.data(), pngSize);
	ASSERT_EQ(pngSize, readSize) << "Error loading PNG image file: "
		<< rp_string_to_utf8(mode.png_filename);

	// Open the gzipped BMP image file being tested.
	path.resize(9);	// Back to "png_data/".
	path += mode.bmp_gz_filename;
	m_gzBmp = gzopen(rp_string_to_utf8(path).c_str(), "rb");
	ASSERT_TRUE(m_gzBmp != nullptr) << "gzopen() failed to open the BMP file:"
		<< rp_string_to_utf8(mode.bmp_gz_filename);

	// Get the decompressed file size.
	// gzseek() does not support SEEK_END.
	// Read through the file until we hit an EOF.
	// NOTE: We could optimize this by reading the uncompressed
	// file size if gzdirect() == 1, but this is a test case,
	// so it doesn't really matter.
	uint8_t buf[4096];
	uint32_t bmpSize = 0;
	while (!gzeof(m_gzBmp)) {
		int sz_read = gzread(m_gzBmp, buf, sizeof(buf));
		ASSERT_NE(sz_read, -1) << "gzread() failed.";
		bmpSize += sz_read;
	}
	gzrewind(m_gzBmp);

	ASSERT_GT(bmpSize, sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER))
		<< "BMP test image is too small.";
	ASSERT_LE(bmpSize, MAX_BMP_IMAGE_FILESIZE)
		<< "BMP test image is too big.";

	// Read the BMP image into memory.
	m_bmp_buf.resize(bmpSize);
	ASSERT_EQ((size_t)bmpSize, m_bmp_buf.size());
	int sz = gzread(m_gzBmp, m_bmp_buf.data(), bmpSize);
	gzclose_r(m_gzBmp);
	m_gzBmp = nullptr;

	ASSERT_EQ(bmpSize, (uint32_t)sz) << "Error loading BMP image file: "
		<< rp_string_to_utf8(mode.bmp_gz_filename);
}

/**
 * TearDown() function.
 * Run after each test.
 */
void RpPngFormatTest::TearDown(void)
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
void RpPngFormatTest::Load_Verify_IHDR(PNG_IHDR_t *ihdr, const uint8_t *ihdr_src)
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
void RpPngFormatTest::Load_Verify_BMP_headers(
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
void RpPngFormatTest::Compare_ARGB32_BMP24(
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
		const uint32_t *pSrc = static_cast<const uint32_t*>(img->scanLine(y));
		for (int x = img->width()-1; x >= 0; x--, pSrc++, pBits += 3) {
			// Convert the BMP's 24-bit pixel to 32-bit.
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
void RpPngFormatTest::Compare_ARGB32_BMP32(
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
		const uint32_t *pSrc = static_cast<const uint32_t*>(img->scanLine(y));
		for (int x = img->width()-1; x >= 0; x--, pSrc++, pBmp32++) {
			const uint32_t pxBmp = le32_to_cpu(*pBmp32);
			xor_result |= (*pSrc ^ pxBmp);
		}
	}

	EXPECT_EQ(0U, xor_result) << "Comparison of ARGB32 rp_image to 32-bit ARGB BMP failed.";
}

/**
 * Compare an rp_image's palette to a BMP palette.
 * @param img rp_image.
 * @param pBmpPalette Bitmap palette.
 * @param pBmpAlpha BMP tRNS chunk from the test parameter. (If nullptr, no tRNS.)
 * @param bmpColorTableSize BMP color table size. (256 for CI8, 2 for monochrome.)
 * @param biClrUsed Number of used colors. (If negative, check all 256 colors.)
 */
void RpPngFormatTest::Compare_Palettes(
	const rp_image *img,
	const uint32_t *pBmpPalette,
	const tRNS_CI8_t *pBmpAlpha,
	int bmpColorTableSize,
	int biClrUsed)
{
	const uint32_t *pSrcPalette = static_cast<const uint32_t*>(img->palette());
	if (biClrUsed < 0) {
		biClrUsed = img->palette_len();
	}

	uint32_t xor_result = 0;
	for (int i = 0; i < biClrUsed; i++, pSrcPalette++, pBmpPalette++) {
		uint32_t bmp32 = le32_to_cpu(*pBmpPalette) & 0x00FFFFFF;
		if (pBmpAlpha) {
			// BMP tRNS chunk is specified.
			bmp32 |= pBmpAlpha->alpha[i] << 24;
		} else {
			// No tRNS chunk. Assume the color is opaque.
			bmp32 |= 0xFF000000;
		}
		xor_result |= (*pSrcPalette ^ bmp32);
	}
	EXPECT_EQ(0U, xor_result) << "CI8 rp_image's palette doesn't match BMP.";

	// Make sure the unused colors in the rp_image are all 0.
	if (biClrUsed < img->palette_len()) {
		uint32_t or_result = 0;
		for (int i = img->palette_len(); i > biClrUsed; i--, pSrcPalette++) {
			or_result |= *pSrcPalette;
		}
		EXPECT_EQ(0U, or_result) << "CI8 rp_image's palette doesn't have unused entries set to 0.";
	}

	// Make sure the unused colors in the BMP are all 0.
	if (biClrUsed < bmpColorTableSize) {
		uint32_t or_result = 0;
		for (int i = img->palette_len(); i > biClrUsed; i--, pBmpPalette++) {
			or_result |= *pBmpPalette;
		}
		EXPECT_EQ(0U, or_result) << "BMP's palette doesn't have unused entries set to 0.";
	}
}

/**
 * Compare a CI8 rp_image to an 8-bit CI8 bitmap.
 * @param img rp_image
 * @param pBits Bitmap image data.
 * @param pBmpPalette Bitmap palette.
 * @param pBmpAlpha BMP tRNS chunk from the test parameter. (If nullptr, no tRNS.)
 * @param biClrUsed Number of used colors. (If negative, check the entire palette.)
 */
void RpPngFormatTest::Compare_CI8_BMP8(
	const rp_image *img,
	const uint8_t *pBits,
	const uint32_t *pBmpPalette,
	const tRNS_CI8_t *pBmpAlpha,
	int biClrUsed)
{
	// BMP images are stored upside-down.
	// TODO: 8px row alignment?

	// To avoid calling EXPECT_EQ() for every single pixel,
	// XOR the two pixels together, then OR it here.
	// The eventual result should be 0 if the images are identical.
	uint32_t xor_result = 0;

	// Check the palette.
	ASSERT_NO_FATAL_FAILURE(Compare_Palettes(img, pBmpPalette, pBmpAlpha, 256, biClrUsed));

	// 256-color BMP images always have an internal width that's
	// a multiple of 8px. If the image isn't a multiple of 8px,
	// then we need to skip the extra pixels.
	int mod8 = img->width() % 8;
	if (mod8 != 0) {
		mod8 = 8 - mod8;
	}

	// Check the image data.
	xor_result = 0;
	const int width = img->width();
	for (int y = img->height()-1; y >= 0; y--) {
		// Do a full memcmp() instead of xoring bytes, since
		// each pixel is a single byte.
		const uint8_t *pSrc = static_cast<const uint8_t*>(img->scanLine(y));
		xor_result |= memcmp(pSrc, pBits, width);
		pBits += width;

		if (mod8 > 0) {
			// Image isn't a multiple of 8 pixels wide.
			// Make sure the extra pixels in the BMP image
			// are all 0.
			for (int x = mod8; x > 0; x--, pBits++) {
				xor_result |= *pBits;
			}
		}
	}
	EXPECT_EQ(0U, xor_result) << "CI8 rp_image's pixel data doesn't match CI8 BMP.";
}

/**
 * Compare an ARGB32 rp_image to an 8-bit CI8 bitmap.
 * NOTE: This should only happen if GDI+ decoded
 * a grayscale image to ARGB32, which seems to happen
 * on wine-1.9.18 and AppVeyor for some reason.
 * @param img rp_image
 * @param pBits Bitmap image data.
 * @param pBmpPalette Bitmap palette.
 */
void RpPngFormatTest::Compare_ARGB32_BMP8(
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

	// 256-color BMP images always have an internal width that's
	// a multiple of 8px. If the image isn't a multiple of 8px,
	// then we need to skip the extra pixels.
	int mod8 = img->width() % 8;
	if (mod8 != 0) {
		mod8 = 8 - mod8;
	}

	// Check the image data.
	xor_result = 0;
	for (int y = img->height()-1; y >= 0; y--) {
		// Compare 32-bit rp_image data to 8-bit BMP data.
		const uint32_t *pSrc = static_cast<const uint32_t*>(img->scanLine(y));
		for (int x = img->width()-1; x >= 0; x--, pSrc++, pBits++) {
			// Look up the 24-bit color entry from the bitmap's palette.
			uint32_t bmp32 = le32_to_cpu(pBmpPalette[*pBits]);
			// Pixel is fully opaque.
			bmp32 |= 0xFF000000;
			xor_result |= (*pSrc ^ bmp32);
		}

		if (mod8 > 0) {
			// Image isn't a multiple of 8 pixels wide.
			// Make sure the extra pixels in the BMP image
			// are all 0.
			for (int x = mod8; x > 0; x--, pBits++) {
				xor_result |= *pBits;
			}
		}
	}
	EXPECT_EQ(0U, xor_result) << "ARGB32 rp_image's pixel data doesn't match CI8 BMP.";
}

/**
 * Compare a CI8 rp_image to a 32-bit RGB bitmap.
 * wine-1.9.18 loads xterm-256color.CI8.tRNS.png as CI8
 * instead of as ARGB32 for some reason.
 * @param img rp_image
 * @param pBits Bitmap image data.
 */
void RpPngFormatTest::Compare_CI8_BMP32(
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

	// Get the rp_image palette.
	const uint32_t *pSrcPalette = img->palette();
	ASSERT_TRUE(pSrcPalette != nullptr);
	ASSERT_EQ(256, img->palette_len());

	// Check the image data.
	xor_result = 0;
	for (int y = img->height()-1; y >= 0; y--) {
		// Compare 8-bit rp_image data to 24-bit BMP data.
		const uint8_t *pSrc = static_cast<const uint8_t*>(img->scanLine(y));
		for (int x = img->width()-1; x >= 0; x--, pSrc++, pBmp32++) {
			// Look up the 32-bit palette entry from the rp_image.
			const uint32_t src32 = pSrcPalette[*pSrc];
			// BMP uses little-endian, so byteswapping is needed.
			const uint32_t bmp32 = le32_to_cpu(*pBmp32);

			// Compare the pixels
			xor_result |= (src32 ^ bmp32);
		}
	}
	EXPECT_EQ(0U, xor_result) << "CI8 rp_image's pixel data doesn't match ARGB32 BMP.";
}

/**
 * Compare a CI8 rp_image to a monochrome bitmap.
 * @param img rp_image
 * @param pBits Bitmap image data.
 * @param pBmpPalette Bitmap palette.
 * @param pBmpAlpha BMP tRNS chunk from the test parameter. (If nullptr, no tRNS.)
 * @param biClrUsed Number of used colors. (If negative, check the entire palette.)
 */
void RpPngFormatTest::Compare_CI8_BMP1(
	const rp_image *img,
	const uint8_t *pBits,
	const uint32_t *pBmpPalette,
	const tRNS_CI8_t *pBmpAlpha,
	int biClrUsed)
{
	// BMP images are stored upside-down.
	// TODO: 8px row alignment?

	// To avoid calling EXPECT_EQ() for every single pixel,
	// XOR the two pixels together, then OR it here.
	// The eventual result should be 0 if the images are identical.
	uint32_t xor_result = 0;

	// Check the palette.
	ASSERT_NO_FATAL_FAILURE(Compare_Palettes(img, pBmpPalette, pBmpAlpha, 2, biClrUsed));

	// Monochrome BMP images always have an internal width that's
	// a multiple of 32px. (stride == 4) If the image isn't a
	// multiple of 32px, then we need to skip the extra pixels.
	int mod32 = img->width() % 32;
	if (mod32 != 0) {
		mod32 = 32 - mod32;
	}

	// Check the image data.
	xor_result = 0;
	for (int y = img->height()-1; y >= 0; y--) {
		// The source image has 8 pixels in each byte,
		// so we have to compare the entire line manually.
		const uint8_t *pSrc = static_cast<const uint8_t*>(img->scanLine(y));
		for (int x = img->width(); x > 0; x -= 8, pBits++) {
			uint8_t mono_pxs = *pBits;
			for (int bit = (x > 8 ? 8 : x); bit > 0; bit--, pSrc++) {
				xor_result |= (mono_pxs >> 7) ^ *pSrc;
				mono_pxs <<= 1;
			}
		}

		if (mod32 > 0) {
			// Skip the remaining bytes.
			// NOTE: The unused pixels aren't necessarily set to 0,
			// so we can't check them, unlike CI8 bitmaps.
			pBits += (mod32 / 8);
		}
	}
	EXPECT_EQ(0U, xor_result) << "CI8 rp_image's pixel data doesn't match monochrome BMP.";
}

/**
 * Run an RpImageLoader test.
 */
TEST_P(RpPngFormatTest, loadTest)
{
	const RpPngFormatTest_mode &mode = GetParam();

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
	unique_ptr<IRpFile> png_mem_file(new RpMemFile(m_png_buf.data(), m_png_buf.size()));
	ASSERT_TRUE(png_mem_file.get() != nullptr);
	ASSERT_TRUE(png_mem_file->isOpen());

	// Load the PNG image from memory.
	unique_ptr<rp_image> img(RpImageLoader::load(png_mem_file.get()));
	ASSERT_TRUE(img.get() != nullptr) << "RpImageLoader failed to load the image.";

	// Check the rp_image parameters.
	EXPECT_EQ((int)mode.ihdr.width, img->width()) << "rp_image width is incorrect.";
	EXPECT_EQ((int)mode.ihdr.height, img->height()) << "rp_image height is incorrect.";

	// Check the image format.
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
			// Comparing an ARGB32 rp_image to an ARGB32 bitmap.
			// TODO: Check the bitfield masks?
			ASSERT_NO_FATAL_FAILURE(Compare_ARGB32_BMP32(img.get(), pBits));
		} else if (bih.biBitCount == 8 && bih.biCompression == BI_RGB) {
			// Comparing an ARGB32 rp_image to a CI8 bitmap.
			// NOTE: This should only happen if GDI+ decoded
			// a grayscale image to ARGB32, which seems to happen
			// on wine-1.9.18 and AppVeyor for some reason.
			// FIXME: This may fail on aligned architectures.
			const uint32_t *pBmpPalette = reinterpret_cast<const uint32_t*>(
				m_bmp_buf.data() + sizeof(BITMAPFILEHEADER) + bih.biSize);
			ASSERT_NO_FATAL_FAILURE(Compare_ARGB32_BMP8(img.get(), pBits, pBmpPalette));
		} else {
			// Unsupported comparison.
			ASSERT_TRUE(false) << "Image format comparison isn't supported.";
		}
	} else if (img->format() == rp_image::FORMAT_CI8) {
		if (bih.biBitCount == 8 && bih.biCompression == BI_RGB) {
			// 256-color image. Get the palette.
			// NOTE: rp_image's palette length is always 256, which may be
			// greater than the used colors in the BMP.
			ASSERT_GE(img->palette_len(), (int)bih.biClrUsed)
				<< "BMP palette is larger than the rp_image palette.";

			// NOTE: Palette has 32-bit entries, but the alpha channel is ignored.
			// FIXME: This may fail on aligned architectures.
			const uint32_t *pBmpPalette = reinterpret_cast<const uint32_t*>(
				m_bmp_buf.data() + sizeof(BITMAPFILEHEADER) + bih.biSize);
			const tRNS_CI8_t *pBmpAlpha = (mode.has_bmp_tRNS ? &mode.bmp_tRNS : nullptr);
			ASSERT_NO_FATAL_FAILURE(Compare_CI8_BMP8(img.get(), pBits,
				pBmpPalette, pBmpAlpha, bih.biClrUsed));
		} else if (bih.biBitCount == 32 && bih.biCompression == BI_BITFIELDS) {
			// Comparing a CI8 rp_image to an ARGB32 bitmap.
			// wine-1.9.18 loads xterm-256color.CI8.tRNS.png as CI8
			// instead of as ARGB32 for some reason.
			ASSERT_NO_FATAL_FAILURE(Compare_CI8_BMP32(img.get(), pBits));
		} else if (bih.biBitCount == 1 && bih.biCompression == BI_RGB) {
			// Monochrome bitmap.

			// NOTE: The color table does have two colors, so we should
			// compare it to the rp_image palette.
			ASSERT_GE(img->palette_len(), (int)bih.biClrUsed)
				<< "BMP palette is larger than the rp_image palette.";

			// 256-color image. Get the palette.
			// NOTE: rp_image's palette length is always 256, which may be
			// greater than the used colors in the BMP.
			ASSERT_GE(img->palette_len(), (int)bih.biClrUsed)
				<< "BMP palette is larger than the rp_image palette.";

			// NOTE: Palette has 32-bit entries, but the alpha channel is ignored.
			// FIXME: This may fail on aligned architectures.
			const uint32_t *pBmpPalette = reinterpret_cast<const uint32_t*>(
				m_bmp_buf.data() + sizeof(BITMAPFILEHEADER) + bih.biSize);
			const tRNS_CI8_t *pBmpAlpha = (mode.has_bmp_tRNS ? &mode.bmp_tRNS : nullptr);
			ASSERT_NO_FATAL_FAILURE(Compare_CI8_BMP1(img.get(), pBits,
				pBmpPalette, pBmpAlpha, bih.biClrUsed));
		} else {
			// Unsupported comparison.
			ASSERT_TRUE(false) << "Image format comparison isn't supported.";
		}
	} else {
		// Unsupported comparison.
		ASSERT_TRUE(false) << "Image format comparison isn't supported.";
	}
}

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string RpPngFormatTest::test_case_suffix_generator(const ::testing::TestParamInfo<RpPngFormatTest_mode> &info)
{
	rp_string suffix = info.param.png_filename;

	// Replace all non-alphanumeric characters with '_'.
	// See gtest-param-util.h::IsValidParamName().
	for (int i = (int)suffix.size()-1; i >= 0; i--) {
		rp_char chr = suffix[i];
		if (!isalnum(chr) && chr != '_') {
			suffix[i] = '_';
		}
	}

	// TODO: rp_string_to_ascii()?
	return rp_string_to_utf8(suffix);
}

// Test cases.

// NOTE: 32-bit ARGB bitmaps use BI_BITFIELDS.
// 24-bit RGB bitmaps use BI_RGB.
// 256-color bitmaps use BI_RGB, unless they're RLE-compressed,
// in which case they use BI_RLE8.

// TODO: Test PNG_COLOR_TYPE_GRAY_ALPHA, paletted images
// with alphatransparency, and gray/paletted images using
// 1-, 2-, 4-, and 8 bits per channel.

// gl_triangle PNG IHDR chunks.
static const PNG_IHDR_t gl_triangle_RGB24_IHDR =
	{400, 352, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0};
static const PNG_IHDR_t gl_triangle_ARGB32_IHDR =
	{400, 352, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0, 0};
static const PNG_IHDR_t gl_triangle_gray_IHDR =
	{400, 352, 8, PNG_COLOR_TYPE_GRAY, 0, 0, 0};
static const PNG_IHDR_t gl_triangle_gray_alpha_IHDR =
	{400, 352, 8, PNG_COLOR_TYPE_GRAY_ALPHA, 0, 0, 0};

// gl_triangle BITMAPINFOHEADER structs.
static const BITMAPINFOHEADER gl_triangle_RGB24_BIH =
	{sizeof(BITMAPINFOHEADER),
		400, 352, 1, 24, BI_RGB, 400*352*(24/8),
		3936, 3936, 0, 0};
static const BITMAPINFOHEADER gl_triangle_RGB24_tRNS_BIH =
	{sizeof(BITMAPINFOHEADER),
		400, 352, 1, 32, BI_BITFIELDS, 400*352*(32/8),
		3936, 3936, 0, 0};
static const BITMAPINFOHEADER gl_triangle_ARGB32_BIH =
	{sizeof(BITMAPINFOHEADER),
		400, 352, 1, 32, BI_BITFIELDS, 400*352*(32/8),
		3936, 3936, 0, 0};
static const BITMAPINFOHEADER gl_triangle_gray_BIH =
	{sizeof(BITMAPINFOHEADER),
		400, 352, 1, 8, BI_RGB, 400*352,
		3936, 3936, 256, 256};
// BMP doesn't support gray+alpha, so we're using ARGB32.
static const BITMAPINFOHEADER gl_triangle_gray_alpha_BIH =
	{sizeof(BITMAPINFOHEADER),
		400, 352, 1, 32, BI_BITFIELDS, 400*352*(32/8),
		3936, 3936, 0, 0};

// gl_triangle PNG image tests.
INSTANTIATE_TEST_CASE_P(gl_triangle_png, RpPngFormatTest,
	::testing::Values(
		RpPngFormatTest_mode(
			_RP("gl_triangle.RGB24.png"),
			_RP("gl_triangle.RGB24.bmp.gz"),
			gl_triangle_RGB24_IHDR,
			gl_triangle_RGB24_BIH,
			rp_image::FORMAT_ARGB32),
		RpPngFormatTest_mode(
			_RP("gl_triangle.RGB24.tRNS.png"),
			_RP("gl_triangle.RGB24.tRNS.bmp.gz"),
			gl_triangle_RGB24_IHDR,
			gl_triangle_RGB24_tRNS_BIH,
			rp_image::FORMAT_ARGB32),
		RpPngFormatTest_mode(
			_RP("gl_triangle.ARGB32.png"),
			_RP("gl_triangle.ARGB32.bmp.gz"),
			gl_triangle_ARGB32_IHDR,
			gl_triangle_ARGB32_BIH,
			rp_image::FORMAT_ARGB32),
		RpPngFormatTest_mode(
			_RP("gl_triangle.gray.png"),
			_RP("gl_triangle.gray.bmp.gz"),
			gl_triangle_gray_IHDR,
			gl_triangle_gray_BIH,
			rp_image::FORMAT_CI8),
		RpPngFormatTest_mode(
			_RP("gl_triangle.gray.alpha.png"),
			_RP("gl_triangle.gray.alpha.bmp.gz"),
			gl_triangle_gray_alpha_IHDR,
			gl_triangle_gray_alpha_BIH,
			rp_image::FORMAT_ARGB32)
		)
	, RpPngFormatTest::test_case_suffix_generator);

// gl_quad PNG IHDR chunks.
static const PNG_IHDR_t gl_quad_RGB24_IHDR =
	{480, 384, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0};
static const PNG_IHDR_t gl_quad_ARGB32_IHDR =
	{480, 384, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0, 0};
static const PNG_IHDR_t gl_quad_gray_IHDR =
	{480, 384, 8, PNG_COLOR_TYPE_GRAY, 0, 0, 0};
static const PNG_IHDR_t gl_quad_gray_alpha_IHDR =
	{480, 384, 8, PNG_COLOR_TYPE_GRAY_ALPHA, 0, 0, 0};

// gl_quad BITMAPINFOHEADER structs.
static const BITMAPINFOHEADER gl_quad_RGB24_BIH =
	{sizeof(BITMAPINFOHEADER),
		480, 384, 1, 24, BI_RGB, 480*384*(24/8),
		3936, 3936, 0, 0};
static const BITMAPINFOHEADER gl_quad_RGB24_tRNS_BIH =
	{sizeof(BITMAPINFOHEADER),
		480, 384, 1, 32, BI_BITFIELDS, 480*384*(32/8),
		3936, 3936, 0, 0};
static const BITMAPINFOHEADER gl_quad_ARGB32_BIH =
	{sizeof(BITMAPINFOHEADER),
		480, 384, 1, 32, BI_BITFIELDS, 480*384*(32/8),
		3936, 3936, 0, 0};
static const BITMAPINFOHEADER gl_quad_gray_BIH =
	{sizeof(BITMAPINFOHEADER),
		480, 384, 1, 8, BI_RGB, 480*384,
		3936, 3936, 256, 256};
// BMP doesn't support gray+alpha, so we're using ARGB32.
static const BITMAPINFOHEADER gl_quad_gray_alpha_BIH =
	{sizeof(BITMAPINFOHEADER),
		480, 384, 1, 32, BI_BITFIELDS, 480*384*(32/8),
		3936, 3936, 0, 0};

// gl_quad PNG image tests.
INSTANTIATE_TEST_CASE_P(gl_quad_png, RpPngFormatTest,
	::testing::Values(
		RpPngFormatTest_mode(
			_RP("gl_quad.RGB24.png"),
			_RP("gl_quad.RGB24.bmp.gz"),
			gl_quad_RGB24_IHDR,
			gl_quad_RGB24_BIH,
			rp_image::FORMAT_ARGB32),
		RpPngFormatTest_mode(
			_RP("gl_quad.RGB24.tRNS.png"),
			_RP("gl_quad.RGB24.tRNS.bmp.gz"),
			gl_quad_RGB24_IHDR,
			gl_quad_RGB24_tRNS_BIH,
			rp_image::FORMAT_ARGB32),
		RpPngFormatTest_mode(
			_RP("gl_quad.ARGB32.png"),
			_RP("gl_quad.ARGB32.bmp.gz"),
			gl_quad_ARGB32_IHDR,
			gl_quad_ARGB32_BIH,
			rp_image::FORMAT_ARGB32),
		RpPngFormatTest_mode(
			_RP("gl_quad.gray.png"),
			_RP("gl_quad.gray.bmp.gz"),
			gl_quad_gray_IHDR,
			gl_quad_gray_BIH,
			rp_image::FORMAT_CI8),
		RpPngFormatTest_mode(
			_RP("gl_quad.gray.alpha.png"),
			_RP("gl_quad.gray.alpha.bmp.gz"),
			gl_quad_gray_alpha_IHDR,
			gl_quad_gray_alpha_BIH,
			rp_image::FORMAT_ARGB32)
		)
	, RpPngFormatTest::test_case_suffix_generator);

// xterm 256-color PNG IHDR chunks.
static const PNG_IHDR_t xterm_256color_CI8_IHDR =
	{608, 720, 8, PNG_COLOR_TYPE_PALETTE, 0, 0, 0};
static const PNG_IHDR_t xterm_256color_CI8_tRNS_IHDR =
	{608, 720, 8, PNG_COLOR_TYPE_PALETTE, 0, 0, 0};

// xterm 256-color BITMAPINFOHEADER structs.
static const BITMAPINFOHEADER xterm_256color_CI8_BIH =
	{sizeof(BITMAPINFOHEADER),
		608, 720, 1, 8, BI_RGB, 608*720,
		3936, 3936, 253, 253};
#if defined(HAVE_PNG)
static const BITMAPINFOHEADER xterm_256color_CI8_tRNS_BIH =
	{sizeof(BITMAPINFOHEADER),
		608, 720, 1, 8, BI_RGB, 608*720,
		3936, 3936, 254, 254};
#elif defined(_WIN32)
// GDI+ converts PNG_COLOR_TYPE_PALETTE + tRNS to 32-bit ARGB.
static const BITMAPINFOHEADER xterm_256color_CI8_tRNS_gdip_BIH =
	{sizeof(BITMAPINFOHEADER),
		608, 720, 1, 32, BI_BITFIELDS, 608*720*(32/8),
		3936, 3936, 0, 0};
#else
#error Missing xterm-256color.CI8.tRNS.png test case.
#endif

#ifdef HAVE_PNG
// tRNS chunk for CI8.tRNS BMPs.
static const tRNS_CI8_t xterm_256color_CI8_tRNS_bmp_tRNS = {{
	0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
}};
#endif /* HAVE_PNG */

// xterm 256-color PNG image tests.
INSTANTIATE_TEST_CASE_P(xterm_256color_png, RpPngFormatTest,
	::testing::Values(
		RpPngFormatTest_mode(
			_RP("xterm-256color.CI8.png"),
			_RP("xterm-256color.CI8.bmp.gz"),
			xterm_256color_CI8_IHDR,
			xterm_256color_CI8_BIH,
			rp_image::FORMAT_CI8)
		)
	, RpPngFormatTest::test_case_suffix_generator);

// xterm 256-color PNG image tests with transparency.
// FIXME: MSVC (2010, others) doesn't support #if/#endif within macros.
// https://connect.microsoft.com/VisualStudio/Feedback/Details/2084691
#if defined(HAVE_PNG)
INSTANTIATE_TEST_CASE_P(xterm_256color_tRNS_png, RpPngFormatTest,
	::testing::Values(
		RpPngFormatTest_mode(
			_RP("xterm-256color.CI8.tRNS.png"),
			_RP("xterm-256color.CI8.tRNS.bmp.gz"),
			xterm_256color_CI8_tRNS_IHDR,
			xterm_256color_CI8_tRNS_BIH,
			xterm_256color_CI8_tRNS_bmp_tRNS,
			rp_image::FORMAT_CI8)
		)
	, RpPngFormatTest::test_case_suffix_generator);
#elif defined(_WIN32)
INSTANTIATE_TEST_CASE_P(xterm_256color_tRNS_png, RpPngFormatTest,
	::testing::Values(
		RpPngFormatTest_mode(
			_RP("xterm-256color.CI8.tRNS.png"),
			_RP("xterm-256color.CI8.tRNS.gdip.bmp.gz"),
			xterm_256color_CI8_tRNS_IHDR,
			xterm_256color_CI8_tRNS_gdip_BIH,
			rp_image::FORMAT_ARGB32)
		)
	, RpPngFormatTest::test_case_suffix_generator);
#endif

/** Low color depth and odd width tests. **/

// 135x270 with 16-color palette.
static const PNG_IHDR_t odd_width_16color_CI4_IHDR =
	{135, 270, 4, PNG_COLOR_TYPE_PALETTE, 0, 0, 0};

// 135x270 with 16-color palette.
// NOTE: Image actually has a 256-color palette because
// RpPng converts it to 8bpp, since RpImage does not
// support CI4 images.
// NOTE 2: Image width is internally a multiple of 8.
static const BITMAPINFOHEADER odd_width_16color_CI8_BIH =
	{sizeof(BITMAPINFOHEADER),
		135, 270, 1, 8, BI_RGB, (135+1)*270,
		3936, 3936, 16, 16};

// odd-width_16color PNG image tests.
INSTANTIATE_TEST_CASE_P(odd_width_16color_png, RpPngFormatTest,
	::testing::Values(
		// TODO: Use a CI4 BMP?
		RpPngFormatTest_mode(
			_RP("odd-width.16color.CI4.png"),
			_RP("odd-width.16color.CI8.bmp.gz"),
			odd_width_16color_CI4_IHDR,
			odd_width_16color_CI8_BIH,
			rp_image::FORMAT_CI8)
		)
	, RpPngFormatTest::test_case_suffix_generator);

/** Monochrome tests. **/

static const PNG_IHDR_t happy_mac_mono_IHDR =
	{512, 342, 1, PNG_COLOR_TYPE_PALETTE, 0, 0, 0};
static const PNG_IHDR_t happy_mac_mono_odd_size_IHDR =
	{75, 73, 1, PNG_COLOR_TYPE_PALETTE, 0, 0, 0};

static const BITMAPINFOHEADER happy_mac_mono_BIH =
	{sizeof(BITMAPINFOHEADER),
		512, 342, 1, 1, BI_RGB, 512*342/8,
		3936, 3936, 2, 2};
// NOTE: Monochrome bitmaps always have a stride of 4 bytes.
// For 75px, that increases it to 96px wide.
// (Internal width is a multiple of 32px.)
static const BITMAPINFOHEADER happy_mac_mono_odd_size_BIH =
	{sizeof(BITMAPINFOHEADER),
		75, 73, 1, 1, BI_RGB, 96*73/8,
		3936, 3936, 2, 2};

// happy_mac_mono PNG image tests.
INSTANTIATE_TEST_CASE_P(happy_mac_mono_png, RpPngFormatTest,
	::testing::Values(
		// Full 512x342 version.
		RpPngFormatTest_mode(
			_RP("happy-mac.mono.png"),
			_RP("happy-mac.mono.bmp.gz"),
			happy_mac_mono_IHDR,
			happy_mac_mono_BIH,
			rp_image::FORMAT_CI8),

		// Cropped 75x73 version.
		RpPngFormatTest_mode(
			_RP("happy-mac.mono.odd-size.png"),
			_RP("happy-mac.mono.odd-size.bmp.gz"),
			happy_mac_mono_odd_size_IHDR,
			happy_mac_mono_odd_size_BIH,
			rp_image::FORMAT_CI8)
		)
	, RpPngFormatTest::test_case_suffix_generator);

} }
