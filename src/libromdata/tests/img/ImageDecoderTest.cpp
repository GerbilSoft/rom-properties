/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * ImageDecoderTest.cpp: ImageDecoder class test.                          *
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

#include "config.librpbase.h"

// Google Test
#include "gtest/gtest.h"

// zlib and libpng
#include <zlib.h>

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

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/uvector.h"
#include "librpbase/TextFuncs.hpp"

#include "librpbase/file/RpFile.hpp"
#include "librpbase/file/RpMemFile.hpp"
#include "librpbase/file/FileSystem.hpp"
#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/RpImageLoader.hpp"
#include "librpbase/img/ImageDecoder.hpp"
using namespace LibRpBase;

// TODO: Separate out the actual DDS texture loader
// from the RomData subclass?
#include "Texture/DirectDrawSurface.hpp"
#include "Texture/SegaPVR.hpp"
#include "Texture/KhronosKTX.hpp"
#include "Texture/ValveVTF.hpp"

// DirectDraw Surface structs.
#include "Texture/dds_structs.h"

// C includes.
#include <stdint.h>
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cctype>
#include <cstring>

// C++ includes.
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
	string dds_gz_filename;	// Source texture to test.
	string png_filename;	// PNG image for comparison.
	bool s3tc;		// Enable S3TC.

	ImageDecoderTest_mode(
		const char *dds_gz_filename,
		const char *png_filename,
#ifdef ENABLE_S3TC
		bool s3tc = true
#else /* !ENABLE_S3TC */
		bool s3tc = false
#endif /* ENABLE_S3TC */
		)
		: dds_gz_filename(dds_gz_filename)
		, png_filename(png_filename)
		, s3tc(s3tc)
	{ }

	// May be required for MSVC 2010?
	ImageDecoderTest_mode(const ImageDecoderTest_mode &other)
		: dds_gz_filename(other.dds_gz_filename)
		, png_filename(other.png_filename)
		, s3tc(other.s3tc)
	{ }

	// Required for MSVC 2010.
	ImageDecoderTest_mode &operator=(const ImageDecoderTest_mode &other)
	{
		dds_gz_filename = other.dds_gz_filename;
		png_filename = other.png_filename;
		s3tc = other.s3tc;
		return *this;
	}
};

// Maximum file size for images.
static const int64_t MAX_DDS_IMAGE_FILESIZE = 2*1024*1024;
static const int64_t MAX_PNG_IMAGE_FILESIZE =    512*1024;

class ImageDecoderTest : public ::testing::TestWithParam<ImageDecoderTest_mode>
{
	protected:
		ImageDecoderTest()
			: ::testing::TestWithParam<ImageDecoderTest_mode>()
			, m_gzDds(nullptr)
			, m_f_dds(nullptr)
			, m_romData(nullptr)
		{ }

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
		static const unsigned int BENCHMARK_ITERATIONS = 100000;

	public:
		// Image buffers.
		ao::uvector<uint8_t> m_dds_buf;
		ao::uvector<uint8_t> m_png_buf;

		// gzip file handle for .dds.gz.
		// Placed here so it can be freed by TearDown() if necessary.
		gzFile m_gzDds;

