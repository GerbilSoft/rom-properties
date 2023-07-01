/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * ImageDecoderTest.cpp: ImageDecoder class test.                          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"
#include "config.libromdata.h"
#include "config.librptexture.h"

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// zlib and libpng
#include <zlib.h>
#ifdef HAVE_PNG
#  include <png.h>
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

// librpbase, librpfile
#include "common.h"
#include "librpbase/img/RpPng.hpp"
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "libromdata/Other/RpTextureWrapper.hpp"
#include "librpfile/MemFile.hpp"
#include "librpfile/RpFile.hpp"
#include "librpfile/FileSystem.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// librptexture
#include "librptexture/img/rp_image.hpp"
#ifdef _WIN32
// rp_image backend registration.
#  include "librptexture/img/RpGdiplusBackend.hpp"
#endif /* _WIN32 */
using namespace LibRpTexture;

// RomDataFactory to load test files.
#include "RomDataFactory.hpp"

// C includes.
#include <stdint.h>
#include <stdlib.h>

// C includes. (C++ namespace)
#include "ctypex.h"
#include <cstring>

// C++ includes.
#include <functional>
#include <memory>
#include <string>
using std::string;
using std::unique_ptr;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData { namespace Tests {

struct ImageDecoderTest_mode
{
	string dds_gz_filename;
	string png_filename;
	string expected_pixel_format;	// for RpTextureWrapper only
	RomData::ImageType imgType;

	ImageDecoderTest_mode(
		const char *dds_gz_filename,
		const char *png_filename,
		RomData::ImageType imgType = RomData::IMG_INT_IMAGE
		)
		: dds_gz_filename(dds_gz_filename)
		, png_filename(png_filename)
		, imgType(imgType)
	{ }

	ImageDecoderTest_mode(
		const char *dds_gz_filename,
		const char *png_filename,
		const char *expected_pixel_format,
		RomData::ImageType imgType = RomData::IMG_INT_IMAGE
		)
		: dds_gz_filename(dds_gz_filename)
		, png_filename(png_filename)
		, expected_pixel_format(expected_pixel_format)
		, imgType(imgType)
	{ }

	// May be required for MSVC 2010?
	ImageDecoderTest_mode(const ImageDecoderTest_mode &other)
		: dds_gz_filename(other.dds_gz_filename)
		, png_filename(other.png_filename)
		, expected_pixel_format(other.expected_pixel_format)
		, imgType(other.imgType)
	{ }

	// Required for MSVC 2010.
	ImageDecoderTest_mode &operator=(const ImageDecoderTest_mode &other)
	{
		dds_gz_filename = other.dds_gz_filename;
		png_filename = other.png_filename;
		expected_pixel_format = other.expected_pixel_format;
		imgType = other.imgType;
		return *this;
	}
};

// Maximum file size for images.
static const size_t MAX_DDS_IMAGE_FILESIZE = 12*1024*1024;
static const size_t MAX_PNG_IMAGE_FILESIZE =  2*1024*1024;

class ImageDecoderTest : public ::testing::TestWithParam<ImageDecoderTest_mode>
{
	protected:
		ImageDecoderTest()
			: ::testing::TestWithParam<ImageDecoderTest_mode>()
			, m_gzDds(nullptr)
			, m_f_dds(nullptr)
			, m_romData(nullptr)
		{
#ifdef _WIN32
			// Register RpGdiplusBackend.
			// TODO: Static initializer somewhere?
			rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
#endif /* _WIN32 */
		}

		void SetUp(void) final;
		void TearDown(void) final;

	public:
		/**
		 * Compare two rp_image objects.
		 * If either rp_image is CI8, a copy of the image
		 * will be created in ARGB32 for comparison purposes.
		 * @param pImgExpected	[in] Expected image data.
		 * @param pImgActual	[in] Actual image data.
		 */
		static void Compare_RpImage(
			const rp_image *pImgExpected,
			const rp_image *pImgActual);

		// Number of iterations for benchmarks.
		static const unsigned int BENCHMARK_ITERATIONS = 1000;
		static const unsigned int BENCHMARK_ITERATIONS_BC7 = 100;

	public:
		// Image buffers.
		ao::uvector<uint8_t> m_dds_buf;
		ao::uvector<uint8_t> m_png_buf;

		// gzip file handle for .dds.gz.
		// Placed here so it can be freed by TearDown() if necessary.
		gzFile m_gzDds;

		// RomData class pointer for .dds.gz.
		// Placed here so it can be freed by TearDown() if necessary.
		// The underlying MemFile is here as well, since we can't
		// delete it before deleting the RomData object.
		MemFile *m_f_dds;
		RomData *m_romData;

	public:
		/** Test case parameters. **/

		/**
		 * Test case suffix generator.
		 * @param info Test parameter information.
		 * @return Test case suffix.
		 */
		static string test_case_suffix_generator(const ::testing::TestParamInfo<ImageDecoderTest_mode> &info);

		/**
		 * Replace slashes with backslashes on Windows.
		 * @param path Pathname.
		 */
		static inline void replace_slashes(string &path);

		/**
		 * Internal test function.
		 */
		void decodeTest_internal(void);

		/**
		 * Internal benchmark function.
		 */
		void decodeBenchmark_internal(void);
};

/**
 * Formatting function for ImageDecoderTest.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const ImageDecoderTest_mode& mode)
{
	return os << mode.dds_gz_filename;
};

/**
 * Replace slashes with backslashes on Windows.
 * @param path Pathname.
 */
inline void ImageDecoderTest::replace_slashes(string &path)
{
#ifdef _WIN32
	std::replace(path.begin(), path.end(), '/', '\\');
#else
	// Nothing to do here...
	RP_UNUSED(path);
#endif /* _WIN32 */
}

/**
 * SetUp() function.
 * Run before each test.
 */
void ImageDecoderTest::SetUp(void)
{
	if (::testing::UnitTest::GetInstance()->current_test_info()->value_param() == nullptr) {
		// Not a parameterized test.
		return;
	}

	// Parameterized test.
	const ImageDecoderTest_mode &mode = GetParam();

	// Open the gzipped DDS texture file being tested.
	string path;
	path += mode.dds_gz_filename;
	replace_slashes(path);
	m_gzDds = gzopen(path.c_str(), "rb");
	ASSERT_TRUE(m_gzDds != nullptr) << "gzopen() failed to open the test file.";

	// Get the decompressed file size.
	// gzseek() does not support SEEK_END.
	// Read through the file until we hit an EOF.
	// NOTE: We could optimize this by reading the uncompressed
	// file size if gzdirect() == 1, but this is a test case,
	// so it doesn't really matter.
	uint8_t buf[4096];
	uint32_t ddsSize = 0;
	while (!gzeof(m_gzDds)) {
		int sz_read = gzread(m_gzDds, buf, sizeof(buf));
		ASSERT_NE(sz_read, -1) << "gzread() failed.";
		ddsSize += sz_read;
	}
	gzrewind(m_gzDds);

	/* FIXME: Per-type minimum sizes.
	 * This fails on some very small SVR files.
	ASSERT_GT(ddsSize, 4+sizeof(DDS_HEADER))
		<< "DDS image is too small.";
	*/
	ASSERT_LE(ddsSize, MAX_DDS_IMAGE_FILESIZE)
		<< "DDS image is too big.";

	// Read the DDS image into memory.
	m_dds_buf.resize(ddsSize);
	ASSERT_EQ((size_t)ddsSize, m_dds_buf.size());
	int sz = gzread(m_gzDds, m_dds_buf.data(), ddsSize);
	gzclose_r(m_gzDds);
	m_gzDds = nullptr;

	ASSERT_EQ(ddsSize, (uint32_t)sz) << "Error loading DDS image file: short read";

	// Open the PNG image file being tested.
	path.clear();
	path += mode.png_filename;
	replace_slashes(path);
	unique_RefBase<RpFile> file(new RpFile(path, RpFile::FM_OPEN_READ));
	ASSERT_TRUE(file->isOpen()) << "Error loading PNG image file: " << strerror(file->lastError());

	// Maximum image size.
	ASSERT_LE(file->size(), MAX_PNG_IMAGE_FILESIZE) << "PNG image is too big.";

	// Read the PNG image into memory.
	const size_t pngSize = static_cast<size_t>(file->size());
	m_png_buf.resize(pngSize);
	ASSERT_EQ(pngSize, m_png_buf.size());
	size_t readSize = file->read(m_png_buf.data(), pngSize);
	ASSERT_EQ(pngSize, readSize) << "Error loading PNG image file: short read";
}

/**
 * TearDown() function.
 * Run after each test.
 */
void ImageDecoderTest::TearDown(void)
{
	UNREF_AND_NULL(m_romData);
	UNREF_AND_NULL(m_f_dds);

	if (m_gzDds) {
		gzclose_r(m_gzDds);
		m_gzDds = nullptr;
	}
}

struct RpImageUnrefDeleter {
	void operator()(rp_image *img) {
		UNREF(img);
	}
};

/**
 * Compare two rp_image objects.
 * If either rp_image is CI8, a copy of the image
 * will be created in ARGB32 for comparison purposes.
 * @param pImgExpected	[in] Expected image data.
 * @param pImgActual	[in] Actual image data.
 */
void ImageDecoderTest::Compare_RpImage(
	const rp_image *pImgExpected,
	const rp_image *pImgActual)
{
	// Make sure we have two ARGB32 images with equal sizes.
	ASSERT_TRUE(pImgExpected->isValid()) << "pImgExpected is not valid.";
	ASSERT_TRUE(pImgActual->isValid())   << "pImgActual is not valid.";
	ASSERT_EQ(pImgExpected->width(),  pImgActual->width())  << "Image sizes don't match.";
	ASSERT_EQ(pImgExpected->height(), pImgActual->height()) << "Image sizes don't match.";

	// Ensure we delete temporary images if they're created.
	unique_ptr<rp_image, RpImageUnrefDeleter> tmpImg_expected(nullptr, RpImageUnrefDeleter());
	unique_ptr<rp_image, RpImageUnrefDeleter> tmpImg_actual(nullptr, RpImageUnrefDeleter());

	switch (pImgExpected->format()) {
		case rp_image::Format::ARGB32:
			// No conversion needed.
			break;

		case rp_image::Format::CI8:
			// Convert to ARGB32.
			tmpImg_expected.reset(pImgExpected->dup_ARGB32());
			ASSERT_TRUE(tmpImg_expected != nullptr);
			ASSERT_TRUE(tmpImg_expected->isValid());
			pImgExpected = tmpImg_expected.get();
			break;

		default:
			ASSERT_TRUE(false) << "pImgExpected: Invalid pixel format for this test.";
			break;
	}

	switch (pImgActual->format()) {
		case rp_image::Format::ARGB32:
			// No conversion needed.
			break;

		case rp_image::Format::CI8:
			// Convert to ARGB32.
			tmpImg_actual.reset(pImgActual->dup_ARGB32());
			ASSERT_TRUE(tmpImg_actual != nullptr);
			ASSERT_TRUE(tmpImg_actual->isValid());
			pImgActual = tmpImg_actual.get();
			break;

		default:
			ASSERT_TRUE(false) << "pImgActual: Invalid pixel format for this test.";
			break;
	}

	// Compare the two images.
	// TODO: rp_image::operator==()?
	const uint32_t *pBitsExpected = static_cast<const uint32_t*>(pImgExpected->bits());
	const uint32_t *pBitsActual   = static_cast<const uint32_t*>(pImgActual->bits());
	const int stride_diff_exp = (pImgExpected->stride() - pImgExpected->row_bytes()) / sizeof(uint32_t);
	const int stride_diff_act = (pImgActual->stride() - pImgActual->row_bytes()) / sizeof(uint32_t);
	const unsigned int width  = static_cast<unsigned int>(pImgExpected->width());
	const unsigned int height = static_cast<unsigned int>(pImgExpected->height());
	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++, pBitsExpected++, pBitsActual++) {
			if (*pBitsExpected != *pBitsActual) {
				printf("ERR: (%u,%u): expected %08X, got %08X\n",
					x, y, *pBitsExpected, *pBitsActual);
			}
			ASSERT_EQ(*pBitsExpected, *pBitsActual) <<
				"Decoded image does not match the expected PNG image.";
		}
		pBitsExpected += stride_diff_exp;
		pBitsActual   += stride_diff_act;
	}
}

