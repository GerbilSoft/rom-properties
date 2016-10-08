/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * GcnFstTest.cpp: GameCube/Wii FST test.                                  *
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

// MiniZip
#include "unzip.h"
#ifdef _WIN32
#include "iowin32.h"
#endif

// libromdata
#include "TextFuncs.hpp"
#include "uvector.h"
#include "disc/GcnFst.hpp"
#ifdef _WIN32
#include "RpWin32.hpp"
#endif
using LibRomData::GcnFst;

// C includes. (C++ namespace)
#include <cctype>

// C++ includes.
#include <sstream>
#include <string>
#include <unordered_set>
using std::string;
using std::unordered_set;

namespace LibRomData { namespace Tests {

struct GcnFstTest_mode
{
	std::string fst_filename;	// FST filename in the FST Zip file.
	uint8_t offsetShift;		// File offset shift. (0 == GCN, 2 == Wii)

	GcnFstTest_mode(
		const char *fst_filename,
		uint8_t offsetShift)
		: fst_filename(fst_filename)
		, offsetShift(offsetShift)
	{ }
};

/**
 * Formatting function for GcnFstTest_mode.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const GcnFstTest_mode& mode) {
        return os << mode.fst_filename;
};

// Maximum file size for FST files.
static const int64_t MAX_GCN_FST_FILESIZE = 1*1024*1024;

class GcnFstTest : public ::testing::TestWithParam<GcnFstTest_mode>
{
	protected:
		GcnFstTest()
			: ::testing::TestWithParam<GcnFstTest_mode>()
			, m_fst(nullptr)
		{ }

		virtual void SetUp(void) override;
		virtual void TearDown(void) override;

		/**
		 * Open a Zip file for reading.
		 * @param filename Zip filename.
		 * @return Zip file, or nullptr on error.
		 */
		static unzFile openZip(const rp_char *filename);

		/**
		 * Get a file from a Zip file.
		 * @param zip_filename	[in] Zip filename.
		 * @param int_filename	[in] Internal filename.
		 * @param buf		[out] uvector buffer for the data.
		 * @return Number of bytes read, or negative on error.
		 */
		static int getFileFromZip(const rp_char *filename, const char *int_filename, ao::uvector<uint8_t>& buf);

	public:
		// FST data.
		ao::uvector<uint8_t> m_fst_buf;
		IFst *m_fst;

		/**
		 * Recursively check a subdirectory for duplicate filenames.
		 * @param subdir Subdirectory path.
		 */
		void checkNoDuplicateFilenames(const rp_char *subdir);

	public:
		/** Test case parameters. **/

		/**
		 
		 * Get the list of FST files from a Zip file.
		 * @param offsetShift File offset shift. (0 == GCN, 2 == Wii)
		 * @return FST files.
		 */
		static std::vector<GcnFstTest_mode> ReadTestCasesFromDisk(uint8_t offsetShift);