		// RomData class pointer for .dds.gz.
		// Placed here so it can be freed by TearDown() if necessary.
		// The underlying RpMemFile is here as well, since we can't
		// delete it before deleting the RomData object.
		RpMemFile *m_f_dds;
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
		static inline void replace_slashes(std::string &path);
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
inline void ImageDecoderTest::replace_slashes(std::string &path)
{
#ifdef _WIN32
	for (size_t n = 0; n < path.size(); n++) {
		if (path[n] == '/') {
			path[n] = '\\';
		}
	}
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

#ifdef ENABLE_S3TC
	// Enable/disable S3TC.
	ImageDecoder::EnableS3TC = mode.s3tc;
#else /* !ENABLE_S3TC */
	// Can't test S3TC in this build.
	ASSERT_FALSE(mode.s3tc) << "Cannot test S3TC compression in this build. Recompile with -DENABLE_S3TC";
#endif /* ENABLE_S3TC */

	// Open the gzipped DDS texture file being tested.
	string path = "ImageDecoder_data";
	path += DIR_SEP_CHR;
	path += mode.dds_gz_filename;
	replace_slashes(path);
	m_gzDds = gzopen(path.c_str(), "rb");
	ASSERT_TRUE(m_gzDds != nullptr) << "gzopen() failed to open the DDS file: "
		<< mode.dds_gz_filename;

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

	ASSERT_GT(ddsSize, 4+sizeof(DDS_HEADER))
		<< "DDS test image is too small.";
	ASSERT_LE(ddsSize, MAX_DDS_IMAGE_FILESIZE)
		<< "DDS test image is too big.";

	// Read the DDS image into memory.
	m_dds_buf.resize(ddsSize);
	ASSERT_EQ((size_t)ddsSize, m_dds_buf.size());
	int sz = gzread(m_gzDds, m_dds_buf.data(), ddsSize);
	gzclose_r(m_gzDds);
	m_gzDds = nullptr;

	ASSERT_EQ(ddsSize, (uint32_t)sz) << "Error loading DDS image file: " <<
		mode.dds_gz_filename << " - short read";

	// Open the PNG image file being tested.
	path.resize(18);	// Back to "ImageDecoder_data/".
	path += mode.png_filename;
	replace_slashes(path);
	unique_ptr<IRpFile> file(new RpFile(path, RpFile::FM_OPEN_READ));
	ASSERT_TRUE(file.get() != nullptr);
	ASSERT_TRUE(file->isOpen()) << "Error loading PNG image file: " <<
		mode.png_filename << " - " << strerror(file->lastError());

	// Maximum image size.
	ASSERT_LE(file->size(), MAX_PNG_IMAGE_FILESIZE) << "PNG test image is too big.";

	// Read the PNG image into memory.
	const size_t pngSize = (size_t)file->size();
	m_png_buf.resize(pngSize);
	ASSERT_EQ(pngSize, m_png_buf.size());
	size_t readSize = file->read(m_png_buf.data(), pngSize);
	ASSERT_EQ(pngSize, readSize) << "Error loading PNG image file: " <<
		mode.png_filename << " - short read";
}

/**
 * TearDown() function.
 * Run after each test.
 */
void ImageDecoderTest::TearDown(void)
{
	if (m_romData) {
		m_romData->unref();
		m_romData = nullptr;
	}

	delete m_f_dds;
	m_f_dds = nullptr;

	if (m_gzDds) {
		gzclose_r(m_gzDds);
		m_gzDds = nullptr;
	}
}

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
	unique_ptr<rp_image> tmpImg_expected;
	unique_ptr<rp_image> tmpImg_actual;

	switch (pImgExpected->format()) {
		case rp_image::FORMAT_ARGB32:
			// No conversion needed.
			break;

		case rp_image::FORMAT_CI8:
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
		case rp_image::FORMAT_ARGB32:
			// No conversion needed.
			break;

		case rp_image::FORMAT_CI8:
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
	const uint8_t *pBitsExpected = static_cast<const uint8_t*>(pImgExpected->bits());
	const uint8_t *pBitsActual   = static_cast<const uint8_t*>(pImgActual->bits());
	const int row_bytes = pImgExpected->row_bytes();
	const int stride_expected = pImgExpected->stride();
	const int stride_actual   = pImgActual->stride();
	for (unsigned int y = (unsigned int)pImgExpected->height(); y > 0; y--) {
		ASSERT_EQ(0, memcmp(pBitsExpected, pBitsActual, row_bytes)) <<
			"Decoded image does not match the expected PNG image.";
		pBitsExpected += stride_expected;
		pBitsActual   += stride_actual;
	}
}

/**
 * Run an ImageDecoder test.
 */
TEST_P(ImageDecoderTest, decodeTest)
{
	// Parameterized test.
	const ImageDecoderTest_mode &mode = GetParam();

	// Load the PNG image.
	unique_ptr<RpMemFile> f_png(new RpMemFile(m_png_buf.data(), m_png_buf.size()));
	ASSERT_TRUE(f_png->isOpen()) << "Could not create RpMemFile for the PNG image.";
	unique_ptr<rp_image> img_png(RpImageLoader::load(f_png.get()));
	ASSERT_TRUE(img_png != nullptr) << "Could not load the PNG image as rp_image.";
	ASSERT_TRUE(img_png->isValid()) << "Could not load the PNG image as rp_image.";

	// Open the image as an IRpFile.
	m_f_dds = new RpMemFile(m_dds_buf.data(), m_dds_buf.size());
	ASSERT_TRUE(m_f_dds->isOpen()) << "Could not create RpMemFile for the DDS image.";

	// Determine the image type by checking the last 7 characters of the filename.
	ASSERT_GT(mode.dds_gz_filename.size(), 7U);
	if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".dds.gz")) {
		// DDS image.
		m_romData = new DirectDrawSurface(m_f_dds);
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".pvr.gz") ||
		   !mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".gvr.gz")) {
		// PVR/GVR image.
		m_romData = new SegaPVR(m_f_dds);
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".ktx.gz")) {
		// Khronos KTX image.
		// TODO: Use .zktx format instead of .ktx.gz.
		// Needs GzFile, a gzip-decompressing IRpFile subclass.
		m_romData = new KhronosKTX(m_f_dds);
	} else if (!mode.dds_gz_filename.compare(mode.dds_gz_filename.size()-7, 7, ".vtf.gz")) {
		// Valve Texture File.
		m_romData = new ValveVTF(m_f_dds);
	} else {
		ASSERT_TRUE(false) << "Unknown image type.";
	}
	ASSERT_TRUE(m_romData->isValid()) << "Could not load the DDS image.";
	ASSERT_TRUE(m_romData->isOpen()) << "Could not load the DDS image.";

	// Get the DDS image as an rp_image.
	const rp_image *const img_dds = m_romData->image(RomData::IMG_INT_IMAGE);
	ASSERT_TRUE(img_dds != nullptr) << "Could not load the DDS image as rp_image.";

	// Compare the image data.
	ASSERT_NO_FATAL_FAILURE(Compare_RpImage(img_png.get(), img_dds));
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
	for (int i = (int)suffix.size()-1; i >= 0; i--) {
		char chr = suffix[i];
		if (!isalnum(chr) && chr != '_') {
			suffix[i] = '_';
		}
	}

	// TODO: Convert to ASCII?
	return suffix;
}