/**
 * Internal test function.
 */
void ImageDecoderTest::decodeTest_internal(void)
{
	// Parameterized test.
	const ImageDecoderTest_mode &mode = GetParam();

	// Load the PNG image.
	unique_RefBase<MemFile> f_png(new MemFile(m_png_buf.data(), m_png_buf.size()));
	ASSERT_TRUE(f_png->isOpen()) << "Could not create MemFile for the PNG image.";
	unique_ptr<rp_image, RpImageUnrefDeleter> img_png(RpPng::load(f_png.get()), RpImageUnrefDeleter());
	ASSERT_NE(img_png,nullptr) << "Could not load the PNG image as rp_image.";
	ASSERT_TRUE(img_png->isValid()) << "Could not load the PNG image as rp_image.";

	// Open the image as an IRpFile.
	m_f_dds = new MemFile(m_dds_buf.data(), m_dds_buf.size());
	ASSERT_TRUE(m_f_dds->isOpen()) << "Could not create MemFile for the DDS image.";
	m_f_dds->setFilename(mode.dds_gz_filename);

	// Load the image file.
	m_romData = RomDataFactory::create(m_f_dds);
	ASSERT_NE(m_romData, nullptr) << "Could not load the DDS image.";
	ASSERT_TRUE(m_romData->isValid()) << "Could not load the DDS image.";
	ASSERT_TRUE(m_romData->isOpen()) << "Could not load the DDS image.";

	// Get the DDS image as an rp_image.
	const rp_image *const img_dds = m_romData->image(mode.imgType);
	ASSERT_NE(img_dds, nullptr) << "Could not load the DDS image as rp_image.";

	// Get the image again.
	// The pointer should be identical to the first one.
	const rp_image *const img_dds_2 = m_romData->image(mode.imgType);
	EXPECT_EQ(img_dds, img_dds_2) << "Retrieving the image twice resulted in a different rp_image object.";

	// Verify the pixel format.
	if (!mode.expected_pixel_format.empty()) {
		// This must be RpTextureWrapper.
		// TODO: Support for other RomData subclasses.
		// NOTE: Using static_cast<> so we don't have to export RpTextureWrapper's vtable.
		const RpTextureWrapper *const rptw = static_cast<RpTextureWrapper*>(m_romData);
		EXPECT_NE(rptw, nullptr);
		if (rptw) {
			// To avoid having to add test-only functions to RpTextureWrapper,
			// we'll search the RomFields for the matching fields.

			// NOTE: The string is localized, but our Google Test initializer
			// sets LC_ALL=C, which disables localization.
			const char *actual_pixel_format = nullptr;
			const RomFields *const fields = rptw->fields();
			for (auto iter = fields->cbegin(); iter != fields->cend(); ++iter) {
				if (iter->type == RomFields::RFT_STRING && !strcmp(iter->name, "Pixel Format")) {
					// Found the DX10 format.
					actual_pixel_format = iter->data.str;
					break;
				}
			}

			if (actual_pixel_format && !strcmp(actual_pixel_format, "DX10")) {
				// Find "DX10 Format".
				for (auto iter = fields->cbegin(); iter != fields->cend(); ++iter) {
					if (iter->type == RomFields::RFT_STRING && !strcmp(iter->name, "DX10 Format")) {
						// Found the DX10 format.
						actual_pixel_format = iter->data.str;
						break;
					}
				}

				// NOTE: If the DX10 format wasn't found, then actual_pixel_format
				// will stay as "DX10".
			}

			EXPECT_NE(actual_pixel_format, nullptr);
			if (actual_pixel_format) {
				EXPECT_EQ(mode.expected_pixel_format, actual_pixel_format);
			}
		}
	}

	// Compare the image data.
	ASSERT_NO_FATAL_FAILURE(Compare_RpImage(img_png.get(), img_dds));
}

/**
 * Run an ImageDecoder test.
 */
TEST_P(ImageDecoderTest, decodeTest)
{
	ASSERT_NO_FATAL_FAILURE(decodeTest_internal());
}

/**
 * Internal benchmark function.
 */
void ImageDecoderTest::decodeBenchmark_internal(void)
{
	// Parameterized test.
	const ImageDecoderTest_mode &mode = GetParam();

	// Open the image as an IRpFile.
	m_f_dds = new MemFile(m_dds_buf.data(), m_dds_buf.size());
	ASSERT_TRUE(m_f_dds->isOpen()) << "Could not create MemFile for the DDS image.";
	m_f_dds->setFilename(mode.dds_gz_filename);

	// NOTE: We can't simply decode the image multiple times.
	// We have to reopen the RomData subclass every time.

	// Benchmark iterations.
	// BC7 has fewer iterations because it's more complicated.
	unsigned int max_iterations;
	if (mode.dds_gz_filename.find("BC7/") == 0) {
		// This is BC7.
		max_iterations = BENCHMARK_ITERATIONS_BC7;
	} else {
		// Not BC7.
		max_iterations = BENCHMARK_ITERATIONS;
	}

	// Load the image file.
	// TODO: RomDataFactory function to retrieve a constructor function?
	auto fn_ctor = [](IRpFile *file) -> RomData* { return RomDataFactory::create(file); };

	// For certain types, increase the number of iterations.
	ASSERT_GT(mode.dds_gz_filename.size(), 4U);
	if (mode.dds_gz_filename.size() >= 8U &&
	    !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-8, 8, ".smdh.gz"))
	{
		// Nintendo 3DS SMDH file
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (mode.dds_gz_filename.size() >= 7U &&
		   !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".gci.gz")) {
		// Nintendo GameCube save file
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (mode.dds_gz_filename.size() >= 4U &&
		   !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-4, 4, ".VMS")) {
		// Sega Dreamcast save file
		// NOTE: RomDataFactory and DreamcastSave don't support gzip at the moment.
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (mode.dds_gz_filename.size() >= 7U &&
		   !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".PSV.gz")) {
		// Sony PlayStation save file
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (mode.dds_gz_filename.size() >= 7U &&
		   !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".nds.gz")) {
		// Nintendo DS ROM image
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (mode.dds_gz_filename.size() >= 7U &&
		   (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".cab.gz") ||
		    !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".prb.gz"))) {
		// Nintendo Badge Arcade texture
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (mode.dds_gz_filename.size() >= 4U &&
		   !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-4, 4, ".tex")) {
		// Leapster Didj texture
		// NOTE: Increased iterations due to smaller files.
		// NOTE: Using RpTextureWrapper.
		max_iterations *= 10;
	}

	for (unsigned int i = max_iterations; i > 0; i--) {
		m_romData = fn_ctor(m_f_dds);
		ASSERT_TRUE(m_romData != nullptr) << "Could not load the DDS image.";
		ASSERT_TRUE(m_romData->isValid()) << "Could not load the DDS image.";
		ASSERT_TRUE(m_romData->isOpen()) << "Could not load the DDS image.";

		// Get the DDS image as an rp_image.
		// TODO: imgType to string?
		const rp_image *const img_dds = m_romData->image(mode.imgType);
		ASSERT_TRUE(img_dds != nullptr) << "Could not load the DDS image as rp_image.";

		UNREF_AND_NULL_NOCHK(m_romData);
	}
}

/**
 * Benchmark an ImageDecoder test.
 */
