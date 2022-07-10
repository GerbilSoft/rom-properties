/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * ImageDecoderTest.cpp: ImageDecoder class test.                          *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
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
#include "librpfile/RpFile.hpp"
#include "librpfile/MemFile.hpp"
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
using std::u8string;
using std::unique_ptr;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData { namespace Tests {

struct ImageDecoderTest_mode
{
	u8string dds_gz_filename;
	u8string png_filename;
	RomData::ImageType imgType;

	ImageDecoderTest_mode(
		const char8_t *dds_gz_filename,
		const char8_t *png_filename,
		RomData::ImageType imgType = RomData::IMG_INT_IMAGE
		)
		: dds_gz_filename(dds_gz_filename)
		, png_filename(png_filename)
		, imgType(imgType)
	{ }

	// May be required for MSVC 2010?
	ImageDecoderTest_mode(const ImageDecoderTest_mode &other)
		: dds_gz_filename(other.dds_gz_filename)
		, png_filename(other.png_filename)
		, imgType(other.imgType)
	{ }

	// Required for MSVC 2010.
	ImageDecoderTest_mode &operator=(const ImageDecoderTest_mode &other)
	{
		dds_gz_filename = other.dds_gz_filename;
		png_filename = other.png_filename;
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
		static inline void replace_slashes(u8string &path);

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
static inline ::std::ostream& operator<<(::std::ostream& os, const ImageDecoderTest_mode& mode)
{
	return os << reinterpret_cast<const char*>(mode.dds_gz_filename.c_str());
};

/**
 * ::testing::Message formatting function for char8_t
 * TODO: Move to a common gtest header file?
 */
static inline ::testing::Message& operator<<(::testing::Message& msg, const char8_t *str)
{
	return msg << reinterpret_cast<const char*>(str);
}

/**
 * ::testing::Message formatting function for u8string
 * TODO: Move to a common gtest header file?
 */
static inline ::testing::Message& operator<<(::testing::Message& msg, const u8string& str)
{
	return msg << reinterpret_cast<const char*>(str.c_str());
}

/**
 * Replace slashes with backslashes on Windows.
 * @param path Pathname.
 */
inline void ImageDecoderTest::replace_slashes(u8string &path)
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
	u8string path = U8("ImageDecoder_data");
	path += DIR_SEP_CHR;
	path += mode.dds_gz_filename;
	replace_slashes(path);
	// FIXME: U8STRFIX
	m_gzDds = gzopen(reinterpret_cast<const char*>(path.c_str()), "rb");
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
	path.resize(18);	// Back to "ImageDecoder_data/".
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
	ASSERT_TRUE(img_png != nullptr) << "Could not load the PNG image as rp_image.";
	ASSERT_TRUE(img_png->isValid()) << "Could not load the PNG image as rp_image.";

	// Open the image as an IRpFile.
	m_f_dds = new MemFile(m_dds_buf.data(), m_dds_buf.size());
	ASSERT_TRUE(m_f_dds->isOpen()) << "Could not create MemFile for the DDS image.";
	m_f_dds->setFilename(mode.dds_gz_filename);

	// Load the image file.
	m_romData = RomDataFactory::create(m_f_dds);
	ASSERT_TRUE(m_romData != nullptr) << "Could not load the DDS image.";
	ASSERT_TRUE(m_romData->isValid()) << "Could not load the DDS image.";
	ASSERT_TRUE(m_romData->isOpen()) << "Could not load the DDS image.";

	// Get the DDS image as an rp_image.
	const rp_image *const img_dds = m_romData->image(mode.imgType);
	ASSERT_TRUE(img_dds != nullptr) << "Could not load the DDS image as rp_image.";

	// Get the image again.
	// The pointer should be identical to the first one.
	const rp_image *const img_dds_2 = m_romData->image(mode.imgType);
	EXPECT_EQ(img_dds, img_dds_2) << "Retrieving the image twice resulted in a different rp_image object.";

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
	if (mode.dds_gz_filename.find(U8("BC7/")) == 0) {
		// This is BC7.
		max_iterations = BENCHMARK_ITERATIONS_BC7;
	} else {
		// Not BC7.
		max_iterations = BENCHMARK_ITERATIONS;
	}

	// Load the image file.
	// TODO: RomDataFactory function to retrieve a constructor function?
	auto fn_ctor = [](IRpFile *file) { return RomDataFactory::create(file); };

	// For certain types, increase the number of iterations.
	ASSERT_GT(mode.dds_gz_filename.size(), 4U);
	if (mode.dds_gz_filename.size() >= 8U &&
	    !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-8, 8, U8(".smdh.gz")))
	{
		// Nintendo 3DS SMDH file
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, U8(".gci.gz"))) {
		// Nintendo GameCube save file
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-4, 4, U8(".VMS"))) {
		// Sega Dreamcast save file
		// NOTE: RomDataFactory and DreamcastSave don't support gzip at the moment.
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, U8(".PSV.gz"))) {
		// Sony PlayStation save file
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, U8(".nds.gz"))) {
		// Nintendo DS ROM image
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, U8(".cab.gz")) ||
		   !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, U8(".prb.gz"))) {
		// Nintendo Badge Arcade texture
		// NOTE: Increased iterations due to smaller files.
		max_iterations *= 10;
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-4, 4, U8(".tex"))) {
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
	// FIXME: U8STRFIX
	string suffix = reinterpret_cast<const char*>(info.param.dds_gz_filename.c_str());

	// Replace all non-alphanumeric characters with '_'.
	// See gtest-param-util.h::IsValidParamName().
	std::replace_if(suffix.begin(), suffix.end(),
		[](char c) { return !ISALNUM(c); }, '_');

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
INSTANTIATE_TEST_SUITE_P(DDS_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("S3TC/dxt1-rgb.dds.gz"),
			U8("S3TC/dxt1-rgb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/dxt2-rgb.dds.gz"),
			U8("S3TC/dxt2-rgb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/dxt2-argb.dds.gz"),
			U8("S3TC/dxt2-argb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/dxt3-rgb.dds.gz"),
			U8("S3TC/dxt3-rgb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/dxt3-argb.dds.gz"),
			U8("S3TC/dxt3-argb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/dxt4-rgb.dds.gz"),
			U8("S3TC/dxt4-rgb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/dxt4-argb.dds.gz"),
			U8("S3TC/dxt4-argb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/dxt5-rgb.dds.gz"),
			U8("S3TC/dxt5-rgb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/dxt5-argb.dds.gz"),
			U8("S3TC/dxt5-argb.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/bc4.dds.gz"),
			U8("S3TC/bc4.s3tc.png")),
		ImageDecoderTest_mode(
			U8("S3TC/bc5.dds.gz"),
			U8("S3TC/bc5.s3tc.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 16-bit RGB)
INSTANTIATE_TEST_SUITE_P(DDS_RGB16, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("RGB/RGB565.dds.gz"),
			U8("RGB/RGB565.png")),
		ImageDecoderTest_mode(
			U8("RGB/xRGB4444.dds.gz"),
			U8("RGB/xRGB4444.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 16-bit ARGB)
INSTANTIATE_TEST_SUITE_P(DDS_ARGB16, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("ARGB/ARGB1555.dds.gz"),
			U8("ARGB/ARGB1555.png")),
		ImageDecoderTest_mode(
			U8("ARGB/ARGB4444.dds.gz"),
			U8("ARGB/ARGB4444.png")),
		ImageDecoderTest_mode(
			U8("ARGB/ARGB8332.dds.gz"),
			U8("ARGB/ARGB8332.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 15-bit RGB)
INSTANTIATE_TEST_SUITE_P(DDS_RGB15, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("RGB/RGB565.dds.gz"),
			U8("RGB/RGB565.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 24-bit RGB)
INSTANTIATE_TEST_SUITE_P(DDS_RGB24, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("RGB/RGB888.dds.gz"),
			U8("RGB/RGB888.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 32-bit RGB)
INSTANTIATE_TEST_SUITE_P(DDS_RGB32, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("RGB/xRGB8888.dds.gz"),
			U8("RGB/xRGB8888.png")),
		ImageDecoderTest_mode(
			U8("RGB/xBGR8888.dds.gz"),
			U8("RGB/xBGR8888.png")),

		// Uncommon formats.
		ImageDecoderTest_mode(
			U8("RGB/G16R16.dds.gz"),
			U8("RGB/G16R16.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Uncompressed 32-bit ARGB)
INSTANTIATE_TEST_SUITE_P(DDS_ARGB32, ImageDecoderTest,
	::testing::Values(
		// 32-bit
		ImageDecoderTest_mode(
			U8("ARGB/ARGB8888.dds.gz"),
			U8("ARGB/ARGB8888.png")),
		ImageDecoderTest_mode(
			U8("ARGB/ABGR8888.dds.gz"),
			U8("ARGB/ABGR8888.png")),

		// Uncommon formats.
		ImageDecoderTest_mode(
			U8("ARGB/A2R10G10B10.dds.gz"),
			U8("ARGB/A2R10G10B10.png")),
		ImageDecoderTest_mode(
			U8("ARGB/A2B10G10R10.dds.gz"),
			U8("ARGB/A2B10G10R10.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Luminance)
INSTANTIATE_TEST_SUITE_P(DDS_Luma, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("Luma/L8.dds.gz"),
			U8("Luma/L8.png")),
		ImageDecoderTest_mode(
			U8("Luma/A4L4.dds.gz"),
			U8("Luma/A4L4.png")),
		ImageDecoderTest_mode(
			U8("Luma/L16.dds.gz"),
			U8("Luma/L16.png")),
		ImageDecoderTest_mode(
			U8("Luma/A8L8.dds.gz"),
			U8("Luma/A8L8.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests (Alpha)
INSTANTIATE_TEST_SUITE_P(DDS_Alpha, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("Alpha/A8.dds.gz"),
			U8("Alpha/A8.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests (square twiddled)
INSTANTIATE_TEST_SUITE_P(PVR_SqTwiddled, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("PVR/bg_00.pvr.gz"),
			U8("PVR/bg_00.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests (VQ)
INSTANTIATE_TEST_SUITE_P(PVR_VQ, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("PVR/mr_128k_huti.pvr.gz"),
			U8("PVR/mr_128k_huti.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests (Small VQ)
INSTANTIATE_TEST_SUITE_P(PVR_SmallVQ, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("PVR/drumfuta1.pvr.gz"),
			U8("PVR/drumfuta1.png")),
		ImageDecoderTest_mode(
			U8("PVR/drum_ref.pvr.gz"),
			U8("PVR/drum_ref.png")),
		ImageDecoderTest_mode(
			U8("PVR/sp_blue.pvr.gz"),
			U8("PVR/sp_blue.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// GVR tests (RGB5A3)
INSTANTIATE_TEST_SUITE_P(GVR_RGB5A3, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("GVR/zanki_sonic.gvr.gz"),
			U8("GVR/zanki_sonic.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// GVR tests (DXT1, S3TC)
INSTANTIATE_TEST_SUITE_P(GVR_DXT1_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("GVR/paldam_off.gvr.gz"),
			U8("GVR/paldam_off.s3tc.png")),
		ImageDecoderTest_mode(
			U8("GVR/paldam_on.gvr.gz"),
			U8("GVR/paldam_on.s3tc.png")),
		ImageDecoderTest_mode(
			U8("GVR/weeklytitle.gvr.gz"),
			U8("GVR/weeklytitle.s3tc.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// KTX tests
#define KTX_IMAGE_TEST(file) ImageDecoderTest_mode( \
			U8("KTX/") file U8(".ktx.gz"), \
			U8("KTX/") file U8(".png"))
INSTANTIATE_TEST_SUITE_P(KTX, ImageDecoderTest,
	::testing::Values(
		// RGB reference image.
		ImageDecoderTest_mode(
			U8("KTX/rgb-reference.ktx.gz"),
			U8("KTX/rgb.png")),
		// RGB reference image, mipmap levels == 0
		ImageDecoderTest_mode(
			U8("KTX/rgb-amg-reference.ktx.gz"),
			U8("KTX/rgb.png")),
		// Orientation: Up (upside-down compared to "normal")
		ImageDecoderTest_mode(
			U8("KTX/up-reference.ktx.gz"),
			U8("KTX/up.png")),
		// Orientation: Down (same as "normal")
		ImageDecoderTest_mode(
			U8("KTX/down-reference.ktx.gz"),
			U8("KTX/up.png")),

		// Luminance (unsized: GL_LUMINANCE)
		ImageDecoderTest_mode(
			U8("KTX/luminance_unsized_reference.ktx.gz"),
			U8("KTX/luminance.png")),
		// Luminance (sized: GL_LUMINANCE8)
		ImageDecoderTest_mode(
			U8("KTX/luminance_sized_reference.ktx.gz"),
			U8("KTX/luminance.png")),

		// ETC1
		KTX_IMAGE_TEST(U8("etc1")),
		// ETC2
		KTX_IMAGE_TEST(U8("etc2-rgb")),
		KTX_IMAGE_TEST(U8("etc2-rgba1")),
		KTX_IMAGE_TEST(U8("etc2-rgba8")),

		// BGR888 (Hi Corp)
		KTX_IMAGE_TEST(U8("hi_mark")),
		KTX_IMAGE_TEST(U8("hi_mark_sq")),

		// EAC
		KTX_IMAGE_TEST(U8("conftestimage_R11_EAC")),
		KTX_IMAGE_TEST(U8("conftestimage_RG11_EAC")),
		KTX_IMAGE_TEST(U8("conftestimage_SIGNED_R11_EAC")),
		KTX_IMAGE_TEST(U8("conftestimage_SIGNED_RG11_EAC")),

		// RGBA reference image
		ImageDecoderTest_mode(
			U8("KTX/rgba-reference.ktx.gz"),
			U8("KTX/rgba.png")))

	, ImageDecoderTest::test_case_suffix_generator);

// KTX2 tests
#define KTX2_IMAGE_TEST(file) ImageDecoderTest_mode( \
			U8("KTX2/") file U8(".ktx2.gz"), \
			U8("KTX2/") file U8(".png"))
INSTANTIATE_TEST_SUITE_P(KTX2, ImageDecoderTest,
	::testing::Values(
		KTX2_IMAGE_TEST(U8("cubemap_yokohama_bc3_unorm")),
		KTX2_IMAGE_TEST(U8("cubemap_yokohama_etc2_unorm")),
		KTX2_IMAGE_TEST(U8("orient-down-metadata-u")),
		KTX2_IMAGE_TEST(U8("orient-up-metadata-u")),
		KTX2_IMAGE_TEST(U8("pattern_02_bc2")),
		KTX2_IMAGE_TEST(U8("rgba-reference-u")),
		KTX2_IMAGE_TEST(U8("rgb-mipmap-reference-u")),
		KTX2_IMAGE_TEST(U8("texturearray_bc3_unorm")),
		KTX2_IMAGE_TEST(U8("texturearray_etc2_unorm")))
	, ImageDecoderTest::test_case_suffix_generator);

// Valve VTF tests (all formats)
INSTANTIATE_TEST_SUITE_P(VTF, ImageDecoderTest,
	::testing::Values(
		// NOTE: VTF channel ordering is usually backwards from ImageDecoder.

		// 32-bit ARGB
		ImageDecoderTest_mode(
			U8("VTF/ABGR8888.vtf.gz"),
			U8("argb-reference.png")),
		ImageDecoderTest_mode(
			U8("VTF/ARGB8888.vtf.gz"),	// NOTE: Actually RABG8888.
			U8("argb-reference.png")),
		ImageDecoderTest_mode(
			U8("VTF/BGRA8888.vtf.gz"),
			U8("argb-reference.png")),
		ImageDecoderTest_mode(
			U8("VTF/RGBA8888.vtf.gz"),
			U8("argb-reference.png")),

		// 32-bit xRGB
		ImageDecoderTest_mode(
			U8("VTF/BGRx8888.vtf.gz"),
			U8("rgb-reference.png")),

		// 24-bit RGB
		ImageDecoderTest_mode(
			U8("VTF/BGR888.vtf.gz"),
			U8("rgb-reference.png")),
		ImageDecoderTest_mode(
			U8("VTF/RGB888.vtf.gz"),
			U8("rgb-reference.png")),

		// 24-bit RGB + bluescreen
		ImageDecoderTest_mode(
			U8("VTF/BGR888_bluescreen.vtf.gz"),
			U8("VTF/BGR888_bluescreen.png")),
		ImageDecoderTest_mode(
			U8("VTF/RGB888_bluescreen.vtf.gz"),
			U8("VTF/BGR888_bluescreen.png")),

		// 16-bit RGB (565)
		// FIXME: Tests are failing.
		ImageDecoderTest_mode(
			U8("VTF/BGR565.vtf.gz"),
			U8("RGB/RGB565.png")),
		ImageDecoderTest_mode(
			U8("VTF/RGB565.vtf.gz"),
			U8("RGB/RGB565.png")),

		// 15-bit RGB (555)
		ImageDecoderTest_mode(
			U8("VTF/BGRx5551.vtf.gz"),
			U8("RGB/RGB555.png")),

		// 16-bit ARGB (4444)
		ImageDecoderTest_mode(
			U8("VTF/BGRA4444.vtf.gz"),
			U8("ARGB/ARGB4444.png")),

		// UV88 (handled as RG88)
		ImageDecoderTest_mode(
			U8("VTF/UV88.vtf.gz"),
			U8("rg-reference.png")),

		// Intensity formats
		ImageDecoderTest_mode(
			U8("VTF/I8.vtf.gz"),
			U8("Luma/L8.png")),
		ImageDecoderTest_mode(
			U8("VTF/IA88.vtf.gz"),
			U8("Luma/A8L8.png")),

		// Alpha format (A8)
		ImageDecoderTest_mode(
			U8("VTF/A8.vtf.gz"),
			U8("Alpha/A8.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// Valve VTF tests (S3TC)
INSTANTIATE_TEST_SUITE_P(VTF_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("VTF/DXT1.vtf.gz"),
			U8("VTF/DXT1.s3tc.png")),
		ImageDecoderTest_mode(
			U8("VTF/DXT1_A1.vtf.gz"),
			U8("VTF/DXT1_A1.s3tc.png")),
		ImageDecoderTest_mode(
			U8("VTF/DXT3.vtf.gz"),
			U8("VTF/DXT3.s3tc.png")),
		ImageDecoderTest_mode(
			U8("VTF/DXT5.vtf.gz"),
			U8("VTF/DXT5.s3tc.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// Valve VTF3 tests (S3TC)
INSTANTIATE_TEST_SUITE_P(VTF3_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("VTF3/elevator_screen_broken_normal.ps3.vtf.gz"),
			U8("VTF3/elevator_screen_broken_normal.ps3.s3tc.png")),
		ImageDecoderTest_mode(
			U8("VTF3/elevator_screen_colour.ps3.vtf.gz"),
			U8("VTF3/elevator_screen_colour.ps3.s3tc.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// Test images from texture-compressor.
// Reference: https://github.com/TimvanScherpenzeel/texture-compressor
INSTANTIATE_TEST_SUITE_P(TCtest, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("tctest/example-etc1.ktx.gz"),
			U8("tctest/example-etc1.ktx.png")),
		ImageDecoderTest_mode(
			U8("tctest/example-etc2.ktx.gz"),
			U8("tctest/example-etc2.ktx.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// texture-compressor tests (S3TC)
INSTANTIATE_TEST_SUITE_P(TCtest_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("tctest/example-dxt1.dds.gz"),
			U8("tctest/example-dxt1.s3tc.dds.png")),
		ImageDecoderTest_mode(
			U8("tctest/example-dxt3.dds.gz"),
			U8("tctest/example-dxt5.s3tc.dds.png")),
		ImageDecoderTest_mode(
			U8("tctest/example-dxt5.dds.gz"),
			U8("tctest/example-dxt5.s3tc.dds.png")))
	, ImageDecoderTest::test_case_suffix_generator);

#ifdef ENABLE_PVRTC
// texture-compressor tests (PVRTC)
INSTANTIATE_TEST_SUITE_P(TCtest_PVRTC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("tctest/example-pvrtc1.pvr.gz"),
			U8("tctest/example-pvrtc1.pvr.png")),
		ImageDecoderTest_mode(
			U8("tctest/example-pvrtc2-2bpp.pvr.gz"),
			U8("tctest/example-pvrtc2-2bpp.pvr.png")),
		ImageDecoderTest_mode(
			U8("tctest/example-pvrtc2-4bpp.pvr.gz"),
			U8("tctest/example-pvrtc2-4bpp.pvr.png")))
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_PVRTC */

#ifdef ENABLE_ASTC
// texture-compressor tests (ASTC)
INSTANTIATE_TEST_SUITE_P(TCtest_ASTC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("tctest/example-astc.dds.gz"),
			U8("tctest/example-astc.png")))
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_ASTC */

// BC7 tests
INSTANTIATE_TEST_SUITE_P(BC7, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("BC7/w5_grass200_abd_a.dds.gz"),
			U8("BC7/w5_grass200_abd_a.png")),
		ImageDecoderTest_mode(
			U8("BC7/w5_grass201_abd.dds.gz"),
			U8("BC7/w5_grass201_abd.png")),
		ImageDecoderTest_mode(
			U8("BC7/w5_grass206_abd.dds.gz"),
			U8("BC7/w5_grass206_abd.png")),
		ImageDecoderTest_mode(
			U8("BC7/w5_rock805_abd.dds.gz"),
			U8("BC7/w5_rock805_abd.png")),
		ImageDecoderTest_mode(
			U8("BC7/w5_rock805_nrm.dds.gz"),
			U8("BC7/w5_rock805_nrm.png")),
		ImageDecoderTest_mode(
			U8("BC7/w5_rope801_prm.dds"),
			U8("BC7/w5_rope801_prm.png")),
		ImageDecoderTest_mode(
			U8("BC7/w5_sand504_abd_a.dds.gz"),
			U8("BC7/w5_sand504_abd_a.png")),
		ImageDecoderTest_mode(
			U8("BC7/w5_wood503_prm.dds.gz"),
			U8("BC7/w5_wood503_prm.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// SMDH tests
// From *New* Nintendo 3DS 9.2.0-20J.
#define SMDH_TEST(file) ImageDecoderTest_mode( \
			U8("SMDH/") file U8(".smdh.gz"), \
			U8("SMDH/") file U8(".png"), \
			RomData::IMG_INT_ICON)
INSTANTIATE_TEST_SUITE_P(SMDH, ImageDecoderTest,
	::testing::Values(
		SMDH_TEST(U8("0004001000020000")),
		SMDH_TEST(U8("0004001000020100")),
		SMDH_TEST(U8("0004001000020400")),
		SMDH_TEST(U8("0004001000020900")),
		SMDH_TEST(U8("0004001000020A00")),
		SMDH_TEST(U8("000400100002BF00")),
		SMDH_TEST(U8("0004001020020300")),
		SMDH_TEST(U8("0004001020020D00")),
		SMDH_TEST(U8("0004001020023100")),
		SMDH_TEST(U8("000400102002C800")),
		SMDH_TEST(U8("000400102002C900")),
		SMDH_TEST(U8("000400102002CB00")),
		SMDH_TEST(U8("0004003000008302")),
		SMDH_TEST(U8("0004003000008402")),
		SMDH_TEST(U8("0004003000008602")),
		SMDH_TEST(U8("0004003000008702")),
		SMDH_TEST(U8("0004003000008D02")),
		SMDH_TEST(U8("0004003000008E02")),
		SMDH_TEST(U8("000400300000BC02")),
		SMDH_TEST(U8("000400300000C602")),
		SMDH_TEST(U8("0004003020008802")))
	, ImageDecoderTest::test_case_suffix_generator);

// GCI tests
// TODO: Use something like GcnFstTest that uses an array of filenames
// to generate tests at runtime instead of compile-time?
#define GCI_ICON_TEST(file) ImageDecoderTest_mode( \
			U8("GCI/") file U8(".gci.gz"), \
			U8("GCI/") file U8(".icon.png"), \
			RomData::IMG_INT_ICON)
#define GCI_BANNER_TEST(file) ImageDecoderTest_mode( \
			U8("GCI/") file U8(".gci.gz"), \
			U8("GCI/") file U8(".banner.png"), \
			RomData::IMG_INT_BANNER)

INSTANTIATE_TEST_SUITE_P(GCI_Icon_1, ImageDecoderTest,
	::testing::Values(
		GCI_ICON_TEST(U8("01-D43E-ZELDA")),
		GCI_ICON_TEST(U8("01-G2ME-MetroidPrime2")),
		GCI_ICON_TEST(U8("01-G4SE-gc4sword")),
		GCI_ICON_TEST(U8("01-G8ME-mariost_save_file")),
		GCI_ICON_TEST(U8("01-GAFE-DobutsunomoriP_MURA")),
		GCI_ICON_TEST(U8("01-GALE-SuperSmashBros0110290334")),
		GCI_ICON_TEST(U8("01-GC6E-pokemon_colosseum")),
		GCI_ICON_TEST(U8("01-GEDE-Eternal Darkness")),
		GCI_ICON_TEST(U8("01-GFEE-FIREEMBLEM8J")),
		GCI_ICON_TEST(U8("01-GKGE-DONKEY KONGA SAVEDATA")),
		GCI_ICON_TEST(U8("01-GLME-LUIGI_MANSION_SAVEDATA_v3")),
		GCI_ICON_TEST(U8("01-GM4E-MarioKart Double Dash!!")),
		GCI_ICON_TEST(U8("01-GM8E-MetroidPrime A")),
		GCI_ICON_TEST(U8("01-GMPE-MARIPA4BOX0")),
		GCI_ICON_TEST(U8("01-GMPE-MARIPA4BOX1")),
		GCI_ICON_TEST(U8("01-GMPE-MARIPA4BOX2")),
		GCI_ICON_TEST(U8("01-GMSE-super_mario_sunshine")),
		GCI_ICON_TEST(U8("01-GP5E-MARIPA5")),
		GCI_ICON_TEST(U8("01-GP6E-MARIPA6")),
		GCI_ICON_TEST(U8("01-GP7E-MARIPA7")),
		GCI_ICON_TEST(U8("01-GPIE-Pikmin dataFile")),
		GCI_ICON_TEST(U8("01-GPVE-Pikmin2_SaveData")),
		GCI_ICON_TEST(U8("01-GPXE-pokemon_rs_memory_box")),
		GCI_ICON_TEST(U8("01-GXXE-PokemonXD")),
		GCI_ICON_TEST(U8("01-GYBE-MainData")),
		GCI_ICON_TEST(U8("01-GZ2E-gczelda2")),
		GCI_ICON_TEST(U8("01-GZLE-gczelda")),
		GCI_ICON_TEST(U8("01-PZLE-NES_ZELDA1_SAVE")),
		GCI_ICON_TEST(U8("01-PZLE-NES_ZELDA2_SAVE")),
		GCI_ICON_TEST(U8("01-PZLE-ZELDA1")),
		GCI_ICON_TEST(U8("01-PZLE-ZELDA2")),
		GCI_ICON_TEST(U8("51-GTKE-Save Game0")),
		GCI_ICON_TEST(U8("52-GTDE-SK5sbltitgaSK5sbltitga")),
		GCI_ICON_TEST(U8("52-GTDE-SK5sirpvsicSK5sirpvsic")),
		GCI_ICON_TEST(U8("52-GTDE-SK5xwkqsbafSK5xwkqsbaf")),
		GCI_ICON_TEST(U8("5D-GE9E-EDEDDNEDDYTHEMIS-EDVENTURES")),
		GCI_ICON_TEST(U8("69-GHSE-POTTERCOS")),
		GCI_ICON_TEST(U8("69-GO7E-BOND")),
		GCI_ICON_TEST(U8("78-GW3E-__2f__w_mania2002")),
		GCI_ICON_TEST(U8("7D-GCBE-CRASHWOC")),
		GCI_ICON_TEST(U8("7D-GCNE-all")),
		GCI_ICON_TEST(U8("8P-G2XE-SONIC GEMS_00")),
		GCI_ICON_TEST(U8("8P-G2XE-SONIC_R")),
		GCI_ICON_TEST(U8("8P-G2XE-STF.DAT")),
		GCI_ICON_TEST(U8("8P-G9SE-SONICHEROES_00")),
		GCI_ICON_TEST(U8("8P-G9SE-SONICHEROES_01")),
		GCI_ICON_TEST(U8("8P-G9SE-SONICHEROES_02")),
		GCI_ICON_TEST(U8("8P-GEZE-billyhatcher")),
		GCI_ICON_TEST(U8("8P-GFZE-f_zero.dat")))
	, ImageDecoderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(GCI_Icon_2, ImageDecoderTest,
	::testing::Values(
		GCI_ICON_TEST(U8("8P-GM2E-rep0000010000C900002497A48E.dat")),
		GCI_ICON_TEST(U8("8P-GM2E-super_monkey_ball_2.dat")),
		GCI_ICON_TEST(U8("8P-GMBE-smkb0058556041f42afb")),
		GCI_ICON_TEST(U8("8P-GMBE-super_monkey_ball.sys")),
		GCI_ICON_TEST(U8("8P-GPUE-Puyo Pop Fever Replay01")),
		GCI_ICON_TEST(U8("8P-GPUE-Puyo Pop Fever System")),
		GCI_ICON_TEST(U8("8P-GSNE-SONIC2B__5f____5f__S01")),
		GCI_ICON_TEST(U8("8P-GSOE-S_MEGA_SYS")),
		GCI_ICON_TEST(U8("8P-GUPE-SHADOWTHEHEDGEHOG")),
		GCI_ICON_TEST(U8("8P-GXEE-SONICRIDERS_GAMEDATA_01")),
		GCI_ICON_TEST(U8("8P-GXSE-SONICADVENTURE_DX_PLAYRECORD_1")),
		GCI_ICON_TEST(U8("AF-GNME-NAMCOMUSEUM")),
		GCI_ICON_TEST(U8("AF-GP2E-PMW2SAVE")),
		GCI_ICON_TEST(U8("AF-GPME-PACMANFEVER")))
	, ImageDecoderTest::test_case_suffix_generator);

// NOTE: Some files don't have banners. They're left in the list for
// consistency, but are commented out.
INSTANTIATE_TEST_SUITE_P(GCI_Banner_1, ImageDecoderTest,
	::testing::Values(
		GCI_BANNER_TEST(U8("01-D43E-ZELDA")),
		GCI_BANNER_TEST(U8("01-G2ME-MetroidPrime2")),
		GCI_BANNER_TEST(U8("01-G4SE-gc4sword")),
		GCI_BANNER_TEST(U8("01-G8ME-mariost_save_file")),
		GCI_BANNER_TEST(U8("01-GAFE-DobutsunomoriP_MURA")),
		GCI_BANNER_TEST(U8("01-GALE-SuperSmashBros0110290334")),
		GCI_BANNER_TEST(U8("01-GC6E-pokemon_colosseum")),
		GCI_BANNER_TEST(U8("01-GEDE-Eternal Darkness")),
		GCI_BANNER_TEST(U8("01-GFEE-FIREEMBLEM8J")),
		GCI_BANNER_TEST(U8("01-GKGE-DONKEY KONGA SAVEDATA")),
		GCI_BANNER_TEST(U8("01-GLME-LUIGI_MANSION_SAVEDATA_v3")),
		GCI_BANNER_TEST(U8("01-GM4E-MarioKart Double Dash!!")),
		GCI_BANNER_TEST(U8("01-GM8E-MetroidPrime A")),
		GCI_BANNER_TEST(U8("01-GMPE-MARIPA4BOX0")),
		GCI_BANNER_TEST(U8("01-GMPE-MARIPA4BOX1")),
		GCI_BANNER_TEST(U8("01-GMPE-MARIPA4BOX2")),
		GCI_BANNER_TEST(U8("01-GMSE-super_mario_sunshine")),
		GCI_BANNER_TEST(U8("01-GP5E-MARIPA5")),
		GCI_BANNER_TEST(U8("01-GP6E-MARIPA6")),
		GCI_BANNER_TEST(U8("01-GP7E-MARIPA7")),
		GCI_BANNER_TEST(U8("01-GPIE-Pikmin dataFile")),
		GCI_BANNER_TEST(U8("01-GPVE-Pikmin2_SaveData")),
		GCI_BANNER_TEST(U8("01-GPXE-pokemon_rs_memory_box")),
		GCI_BANNER_TEST(U8("01-GXXE-PokemonXD")),
		GCI_BANNER_TEST(U8("01-GYBE-MainData")),
		GCI_BANNER_TEST(U8("01-GZ2E-gczelda2")),
		GCI_BANNER_TEST(U8("01-GZLE-gczelda")),
		GCI_BANNER_TEST(U8("01-PZLE-NES_ZELDA1_SAVE")),
		GCI_BANNER_TEST(U8("01-PZLE-NES_ZELDA2_SAVE")),
		GCI_BANNER_TEST(U8("01-PZLE-ZELDA1")),
		GCI_BANNER_TEST(U8("01-PZLE-ZELDA2")),
		//GCI_BANNER_TEST(U8("51-GTKE-Save Game0")),
		GCI_BANNER_TEST(U8("52-GTDE-SK5sbltitgaSK5sbltitga")),
		GCI_BANNER_TEST(U8("52-GTDE-SK5sirpvsicSK5sirpvsic")),
		GCI_BANNER_TEST(U8("52-GTDE-SK5xwkqsbafSK5xwkqsbaf")),
		//GCI_BANNER_TEST(U8("5D-GE9E-EDEDDNEDDYTHEMIS-EDVENTURES")),
		//GCI_BANNER_TEST(U8("69-GHSE-POTTERCOS")),
		GCI_BANNER_TEST(U8("69-GO7E-BOND")),
		GCI_BANNER_TEST(U8("78-GW3E-__2f__w_mania2002")),
		//GCI_BANNER_TEST(U8("7D-GCBE-CRASHWOC")),
		GCI_BANNER_TEST(U8("7D-GCNE-all")),
		GCI_BANNER_TEST(U8("8P-G2XE-SONIC GEMS_00")),
		GCI_BANNER_TEST(U8("8P-G2XE-SONIC_R")),
		GCI_BANNER_TEST(U8("8P-G2XE-STF.DAT")),
		GCI_BANNER_TEST(U8("8P-G9SE-SONICHEROES_00")),
		GCI_BANNER_TEST(U8("8P-G9SE-SONICHEROES_01")),
		GCI_BANNER_TEST(U8("8P-G9SE-SONICHEROES_02")),
		GCI_BANNER_TEST(U8("8P-GEZE-billyhatcher")),
		GCI_BANNER_TEST(U8("8P-GFZE-f_zero.dat")))
	, ImageDecoderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(GCI_Banner_2, ImageDecoderTest,
	::testing::Values(
		GCI_BANNER_TEST(U8("8P-GM2E-rep0000010000C900002497A48E.dat")),
		GCI_BANNER_TEST(U8("8P-GM2E-super_monkey_ball_2.dat")),
		GCI_BANNER_TEST(U8("8P-GMBE-smkb0058556041f42afb")),
		GCI_BANNER_TEST(U8("8P-GMBE-super_monkey_ball.sys")),
		//GCI_BANNER_TEST(U8("8P-GPUE-Puyo Pop Fever Replay01")),
		//GCI_BANNER_TEST(U8("8P-GPUE-Puyo Pop Fever System")),
		GCI_BANNER_TEST(U8("8P-GSNE-SONIC2B__5f____5f__S01")),
		GCI_BANNER_TEST(U8("8P-GSOE-S_MEGA_SYS")),
		GCI_BANNER_TEST(U8("8P-GUPE-SHADOWTHEHEDGEHOG")),
		GCI_BANNER_TEST(U8("8P-GXEE-SONICRIDERS_GAMEDATA_01")),
		GCI_BANNER_TEST(U8("8P-GXSE-SONICADVENTURE_DX_PLAYRECORD_1")),
		GCI_BANNER_TEST(U8("AF-GNME-NAMCOMUSEUM")),
		GCI_BANNER_TEST(U8("AF-GP2E-PMW2SAVE")),
		GCI_BANNER_TEST(U8("AF-GPME-PACMANFEVER")))
	, ImageDecoderTest::test_case_suffix_generator);

// VMS tests
INSTANTIATE_TEST_SUITE_P(VMS, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("Misc/BIOS002.VMS"),
			U8("Misc/BIOS002.png"),
			RomData::IMG_INT_ICON),
		ImageDecoderTest_mode(
			U8("Misc/SONIC2C.VMS"),
			U8("Misc/SONIC2C.png"),
			RomData::IMG_INT_ICON))
	, ImageDecoderTest::test_case_suffix_generator);

// PSV tests
INSTANTIATE_TEST_SUITE_P(PSV, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			U8("Misc/BASCUS-94228535059524F.PSV.gz"),
			U8("Misc/BASCUS-94228535059524F.png"),
			RomData::IMG_INT_ICON),
		ImageDecoderTest_mode(
			U8("Misc/BASCUS-949003034323235383937.PSV.gz"),
			U8("Misc/BASCUS-949003034323235383937.png"),
			RomData::IMG_INT_ICON))
	, ImageDecoderTest::test_case_suffix_generator);

// NDS tests
// TODO: Use something like GcnFstTest that uses an array of filenames
// to generate tests at runtime instead of compile-time?
#define NDS_ICON_TEST(file) ImageDecoderTest_mode( \
			U8("NDS/") file U8(".header-icon.nds.gz"), \
			U8("NDS/") file U8(".header-icon.png"), \
			RomData::IMG_INT_ICON)

INSTANTIATE_TEST_SUITE_P(NDS, ImageDecoderTest,
	::testing::Values(
		NDS_ICON_TEST(U8("A2DE01")),
		NDS_ICON_TEST(U8("A3YE8P")),
		NDS_ICON_TEST(U8("AIZE01")),
		NDS_ICON_TEST(U8("AKWE01")),
		NDS_ICON_TEST(U8("AMHE01")),
		NDS_ICON_TEST(U8("ANDE01")),
		NDS_ICON_TEST(U8("ANME01")),
		NDS_ICON_TEST(U8("AOSE01")),
		NDS_ICON_TEST(U8("APAE01")),
		NDS_ICON_TEST(U8("ASCE8P")),
		NDS_ICON_TEST(U8("ASME01")),
		NDS_ICON_TEST(U8("ATKE01")),
		NDS_ICON_TEST(U8("AY9E8P")),
		NDS_ICON_TEST(U8("AYWE01")),
		NDS_ICON_TEST(U8("BFUE41")),
		NDS_ICON_TEST(U8("BOOE08")),
		NDS_ICON_TEST(U8("BSLEWR")),
		NDS_ICON_TEST(U8("BXSE8P")),
		NDS_ICON_TEST(U8("CBQEG9")),
		NDS_ICON_TEST(U8("COLE8P")),
		NDS_ICON_TEST(U8("CS3E8P")),
		NDS_ICON_TEST(U8("DMFEA4")),
		NDS_ICON_TEST(U8("DSYESZ")),
		NDS_ICON_TEST(U8("NTRJ01.Tetris-THQ")),
		NDS_ICON_TEST(U8("VSOE8P")),
		NDS_ICON_TEST(U8("YDLE20")),
		NDS_ICON_TEST(U8("YLZE01")),
		NDS_ICON_TEST(U8("YWSE8P")))
	, ImageDecoderTest::test_case_suffix_generator);

// NintendoBadge tests
// TODO: Use something like GcnFstTest that uses an array of filenames
// to generate tests at runtime instead of compile-time?
#define BADGE_IMAGE_ONLY_TEST(file) ImageDecoderTest_mode( \
			U8("Misc/") file U8(".gz"), \
			U8("Misc/") file U8(".image.png"), \
			RomData::IMG_INT_IMAGE)
#define BADGE_ICON_IMAGE_TEST(file) ImageDecoderTest_mode( \
			U8("Misc/") file U8(".gz"), \
			U8("Misc/") file U8(".icon.png"), \
			RomData::IMG_INT_ICON), \
			BADGE_IMAGE_ONLY_TEST(file)
INSTANTIATE_TEST_SUITE_P(NintendoBadge, ImageDecoderTest,
	::testing::Values(
		BADGE_ICON_IMAGE_TEST(U8("MroKrt8.cab")),
		BADGE_IMAGE_ONLY_TEST(U8("MroKrt8_Chara_Luigi000.prb")),
		BADGE_IMAGE_ONLY_TEST(U8("MroKrt8_Chara_Mario000.prb")),
		BADGE_IMAGE_ONLY_TEST(U8("MroKrt8_Chara_Peach000.prb")),
		BADGE_IMAGE_ONLY_TEST(U8("Pr_Animal_12Sc_edit.prb")),
		BADGE_ICON_IMAGE_TEST(U8("Pr_Animal_17Sc_mset.prb")),
		BADGE_ICON_IMAGE_TEST(U8("Pr_FcRemix_2_drM_item05.prb")),
		BADGE_ICON_IMAGE_TEST(U8("Pr_FcRemix_2_punch_char01_3_Sep.prb")))
	, ImageDecoderTest::test_case_suffix_generator);

// SVR tests
// TODO: Use something like GcnFstTest that uses an array of filenames
// to generate tests at runtime instead of compile-time?
// FIXME: Puyo Tools doesn't support format 0x61.
#define SVR_IMAGE_TEST(file) ImageDecoderTest_mode( \
			U8("SVR/") file U8(".svr.gz"), \
			U8("SVR/") file U8(".png"))
INSTANTIATE_TEST_SUITE_P(SVR_1, ImageDecoderTest,
	::testing::Values(
		SVR_IMAGE_TEST(U8("1channel_01")),
		SVR_IMAGE_TEST(U8("1channel_02")),
		SVR_IMAGE_TEST(U8("1channel_03")),
		SVR_IMAGE_TEST(U8("1channel_04")),
		SVR_IMAGE_TEST(U8("1channel_05")),
		SVR_IMAGE_TEST(U8("1channel_06")),
		SVR_IMAGE_TEST(U8("2channel_01")),
		SVR_IMAGE_TEST(U8("2channel_02")),
		SVR_IMAGE_TEST(U8("2channel_03")),
		SVR_IMAGE_TEST(U8("3channel_01")),
		SVR_IMAGE_TEST(U8("3channel_02")),
		SVR_IMAGE_TEST(U8("3channel_03")),
		SVR_IMAGE_TEST(U8("al_egg01")),
		SVR_IMAGE_TEST(U8("ani_han_karada")),
		SVR_IMAGE_TEST(U8("ani_han_parts")),
		SVR_IMAGE_TEST(U8("ani_kao_005")),
		SVR_IMAGE_TEST(U8("c_butterfly_k00")),
		SVR_IMAGE_TEST(U8("c_env_k00")),
		//SVR_IMAGE_TEST(U8("c_env_k01")),
		SVR_IMAGE_TEST(U8("c_env_k02")),
		SVR_IMAGE_TEST(U8("c_env_k04")),
		//SVR_IMAGE_TEST(U8("c_gold_k00")),
		//SVR_IMAGE_TEST(U8("c_green_k00")),
		SVR_IMAGE_TEST(U8("c_green_k01")),
		SVR_IMAGE_TEST(U8("channel_off")),
		SVR_IMAGE_TEST(U8("cha_trt_back01")),
		//SVR_IMAGE_TEST(U8("cha_trt_shadow01")),
		SVR_IMAGE_TEST(U8("cha_trt_water01")),
		SVR_IMAGE_TEST(U8("cha_trt_water02")),
		SVR_IMAGE_TEST(U8("c_paint_aiai")),
		SVR_IMAGE_TEST(U8("c_paint_akira")),
		SVR_IMAGE_TEST(U8("c_paint_amigo")),
		SVR_IMAGE_TEST(U8("c_paint_cake")),
		SVR_IMAGE_TEST(U8("c_paint_chao")),
		SVR_IMAGE_TEST(U8("c_paint_chu")),
		SVR_IMAGE_TEST(U8("c_paint_egg")),
		SVR_IMAGE_TEST(U8("c_paint_flower")),
		SVR_IMAGE_TEST(U8("c_paint_nights")),
		SVR_IMAGE_TEST(U8("c_paint_puyo")),
		SVR_IMAGE_TEST(U8("c_paint_shadow")),
		SVR_IMAGE_TEST(U8("c_paint_soccer")),
		SVR_IMAGE_TEST(U8("c_paint_sonic")),
		SVR_IMAGE_TEST(U8("c_paint_taxi")),
		SVR_IMAGE_TEST(U8("c_paint_urara")),
		SVR_IMAGE_TEST(U8("c_paint_zombie")),
		SVR_IMAGE_TEST(U8("c_pumpkinhead")),
		SVR_IMAGE_TEST(U8("cs_book_02")),
		//SVR_IMAGE_TEST(U8("c_silver_k00")),
		SVR_IMAGE_TEST(U8("c_toy_k00")))
		//SVR_IMAGE_TEST(U8("c_toy_k01")))
	, ImageDecoderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(SVR_2, ImageDecoderTest,
	::testing::Values(
		SVR_IMAGE_TEST(U8("c_toy_k02")),
		SVR_IMAGE_TEST(U8("c_toy_k03")),
		SVR_IMAGE_TEST(U8("c_toy_k04")),
		//SVR_IMAGE_TEST(U8("c_tree_k00")),
		//SVR_IMAGE_TEST(U8("c_tree_k01")),
		//SVR_IMAGE_TEST(U8("eft_drop_t01")),
		//SVR_IMAGE_TEST(U8("eft_ripple01")),
		//SVR_IMAGE_TEST(U8("eft_splash_t01")),
		SVR_IMAGE_TEST(U8("kin_blbaketu2")),
		SVR_IMAGE_TEST(U8("kin_blbaketu")),
		SVR_IMAGE_TEST(U8("kin_blcarpet")),
		SVR_IMAGE_TEST(U8("kin_bldai")),
		SVR_IMAGE_TEST(U8("kin_bldesk1")),
		SVR_IMAGE_TEST(U8("kin_blfire")),
		SVR_IMAGE_TEST(U8("kin_blhako1")),
		SVR_IMAGE_TEST(U8("kin_blhako2")),
		SVR_IMAGE_TEST(U8("kin_blhako3")),
		SVR_IMAGE_TEST(U8("kin_blhako4")),
		SVR_IMAGE_TEST(U8("kin_blhako5")),
		SVR_IMAGE_TEST(U8("kin_blhako6")),
		SVR_IMAGE_TEST(U8("kin_blhouki2")),
		SVR_IMAGE_TEST(U8("kin_blhouki3")),
		SVR_IMAGE_TEST(U8("kin_blhouki4")),
		SVR_IMAGE_TEST(U8("kin_blhouki5")),
		SVR_IMAGE_TEST(U8("kin_blhouki")),
		SVR_IMAGE_TEST(U8("kin_blkabekake1")),
		SVR_IMAGE_TEST(U8("kin_blkabekake2")),
		SVR_IMAGE_TEST(U8("kin_blkasa")),
		SVR_IMAGE_TEST(U8("kin_blkumo2")),
		SVR_IMAGE_TEST(U8("kin_blkumo")),
		SVR_IMAGE_TEST(U8("kin_bllamp1")),
		SVR_IMAGE_TEST(U8("kin_bllamp2")),
		SVR_IMAGE_TEST(U8("kin_bllamp3")),
		SVR_IMAGE_TEST(U8("kin_blmaf")),
		SVR_IMAGE_TEST(U8("kin_blmask")),
		SVR_IMAGE_TEST(U8("kin_blphot")),
		SVR_IMAGE_TEST(U8("kin_blregi2")),
		SVR_IMAGE_TEST(U8("kin_blregi")),
		SVR_IMAGE_TEST(U8("kin_blreji3")),
		SVR_IMAGE_TEST(U8("kin_blreji4")),
		SVR_IMAGE_TEST(U8("kin_blreji5")),
		SVR_IMAGE_TEST(U8("kin_blspeeker2")),
		SVR_IMAGE_TEST(U8("kin_blspeeker3")),
		SVR_IMAGE_TEST(U8("kin_blspeeker")),
		SVR_IMAGE_TEST(U8("kin_blvase1")),
		SVR_IMAGE_TEST(U8("kin_blvase2")),
		SVR_IMAGE_TEST(U8("kin_blwall2")),
		SVR_IMAGE_TEST(U8("kin_blwall")),
		SVR_IMAGE_TEST(U8("kin_cbdoor2")),
		SVR_IMAGE_TEST(U8("kin_copdwaku2")))
	, ImageDecoderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(SVR_3, ImageDecoderTest,
	::testing::Values(
		SVR_IMAGE_TEST(U8("kin_doglass1")),
		SVR_IMAGE_TEST(U8("kin_dolabel2")),
		SVR_IMAGE_TEST(U8("kin_dolabel3")),
		SVR_IMAGE_TEST(U8("kin_kagebkaku")),
		SVR_IMAGE_TEST(U8("kin_kagebmaru")),
		SVR_IMAGE_TEST(U8("kin_kobookstand")),
		SVR_IMAGE_TEST(U8("kin_koedbook2")),
		SVR_IMAGE_TEST(U8("kin_kohibook2")),
		SVR_IMAGE_TEST(U8("kin_kohibook3")),
		SVR_IMAGE_TEST(U8("kin_kokami")),
		SVR_IMAGE_TEST(U8("kin_kophotstand1")),
		SVR_IMAGE_TEST(U8("kin_koribbon")),
		SVR_IMAGE_TEST(U8("trt_g00_gass")),
		SVR_IMAGE_TEST(U8("trt_g00_lawn00small")),
		SVR_IMAGE_TEST(U8("trt_g00_river01")),
		SVR_IMAGE_TEST(U8("trt_g00_river02")))
	, ImageDecoderTest::test_case_suffix_generator);

// NOTE: DidjTex files aren't gzipped because the texture data is
// internally compressed using zlib.
#define DidjTex_IMAGE_TEST(file) ImageDecoderTest_mode( \
			U8("DidjTex/") file U8(".tex"), \
			U8("DidjTex/") file U8(".png"))
INSTANTIATE_TEST_SUITE_P(DidjTex, ImageDecoderTest,
	::testing::Values(
		DidjTex_IMAGE_TEST(U8("LeftArrow")),
		DidjTex_IMAGE_TEST(U8("LightOff")),
		DidjTex_IMAGE_TEST(U8("Slider")),
		DidjTex_IMAGE_TEST(U8("StaticTVImage")),
		DidjTex_IMAGE_TEST(U8("Zone1Act1Icon_Alpha")),
		DidjTex_IMAGE_TEST(U8("Zone1Act1Icon")))
	, ImageDecoderTest::test_case_suffix_generator);

#ifdef ENABLE_PVRTC
// PowerVR3 tests
#define PowerVR3_IMAGE_TEST(file) ImageDecoderTest_mode( \
			U8("PowerVR3/") file U8(".pvr.gz"), \
			U8("PowerVR3/") file U8(".pvr.png"))
INSTANTIATE_TEST_SUITE_P(PowerVR3, ImageDecoderTest,
	::testing::Values(
		//PowerVR3_IMAGE_TEST(U8("brdfLUT")),				// TODO: R16fG16f
		//PowerVR3_IMAGE_TEST(U8("GnomeHorde-bigMushroom_texture")),	// FIXME: Failing (PVRTC-I 4bpp RGB)
		//PowerVR3_IMAGE_TEST(U8("GnomeHorde-fern")),			// FIXME: Failing (PVRTC-I 4bpp RGBA)
		PowerVR3_IMAGE_TEST(U8("Navigation3D-font")),
		//PowerVR3_IMAGE_TEST(U8("Navigation3D-Road")),			// FIXME: Failing (LA88)
		//PowerVR3_IMAGE_TEST(U8("Satyr-Table")),				// FIXME: Failing (RGBA8888)
		PowerVR3_IMAGE_TEST(U8("text-fri"))				// 32x16, caused rp_image::flip(FLIP_V) to break
		)
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_PVRTC */

// TGA tests
#define TGA_IMAGE_TEST(file) ImageDecoderTest_mode( \
			U8("TGA/") file U8(".tga.gz"), \
			U8("TGA/") file U8(".png"))
INSTANTIATE_TEST_SUITE_P(TGA, ImageDecoderTest,
	::testing::Values(
		// Reference images.
		ImageDecoderTest_mode(U8("TGA/TGA_1_CM24_IM8.tga.gz"),	U8("CI8-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_1_CM32_IM8.tga.gz"),	U8("CI8a-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_2_24.tga.gz"),	U8("rgb-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_2_32.tga.gz"),	U8("argb-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_3_8.tga.gz"),		U8("gray-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_9_CM24_IM8.tga.gz"),	U8("CI8-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_9_CM32_IM8.tga.gz"),	U8("CI8a-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_10_24.tga.gz"),	U8("rgb-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_10_32.tga.gz"),	U8("argb-reference.png")),
		ImageDecoderTest_mode(U8("TGA/TGA_11_8.tga.gz"),	U8("gray-reference.png")),

		// TGA 2.0 conformance test suite
		// FIXME: utc16, utc32 have incorrect alpha values?
		// Both gimp and imagemagick interpret them as completely transparent.
		TGA_IMAGE_TEST(U8("conformance/cbw8")),
		TGA_IMAGE_TEST(U8("conformance/ccm8")),
		TGA_IMAGE_TEST(U8("conformance/ctc24")),
		TGA_IMAGE_TEST(U8("conformance/ubw8")),
		TGA_IMAGE_TEST(U8("conformance/ucm8")),
		//TGA_IMAGE_TEST(U8("conformance/utc16")),
		TGA_IMAGE_TEST(U8("conformance/utc24")),
		//TGA_IMAGE_TEST(U8("conformance/utc32")),

		// Test images from tga-go
		// https://github.com/ftrvxmtrx/tga
		// FIXME: Some incorrect alpha values...
		// NOTE: The rgb24/rgb32 colormap images use .1.png; others use .0.png.
		//ImageDecoderTest_mode(U8("TGA/tga-go/ctc16.tga.gz"), U8("TGA/tga-go/color.png"),
		ImageDecoderTest_mode(U8("TGA/tga-go/monochrome8_bottom_left_rle.tga.gz"),	U8("TGA/tga-go/monochrome8.png")),
		ImageDecoderTest_mode(U8("TGA/tga-go/monochrome8_bottom_left.tga.gz"),		U8("TGA/tga-go/monochrome8.png")),
		//ImageDecoderTest_mode(U8("TGA/tga-go/monochrome16_bottom_left_rle.tga.gz"),	U8("TGA/tga-go/monochrome16.png")),
		//ImageDecoderTest_mode(U8("TGA/tga-go/monochrome16_bottom_left.tga.gz"),		U8("TGA/tga-go/monochrome16.png")),
		ImageDecoderTest_mode(U8("TGA/tga-go/rgb24_bottom_left_rle.tga.gz"),		U8("TGA/tga-go/rgb24.0.png")),
		ImageDecoderTest_mode(U8("TGA/tga-go/rgb24_top_left_colormap.tga.gz"),		U8("TGA/tga-go/rgb24.1.png")),
		ImageDecoderTest_mode(U8("TGA/tga-go/rgb24_top_left.tga.gz"),			U8("TGA/tga-go/rgb24.0.png")),
		ImageDecoderTest_mode(U8("TGA/tga-go/rgb32_bottom_left.tga.gz"),		U8("TGA/tga-go/rgb32.0.png")),
		//ImageDecoderTest_mode(U8("TGA/tga-go/rgb32_top_left_rle_colormap.tga.gz"),	U8("TGA/tga-go/rgb32.1.png")),
		ImageDecoderTest_mode(U8("TGA/tga-go/rgb32_top_left_rle.tga.gz"),		U8("TGA/tga-go/rgb32.0.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// Godot STEX3 tests
INSTANTIATE_TEST_SUITE_P(STEX3, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(U8("STEX3/argb.BPTC_RGBA.stex.gz"),	U8("STEX3/argb.BPTC_RGBA.png")),
		ImageDecoderTest_mode(U8("STEX3/argb.DXT5.stex.gz"),		U8("STEX3/argb.DXT5.png")),
		ImageDecoderTest_mode(U8("STEX3/argb.ETC2_RGBA8.stex.gz"),	U8("STEX3/argb.ETC2_RGBA8.png")),
		ImageDecoderTest_mode(U8("STEX3/argb.PVRTC1_4A.stex.gz"),	U8("STEX3/argb.PVRTC1_4A.png")),
		ImageDecoderTest_mode(U8("STEX3/argb.RGBA4444.stex.gz"),	U8("STEX3/argb.RGBA4444.png")),
		ImageDecoderTest_mode(U8("STEX3/argb.RGBA8.stex.gz"),		U8("argb-reference.png")),

		ImageDecoderTest_mode(U8("STEX3/rgb.BPTC_RGBA.stex.gz"),	U8("STEX3/rgb.BPTC_RGBA.png")),
		ImageDecoderTest_mode(U8("STEX3/rgb.DXT1.stex.gz"),		U8("STEX3/rgb.DXT1.png")),
		ImageDecoderTest_mode(U8("STEX3/rgb.ETC2_RGB8.stex.gz"),	U8("STEX3/rgb.ETC2_RGB8.png")),
		ImageDecoderTest_mode(U8("STEX3/rgb.ETC.stex.gz"),		U8("STEX3/rgb.ETC.png")),
		ImageDecoderTest_mode(U8("STEX3/rgb.PVRTC1_4.stex.gz"),		U8("STEX3/rgb.PVRTC1_4.png")),
		ImageDecoderTest_mode(U8("STEX3/rgb.RGB8.stex.gz"),		U8("rgb-reference.png")),

		ImageDecoderTest_mode(U8("STEX3/gray.BPTC_RGBA.stex.gz"),	U8("STEX3/gray.BPTC_RGBA.png")),
		ImageDecoderTest_mode(U8("STEX3/gray.DXT1.stex.gz"),		U8("STEX3/gray.DXT1.png")),
		ImageDecoderTest_mode(U8("STEX3/gray.ETC2_RGBA8.stex.gz"),	U8("STEX3/gray.ETC2_RGBA8.png")),
		ImageDecoderTest_mode(U8("STEX3/gray.ETC.stex.gz"),		U8("STEX3/gray.ETC.png")),
		ImageDecoderTest_mode(U8("STEX3/gray.PVRTC1_4.stex.gz"),	U8("STEX3/gray.PVRTC1_4.png")),
		ImageDecoderTest_mode(U8("STEX3/gray.L8.stex.gz"),		U8("gray-reference.png")),

		ImageDecoderTest_mode(U8("STEX3/TEST_RR_areaMap-bg.tga-RGBE9995.stex.gz"),
		                      U8("STEX3/TEST_RR_areaMap-bg.tga-RGBE9995.png")))
	, ImageDecoderTest::test_case_suffix_generator);

// Godot STEX4 tests
// NOTE: Godot 4 uses different encoders for DXTn and ETCn,
// so the decompressed images will not match STEX3.
INSTANTIATE_TEST_SUITE_P(STEX4, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(U8("STEX4/argb.DXT5.stex.gz"),		U8("STEX4/argb.DXT5.png")),
		ImageDecoderTest_mode(U8("STEX4/argb.ETC2_RGBA8.stex.gz"),	U8("STEX4/argb.ETC2_RGBA8.png")),
		ImageDecoderTest_mode(U8("STEX4/argb.RGBA8.stex.gz"),		U8("argb-reference.png")),

		// Godot 4 encodes rgb-reference.png using DXT5 instead of DXT1 for some reason.
		ImageDecoderTest_mode(U8("STEX4/rgb.DXT5.stex.gz"),		U8("STEX4/rgb.DXT5.png")),
		ImageDecoderTest_mode(U8("STEX4/rgb.ETC2_RGB8.stex.gz"),	U8("STEX4/rgb.ETC2_RGB8.png")),
		ImageDecoderTest_mode(U8("STEX4/rgb.RGB8.stex.gz"),		U8("rgb-reference.png")),

		ImageDecoderTest_mode(U8("STEX4/gray.DXT1.stex.gz"),		U8("STEX4/gray.DXT1.png")),
		ImageDecoderTest_mode(U8("STEX4/gray.ETC.stex.gz"),		U8("STEX4/gray.ETC.png")),
		ImageDecoderTest_mode(U8("STEX4/gray.L8.stex.gz"),		U8("gray-reference.png")))
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

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