// Test cases.

#ifdef ENABLE_S3TC
// DirectDrawSurface tests. (S3TC)
INSTANTIATE_TEST_CASE_P(DDS_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"S3TC/dxt1-rgb.dds.gz",
			"S3TC/dxt1-rgb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/dxt2-rgb.dds.gz",
			"S3TC/dxt2-rgb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/dxt2-argb.dds.gz",
			"S3TC/dxt2-argb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/dxt3-rgb.dds.gz",
			"S3TC/dxt3-rgb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/dxt3-argb.dds.gz",
			"S3TC/dxt3-argb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/dxt4-rgb.dds.gz",
			"S3TC/dxt4-rgb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/dxt4-argb.dds.gz",
			"S3TC/dxt4-argb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/dxt5-rgb.dds.gz",
			"S3TC/dxt5-rgb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/dxt5-argb.dds.gz",
			"S3TC/dxt5-argb.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/bc4.dds.gz",
			"S3TC/bc4.s3tc.png"),
		ImageDecoderTest_mode(
			"S3TC/bc5.dds.gz",
			"S3TC/bc5.s3tc.png"))
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_S3TC */

// DirectDrawSurface tests. (S2TC)
INSTANTIATE_TEST_CASE_P(DDS_S2TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"S3TC/dxt1-rgb.dds.gz",
			"S3TC/dxt1-rgb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/dxt2-rgb.dds.gz",
			"S3TC/dxt2-rgb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/dxt2-argb.dds.gz",
			"S3TC/dxt2-argb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/dxt3-rgb.dds.gz",
			"S3TC/dxt3-rgb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/dxt3-argb.dds.gz",
			"S3TC/dxt3-argb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/dxt4-rgb.dds.gz",
			"S3TC/dxt4-rgb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/dxt4-argb.dds.gz",
			"S3TC/dxt4-argb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/dxt5-rgb.dds.gz",
			"S3TC/dxt5-rgb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/dxt5-argb.dds.gz",
			"S3TC/dxt5-argb.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/bc4.dds.gz",
			"S3TC/bc4.s2tc.png", false),
		ImageDecoderTest_mode(
			"S3TC/bc5.dds.gz",
			"S3TC/bc5.s2tc.png", false))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests. (Uncompressed 16-bit RGB)