TEST_P(ImageDecoderTest, decodeBenchmark)
{
	ASSERT_NO_FATAL_FAILURE(decodeBenchmark_internal());
}

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string ImageDecoderTest::test_case_suffix_generator(const ::testing::TestParamInfo<ImageDecoderTest_mode> &info)
{
	string suffix = info.param.dds_gz_filename;

	// Replace all non-alphanumeric characters with '_'.
	// See gtest-param-util.h::IsValidParamName().
	std::replace_if(suffix.begin(), suffix.end(),
		[](char c) noexcept -> bool { return !ISALNUM(c); }, '_');

	// Append the image type to allow checking multiple types
	// of images in the same file.
	static const char s_imgType[][8] = {
		"_Icon", "_Banner", "_Media", "_Image"
	};
	static_assert(ARRAY_SIZE(s_imgType) == RomData::IMG_INT_MAX - RomData::IMG_INT_MIN + 1,
		"s_imgType[] needs to be updated.");
	assert(info.param.imgType >= RomData::IMG_INT_MIN);
	assert(info.param.imgType <= RomData::IMG_INT_MAX);
	if (info.param.imgType >= RomData::IMG_INT_MIN && info.param.imgType <= RomData::IMG_INT_MAX) {
		suffix += s_imgType[info.param.imgType - RomData::IMG_INT_MIN];
	}

	// TODO: Convert to ASCII?
	return suffix;
}

/** Test cases **/

// DirectDrawSurface tests (S3TC)
#define S3TC_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"S3TC/" file ".dds.gz", \
			"S3TC/" file ".s3tc.png", (format))