		/**
		 * Test case suffix generator.
		 * @param info Test parameter information.
		 * @return Test case suffix.
		 */
		static string test_case_suffix_generator(::testing::TestParamInfo<GcnFstTest_mode> info);
};

/**
 * SetUp() function.
 * Run before each test.
 */
void GcnFstTest::SetUp(void)
{
	// Parameterized test.
	const GcnFstTest_mode &mode = GetParam();

	// Open the Zip file.
	const rp_char *zip_filename;
	switch (mode.offsetShift) {
		case 0:
			zip_filename = _RP("GameCube.fst.zip");
			break;
		case 2:
			zip_filename = _RP("Wii.fst.zip");
			break;
		default:
			ASSERT_TRUE(false) << "offsetShift is " << (int)mode.offsetShift << "; should be either 0 or 2.";
	}

	ASSERT_GT(getFileFromZip(zip_filename, mode.fst_filename.c_str(), m_fst_buf), 0);

	// Create the GcnFst object.
	m_fst = new GcnFst(m_fst_buf.data(), (uint32_t)m_fst_buf.size(), mode.offsetShift);
}

/**
 * TearDown() function.
 * Run after each test.
 */
void GcnFstTest::TearDown(void)
{
	delete m_fst;
	m_fst = nullptr;
}

/**
 * Open a Zip file for reading.
 * @param filename Zip filename.
 * @return Zip file, or nullptr on error.
 */
unzFile GcnFstTest::openZip(const rp_char *filename)
{
#ifdef _WIN32
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64W(&ffunc);
	return unzOpen2_64(RP2W_c(filename), &ffunc);
#else /* !_WIN32 */
#ifdef RP_UTF8
	return unzOpen(filename);
#else /* RP_UTF16 */
	return unzOpen(rp_string_to_utf8(filename, rp_strlen(filename)));
#endif
#endif /* _WIN32 */
}

/**
 * Get a file from a Zip file.
 * @param zip_filename	[in] Zip filename.
 * @param int_filename	[in] Internal filename.
 * @param buf		[out] uvector buffer for the data.
 * @return Number of bytes read, or negative on error.
 */
int GcnFstTest::getFileFromZip(const rp_char *zip_filename, const char *int_filename, ao::uvector<uint8_t>& buf)
{
	// Open the Zip file.
	unzFile unz = openZip(zip_filename);
	EXPECT_TRUE(unz != nullptr) <<
		"Could not open '" << rp_string_to_utf8(zip_filename) << "', check the test directory!";
	if (!unz) {
		return -1;
	}

	// Locate the required FST file.
	// TODO: Always case-insensitive? (currently using OS-dependent value)
	int ret = unzLocateFile(unz, int_filename, 0);
	EXPECT_EQ(UNZ_OK, ret);
	if (ret != UNZ_OK) {
		unzClose(unz);
		return -2;
	}

	// Verify the FST file size.
	unz_file_info64 file_info;
	ret = unzGetCurrentFileInfo64(unz,
		&file_info,
		nullptr, 0,
		nullptr, 0,
		nullptr, 0);
	EXPECT_EQ(UNZ_OK, ret);
	if (ret != UNZ_OK) {
		unzClose(unz);
		return -3;
	}
	EXPECT_LE(file_info.uncompressed_size, (uLong)MAX_GCN_FST_FILESIZE) <<
		"Compressed file '" << int_filename << "' is too big.";
	if (file_info.uncompressed_size > (uLong)MAX_GCN_FST_FILESIZE) {
		unzClose(unz);
		return -4;
	}

	// Open the FST file.
	ret = unzOpenCurrentFile(unz);
	EXPECT_EQ(UNZ_OK, ret);
	if (ret != UNZ_OK) {
		unzClose(unz);
		return -5;
	}

	// Read the FST file.
	buf.resize((size_t)file_info.uncompressed_size);
	ret = unzReadCurrentFile(unz, buf.data(), (unsigned int)buf.size());
	EXPECT_GT(ret, 0);
	if (ret <= 0) {
		unzClose(unz);
		return -6;
	}

	// Close the FST file.
	// An error will occur here if the CRC is incorrect.
	ret = unzCloseCurrentFile(unz);
	EXPECT_EQ(UNZ_OK, ret);
	if (ret != UNZ_OK) {
		unzClose(unz);
		return -7;
	}

	// Close the Zip file.
	unzClose(unz);
	return (int)file_info.uncompressed_size;
}

/**
 * Recursively check a subdirectory for duplicate filenames.
 * @param subdir Subdirectory path.
 */
void GcnFstTest::checkNoDuplicateFilenames(const rp_char *subdir)
{
	unordered_set<rp_string> filenames;
	unordered_set<rp_string> subdirs;

	IFst::Dir *dirp = m_fst->opendir(subdir);
	ASSERT_TRUE(dirp != nullptr) <<
		"Failed to open directory '" << subdir << "'.";

	IFst::DirEnt *dirent = m_fst->readdir(dirp);
	while (dirent != nullptr) {
		// Make sure we haven't seen this filename in
		// the current subdirectory yet.
		EXPECT_TRUE(filenames.find(dirent->name) == filenames.end()) <<
			"Directory '" << rp_string_to_utf8(subdir) << "' has duplicate filename '" << dirent->name << "'.";

		// Filename has been seen now.
		filenames.insert(dirent->name);

		// Is this a subdirectory?
		if (dirent->type == DT_DIR) {
			subdirs.insert(dirent->name);
		}

		// Next entry.
		dirent = m_fst->readdir(dirp);
	}

	// Check subdirectories.
	for (unordered_set<string>::const_iterator iter = subdirs.begin();
	     iter != subdirs.end(); ++iter)
	{
		rp_string path = subdir;
		if (!path.empty() && path[path.size()-1] != '/') {
			path += _RP_CHR('/');
		}
		path += iter->c_str();
		checkNoDuplicateFilenames(path.c_str());
	}

	// End of directory.
	m_fst->closedir(dirp);
}

/**
 * Make sure there aren't any duplicate filenames in all subdirectories.
 */
TEST_P(GcnFstTest, NoDuplicateFilenames)
{
	ASSERT_NO_FATAL_FAILURE(checkNoDuplicateFilenames(_RP("/")));
}

/** Test case parameters. **/

/**
 * Get the list of FST files from a Zip file.
 * @param offsetShift File offset shift. (0 == GCN, 2 == Wii)
 * @return FST files.
 */
std::vector<GcnFstTest_mode> GcnFstTest::ReadTestCasesFromDisk(uint8_t offsetShift)
{
	// NOTE: Cannot use ASSERT_TRUE() here.
	std::vector<GcnFstTest_mode> files;

	// Open the Zip file.
	const rp_char *zip_filename;
	switch (offsetShift) {
		case 0:
			zip_filename = _RP("GameCube.fst.zip");
			break;
		case 2:
			zip_filename = _RP("Wii.fst.zip");
			break;
		default:
			EXPECT_TRUE(false) << "offsetShift is " << (int)offsetShift << "; should be either 0 or 2.";
			return files;
	}

	unzFile unz = openZip(zip_filename);
	EXPECT_TRUE(unz != nullptr) <<
		"Could not open '" << rp_string_to_utf8(zip_filename) << "', check the test directory!";
	if (!unz) {
		return files;
	}

	// Read the filenames.
	int ret = UNZ_OK;
	char filename[256];
	unz_file_info64 file_info;
	do {
		ret = unzGetCurrentFileInfo64(unz,
			&file_info,
			filename, sizeof(filename),
			nullptr, 0,
			nullptr, 0);
		if (ret != UNZ_OK)
			break;

		EXPECT_LE(file_info.uncompressed_size, (uLong)MAX_GCN_FST_FILESIZE) <<
			"GCN FST file '" << filename << "' is too big. (maximum size is 1 MB)";
		if (file_info.uncompressed_size <= MAX_GCN_FST_FILESIZE) {
			// Add this filename to the list.
			GcnFstTest_mode mode(filename, offsetShift);
			files.push_back(mode);
		}

		// Next file.
		ret = unzGoToNextFile(unz);
	} while (ret == UNZ_OK);
	unzClose(unz);

	// Handle "end of file list" as OK.
	if (ret == UNZ_END_OF_LIST_OF_FILE)
		ret = UNZ_OK;

	EXPECT_EQ(UNZ_OK, ret);
	EXPECT_GT(files.size(), 0U);
	return files;
}

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string GcnFstTest::test_case_suffix_generator(::testing::TestParamInfo<GcnFstTest_mode> info)
{
	string suffix = info.param.fst_filename;

	// Replace all non-alphanumeric characters with '_'.
	// See gtest-param-util.h::IsValidParamName().
	for (int i = (int)suffix.size()-1; i >= 0; i--) {
		char chr = suffix[i];
		if (!isalnum(chr) && chr != '_') {
			suffix[i] = '_';
		}
	}

	return suffix;
}

INSTANTIATE_TEST_CASE_P(GameCube, GcnFstTest,
	testing::ValuesIn(GcnFstTest::ReadTestCasesFromDisk(0))
	, GcnFstTest::test_case_suffix_generator);

INSTANTIATE_TEST_CASE_P(Wii, GcnFstTest,
	testing::ValuesIn(GcnFstTest::ReadTestCasesFromDisk(2))
	, GcnFstTest::test_case_suffix_generator);

} }

extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: GcnFst tests.\n\n");
	fflush(nullptr);

	// Make sure the CRC32 table is initialized.
	get_crc_table();

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