INSTANTIATE_TEST_CASE_P(DDS_RGB16, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"RGB/RGB565.dds.gz",
			"RGB/RGB565.png"),
		ImageDecoderTest_mode(
			"RGB/xRGB4444.dds.gz",
			"RGB/xRGB4444.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests. (Uncompressed 16-bit ARGB)
INSTANTIATE_TEST_CASE_P(DDS_ARGB16, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"ARGB/ARGB1555.dds.gz",
			"ARGB/ARGB1555.png"),
		ImageDecoderTest_mode(
			"ARGB/ARGB4444.dds.gz",
			"ARGB/ARGB4444.png"),
		ImageDecoderTest_mode(
			"ARGB/ARGB8332.dds.gz",
			"ARGB/ARGB8332.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests. (Uncompressed 15-bit RGB)
INSTANTIATE_TEST_CASE_P(DDS_RGB15, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"RGB/RGB565.dds.gz",
			"RGB/RGB565.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests. (Uncompressed 24-bit RGB)
INSTANTIATE_TEST_CASE_P(DDS_RGB24, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"RGB/RGB888.dds.gz",
			"RGB/RGB888.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests. (Uncompressed 32-bit RGB)
INSTANTIATE_TEST_CASE_P(DDS_RGB32, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"RGB/xRGB8888.dds.gz",
			"RGB/xRGB8888.png"),
		ImageDecoderTest_mode(
			"RGB/xBGR8888.dds.gz",
			"RGB/xBGR8888.png"),

		// Uncommon formats.
		ImageDecoderTest_mode(
			"RGB/G16R16.dds.gz",
			"RGB/G16R16.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests. (Uncompressed 32-bit ARGB)
INSTANTIATE_TEST_CASE_P(DDS_ARGB32, ImageDecoderTest,
	::testing::Values(
		// 32-bit
		ImageDecoderTest_mode(
			"ARGB/ARGB8888.dds.gz",
			"ARGB/ARGB8888.png"),
		ImageDecoderTest_mode(
			"ARGB/ABGR8888.dds.gz",
			"ARGB/ABGR8888.png"),

		// Uncommon formats.
		ImageDecoderTest_mode(
			"ARGB/A2R10G10B10.dds.gz",
			"ARGB/A2R10G10B10.png"),
		ImageDecoderTest_mode(
			"ARGB/A2B10G10R10.dds.gz",
			"ARGB/A2B10G10R10.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests. (Luminance)
INSTANTIATE_TEST_CASE_P(DDS_Luma, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"Luma/L8.dds.gz",
			"Luma/L8.png"),
		ImageDecoderTest_mode(
			"Luma/A4L4.dds.gz",
			"Luma/A4L4.png"),
		ImageDecoderTest_mode(
			"Luma/L16.dds.gz",
			"Luma/L16.png"),
		ImageDecoderTest_mode(
			"Luma/A8L8.dds.gz",
			"Luma/A8L8.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// DirectDrawSurface tests. (Alpha)
INSTANTIATE_TEST_CASE_P(DDS_Alpha, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"Alpha/A8.dds.gz",
			"Alpha/A8.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests. (square twiddled)
INSTANTIATE_TEST_CASE_P(PVR_SqTwiddled, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"PVR/bg_00.pvr.gz",
			"PVR/bg_00.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests. (VQ)
INSTANTIATE_TEST_CASE_P(PVR_VQ, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"PVR/mr_128k_huti.pvr.gz",
			"PVR/mr_128k_huti.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// PVR tests. (Small VQ)
INSTANTIATE_TEST_CASE_P(PVR_SmallVQ, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"PVR/drumfuta1.pvr.gz",
			"PVR/drumfuta1.png"),
		ImageDecoderTest_mode(
			"PVR/drum_ref.pvr.gz",
			"PVR/drum_ref.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// GVR tests. (RGB5A3)
INSTANTIATE_TEST_CASE_P(GVR_RGB5A3, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"GVR/zanki_sonic.gvr.gz",
			"GVR/zanki_sonic.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// GVR tests. (DXT1, S3TC)
INSTANTIATE_TEST_CASE_P(GVR_DXT1_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"GVR/paldam_off.gvr.gz",
			"GVR/paldam_off.s3tc.png"),
		ImageDecoderTest_mode(
			"GVR/paldam_on.gvr.gz",
			"GVR/paldam_on.s3tc.png"),
		ImageDecoderTest_mode(
			"GVR/weeklytitle.gvr.gz",
			"GVR/weeklytitle.s3tc.png"))
	, ImageDecoderTest::test_case_suffix_generator);

// GVR tests. (DXT1, S2TC)
INSTANTIATE_TEST_CASE_P(GVR_DXT1_S2TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"GVR/paldam_off.gvr.gz",
			"GVR/paldam_off.s2tc.png", false),
		ImageDecoderTest_mode(
			"GVR/paldam_on.gvr.gz",
			"GVR/paldam_on.s2tc.png", false),
		ImageDecoderTest_mode(
			"GVR/weeklytitle.gvr.gz",
			"GVR/weeklytitle.s2tc.png", false))
	, ImageDecoderTest::test_case_suffix_generator);

// KTX tests.
INSTANTIATE_TEST_CASE_P(KTX, ImageDecoderTest,
	::testing::Values(
		// RGB reference image.
		ImageDecoderTest_mode(
			"KTX/rgb-reference.ktx.gz",
			"KTX/rgb.png"),
		// RGB reference image, mipmap levels == 0
		ImageDecoderTest_mode(
			"KTX/rgb-amg-reference.ktx.gz",
			"KTX/rgb.png"),
		// Orientation: Up (upside-down compared to "normal")
		ImageDecoderTest_mode(
			"KTX/up-reference.ktx.gz",
			"KTX/up.png"),
		// Orientation: Down (same as "normal")
		ImageDecoderTest_mode(
			"KTX/down-reference.ktx.gz",
			"KTX/up.png"),

		// Luminance (unsized: GL_LUMINANCE)
		ImageDecoderTest_mode(
			"KTX/luminance_unsized_reference.ktx.gz",
			"KTX/luminance.png"),
		// Luminance (sized: GL_LUMINANCE8)
		ImageDecoderTest_mode(
			"KTX/luminance_sized_reference.ktx.gz",
			"KTX/luminance.png"),

		// ETC1
		ImageDecoderTest_mode(
			"KTX/etc1.ktx.gz",
			"KTX/etc1.png"),

		// ETC2
		ImageDecoderTest_mode(
			"KTX/etc2-rgb.ktx.gz",
			"KTX/etc2-rgb.png"),
		ImageDecoderTest_mode(
			"KTX/etc2-rgba1.ktx.gz",
			"KTX/etc2-rgba1.png"),
		ImageDecoderTest_mode(
			"KTX/etc2-rgba8.ktx.gz",
			"KTX/etc2-rgba8.png"),

		// BGR888 (Hi Corp)
		ImageDecoderTest_mode(
			"KTX/hi_mark.ktx.gz",
			"KTX/hi_mark.png"),
		ImageDecoderTest_mode(
			"KTX/hi_mark_sq.ktx.gz",
			"KTX/hi_mark_sq.png"),

		// RGBA reference image.
		ImageDecoderTest_mode(
			"KTX/rgba-reference.ktx.gz",
			"KTX/rgba.png"))

	, ImageDecoderTest::test_case_suffix_generator);

// Valve VTF tests. (all formats)
INSTANTIATE_TEST_CASE_P(VTF, ImageDecoderTest,
	::testing::Values(
		// NOTE: VTF channel ordering is usually backwards from ImageDecoder.

		// 32-bit ARGB
		ImageDecoderTest_mode(
			"VTF/ABGR8888.vtf.gz",
			"argb-reference.png"),
		ImageDecoderTest_mode(
			"VTF/ARGB8888.vtf.gz",	// NOTE: Actually RABG8888.
			"argb-reference.png"),
		ImageDecoderTest_mode(
			"VTF/BGRA8888.vtf.gz",
			"argb-reference.png"),
		ImageDecoderTest_mode(
			"VTF/RGBA8888.vtf.gz",
			"argb-reference.png"),

		// 32-bit xRGB
		ImageDecoderTest_mode(
			"VTF/BGRx8888.vtf.gz",
			"rgb-reference.png"),

		// 24-bit RGB
		ImageDecoderTest_mode(
			"VTF/BGR888.vtf.gz",
			"rgb-reference.png"),
		ImageDecoderTest_mode(
			"VTF/RGB888.vtf.gz",
			"rgb-reference.png"),

		// 24-bit RGB + bluescreen
		ImageDecoderTest_mode(
			"VTF/BGR888_bluescreen.vtf.gz",
			"VTF/BGR888_bluescreen.png"),
		ImageDecoderTest_mode(
			"VTF/RGB888_bluescreen.vtf.gz",
			"VTF/BGR888_bluescreen.png"),

		// 16-bit RGB (565)
		// FIXME: Tests are failing.
		ImageDecoderTest_mode(
			"VTF/BGR565.vtf.gz",
			"RGB/RGB565.png"),
		ImageDecoderTest_mode(
			"VTF/RGB565.vtf.gz",
			"RGB/RGB565.png"),

		// 15-bit RGB (555)
		ImageDecoderTest_mode(
			"VTF/BGRx5551.vtf.gz",
			"RGB/RGB555.png"),

		// 16-bit ARGB (4444)
		ImageDecoderTest_mode(
			"VTF/BGRA4444.vtf.gz",
			"ARGB/ARGB4444.png"),

		// UV88 (handled as RG88)
		ImageDecoderTest_mode(
			"VTF/UV88.vtf.gz",
			"rg-reference.png"),

		// Intensity formats
		ImageDecoderTest_mode(
			"VTF/I8.vtf.gz",
			"Luma/L8.png"),
		ImageDecoderTest_mode(
			"VTF/IA88.vtf.gz",
			"Luma/A8L8.png"),

		// Alpha format (A8)
		ImageDecoderTest_mode(
			"VTF/A8.vtf.gz",
			"Alpha/A8.png"))
	, ImageDecoderTest::test_case_suffix_generator);

#ifdef ENABLE_S3TC
// Valve VTF tests. (S3TC)
INSTANTIATE_TEST_CASE_P(VTF_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"VTF/DXT1.vtf.gz",
			"VTF/DXT1.s3tc.png"),
		ImageDecoderTest_mode(
			"VTF/DXT1_A1.vtf.gz",
			"VTF/DXT1_A1.s3tc.png"),
		ImageDecoderTest_mode(
			"VTF/DXT3.vtf.gz",
			"VTF/DXT3.s3tc.png"),
		ImageDecoderTest_mode(
			"VTF/DXT5.vtf.gz",
			"VTF/DXT5.s3tc.png"))
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_S3TC */

// Valve VTF tests. (S2TC)
INSTANTIATE_TEST_CASE_P(VTF_S2TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"VTF/DXT1.vtf.gz",
			"VTF/DXT1.s2tc.png", false),
		ImageDecoderTest_mode(
			"VTF/DXT1_A1.vtf.gz",
			"VTF/DXT1_A1.s2tc.png", false),
		ImageDecoderTest_mode(
			"VTF/DXT3.vtf.gz",
			"VTF/DXT3.s2tc.png", false),
		ImageDecoderTest_mode(
			"VTF/DXT5.vtf.gz",
			"VTF/DXT5.s2tc.png", false))
	, ImageDecoderTest::test_case_suffix_generator);

// Test images from texture-compressor.
// Reference: https://github.com/TimvanScherpenzeel/texture-compressor
INSTANTIATE_TEST_CASE_P(TCtest, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"tctest/example-etc1.ktx.gz",
			"tctest/example-etc1.ktx.png"),
		ImageDecoderTest_mode(
			"tctest/example-etc2.ktx.gz",
			"tctest/example-etc2.ktx.png"))
	, ImageDecoderTest::test_case_suffix_generator);

#ifdef ENABLE_S3TC
// texture-compressor tests. (S3TC)
INSTANTIATE_TEST_CASE_P(TCtest_S3TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"tctest/example-dxt1.dds.gz",
			"tctest/example-dxt1.s3tc.dds.png"),
		ImageDecoderTest_mode(
			"tctest/example-dxt3.dds.gz",
			"tctest/example-dxt5.s3tc.dds.png"),
		ImageDecoderTest_mode(
			"tctest/example-dxt5.dds.gz",
			"tctest/example-dxt5.s3tc.dds.png"))
	, ImageDecoderTest::test_case_suffix_generator);
#endif /* ENABLE_S3TC */

// texture-compressor tests. (S2TC)
INSTANTIATE_TEST_CASE_P(TCtest_S2TC, ImageDecoderTest,
	::testing::Values(
		ImageDecoderTest_mode(
			"tctest/example-dxt1.dds.gz",
			"tctest/example-dxt1.s2tc.dds.png", false),
		ImageDecoderTest_mode(
			"tctest/example-dxt3.dds.gz",
			"tctest/example-dxt5.s2tc.dds.png", false),
		ImageDecoderTest_mode(
			"tctest/example-dxt5.dds.gz",
			"tctest/example-dxt5.s2tc.dds.png", false))
	, ImageDecoderTest::test_case_suffix_generator);

} }

/**
 * Test suite main function.
 * Called by gtest_init.c.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: ImageDecoder tests.\n\n");
	fflush(nullptr);

	// Make sure the CRC32 table is initialized.
	get_crc_table();

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