#define S3TC_IMAGE_TEST2(file, format) ImageDecoderTest_mode( \
			"S3TC/" file ".dds.gz", \
			"S3TC/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(DDS_S3TC, ImageDecoderTest,
	::testing::Values(
		S3TC_IMAGE_TEST("dxt1-rgb", "DXT1"),
		S3TC_IMAGE_TEST("dxt2-rgb", "DXT2"),
		S3TC_IMAGE_TEST("dxt2-argb", "DXT2"),
		S3TC_IMAGE_TEST("dxt3-rgb", "DXT3"),
		S3TC_IMAGE_TEST("dxt3-argb", "DXT3"),
		S3TC_IMAGE_TEST("dxt4-rgb", "DXT4"),
		S3TC_IMAGE_TEST("dxt4-argb", "DXT4"),
		S3TC_IMAGE_TEST("dxt5-rgb", "DXT5"),
		S3TC_IMAGE_TEST("dxt5-argb", "DXT5"),
		S3TC_IMAGE_TEST("bc4", "ATI1"),
		S3TC_IMAGE_TEST("bc5", "ATI2"),

		// from Blender T101405
		// https://developer.blender.org/T101405
		S3TC_IMAGE_TEST2("tex_cmp_bc3rxgb", "RXGB"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 8-bit RGB)
#define RGB_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"RGB/" file ".dds.gz", \
			"RGB/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(DDS_RGB8, ImageDecoderTest,
	::testing::Values(
		RGB_IMAGE_TEST("tex_dds_rgb332", "RGB332"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 16-bit RGB)
INSTANTIATE_TEST_SUITE_P(DDS_RGB16, ImageDecoderTest,
	::testing::Values(
		RGB_IMAGE_TEST("RGB565", "RGB565"),
		RGB_IMAGE_TEST("xRGB4444", "xRGB4444"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 16-bit ARGB)
#define ARGB_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"ARGB/" file ".dds.gz", \
			"ARGB/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(DDS_ARGB16, ImageDecoderTest,
	::testing::Values(
		ARGB_IMAGE_TEST("ARGB1555", "ARGB1555"),
		ARGB_IMAGE_TEST("ARGB4444", "ARGB4444"),
		ARGB_IMAGE_TEST("ARGB8332", "DXT3"))	// FIXME: Why is this DXT3?
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 15-bit RGB)
INSTANTIATE_TEST_SUITE_P(DDS_RGB15, ImageDecoderTest,
	::testing::Values(
		RGB_IMAGE_TEST("RGB565", "RGB565"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 24-bit RGB)
INSTANTIATE_TEST_SUITE_P(DDS_RGB24, ImageDecoderTest,
	::testing::Values(
		RGB_IMAGE_TEST("RGB888", "RGB888"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 32-bit RGB)
INSTANTIATE_TEST_SUITE_P(DDS_RGB32, ImageDecoderTest,
	::testing::Values(
		RGB_IMAGE_TEST("xRGB8888", "xRGB8888"),
		RGB_IMAGE_TEST("xBGR8888", "xBGR8888"),

		// Uncommon formats
		RGB_IMAGE_TEST("G16R16", "G16R16"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 32-bit ARGB)
INSTANTIATE_TEST_SUITE_P(DDS_ARGB32, ImageDecoderTest,
	::testing::Values(
		// 32-bit
		ARGB_IMAGE_TEST("ARGB8888", "ARGB8888"),
		ARGB_IMAGE_TEST("ABGR8888", "ABGR8888"),

		// Uncommon formats
		ARGB_IMAGE_TEST("A2R10G10B10", "A2R10G10B10"),
		ARGB_IMAGE_TEST("A2B10G10R10", "A2B10G10R10"),

		// DXGI format is set; legacy bitmasks are not.
		// (from Pillow)
		ARGB_IMAGE_TEST("argb-32bpp_MipMaps-1", "R8G8B8A8_UNORM"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Luminance)
#define Luma_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"Luma/" file ".dds.gz", \
			"Luma/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(DDS_Luma, ImageDecoderTest,
	::testing::Values(
		Luma_IMAGE_TEST("L8", "L8"),
		Luma_IMAGE_TEST("A4L4", "A4L4"),
		Luma_IMAGE_TEST("L16", "L16"),
		Luma_IMAGE_TEST("A8L8", "A8L8"),

		// (from Pillow)
		Luma_IMAGE_TEST("uncompressed_l", "L8"),
		Luma_IMAGE_TEST("uncompressed_la", "A8L8"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Alpha)
#define Alpha_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"Alpha/" file ".dds.gz", \
			"Alpha/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(DDS_Alpha, ImageDecoderTest,
	::testing::Values(
		Alpha_IMAGE_TEST("A8", "A8"))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests (square twiddled)
#define PVR_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"PVR/" file ".pvr.gz", \
			"PVR/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(PVR_SqTwiddled, ImageDecoderTest,
	::testing::Values(
		PVR_IMAGE_TEST("bg_00", "ARGB4444"))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests (VQ)
INSTANTIATE_TEST_SUITE_P(PVR_VQ, ImageDecoderTest,
	::testing::Values(
		PVR_IMAGE_TEST("mr_128k_huti", "RGB565"))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests (Small VQ)
INSTANTIATE_TEST_SUITE_P(PVR_SmallVQ, ImageDecoderTest,
	::testing::Values(
		PVR_IMAGE_TEST("drumfuta1", "RGB565"),
		PVR_IMAGE_TEST("drum_ref", "RGB565"),
		PVR_IMAGE_TEST("sp_blue", "RGB565"))
	, ImageDecoderTest::test_case_suffix_generator);

// GVR tests (RGB5A3)
#define GVR_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"GVR/" file ".gvr.gz", \
			"GVR/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(GVR_RGB5A3, ImageDecoderTest,
	::testing::Values(
		GVR_IMAGE_TEST("zanki_sonic", "IA8"))
	, ImageDecoderTest::test_case_suffix_generator);

// GVR tests (DXT1, S3TC)
#define GVR_S3TC_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"GVR/" file ".gvr.gz", \
			"GVR/" file ".s3tc.png", (format))
INSTANTIATE_TEST_SUITE_P(GVR_DXT1_S3TC, ImageDecoderTest,
	::testing::Values(
		// FIXME: These are DXT1; pixel format is incorrect...
		GVR_S3TC_IMAGE_TEST("paldam_off", "IA8"),
		GVR_S3TC_IMAGE_TEST("paldam_on", "RGB565"),
		GVR_S3TC_IMAGE_TEST("weeklytitle", "RGB565"))
	, ImageDecoderTest::test_case_suffix_generator);

// KTX tests
#define KTX_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"KTX/" file ".ktx.gz", \
			"KTX/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(KTX, ImageDecoderTest,
	::testing::Values(
		// RGB reference image.
		ImageDecoderTest_mode(
			"KTX/rgb-reference.ktx.gz",
			"KTX/rgb.png", "RGB"),
		// RGB reference image, mipmap levels == 0
		ImageDecoderTest_mode(
			"KTX/rgb-amg-reference.ktx.gz",
			"KTX/rgb.png", "RGB"),
		// Orientation: Up (upside-down compared to "normal")
		ImageDecoderTest_mode(
			"KTX/up-reference.ktx.gz",
			"KTX/up.png", "RGB"),
		// Orientation: Down (same as "normal")
		ImageDecoderTest_mode(
			"KTX/down-reference.ktx.gz",
			"KTX/up.png", "RGB"),

		// Luminance (unsized: GL_LUMINANCE)
		ImageDecoderTest_mode(
			"KTX/luminance_unsized_reference.ktx.gz",
			"KTX/luminance.png", "LUMINANCE"),
		// Luminance (sized: GL_LUMINANCE8)
		ImageDecoderTest_mode(
			"KTX/luminance_sized_reference.ktx.gz",
			"KTX/luminance.png", "LUMINANCE8"),

		// ETC1
		KTX_IMAGE_TEST("etc1", "ETC1_RGB8_OES"),
		// ETC2
		KTX_IMAGE_TEST("etc2-rgb", "COMPRESSED_RGB8_ETC2"),
		KTX_IMAGE_TEST("etc2-rgba1", "COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2"),
		KTX_IMAGE_TEST("etc2-rgba8", "COMPRESSED_RGBA8_ETC2_EAC"),

		// BGR888 (Hi Corp)
		KTX_IMAGE_TEST("hi_mark", "RGB"),
		KTX_IMAGE_TEST("hi_mark_sq", "RGB"),

		// EAC
		KTX_IMAGE_TEST("conftestimage_R11_EAC", "COMPRESSED_R11_EAC"),
		KTX_IMAGE_TEST("conftestimage_RG11_EAC", "COMPRESSED_RG11_EAC"),
		KTX_IMAGE_TEST("conftestimage_SIGNED_R11_EAC", "COMPRESSED_SIGNED_R11_EAC"),
		KTX_IMAGE_TEST("conftestimage_SIGNED_RG11_EAC", "COMPRESSED_SIGNED_RG11_EAC"),

		// RGBA reference image
		ImageDecoderTest_mode(
			"KTX/rgba-reference.ktx.gz",
			"KTX/rgba.png", "RGBA"))

	, ImageDecoderTest::test_case_suffix_generator);

// KTX2 tests
#define KTX2_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"KTX2/" file ".ktx2.gz", \
			"KTX2/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(KTX2, ImageDecoderTest,
	::testing::Values(
		KTX2_IMAGE_TEST("cubemap_yokohama_bc3_unorm", "BC3_UNORM_BLOCK"),
		KTX2_IMAGE_TEST("cubemap_yokohama_etc2_unorm", "ETC2_R8G8B8_UNORM_BLOCK"),
		KTX2_IMAGE_TEST("luminance_alpha_reference_u", "R8G8_UNORM"),
		KTX2_IMAGE_TEST("orient-down-metadata-u", "R8G8B8_SRGB"),
		KTX2_IMAGE_TEST("orient-up-metadata-u", "R8G8B8_SRGB"),
		KTX2_IMAGE_TEST("pattern_02_bc2", "BC2_UNORM_BLOCK"),
		KTX2_IMAGE_TEST("rg_reference_u", "R8G8_UNORM"),
		KTX2_IMAGE_TEST("rgba-reference-u", "R8G8B8A8_SRGB"),
		KTX2_IMAGE_TEST("rgb-mipmap-reference-u", "R8G8B8_SRGB"),
		KTX2_IMAGE_TEST("texturearray_bc3_unorm", "BC3_UNORM_BLOCK"),
		KTX2_IMAGE_TEST("texturearray_etc2_unorm", "ETC2_R8G8B8_UNORM_BLOCK"))
	, ImageDecoderTest::test_case_suffix_generator);

// Valve VTF tests (all formats)
#define VTF_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"VTF/" file ".vtf.gz", \
			"VTF/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(VTF, ImageDecoderTest,
	::testing::Values(
		// NOTE: VTF channel ordering is usually backwards from ImageDecoder.

		// 32-bit ARGB
		ImageDecoderTest_mode(
			"VTF/ABGR8888.vtf.gz",
			"argb-reference.png", "ABGR8888"),
		ImageDecoderTest_mode(
			"VTF/ARGB8888.vtf.gz",	// NOTE: Actually RABG8888.
			"argb-reference.png", "ARGB8888"),
		ImageDecoderTest_mode(
			"VTF/BGRA8888.vtf.gz",
			"argb-reference.png", "BGRA8888"),
		ImageDecoderTest_mode(
			"VTF/RGBA8888.vtf.gz",
			"argb-reference.png", "RGBA8888"),

		// 32-bit xRGB
		ImageDecoderTest_mode(
			"VTF/BGRx8888.vtf.gz",
			"rgb-reference.png", "BGRx8888"),

		// 24-bit RGB
		ImageDecoderTest_mode(
			"VTF/BGR888.vtf.gz",
			"rgb-reference.png", "BGR888"),
		ImageDecoderTest_mode(
			"VTF/RGB888.vtf.gz",
			"rgb-reference.png", "RGB888"),

		// 24-bit RGB + bluescreen
		VTF_IMAGE_TEST("BGR888_bluescreen", "BGR888 (Bluescreen)"),
		ImageDecoderTest_mode(
			"VTF/RGB888_bluescreen.vtf.gz",
			"VTF/BGR888_bluescreen.png", "RGB888 (Bluescreen)"),

		// 16-bit RGB (565)
		// FIXME: Tests are failing.
		ImageDecoderTest_mode(
			"VTF/BGR565.vtf.gz",
			"RGB/RGB565.png", "BGR565"),
		ImageDecoderTest_mode(
			"VTF/RGB565.vtf.gz",
			"RGB/RGB565.png", "RGB565"),

		// 15-bit RGB (555)
		ImageDecoderTest_mode(
			"VTF/BGRx5551.vtf.gz",
			"RGB/RGB555.png", "BGRx5551"),

		// 16-bit ARGB (4444)
		ImageDecoderTest_mode(
			"VTF/BGRA4444.vtf.gz",
			"ARGB/ARGB4444.png", "BGRA4444"),

		// UV88 (handled as RG88)
		ImageDecoderTest_mode(
			"VTF/UV88.vtf.gz",
			"rg-reference.png", "UV88"),

		// Intensity formats
		ImageDecoderTest_mode(
			"VTF/I8.vtf.gz",
			"Luma/L8.png", "I8"),
		ImageDecoderTest_mode(
			"VTF/IA88.vtf.gz",
			"Luma/A8L8.png", "IA88"),

		// Alpha format (A8)
		ImageDecoderTest_mode(
			"VTF/A8.vtf.gz",
			"Alpha/A8.png", "A8"))
	, ImageDecoderTest::test_case_suffix_generator);

// Valve VTF tests (S3TC)
#define VTF_S3TC_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"VTF/" file ".vtf.gz", \
			"VTF/" file ".s3tc.png", (format))
INSTANTIATE_TEST_SUITE_P(VTF_S3TC, ImageDecoderTest,
	::testing::Values(
		VTF_S3TC_IMAGE_TEST("DXT1", "DXT1"),
		VTF_S3TC_IMAGE_TEST("DXT1_A1", "DXT1_A1"),
		VTF_S3TC_IMAGE_TEST("DXT3", "DXT3"),
		VTF_S3TC_IMAGE_TEST("DXT5", "DXT5"))
	, ImageDecoderTest::test_case_suffix_generator);

// Valve VTF3 tests (S3TC)
#define VTF3_S3TC_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"VTF3/" file ".vtf.gz", \
			"VTF3/" file ".s3tc.png", (format))
INSTANTIATE_TEST_SUITE_P(VTF3_S3TC, ImageDecoderTest,
	::testing::Values(
		VTF3_S3TC_IMAGE_TEST("elevator_screen_broken_normal.ps3", "DXT5"),
		VTF3_S3TC_IMAGE_TEST("elevator_screen_colour.ps3", "DXT1"))
	, ImageDecoderTest::test_case_suffix_generator);

// Test images from texture-compressor.
// Reference: https://github.com/TimvanScherpenzeel/texture-compressor
INSTANTIATE_TEST_SUITE_P(TCtest, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"tctest/example-etc1.ktx.gz",
			"tctest/example-etc1.ktx.png", "ETC1_RGB8_OES"),
		ImageDecoderTest_mode(
			"tctest/example-etc2.ktx.gz",
			"tctest/example-etc2.ktx.png", "COMPRESSED_RGB8_ETC2"))
	, ImageDecoderTest::test_case_suffix_generator);

// texture-compressor tests (S3TC)
INSTANTIATE_TEST_SUITE_P(TCtest_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"tctest/example-dxt1.dds.gz",
			"tctest/example-dxt1.s3tc.dds.png", "DXT1"),
		ImageDecoderTest_mode(
			"tctest/example-dxt3.dds.gz",
			"tctest/example-dxt5.s3tc.dds.png", "DXT3"),
		ImageDecoderTest_mode(
			"tctest/example-dxt5.dds.gz",
			"tctest/example-dxt5.s3tc.dds.png", "DXT5"))
	, ImageDecoderTest::test_case_suffix_generator);

#ifdef ENABLE_PVRTC
// texture-compressor tests (PVRTC)
INSTANTIATE_TEST_SUITE_P(TCtest_PVRTC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"tctest/example-pvrtc1.pvr.gz",
			"tctest/example-pvrtc1.pvr.png", "PVRTC 2bpp RGB"),
		ImageDecoderTest_mode(
			"tctest/example-pvrtc2-2bpp.pvr.gz",
			"tctest/example-pvrtc2-2bpp.pvr.png", "PVRTC-II 2bpp"),
		ImageDecoderTest_mode(
			"tctest/example-pvrtc2-4bpp.pvr.gz",
			"tctest/example-pvrtc2-4bpp.pvr.png", "PVRTC-II 4bpp"))
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_PVRTC */

#ifdef ENABLE_ASTC
// texture-compressor tests (ASTC)
INSTANTIATE_TEST_SUITE_P(TCtest_ASTC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"tctest/example-astc.dds.gz",
			"tctest/example-astc.png", "ASTC_8x8"))
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_ASTC */

// BC7 tests
#define BC7_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"BC7/" file ".dds.gz", \
			"BC7/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(BC7, ImageDecoderTest,
	::testing::Values(
		BC7_IMAGE_TEST("w5_grass200_abd_a", "BC7_UNORM"),
		BC7_IMAGE_TEST("w5_grass201_abd", "BC7_UNORM"),
		BC7_IMAGE_TEST("w5_grass206_abd", "BC7_UNORM"),
		BC7_IMAGE_TEST("w5_rock805_abd", "BC7_UNORM"),
		BC7_IMAGE_TEST("w5_rock805_nrm", "BC7_UNORM"),
		BC7_IMAGE_TEST("w5_sand504_abd_a", "BC7_UNORM"),
		BC7_IMAGE_TEST("w5_wood503_prm", "BC7_UNORM"),

		// Non-gzipped DDS, since gzip doesn't help much
		ImageDecoderTest_mode(
			"BC7/w5_rope801_prm.dds",
			"BC7/w5_rope801_prm.png", "BC7_UNORM"))
	, ImageDecoderTest::test_case_suffix_generator);

// SMDH tests
// From *New* Nintendo 3DS 9.2.0-20J.
#define SMDH_TEST(file) ImageDecoderTest_mode( \
			"SMDH/" file ".smdh.gz", \
			"SMDH/" file ".png", RomData::IMG_INT_ICON)
INSTANTIATE_TEST_SUITE_P(SMDH, ImageDecoderTest,
	::testing::Values(
		SMDH_TEST("0004001000020000"),
		SMDH_TEST("0004001000020100"),
		SMDH_TEST("0004001000020400"),
		SMDH_TEST("0004001000020900"),
		SMDH_TEST("0004001000020A00"),
		SMDH_TEST("000400100002BF00"),
		SMDH_TEST("0004001020020300"),
		SMDH_TEST("0004001020020D00"),
		SMDH_TEST("0004001020023100"),
		SMDH_TEST("000400102002C800"),
		SMDH_TEST("000400102002C900"),
		SMDH_TEST("000400102002CB00"),
		SMDH_TEST("0004003000008302"),
		SMDH_TEST("0004003000008402"),
		SMDH_TEST("0004003000008602"),
		SMDH_TEST("0004003000008702"),
		SMDH_TEST("0004003000008D02"),
		SMDH_TEST("0004003000008E02"),
		SMDH_TEST("000400300000BC02"),
		SMDH_TEST("000400300000C602"),
		SMDH_TEST("0004003020008802"))
	, ImageDecoderTest::test_case_suffix_generator);

// GCI tests
// TODO: Use something like GcnFstTest that uses an array of filenames
// to generate tests at runtime instead of compile-time?
#define GCI_ICON_TEST(file) ImageDecoderTest_mode( \
			"GCI/" file ".gci.gz", \
			"GCI/" file ".icon.png", RomData::IMG_INT_ICON)
#define GCI_BANNER_TEST(file) ImageDecoderTest_mode( \
			"GCI/" file ".gci.gz", \
			"GCI/" file ".banner.png", RomData::IMG_INT_BANNER)

INSTANTIATE_TEST_SUITE_P(GCI_Icon_1, ImageDecoderTest,
	::testing::Values(
		GCI_ICON_TEST("01-D43E-ZELDA"),
		GCI_ICON_TEST("01-G2ME-MetroidPrime2"),
		GCI_ICON_TEST("01-G4SE-gc4sword"),
		GCI_ICON_TEST("01-G8ME-mariost_save_file"),
		GCI_ICON_TEST("01-GAFE-DobutsunomoriP_MURA"),
		GCI_ICON_TEST("01-GALE-SuperSmashBros0110290334"),
		GCI_ICON_TEST("01-GC6E-pokemon_colosseum"),
		GCI_ICON_TEST("01-GEDE-Eternal Darkness"),
		GCI_ICON_TEST("01-GFEE-FIREEMBLEM8J"),
		GCI_ICON_TEST("01-GKGE-DONKEY KONGA SAVEDATA"),
		GCI_ICON_TEST("01-GLME-LUIGI_MANSION_SAVEDATA_v3"),
		GCI_ICON_TEST("01-GM4E-MarioKart Double Dash!!"),
		GCI_ICON_TEST("01-GM8E-MetroidPrime A"),
		GCI_ICON_TEST("01-GMPE-MARIPA4BOX0"),
		GCI_ICON_TEST("01-GMPE-MARIPA4BOX1"),
		GCI_ICON_TEST("01-GMPE-MARIPA4BOX2"),
		GCI_ICON_TEST("01-GMSE-super_mario_sunshine"),
		GCI_ICON_TEST("01-GP5E-MARIPA5"),
		GCI_ICON_TEST("01-GP6E-MARIPA6"),
		GCI_ICON_TEST("01-GP7E-MARIPA7"),
		GCI_ICON_TEST("01-GPIE-Pikmin dataFile"),
		GCI_ICON_TEST("01-GPVE-Pikmin2_SaveData"),
		GCI_ICON_TEST("01-GPXE-pokemon_rs_memory_box"),
		GCI_ICON_TEST("01-GXXE-PokemonXD"),
		GCI_ICON_TEST("01-GYBE-MainData"),
		GCI_ICON_TEST("01-GZ2E-gczelda2"),
		GCI_ICON_TEST("01-GZLE-gczelda"),
		GCI_ICON_TEST("01-PZLE-NES_ZELDA1_SAVE"),
		GCI_ICON_TEST("01-PZLE-NES_ZELDA2_SAVE"),
		GCI_ICON_TEST("01-PZLE-ZELDA1"),
		GCI_ICON_TEST("01-PZLE-ZELDA2"),
		GCI_ICON_TEST("51-GTKE-Save Game0"),
		GCI_ICON_TEST("52-GTDE-SK5sbltitgaSK5sbltitga"),
		GCI_ICON_TEST("52-GTDE-SK5sirpvsicSK5sirpvsic"),
		GCI_ICON_TEST("52-GTDE-SK5xwkqsbafSK5xwkqsbaf"),
		GCI_ICON_TEST("5D-GE9E-EDEDDNEDDYTHEMIS-EDVENTURES"),
		GCI_ICON_TEST("69-GHSE-POTTERCOS"),
		GCI_ICON_TEST("69-GO7E-BOND"),
		GCI_ICON_TEST("78-GW3E-__2f__w_mania2002"),
		GCI_ICON_TEST("7D-GCBE-CRASHWOC"),
		GCI_ICON_TEST("7D-GCNE-all"),
		GCI_ICON_TEST("8P-G2XE-SONIC GEMS_00"),
		GCI_ICON_TEST("8P-G2XE-SONIC_R"),
		GCI_ICON_TEST("8P-G2XE-STF.DAT"),
		GCI_ICON_TEST("8P-G9SE-SONICHEROES_00"),
		GCI_ICON_TEST("8P-G9SE-SONICHEROES_01"),
		GCI_ICON_TEST("8P-G9SE-SONICHEROES_02"),
		GCI_ICON_TEST("8P-GEZE-billyhatcher"),
		GCI_ICON_TEST("8P-GFZE-f_zero.dat"))
	, ImageDecoderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(GCI_Icon_2, ImageDecoderTest,
	::testing::Values(
		GCI_ICON_TEST("8P-GM2E-rep0000010000C900002497A48E.dat"),
		GCI_ICON_TEST("8P-GM2E-super_monkey_ball_2.dat"),
		GCI_ICON_TEST("8P-GMBE-smkb0058556041f42afb"),
		GCI_ICON_TEST("8P-GMBE-super_monkey_ball.sys"),
		GCI_ICON_TEST("8P-GPUE-Puyo Pop Fever Replay01"),
		GCI_ICON_TEST("8P-GPUE-Puyo Pop Fever System"),
		GCI_ICON_TEST("8P-GSNE-SONIC2B__5f____5f__S01"),
		GCI_ICON_TEST("8P-GSOE-S_MEGA_SYS"),
		GCI_ICON_TEST("8P-GUPE-SHADOWTHEHEDGEHOG"),
		GCI_ICON_TEST("8P-GXEE-SONICRIDERS_GAMEDATA_01"),
		GCI_ICON_TEST("8P-GXSE-SONICADVENTURE_DX_PLAYRECORD_1"),
		GCI_ICON_TEST("AF-GNME-NAMCOMUSEUM"),
		GCI_ICON_TEST("AF-GP2E-PMW2SAVE"),
		GCI_ICON_TEST("AF-GPME-PACMANFEVER"))
	, ImageDecoderTest::test_case_suffix_generator);

// NOTE: Some files don't have banners. They're left in the list for
// consistency, but are commented out.
INSTANTIATE_TEST_SUITE_P(GCI_Banner_1, ImageDecoderTest,
	::testing::Values(
		GCI_BANNER_TEST("01-D43E-ZELDA"),
		GCI_BANNER_TEST("01-G2ME-MetroidPrime2"),
		GCI_BANNER_TEST("01-G4SE-gc4sword"),
		GCI_BANNER_TEST("01-G8ME-mariost_save_file"),
		GCI_BANNER_TEST("01-GAFE-DobutsunomoriP_MURA"),
		GCI_BANNER_TEST("01-GALE-SuperSmashBros0110290334"),
		GCI_BANNER_TEST("01-GC6E-pokemon_colosseum"),
		GCI_BANNER_TEST("01-GEDE-Eternal Darkness"),
		GCI_BANNER_TEST("01-GFEE-FIREEMBLEM8J"),
		GCI_BANNER_TEST("01-GKGE-DONKEY KONGA SAVEDATA"),
		GCI_BANNER_TEST("01-GLME-LUIGI_MANSION_SAVEDATA_v3"),
		GCI_BANNER_TEST("01-GM4E-MarioKart Double Dash!!"),
		GCI_BANNER_TEST("01-GM8E-MetroidPrime A"),
		GCI_BANNER_TEST("01-GMPE-MARIPA4BOX0"),
		GCI_BANNER_TEST("01-GMPE-MARIPA4BOX1"),
		GCI_BANNER_TEST("01-GMPE-MARIPA4BOX2"),
		GCI_BANNER_TEST("01-GMSE-super_mario_sunshine"),
		GCI_BANNER_TEST("01-GP5E-MARIPA5"),
		GCI_BANNER_TEST("01-GP6E-MARIPA6"),
		GCI_BANNER_TEST("01-GP7E-MARIPA7"),
		GCI_BANNER_TEST("01-GPIE-Pikmin dataFile"),
		GCI_BANNER_TEST("01-GPVE-Pikmin2_SaveData"),
		GCI_BANNER_TEST("01-GPXE-pokemon_rs_memory_box"),
		GCI_BANNER_TEST("01-GXXE-PokemonXD"),
		GCI_BANNER_TEST("01-GYBE-MainData"),
		GCI_BANNER_TEST("01-GZ2E-gczelda2"),
		GCI_BANNER_TEST("01-GZLE-gczelda"),
		GCI_BANNER_TEST("01-PZLE-NES_ZELDA1_SAVE"),
		GCI_BANNER_TEST("01-PZLE-NES_ZELDA2_SAVE"),
		GCI_BANNER_TEST("01-PZLE-ZELDA1"),
		GCI_BANNER_TEST("01-PZLE-ZELDA2"),
		//GCI_BANNER_TEST("51-GTKE-Save Game0"),
		GCI_BANNER_TEST("52-GTDE-SK5sbltitgaSK5sbltitga"),
		GCI_BANNER_TEST("52-GTDE-SK5sirpvsicSK5sirpvsic"),
		GCI_BANNER_TEST("52-GTDE-SK5xwkqsbafSK5xwkqsbaf"),
		//GCI_BANNER_TEST("5D-GE9E-EDEDDNEDDYTHEMIS-EDVENTURES"),
		//GCI_BANNER_TEST("69-GHSE-POTTERCOS"),
		GCI_BANNER_TEST("69-GO7E-BOND"),
		GCI_BANNER_TEST("78-GW3E-__2f__w_mania2002"),
		//GCI_BANNER_TEST("7D-GCBE-CRASHWOC"),
		GCI_BANNER_TEST("7D-GCNE-all"),
		GCI_BANNER_TEST("8P-G2XE-SONIC GEMS_00"),
		GCI_BANNER_TEST("8P-G2XE-SONIC_R"),
		GCI_BANNER_TEST("8P-G2XE-STF.DAT"),
		GCI_BANNER_TEST("8P-G9SE-SONICHEROES_00"),
		GCI_BANNER_TEST("8P-G9SE-SONICHEROES_01"),
		GCI_BANNER_TEST("8P-G9SE-SONICHEROES_02"),
		GCI_BANNER_TEST("8P-GEZE-billyhatcher"),
		GCI_BANNER_TEST("8P-GFZE-f_zero.dat"))
	, ImageDecoderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(GCI_Banner_2, ImageDecoderTest,
	::testing::Values(
		GCI_BANNER_TEST("8P-GM2E-rep0000010000C900002497A48E.dat"),
		GCI_BANNER_TEST("8P-GM2E-super_monkey_ball_2.dat"),
		GCI_BANNER_TEST("8P-GMBE-smkb0058556041f42afb"),
		GCI_BANNER_TEST("8P-GMBE-super_monkey_ball.sys"),
		//GCI_BANNER_TEST("8P-GPUE-Puyo Pop Fever Replay01"),
		//GCI_BANNER_TEST("8P-GPUE-Puyo Pop Fever System"),
		GCI_BANNER_TEST("8P-GSNE-SONIC2B__5f____5f__S01"),
		GCI_BANNER_TEST("8P-GSOE-S_MEGA_SYS"),
		GCI_BANNER_TEST("8P-GUPE-SHADOWTHEHEDGEHOG"),
		GCI_BANNER_TEST("8P-GXEE-SONICRIDERS_GAMEDATA_01"),
		GCI_BANNER_TEST("8P-GXSE-SONICADVENTURE_DX_PLAYRECORD_1"),
		GCI_BANNER_TEST("AF-GNME-NAMCOMUSEUM"),
		GCI_BANNER_TEST("AF-GP2E-PMW2SAVE"),
		GCI_BANNER_TEST("AF-GPME-PACMANFEVER"))
	, ImageDecoderTest::test_case_suffix_generator);

// VMS tests
INSTANTIATE_TEST_SUITE_P(VMS, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"Misc/BIOS002.VMS",
			"Misc/BIOS002.png",
			RomData::IMG_INT_ICON),
		ImageDecoderTest_mode(
			"Misc/SONIC2C.VMS",
			"Misc/SONIC2C.png",
			RomData::IMG_INT_ICON))
	, ImageDecoderTest::test_case_suffix_generator);

// PSV tests
INSTANTIATE_TEST_SUITE_P(PSV, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"Misc/BASCUS-94228535059524F.PSV.gz",
			"Misc/BASCUS-94228535059524F.png",
			RomData::IMG_INT_ICON),
		ImageDecoderTest_mode(
			"Misc/BASCUS-949003034323235383937.PSV.gz",
			"Misc/BASCUS-949003034323235383937.png",
			RomData::IMG_INT_ICON))
	, ImageDecoderTest::test_case_suffix_generator);

// NDS tests
// TODO: Use something like GcnFstTest that uses an array of filenames
// to generate tests at runtime instead of compile-time?
#define NDS_ICON_TEST(file) ImageDecoderTest_mode( \
			"NDS/" file ".header-icon.nds.gz", \
			"NDS/" file ".header-icon.png", RomData::IMG_INT_ICON)

INSTANTIATE_TEST_SUITE_P(NDS, ImageDecoderTest,
	::testing::Values(
		NDS_ICON_TEST("A2DE01"),
		NDS_ICON_TEST("A3YE8P"),
		NDS_ICON_TEST("AIZE01"),
		NDS_ICON_TEST("AKWE01"),
		NDS_ICON_TEST("AMHE01"),
		NDS_ICON_TEST("ANDE01"),
		NDS_ICON_TEST("ANME01"),
		NDS_ICON_TEST("AOSE01"),
		NDS_ICON_TEST("APAE01"),
		NDS_ICON_TEST("ASCE8P"),
		NDS_ICON_TEST("ASME01"),
		NDS_ICON_TEST("ATKE01"),
		NDS_ICON_TEST("AY9E8P"),
		NDS_ICON_TEST("AYWE01"),
		NDS_ICON_TEST("BFUE41"),
		NDS_ICON_TEST("BOOE08"),
		NDS_ICON_TEST("BSLEWR"),
		NDS_ICON_TEST("BXSE8P"),
		NDS_ICON_TEST("CBQEG9"),
		NDS_ICON_TEST("COLE8P"),
		NDS_ICON_TEST("CS3E8P"),
		NDS_ICON_TEST("DMFEA4"),
		NDS_ICON_TEST("DSYESZ"),
		NDS_ICON_TEST("NTRJ01.Tetris-THQ"),
		NDS_ICON_TEST("VSOE8P"),
		NDS_ICON_TEST("YDLE20"),
		NDS_ICON_TEST("YLZE01"),
		NDS_ICON_TEST("YWSE8P"))
	, ImageDecoderTest::test_case_suffix_generator);

// NintendoBadge tests
// TODO: Use something like GcnFstTest that uses an array of filenames
// to generate tests at runtime instead of compile-time?
#define BADGE_IMAGE_ONLY_TEST(file) ImageDecoderTest_mode( \
			"Misc/" file ".gz", \
			"Misc/" file ".image.png", RomData::IMG_INT_IMAGE)
#define BADGE_ICON_IMAGE_TEST(file) ImageDecoderTest_mode( \
			"Misc/" file ".gz", \
			"Misc/" file ".icon.png", RomData::IMG_INT_ICON), \
			BADGE_IMAGE_ONLY_TEST(file)
INSTANTIATE_TEST_SUITE_P(NintendoBadge, ImageDecoderTest,
	::testing::Values(
		BADGE_ICON_IMAGE_TEST("MroKrt8.cab"),
		BADGE_IMAGE_ONLY_TEST("MroKrt8_Chara_Luigi000.prb"),
		BADGE_IMAGE_ONLY_TEST("MroKrt8_Chara_Mario000.prb"),
		BADGE_IMAGE_ONLY_TEST("MroKrt8_Chara_Peach000.prb"),
		BADGE_IMAGE_ONLY_TEST("Pr_Animal_12Sc_edit.prb"),
		BADGE_ICON_IMAGE_TEST("Pr_Animal_17Sc_mset.prb"),
		BADGE_ICON_IMAGE_TEST("Pr_FcRemix_2_drM_item05.prb"),
		BADGE_ICON_IMAGE_TEST("Pr_FcRemix_2_punch_char01_3_Sep.prb"))
	, ImageDecoderTest::test_case_suffix_generator);

// SVR tests
// TODO: Use something like GcnFstTest that uses an array of filenames
// to generate tests at runtime instead of compile-time?
// FIXME: Puyo Tools doesn't support format 0x61.
#define SVR_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"SVR/" file ".svr.gz", \
			"SVR/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(SVR_1, ImageDecoderTest,
	::testing::Values(
		SVR_IMAGE_TEST("1channel_01", "BGR5A3"),
		SVR_IMAGE_TEST("1channel_02", "BGR5A3"),
		SVR_IMAGE_TEST("1channel_03", "BGR5A3"),
		SVR_IMAGE_TEST("1channel_04", "BGR5A3"),
		SVR_IMAGE_TEST("1channel_05", "BGR5A3"),
		SVR_IMAGE_TEST("1channel_06", "BGR5A3"),
		SVR_IMAGE_TEST("2channel_01", "BGR5A3"),
		SVR_IMAGE_TEST("2channel_02", "BGR5A3"),
		SVR_IMAGE_TEST("2channel_03", "BGR5A3"),
		SVR_IMAGE_TEST("3channel_01", "BGR5A3"),
		SVR_IMAGE_TEST("3channel_02", "BGR5A3"),
		SVR_IMAGE_TEST("3channel_03", "BGR5A3"),
		SVR_IMAGE_TEST("al_egg01", "BGR5A3"),
		SVR_IMAGE_TEST("ani_han_karada", "BGR5A3"),
		SVR_IMAGE_TEST("ani_han_parts", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("ani_kao_005", "BGR5A3"),
		SVR_IMAGE_TEST("c_butterfly_k00", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_env_k00", "BGR5A3"),
		//SVR_IMAGE_TEST("c_env_k01", "BGR5A3"),
		SVR_IMAGE_TEST("c_env_k02", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_env_k04", "BGR5A3"),
		//SVR_IMAGE_TEST("c_gold_k00", "BGR5A3"),
		//SVR_IMAGE_TEST("c_green_k00", "BGR888_ABGR7888),
		SVR_IMAGE_TEST("c_green_k01", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("channel_off", "BGR5A3"),
		SVR_IMAGE_TEST("cha_trt_back01", "BGR888_ABGR7888"),
		//SVR_IMAGE_TEST("cha_trt_shadow01", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("cha_trt_water01", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("cha_trt_water02", "BGR5A3"),
		SVR_IMAGE_TEST("c_paint_aiai", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_akira", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_amigo", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_cake", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_chao", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_chu", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_egg", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_flower", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_nights", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_puyo", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_shadow", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_soccer", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_sonic", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_taxi", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_urara", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_paint_zombie", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_pumpkinhead", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("cs_book_02", "BGR5A3"),
		//SVR_IMAGE_TEST("c_silver_k00", "BGR5A3"),
		SVR_IMAGE_TEST("c_toy_k00", "BGR5A3"))
		//SVR_IMAGE_TEST("c_toy_k01", "BGR5A3"))
	, ImageDecoderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(SVR_2, ImageDecoderTest,
	::testing::Values(
		SVR_IMAGE_TEST("c_toy_k02", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("c_toy_k03", "BGR5A3"),
		SVR_IMAGE_TEST("c_toy_k04", "BGR888_ABGR7888"),
		//SVR_IMAGE_TEST("c_tree_k00", "BGR5A3"),
		//SVR_IMAGE_TEST("c_tree_k01", "BGR5A3"),
		//SVR_IMAGE_TEST("eft_drop_t01", "BGR888_ABGR7888"),
		//SVR_IMAGE_TEST("eft_ripple01", "BGR888_ABGR7888"),
		//SVR_IMAGE_TEST("eft_splash_t01", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blbaketu2", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blbaketu", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blcarpet", "BGR5A3"),
		SVR_IMAGE_TEST("kin_bldai", "BGR5A3"),
		SVR_IMAGE_TEST("kin_bldesk1", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blfire", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blhako1", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blhako2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blhako3", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blhako4", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blhako5", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blhako6", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blhouki", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blhouki2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blhouki3", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blhouki4", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blhouki5", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blkabekake1", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blkabekake2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blkasa", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blkumo", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blkumo2", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_bllamp1", "BGR5A3"),
		SVR_IMAGE_TEST("kin_bllamp2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_bllamp3", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blmaf", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blmask", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_blphot", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blregi", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blregi2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blreji3", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blreji4", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blreji5", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blspeeker", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blspeeker2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blspeeker3", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blvase1", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blvase2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blwall", "BGR5A3"),
		SVR_IMAGE_TEST("kin_blwall2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_cbdoor2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_copdwaku2", "BGR5A3"))
	, ImageDecoderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(SVR_3, ImageDecoderTest,
	::testing::Values(
		SVR_IMAGE_TEST("kin_doglass1", "BGR5A3"),
		SVR_IMAGE_TEST("kin_dolabel2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_dolabel3", "BGR5A3"),
		SVR_IMAGE_TEST("kin_kagebkaku", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_kagebmaru", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("kin_kobookstand", "BGR5A3"),
		SVR_IMAGE_TEST("kin_koedbook2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_kohibook2", "BGR5A3"),
		SVR_IMAGE_TEST("kin_kohibook3", "BGR5A3"),
		SVR_IMAGE_TEST("kin_kokami", "BGR5A3"),
		SVR_IMAGE_TEST("kin_kophotstand1", "BGR5A3"),
		SVR_IMAGE_TEST("kin_koribbon", "BGR5A3"),
		SVR_IMAGE_TEST("trt_g00_gass", "BGR888_ABGR7888"),
		SVR_IMAGE_TEST("trt_g00_lawn00small", "BGR5A3"),
		SVR_IMAGE_TEST("trt_g00_river01", "BGR5A3"),
		SVR_IMAGE_TEST("trt_g00_river02", "BGR5A3"))
	, ImageDecoderTest::test_case_suffix_generator);

// NOTE: DidjTex files aren't gzipped because the texture data is
// internally compressed using zlib.
#define DidjTex_IMAGE_TEST(file) ImageDecoderTest_mode( \
			"DidjTex/" file ".tex", \
			"DidjTex/" file ".png")
INSTANTIATE_TEST_SUITE_P(DidjTex, ImageDecoderTest,
	::testing::Values(
		DidjTex_IMAGE_TEST("LeftArrow"),
		DidjTex_IMAGE_TEST("LightOff"),
		DidjTex_IMAGE_TEST("Slider"),
		DidjTex_IMAGE_TEST("StaticTVImage"),
		DidjTex_IMAGE_TEST("Zone1Act1Icon_Alpha"),
		DidjTex_IMAGE_TEST("Zone1Act1Icon"))
	, ImageDecoderTest::test_case_suffix_generator);

#ifdef ENABLE_PVRTC
// PowerVR3 tests
#define PowerVR3_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"PowerVR3/" file ".pvr.gz", \
			"PowerVR3/" file ".pvr.png", (format))
INSTANTIATE_TEST_SUITE_P(PowerVR3, ImageDecoderTest,
	::testing::Values(
		//PowerVR3_IMAGE_TEST("brdfLUT", "RG1616"),					// TODO: R16fG16f
		//PowerVR3_IMAGE_TEST("GnomeHorde-bigMushroom_texture", "PVRTC 4bpp RGB"),	// FIXME: Failing (PVRTC-I 4bpp RGB)
		//PowerVR3_IMAGE_TEST("GnomeHorde-fern", "PVRTC 4bpp RGBA"),			// FIXME: Failing (PVRTC-I 4bpp RGBA)
		PowerVR3_IMAGE_TEST("Navigation3D-font", "A8"),
		//PowerVR3_IMAGE_TEST("Navigation3D-Road", "LA88"),				// FIXME: Failing (LA88)
		//PowerVR3_IMAGE_TEST("Satyr-Table", "RGBA8888"),				// FIXME: Failing (RGBA8888)
		PowerVR3_IMAGE_TEST("text-fri", "RGBA8888"))					// 32x16, caused rp_image::flip(FLIP_V) to break
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_PVRTC */

// TGA tests
#define TGA_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"TGA/" file ".tga.gz", \
			"TGA/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(TGA, ImageDecoderTest,
	::testing::Values(
		// Reference images.
		ImageDecoderTest_mode("TGA/TGA_1_CM24_IM8.tga.gz", "CI8-reference.png", "8bpp with RGB888 palette"),
		ImageDecoderTest_mode("TGA/TGA_1_CM32_IM8.tga.gz", "CI8a-reference.png", "8bpp with ARGB8888 palette"),
		ImageDecoderTest_mode("TGA/TGA_2_24.tga.gz", "rgb-reference.png", "RGB888"),
		ImageDecoderTest_mode("TGA/TGA_2_32.tga.gz", "argb-reference.png", "ARGB8888"),
		ImageDecoderTest_mode("TGA/TGA_3_8.tga.gz", "gray-reference.png", "8bpp grayscale"),
		ImageDecoderTest_mode("TGA/TGA_9_CM24_IM8.tga.gz", "CI8-reference.png", "8bpp with RGB888 palette"),
		ImageDecoderTest_mode("TGA/TGA_9_CM32_IM8.tga.gz", "CI8a-reference.png", "8bpp with ARGB8888 palette"),
		ImageDecoderTest_mode("TGA/TGA_10_24.tga.gz", "rgb-reference.png", "RGB888"),
		ImageDecoderTest_mode("TGA/TGA_10_32.tga.gz", "argb-reference.png", "ARGB8888"),
		ImageDecoderTest_mode("TGA/TGA_11_8.tga.gz", "gray-reference.png", "8bpp grayscale"),

		// TGA 2.0 conformance test suite
		// FIXME: utc16, utc32 have incorrect alpha values?
		// Both gimp and imagemagick interpret them as completely transparent.
		TGA_IMAGE_TEST("conformance/cbw8", "8bpp grayscale"),
		TGA_IMAGE_TEST("conformance/ccm8", "8bpp with RGB555 palette"),
		TGA_IMAGE_TEST("conformance/ctc24", "RGB888"),
		TGA_IMAGE_TEST("conformance/ubw8", "8bpp grayscale"),
		TGA_IMAGE_TEST("conformance/ucm8", "8bpp with RGB555 palette"),
		//TGA_IMAGE_TEST("conformance/utc16", "ARGB1555"),
		TGA_IMAGE_TEST("conformance/utc24", "RGB888"),
		//TGA_IMAGE_TEST("conformance/utc32", "ARGB8888"),

		// Test images from tga-go
		// https://github.com/ftrvxmtrx/tga
		// FIXME: Some incorrect alpha values...
		// NOTE: The rgb24/rgb32 colormap images use .1.png; others use .0.png.
		//ImageDecoderTest_mode("TGA/tga-go/ctc16.tga.gz", "TGA/tga-go/color.png", "ARGB1555"),
		//ImageDecoderTest_mode("TGA/tga-go/ctc32.tga.gz", "TGA/tga-go/ctc32-TODO.png", "ARGB8888"),
		ImageDecoderTest_mode("TGA/tga-go/monochrome8_bottom_left_rle.tga.gz", "TGA/tga-go/monochrome8.png", "8bpp grayscale"),
		ImageDecoderTest_mode("TGA/tga-go/monochrome8_bottom_left.tga.gz", "TGA/tga-go/monochrome8.png", "8bpp grayscale"),
		//ImageDecoderTest_mode("TGA/tga-go/monochrome16_bottom_left_rle.tga.gz", "TGA/tga-go/monochrome16.png", "IA8"),
		//ImageDecoderTest_mode("TGA/tga-go/monochrome16_bottom_left.tga.gz", "TGA/tga-go/monochrome16.png", "IA8"),
		ImageDecoderTest_mode("TGA/tga-go/rgb24_bottom_left_rle.tga.gz", "TGA/tga-go/rgb24.0.png", "RGB888"),
		ImageDecoderTest_mode("TGA/tga-go/rgb24_top_left_colormap.tga.gz", "TGA/tga-go/rgb24.1.png", "8bpp with RGB888 palette"),
		ImageDecoderTest_mode("TGA/tga-go/rgb24_top_left.tga.gz", "TGA/tga-go/rgb24.0.png", "RGB888"),
		ImageDecoderTest_mode("TGA/tga-go/rgb32_bottom_left.tga.gz", "TGA/tga-go/rgb32.0.png", "ARGB8888"),
		//ImageDecoderTest_mode("TGA/tga-go/rgb32_top_left_rle_colormap.tga.gz", "TGA/tga-go/rgb32.1.png", "8bpp with xRGB8888 palette"),
		ImageDecoderTest_mode("TGA/tga-go/rgb32_top_left_rle.tga.gz", "TGA/tga-go/rgb32.0.png", "ARGB8888"))
	, ImageDecoderTest::test_case_suffix_generator);

// Godot STEX3 tests
#define STEX3_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"STEX3/" file ".stex.gz", \
			"STEX3/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(STEX3, ImageDecoderTest,
	::testing::Values(
		STEX3_IMAGE_TEST("argb.BPTC_RGBA", "BPTC_RGBA"),
		STEX3_IMAGE_TEST("argb.DXT5", "DXT5"),
		STEX3_IMAGE_TEST("argb.ETC2_RGBA8", "ETC2_RGBA8"),
		STEX3_IMAGE_TEST("argb.PVRTC1_4A", "PVRTC1_4A"),
		STEX3_IMAGE_TEST("argb.RGBA4444", "RGBA4444"),
		ImageDecoderTest_mode("STEX3/argb.RGBA8.stex.gz", "argb-reference.png", "RGBA8"),

		STEX3_IMAGE_TEST("rgb.BPTC_RGBA", "BPTC_RGBA"),
		STEX3_IMAGE_TEST("rgb.DXT1", "DXT1"),
		STEX3_IMAGE_TEST("rgb.ETC2_RGB8", "ETC2_RGB8"),
		STEX3_IMAGE_TEST("rgb.ETC", "ETC"),
		STEX3_IMAGE_TEST("rgb.PVRTC1_4", "PVRTC1_4"),
		ImageDecoderTest_mode("STEX3/rgb.RGB8.stex.gz", "rgb-reference.png", "RGB8"),

		STEX3_IMAGE_TEST("gray.BPTC_RGBA", "BPTC_RGBA"),
		STEX3_IMAGE_TEST("gray.DXT1", "DXT1"),
		STEX3_IMAGE_TEST("gray.ETC2_RGBA8", "ETC2_RGBA8"),
		STEX3_IMAGE_TEST("gray.ETC", "ETC"),
		STEX3_IMAGE_TEST("gray.PVRTC1_4", "PVRTC1_4"),
		ImageDecoderTest_mode("STEX3/gray.L8.stex.gz", "gray-reference.png", "L8"),

		// Sonic Colors Ultimate test textures
		STEX3_IMAGE_TEST("TEST_RR_areaMap-bg.tga-RGBE9995", "RGBE9995"),
		STEX3_IMAGE_TEST("2K_Sonic_Colors_Logo_ULTIMATE_JP_FLAT.tga-e7746b1823e491fe8eda38393405ae1b.astc-low", "ASTC_8x8"),

		// NOTE: No pixel format for embedded PNGs.
		ImageDecoderTest_mode("STEX3/argb.PNG.mipmaps.stex", "argb-reference.png", ""),
		ImageDecoderTest_mode("STEX3/rgb.PNG.stex", "rgb-reference.png", ""))
	, ImageDecoderTest::test_case_suffix_generator);

// Godot STEX4 tests
// NOTE: Godot 4 uses different encoders for DXTn and ETCn,
// so the decompressed images will not match STEX3.
#define STEX4_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"STEX4/" file ".stex.gz", \
			"STEX4/" file ".png", (format))
#define CTEX4_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"STEX4/" file ".ctex.gz", \
			"STEX4/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(STEX4, ImageDecoderTest,
	::testing::Values(
		STEX4_IMAGE_TEST("argb.DXT5", "DXT5"),
		STEX4_IMAGE_TEST("argb.ETC2_RGBA8", "ETC2_RGBA8"),
		ImageDecoderTest_mode("STEX4/argb.RGBA8.stex.gz", "argb-reference.png", "RGBA8"),

		// Godot 4 encodes rgb-reference.png using DXT5 instead of DXT1 for some reason.
		STEX4_IMAGE_TEST("rgb.DXT5", "DXT5"),
		STEX4_IMAGE_TEST("rgb.ETC2_RGB8", "ETC2_RGB8"),
		ImageDecoderTest_mode("STEX4/rgb.RGB8.stex.gz", "rgb-reference.png", "RGB8"),

		STEX4_IMAGE_TEST("gray.DXT1", "DXT1"),
		STEX4_IMAGE_TEST("gray.ETC", "ETC"),
		ImageDecoderTest_mode("STEX4/gray.L8.stex.gz", "gray-reference.png"),

		// Godot 4 prefers the .ctex extension now, so any new
		// tests added after this point should use .ctex.
		CTEX4_IMAGE_TEST("argb.ASTC_4x4", "ASTC_4x4"),
		CTEX4_IMAGE_TEST("argb.BPTC_RGBA", "BPTC_RGBA"),

		// NOTE: No pixel format for embedded PNGs.
		ImageDecoderTest_mode("STEX4/argb.PNG.mipmaps.ctex", "argb-reference.png", ""),
		ImageDecoderTest_mode("STEX4/rgb.PNG.ctex", "rgb-reference.png", ""))
	, ImageDecoderTest::test_case_suffix_generator);

// Xbox XPR tests
#define XPR_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"XPR/" file ".xpr.gz", \
			"XPR/" file ".png", (format))
#define XBX_IMAGE_TEST(file, format) ImageDecoderTest_mode( \
			"XPR/" file ".xbx.gz", \
			"XPR/" file ".png", (format))
INSTANTIATE_TEST_SUITE_P(XPR, ImageDecoderTest,
	::testing::Values(
		XPR_IMAGE_TEST("bkgd_load9", "DXT1"),
		XPR_IMAGE_TEST("bkgd_main", "Linear ARGB8888"),
		XPR_IMAGE_TEST("bkgd_title", "Linear ARGB8888"),
		XBX_IMAGE_TEST("SE-043.TitleImage", "DXT1"),
		XPR_IMAGE_TEST("SplashScreen_JunkieXl", "Linear ARGB8888"))
	, ImageDecoderTest::test_case_suffix_generator);

// TODO: NPOT tests for compressed formats. (partial block sizes)

} }

/**
 * Test suite main function.
 * Called by gtest_init.cpp.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fprintf(stderr, "LibRomData test suite: ImageDecoder tests.\n\n");
	fprintf(stderr, "Benchmark iterations: %u (%u for BC7)\n",
		LibRomData::Tests::ImageDecoderTest::BENCHMARK_ITERATIONS,
		LibRomData::Tests::ImageDecoderTest::BENCHMARK_ITERATIONS_BC7);
	fflush(nullptr);

	// Check for the ImageDecoder_data directory and chdir() into it.
#ifdef _WIN32
	static const TCHAR *const subdirs[] = {
		_T("ImageDecoder_data"),
		_T("bin\\ImageDecoder_data"),
		_T("src\\libromdata\\tests\\img\\ImageDecoder_data"),
		_T("..\\src\\libromdata\\tests\\img\\ImageDecoder_data"),
		_T("..\\..\\src\\libromdata\\tests\\img\\ImageDecoder_data"),
		_T("..\\..\\..\\src\\libromdata\\tests\\img\\ImageDecoder_data"),
		_T("..\\..\\..\\..\\src\\libromdata\\tests\\img\\ImageDecoder_data"),
		_T("..\\..\\..\\..\\..\\src\\libromdata\\tests\\img\\ImageDecoder_data"),
		_T("..\\..\\..\\bin\\ImageDecoder_data"),
		_T("..\\..\\..\\bin\\Debug\\ImageDecoder_data"),
		_T("..\\..\\..\\bin\\Release\\ImageDecoder_data"),
	};
#else /* !_WIN32 */
	static const TCHAR *const subdirs[] = {
		_T("ImageDecoder_data"),
		_T("bin/ImageDecoder_data"),
		_T("src/libromdata/tests/img/ImageDecoder_data"),
		_T("../src/libromdata/tests/img/ImageDecoder_data"),
		_T("../../src/libromdata/tests/img/ImageDecoder_data"),
		_T("../../../src/libromdata/tests/img/ImageDecoder_data"),
		_T("../../../../src/libromdata/tests/img/ImageDecoder_data"),
		_T("../../../../../src/libromdata/tests/img/ImageDecoder_data"),
		_T("../../../../../../src/libromdata/tests/img/ImageDecoder_data"),
		_T("../../../bin/ImageDecoder_data"),
	};
#endif /* _WIN32 */

	bool is_found = false;
	for (const TCHAR *const subdir : subdirs) {
		if (!_taccess(subdir, R_OK)) {
			if (_tchdir(subdir) == 0) {
				is_found = true;
				break;
			}
		}
	}

	if (!is_found) {
		fputs("*** ERROR: Cannot find the ImageDecoder_data test images directory.\n", stderr);
		return EXIT_FAILURE;
	}

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
